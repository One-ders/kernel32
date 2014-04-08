/* $FrameWorks: , v1.1 2014/04/07 21:44:00 anders Exp $ */

/*
 * Copyright (c) 2014, Anders Franzen.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @(#)sys.c
 */
#include "stm32f4xx.h"
#include "io.h"
#include "sys.h"

#include <string.h>

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

struct task * volatile ready[5];
struct task * volatile ready_last[5];

struct task main_task = { "init_main", (void *)0x20020000, 0, 0, 1, 3, 0,0,512 };
struct task *troot =&main_task;
struct task * volatile current = &main_task;

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

void *getSlab_768(void) {
	int t=free_slab_256;
	free_slab_256+=3;
	return &slab_256[t];
};

void *getSlab_1024(void) {
	int t=free_slab_256;
	free_slab_256+=4;
	return &slab_256[t];
};


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
//			"bl %[PendSV_Handler_c]\n\t"
			"bl PendSV_Handler_c\n\t"
			"pop {r1-r2}\n\t"
			"mov lr,r1\n\t"
			"cmp	r0,#0\n\t"
			"bne.n	1f\n\t"
			"cpsie i\n\t"
			"bx lr\n\t"
			"1:\n\t"
			"stmfd r2!,{r4-r11}\n\t"
			"ldr r1,.L1\n\t"
			"ldr r1,[r1]\n\t"
			"str r2,[r1,#4]\n\t"
			"ldr r1,.L1\n\t"
			"str r0,[r1,#0]\n\t"
			"movs r2,#1\n\t"
			"str r2,[r0,#16]\n\t"
			"ldr r0,[r0,#4]\n\t"
			"mov sp,r0\n\t"
			"ldmfd sp!,{r4-r11}\n\t"
			"cpsie i\n\t"
			"bx lr\n\t"
			".L1: .word current\n\t"
			:
			:
//			: [PendSV_Handler_c] "i" (PendSV_Handler_c)
//			: [current] "m" (current)
			:
	);
}



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
struct tq {
	struct blocker *tq_out_first;
	struct blocker *tq_out_last;
} tq[1024];

volatile unsigned int tq_tic;

#define LR(a)	((unsigned int *)a)[5]
#define XPSR(a)	((unsigned int *)a)[7]
#define TASK_XPSR(a)	((unsigned int *)a)[15]

static inline unsigned int  xpsr() {
	register unsigned int r0 asm("r0");
	__asm__ __volatile__("mrs r0,xpsr"::: "r0");
	return r0;
}

#if 0
void check_stack(struct task *t,struct task *n) {
	unsigned int stack_size=768-sizeof(*t);
	unsigned int stack_base=(t==&main_task)?0x20020000:((unsigned int)t)+768;
	if ((((unsigned int)t->sp)<(stack_base-stack_size))||(((unsigned int)t->sp)>stack_base)) {
		io_setpolled(1);
		sys_printf("stack corruption on task %s(%x)\n", t->name,t);
		ASSERT(0);
	}

}
#endif

void *PendSV_Handler_c(unsigned long int *save_sp) {
	int i=0;	
	struct task *t;
	sys_irqs++;
	while(i<MAX_PRIO) {
		t=ready[i];	
		if (t) {
			ready[i]=t->next;
			t->next=0;
			break;
		}
		i++;
	}
	if (!t) {
		ASSERT(0);
	}
	if (t==current) {
		ASSERT(0);
	}
	/* Have next, move away current if still here, timer and blocker moves it themselves */
	if (current->state==TASK_STATE_RUNNING) {
		int prio=current->prio_flags&0xf;
		current->state=TASK_STATE_READY;
		ASSERT(!current->next);
		CLR_TMARK(current);
		if (prio>4) prio=4;
		if (ready[prio]) {
			ready_last[prio]->next=current;
		} else {
			ready[prio]=current;
		}
		ready_last[prio]=current;
	}

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
			"cpsid i\n\t"
			"mrs r0,psr\n\t"
			"orr r0,r0,0x01000000\n\t"
			"stmfd sp!,{r0}\n\t"
			"push {lr}\n\t"
			"stmfd sp!,{r0-r3,r12,lr}\n\t"
			"mvns lr,#0x0e\n\t"
			"b	PendSV_Handler\n\t"
			:::);
	return 1;
}
//			"cpsie i\n\t"

void Error_Handler_c(void *sp);

void SysTick_Handler(void) {
	struct tq *tqp;
	int p=5;

	current->active_tics++;
	sys_irqs++;
	tq_tic++;
	tqp=&tq[tq_tic%1024];

	if (tqp->tq_out_first) {
		struct blocker *b=tqp->tq_out_first;
		while(b) {
			struct blocker *bnext=b->next;
			struct task *t=container_of(b,struct task,blocker);
			int prio=t->prio_flags&0xf;
			if (prio>MAX_PRIO) prio=MAX_PRIO;
			ASSERT(t->state!=TASK_STATE_RUNNING);
			t->state=TASK_STATE_READY;
			t->next=0;
			b->next=0;
			if (b->next2) {
				sys_printf("systick: blocker hae next2 link %x\n", b->next2);
			}
			if(prio<p) p=prio;
			if (!ready[prio]) {
				ready[prio]=t;
			} else {
				ready_last[prio]->next=t;
			}
			ready_last[prio]=t;
			DEBUGP(DLEV_SCHED,"timer_wakeup: readying %s\n", t->name);
			b=bnext;
		}
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
	sys_printf("in usagefault handler\n");
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
			"bx lr\n\t"
			:
			: [Error_Handler_c] "i" (Error_Handler_c)
			: 
	);
}

void Error_Handler_c(void *sp_v) {
	unsigned int *sp=(unsigned int *)sp_v;
	io_setpolled(1);
	sys_printf("\nerrorhandler trap, current=%s@0x%08x sp=0x%08x\n",current->name,current,sp);
	sys_printf("Value of CFSR register 0x%08x\n", SCB->CFSR);
	sys_printf("Value of HFSR register 0x%08x\n", SCB->HFSR);
	sys_printf("Value of DFSR register 0x%08x\n", SCB->DFSR);
	sys_printf("Value of MMFAR register 0x%08x\n", SCB->MMFAR);
	sys_printf("Value of BFAR register 0x%08x\n", SCB->BFAR);
	sys_printf("r0 =0x%08x, r1=0x%08x, r2=0x%08x, r3  =0x%08x\n", sp[0], sp[1], sp[2], sp[3]);
	sys_printf("r12=0x%08x, LR=0x%08x, PC=0x%08x, xPSR=0x%08x\n", sp[4], sp[5], sp[6], sp[7]);
	
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
			"bx lr\n\t"
			:
			: [SVC_Handler_c] "i" (SVC_Handler_c)
			: 
	);
}

int svc_kill_self(void);

struct user_fd {
	struct driver *driver;
	struct device_handle *dev_handle;
	unsigned int flags;
	struct user_fd *next;
};


static struct device_handle *console_open(void *driver_instance, DRV_CBH cb, void *dum);
static int console_close(struct device_handle *dh);
static int console_control(struct device_handle *dh, int cmd, void *arg1, int arg2);
static int console_init(void *instance);
static int console_start(void *instance);


static struct driver_ops console_ops = {
	console_open,
	console_close,
	console_control,
	console_init,
	console_start
};

static struct driver console = {
	"console",
	0,
	&console_ops,
};

struct driver *drv_root=&console;

#define FDTAB_SIZE 16

struct user_fd fd_tab[FDTAB_SIZE] = {{&console,0,}, };

int get_user_fd(struct driver *d, struct device_handle *dh) {
	int i;
	for(i=1;i<FDTAB_SIZE;i++) {
		if (!fd_tab[i].driver) {
			fd_tab[i].driver=d;
			fd_tab[i].dev_handle=dh;

			fd_tab[i].next=current->fd_list;
			current->fd_list=&fd_tab[i];
			return i;
		}
	}
	return -1;
}


int close_drivers(void) {
	struct user_fd *fdl=current->fd_list;

	current->fd_list=0;
	while(fdl) {
		struct user_fd *fdlnext=fdl->next;
		fdl->driver->ops->close(fdl->dev_handle);
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


static int sys_drv_wakeup(struct device_handle *dh, int ev, void *user_ref) {
	struct task *task=(struct task *)user_ref;
	int fd=dh->user_data1;
	if (task==current) {
		sys_printf("sys_drv_wakeup: wake up current\n");
		goto out_err;
	}
	if (task->state==TASK_STATE_RUNNING) {
		sys_printf("sys_drv_wakeup: wake up running task\n");
		goto out_err;
	}
	if (!task->sel_args) {
		goto out;
	}
	DEBUGP(DLEV_SCHED,"wakeup descriptor %d in task %s for ev %d\n", fd, task->name,ev);
	if (ev&EV_READ) {
		(*task->sel_args->rfds)|=(1<<fd);
	}
	if (ev&EV_WRITE) {
		(*task->sel_args->wfds)|=(1<<fd);
	}
	if (ev&EV_STATE) {
		(*task->sel_args->stfds)|=(1<<fd);
	}
	task->sel_args->nfds++;
out:
	sys_wakeup(&task->blocker,0,0);
out_err:
	return 0;
}

static int dump_blockers(struct device_handle *dh) {
	struct blocker *b=dh->blocker;
	int i=0;
	while (b) {
		struct task *t=container_of(b,struct task,blocker);
		sys_printf("%d: blocker for task %s\n",i, t->name);
		b=b->next;
		i++;
	}
	return 0;
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
			struct task *t=(struct task *)getSlab_1024();
			unsigned long int *stackp;

			if (prio>MAX_PRIO) {
				svc_args[0]=-1;
				return 0;
			}
			__builtin_memset(t,0,1024);
			t->sp=(void *)(((unsigned char *)t)+1024);
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
			if (current->state!=TASK_STATE_RUNNING) return 0;
			DEBUGP(DLEV_SCHED,"sleep task: name %s\n", current->name);
			sys_sleepon(&current->blocker,0,0,(unsigned int *)&svc_args[0]);
			return 0;
			break;
		}
		case SVC_IO_OPEN: {
			struct driver *drv;
			struct device_handle *dh;
			char *drvname=(char *)svc_args[0];
			int  ufd;
			drv=driver_lookup(drvname);
			if (!drv) {
				svc_args[0]=-1;
				return 0;
			}
			ufd=get_user_fd(((struct driver *)0xffffffff),0);
			if (ufd<0) {
				svc_args[0]=-1;
				return 0;
			}
			dh=drv->ops->open(drv->instance,sys_drv_wakeup,current);
			if (!dh) {
				detach_driver_fd(&fd_tab[ufd]);
				svc_args[0]=-1;
				return 0;
			}
			dh->user_data1=ufd;
			fd_tab[ufd].driver=drv;
			fd_tab[ufd].dev_handle=dh;
			fd_tab[ufd].flags=0;
			svc_args[0]=ufd;
			return 0;
		}
		case SVC_IO_READ: {
			int fd=(int)svc_args[0];
			int rc;
			struct user_fd *fdd=&fd_tab[fd];
			struct driver *driver=fdd->driver;
			struct device_handle *dh=fdd->dev_handle;
			if (!driver) {
				svc_args[0]=-1;
				return 0;
			}

			if (dh->blocker) {
				dump_blockers(dh);
				sys_printf("oh shit!\n");
			}
			while(1) {
				rc=driver->ops->control(dh,RD_CHAR,(void *)svc_args[1],svc_args[2]);
				if (rc==-DRV_AGAIN&&(!fdd->flags&O_NONBLOCK)) {
					sys_sleepon(&current->blocker,0,0,0);
				} else {
					break;
				}
			}
			svc_args[0]=rc;
			return 0;
		}
		case SVC_IO_WRITE: {
			int fd=(int)svc_args[0];
			struct user_fd *fdd=&fd_tab[fd];
			struct driver *driver=fdd->driver;
			struct device_handle *dh=fdd->dev_handle;
			int rc;
			if (!driver) {
				svc_args[0]=-1;
				return 0;
			}

again:
			rc=driver->ops->control(dh,WR_CHAR,(void *)svc_args[1],svc_args[2]);

			if (rc==-DRV_AGAIN&&(!(fdd->flags&O_NONBLOCK))) {
				sys_sleepon(&current->blocker,0,0,0);
				goto again;
			}
			if (rc==-DRV_INPROGRESS&&(!(fdd->flags&O_NONBLOCK))) {
				int result;
				sys_sleepon(&current->blocker,0,0,0);
				rc=driver->ops->control(dh,WR_GET_RESULT,&result,sizeof(result));
				if (rc<0) {
					svc_args[0]=rc;
				} else {
					svc_args[0]=result;
				}
				return 0;
			}
			svc_args[0]=rc;
			return 0;
		}
		case SVC_IO_CONTROL: {
			int fd=(int)svc_args[0];
			struct user_fd *fdd=&fd_tab[fd];
			struct driver *driver=fdd->driver;
			struct device_handle *dh=fdd->dev_handle;
			if (!driver) {
				svc_args[0]=-1;
				return 0;
			}

			if (svc_args[1]==F_SETFL) {
				fdd->flags=svc_args[2];
				return 0;
			}
			svc_args[0]=driver->ops->control(dh,svc_args[1],(void *)svc_args[2],svc_args[3]);
			return 0;
		}
		case SVC_IO_CLOSE: {
			int fd=(int)svc_args[0];
			struct driver *driver=fd_tab[fd].driver;
			struct device_handle *dh=fd_tab[fd].dev_handle;
			if (!driver) {
				svc_args[0]=-1;
				return 0;
			}

			detach_driver_fd(&fd_tab[fd]);

			svc_args[0]=driver->ops->close(dh);
			return 0;
		}
		case SVC_IO_SELECT: {
			struct sel_args *sel_args=(struct sel_args *)svc_args[0];
			int nfds=sel_args->nfds;
			fd_set rfds=*sel_args->rfds;
			fd_set wfds=*sel_args->wfds;
			fd_set stfds=*sel_args->stfds;
			unsigned int *tout=sel_args->tout;
			int i;

			sel_args->nfds=0;
			*sel_args->rfds=0;
			*sel_args->wfds=0;
			*sel_args->stfds=0;

			for(i=0;i<nfds;i++) {
				if (wfds&(1<<i)) {
					struct driver *driver=fd_tab[i].driver;
					struct device_handle *dh=fd_tab[i].dev_handle;
					if (driver) {
						if (driver->ops->control(dh,IO_POLL,(void *)EV_WRITE,0)) {
							*sel_args->wfds=(1<<i);
							svc_args[0]=1;
							return 0;
						}
					} else {
						svc_args[0]=-1;
						return 0;
					}
				}
			}

			for(i=0;i<nfds;i++) {
				if (rfds&(1<<i)) {
					struct driver *driver=fd_tab[i].driver;
					struct device_handle *dh=fd_tab[i].dev_handle;
					if (driver) {
						if (driver->ops->control(dh,IO_POLL,(void *)EV_READ,0)) {
							*sel_args->rfds=(1<<i);
							svc_args[0]=1;
							return 0;
						}
					} else {
						svc_args[0]=-1;
						return 0;
					}
				}
			}

			for(i=0;i<nfds;i++) {
				if (stfds&(1<<i)) {
					struct driver *driver=fd_tab[i].driver;
					struct device_handle *dh=fd_tab[i].dev_handle;
					if (driver) {
						if (driver->ops->control(dh,IO_POLL,(void *)EV_STATE,0)) {
							*sel_args->stfds=(1<<i);
							svc_args[0]=1;
							return 0;
						}
					} else {
						svc_args[0]=-1;
						return 0;
					}
				}
			}

			current->sel_args=sel_args;
			sys_sleepon(&current->blocker,0,0,tout);
			if (current->sel_args!=sel_args) {
				ASSERT(0);
			}
			current->sel_args=0;

			svc_args[0]=sel_args->nfds;
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
	sys_sleepon(&current->blocker,0,0,&ms);
	return 0;
}

struct tq *sys_timer(struct blocker *so, unsigned int ms) {
	int wtic=(ms/10)+tq_tic;
	struct tq *tout=&tq[wtic%1024];
	so->wakeup_tic=wtic;
	if (!tout->tq_out_first) {
		tout->tq_out_first=so;
		tout->tq_out_last=so;
	} else {
		tout->tq_out_last->next=so;
		tout->tq_out_last=so;
	}
	return tout;
}

void sys_timer_remove(struct blocker *b) {
	struct tq *tout=&tq[b->wakeup_tic%1024];
	struct blocker *bp=tout->tq_out_first;
	struct blocker **bpp=&tout->tq_out_first;
	struct blocker *bprev=0;
	while(bp) {
		if (bp==b) {
			(*bpp)=bp->next;
			if (bp==tq->tq_out_last) {
				tq->tq_out_last=bprev;
			}
			return ;
		}
		bprev=bp;
		bpp=&bp->next;
		bp=bp->next;
	}
}

int sleepable(void) {
	int in_irq=xpsr()&0x1ff;
	if (in_irq&&(in_irq!=0xb)) {
                return 0;
	}
	return 1;
}

/* not to be called from irq, cant sleep */
void *sys_sleepon(struct blocker *so, void *bp, int bsize,unsigned int *tout) {
	int in_irq=xpsr()&0x1ff;
	if (in_irq&&(in_irq!=0xb)) {
		return 0;
	}
	
	if (current->state!=TASK_STATE_RUNNING) {
		return 0;
	}
	current->bp=bp;
	current->bsize=bsize;

	CLR_TMARK(current);

	if (tout) {
		sys_timer(so,*tout);
	} else {
		so->wakeup_tic=0;
	}
	DEBUGP(DLEV_SCHED,"sys sleepon %x task: name %s\n", so, current->name);
	current->state=TASK_STATE_IO;
	fake_pendSV();
	*((volatile unsigned int *)0xe000ed24)|=0x80; /* set cpu state to
							SVC call active */
	if (tout) {
		if (so->wakeup_tic!=tq_tic) {
			*tout=(so->wakeup_tic-tq_tic)*10;
		}
		so->wakeup_tic=0;
	}
	return so;
}

void *sys_wakeup(struct blocker *so, void *bp, int bsize) {
	struct task *n = container_of(so,struct task, blocker);
	int prio;

	if (!n) {
		return 0;
	}
	if (n->bsize<bsize) {
		return 0;
	}
	if (n==current) {
		sys_printf("waking up current\n");
		return 0;
	}
	if (n->state==TASK_STATE_RUNNING) {
		sys_printf("waking up running process\n");
		return 0;
	}

	if (so->next) {
		sys_printf("sys_wakeup:blocker has next link to %x in wakup\n", so->next);
	}

	if (so->next2) {
		sys_printf("sys_wakeup:blocker has next2 link to %x in wakup\n", so->next2);
	}
	if (so->wakeup_tic) {
		sys_timer_remove(so);
	}
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
		DEBUGP(DLEV_SCHED,"wakeup(readying) %x task: name %s, readying %s\n",so,current->name,n->name);
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

/**********************************************/

static struct driver *my_usart;
static struct device_handle *my_usart_dh;

static struct device_handle *console_open(void *driver_instance, DRV_CBH cb, void *dum) {
	return 0;
}

static int console_close(struct device_handle *dh) {
	return 0;
}

static int console_control(struct device_handle *dh, int cmd, void *arg1, int arg2) {
	if(cmd==WR_CHAR) {
		if (my_usart_dh) {
			my_usart->ops->control(my_usart_dh,cmd,arg1,arg2);
		}
	}
	return arg2;
}

static int console_init(void *instance) {
	return 0;
}

static int console_start(void *instance) {
	my_usart=driver_lookup("usart0");
	my_usart_dh=my_usart->ops->open(my_usart->instance,0,0);
	return 0;
}



/*** Sys mon functions ***********/

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
	if (argc>1) {
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


static struct cmd cmd_root[] = {
		{"help",generic_help_fnc},
		{"ps",ps_fnc},
		{"lsdrv",lsdrv_fnc},
		{"debug",debug_fnc},
		{"reboot",reboot_fnc},
		{"block",block_fnc},
		{"unblock",unblock_fnc},
		{"setprio",setprio_fnc},
		{0,0}
};

struct cmd_node my_cmd_node = {
	"",
	cmd_root,
};


/*******************************************************************************/
/*  Sys support functions and unecessary stuff                                 */
/*                                                                             */

extern unsigned long int init_func_begin[];
extern unsigned long int init_func_end[];
typedef void (*ifunc)(void);

void init_sys(void) {
	unsigned long int *i;
	NVIC_SetPriority(PendSV_IRQn,0xf);
	NVIC_SetPriority(SVCall_IRQn,0xe);
	NVIC_SetPriority(SysTick_IRQn,0xd);  /* preemptive tics... */
	*((unsigned int *)0xe000ed14)&=~0x200;
	*((unsigned int *)0xe000ed14)|=1;

	SysTick_Config(SystemCoreClock/100); // 10 mS tic is 100/Sec
	for(i=init_func_begin;i<init_func_end;i++) {
		((ifunc)*i)();
	}
	
	driver_init();
	driver_start();
}

extern void sys_mon(void *);

void start_sys(void) {
	thread_create(sys_mon,"usart0",3,"sys_mon");
//	thread_create(sys_mon,"stterm0",3,"sys_mon");
//	thread_create(sys_mon,"usb_serial0",2,"sys_mon");
}
