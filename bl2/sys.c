
#include "stm32f4xx_conf.h"
#include "io.h"
#include "sys.h"
#include "stm32f407.h"

#include "usart_drv.h"

#include <string.h>

#define ASSERT(a) { while (!(a)) ; }

int sys_lev;

struct task {
	char 		*name;            /* 0-3 */
	unsigned int 	sp;               /* 4-7 */
	
	struct task 	*next;            /* 8-11 */
	struct task	*next2;		  /* 12-15 */
	int 		state;
	void		*bp;
	int		bsize;
	int		stack_sz;         /* 16-19 */
};


struct task *ready;
struct task *ready_last;


struct task main_task = { "init_main", 0x20020000, 0, 0, 0, 0,0,512 };
struct task *current = &main_task;
struct task *troot =&main_task;

#if 0
int __attribute__ (( naked )) save_context(void);
int __attribute__ (( naked )) enter_context(struct task *next);
#endif
int save_context(void);
int enter_context(struct task *next);
int switch_context(struct task *next);


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
	free_slab_256+=2;
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


#if 0
void *SysTick_Handler_c(unsigned int *sp);

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
  
#if 0
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
			"ldr r0,[r0,#4]\n\t"
			"mov sp,r0\n\t"
			"ldmfd sp!,{r4-r11}\n\t"
			"bx lr\n\t"
			:
			: [SysTick_Handler_c] "i" (SysTick_Handler_c)
			: "r0"
	);
#endif
}
#endif

void SysTick_Handler(void) {
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
		ASSERT(ready);
//		while(t->next) t=t->next;
		ready_last=tqp->tq_out_last;
		tqp->tq_out_first=tqp->tq_out_last=0;
	}

	if (ready) {
		struct task *n=0;
		if (current->state!=1) return;
		ready_last->next=current;
		ready_last=current;
		current->state=2;

		n=ready;
		ready=ready->next;
		ASSERT(ready);
		n->next=0;
		n->state=1;

		DEBUGP(DLEV_SCHED,"time slice: switch out %s, switch in %s\n", current->name, n->name);
		switch_context(n);
		return;
	}
	return;
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
			"ldr r1,=current\n\t"
			"str r0,[r1,#0]\n\t"
			"ldr r0,[r0,#4]\n\t"
			"mov sp,r0\n\t"
			"ldmfd sp!,{r4-r11}\n\t"
			"bx lr\n\t"
			:
			: [SVC_Handler_c] "i" (SVC_Handler_c)
			: "r0"
	);
}
#if 0
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
			"stmfd r2!,{r4-r11}\n\t"
			"ldr r1,=current\n\t"
			"ldr r1,[r1]\n\t"
			"str r2,[r1,#4]\n\t"
			"ldr r1,=current\n\t"
			"str r0,[r1,#0]\n\t"
			"ldr r0,[r0,#4]\n\t"
			"mov sp,r0\n\t"
			"ldmfd sp!,{r4-r11}\n\t"
			"bx lr\n\t"
			:
			: [SVC_Handler_c] "i" (SVC_Handler_c)
			: "r0"
	);
}

#endif

#define SVC_CREATE_TASK 1
#define SVC_SLEEP	2
#define SVC_SLEEP_ON    3
#define SVC_WAKEUP      4
#define SVC_IO_OPEN	5
#define SVC_IO_READ	6
#define SVC_IO_WRITE	7
#define SVC_IO_CONTROL	8
#define SVC_IO_CLOSE	9

void *SVC_Handler_c(unsigned long int *svc_args) {
	unsigned int svc_number;
	/*
	 * Stack contains:
	 * r0, r1, r2, r3, r12, r14, the return address and xPSR
	 * First argument (r0) is svc_args[0]
	 */

	svc_number=((char *)svc_args[6])[-2];
	switch(svc_number) {
#if 0
		case SVC_SWITCH_TO: {
			struct task *n=(struct task *)svc_args[0];

			current->state=2;
			if(!ready) {
				ready_last=current;
				ready=current;
			} else {
				ready_last->next=current;
				ready_last=current;
			}

			if (save_context()) {
				return 0;
			}
			DEBUGP(DLEV_SCHED,"Create task: name %s, switch out %s\n", n->name, current->name);
			n->next=0;
			n->state=1;
			return n;
			break;
		}
#endif
		case SVC_CREATE_TASK: {
			unsigned int fnc=svc_args[0];
			unsigned int val=svc_args[1];
			unsigned int stacksz=svc_args[2];
			char *name=(char *)svc_args[3];
			struct task *t=(struct task *)getSlab_512();
			unsigned int *stackp;
			__builtin_memset(t,0,512);
			t->sp=((unsigned int)((unsigned char *)t)+512);
			stackp=(unsigned int *)t->sp;
			*(--stackp)=0x01000000; 		 // xPSR
			*(--stackp)=(unsigned int)fnc;    // r15
			*(--stackp)=0;     // r14
			*(--stackp)=0;     // r12
			*(--stackp)=0;     // r3
			*(--stackp)=0;     // r2
			*(--stackp)=0;     // r1
			*(--stackp)=(unsigned int)val;    // r0
////
			*(--stackp)=0;     // r4
			*(--stackp)=0;     // r5
			*(--stackp)=0;     // r6
			*(--stackp)=0;     // r7
			*(--stackp)=0;     // r8
			*(--stackp)=0;     // r9
			*(--stackp)=0;     // r10
			*(--stackp)=0;     // r11

			t->sp=(unsigned int) stackp;
			t->state=0;
			t->name=name;
			t->next2=troot;
			troot=t;

			current->state=2;
			if(!ready) {
				ready_last=current;
				ready=current;
			} else {
				ready_last->next=current;
				ready_last=current;
			}
			ASSERT(ready);

			if (save_context()) {
				return 0;
			}

			DEBUGP(DLEV_SCHED,"Create task: name %s, switch out %s\n", t->name, current->name);
			t->next=0;
			t->state=1;
			return t;
			break;
		}
		case SVC_SLEEP: {
			struct task *n=0;
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

			ASSERT(ready);
			n=ready;
			ready=ready->next;
			n->next=0;
			n->state=1;
			DEBUGP(DLEV_SCHED,"sleep task: name %s, switch in %s\n", current->name, n->name);
			switch_context(n);

			return 0;
			break;
		}
		case SVC_SLEEP_ON: {
			struct sleep_obj *so=(struct sleep_obj *)svc_args[0];
			void *bp=(void *)svc_args[1];
			int   bsize=svc_args[2];
			struct task *n=0;

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

			ASSERT(ready);
			n=ready;
			ready=ready->next;
			n->next=0;
			n->state=1;

			DEBUGP(DLEV_SCHED,"sleepon %s task: name %s, switch in %s\n", so->name,current->name, n->name);
			switch_context(n);
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

			current->state=2;

			if(ready) {
				ready_last->next=current;
			} else {
				ready=current;
			}
			ready_last=current;
			ASSERT(ready);
			n->next=0;
			n->state=1;

			n->bsize=bsize;
			if (bp&&bsize) {
				__builtin_memcpy(n->bp,bp,bsize);
			} 
			DEBUGP(DLEV_SCHED,"wakeup %s task: name %s, switch in %s\n",so->name,n->name,current->name);
			switch_context(n);
		 	return 0;
			break;
		}
		case SVC_IO_OPEN: {
			char *drvname=(char *)svc_args[0];
			struct driver *drv=driver_lookup(drvname);
			int inst;
			if (!drv) {
				svc_args[0]=-1;
				return 0;
			}
			inst=drv->open(drv,0);
			if (inst<0) {
				svc_args[0]=-1;
				return 0;
			}
			svc_args[0]=get_user_fd(drv,inst);
			return 0;
		}
		case SVC_IO_READ: {
			int fd=(int)svc_args[0];
			struct driver *driver=fd_tab[fd].driver;
			int driver_ix=fd_tab[fd].driver_ix;

			svc_args[0]=driver->control(driver_ix,RD_CHAR,(void *)svc_args[1],svc_args[2]);
			return 0;
		}
		case SVC_IO_WRITE: {
			int fd=(int)svc_args[0];
			struct driver *driver=fd_tab[fd].driver;
			int driver_ix=fd_tab[fd].driver_ix;

			svc_args[0]=driver->control(driver_ix,WR_CHAR,(void *)svc_args[1],svc_args[2]);
			return 0;
		}
		default:
			break;
	}
	return 0;
}


int __attribute__ (( naked )) save_context(void) {
 /*
  */
  
	asm volatile ( 
//			"stmfd sp!, {R15}\n\t"
			"stmfd sp!,{r0-r12,r14}\n\t"
			"ldr r0,=current\n\t"
			"ldr r0,[r0]\n\t"
			"str sp,[r0,#4]\n\t"
			"add sp,sp,#56\n"
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


int __attribute__ (( naked )) switch_context(struct task *next) {
 /*
  */
  
	asm volatile ( 
			"stmfd sp!,{r0-r12,r14}\n\t"
			"ldr r1,=current\n\t"
			"ldr r1,[r1]\n\t"
			"str sp,[r1,#4]\n\t"
                        "ldr r1,=current\n\t"
                        "str %[next],[r1,#0]\n\t"
                        "ldr r0,[r0,#4]\n\t"
                        "mov sp,r0\n\t"
                        "ldmfd sp!,{r0-r12,r14}\n\t"
			"bx lr\n\t"
			:
			: [next] "r" (next)
			: 
	);
	return 0;
}



void *sys_sleepon(struct sleep_obj *so, void *bp, int bsize) {
	struct task *n=0;
	
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

	ASSERT(ready);
	n=ready;
	ready=ready->next;
	n->next=0;
	n->state=1;

	DEBUGP(DLEV_SCHED,"sleepon %s task: name %s, switch in %s\n", so->name, current->name, n->name);
	switch_context(n);
	return n;
}

void *sys_wakeup(struct sleep_obj *so, void *bp, int bsize) {
	struct task *n;

	if (!so->task_list) return 0;
	n=so->task_list;
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
	ASSERT(ready);
		
	DEBUGP(DLEV_SCHED,"wakeup(readying) %s task: name %s, switch in %s\n",so->name,n->name,current->name);
#if 0
	current->state=2;

	if(ready) {
		ready_last->next=current;
	} else {
		ready=current;
	}
	ready_last=current;
	n->next=0;
	n->state=1;

	n->bsize=bsize;
	if (bp&&bsize) {
		__builtin_memcpy(n->bp,bp,bsize);
	}
	DEBUGP(DLEV_SCHED,"wakeup %s task: name %s, switch in %s\n",so->name,n->name,current->name);
	switch_context(n);
#endif
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
#if 0
	struct task *t=(struct task *)getSlab_512();
	unsigned int *stackp;
	memset(t,0,512);
	t->sp=((unsigned int)((unsigned char *)t)+512);
	stackp=(unsigned int *)t->sp;
	*(--stackp)=0x01000000; 		 // xPSR
	*(--stackp)=(unsigned int)fnc;    // r15
	*(--stackp)=0;     // r14
	*(--stackp)=0;     // r12
	*(--stackp)=0;     // r3
	*(--stackp)=0;     // r2
	*(--stackp)=0;     // r1
	*(--stackp)=(unsigned int)val;    // r0
////
	*(--stackp)=0;     // r4
	*(--stackp)=0;     // r5
	*(--stackp)=0;     // r6
	*(--stackp)=0;     // r7
	*(--stackp)=0;     // r8
	*(--stackp)=0;     // r9
	*(--stackp)=0;     // r10
	*(--stackp)=0;     // r11

	t->sp=(unsigned int) stackp;
	t->state=0;
	t->name=name;
	t->next2=troot;
	troot=t;
	svc_switch_to(t);
	return 1;
#endif
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
	drv->next=drv_root;	
	drv_root=drv;
	return 0;
}

int driver_init() {
	struct driver *d=drv_root;
	while(d) {
		if (d->init) d->init();
		d=d->next;
	}
	return 0;
}

int driver_start() {
	struct driver *d=drv_root;
	while(d) {
		if (d->start) d->start();
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

struct driver *driver_lookup(char *name) {
	struct driver *d=drv_root;
	while(d) {
		if (strcmp(name,d->name)==0) return d;
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
	return -1;
}

int io_close(int fd) {
	return -1;
}



#endif

#ifdef UNECESS

int help_fnc(int argc, char **argv) {
	io_printf("help called with %d args\n", argc);
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


int ps_fnc(int argc, char **argv) {
	struct task *t=troot;
	while(t) {
		printf("task(%x) %12s, sp=0x%08x, pc=0x%08x, state=%c\n", 
			t, t->name, t->sp, ((unsigned int *)t->sp)[0], get_state(t));
		t=t->next2;
	}
	return 0;
}

int debug_fnc(int argc, char **argv) {
	if (argc>0) {
		dbglev=10;
	} else {
		dbglev=0;
	}
	return 0;
}

int reboot_fnc(int argc, char **argv) {
//	set_reg(AIRC,VECTKEY|SYSRESETREQ);
	NVIC_SystemReset();
	return 0;
}
	
typedef int (*cmdFunc)(int,char **);

struct cmd {
	char *name;
	cmdFunc fnc;
} cmd_root[] = {{"help",help_fnc},
		{"ps",ps_fnc},
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
		ac++;
	}
	return ac;
}

int argc;
char *argv[16];

void sys_mon(void *dum) {
	char *buf=getSlab_256();
	int fd=io_open(USART_DRV);
	io_write(fd,"Starting sys_mon\n",17);

	while(1) {
		printf("\n--->");
		{
		int rc=io_read(fd,buf,256);
		if (rc>0) {
			struct cmd *cmd;
			buf[rc]=0;
			fprintf(fd,"got %s from st_term\n",buf);
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
			cmd->fnc(argc,argv);
		}
		}
	}

}

/*******************************************************************************/
/*  Sys support functions and unecessary stuff                                 */
/*                                                                             */

void init_sys(void) {
#ifdef DRIVERSUPPORT
	driver_init();
	driver_start();
#endif	
}

void start_sys(void) {
	thread_create(sys_mon,0,256,"sys_mon");
}

#else

void init_sys(void) {
#ifdef DRIVERSUPPORT
	driver_ini();
	driver_start();
#endif	
}

void start_sys(void) {
}

#endif
