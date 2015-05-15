/* $notix/leanaux: , v1.1 2014/04/07 21:44:00 anders Exp $ */

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
//#include "stm32/stm32f407.h"
#include "io.h"
#include "sys.h"

#include <string.h>

unsigned int sys_irqs;

struct task * volatile ready[5];
struct task * volatile ready_last[5];

struct task main_task = { "init_main", (void *)(0x20020000), 256, 0, 0, 1, 3, (void *)(0x20020000-0x800) };
//struct task main_task = { "init_main", (void *)(0x84000000-0x2000), 256, 0, 0, 1, 3, (void *)(0x84000000-0x3000) };
struct task *troot =&main_task;
struct task * volatile current = &main_task;


struct task *t_array[256];

void init_task_list() {
        int i;
        for(i=0;i<255;i++) {
                t_array[i]=0;
        }
}

int allocate_task_id(struct task *t) {
        int i;
        for(i=0;i<255;i++) {
                if (!t_array[i]) {
                        t_array[i]=t;
                        t->id=i;
                        return i;
                }
        }
        return -1;
}

struct task *lookup_task_for_name(char *task_name) {
	struct task *t=troot;
	while(t) {
		if (__builtin_strncmp(t->name,task_name,__builtin_strlen(t->name)+1)==0) {
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

void SysTick_Handler(void) {
	struct tq *tqp;
	int p=5;

	current->active_tics++;
	sys_irqs++;
	tq_tic++;
	tqp=&tq[tq_tic%1024];

	enable_interrupts();

#if 0
	if (!(sys_irqs%1000)) {
		sys_printf("sys_tic: current %x\n",current);
	}
#endif

	if (tqp->tq_out_first) {
		struct blocker *b=tqp->tq_out_first;
		while(b) {
			unsigned long int cpu_flags;
			struct blocker *bnext=b->next;
			struct task *t=container_of(b,struct task,blocker);
			int prio=t->prio_flags&0xf;
			if (prio>MAX_PRIO) prio=MAX_PRIO;
			ASSERT(t->state!=TASK_STATE_RUNNING);
			cpu_flags=disable_interrupts();
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
			restore_cpu_flags(cpu_flags);
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

/*
 * Define the interrupt handler for the service calls,
 * It will call the C-function after retrieving the
 * stack of the svc caller.
 */

//int svc_kill_self(void);

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

static void sys_timer_remove(struct blocker *b);

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
	} else {
		if (task->blocker.ev!=ev) {
			sys_printf("sys_drv_wakeup: masked ev\n");
			return 0;
		}
	}

	if (task==current) {
		current->blocker.wake=1;
		if (current->blocker.wakeup_tic) {
			sys_timer_remove(&current->blocker);
		}
//		sys_printf("sys_drv_wakeup: wake up current\n");
		goto out;
	} else {
//		sys_printf("sys_drv_wakeup: wake up %s\n", task->name);
	}

	if (task->state!=TASK_STATE_RUNNING) {
		sys_wakeup(&task->blocker);
	}
out:
	return 0;
}

void *handle_syscall(unsigned long int *svc_sp) {
	unsigned int svc_number;

	sys_irqs++;
	/*
	 * Stack contains:
	 * r0, r1, r2, r3, r12, r14, the return address and xPSR
	 * First argument (r0) is svc_args[0]
	 */

	svc_number=get_svc_number(svc_sp);
	switch(svc_number) {
		case SVC_CREATE_TASK: {
			struct task_create_args *tca=
				(struct task_create_args *)get_svc_arg(svc_sp,0);
			struct task *t=(struct task *)get_page();	
			unsigned char *estack=((unsigned char *)t)+2048;
			unsigned long int *stackp=(unsigned long int *)(estack+2048);
			unsigned char *uval=tca->val;
			memset(t,0,sizeof(struct task));
			if (allocate_task_id(t)<0) {
				sys_printf("proc table full\n");
				put_page(t);
				set_svc_ret(svc_sp,-1);
				return 0;
			}
			if ((!t)||(!estack)) {
				set_svc_ret(svc_sp,-1);
				return 0;
			}

			map_tmp_stack_page((unsigned long int)estack,9);

			if (tca->prio>MAX_PRIO) {
				set_svc_ret(svc_sp,-1);
				return 0;
			}
			__builtin_memset(estack,0,2048);
			t->estack=estack;
			if (tca->val_size) {
				int aval_size=(tca->val_size+7)&~0x7;
				uval=((unsigned char *)stackp)-aval_size;
				memcpy(uval,tca->val,tca->val_size);
				stackp=(unsigned long int *)uval;
			}
			setup_return_stack(t,(void *)stackp,(unsigned long int)tca->fnc,0,uval,8);

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
				DEBUGP(DLEV_SCHED,"Create task: %s, switch out %s\n", t->name, current->name);
				switch_on_return();
			} else {
				DEBUGP(DLEV_SCHED,"Create task: %s\n", t->name);
			}

			set_svc_ret(svc_sp,0);
			return 0;
			break;
		}
		case SVC_KILL_SELF: {
			if (current->state!=TASK_STATE_RUNNING) return 0;

			close_drivers();
			current->state=TASK_STATE_DEAD;

			DEBUGP(DLEV_SCHED,"kill self task: %s\n", current->name);
			switch_on_return();
			return 0;
		}
		case SVC_SLEEP: {
			unsigned int tout=get_svc_arg(svc_sp,0);
			if (current->state!=TASK_STATE_RUNNING) return 0;
			DEBUGP(DLEV_SCHED,"sleep current task: %s\n", current->name);
			current->blocker.driver=0;
			sys_sleepon(&current->blocker,(unsigned int *)&tout);
			set_svc_ret(svc_sp,tout);
			return 0;
			break;
		}
		case SVC_IO_OPEN: {
			struct driver *drv;
			struct device_handle *dh;
			char *drvname=(char *)get_svc_arg(svc_sp,0);
			int  ufd;
			drv=driver_lookup(drvname);
			if (!drv) {
				set_svc_ret(svc_sp,-1);
				sys_printf("SVC_IO_OPEN: return -1 no drv\n");
				return 0;
			}
			ufd=get_user_fd(((struct driver *)0xffffffff),0);
			if (ufd<0) {
				set_svc_ret(svc_sp,-1);
//				sys_printf("SVC_IO_OPEN: return -1 get user\n");
				return 0;
			}
			dh=drv->ops->open(drv->instance,sys_drv_wakeup,current);
			if (!dh) {
				detach_driver_fd(&fd_tab[ufd]);
				set_svc_ret(svc_sp,-1);
				sys_printf("SVC_IO_OPEN: return -1 drv open\n");
				return 0;
			}
			dh->user_data1=ufd;
			fd_tab[ufd].driver=drv;
			fd_tab[ufd].dev_handle=dh;
			fd_tab[ufd].flags=0;
			set_svc_ret(svc_sp,ufd);
			return 0;
		}
		case SVC_IO_READ: {
			int fd=(int)get_svc_arg(svc_sp,0);
			int rc;
			struct user_fd *fdd=&fd_tab[fd];
			struct driver *driver=fdd->driver;
			struct device_handle *dh=fdd->dev_handle;
			if (!driver) {
				set_svc_ret(svc_sp,-1);
				return 0;
			}

			current->blocker.dh=dh;
			current->blocker.ev=EV_READ;

			while(1) {
				rc=driver->ops->control(dh,RD_CHAR,(void *)get_svc_arg(svc_sp,1),get_svc_arg(svc_sp,2));
				if (rc==-DRV_AGAIN&&(!fdd->flags&O_NONBLOCK)) {
					current->blocker.driver=driver;
					sys_sleepon(&current->blocker,0);
				} else {
					break;
				}
			}
			set_svc_ret(svc_sp,rc);
			return 0;
		}
		case SVC_IO_WRITE: {
			int fd=(int)get_svc_arg(svc_sp,0);
			struct user_fd *fdd=&fd_tab[fd];
			struct driver *driver=fdd->driver;
			struct device_handle *dh=fdd->dev_handle;
			int rc=0;
			int done=0;
			if (!driver) {
				set_svc_ret(svc_sp,-1);
				return 0;
			}

			current->blocker.dh=dh;
			current->blocker.ev=EV_WRITE;

again:
			rc=driver->ops->control(dh,WR_CHAR,(void *)get_svc_arg(svc_sp,1)+done,get_svc_arg(svc_sp,2)-done);

			if (rc==-DRV_AGAIN&&(!(fdd->flags&O_NONBLOCK))) {
				current->blocker.driver=driver;
				sys_sleepon(&current->blocker,0);
				goto again;
			}

			if (rc==-DRV_INPROGRESS&&(!(fdd->flags&O_NONBLOCK))) {
				int result;
				current->blocker.driver=driver;
				sys_sleepon(&current->blocker,0);
				rc=driver->ops->control(dh,WR_GET_RESULT,&result,sizeof(result));
				if (rc<0) {
					set_svc_ret(svc_sp,rc);
				} else {
					set_svc_ret(svc_sp,result);
				}
				return 0;
			}
			if (rc>=0) done+=rc;
			if ((done!=get_svc_arg(svc_sp,2))&&(!(fdd->flags&O_NONBLOCK))) {
				goto again;
			}
			set_svc_ret(svc_sp,done);
			return 0;
		}
		case SVC_IO_CONTROL: {
			int	fd	=(int)get_svc_arg(svc_sp,0);
			struct	user_fd		*fdd=&fd_tab[fd];
			struct	driver		*driver=fdd->driver;
			struct	device_handle	*dh=fdd->dev_handle;
			int	ufd;
			int	rc;

			if (!driver) {
				set_svc_ret(svc_sp,-1);
				return 0;
			}
		
			if (get_svc_arg(svc_sp,1)==DYNOPEN) {
				struct dyn_open_args doargs;

				doargs.name=(char *)get_svc_arg(svc_sp,2);
				ufd=get_user_fd(((struct driver *)0xffffffff),0);
				if (ufd<0) {
					set_svc_ret(svc_sp,-1);
					return 0;
				}

				rc=driver->ops->control(dh,DYNOPEN,(void *)&doargs,get_svc_arg(svc_sp,3));
				if (rc<0) {
					detach_driver_fd(&fd_tab[ufd]);
					set_svc_ret(svc_sp,-1);
					return 0;
				}
				doargs.dh->user_data1=ufd;
				fd_tab[ufd].driver=driver;
				fd_tab[ufd].dev_handle=doargs.dh;
				fd_tab[ufd].flags=0;
				set_svc_ret(svc_sp,ufd);
				return 0;
			}

			if (get_svc_arg(svc_sp,1)==F_SETFL) {
				fdd->flags=get_svc_arg(svc_sp,2);
				return 0;
			}
			rc=driver->ops->control(dh,get_svc_arg(svc_sp,1),(void *)get_svc_arg(svc_sp,2),get_svc_arg(svc_sp,3));
			set_svc_ret(svc_sp,rc);
			return 0;
		}
		case SVC_IO_CLOSE: {
			int fd=(int)get_svc_arg(svc_sp,0);
			struct driver *driver=fd_tab[fd].driver;
			struct device_handle *dh=fd_tab[fd].dev_handle;
			int rc;
			if (!driver) {
				set_svc_ret(svc_sp,-1);
				return 0;
			}

			detach_driver_fd(&fd_tab[fd]);

			rc=driver->ops->close(dh);
			set_svc_ret(svc_sp,rc);
			return 0;
		}
		case SVC_IO_SELECT: {
			struct sel_args *sel_args=(struct sel_args *)get_svc_arg(svc_sp,0);
			int nfds=sel_args->nfds;
			fd_set rfds=*sel_args->rfds;
			fd_set wfds=*sel_args->wfds;
			fd_set stfds=*sel_args->stfds;
			unsigned int *tout=sel_args->tout;
			int i;
			unsigned long int cpu_flags;

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
							set_svc_ret(svc_sp,1);
							return 0;
						}
					} else {
						set_svc_ret(svc_sp,-1);
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
							set_svc_ret(svc_sp,1);
							return 0;
						}
					} else {
						set_svc_ret(svc_sp,-1);
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
							set_svc_ret(svc_sp,1);
							return 0;
						}
					} else {
						set_svc_ret(svc_sp,-1);
						return 0;
					}
				}
			}

			current->blocker.driver=0;
			cpu_flags=disable_interrupts();
			if (!current->sel_data.nfds) {
				sys_sleepon(&current->blocker,tout);
			}
			current->sel_data_valid=0;
			restore_cpu_flags(cpu_flags);

			*sel_args->rfds=current->sel_data.rfds;
			*sel_args->wfds=current->sel_data.wfds;
			*sel_args->stfds=current->sel_data.stfds;
			set_svc_ret(svc_sp,current->sel_data.nfds);
			return 0;
		}
		case SVC_BLOCK_TASK: {
			char *name=(char *)get_svc_arg(svc_sp,0);
			struct task *t=lookup_task_for_name(name);
			if (!t) {
				set_svc_ret(svc_sp,-1);
				return 0;
			}
			if (t==current) {
				SET_PRIO(t,t->prio_flags|0x8);
				DEBUGP(DLEV_SCHED,"blocking current task: %s\n",current->name);
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
				DEBUGP(DLEV_SCHED,"blocking task: %s, current %s\n",t->name,current->name);
			}
			set_svc_ret(svc_sp,0);
			return 0;
		}
		case SVC_UNBLOCK_TASK: {
			char *name=(char *)get_svc_arg(svc_sp,0);
			struct task *b=ready[MAX_PRIO];
			struct task * volatile *b_prev=&ready[MAX_PRIO];
			struct task *t=lookup_task_for_name(name);
			int prio;
			if (!t) {
				set_svc_ret(svc_sp,-1);
				return 0;
			}
			prio=t->prio_flags&0xf;
			if (prio>MAX_PRIO) prio=MAX_PRIO;
			if (prio<MAX_PRIO) {
				set_svc_ret(svc_sp,-1);
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
			set_svc_ret(svc_sp,-1);
			return 0;
set_ready:
			DEBUGP(DLEV_SCHED,"unblocking task: %s, current %s\n",t->name,current->name);
			t->next=0;
			SET_PRIO(t,t->prio_flags&3);
			if(ready[GET_PRIO(t)]) {
				ready_last[GET_PRIO(t)]->next=t;
			} else {
				ready[GET_PRIO(t)]=t;
			}
			ready_last[GET_PRIO(t)]=t;

			set_svc_ret(svc_sp,0);
			return 0;
		}
		case SVC_SETPRIO_TASK: {
			char *name=(char *)get_svc_arg(svc_sp,0);
			int prio=get_svc_arg(svc_sp,1);
			int curr_prio=GET_PRIO(current);
			struct task *t=lookup_task_for_name(name);
			if (!t) {
				set_svc_ret(svc_sp,-1);
				return 0;
			}
			if (prio>MAX_PRIO) {
				set_svc_ret(svc_sp,-2);
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
			set_svc_ret(svc_sp,0);
			return 0;
check_resched:
			if (prio<curr_prio) {
				DEBUGP(DLEV_SCHED,"setprio task: name %s prio=%d, current %s, reschedule\n",t->name,t->prio_flags,current->name);
				switch_on_return();
			}
			set_svc_ret(svc_sp,0);
			return 0;
		}
		case SVC_SETDEBUG_LEVEL: {
			unsigned int dl =get_svc_arg(svc_sp,0);
			if (dl>0) {
				set_svc_ret(svc_sp,-1);
			}
#ifdef DEBUG
			dbglev=dl;
			set_svc_ret(svc_sp,0);
#else
			set_svc_ret(svc_sp,-1);
#endif
			return 0;
		}
		case SVC_REBOOT: {
			unsigned int cookie =get_svc_arg(svc_sp,0);
			if (cookie==0x5a5aa5a5) {
				board_reboot();
			}
			set_svc_ret(svc_sp,-1);
			return 0;
		}
		case SVC_GETTIC: {
			set_svc_ret(svc_sp,tq_tic);
			return 0;
		}
		default:
			sys_printf("bad syscall %x\n",svc_number);
			set_svc_ret(svc_sp,-1);
			break;
	}
	return 0;
}


void *sys_sleep(unsigned int ms) {
	current->blocker.driver=0;
	sys_sleepon(&current->blocker,&ms);
	return 0;
}

struct tq *sys_timer(struct blocker *so, unsigned int ms) {
	int wtic=(ms/10)+tq_tic;
	struct tq *tout=&tq[wtic%1024];
	so->wakeup_tic=wtic;
	so->next=0;
	if (!tout->tq_out_first) {
		tout->tq_out_first=so;
		tout->tq_out_last=so;
	} else {
		tout->tq_out_last->next=so;
		tout->tq_out_last=so;
	}
	return tout;
}

static void sys_timer_remove(struct blocker *b) {
	struct tq *tout=&tq[b->wakeup_tic%1024];
	struct blocker *bp=tout->tq_out_first;
	struct blocker **bpp=&tout->tq_out_first;
	struct blocker *bprev=0;

	while(bp) {
		if (bp==b) {
			(*bpp)=bp->next;
			bp->next=0;
			if (bp==tout->tq_out_last) {
				tout->tq_out_last=bprev;
			}
			return;
		}
		bprev=bp;
		bpp=&bp->next;
		bp=bp->next;
	}
	b->next=0;
}

int device_ready(struct blocker *b) {
	if (b->ev&0x80) return 0;  // blocking from a list
	return b->driver->ops->control(b->dh, IO_POLL, (void *)b->ev, 0);
}

/* not to be called from irq, cant sleep */
void *sys_sleepon(struct blocker *so, unsigned int *tout) {
	unsigned long int cpu_flags;
	if (!task_sleepable()) return 0;
	cpu_flags=disable_interrupts();

	if (so->wake) {
		so->wake=0;
		restore_cpu_flags(cpu_flags);
		return 0;
	}
	if (so->driver && device_ready(so)) {
		restore_cpu_flags(cpu_flags);
		return 0;
	}
	if (current->state!=TASK_STATE_RUNNING) {
		restore_cpu_flags(cpu_flags);
		sys_printf("wanted to sleep, non running task\n");
		return 0;
	}

	current->state=TASK_STATE_IO; /* Do not do any more debug when state
					is running */
	restore_cpu_flags(cpu_flags);
	CLR_TMARK(current);

	if (tout&&*tout) {
		current->state=TASK_STATE_TIMER;
		sys_timer(so,*tout);
	} else {
		so->wakeup_tic=0;
	}

	DEBUGP(DLEV_SCHED,"sys sleepon %x task: %s\n", so, current->name);
	switch_now();

	so->ev&=~0x80; // Clear list sleep
	if (tout&&*tout) {
		if (so->wakeup_tic!=tq_tic) {
			*tout=(so->wakeup_tic-tq_tic)*10;
		}
		so->wakeup_tic=0;
	}
	DEBUGP(DLEV_SCHED,"sys sleepon woke up %x task: %s\n", so, current->name);
	return so;
}

void *sys_sleepon_update_list(struct blocker *b, struct blocker_list *bl_ptr) {
	void *rc=0;
	struct blocker **pprev;
	struct blocker *tmp;
	unsigned long int cpu_flags;
	if (current->state!=TASK_STATE_RUNNING) {
		return 0;
	}
	cpu_flags=disable_interrupts();
	if (!bl_ptr) goto out;
	if (bl_ptr->is_ready()) goto out;
	b->next2=0;
	if (bl_ptr->first) {
		bl_ptr->last->next2=b;
	} else {
		bl_ptr->first=b;
	}
	bl_ptr->last=b;
	b->ev|=0x80;
	restore_cpu_flags(cpu_flags);
	rc=sys_sleepon(b,0);
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
	restore_cpu_flags(cpu_flags);
	return rc;
}

void *sys_wakeup(struct blocker *so) {
	struct task *n = container_of(so,struct task, blocker);
	int prio;
	unsigned long int cpu_flags;

	if (!n) {
		return 0;
	}
	if (n==current) {
		so->wake=1;
		if (so->wakeup_tic) {
			sys_timer_remove(so);
		}
		return 0;
	}

	if (n->state==TASK_STATE_RUNNING) {
		so->wake=1;
		if (so->wakeup_tic) {
			sys_timer_remove(so);
		}
//		sys_printf("waking up running process\n");
//		ASSERT(0);
		return 0;
	}

	if (n->state==TASK_STATE_READY) {
//		sys_printf("waking up ready process\n");
//		ASSERT(0);
		return 0;
	}

	if (so->next2) {
		sys_printf("sys_wakeup:blocker has next2 link to %x in wakup\n", so->next2);
	}

	if (so->wakeup_tic) {
		sys_timer_remove(so);
	}

	if (so->next) { /* means we have timer objects still linked in */
		sys_printf("sys_wakeup:blocker %x has next link to %x in wakup\n", so, so->next);
	}


	cpu_flags=disable_interrupts();
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
	restore_cpu_flags(cpu_flags);
	
	if (prio<GET_PRIO(current)) {
		DEBUGP(DLEV_SCHED,"wakeup(immed readying) %x current task: %s, readying %s\n",so,current->name,n->name);
		switch_on_return();
		return n;
	}
	DEBUGP(DLEV_SCHED,"wakeup(readying) %x current task: %s, readying %s\n",so,current->name,n->name);
 	return n;
}

void *sys_wakeup_from_list(struct blocker_list *bl) {
	struct blocker *tmp;
	void *rc=0;
	unsigned long int cpu_flags=disable_interrupts();
	tmp=bl->first;
	while (bl->first) {
		tmp=bl->first;
		bl->first=tmp->next2;
		tmp->next2=0;
		restore_cpu_flags(cpu_flags);
		rc=sys_wakeup(tmp);
		cpu_flags=disable_interrupts();
//		ASSERT(rc);
	}
	restore_cpu_flags(cpu_flags);
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
		if (__builtin_strcmp(drv->name,tmp->name)==0) {
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
		if (__builtin_strcmp(name,d->name)==0) {
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


/*******************************************************************************/
/*  Sys support functions and unecessary stuff                                 */
/*                                                                             */

extern unsigned long int init_func_begin[];
extern unsigned long int init_func_end[];
typedef void (*ifunc)(void);

void init_sys(void) {
	unsigned long int *i;
	sys_printf("In init_sys\n");
	init_sys_arch();
	init_memory_protection();
#if 0
	slab_256=(struct Slab_256 *)((unsigned int)((slab_256+0xff))&~0xff);
	max_slab_256=(((unsigned int)slab_1024)-((unsigned int)slab_256))/0x100;
	unmap_stack_memory((unsigned long int)slab_1024);
	map_stack_page((unsigned long int)&slab_1024[15], 9); /* map in last stackpage, size 1k */
	__builtin_memset(&slab_1024[15],0,900);
#endif
	activate_memory_protection();
	init_task_list();
	init_switcher();
	init_irq();

	config_sys_tic(10);
	for(i=init_func_begin;i<init_func_end;i++) {
		((ifunc)*i)();
	}
	
	driver_init();
	driver_start();
}

extern const char *ver;
void start_up(void) {
        /* initialize the executive */
        init_sys();
        init_io();

        /* start the executive */
        sys_printf("notix git ver %s, starting tasks\n",ver);
        start_sys();
}

extern void sys_mon(void *);
extern int switch_flag;

extern int __usr_main(int argc, char **argv);

void start_sys(void) {
	
	unsigned long int stackp;
#ifdef MMU
	struct task *t=create_user_context();
	sys_printf("enter start_sys: got task ptr %x\n", t);
	load_init(t);
#else
	struct task *t=(struct task *)get_page();	
	memset(t,0,sizeof(struct task));
	if (allocate_task_id(t)<0) {
		sys_printf("proc table full\n");
		put_page(t);
		while(1);
	}
#endif
	
	t->name="sys_mon";
	t->state=TASK_STATE_READY;
	t->prio_flags=3;
	stackp=((unsigned long int)t)+4096;
	t->estack=((unsigned long int)t)+2048;
	stackp=stackp-8;
	strcpy((void *)stackp,"usart0");
	setup_return_stack(t,(void *)stackp,(unsigned long int)__usr_main,0, (void *)stackp, (void *)8);
	

	t->next2=troot;
	troot=t;

	if(!ready[GET_PRIO(t)]) {
		ready[GET_PRIO(t)]=t;
	} else {
		ready_last[GET_PRIO(t)]->next=t;
	}
	ready_last[GET_PRIO(t)]=t;
	sys_printf("leaving start_sys: got task ptr %x\n", t);
	clear_all_interrupts();
	switch_on_return();  /* will switch in the task on return from next irq */
	while(1);
	
//	thread_create(sys_mon,"usart0",0,3,"sys_mon");
////	thread_create(sys_mon,"stterm0",3,"sys_mon");
////	thread_create(sys_mon,"usb_serial0",2,"sys_mon");
}
