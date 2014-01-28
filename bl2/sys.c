
#include "stm32f4xx_conf.h"
#include "io.h"
#include "sys.h"
#include "stm32f407.h"

#include "usart_drv.h"

#include <string.h>

#define ASSERT(a) { if (!(a)) {io_setpolled(1); io_printf("assert stuck\n");} while (!(a)) ; }

int sys_lev;

struct task {
	char 		*name;            /* 0-3 */
	void		*sp;               /* 4-7 */
	
	struct task 	*next;            /* 8-11 */
	struct task	*next2;		  /* 12-15 */
	int 		state;		  /* 16-19 */
	void		*bp;		  /* 20-23 */ /* communication buffer for sleeping context */
	int		bsize;            /* 24-27 */ /* size of the buffer */
	int		stack_sz;         /* 28-31 */
};


struct task * volatile ready;
struct task * volatile ready_last;


struct task main_task = { "init_main", (void *)0x20020000, 0, 0, 1, 0,0,512 };
struct task * volatile current = &main_task;
struct task *troot =&main_task;

int save_context(unsigned int);
int enter_context(struct task *next);
int switch_context(struct task *next, struct task *prev);


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


struct user_fd {
	struct driver *driver;
	int driver_ix;
};

#define FDTAB_SIZE 16

struct user_fd fd_tab[FDTAB_SIZE];

int get_user_fd(struct driver *d, int driver_ix) {
	int i;
	for(i=0;i<FDTAB_SIZE;i++) {
		if (!fd_tab[i].driver) {
			fd_tab[i].driver=d;
			fd_tab[i].driver_ix=driver_ix;
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






static inline unsigned int  r0() {
	register unsigned int r0 asm("r0");
	return r0;
}
static inline unsigned int  r1() {
	register unsigned int r0 asm("r0");
	__asm__ __volatile__("mov r0,r1"::: "r0");
	return r0;
}
static inline unsigned int  r2() {
	register unsigned int r0 asm("r0");
	__asm__ __volatile__("mov r0,r2"::: "r0");
	return r0;
}
static inline unsigned int  r3() {
	register unsigned int r0 asm("r0");
	__asm__ __volatile__("mov r0,r3"::: "r0");
	return r0;
}
static inline unsigned int  r12() {
	register unsigned int r0 asm("r0");
	__asm__ __volatile__("mov r0,r12"::: "r0");
	return r0;
}
static inline unsigned int  lr() {
	register unsigned int r0 asm("r0");
	__asm__ __volatile__("mov r0,lr"::: "r0");
	return r0;
}
static inline unsigned int  pc() {
	register unsigned int r0 asm("r0");
	__asm__ __volatile__("mov r0,pc"::: "r0");
	return r0;
}


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
			"bl %[PendSV_Handler_c]\n\t"
			"pop {r1-r2}\n\t"
			"mov lr,r1\n\t"
			"cmp	r0,#0\n\t"
			"bne.n	1f\n\t"
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
			"bx lr\n\t"
			:
			: [PendSV_Handler_c] "i" (PendSV_Handler_c)
			: 
	);
}

void *PendSV_Handler_c(unsigned long int *save_sp) {
	struct task *t=ready;	
	if (!ready) {
		ASSERT(0);
	}

	if (current->state==1) {
		current->state=2;
		ready_last->next=current;
		ready_last=current;
	}
	ready=ready->next;
	t->next=0;
	if ((TASK_XPSR(t->sp)&0xff)==0xb) {
		*(save_sp-2)=0xfffffff1;
	}
	return t;
}

void  pendSV(void) {
	*((uint32_t volatile *)0xE000ED04) = 0x10000000; // trigger PendSV
}

struct HardFrame {
	unsigned long int r0;
	unsigned long int r1;
	unsigned long int r2;
	unsigned long int r3;
	unsigned long int r12;
	unsigned long int lr;
	unsigned long int pc;
	unsigned long int psr;
};

__attribute__ ((naked)) int do_pendSV(volatile struct HardFrame *hf, void *nlr) {
	register unsigned long int r0 asm("r0");
	register unsigned long int lr asm("lr");
	hf[1].pc=lr;
	asm volatile ("mov sp,%[hf]\n\t"
		      "mov lr,%[nlr]\n\t"
		      "bx lr\n\t"
			:
			: [hf] "r" (hf), [nlr] "r" (nlr)
			: );
	return r0;
}

int fake_pendSV(void) {
	volatile struct HardFrame hf[2];
	void *nlr=(void *)0xfffffff1;

	hf[1].r0=0xff;
	hf[1].r1=r1();
	hf[1].r2=r2();
	hf[1].r3=r3();
	hf[1].r12=r3();
	hf[1].lr=lr();
	hf[1].psr=xpsr()|0x0100000b;

	hf[0].r0=0;
	hf[0].r1=0;
	hf[0].r2=0;
	hf[0].r3=0;
	hf[0].r12=0;
	hf[0].lr=0xfffffff9;
	hf[0].pc=((unsigned long int)PendSV_Handler)|1;
	hf[0].psr=(xpsr()&~0xff)|0x0100000e;

	if( (*((volatile unsigned int *)0xe000ed24))&0x400) {
		ASSERT(0);
	};
	(*((volatile unsigned int *)0xe000ed24))|=0x400;

	enable_interrupt();

	do_pendSV(hf,nlr);

	asm volatile ("sub sp,sp,0x40\n\t");
	(*((volatile unsigned int *)0xe000ed24))=0x80;
	
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

void fixup_stack(unsigned long int *sp) {

	if((*(sp-2)&0xF)==9) {
		// return to thread
#if 1
		if ((XPSR(sp)&0xff)!=0) {
			BP("fixup stack: returning to thread with nz xpsr\n");
		}
#endif
		if ((xpsr()&0xff)==0xf) {
//			BP("write: returning to thread with bad xpsr, fixup SHSR %x\n",xpsr());
			(*((unsigned int *)0xe000ed24))&=~0x80;
			(*((unsigned int *)0xe000ed24))|=0x800;
		} else if ((xpsr()&0xff)==0xb) {
			(*((unsigned int *)0xe000ed24))&=~0x800;
			(*((unsigned int *)0xe000ed24))|=0x80;
		} else {
			BP("fixup stack: returning to thread with weird xpsr %x, shcsr %x\n",xpsr(), *((unsigned int *)0xe000ed24));
			(*((unsigned int *)0xe000ed24))=0x0;
		}
	} else if ((*(sp-2)&0xF)==1) {
		// return to handler
		if ((XPSR(sp)&0xff)==0) {
			BP("fixup: returning to handler with zero xpsr\n");
		}
		if ((XPSR(sp)&0xff)==0x37) {
			BP("fixup: returning to handler with usart xpsr\n");
		}
		if ((XPSR(sp)&0xff)==(xpsr()&0xff)) {
			XPSR(sp)&=0xffffff00;
			if ((xpsr()&0xff)==0xb) {
				XPSR(sp)|=0xf;
			} else {
				XPSR(sp)|=0xb;
			}
			(*((unsigned int *)0xe000ed24))|=0x880;
		}
	}
}

void SysTick_Handler_c(void *sp) {
	struct tq *tqp;
	tq_tic++;
	tqp=&tq[tq_tic%1024];


	if (tqp->tq_out_first) {
		struct task *t=tqp->tq_out_first;
		t->state=2;
		if (!ready) {
			ready=t;
		} else {
			ready_last->next=t;
		}
		ready_last=tqp->tq_out_last;
//		while(t->next) t=t->next;
		tqp->tq_out_first=tqp->tq_out_last=0;
	}

	if (ready) {
		DEBUGP(DLEV_SCHED,"time slice: switch out %s, switch in %s\n", current->name, ready->name);
		pendSV();
	}
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

int svc_kill_self(void);



void *SVC_Handler_c(unsigned long int *svc_args) {
	unsigned int svc_number;
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
			struct task *t=(struct task *)getSlab_512();
			unsigned long int *stackp;
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
			t->state=2;
			t->name=name;
			t->next2=troot;
			troot=t;

			if(!ready) {
				ready=t;
			} else {
				ready_last->next=t;
			}
			ready_last=t;

			DEBUGP(DLEV_SCHED,"Create task: name %s, switch out %s\n", t->name, ready_last->name);
			pendSV();

			return 0;
			break;
		}
		case SVC_KILL_SELF: {
			struct task *n=0;
			struct task *p=0;
			if (current->state!=1) return 0;

			current->state=6;

			while(!ready);

			DEBUGP(DLEV_SCHED,"kill self  task: name %s, switch in %s\n", p->name, n->name);
			pendSV();
			return 0;
		}
		case SVC_SLEEP: {
			struct tq *tout=&tq[(svc_args[0]+tq_tic)%1024];
			if (current->state!=1) return 0;
			current->state=3;
			if (!tout->tq_out_first) {
				tout->tq_out_first=current;
				tout->tq_out_last=current;
			} else {
				tout->tq_out_last->next=current;
				tout->tq_out_last=current;
			}

			while(!ready);
			DEBUGP(DLEV_SCHED,"sleep task: name %s, switch in %s\n", current->name, ready->name);
			pendSV();
			return 0;
			break;
		}
		case SVC_SLEEP_ON: {
			struct sleep_obj *so=(struct sleep_obj *)svc_args[0];
			void *bp=(void *)svc_args[1];
			int   bsize=svc_args[2];
			struct task *n=0;
			struct task *p=current;

			if (current->state!=1) return 0;
			current->bp=bp;
			current->bsize=bsize;

			current->state=4;
			if (so->task_list) {
				so->task_list_last->next=current;
			} else {
				so->task_list=current;
			}
			so->task_list_last=current;

			DEBUGP(DLEV_SCHED,"sleepon %s task: name %s, switch in %s\n", so->name,p->name,n->name);
			pendSV();
			return 0;
			break;
			
		}
		case SVC_WAKEUP: {
			struct sleep_obj *so=(struct sleep_obj *)svc_args[0];
			void *bp=(void *)svc_args[1];
			int   bsize=svc_args[2];
			struct task *n;

			if (current->state!=1) return 0;

			if (!so->task_list) return 0;
			n=so->task_list;
			if (n->bsize<bsize) return 0;
			so->task_list=so->task_list->next;

			if(ready) {
				ready_last->next=n;
			} else {
				ready=n;
			}
			ready_last=n;

			n->bsize=bsize;
			if (bp&&bsize) {
				__builtin_memcpy(n->bp,bp,bsize);
			} 
			DEBUGP(DLEV_SCHED,"wakeup %s task: name %s, switch in %s\n",so->name,current->name,n->name);
			pendSV();
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
			kfd=drv->ops->open(drv->instance,0);
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
		default:
			break;
	}
	return 0;
}


int __attribute__ (( naked )) save_context(unsigned int val) {
 /*
  */
  
	asm volatile ( 
//			"stmfd sp!, {R15}\n\t"
			"stmfd sp!,{r0-r12,r14}\n\t"
			"ldr r0,=current\n\t"
			"ldr r0,[r0]\n\t"
			"str sp,[r0,#4]\n\t"
			"add sp,sp,#56\n\t"
			"movs r0,#0\n\t"
			"bx lr\n\t"
			:
			:
			: 
	);
	return 0;
}

int __attribute__ (( naked )) enter_context(struct task *next) {
 /*
  */
	asm volatile (  
                        "ldr r1,=current\n\t"
                        "str %[next],[r1,#0]\n\t"
                        "ldr r0,[r0,#4]\n\t"
                        "mov sp,r0\n\t"
                        "ldmfd sp!,{r0-r12,r14}\n\t"
			"movs r0,#1\n\t"
			"bx lr\n\t"
			:
			: [next] "r" (next)
			:
	);

	return 1;
}


int __attribute__ (( naked )) switch_context(struct task *next, struct task *old) {
 /*
  */
  
	asm volatile ( 
			"cpsid i\n\t"
			"stmfd sp!,{r0-r12,r14}\n\t"
			"str sp,[%[old],#4]\n\t"
                        "ldr r2,[%[next],#4]\n\t"
                        "mov sp,r2\n\t"
			"movs r2,#1\n\t"
			"str r2,[%[next],#16]\n\t"
                        "ldmfd sp!,{r0-r12,r14}\n\t"
			"cpsie i\n\t"
			"bx lr\n\t"
			:
			: [next] "r" (next), [old] "r" (old)
			: 
	);
	return 0;
}


void *sys_sleep(unsigned int ms) {
	struct tq *tout=&tq[((ms/10)+tq_tic)%1024];
	int in_irq=xpsr()&0xff;

	if (in_irq&&(in_irq!=0xb)) {
		return 0;
	}

	if (current->state!=1) return 0;
	current->state=3;
	if (!tout->tq_out_first) {
		tout->tq_out_first=current;
		tout->tq_out_last=current;
	} else {
		tout->tq_out_last->next=current;
		tout->tq_out_last=current;
	}

	DEBUGP(DLEV_SCHED,"sys_sleep task: name %s, switch in %s\n", current->name, ready->name);
	pendSV();
	return 0;
}

/* not to be called from irq, cant sleep */
void *sys_sleepon(struct sleep_obj *so, void *bp, int bsize) {
	int in_irq=xpsr()&0xff;
	
	if (in_irq&&(in_irq!=0xb)) {
		return 0;
	}
	if (!ready) return 0;
	
	if (current->state!=1) return 0;
	current->bp=bp;
	current->bsize=bsize;

	current->state=4;


	if (so->task_list) {
		so->task_list_last->next=current;
	} else {
		so->task_list=current;
	}
	so->task_list_last=current;

	DEBUGP(DLEV_SCHED,"sys sleepon %s task: name %s, switch in %s\n", so->name, current->name, ready->name);
	fake_pendSV();
	return 0;
}

void *sys_wakeup(struct sleep_obj *so, void *bp, int bsize) {
	struct task *n;

	n=so->task_list;
	if (!n) return 0;
	if (n->bsize<bsize) return 0;
	so->task_list=so->task_list->next;
	n->next=0;
	n->state=2;

	if (ready) {
		ready_last->next=n;
	} else {
		ready=n;
	}
	ready_last=n;
		
	DEBUGP(DLEV_SCHED,"wakeup(readying) %s task: name %s, switch in %s\n",so->name,current->name,n->name);
	pendSV();
 	return 0;
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
int svc_create_task(void *fnc, void *val, int stacksz, char *name);
int svc_sleep(unsigned int);
int svc_sleep_on(struct sleep_obj *, void *buf, int size);
int svc_wakeup(struct sleep_obj *, void *buf, int size);

int svc_io_open(const char *);
int svc_io_read(int fd, const char *b, int size);
int svc_io_write(int fd, const char *b, int size);
int svc_io_control(int fd, int cmd, void *b, int s);
int svc_io_close(int fd);

#if 0
__attribute__ ((noinline)) int svc_switch_to(void *task) {
	register int rc asm("r0");
	svc(SVC_SWITCH_TO);
	return rc;
}
#endif

__attribute__ ((noinline)) int svc_create_task(void *fnc, void *val, int stacksz, char *name) {
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


int thread_create(void *fnc, void *val, int stacksz, char *name) {
	return svc_create_task(fnc, val, stacksz, name);
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

int help_fnc(int argc, char **argv, struct Env *env) {
	fprintf(env->io_fd, "help called with %d args\n", argc);
	return 0;
}

int get_state(struct task *t) {
	switch(t->state) {
		case 0: return 'i';
		case 1: return 'r';
		case 2: return 'w';
		case 3: return 't';
		case 4: return 'b';
		default: return '?';
	}
}


int ps_fnc(int argc, char **argv, struct Env *env) {
	struct task *t=troot;
	while(t) {
		fprintf(env->io_fd,"task(%x) %12s, sp=0x%08x, pc=0x%08x, state=%c\n", 
			t, t->name, t->sp, (t->state!=1)?((unsigned int *)t->sp)[13]:0xffffffff, get_state(t));
		t=t->next2;
	}
	return 0;
}

int lsdrv_fnc(int argc, char **argv, struct Env *env) {
	struct driver *d=drv_root;
	fprintf(env->io_fd,"=========== Installed drivers =============\n");
	while(d) {
		fprintf(env->io_fd,"%s\n", d->name);
		d=d->next;
	}
	return 0;
}


int debug_fnc(int argc, char **argv, struct Env *env) {
	if (argc>0) {
		dbglev=10;
	} else {
		dbglev=0;
	}
	return 0;
}

int reboot_fnc(int argc, char **argv, struct Env *env) {
	fprintf(env->io_fd, "Rebooting \n\n\n");
	sleep(100);
	NVIC_SystemReset();
	return 0;
}
	
typedef int (*cmdFunc)(int,char **, struct Env *);

struct cmd {
	char *name;
	cmdFunc fnc;
} cmd_root[] = {{"help",help_fnc},
		{"ps",ps_fnc},
		{"lsdrv",lsdrv_fnc},
		{"debug",debug_fnc},
		{"reboot",reboot_fnc},
		{0,0}};

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
		fprintf(fd,"\n--->");
		{
		int rc=io_read(fd,buf,200);
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
	SCB->CCR|=1;
	for(i=init_func_begin;i<init_func_end;i++) {
		((ifunc)*i)();
	}
	
#ifdef DRIVERSUPPORT
	driver_init();
	driver_start();
#endif	
}

void start_sys(void) {
	thread_create(sys_mon,"usart0",256,"sys_mon");
	thread_create(sys_mon,"stterm0",256,"sys_mon");
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
