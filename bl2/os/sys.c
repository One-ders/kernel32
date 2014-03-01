
//#include "stm32f4xx_conf.h"
#include "stm32f4xx.h"
#include "io.h"
#include "sys.h"
//#include "stm32f407.h"

#include <string.h>

#define ASSERT(a) { if (!(a)) {io_setpolled(1); io_printf("assert stuck\n");} while (!(a)) ; }

int sys_lev;
unsigned int sys_irqs;

#define TASK_STATE_IDLE		0
#define TASK_STATE_RUNNING	1
#define TASK_STATE_READY	2
#define TASK_STATE_TIMER	3
#define TASK_STATE_IO		4
#define TASK_STATE_DEAD		6

#define MAX_PRIO		4
#define GET_PRIO(a)		((a)->prio_flags&0x3)
#define SET_PRIO(a,b)		((a)->prio_flags=(b)&0xf)
#define GET_TMARK(a)		((a)->prio_flags&0x10)
#define SET_TMARK(a)		((a)->prio_flags|=0x10)
#define CLR_TMARK(a)		((a)->prio_flags&=0xEF)

struct user_fd;

struct task {
	char 		*name;            /* 0-3 */
	void		*sp;              /* 4-7 */
	
	struct task 	*next;            /* 8-11 */   /* link of task of same state */
	struct task	*next2;		  /* 12-15 */  /* all tasks chain */
	int 		state;		  /* 16-19 */
	int		prio_flags;	  /* 20-23 */
	void		*bp;		  /* 24-27 */ /* communication buffer for sleeping context */
	int		bsize;            /* 28-31 */ /* size of the buffer */
	int		stack_sz;         /* 32-35 */
	struct user_fd  *fd_list;	  /* 36-39 */ /* open driver list */
	unsigned int    active_tics;	  /* 36-39 */
};


struct task * volatile ready[5];
struct task * volatile ready_last[5];

struct task main_task = { "init_main", (void *)0x20020000, 0, 0, 1, 3, 0,0,512 };
struct task * volatile current = &main_task;
struct task *troot =&main_task;

struct Slab_256 {
	unsigned char mem[256];
};

struct Slab_256 slab_256[80];
int free_slab_256=0;

void *getSlab_256(void) {
	if (free_slab_256<80) {
		return &slab_256[free_slab_256++];
	}
	return 0;
}

void *getSlab_512(void) {
	int t=free_slab_256;
	free_slab_256+=3;
	return &slab_256[t];
};


struct task *lookup_task_for_name(char *task_name) {
	struct task *t=troot;
	while(t) {
		if (__builtin_strncmp(t->name,task_name,strlen(t->name)+1)==0) {
			return t;
		}
		t=t->next2;
	}
	return 0;
}

struct user_fd {
	struct driver *driver;
	int driver_ix;
	struct user_fd *next;
};

#define FDTAB_SIZE 16

struct user_fd fd_tab[FDTAB_SIZE];

int get_user_fd(struct driver *d, int driver_ix) {
	int i;
	for(i=0;i<FDTAB_SIZE;i++) {
		if (!fd_tab[i].driver) {
			fd_tab[i].driver=d;
			fd_tab[i].driver_ix=driver_ix;

			fd_tab[i].next=current->fd_list;
			current->fd_list=&fd_tab[i];
			return i;
		}
	}
	return -1;
}

struct tq {
	struct task *tq_out_first;
	struct task *tq_out_last;
} tq[1024];

volatile unsigned int tq_tic;


#define LR(a)	((unsigned int *)a)[5]
#define XPSR(a)	((unsigned int *)a)[7]
#define TASK_XPSR(a)	((unsigned int *)a)[15]

#define BP(a...) {disable_interrupt();io_setpolled(1); io_printf(a); enable_interrupt();io_setpolled(0);}






static inline unsigned int  xpsr() {
	register unsigned int r0 asm("r0");
	__asm__ __volatile__("mrs r0,xpsr"::: "r0");
	return r0;
}

void check_stack(struct task *t,struct task *n) {
#if 1
	unsigned int stack_size=768-sizeof(*t);
	unsigned int stack_base=(t==&main_task)?0x20020000:((unsigned int)t)+768;
	if ((((unsigned int)t->sp)<(stack_base-stack_size))||(((unsigned int)t->sp)>stack_base)) {
		io_setpolled(1);
		io_printf("stack corruption on task %x\n", t);
		ASSERT(0);
	}

#endif
}

/*
 * PendSV handler
 */
void *PendSV_Handler_c(unsigned long int *svc_args);

void __attribute__ (( naked )) PendSV_Handler(void) {
	asm volatile ( "tst lr,#4\n\t"
			"ite eq\n\t"
			"mrseq r0,msp\n\t"
			"mrsne r0,psp\n\t"
			"mov r1,lr\n\t"
			"mov r2,r0\n\t"
			"push {r1-r2}\n\t"
			"cpsid i\n\t"    // I need to run this with irq off, usart driver chrashes all when unplugged
			"bl %[PendSV_Handler_c]\n\t"
			"pop {r1-r2}\n\t"
			"mov lr,r1\n\t"
			"cmp	r0,#0\n\t"
			"bne.n	1f\n\t"
			"cpsie i\n\t"
			"bx lr\n\t"
			"1:\n\t"
			"stmfd r2!,{r4-r11}\n\t"
			"ldr r1,=current\n\t"
			"ldr r1,[r1]\n\t"
			"str r2,[r1,#4]\n\t"
			"ldr r1,=current\n\t"
			"str r0,[r1,#0]\n\t"
			"movs r2,#1\n\t"
			"str r2,[r0,#16]\n\t"
			"ldr r0,[r0,#4]\n\t"
			"mov sp,r0\n\t"
			"ldmfd sp!,{r4-r11}\n\t"
			"cpsie i\n\t"
			"bx lr\n\t"
			:
			: [PendSV_Handler_c] "i" (PendSV_Handler_c)
			: 
	);
}

void *PendSV_Handler_c(unsigned long int *save_sp) {
	int i=0;	
	struct task *t;
	sys_irqs++;
	while(i<MAX_PRIO) {
		t=ready[i];	
		if (t) {
			ready[i]=t->next;
			break;
		}
		i++;
	}
	if (!t) {
		ASSERT(0);
	}
	/* Have next, move away current if still here, timer and blocker moves it themselves */
	if (current->state==TASK_STATE_RUNNING) {
		int prio=current->prio_flags&0xf;
		current->state=TASK_STATE_READY;
		CLR_TMARK(current);
		if (prio>4) prio=4;
		if (ready[prio]) {
			ready_last[prio]->next=current;
		} else {
			ready[prio]=current;
		}
		ready_last[prio]=current;
	}
	t->next=0;
	if ((TASK_XPSR(t->sp)&0x1ff)!=0) {
		*(save_sp-2)=0xfffffff1;
	} else {
		*(save_sp-2)=0xfffffff9;
	}
	return t;
}

void  pendSV(void) {
	*((unsigned int volatile *)0xE000ED04) = 0x10000000; // trigger PendSV
}

__attribute__ ((naked)) int fake_pendSV(void) {
	asm volatile (
			"mrs r0,psr\n\t"
			"orr r0,r0,0x01000000\n\t"
			"stmfd sp!,{r0}\n\t"
			"push {lr}\n\t"
			"stmfd sp!,{r0-r3,r12,lr}\n\t"
			"mvns lr,#0x0e\n\t"
			"cpsie i\n\t"
			"b	PendSV_Handler\n\t"
			:::);
	return 1;
}

#if 1
void SysTick_Handler_c(void *sp);

void __attribute__ (( naked )) SysTick_Handler(void) {
 /*
  * Get the pointer to the stack frame which was saved before the SVC
  * call and use it as first parameter for the C-function (r0)
  * All relevant registers (r0 to r3, r12 (scratch register), r14 or lr
  * (link register), r15 or pc (programm counter) and xPSR (program
  * status register) are saved by hardware.
  */
	asm volatile ( "tst lr,#4\n\t"
			"ite eq\n\t"
			"mrseq r0,msp\n\t"
			"mrsne r0,psp\n\t"
			"mov r1,lr\n\t"
			"mov r2,r0\n\t"
			"push {r1-r2}\n\t"
			"bl %[SysTick_Handler_c]\n\t"
			"pop {r1-r2}\n\t"
			"mov lr,r1\n\t"
			"bx lr\n\t"
			:
			: [SysTick_Handler_c] "i" (SysTick_Handler_c)
			: "r0"
	);
  
}
#endif

void Error_Handler_c(void *sp);

void SysTick_Handler_c(void *sp) {
	struct tq *tqp;
	int p=5;
	current->active_tics++;
	sys_irqs++;
	tq_tic++;
	tqp=&tq[tq_tic%1024];


	if (tqp->tq_out_first) {
		struct task *t=tqp->tq_out_first;
		while(t) {
			struct task *tnext=t->next;
			int prio=t->prio_flags&0xf;
			if (prio>MAX_PRIO) prio=MAX_PRIO;
			t->state=TASK_STATE_READY;
			t->next=0;
			if(prio<p) p=prio;
			if (!ready[prio]) {
				ready[prio]=t;
			} else {
				ready_last[prio]->next=t;
			}
			ready_last[prio]=t;
			DEBUGP(DLEV_SCHED,"timer_wakeup: readying %s\n", t->name);
			t=tnext;
		}
//		while(t->next) t=t->next;
		tqp->tq_out_first=tqp->tq_out_last=0;
	}

	if (p<GET_PRIO(current)) {
		pendSV();
		return;
	}

	if (GET_TMARK(current) && ready[GET_PRIO(current)]) {
		DEBUGP(DLEV_SCHED,"time slice: switch out %s, switch in %s\n", current->name, ready[GET_PRIO(current)]->name);
		pendSV();
		return;
	}
	SET_TMARK(current);
	return;
}



void __attribute__ (( naked )) UsageFault_Handler(void) {
	io_setpolled(1);
	io_printf("in usagefault handler\n");
	ASSERT(0);
}
  
void __attribute__ (( naked )) HardFault_Handler(void) {
	asm volatile ( "tst lr,#4\n\t"
			"ite eq\n\t"
			"mrseq r0,msp\n\t"
			"mrsne r0,psp\n\t"
			"mov r1,lr\n\t"
			"mov r2,r0\n\t"
			"push {r1-r2}\n\t"
			"bl %[Error_Handler_c]\n\t"
			"pop {r1-r2}\n\t"
			"mov lr,r1\n\t"
			"cmp	r0,#0\n\t"
			"bne.n	1f\n\t"
			"bx lr\n\t"
			"1:\n\t"
			"ldr r1,=current\n\t"
			"str r0,[r1,#0]\n\t"
			"ldr r0,[r0,#4]\n\t"
			"mov sp,r0\n\t"
			"ldmfd sp!,{r4-r11}\n\t"
			"bx lr\n\t"
			:
			: [Error_Handler_c] "i" (Error_Handler_c)
			: 
	);
}

/*
 * WWDG_IRQHandler
 */
void __attribute__ (( naked )) WWDG_IRQHandler(void) {
	io_setpolled(1);
	io_printf("in wwdg handler\n");
	ASSERT(0);
  
#if 0
	asm volatile ( "tst lr,#4\n\t"
			"ite eq\n\t"
			"mrseq r0,msp\n\t"
			"mrsne r0,psp\n\t"
			"mov r1,lr\n\t"
			"mov r2,r0\n\t"
			"push {r1-r2}\n\t"
			"bl %[Error_Handler_c]\n\t"
			"pop {r1-r2}\n\t"
			"mov lr,r1\n\t"
			"cmp	r0,#0\n\t"
			"bne.n	1f\n\t"
			"bx lr\n\t"
			"1:\n\t"
			"ldr r1,=current\n\t"
			"str r0,[r1,#0]\n\t"
			"ldr r0,[r0,#4]\n\t"
			"mov sp,r0\n\t"
			"ldmfd sp!,{r4-r11}\n\t"
			"bx lr\n\t"
			:
			: [Error_Handler_c] "i" (Error_Handler_c)
			: 
	);
#endif
}

void Error_Handler_c(void *sp_v) {
	unsigned int *sp=(unsigned int *)sp_v;
	io_setpolled(1);
	io_printf("\nerrorhandler trap, current=%s@0x%08x sp=0x%08x\n",current->name,current,sp);
	io_printf("Value of CFSR register 0x%08x\n", SCB->CFSR);
	io_printf("Value of HFSR register 0x%08x\n", SCB->HFSR);
	io_printf("Value of DFSR register 0x%08x\n", SCB->DFSR);
	io_printf("Value of MMFAR register 0x%08x\n", SCB->MMFAR);
	io_printf("Value of BFAR register 0x%08x\n", SCB->BFAR);
	io_printf("r0 =0x%08x, r1=0x%08x, r2=0x%08x, r3  =0x%08x\n", sp[0], sp[1], sp[2], sp[3]);
	io_printf("r12=0x%08x, LR=0x%08x, PC=0x%08x, xPSR=0x%08x\n", sp[4], sp[5], sp[6], sp[7]);
	
	ASSERT(0);
}

/*
 * Define the interrupt handler for the service calls,
 * It will call the C-function after retrieving the
 * stack of the svc caller.
 */

void *SVC_Handler_c(unsigned long int *sp);

void __attribute__ (( naked )) SVC_Handler(void) {
 /*
  * Get the pointer to the stack frame which was saved before the SVC
  * call and use it as first parameter for the C-function (r0)
  * All relevant registers (r0 to r3, r12 (scratch register), r14 or lr
  * (link register), r15 or pc (programm counter) and xPSR (program
  * status register) are saved by hardware.
  */
  
	asm volatile ( "tst lr,#4\n\t"
			"ite eq\n\t"
			"mrseq r0,msp\n\t"
			"mrsne r0,psp\n\t"
			"mov r1,lr\n\t"
			"mov r2,r0\n\t"
			"push {r1-r2}\n\t"
			"bl %[SVC_Handler_c]\n\t"
			"pop {r1-r2}\n\t"
			"mov lr,r1\n\t"
			"cmp	r0,#0\n\t"
			"bne.n	1f\n\t"
			"bx lr\n\t"
			"1:\n\t"
			"ldr r0,[r0,#4]\n\t"
			"mov sp,r0\n\t"
			"ldmfd sp!,{r4-r11}\n\t"
			"bx lr\n\t"
			:
			: [SVC_Handler_c] "i" (SVC_Handler_c)
			: 
	);
}

#define SVC_CREATE_TASK 1
#define SVC_SLEEP	2
#define SVC_SLEEP_ON    3
#define SVC_WAKEUP      4
#define SVC_IO_OPEN	5
#define SVC_IO_READ	6
#define SVC_IO_WRITE	7
#define SVC_IO_CONTROL	8
#define SVC_IO_CLOSE	9
#define SVC_KILL_SELF  10
#define SVC_BLOCK_TASK	11
#define SVC_UNBLOCK_TASK 12
#define SVC_SETPRIO_TASK 13

int svc_kill_self(void);

int close_drivers(void) {
	struct user_fd *fdl=current->fd_list;

	current->fd_list=0;
	while(fdl) {
		struct user_fd *fdlnext=fdl->next;
		fdl->driver->ops->close(fdl->driver_ix);
		fdl->driver=0;
		fdl->next=0;
		fdl=fdlnext;
	}
	return 0;
}

int detach_driver_fd(struct user_fd *fd) {
	struct user_fd *fdl=current->fd_list;
	struct user_fd **fdprev=&current->fd_list;

	while(fdl) {
		if (fdl==fd) {
			(*fdprev)=fd->next;
			fd->next=0;
			fd->driver=0;
			return 0;
		}
		fdprev=&fdl->next;
		fdl=fdl->next;
	}
	return -1;
}

void *SVC_Handler_c(unsigned long int *svc_args) {
	unsigned int svc_number;

	sys_irqs++;
	/*
	 * Stack contains:
	 * r0, r1, r2, r3, r12, r14, the return address and xPSR
	 * First argument (r0) is svc_args[0]
	 */


	svc_number=((char *)svc_args[6])[-2];
	switch(svc_number) {
		case SVC_CREATE_TASK: {
			unsigned long int fnc=svc_args[0];
			unsigned long int val=svc_args[1];
//			unsigned int stacksz=svc_args[2];
			char *name=(char *)svc_args[3];
			int prio = svc_args[2];
			struct task *t=(struct task *)getSlab_512();
			unsigned long int *stackp;

			if (prio>MAX_PRIO) {
				svc_args[0]=-1;
				return 0;
			}
			__builtin_memset(t,0,768);
			t->sp=(void *)(((unsigned char *)t)+768);
			stackp=(unsigned long int *)t->sp;
			*(--stackp)=0x01000000; 		 // xPSR
			*(--stackp)=(unsigned long int)fnc;    // r15
			*(--stackp)=(unsigned long int)svc_kill_self;     // r14
			*(--stackp)=0;     // r12
			*(--stackp)=0;     // r3
			*(--stackp)=0;     // r2
			*(--stackp)=0;     // r1
			*(--stackp)=(unsigned long int)val;    // r0
////
			*(--stackp)=0;     // r4
			*(--stackp)=0;     // r5
			*(--stackp)=0;     // r6
			*(--stackp)=0;     // r7
			*(--stackp)=0;     // r8
			*(--stackp)=0;     // r9
			*(--stackp)=0;     // r10
			*(--stackp)=0;     // r11

			t->sp=(void *) stackp;
			t->state=TASK_STATE_READY;
			t->name=name;
			SET_PRIO(t,prio);
			t->next2=troot;
			troot=t;

			if(!ready[prio]) {
				ready[prio]=t;
			} else {
				ready_last[prio]->next=t;
			}
			ready_last[prio]=t;

			if (prio<GET_PRIO(current)) {
				DEBUGP(DLEV_SCHED,"Create task: name %s, switch out %s\n", t->name, current->name);
				pendSV();
			} else {
				DEBUGP(DLEV_SCHED,"Create task: name %s\n", t->name);
			}

			svc_args[0]=0;
			return 0;
			break;
		}
		case SVC_KILL_SELF: {
			if (current->state!=TASK_STATE_RUNNING) return 0;

			close_drivers();
			current->state=TASK_STATE_DEAD;

			DEBUGP(DLEV_SCHED,"kill self  task: name %s\n", current->name);
			pendSV();
			return 0;
		}
		case SVC_SLEEP: {
			struct tq *tout=&tq[(svc_args[0]+tq_tic)%1024];
			if (current->state!=TASK_STATE_RUNNING) return 0;
			CLR_TMARK(current);
			current->state=TASK_STATE_TIMER;
			if (!tout->tq_out_first) {
				tout->tq_out_first=current;
				tout->tq_out_last=current;
			} else {
				tout->tq_out_last->next=current;
				tout->tq_out_last=current;
			}

			DEBUGP(DLEV_SCHED,"sleep task: name %s\n", current->name);
			pendSV();
			return 0;
			break;
		}
		case SVC_SLEEP_ON: {
			struct sleep_obj *so=(struct sleep_obj *)svc_args[0];
			void *bp=(void *)svc_args[1];
			int   bsize=svc_args[2];

			if (current->state!=TASK_STATE_RUNNING) return 0;
			current->bp=bp;
			current->bsize=bsize;

			CLR_TMARK(current);
			current->state=TASK_STATE_IO;
			if (so->task_list) {
				so->task_list_last->next=current;
			} else {
				so->task_list=current;
			}
			so->task_list_last=current;

			DEBUGP(DLEV_SCHED,"sleepon %s task: name %s\n", so->name,current->name);
			pendSV();
			return 0;
			break;
			
		}
		case SVC_WAKEUP: {
			struct sleep_obj *so=(struct sleep_obj *)svc_args[0];
			void *bp=(void *)svc_args[1];
			int   bsize=svc_args[2];
			struct task *n;
			int prio;

			if (current->state!=TASK_STATE_RUNNING) return 0;

			if (!so->task_list) return 0;
			n=so->task_list;
			if (n->bsize<bsize) return 0;
			so->task_list=so->task_list->next;

			n->bsize=bsize;
			if (bp&&bsize) {
				__builtin_memcpy(n->bp,bp,bsize);
			} 

			n->state=TASK_STATE_READY;
			n->next=0;
			prio=n->prio_flags&0xf;
			if (prio>4) prio=4;
			if(ready[prio]) {
				ready_last[prio]->next=n;
			} else {
				ready[prio]=n;
			}
			ready_last[prio]=n;

			DEBUGP(DLEV_SCHED,"wakeup %s current task: name %s, readying %s\n",so->name,current->name,n->name);
			if (prio<GET_PRIO(current)) {
				pendSV();
			}
			svc_args[0]=0;
		 	return 0;
			break;
		}
		case SVC_IO_OPEN: {
			char *drvname=(char *)svc_args[0];
			int  kfd;
			struct driver *drv=driver_lookup(drvname);
			if (!drv) {
				svc_args[0]=-1;
				return 0;
			}
			kfd=drv->ops->open(drv->instance,0,0);
			if (kfd<0) {
				svc_args[0]=-1;
				return 0;
			}
			svc_args[0]=get_user_fd(drv,kfd);
			return 0;
		}
		case SVC_IO_READ: {
			int fd=(int)svc_args[0];
			struct driver *driver=fd_tab[fd].driver;
			int kfd=fd_tab[fd].driver_ix;
			if (!driver) {
				svc_args[0]=-1;
				return 0;
			}

			svc_args[0]=driver->ops->control(kfd,RD_CHAR,(void *)svc_args[1],svc_args[2]);
			return 0;
		}
		case SVC_IO_WRITE: {
			int fd=(int)svc_args[0];
			struct driver *driver=fd_tab[fd].driver;
			int kfd=fd_tab[fd].driver_ix;
			if (!driver) {
				svc_args[0]=-1;
				return 0;
			}

			svc_args[0]=driver->ops->control(kfd,WR_CHAR,(void *)svc_args[1],svc_args[2]);
			return 0;
		}
		case SVC_IO_CONTROL: {
			int fd=(int)svc_args[0];
			struct driver *driver=fd_tab[fd].driver;
			int kfd=fd_tab[fd].driver_ix;
			if (!driver) {
				svc_args[0]=-1;
				return 0;
			}

			svc_args[0]=driver->ops->control(kfd,svc_args[1],(void *)svc_args[2],svc_args[3]);
			return 0;
		}
		case SVC_IO_CLOSE: {
			int fd=(int)svc_args[0];
			struct driver *driver=fd_tab[fd].driver;
			int kfd=fd_tab[fd].driver_ix;
			if (!driver) {
				svc_args[0]=-1;
				return 0;
			}

			detach_driver_fd(&fd_tab[fd]);

			svc_args[0]=driver->ops->close(kfd);
			return 0;
		}
		case SVC_BLOCK_TASK: {
			char *name=(char *)svc_args[0];
			struct task *t=lookup_task_for_name(name);
			if (!t) {
				svc_args[0]=-1;
				return 0;
			}
			if (t==current) {
				SET_PRIO(t,t->prio_flags|0x8);
				DEBUGP(DLEV_SCHED,"blocking current task: name %s\n",current->name);
				pendSV();
			} else {
				struct task *p=ready[GET_PRIO(t)];
				struct task * volatile *p_prev=&ready[GET_PRIO(t)];
				SET_PRIO(t,t->prio_flags|0x8);
				while(p) {
					if (p==t) {
						(*p_prev)=p->next;
						p->next=0;
						if (!ready[MAX_PRIO]){
							ready[MAX_PRIO]=p;
						} else {
							ready_last[MAX_PRIO]->next=p;
						}
						ready_last[MAX_PRIO]=p;
						break;
					} 
					p_prev=&p->next;
					p=p->next;
				}
				DEBUGP(DLEV_SCHED,"blocking task: name %s, current %s\n",t->name,current->name);
			}
			svc_args[0]=0;
			return 0;
		}
		case SVC_UNBLOCK_TASK: {
			char *name=(char *)svc_args[0];
			struct task *b=ready[MAX_PRIO];
			struct task * volatile *b_prev=&ready[MAX_PRIO];
			struct task *t=lookup_task_for_name(name);
			int prio;
			if (!t) {
				svc_args[0]=-1;
				return 0;
			}
			prio=t->prio_flags&0xf;
			if (prio>MAX_PRIO) prio=MAX_PRIO;
			if (prio<MAX_PRIO) {
				svc_args[0]=-1;
				return 0;
			}

			while(b) {
				if (b==t) {
					(*b_prev)=b->next;
					goto set_ready;
				}
				b_prev=&b->next;
				b=b->next;
			}
			svc_args[0]=-1;
			return 0;
set_ready:
			DEBUGP(DLEV_SCHED,"unblocking task: name %s, current %s\n",t->name,current->name);
			t->next=0;
			SET_PRIO(t,t->prio_flags&3);
			if(ready[GET_PRIO(t)]) {
				ready_last[GET_PRIO(t)]->next=t;
			} else {
				ready[GET_PRIO(t)]=t;
			}
			ready_last[GET_PRIO(t)]=t;

			svc_args[0]=0;
			return 0;
		}
		case SVC_SETPRIO_TASK: {
			char *name=(char *)svc_args[0];
			int prio=svc_args[1];
			int curr_prio=GET_PRIO(current);
			struct task *t=lookup_task_for_name(name);
			if (!t) {
				svc_args[0]=-1;
				return 0;
			}
			if (prio>MAX_PRIO) {
				svc_args[0]=-2;
				return 0;
			}
			if (t==current) {
				SET_PRIO(t,prio);
				if (curr_prio<prio) { /* degrading prio of current, shall we switch? */
					int i=0;
					while(!ready[i]){
						i++;
						if (i>=prio) break;
					}
					if (i<prio) {
						DEBUGP(DLEV_SCHED,"degrading current prio task: name %s\n",current->name);
						pendSV();
					}
				}
			} else {
				int tprio=((t->prio_flags&0xf)>MAX_PRIO)?MAX_PRIO:(t->prio_flags&0xf);
				struct task *p=ready[tprio];
				struct task * volatile *p_prev=&ready[tprio];
				SET_PRIO(t,prio);
				while(p) {
					if (p==t) {
						(*p_prev)=p->next;
						p->next=0;
						if (!ready[prio]){
							ready[prio]=p;
						} else {
							ready_last[prio]->next=p;
						}
						ready_last[prio]=p;
						goto check_resched;
					} 
					p_prev=&p->next;
					p=p->next;
				}
				DEBUGP(DLEV_SCHED,"setprio task: name %s prio=%d, current %s\n",t->name,t->prio_flags,current->name);
			}
			svc_args[0]=0;
			return 0;
check_resched:
			if (prio<curr_prio) {
				DEBUGP(DLEV_SCHED,"setprio task: name %s prio=%d, current %s, reschedule\n",t->name,t->prio_flags,current->name);
				pendSV();
			}
			svc_args[0]=0;
			return 0;

		}
		default:
			break;
	}
	return 0;
}


void *sys_sleep(unsigned int ms) {
	struct tq *tout=&tq[((ms/10)+tq_tic)%1024];
	int in_irq=xpsr()&0x1ff;

	if (in_irq&&(in_irq!=0xb)) {
		return 0;
	}

	if (current->state!=TASK_STATE_RUNNING) return 0;
	CLR_TMARK(current);
	current->state=TASK_STATE_TIMER;
	if (!tout->tq_out_first) {
		tout->tq_out_first=current;
		tout->tq_out_last=current;
	} else {
		tout->tq_out_last->next=current;
		tout->tq_out_last=current;
	}

	DEBUGP(DLEV_SCHED,"sys_sleep task: name %s\n", current->name);
	pendSV();
	return 0;
}

/* not to be called from irq, cant sleep */
void *sys_sleepon(struct sleep_obj *so, void *bp, int bsize) {
	int in_irq=xpsr()&0x1ff;
	if (in_irq&&(in_irq!=0xb)) {
		return 0;
	}
	
	if (current->state!=TASK_STATE_RUNNING) return 0;
	current->bp=bp;
	current->bsize=bsize;

	CLR_TMARK(current);
	current->state=TASK_STATE_IO;

	if (so->task_list) {
		so->task_list_last->next=current;
	} else {
		so->task_list=current;
	}
	so->task_list_last=current;

	DEBUGP(DLEV_SCHED,"sys sleepon %s task: name %s\n", so->name, current->name);
	fake_pendSV();

	*((volatile unsigned int *)0xe000ed24)|=0x80;
	return 0;
}

void *sys_wakeup(struct sleep_obj *so, void *bp, int bsize) {
	struct task *n;
	int prio;

	n=so->task_list;
	if (!n) return 0;
	if (n->bsize<bsize) return 0;
	so->task_list=so->task_list->next;
	n->next=0;
	n->state=TASK_STATE_READY;
	prio=n->prio_flags&0xf;
	if (prio>MAX_PRIO) prio=MAX_PRIO;

	if (ready[prio]) {
		ready_last[prio]->next=n;
	} else {
		ready[prio]=n;
	}
	ready_last[prio]=n;
	
	if (prio<GET_PRIO(current)) {
		DEBUGP(DLEV_SCHED,"wakeup(readying) %s task: name %s, readying %s\n",so->name,current->name,n->name);
		pendSV();
	}
 	return n;
}

/***********************************************************/
/* DBG                                                     */
/***********************************************************/
#ifdef DEBUG
int dbglev=0;

#endif


/*
 *    Task side library calls
 */

/*
 * Inline assembler helper directive: call SVC with the given immediate
 */
#define svc(code) asm volatile ("svc %[immediate]\n\t"::[immediate] "I" (code))
 
/*
 * Use a normal C function, the compiler will make sure that this is going
 * to be called using the standard C ABI which ends in a correct stack
 * frame for our SVC call
 */

//int svc_switch_to(void *);
int svc_create_task(void *fnc, void *val, int prio, char *name);
int svc_sleep(unsigned int);
int svc_sleep_on(struct sleep_obj *, void *buf, int size);
int svc_wakeup(struct sleep_obj *, void *buf, int size);

int svc_io_open(const char *);
int svc_io_read(int fd, const char *b, int size);
int svc_io_write(int fd, const char *b, int size);
int svc_io_control(int fd, int cmd, void *b, int s);
int svc_io_close(int fd);
int svc_block_task(char *name);
int svc_unblock_task(char *name);
int svc_setprio_task(char *name, int prio);


__attribute__ ((noinline)) int svc_create_task(void *fnc, void *val, int prio, char *name) {
	register int rc asm("r0");
	svc(SVC_CREATE_TASK);
	return rc;
}


__attribute__ ((noinline)) int svc_kill_self() {
	svc(SVC_KILL_SELF);
	return 0;
}


__attribute__ ((noinline)) int svc_sleep(unsigned int ms) {
	register int rc asm("r0");
	svc(SVC_SLEEP); 
	return rc;
}

__attribute__ ((noinline)) int svc_sleep_on(struct sleep_obj *so, void *buf, int bsize) {
	register int rc asm("r0");
	svc(SVC_SLEEP_ON);
	return rc;
}

__attribute__ ((noinline)) int svc_wakeup(struct sleep_obj *so, void *buf, int bsize) {
	register int rc asm("r0");
	svc(SVC_WAKEUP);
	return rc;
}


__attribute__ ((noinline)) int svc_io_open(const char *name) {
	register int rc asm("r0");
	svc(SVC_IO_OPEN);
	return rc;
}

__attribute__ ((noinline)) int svc_io_read(int fd, const char *b, int size) {
	register int rc asm("r0");
	svc(SVC_IO_READ);
	return rc;
}

__attribute__ ((noinline)) int svc_io_write(int fd, const char *b, int size) {
	register int rc asm("r0");
	svc(SVC_IO_WRITE);
	return rc;
}

__attribute__ ((noinline)) int svc_io_control(int fd, int cmd, void *b, int s) {
	register int rc asm("r0");
	svc(SVC_IO_CONTROL);
	return rc;
}

__attribute__ ((noinline)) int svc_io_close(int fd) {
	register int rc asm("r0");
	svc(SVC_IO_CLOSE);
	return rc;
}

__attribute__ ((noinline)) int svc_block_task(char *name) {
	register int rc asm("r0");
	svc(SVC_BLOCK_TASK);
	return rc;
}

__attribute__ ((noinline)) int svc_unblock_task(char *name) {
	register int rc asm("r0");
	svc(SVC_UNBLOCK_TASK);
	return rc;
}

__attribute__ ((noinline)) int svc_setprio_task(char *name, int prio) {
	register int rc asm("r0");
	svc(SVC_SETPRIO_TASK);
	return rc;
}





int thread_create(void *fnc, void *val, int prio, char *name) {
	return svc_create_task(fnc, val, prio, name);
}


int sleep(unsigned int ms) {
	unsigned int tics=ms/10;
	return svc_sleep(tics);
}

int sleep_on(struct sleep_obj *so, void *buf, int bsize) {
	svc_sleep_on(so,buf,bsize);
	return current->bsize;
}

int wakeup(struct sleep_obj *so, void *dbuf, int dsize) {
	return svc_wakeup(so,dbuf,dsize);
}

int block_task(char *name) {
	return svc_block_task(name);
}

int unblock_task(char *name) {
	return svc_unblock_task(name);
}

int setprio_task(char *name,int prio) {
	return svc_setprio_task(name,prio);
}


#ifdef DRIVERSUPPORT

struct driver *drv_root;

int driver_publish(struct driver *drv) {
	struct driver *p=drv_root;
	while(p&&p!=drv) p=p->next;
	if (!p) {
		drv->next=drv_root;	
		drv_root=drv;
	}
	return 0;
}

int driver_init() {
	struct driver *d=drv_root;
	while(d) {
		if (d->ops->init) {
			d->ops->init(d->instance);
		}
		d=d->next;
	}
	return 0;
}

int driver_start() {
	struct driver *d=drv_root;
	while(d) {
		if (d->ops->start) {
			d->ops->start(d->instance);
		}
		d=d->next;
	}
	return 0;
}

int driver_unpublish(struct driver *drv) {
	struct driver *tmp=drv_root;
	struct driver **prevNext=&drv_root;
	
	while(tmp) {
		if (strcmp(drv->name,tmp->name)==0) {
			*prevNext=tmp->next;
			return 0;
		}
		prevNext=&tmp->next;
		tmp=tmp->next;
	}
	return -1;
}

struct driver  *driver_lookup(char *name) {
	struct driver *d=drv_root;
	while(d) {
		if (strcmp(name,d->name)==0) {
			return d;
		}
		d=d->next;
	}
	return 0;
}




/**********************************************************************/
/*  Driver service calls                                              */

int io_open(const char *drvname) {
	return svc_io_open(drvname);
}

int io_read(int fd, char *buf, int size) {
	return svc_io_read(fd,buf,size);
}

int io_write(int fd, const char *buf, int size) {
	return svc_io_write(fd,buf,size);
}

int io_control(int fd, int cmd, void *d, int sz) {
	return svc_io_control(fd,cmd,d,sz);
}

int io_close(int fd) {
	return -1;
}



#endif

#ifdef UNECESS

struct Env {
	int io_fd;
};

static int help_fnc(int argc, char **argv, struct Env *env);

static int get_state(struct task *t) {
	unsigned int state=t->state;
	switch(state) {
		case TASK_STATE_IDLE: return 'i';
		case TASK_STATE_RUNNING: return 'r';
		case TASK_STATE_READY: return 'w';
		case TASK_STATE_TIMER: return 't';
		case TASK_STATE_IO: return 'b';
		default: return '?';
	}
}


static int ps_fnc(int argc, char **argv, struct Env *env) {
	struct task *t=troot;
	fprintf(env->io_fd," sys_irqs %d\n", sys_irqs);
	while(t) {
		fprintf(env->io_fd,"task(%x) %12s, sp=0x%08x, pc=0x%08x, prio=%x, state=%c, atics=%d\n", 
			t, t->name, t->sp, (t->state!=TASK_STATE_RUNNING)?((unsigned int *)t->sp)[13]:0xffffffff, t->prio_flags, get_state(t), t->active_tics);
		t=t->next2;
	}
	return 0;
}

static int lsdrv_fnc(int argc, char **argv, struct Env *env) {
	struct driver *d=drv_root;
	fprintf(env->io_fd,"=========== Installed drivers =============\n");
	while(d) {
		struct task *t=troot;
		fprintf(env->io_fd,"%s\t\t", d->name);
		while(t) {
			struct user_fd *ofd=t->fd_list;
			while(ofd) {
				if (ofd->driver==d) {
					fprintf(env->io_fd,"%s ",t->name);
				}
				ofd=ofd->next;
			}
			t=t->next2;
		}
		d=d->next;
		fprintf(env->io_fd,"\n");
	}
	fprintf(env->io_fd,"=========== End Installed drivers =============\n");
	return 0;
}


static int debug_fnc(int argc, char **argv, struct Env *env) {
	if (argc>0) {
		dbglev=10;
	} else {
		dbglev=0;
	}
	return 0;
}

static int reboot_fnc(int argc, char **argv, struct Env *env) {
	fprintf(env->io_fd, "Rebooting \n\n\n");
	sleep(100);
	NVIC_SystemReset();
	return 0;
}

static int block_fnc(int argc, char **argv, struct Env *env) {
	int rc;
	fprintf(env->io_fd, "blocking %s, ", argv[1]);
	rc=block_task(argv[1]);
	fprintf(env->io_fd, "returned %d\n", rc);
	
	return 0;
}

static int unblock_fnc(int argc, char **argv, struct Env *env) {
	int rc;
	fprintf(env->io_fd, "unblocking %s, ", argv[1]);
	rc=unblock_task(argv[1]);
	fprintf(env->io_fd, "returned %d\n", rc);
	
	return 0;
}

unsigned long int strtoul(char *str, char *endp, int base) {
	char *p=str;
	int num=0;
	int mode;
	unsigned int val=0;

	if (base) {
		mode=base;
	} else {
		if ((p[0]=='0')&&(__builtin_tolower(p[1])=='x')) {
			mode=16;
			p=&p[2];
		} else if (p[0]=='0') {
			mode=8;
			p=&p[1];
		} else {
			mode=10;
		}
	}
	while(*p) {
		switch (*p) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				num=(*p)-'0';
				break;
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
				num=__builtin_tolower(*p)-'a';
				num+=10;
				break;
		}
		val=(val<<mode)|num;
		p++;
	}
	return val;
}

static int setprio_fnc(int argc, char **argv, struct Env *env) {
	int rc;
	int prio=strtoul(argv[2],0,0);
	if (prio>MAX_PRIO) {
		fprintf(env->io_fd,"setprio: prio must be between 0-4\n");
		return 0;
	}
	fprintf(env->io_fd, "set prio of %s to %d", argv[1], prio);
	rc=setprio_task(argv[1],prio);
	fprintf(env->io_fd, "returned %d\n", rc);
	
	return 0;
}




typedef int (*cmdFunc)(int,char **, struct Env *);

static struct cmd {
	char *name;
	cmdFunc fnc;
} cmd_root[] = {{"help",help_fnc},
		{"ps",ps_fnc},
		{"lsdrv",lsdrv_fnc},
		{"debug",debug_fnc},
		{"reboot",reboot_fnc},
		{"block",block_fnc},
		{"unblock",unblock_fnc},
		{"setprio",setprio_fnc},
		{0,0}};

static int help_fnc(int argc, char **argv, struct Env *env) {
	struct cmd *cmd=cmd_root;
	fprintf(env->io_fd, "help called with %d args\n", argc);
	fprintf(env->io_fd, "available commands: ");
	while(cmd->name) {
		fprintf(env->io_fd, "%s, ", cmd->name);
		cmd++;
	}
	fprintf(env->io_fd, "\n");
	return 0;
}

struct cmd *lookup_cmd(char *name) {
	struct cmd *cmd=cmd_root;
	while(cmd->name) {
		if (__builtin_strcmp(cmd->name,name)==0) {
			return cmd;
		}
		cmd++;
	}
	return 0;
}

int argit(char *str, int len, char *argv[16]) {
	int ac=0;
	char *p=str;
	while(1) {
		while((*p)==' ') {
			p++;
			if (p>=(str+len)) {
				return ac;
			}
		}
		argv[ac]=p;
		while((*p)!=' ') {
			p++;
			if (p>=(str+len)) {
				return ac;
			}		
		}
		*p=0;
		p++;
		ac++;
		if (ac>=16) {
			return 0;
		}
	}
	return ac;
}

int argc;
char *argv[16];

void sys_mon(void *dum) {
	char *buf=getSlab_256();
	int fd=io_open((char *)dum);
	struct Env env;
	if (fd<0) return;
	env.io_fd=fd;
	io_write(fd,"Starting sys_mon\n",17);

	while(1) {
		int rc;
		fprintf(fd,"\n--->");
		rc=io_read(fd,buf,200);
		if (rc>0) {
			struct cmd *cmd;
			if (rc>200) {
				rc=200;
			}
			buf[rc]=0;
			fprintf(fd,"\ngot %s from st_term, len %d\n",buf,rc);
			rc=argit(buf,rc,argv);
			if (rc<0) {
				continue;
			}
			argc=rc;
			cmd=lookup_cmd(argv[0]);
			if (!cmd) {
				fprintf(fd,"cmd %s, not found\n", argv[0]);
				continue;
			}
			cmd->fnc(argc,argv,&env);
		}
	}

}

/*******************************************************************************/
/*  Sys support functions and unecessary stuff                                 */
/*                                                                             */

extern unsigned long int init_func_begin[];
extern unsigned long int init_func_end[];
typedef void (*ifunc)(void);

void init_sys(void) {
	unsigned long int *i;
//	*((unsigned long int *)0xe000ed1c)=0xff000000; /* SHPR2, lower svc prio */
	NVIC_SetPriority(PendSV_IRQn,0xf);
	NVIC_SetPriority(SVCall_IRQn,0xe);
	NVIC_SetPriority(SysTick_IRQn,0xd);  /* preemptive tics... */

	SysTick_Config(SystemCoreClock/100); // 10 mS tic is 100/Sec
//	SCB->CCR|=1;
	for(i=init_func_begin;i<init_func_end;i++) {
		((ifunc)*i)();
	}
	
#ifdef DRIVERSUPPORT
	driver_init();
	driver_start();
#endif	
}

void start_sys(void) {
	thread_create(sys_mon,"usart0",3,"sys_mon");
	thread_create(sys_mon,"stterm0",3,"sys_mon");
	thread_create(sys_mon,"usb_serial0",2,"sys_mon");
}

#else

void init_sys(void) {
//	*((unsigned long int *)0xe000ed1c)=0xff000000; /* SHPR2, lower svc prio */
#ifdef DRIVERSUPPORT
	driver_init();
	driver_start();
#endif	
}

void start_sys(void) {
}

#endif
