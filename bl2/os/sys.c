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
#include "stm32/stm32f407.h"
#include "io.h"
#include "sys.h"

#include <string.h>

unsigned int sys_irqs;

struct user_fd;

struct task * volatile ready[5];
struct task * volatile ready_last[5];

struct task main_task = { "init_main", (void *)0x20020000, 0, 0, 1, 3, 0,0,(void *)(0x20020000-1024) };
struct task *troot =&main_task;
struct task * volatile current = &main_task;

struct Slab_256 {
	unsigned char mem[256];
};

struct Slab_1024 {
	unsigned char mem[1024];
};

extern unsigned char __bss_end__[];
struct Slab_256 *slab_256=(struct Slab_256 *)__bss_end__;
unsigned int free_slab_256=0;

#define MSTACK (0x20020000)
#define MSTACK_SIZE (0x400)

struct Slab_1024 *slab_1024=(struct Slab_1024 *)(MSTACK-(MSTACK_SIZE*16));

unsigned int max_slab_256;
unsigned int max_slab_1024=16;
unsigned int free_slab_1024;



void *getSlab_256(void) {
	if (free_slab_256<max_slab_256) {
		return &slab_256[free_slab_256++];
	}
	return 0;
}

void *getSlab_1024(void) {
	if (free_slab_1024<(max_slab_1024-1)) {
		return &slab_1024[free_slab_1024++];
	}
	return 0;
};

struct task *tpool;
unsigned int tpool_free;

struct task *get_task() {

again:
	if (tpool_free) {
		struct task *tmp=tpool;
		tpool_free--;
		tpool=tmp->next2;
		__builtin_memset(tmp,0,sizeof(struct task));
		return tmp;
	} else {
		struct task *tmp=getSlab_256();
		int num=256/sizeof(struct task);
		int i;
		if (!tmp) return 0;
		tmp[0].next2=0;
		for(i=1;i<num;i++) {
			tmp[i-1].next2=&tmp[i];
		}
		tmp[i].next2=0;
		tpool_free=num;
		tpool=tmp;
		goto again;
	}
	return 0;
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
			disable_interrupt();
			t->state=TASK_STATE_READY;
			t->next=0;
			b->next=0;
			if (b->next2) {
				sys_printf("systick: blocker have next2 link %x\n", b->next2);
			}
			if(prio<p) p=prio;
			if (!ready[prio]) {
				ready[prio]=t;
			} else {
				ready_last[prio]->next=t;
			}
			ready_last[prio]=t;
			enable_interrupt();
			DEBUGP(DLEV_SCHED,"timer_wakeup: readying %s\n", t->name);
			b=bnext;
		}
		tqp->tq_out_first=tqp->tq_out_last=0;
	}

	if (p<GET_PRIO(current)) {
		switch_on_return();
		return;
	}

	if (GET_TMARK(current) && ready[GET_PRIO(current)]) {
		DEBUGP(DLEV_SCHED,"time slice: switch out %s, switch in %s\n", current->name, ready[GET_PRIO(current)]->name);
		switch_on_return();
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

	if (task->sel_data_valid) {
		DEBUGP(DLEV_SCHED,"wakeup descriptor %d in task %s for ev %d\n", fd, task->name,ev);
		if (ev&EV_READ) {
			(task->sel_data.rfds)|=(1<<fd);
		}
		if (ev&EV_WRITE) {
			(task->sel_data.wfds)|=(1<<fd);
		}
		if (ev&EV_STATE) {
			(task->sel_data.stfds)|=(1<<fd);
		}
		task->sel_data.nfds++;
	}

	if (task==current) {
		current->blocker.wake=1;
//		sys_printf("sys_drv_wakeup: wake up current\n");
		goto out_err;
	}

	if (task->state!=TASK_STATE_RUNNING) {
		sys_wakeup(&task->blocker,0,0);
	}
out_err:
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
			struct task_create_args *tca=
				(struct task_create_args *)svc_args[0];
			unsigned char *estack=getSlab_1024();
			struct task *t=get_task();
			unsigned long int *stackp=(unsigned long int *)(estack+1024);
			unsigned char *uval=tca->val;
			if ((!t)||(!estack)) {
				svc_args[0]=-1;
				return 0;
			}

			map_tmp_stack_page((unsigned long int)estack,9);

			if (tca->prio>MAX_PRIO) {
				svc_args[0]=-1;
				return 0;
			}
			__builtin_memset(estack,0,1020);
			t->estack=estack;
			if (tca->val_size) {
				int aval_size=(tca->val_size+7)&~0x7;
				uval=((unsigned char *)stackp)-aval_size;
				__builtin_memcpy(uval,tca->val,tca->val_size);
				stackp=(unsigned long int *)uval;
			}
			*(--stackp)=0x01000000; 		 // xPSR
			*(--stackp)=(unsigned long int)tca->fnc;    // r15
			*(--stackp)=(unsigned long int)svc_kill_self;     // r14
			*(--stackp)=0;     // r12
			*(--stackp)=0;     // r3
			*(--stackp)=0;     // r2
			*(--stackp)=0;     // r1
			*(--stackp)=(unsigned long int)uval;    // r0
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
			t->name=tca->name;
			SET_PRIO(t,tca->prio);
			t->next2=troot;
			troot=t;

			if(!ready[tca->prio]) {
				ready[tca->prio]=t;
			} else {
				ready_last[tca->prio]->next=t;
			}
			ready_last[tca->prio]=t;
			unmap_tmp_stack_page();

			if (tca->prio<GET_PRIO(current)) {
				DEBUGP(DLEV_SCHED,"Create task: name %s, switch out %s\n", t->name, current->name);
				switch_on_return();
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
			switch_on_return();
			return 0;
		}
		case SVC_SLEEP: {
			if (current->state!=TASK_STATE_RUNNING) return 0;
			DEBUGP(DLEV_SCHED,"sleep task: name %s\n", current->name);
			current->blocker.driver=0;
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

			current->blocker.dh=dh;
			current->blocker.ev=EV_READ;

			while(1) {
				rc=driver->ops->control(dh,RD_CHAR,(void *)svc_args[1],svc_args[2]);
				if (rc==-DRV_AGAIN&&(!fdd->flags&O_NONBLOCK)) {
					current->blocker.driver=driver;
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
			int rc=0;
			int done=0;
			if (!driver) {
				svc_args[0]=-1;
				return 0;
			}

			current->blocker.dh=dh;
			current->blocker.ev=EV_WRITE;

again:
			rc=driver->ops->control(dh,WR_CHAR,(void *)svc_args[1]+done,svc_args[2]-done);

			if (rc==-DRV_AGAIN&&(!(fdd->flags&O_NONBLOCK))) {
				current->blocker.driver=driver;
				sys_sleepon(&current->blocker,0,0,0);
				goto again;
			}

			if (rc==-DRV_INPROGRESS&&(!(fdd->flags&O_NONBLOCK))) {
				int result;
				current->blocker.driver=driver;
				sys_sleepon(&current->blocker,0,0,0);
				rc=driver->ops->control(dh,WR_GET_RESULT,&result,sizeof(result));
				if (rc<0) {
					svc_args[0]=rc;
				} else {
					svc_args[0]=result;
				}
				return 0;
			}
			if (rc>=0) done+=rc;
			if (done!=svc_args[2]) {
				goto again;
			}
			svc_args[0]=done;
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

			current->sel_data.nfds=0;
			current->sel_data.rfds=0;
			current->sel_data.wfds=0;
			current->sel_data.stfds=0;

			current->sel_data_valid=1;
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

			current->blocker.driver=0;
			disable_interrupt();
			if (!current->sel_data.nfds) {
				sys_sleepon(&current->blocker,0,0,tout);
			}
			current->sel_data_valid=0;
			enable_interrupt();

			*sel_args->rfds=current->sel_data.rfds;
			*sel_args->wfds=current->sel_data.wfds;
			*sel_args->stfds=current->sel_data.stfds;
			svc_args[0]=current->sel_data.nfds;
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
				switch_on_return();
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
						switch_on_return();
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
				switch_on_return();
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
	current->blocker.driver=0;
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

int device_ready(struct blocker *b) {
	return b->driver->ops->control(b->dh, IO_POLL, (void *)b->ev, 0);
}

/* not to be called from irq, cant sleep */
void *sys_sleepon(struct blocker *so, void *bp, int bsize,unsigned int *tout) {
	if (!task_sleepable()) return 0;
	disable_interrupt();

	if (so->wake) {
		so->wake=0;
		enable_interrupt();
		return 0;
	}
	if (so->driver && device_ready(so)) {
		enable_interrupt();
		return 0;
	}
	if (current->state!=TASK_STATE_RUNNING) {
		enable_interrupt();
		sys_printf("wanted to sleep, non running task\n");
		return 0;
	}

	current->state=TASK_STATE_IO; /* Do not do any more debug when state
					is running */
	enable_interrupt();
	CLR_TMARK(current);

	if (tout) {
		current->state=TASK_STATE_TIMER;
		sys_timer(so,*tout);
	} else {
		so->wakeup_tic=0;
	}

	DEBUGP(DLEV_SCHED,"sys sleepon %x task: name %s\n", so, current->name);
	switch_now();

	if (tout) {
		if (so->wakeup_tic!=tq_tic) {
			*tout=(so->wakeup_tic-tq_tic)*10;
		}
		so->wakeup_tic=0;
	}
	DEBUGP(DLEV_SCHED,"sys sleepon woke up %x task: name %s\n", so, current->name);
	return so;
}

void *sys_sleepon_update_list(struct blocker *b, struct blocker_list *bl_ptr) {
	void *rc=0;
	struct blocker **pprev;
	struct blocker *tmp;
	if (current->state!=TASK_STATE_RUNNING) {
		return 0;
	}
	disable_interrupt();
	if (!bl_ptr) goto out;
	if (bl_ptr->is_ready()) goto out;
	b->next2=0;
	if (bl_ptr->first) {
		bl_ptr->last->next2=b;
	} else {
		bl_ptr->first=b;
	}
	bl_ptr->last=b;
	enable_interrupt();
	rc=sys_sleepon(b,0,0,0);
	if (!rc) { 
		/* return 0, means no sleep */
		/* link out the blocker */
		pprev=&bl_ptr->first;
		tmp=bl_ptr->first;
		while(tmp) {
			if (tmp==b) {
				*pprev=b->next2;
				b->next2=0;
				break;
			}
			pprev=&tmp->next2;
			tmp=tmp->next2;
		}
		if (tmp==b) {
			sys_printf("sleep_list: failed, linked out blocker\n");
		} else {
			sys_printf("sleep_list: failed, blocker not found\n");
		}
	}
	return rc;
out:
	enable_interrupt();
	return rc;
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
		so->wake=1;
		return 0;
	}

	if (n->state==TASK_STATE_RUNNING) {
		so->wake=1;
//		sys_printf("waking up running process\n");
//		ASSERT(0);
		return 0;
	}

	if (n->state==TASK_STATE_READY) {
//		sys_printf("waking up ready process\n");
//		ASSERT(0);
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

	disable_interrupt();
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
	enable_interrupt();
	
	if (prio<GET_PRIO(current)) {
		DEBUGP(DLEV_SCHED,"wakeup(immed readying) %x task: name %s, readying %s\n",so,current->name,n->name);
		switch_on_return();
		return n;
	}
	DEBUGP(DLEV_SCHED,"wakeup(readying) %x task: name %s, readying %s\n",so,current->name,n->name);
 	return n;
}

void *sys_wakeup_from_list(struct blocker_list *bl) {
	struct blocker *tmp;
	void *rc=0;
	disable_interrupt();
	tmp=bl->first;
	while (bl->first) {
		tmp=bl->first;
		bl->first=tmp->next2;
		tmp->next2=0;
		enable_interrupt();
		rc=sys_wakeup(tmp,0,0);
		disable_interrupt();
//		ASSERT(rc);
	}
	enable_interrupt();
	return rc;
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
//		fprintf(env->io_fd,"task(%x) %12s, sp=0x%08x, pc=0x%08x, prio=%x, state=%c, atics=%d\n", 
//			t, t->name, t->sp, (t->state!=TASK_STATE_RUNNING)?((unsigned int *)t->sp)[13]:0xffffffff, t->prio_flags, get_state(t), t->active_tics);
		fprintf(env->io_fd,"task(%x) %12s, sp=0x%08x, prio=%x, state=%c, atics=%d\n", 
			t, t->name, t->sp, t->prio_flags, get_state(t), t->active_tics);
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
		if (__builtin_strcmp(argv[1],"on")==0) {
			dbglev=10;
		} else if (__builtin_strcmp(argv[1],"off")==0) {
			dbglev=0;
		} else {
			fprintf(env->io_fd,"debug <on> | <off>\n");
		}
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
	init_memory_protection();
	slab_256=(struct Slab_256 *)((unsigned int)((slab_256+0xff))&~0xff);
	max_slab_256=(((unsigned int)slab_1024)-((unsigned int)slab_256))/0x100;
	unmap_stack_memory((unsigned long int)slab_1024);
	map_stack_page((unsigned long int)&slab_1024[15], 9); /* map in last stackpage, size 1k */
	__builtin_memset(&slab_1024[15],0,900);
	activate_memory_protection();
	init_switcher();
//	NVIC_SetPriority(SVCall_IRQn,0xe);
	NVIC_SetPriority(SVCall_IRQn,0xf);  /* try to share level with pendsv */
	NVIC_SetPriority(SysTick_IRQn,0xd);  /* preemptive tics... */

	SysTick_Config(SystemCoreClock/100); // 10 mS tic is 100/Sec
	for(i=init_func_begin;i<init_func_end;i++) {
		((ifunc)*i)();
	}
	
	driver_init();
	driver_start();
}

extern void sys_mon(void *);

void start_sys(void) {
	thread_create(sys_mon,"usart0",0,3,"sys_mon");
//	thread_create(sys_mon,"stterm0",3,"sys_mon");
//	thread_create(sys_mon,"usb_serial0",2,"sys_mon");
}
