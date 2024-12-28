/* $TSOS: , v1.1 2014/04/07 21:44:00 anders Exp $ */

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
#include "io.h"
#include "sys.h"

#include <nand.h>

#include <string.h>

#include <yaffsfs.h>

#define TQ_SIZE 1024

void sys_thread_exit(int status);
int sys_getdents64(unsigned int fd,
                        void *dirp,
                        unsigned int count);
static int block_task_t(struct task *t);
static int unblock_task_t(struct task *t);

struct iovec {
	void *iov_base;
	size_t iov_len;
};

unsigned long int sys_irqs;

struct task * volatile ready[MAX_PRIO+1];
struct task * volatile ready_last[MAX_PRIO+1];

struct task * volatile blocked_list;
struct task * volatile blocked_list_last;

struct task * task_cemetery;

struct task *troot =&idle_task;
struct task * volatile current = &idle_task;


#if 0
struct task *t_array[256];

void init_task_list() {
        int i;
        for(i=0;i<255;i++) {
                t_array[i]=0;
        }
}
#endif

CREATE_BITMAP(t_bp,512);
CREATE_BITMAP(as_bp,256);

int allocate_task_id(struct task *t) {
        t->id=bitmap_alloc_first_free(t_bp);
	return t->id;
}

void dealloc_task_id(int id) {
	bitmap_dealloc(t_bp,id);
}

int allocate_as_id() {
	return bitmap_alloc_first_free(as_bp);
}

void dealloc_as_id(int id)  {
	bitmap_dealloc(as_bp,id);
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
} tq[TQ_SIZE];

volatile unsigned long int tq_tic;

void SysTick_Handler(void) {
	struct tq *tqp;
	int p=5;

	current->active_tics++;
	sys_irqs++;
#if 0
	if (!(tq_tic%100)) {
		sys_printf("sys_tic: +100, tic: %d current %s\n",tq_tic,current->name);
	}
#endif
	tq_tic++;
	tqp=&tq[tq_tic%TQ_SIZE];

#if DEBUG
	if ((trace_sys&DSYS_TIMER)&&(trace_lev>DLEV_INFO)) {
		if (!(tq_tic%100)) {
			sys_printf("sys_tic: +100, tic: %x current %s\n",tq_tic,current->name);
		}
	}
#endif

	enable_interrupts();

	if (tqp->tq_out_first) {
		struct blocker *b=tqp->tq_out_first;
		while(b) {
			unsigned long int cpu_flags;
			struct blocker *bnext=b->next;
			struct task *t=container_of(b,struct task,blocker);
			int prio=GET_PRIO(t);
			if (GET_BLCK_STATE(t)) {
				sys_printf("blocked task in tqueue\n");
				ASSERT(0);
			}
			if (t==&idle_task) {
				sys_printf("idle task in tqueue\n");
				ASSERT(0);
			}
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
			if (p<GET_PRIO(current)||(current==&idle_task)) {
				switch_on_return();
			}
			restore_cpu_flags(cpu_flags);
			DEBUGP(DSYS_SCHED,DLEV_INFO,"timer_wakeup: readying %s\n", t->name);
			b=bnext;
		}
		tqp->tq_out_first=tqp->tq_out_last=0;
	}


	disable_interrupts();
	if (current!=&idle_task) {
		if (GET_TMARK(current) && ready[GET_PRIO(current)]) {
			DEBUGP(DSYS_SCHED,DLEV_INFO, "time slice: switch out %s, switch in %s\n", current->name, ready[GET_PRIO(current)]->name);
			switch_on_return();
			enable_interrupts();
			return;
		}
		SET_TMARK(current);
	}

	if (task_cemetery) {
		struct task *t;
		for(t=task_cemetery;t;t=t->next) {
			sys_printf("found task: %s in cemetry\n", t->name);
			if (t->sp!=0) {
				task_cemetery=t->next;
			} else {
				sys_printf("task still not swiched out\n");
			}
		}
	}
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

#if 0
#define FDTAB_SIZE 16

struct user_fd fd_tab[FDTAB_SIZE] = {{&console,0,},
					{&console,0,},
					{&console,0,}, };

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
#endif

struct user_fd fd_con = {&console,0,};

int get_user_fd(struct address_space *asp, struct driver *d, struct device_handle *dh) {
	int i;
	for(i=1;i<FDTAB_SIZE;i++) {
		if (!asp->fd_tab[i].driver) {
			asp->fd_tab[i].driver=d;
			asp->fd_tab[i].dev_handle=dh;

			asp->fd_tab[i].next=current->fd_list;
			asp->fd_list=&asp->fd_tab[i];
			return i;
		}
	}
	return -1;
}


int close_drivers(struct task *t) {
	struct user_fd *fdl=t->asp->fd_list;

	t->asp->fd_list=0;
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
	struct user_fd *fdl=current->asp->fd_list;
	struct user_fd **fdprev=&current->asp->fd_list;

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
		DEBUGP(DSYS_SCHED,DLEV_INFO,"wakeup descriptor %d in task %s for ev %d\n", fd, task->name,ev);
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
		if (!(task->blocker.ev&ev)) {
			sys_printf("sys_drv_wakeup: masked. b.ev(%x) r.ev(%x)\n",task->blocker.ev,ev);
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

void *alloc_kheap(struct task *t, unsigned int size) {
	unsigned long int a=t->kheap;
	if (a+size>(((unsigned long int)t)+2048)) {
		sys_printf("allocating outside kheap\n");
		return 0;
	}
	t->kheap=a+size;
	return (void *)a;
}

static int create_task(unsigned long int *svc_sp) {
	struct task_create_args *tca=
		(struct task_create_args *)get_svc_arg(svc_sp,0);
	struct task *t=(struct task *)get_page();
	unsigned char *estack=((unsigned char *)t)+2048;
	unsigned long int *stackp=(unsigned long int *)(estack+2048);
	unsigned char *uval=tca->val;
	char *targs[]={uval,(char *)0x0};
	char *environ[] = { (char *)0};

	if (!t) {
		set_svc_ret(svc_sp,-1);
		return 0;
	}

	memset(t,0,sizeof(struct task));

	t->kheap=(unsigned long int)(t+1);

	if (allocate_task_id(t)<0) {
		sys_printf("proc table full\n");
		put_page(t);
		set_svc_ret(svc_sp,-1);
		return 0;
	}

	if (tca->prio>MAX_PRIO) {
		set_svc_ret(svc_sp,-1);
		return 0;
	}
	__builtin_memset(estack,0,2048);
	t->estack=estack;
#if 0
	if (tca->val_size) {
		int aval_size=(tca->val_size+7)&~0x7;
		uval=((unsigned char *)stackp)-aval_size;
		memcpy(uval,tca->val,tca->val_size);
		stackp=(unsigned long int *)uval;
	}
#endif
	t->asp=current->asp;
	t->asp->ref++;
	setup_return_stack(t,(void *)stackp,(unsigned long int)tca->fnc,0,targs,environ);

	t->state=TASK_STATE_READY;
	t->name=alloc_kheap(t,strlen(tca->name)+5);
	sys_sprintf(t->name,"%s:%03d",tca->name,t->asp->id);
	SET_PRIO(t,tca->prio);
	t->next2=troot;
	troot=t;

	if(!ready[tca->prio]) {
		ready[tca->prio]=t;
	} else {
		ready_last[tca->prio]->next=t;
	}
	ready_last[tca->prio]=t;

	if (tca->prio<GET_PRIO(current)) {
		DEBUGP(DSYS_SCHED,DLEV_INFO,"Create task: %s, switch out %s\n", t->name, current->name);
		switch_on_return();
	} else {
		DEBUGP(DSYS_SCHED,DLEV_INFO,"Create task: %s\n", t->name);
	}

	set_svc_ret(svc_sp,0);
	return 0;
}

static struct task *create_task_sa(struct task_create_args *tca) {
	struct task *t=(struct task *)get_page();
	unsigned char *estack=((unsigned char *)t)+2048;
	unsigned long int *stackp=(unsigned long int *)(estack+2048);
	unsigned char *uval=tca->val;
	char *targs[]={uval,(char *)0x0};
	char *environ[] = { (char *)0};

	if (!t) {
		sys_printf("get page -> 0\n");
		return 0;
	}

	memset(t,0,sizeof(struct task));

	t->kheap=(unsigned long int)(t+1);

	if (allocate_task_id(t)<0) {
		sys_printf("proc table full\n");
		put_page(t);
		return 0;
	}

	if (tca->prio>MAX_PRIO) {
		return 0;
	}
	__builtin_memset(estack,0,2048);
	t->estack=estack;
#if 0
	if (tca->val_size) {
		int aval_size=(tca->val_size+7)&~0x7;
		uval=((unsigned char *)stackp)-aval_size;
		memcpy(uval,tca->val,tca->val_size);
		stackp=(unsigned long int *)uval;
	}
#endif
	t->asp=current->asp;
	t->asp->ref++;
	setup_return_stack(t,(void *)stackp,(unsigned long int)tca->fnc,0,targs,environ);

	t->state=TASK_STATE_READY;
	t->name=alloc_kheap(t,strlen(tca->name)+5);
	sys_sprintf(t->name,"%s:%03d",tca->name,t->asp->id);
	SET_PRIO(t,tca->prio);
	t->next2=troot;
	troot=t;

	if(!ready[tca->prio]) {
		ready[tca->prio]=t;
	} else {
		ready_last[tca->prio]->next=t;
	}
	ready_last[tca->prio]=t;

	if (tca->prio<GET_PRIO(current)) {
		DEBUGP(DSYS_SCHED,DLEV_INFO,"Create task: %s, switch out %s\n", t->name, current->name);
		switch_on_return();
	} else {
		DEBUGP(DSYS_SCHED,DLEV_INFO,"Create task: %s\n", t->name);
	}

	return t;
}


static int fork(unsigned long int *svc_sp) {
	struct task *t;
	unsigned char *estack;
	unsigned long int *stackp;
	int prio=GET_PRIO(current);
	unsigned long int cpu_flags;
	int rc;

	t=create_user_context();
	if (!t) {
		set_svc_ret(svc_sp,-1);
		return 0;
	}
	t->asp->brk=current->asp->brk;
	rc=share_process_pages(t,current);
	if (rc<0) {
		delete_user_context(t);
		set_svc_ret(svc_sp,-1);
		return 0;
	}

	stackp=(void *)(((unsigned long int)t)+4096);
	t->estack=(void *)(((unsigned long int)t)+2048);
	t->prio_flags=current->prio_flags;

	t->asp->fd_tab[0]=current->asp->fd_tab[0];
	t->asp->fd_tab[1]=current->asp->fd_tab[1];
	t->asp->fd_tab[2]=current->asp->fd_tab[2];

	cpu_flags=disable_interrupts();
	estack=t->estack;
	__builtin_memcpy(estack,current->estack,2048);
	t->sp=(((unsigned long int)svc_sp)-((unsigned long int)current->estack))+t->estack;

	t->state=TASK_STATE_READY;
	t->name=alloc_kheap(t,strlen(current->name)+1);
	if (!t->name) {
		sys_printf("could not get space for task name\n");
	}
	strcpy(t->name,current->name);
	*__builtin_strchr(t->name,':')=0;
	sys_sprintf(t->name,"%s:%03d",t->name,t->asp->id);
	t->next2=troot;
	troot=t;

	/* setup parent relation */
	t->asp->parent=current->asp;
	/* update currents child list */
	t->asp->next=current->asp->child;
	current->asp->child=t->asp;

	if(!ready[prio]) {
		ready[prio]=t;
	} else {
		ready_last[prio]->next=t;
	}
	ready_last[prio]=t;

	restore_cpu_flags(cpu_flags);
	set_svc_ret(t->sp,0);
	set_svc_ret(svc_sp,t->id);
	return 0;
}

static int delete_thread(struct task *t) {
	int rc;
	unsigned long int cpu_flags=disable_interrupts();
	if (t->blocker.wakeup_tic) {
		sys_timer_remove(&t->blocker);
	}

	rc=block_task_t(t);
	if (!rc) {
		sys_printf("removed %s, from ready queue\n",t->name);
	}
	t->state=TASK_STATE_DEAD;
	close_drivers(t);
	sys_printf("removing task %s\n",t->name);
	t->asp=0;

	t->next=task_cemetery;
	task_cemetery=t;
	restore_cpu_flags(cpu_flags);
	return 0;
}

/* address space: similar to unix process
 * task: similar to unix thread.
 */
static int process_terminated(struct address_space *asp) {
	struct address_space *child=asp->child;
	struct address_space *pchild=asp->parent->child;
	struct address_space **ppchild=&asp->parent->child;
	struct task *t;

	/* remove ourself from parents child list */
        /* and add our children to parents child list */
	while(pchild) {
		if (pchild==asp) {
			*ppchild=pchild->next;
		} else {
			ppchild=&pchild->next;
		}
		pchild=pchild->next;
	}

	*ppchild=asp->child;

	/* go through child list, make children point to our
	 * parent, in order to link our self out.
	 */

	while(child) {
		child->parent=asp->parent;
		child=child->next;
	}

	// wakeup any threads in parent process that blocked on this
	t=troot;
	while(t) {
		if (t->asp==asp->parent) {
			if (t->state==TASK_STATE_WAIT) {
				t->state=TASK_STATE_READY;
				{
					struct task *b=blocked_list;
					t->next=b;
					blocked_list=t;
					unblock_task_t(t);
				}
			}
		}
		t=t->next2;
	}
}

static int kill_proc(unsigned long int *svc_sp) {
	char *name=(char *)get_svc_arg(svc_sp,0);
	struct task *t=lookup_task_for_name(name);
	struct task *t1=troot;
	struct task **tprev=&troot;
	struct address_space as;
	struct address_space *asp;
	int    killself=0;
	unsigned long int cpu_flags;

	DEBUGP(DSYS_SCHED,DLEV_INFO,"kill proc task: %s\n", t->name);

	if (!t) {
		set_svc_ret(svc_sp,-1);
		return 0;
	}

	asp=t->asp; /* get the address space pointer */

	if (!asp->parent) {
		sys_printf("cant kill top process\n");
		set_svc_ret(svc_sp,-1);
		return 0;
	}

// Doing extreeme interrupts disable, needs to be addressed later.
	cpu_flags=disable_interrupts();
	while(t1) {
		struct task *tnext=t1->next2;
		if (t1->asp==asp) {
			struct address_space *a=(struct address_space *)(t1+1);
			if (a==asp) {
				sys_printf("defering delete of main process leader task\n");
			} else {
				if (t1==current) {
					killself=1;
					sys_printf("kill self 1\n");
				}
				delete_thread(t1);
				*tprev=t1->next2;
				t1->next2=0;
			}
		} else {
			tprev=&t1->next2;
		}
		t1=tnext;
	}
	restore_cpu_flags(cpu_flags);

	/* restart scan, now there shall only be the proces leader left */
	t1=troot;
	tprev=&troot;

	cpu_flags=disable_interrupts();
	while(t1) {
		struct task *tnext=t1->next2;
		if (t1->asp==asp) {
			struct address_space *a=(struct address_space *)(t1+1);
			if (a==asp) {
				sys_printf("delete of main process leader task\n");
				if (t1==current) {
					killself=1;
					sys_printf("kill self 2\n");
				}
				delete_thread(t1);
				*tprev=t1->next2;
				t1->next2=0;
				sys_printf("maybe destroy as, %x\n",a->pgd);
				if (a->pgd) {  // destroy asp if not already done
					process_terminated(a);
					free_asp_pages(a);
					put_page(a->pgd);
					a->pgd=0;
					dealloc_as_id(a->id);
				}
			} else {
				sys_printf("a task struct without asp, error error\n");
			}
		} else {
			tprev=&t1->next2;
		}
		t1=tnext;
	}
	restore_cpu_flags(cpu_flags);

	if (killself) {
		current->sp=0;  // zero out old stack ptr, the switch will save current sp, so we can monitor
			   // when it is out.
		switch_on_return();
	}
	return 0;
}

static int sleep_ms(unsigned long int *svc_sp) {
	unsigned int tout=get_svc_arg(svc_sp,0);
	if (current->state!=TASK_STATE_RUNNING) return 0;
	DEBUGP(DSYS_SCHED,DLEV_INFO,"sleep current task: %s\n", current->name);
	current->blocker.driver=0;
	sys_sleepon(&current->blocker,(unsigned int *)&tout);
	set_svc_ret(svc_sp,tout);
	return 0;
}

static int block_task(unsigned long int *svc_sp) {
	char *name=(char *)get_svc_arg(svc_sp,0);
	struct task *t=lookup_task_for_name(name);
	if (!t) {
		set_svc_ret(svc_sp,-1);
		return 0;
	}

	if (t==&idle_task) {
		sys_printf("block idle_task\n");
		ASSERT(0);
	}
	if (t==current) {
		SET_BLCK_STATE(t);
		DEBUGP(DSYS_SCHED,DLEV_INFO,"blocking current task: %s\n",current->name);
		sys_printf("blocking current task: %s\n",current->name);
		switch_on_return();
	} else {
		struct task *p=ready[GET_PRIO(t)];
		struct task * volatile *p_prev=&ready[GET_PRIO(t)];
		SET_BLCK_STATE(t);
		sys_printf("blocking  task: %s\n",t->name);
		while(p) {
			if (p==t) {
				(*p_prev)=p->next;
				p->next=0;
				if (!blocked_list){
					blocked_list=p;
				} else {
					blocked_list_last->next=p;
				}
				blocked_list_last=p;
				break;
			}
			p_prev=&p->next;
			p=p->next;
		}
		DEBUGP(DSYS_SCHED,DLEV_INFO,"blocking task: %s, current %s\n",t->name,current->name);
	}
	set_svc_ret(svc_sp,0);
	return 0;
}

static int block_task_t(struct task *t) {
	if (!t) {
		return -1;
	}

	if (t==&idle_task) {
		sys_printf("block idle_task\n");
		ASSERT(0);
	}

	if (t==current) {
		SET_BLCK_STATE(t);
		DEBUGP(DSYS_SCHED,DLEV_INFO,"blocking current task: %s\n",current->name);
		switch_on_return();
		return 0;
	} else {
		struct task *p=ready[GET_PRIO(t)];
		struct task * volatile *p_prev=&ready[GET_PRIO(t)];
		SET_BLCK_STATE(t);
		while(p) {
			if (p==t) {
				(*p_prev)=p->next;
				p->next=0;
				if (!blocked_list){
					blocked_list=p;
				} else {
					blocked_list_last->next=p;
				}
				blocked_list_last=p;
				break;
			}
			p_prev=&p->next;
			p=p->next;
		}
		DEBUGP(DSYS_SCHED,DLEV_INFO,"blocking task: %s, current %s\n",t->name,current->name);
		return 0;
	}
	return -1;
}


static int unblock_task(unsigned long int *svc_sp) {
	char *name=(char *)get_svc_arg(svc_sp,0);
	struct task *b=blocked_list;
	struct task * volatile *b_prev=&blocked_list;
	struct task *t=lookup_task_for_name(name);
	int prio;
	if (!t) {
		set_svc_ret(svc_sp,-1);
		return 0;
	}

	if (!GET_BLCK_STATE(t)) {
		set_svc_ret(svc_sp,-1);
		return 0;
	}

	if (t==&idle_task) {
		sys_printf("unblock idle_task\n");
		ASSERT(0);
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
	DEBUGP(DSYS_SCHED,DLEV_INFO,"unblocking task: %s, current %s\n",t->name,current->name);
	t->next=0;
	CLR_BLCK_STATE(t);
	if(ready[GET_PRIO(t)]) {
		ready_last[GET_PRIO(t)]->next=t;
	} else {
		ready[GET_PRIO(t)]=t;
	}
	ready_last[GET_PRIO(t)]=t;

	set_svc_ret(svc_sp,0);
	return 0;
}

static int unblock_task_t(struct task *t) {
	struct task *b=blocked_list;
	struct task * volatile *b_prev=&blocked_list;
	int prio;
	if (!t) {
		return -1;
	}
	if (!GET_BLCK_STATE(t)) {
		return -1;
	}

	if (t==&idle_task) {
		sys_printf("unblock idle_task\n");
		ASSERT(0);
	}

	while(b) {
		if (b==t) {
			(*b_prev)=b->next;
			goto set_ready;
		}
		b_prev=&b->next;
		b=b->next;
	}
	return -1;

set_ready:
	DEBUGP(DSYS_SCHED,DLEV_INFO,"unblocking task: %s, current %s\n",t->name,current->name);
	t->next=0;
	CLR_BLCK_STATE(t);
	if(ready[GET_PRIO(t)]) {
		ready_last[GET_PRIO(t)]->next=t;
	} else {
		ready[GET_PRIO(t)]=t;
	}
	ready_last[GET_PRIO(t)]=t;

	return 0;
}


static int setprio_task(unsigned long int *svc_sp) {
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
				DEBUGP(DSYS_SCHED,DLEV_INFO,"degrading current prio task: name %s\n",current->name);
				switch_on_return();
			}
		}
	} else {
		if (GET_BLCK_STATE(t)) {
			int tprio=GET_PRIO(t);
			struct task *p=blocked_list;
			struct task * volatile *p_prev=&blocked_list;
			SET_PRIO(t,prio);
			while(p) {
				if (p==t) {
					(*p_prev)=p->next;
					p->next=0;
					if (!blocked_list){
						blocked_list=p;
					} else {
						blocked_list_last->next=p;
					}
					blocked_list_last=p;
					break;   // changed prio on blocked task
				}
				p_prev=&p->next;
				p=p->next;
			}
		} else {
			int tprio=GET_PRIO(t);
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
		}
		DEBUGP(DSYS_SCHED,DLEV_INFO,"setprio task: name %s prio=%d, current %s\n",t->name,t->prio_flags,current->name);
	}
	set_svc_ret(svc_sp,0);
	return 0;

check_resched:
	if (prio<curr_prio) {
		DEBUGP(DSYS_SCHED,DLEV_INFO,"setprio task: name %s prio=%d, current %s, reschedule\n",t->name,t->prio_flags,current->name);
		switch_on_return();
	}
	set_svc_ret(svc_sp,0);
	return 0;
}

static int wait(unsigned long int *svc_sp) {
	char *name=(char *)get_svc_arg(svc_sp,0);
	block_task_t(current);
	current->state=TASK_STATE_WAIT;
	set_svc_ret(svc_sp,0);
	return 0;
}

/*
 * io routines
 */

static int io_open(unsigned long int *svc_sp) {
	struct driver *drv;
	struct device_handle *dh;
	char *drvname=(char *)get_svc_arg(svc_sp,0);
	int  ufd;
	drv=driver_lookup(drvname);
	if (!drv) {
		set_svc_ret(svc_sp,-1);
		sys_printf("SVC_IO_OPEN: return -1 no drv:\n");
		return 0;
	}
	ufd=get_user_fd(current->asp,((struct driver *)0xffffffff),0);
	if (ufd<0) {
		set_svc_ret(svc_sp,-1);
//		sys_printf("SVC_IO_OPEN: return -1 get user\n");
		return 0;
	}
	dh=drv->ops->open(drv->instance,sys_drv_wakeup,current);
	if (!dh) {
		detach_driver_fd(&current->asp->fd_tab[ufd]);
		set_svc_ret(svc_sp,-1);
		sys_printf("SVC_IO_OPEN: return -1 drv open\n");
		return 0;
	}
	dh->user_data1=ufd;
	current->asp->fd_tab[ufd].driver=drv;
	current->asp->fd_tab[ufd].dev_handle=dh;
	current->asp->fd_tab[ufd].flags=0;
	set_svc_ret(svc_sp,ufd);
	return 0;
}

static int io_read(unsigned long int *svc_sp) {
	int rc;
	struct user_fd *fdd;
	struct driver *driver;
	struct device_handle *dh;
	int fd=(int)get_svc_arg(svc_sp,0);
	if (fd<0) {
		set_svc_ret(svc_sp,-1);
		return 0;
	}
	fdd=&current->asp->fd_tab[fd];
	driver=fdd->driver;
	dh=fdd->dev_handle;
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

static int io_write(unsigned long int *svc_sp) {
	int rc=0;
	int done=0;
	struct user_fd *fdd;
	struct driver *driver;
	struct device_handle *dh;
	int fd=(int)get_svc_arg(svc_sp,0);
	if (fd<0) {
		set_svc_ret(svc_sp,-1);
		return 0;
	}
	fdd=&current->asp->fd_tab[fd];
	driver=fdd->driver;
	dh=fdd->dev_handle;
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
	if (rc>=0) {
		done+=rc;
		if ((done!=get_svc_arg(svc_sp,2))&&(!(fdd->flags&O_NONBLOCK))) {
			goto again;
		}
	}
	set_svc_ret(svc_sp,done);
	return 0;
}

static int io_control(unsigned long int *svc_sp) {
	int	ufd;
	int	rc;
	struct	user_fd		*fdd;
	struct	driver		*driver;
	struct	device_handle	*dh;
	int	fd	=(int)get_svc_arg(svc_sp,0);
	if (fd<0) {
		set_svc_ret(svc_sp,-1);
		return 0;
	}
	fdd=&current->asp->fd_tab[fd];
	driver=fdd->driver;
	dh=fdd->dev_handle;

	if (!driver) {
		set_svc_ret(svc_sp,-1);
		return 0;
	}

	if (get_svc_arg(svc_sp,1)==DYNOPEN) {
		struct dyn_open_args doargs;

		doargs.name=(char *)get_svc_arg(svc_sp,2);
		ufd=get_user_fd(current->asp,((struct driver *)0xffffffff),0);
		if (ufd<0) {
			set_svc_ret(svc_sp,-1);
			return 0;
		}

		rc=driver->ops->control(dh,DYNOPEN,(void *)&doargs,get_svc_arg(svc_sp,3));
		if (rc<0) {
			detach_driver_fd(&current->asp->fd_tab[ufd]);
			set_svc_ret(svc_sp,-1);
			return 0;
		}
		doargs.dh->user_data1=ufd;
		current->asp->fd_tab[ufd].driver=driver;
		current->asp->fd_tab[ufd].dev_handle=doargs.dh;
		current->asp->fd_tab[ufd].flags=0;
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

static int io_lseek(unsigned long int *svc_sp) {
	int rc;
	int fd=(int)get_svc_arg(svc_sp,0);
	struct driver *driver;
	struct device_handle *dh;

	if (fd<0) {
		set_svc_ret(svc_sp,-1);
		return 0;
	}

	driver=current->asp->fd_tab[fd].driver;
	dh=current->asp->fd_tab[fd].dev_handle;
	if (!driver) {
		set_svc_ret(svc_sp,-1);
		return 0;
	}
	rc=driver->ops->control(dh,IO_LSEEK,(void *)get_svc_arg(svc_sp,1),get_svc_arg(svc_sp,2));
	set_svc_ret(svc_sp,rc);
	return 0;
}

static int io_select(unsigned long int *svc_sp) {
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
			struct driver *driver=current->asp->fd_tab[i].driver;
			struct device_handle *dh=current->asp->fd_tab[i].dev_handle;
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
			struct driver *driver=current->asp->fd_tab[i].driver;
			struct device_handle *dh=current->asp->fd_tab[i].dev_handle;
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
			struct driver *driver=current->asp->fd_tab[i].driver;
			struct device_handle *dh=current->asp->fd_tab[i].dev_handle;
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

static int io_close(unsigned long int *svc_sp) {
	int fd=(int)get_svc_arg(svc_sp,0);
	struct driver *driver=current->asp->fd_tab[fd].driver;
	struct device_handle *dh=current->asp->fd_tab[fd].dev_handle;
	int rc;
	if (!driver) {
		set_svc_ret(svc_sp,-1);
		return 0;
	}

	detach_driver_fd(&current->asp->fd_tab[fd]);

	rc=driver->ops->close(dh);
	set_svc_ret(svc_sp,rc);
	return 0;
}

static int io_mmap(unsigned long int *svc_sp) {
	void *addr=(void *)get_svc_arg(svc_sp,0);
	unsigned int len=(unsigned int)get_svc_arg(svc_sp,1);
	int prot=(int)get_svc_arg(svc_sp,2);
	int flags=(int)get_svc_arg(svc_sp,3);
	long int offset=(long int)get_svc_arg(svc_sp,5);
	unsigned long int paddr;
	unsigned long int vaddr;
	unsigned long int start_vaddr;
	unsigned int npages;
	int i;
	int rc;
	struct driver *driver;
	struct device_handle *dh;
	int fd=(int)get_svc_arg(svc_sp,4);
	if (fd<0) {
		set_svc_ret(svc_sp,-1);
		return 0;
	}
	driver=current->asp->fd_tab[fd].driver;
	dh=current->asp->fd_tab[fd].dev_handle;

	sys_printf("IO_MMAP: addr=%x, len=%d, prot=%d, flags=%d, fd=%d, offset=%d\n",
				addr,len,prot,flags,fd,offset);
	if (!driver) {
		set_svc_ret(svc_sp,0);
		return 0;
	}
	npages=((len-1)/PAGE_SIZE)+1;
	paddr=driver->ops->control(dh,IO_MMAP,(void *)offset,len);
	vaddr=get_mmap_vaddr(current,len);

	sys_printf("IO_MMAP: need %d pages, vaddr %x, paddr %x\n", npages, vaddr, paddr);
			start_vaddr=vaddr;
	for(i=0;i<npages;i++) {
		rc=mapmem(current,vaddr,paddr, MAP_NO_CACHE | MAP_WRITE);
		if (rc<0) {
			set_svc_ret(svc_sp,0);
			return 0;
		}
		vaddr+=PAGE_SIZE;
		paddr+=PAGE_SIZE;
	}
	set_svc_ret(svc_sp,start_vaddr);
	return 0;
}

static int io_munmap(unsigned long int *svc_sp) {
	unsigned long int vaddr=(unsigned long int)get_svc_arg(svc_sp,0);
	unsigned int len=(unsigned int)get_svc_arg(svc_sp,1);
	unsigned int npages;
	int i;

	npages=((len-1)/PAGE_SIZE)+1;
	for(i=0;i<npages;i++) {
		int rc=unmapmem(current,vaddr);
		if (rc<0) {
			set_svc_ret(svc_sp,0);
			return 0;
		}
		vaddr+=PAGE_SIZE;
	}
	set_svc_ret(svc_sp,0);
	return 0;
}

static int set_debug_level(unsigned long int *svc_sp) {
	unsigned int dl =get_svc_arg(svc_sp,0);
	if (dl>0) {
		set_svc_ret(svc_sp,-1);
	}
#ifdef DEBUG
	trace_sys=dl>>16;
	trace_lev=dl&0xffff;
	set_svc_ret(svc_sp,0);
#else
	set_svc_ret(svc_sp,-1);
#endif
	return 0;
}

/* Linux syscall emulation, for musl */

extern struct driver yaffsfs_drv;

struct yaffsfs_Handle {
	short int fdId;
	short int useCount;
};

static int ufd2yaffs(int ufd) {
	struct driver *driver=current->asp->fd_tab[ufd].driver;
	struct device_handle *dh=current->asp->fd_tab[ufd].dev_handle;
	struct yaffsfs_Handle *yf=(struct yaffsfs_Handle *)dh;

	return yf->fdId;
}

static int lnx_open(unsigned long int *svc_sp) {
	char *path=(char *)get_svc_arg(svc_sp,0);
	int  flags=(int)get_svc_arg(svc_sp,1);
	int  mode=(int)get_svc_arg(svc_sp,2);
	int fd=0;

	fd=sys_open(path,flags,mode);
	if (fd<0) {
		set_svc_ret(svc_sp, -yaffsfs_GetLastError());
		set_svc_lret(svc_sp,-1);
	} else {
		void *dh=yaffsfs_HandleToPointer(fd);
		int ufd=get_user_fd(current->asp,&yaffsfs_drv,dh);
		set_svc_ret(svc_sp,ufd);
		set_svc_lret(svc_sp,0);
	}
	return 0;
}

static int lnx_read(unsigned long int *svc_sp) {
	int fd=(int)get_svc_arg(svc_sp,0);
	void *buf=(void *)get_svc_arg(svc_sp,1);
	size_t count=get_svc_arg(svc_sp,2);
	int rc=0;
	if (fd>2) {
		rc=sys_read(ufd2yaffs(fd),buf,count);
	}

	if (rc<0) {
		set_svc_ret(svc_sp, -yaffsfs_GetLastError());
		set_svc_lret(svc_sp,-1);
	} else {
		set_svc_ret(svc_sp,rc);
		set_svc_lret(svc_sp,0);
	}

	return 0;
}

static int lnx_close(unsigned long int *svc_sp) {
	int fd=(int)get_svc_arg(svc_sp,0);
	int rc=sys_close(ufd2yaffs(fd));
	set_svc_ret(svc_sp,rc);
	set_svc_lret(svc_sp,0);
	return 0;
}

static int lnx_execve(unsigned long int *svc_sp) {
	char *path=(char *)get_svc_arg(svc_sp,0);
	char npath[strlen(path + 1)];
	char **argv=(char **)get_svc_arg(svc_sp,1);
//	char **envp=(char **)get_svc_arg(svc_sp,2);
	int nr_args=array_size(argv);
	int argv_storage_size=args_size(argv);
	char *nargv[nr_args+1];
	char argv_storage[argv_storage_size];
	struct address_space *asp=current->asp;
	char *envp[]={ NULL };

	// copy stuff we need to kernel space
	strcpy(npath,path);

	copy_arguments(nargv,argv,argv_storage,nr_args);
	nargv[nr_args]=0;

	// forget all about this process
	free_asp_pages(asp);
	if (load_binary(npath)<0) {
		sys_printf("must kill self\n");
	}

#if 0
	current->kheap=((unsigned char *)current)+(sizeof(struct task)+sizeof(struct address_space));
	current->name=alloc_kheap(current,strlen(argv[0])+5);
	sys_sprintf(current->name,"%s:%03d",argv[0],current->asp->id);
#endif
	setup_return_stack(current, (void *)(((unsigned long int)svc_sp)+148), 0x40000, 0, nargv, envp);

	return 0;
}

static int lnx_brk(unsigned long int *svc_sp) {
	unsigned long int nbrk=(unsigned long int)get_svc_arg(svc_sp,0);
	unsigned long int ret=0;
	if (!nbrk) {
		ret=(unsigned long int)sys_sbrk(current,0);
		set_svc_ret(svc_sp,ret);
		set_svc_lret(svc_sp,0);
	} else {
		int rc=sys_brk(current,(void *)nbrk);
		if (!rc) {
			set_svc_ret(svc_sp,nbrk);
			set_svc_lret(svc_sp,0);
		} else {
			set_svc_ret(svc_sp,rc);
			set_svc_lret(svc_sp,rc);
		}
	}
	return 0;
}

static int lnx_ioctl(unsigned long int *svc_sp) {
	int fd=(int)get_svc_arg(svc_sp,0);
	int cmd=(int)get_svc_arg(svc_sp,1);
	if (cmd!=TIOCGWINSZ) {
		sys_printf("ioctl: got cmd %x\n", cmd);
	}
	set_svc_lret(svc_sp,-1);
	return 0;
}

static int lnx_writev(unsigned long int *svc_sp) {
	int fd=(int)get_svc_arg(svc_sp,0);
	struct iovec	*msg_iov=(void *)get_svc_arg(svc_sp,1);
	size_t		msg_iovlen=get_svc_arg(svc_sp,2);
	struct user_fd *fdd=&current->asp->fd_tab[fd];
	struct driver *driver=fdd->driver;
	struct device_handle *dh=fdd->dev_handle;
	int rc=0;
	int i;
	int total_done=0;

	if (!driver) {
		set_svc_ret(svc_sp,-1);
		set_svc_lret(svc_sp,0);
		return 0;
	}

	current->blocker.dh=dh;
	current->blocker.ev=EV_WRITE;

	for(i=0;i<msg_iovlen&&msg_iov[i].iov_base;i++) {
		int done=0;
again_wr:
		rc=driver->ops->control(dh,WR_CHAR,msg_iov[i].iov_base+done,msg_iov[i].iov_len-done);

		if (rc==-DRV_AGAIN&&(!(fdd->flags&O_NONBLOCK))) {
			current->blocker.driver=driver;
			sys_sleepon(&current->blocker,0);
			goto again_wr;
		}

		if (rc==-DRV_INPROGRESS&&(!(fdd->flags&O_NONBLOCK))) {
			int result;
			current->blocker.driver=driver;
			sys_sleepon(&current->blocker,0);
			rc=driver->ops->control(dh,WR_GET_RESULT,&result,sizeof(result));
			if (rc<0) {
				set_svc_ret(svc_sp,rc);
				set_svc_lret(svc_sp,0);
			} else {
				set_svc_ret(svc_sp,result+total_done);
				set_svc_lret(svc_sp,0);
			}
			return 0;
		}
		if (rc>=0) {
			done+=rc;
			if ((done!=msg_iov[i].iov_len)&&(!(fdd->flags&O_NONBLOCK))) {
				goto again_wr;
			}
		}
		total_done+=done;
	}
	set_svc_ret(svc_sp,total_done);
	set_svc_lret(svc_sp,0);
	return 0;
}

static int lnx_nsleep(unsigned long int *svc_sp) {
	struct timespec *req=
	  (struct timespec *)get_svc_arg(svc_sp,0);
	struct timespec *rem=
	  (struct timespec *)get_svc_arg(svc_sp,1);
	unsigned long int ms;
	ms=(req->tv_sec*1000)+
		(req->tv_nsec/1000000);
	if (current->state!=TASK_STATE_RUNNING) return 0;
	DEBUGP(DSYS_SCHED,DLEV_INFO,"sleep current task: %s\n", current->name);
	current->blocker.driver=0;
	sys_sleepon(&current->blocker,(unsigned int *)&ms);
	set_svc_ret(svc_sp,0);
	set_svc_lret(svc_sp,0);
	return 0;
}

static int lnx_wait(unsigned long int *svc_sp) {
	int *wstatus=
	  (int *)get_svc_arg(svc_sp,0);
	if (current->state!=TASK_STATE_RUNNING) return 0;
	DEBUGP(DSYS_SCHED,DLEV_INFO,"wait for child task: %s\n", current->name);
	block_task_t(current);
	current->state=TASK_STATE_WAIT;
	set_svc_ret(svc_sp,0);
	set_svc_lret(svc_sp,0);
	return 0;
}


static int lnx_mmap2(unsigned long int *svc_sp) {
	unsigned long int addr=(unsigned long int)get_svc_arg(svc_sp,0);
	unsigned long int length=(unsigned long int)get_svc_arg(svc_sp,1);
	unsigned long int prot=(unsigned long int)get_svc_arg(svc_sp,2);
	unsigned long int flags=(unsigned long int)get_svc_arg(svc_sp,3);
	unsigned long int fd=(unsigned long int)get_svc_arg(svc_sp,4);
	unsigned long int pgoffset=(unsigned long int)get_svc_arg(svc_sp,5);

	sys_printf("mmap: addr %x, len %x, prot %x, flags %x, fd %x, pgoffset %x\n",
			addr, length, prot, flags, fd, pgoffset);

	set_svc_ret(svc_sp, 8);
	set_svc_lret(svc_sp,-1);

	return 0;
}

static int lnx_stat(unsigned long int *svc_sp) {
	char *path=(char *)get_svc_arg(svc_sp,0);
	struct stat *stbuf=(struct stat *)get_svc_arg(svc_sp,1);
	int rc;

	rc=sys_stat(path,stbuf);
	if (rc<0) {
		set_svc_ret(svc_sp, -yaffsfs_GetLastError());
		set_svc_lret(svc_sp,-1);
	} else {
		set_svc_ret(svc_sp,rc);
		set_svc_lret(svc_sp,0);
	}
	return 0;
}

static int lnx_lstat(unsigned long int *svc_sp) {
	char *path=(char *)get_svc_arg(svc_sp,0);
	struct stat *stbuf=(struct stat *)get_svc_arg(svc_sp,1);
	int rc;

	rc=sys_lstat(path,stbuf);
	if (rc<0) {
		set_svc_ret(svc_sp, -yaffsfs_GetLastError());
		set_svc_lret(svc_sp,-1);
	} else {
		set_svc_ret(svc_sp,rc);
		set_svc_lret(svc_sp,0);
	}
	return 0;
}


static int lnx_dirent64(unsigned long int *svc_sp) {
	int fd=(int)get_svc_arg(svc_sp,0);
	void *dirp64=(void *)get_svc_arg(svc_sp,1);
	int count=(int)get_svc_arg(svc_sp,2);
	int rc=sys_getdents64(ufd2yaffs(fd),dirp64,count);
	set_svc_ret(svc_sp,rc);
	set_svc_lret(svc_sp,0);
	return 0;
}

static int lnx_fcntl(unsigned long int *svc_sp) {
	int fd=(int)get_svc_arg(svc_sp,0);
	int cmd=(int)get_svc_arg(svc_sp,1);
	unsigned long int p1=(unsigned long int)get_svc_arg(svc_sp,2);
	unsigned long int p2=(unsigned long int)get_svc_arg(svc_sp,3);
	int rc;
	rc=sys_fcntl(ufd2yaffs(fd),cmd,p1,p2);
	set_svc_ret(svc_sp,rc);
	set_svc_lret(svc_sp,0);
	return 0;
}

void *handle_syscall(unsigned long int *svc_sp) {
	unsigned int sys_domain;
	unsigned int svc_number;

	sys_irqs++;
	/*
	 * Stack contains:
	 * ! for cortex M
	 * r0, r1, r2, r3, r12, r14, the return address and xPSR
	 * First argument (r0) is svc_args[0]
	 */

	svc_number=get_svc_number(svc_sp,&sys_domain);
	if (sys_domain==SYSCALL_NATIVE) {
		switch(svc_number) {
			case SVC_CREATE_TASK: {
				create_task(svc_sp);
				return 0;
				break;
			}
			case SVC_KILL_SELF: {
				if (current->state!=TASK_STATE_RUNNING) return 0;

				close_drivers(current);
				current->state=TASK_STATE_DEAD;

				DEBUGP(DSYS_SCHED,DLEV_INFO,"kill self task: %s\n", current->name);
				switch_on_return();
				return 0;
			}
			case SVC_KILL_PROC: {
				kill_proc(svc_sp);
				return 0;
			}
			case SVC_MSLEEP: {
				sleep_ms(svc_sp);
				return 0;
			}
			/* IO functions */
			case SVC_IO_OPEN: {
				io_open(svc_sp);
				return 0;
			}
			case SVC_IO_READ: {
				io_read(svc_sp);
				return 0;
			}
			case SVC_IO_WRITE: {
				io_write(svc_sp);
				return 0;
			}
			case SVC_IO_CONTROL: {
				io_control(svc_sp);
				return 0;
			}
			case SVC_IO_LSEEK: {
				io_lseek(svc_sp);
				return 0;
			}
			case SVC_IO_CLOSE: {
				io_close(svc_sp);
				return 0;
			}
			case SVC_IO_SELECT: {
				io_select(svc_sp);
				return 0;
			}
			case SVC_IO_MMAP: {
				io_mmap(svc_sp);
				return 0;
			}
			case SVC_IO_MUNMAP: {
				io_munmap(svc_sp);
				return 0;
			}
			case SVC_BLOCK_TASK: {
				block_task(svc_sp);
				return 0;
			}
			case SVC_UNBLOCK_TASK: {
				unblock_task(svc_sp);
				return 0;
			}
			case SVC_SETPRIO_TASK: {
				setprio_task(svc_sp);
				return 0;
			}
			case SVC_SETDEBUG_LEVEL: {
				set_debug_level(svc_sp);
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
			case SVC_SBRK: {
#ifdef MMU
				long int incr=get_svc_arg(svc_sp,0);
				void *ret=sys_sbrk(current,incr);
				set_svc_ret(svc_sp,(unsigned long)ret);
#else
				set_svc_ret(svc_sp,0);
#endif
				return 0;
			}
			case SVC_BRK: {
#ifdef MMU
				void *nbrk=(void *)get_svc_arg(svc_sp,0);
				int ret=sys_brk(current,nbrk);
				set_svc_ret(svc_sp,ret);
#else
				set_svc_ret(svc_sp,-1);
#endif
				return 0;
			}
			case SVC_FORK: {
				fork(svc_sp);
				return 0;
			}
			case SVC_EXIT: {
				int status=(int)get_svc_arg(svc_sp,0);
				sys_thread_exit(status);
				current->sp=0;  // mark it so we can see when switcher has moved it out
				switch_on_return();
				return 0;
			}
			case SVC_WAIT: {
				wait(svc_sp);
				return 0;
			}
			default:
				io_setpolled(1);
				sys_printf("%s: bad syscall %x\n",current->name,svc_number);
				set_svc_ret(svc_sp,-1);
				while(1);
				break;
		}
	} else if (sys_domain==SYSCALL_LNX) {
		switch(svc_number) {
			case LNX_READ: {  // Linux read
				lnx_read(svc_sp);
				return 0;
			}
			case LNX_OPEN: {  // Linux open
				lnx_open(svc_sp);
				return 0;
			}
			case LNX_CLOSE: {  // Linux close
				lnx_close(svc_sp);
				return 0;
			}
			case LNX_EXECVE: {  // Linux execve
				lnx_execve(svc_sp);
				return 0;
			}
			case LNX_BRK: { // Linux sys brk
				lnx_brk(svc_sp);
				return 0;
			}
			case LNX_IOCTL: {  // Linux ioctl
				lnx_ioctl(svc_sp);
				return 0;
			}
			case LNX_WAIT4: { // Linux wait
				lnx_wait(svc_sp);
				return 0;
			}
			case LNX_WRITEV: {	// Linux writev
				lnx_writev(svc_sp);
				return 0;
			}
			case LNX_NANOSLEEP: { // Linux nano sleep
				lnx_nsleep(svc_sp);
				return 0;
			}
			case LNX_MMAP2: { // Linux mmap2
				lnx_mmap2(svc_sp);
				return 0;
			}
			case LNX_STAT64: { // Linux stat
				lnx_stat(svc_sp);
				return 0;
			}
			case LNX_LSTAT64: { // Linux lstat
				lnx_lstat(svc_sp);
				return 0;
			}
			case LNX_GETDENTS64: { // Linux dirent64
				lnx_dirent64(svc_sp);
				return 0;
			}
			case LNX_FCNTL64: { // Linux fcntl
				lnx_fcntl(svc_sp);
				return 0;
			}
			default:
				io_setpolled(1);
				sys_printf("%s: bad lnx syscall %x\n",current->name,svc_number);
				set_svc_ret(svc_sp,-1);
				while(1);
				break;
		}
	} else {
		io_setpolled(1);
		sys_printf("%s: bad syscall domain %x\n",current->name,sys_domain);
		set_svc_ret(svc_sp,-1);
		while(1);
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

extern unsigned int irq_lev;

/* not to be called from irq, cant sleep */
void *sys_sleepon(struct blocker *so, unsigned int *tout) {
	unsigned long int cpu_flags;
	if (!task_sleepable()) {
		DEBUGP(DSYS_SCHED,DLEV_INFO,"sleep called from irq: %s, irq_lev %d\n", current->name,irq_lev);
		return 0;
	}
	cpu_flags=disable_interrupts();

	if (so->wake) {
		so->wake=0;
		DEBUGP(DSYS_SCHED,DLEV_INFO,"sleep called with wake up marked: %s\n", current->name);
		restore_cpu_flags(cpu_flags);
		return 0;
	}
	if (so->driver && device_ready(so)) {
		DEBUGP(DSYS_SCHED,DLEV_INFO,"sleep called with device ready: %s\n", current->name);
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
//	restore_cpu_flags(cpu_flags);
	CLR_TMARK(current);

	if (tout&&*tout) {
		current->state=TASK_STATE_TIMER;
		sys_timer(so,*tout);
	} else {
		so->wakeup_tic=0;
	}

	DEBUGP(DSYS_SCHED,DLEV_INFO,"sys sleepon %x task: %s\n", so, current->name);
	switch_now();

// Moved to architecture return, assymetric and uggly!!
//	/* this is a thread newly awaken, clear out the 'list sleep' flag,
//	 * it could have been set before sleep... */
//	so->ev&=~0x80; // Clear list sleep
	if (tout&&*tout) {
		if (so->wakeup_tic!=tq_tic) {
			*tout=(so->wakeup_tic-tq_tic)*10;
		}
		so->wakeup_tic=0;
	}
//	DEBUGP(DLEV_SCHED,"sys sleepon woke up %x task: %s\n", so, current->name);
	return so;
}

// Call with interrupts off
// a blocker list is typically used by a common resource, like a device driver for
// a serial interface. Many processes may access the driver, if the driver can not perform
// a requested service, because of waiting for some external state, the process will be put to
// sleep. Each thread has a blocker object. The driver will have a blocker_list, where it put
// the blocker of each blocked thread.
//
void *sys_sleepon_update_list(struct blocker *b, struct blocker_list *bl_ptr) {
	void *rc=0;
	struct blocker **pprev;
	struct blocker *tmp;
	unsigned long int cpu_flags;
	if (current->state!=TASK_STATE_RUNNING) {
		return 0;
	}

	if (!bl_ptr) goto out;
	if (bl_ptr->is_ready()) goto out;
	b->next2=0;
	if (bl_ptr->first) {
		bl_ptr->last->next2=b;
	} else {
		bl_ptr->first=b;
	}
	bl_ptr->last=b;

	/* secondary blocking from a device we depend on,
	 * mask out the first level device, otherwise a wake
	 * up will try to wake up wrong thread */
	b->ev|=0x80;
	rc=sys_sleepon(b,0);
	if (!rc) {
		/* return 0, means no sleep */
		/* link out the blocker */
		cpu_flags=disable_interrupts();
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
		restore_cpu_flags(cpu_flags);
		if (tmp==b) {
			sys_printf("sleep_list: failed, linked out blocker\n");
		} else {
			sys_printf("sleep_list: failed, blocker not found\n");
		}
	}
out:
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
		sys_printf("waking up running not current process\n");
		ASSERT(0);
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
	prio=GET_PRIO(n);

	if (GET_BLCK_STATE(n)) {
		sys_printf("wakeup blocked proc\n");
		ASSERT(0);
	}

	if (n==&idle_task) {
		sys_printf("wakeup idle_task\n");
		ASSERT(0);
	}

	if (ready[prio]) {
		ready_last[prio]->next=n;
	} else {
		ready[prio]=n;
	}
	ready_last[prio]=n;

	if ((current==&idle_task)||(prio<GET_PRIO(current))) {
		DEBUGP(DSYS_SCHED,DLEV_INFO,"wakeup(immed readying) %x current task: %s, readying %s\n",so,current->name,n->name);
		switch_on_return();
		restore_cpu_flags(cpu_flags);
		return n;
	}
	restore_cpu_flags(cpu_flags);
	DEBUGP(DSYS_SCHED,DLEV_INFO,"wakeup(readying) %x current task: %s, readying %s\n",so,current->name,n->name);
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
/*                                                         */
/***********************************************************/

void sys_thread_exit(int status) {
	struct task **prevp;
	struct task *i;
	int nt=0;
	unsigned long int cpu_flags;

// For now, disable interrupts during this job,
// on jz4750d hw, we get timesliced otherwise.
// which means 2 sys_tics, ~20mS!!!
// Or uart runs synch.... Look into later
	cpu_flags=disable_interrupts();
	current->state=TASK_STATE_DEAD;

	prevp=&troot;
	i=troot;

	// link task out from main task list
	while(i) {
		if (i==current) {
			*prevp=current->next2;
			break;
		}
		prevp=&i->next2;
		i=i->next2;
	}

	// link task out from ready list (should never be there)
	if (GET_BLCK_STATE(current)) {
		sys_printf("running blocked task\n");
		ASSERT(0);
	}
	prevp=(struct task **)&ready[GET_PRIO(current)];
	i=ready[GET_PRIO(current)];

	while(i) {
		if (i==current) {
			sys_printf("found task %s in ready list at exit\n",
					current->name);
			*prevp=current->next;
			break;
		}
		prevp=&i->next;
		i=i->next;
	}

	current->next=0;

	if (current->blocker.wakeup_tic) {
		sys_timer_remove(&current->blocker);
		sys_printf("task %s, blocked on timer\n", current->name);
	}

	if (current->blocker.driver) {
		sys_printf("task %s, blocked from driver\n", current->name);
	}

	nt=decr_address_space_users(current->asp);

	if(!nt) {
		process_terminated(current->asp);
		close_drivers(current);
		free_asp_pages(current->asp);
		put_page(current->asp->pgd);
		current->asp->pgd=0;
		dealloc_as_id(current->asp->id);
	}

	current->next=task_cemetery;
	task_cemetery=current;
	restore_cpu_flags(cpu_flags);
}

/***********************************************************/
/* DBG                                                     */
/***********************************************************/
#ifdef DEBUG
int trace_lev=DEFAULT_TRACE_LEV;
int trace_sys=DEFAULT_TRACE_SYS;
#endif

#define DRV_SYS_STAT_COLD	0
#define DRV_SYS_STAT_INITED 	1
#define DRV_SYS_STAT_STARTED 	2
static int drv_sys_stat=0;


int driver_publish(struct driver *drv) {
	struct driver *p=drv_root;
//	sys_printf("driver_publish: %s\n", drv->name);
	while(p&&p!=drv) p=p->next;
	if (!p) {
		drv->next=drv_root;
		drv_root=drv;
	}

	if (drv_sys_stat) {
		/* next two blocks are for late registers */
		if (!(drv->stat&DRV_SYS_STAT_INITED)) {
			drv->stat|=DRV_SYS_STAT_INITED;
			if (drv->ops->init) {
				drv->ops->init(drv->instance);
			}
		}

		if (!(drv->stat&DRV_SYS_STAT_STARTED)) {
			drv->stat|=DRV_SYS_STAT_STARTED;
			if (drv->ops->start) {
				drv->ops->start(drv->instance);
			}
		}
	}

	return 0;
}

int driver_init() {
	struct driver *d=drv_root;

	if (drv_sys_stat&DRV_SYS_STAT_INITED) {
		return 0;
	}
	drv_sys_stat|=DRV_SYS_STAT_INITED;
	while(d) {
		if (d->ops->init&&!(d->stat&DRV_SYS_STAT_INITED)) {
			d->stat|=DRV_SYS_STAT_INITED;
			d->ops->init(d->instance);
		}
		d=d->next;
	}
	return 0;
}

int driver_start() {
	struct driver *d=drv_root;
	if (drv_sys_stat&DRV_SYS_STAT_STARTED) {
		return 0;
	}
	drv_sys_stat|=DRV_SYS_STAT_STARTED;
	while(d) {
		if (d->ops->start&&!(d->stat&DRV_SYS_STAT_STARTED)) {
			d->stat|=DRV_SYS_STAT_STARTED;
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

void driver_user_data_init(struct device_handle **root,
			struct device_handle *handlers,
			unsigned int num_handlers) {
	int i;
	struct device_handle **p=root;
	for(i=0;i<num_handlers;i++) {
		(*p)=&handlers[i];
		p=&handlers[i].next;
	}
}

struct device_handle *driver_user_get_udata(struct device_handle *root) {
	struct device_handle *res;
	if ((res=root->next)) {
		root->next=root->next->next;
		return res;
	}
	return 0;
}

void driver_user_put_udata(struct device_handle *root,
				struct device_handle *dh) {
	dh->next=root->next;
	root->next=dh;
}



/**********************************************/

static struct driver *my_uart;
static struct device_handle *my_uart_dh;

static struct device_handle *console_open(void *driver_instance, DRV_CBH cb, void *dum) {
	return 0;
}

static int console_close(struct device_handle *dh) {
	return 0;
}

static int console_control(struct device_handle *dh, int cmd, void *arg1, int arg2) {
	int rc=0;
		if (my_uart_dh) {
			rc=my_uart->ops->control(my_uart_dh,cmd,arg1,arg2);
		}
	return rc;
}

static int console_init(void *instance) {
	return 0;
}

static int console_start(void *instance) {
	my_uart=driver_lookup(CFG_CONSOLE_DEV);
	if (!my_uart) {
		sys_printf("dont have uart\n");
		return 0;
	}
	my_uart_dh=my_uart->ops->open(my_uart->instance,0,0);
	return 0;
}


/*******************************************************************************/
/*  Sys support functions and unecessary stuff                                 */
/*                                                                             */

extern void serial_puts(char *);
extern unsigned long int init_func_begin[];
extern unsigned long int init_func_end[];
typedef void (*ifunc)(void);

void init_sys(void) {
	unsigned long int *i;
	init_sys_arch();
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
        init_io();
        init_sys();
        init_io();

        /* start the executive */
        sys_printf("tsos git ver %s, starting tasks\n",ver);
	enable_interrupts();
        start_sys();
}

extern void sys_mon(void *);
extern int switch_flag;

//extern int __usr_main(int argc, char **argv);
extern void *usr_init;

int mount_nand(char *nand_dev_name);

int dump_page(unsigned char *buf, int len, char *prefix) {
	int i=0;
	sys_printf("\nStart of dump\n");

	while (i<len) {
		if ((i%16)==0) {
			sys_printf("%s%08x:", prefix,i);
		}

		sys_printf(" %02x", buf[i]);

		if ((i+1)%16) {
			sys_printf(",");
		} else {
			sys_printf("\n");
		}
		i++;
	}
}

extern int __usr_main(int argc, char **argv);
void start_sys(void) {

	unsigned long int stackp;
	struct task *t;
	char *targs[]={CFG_CONSOLE_DEV, (char *)0x0};
	char *environ[]={(char *)0x0};

#ifdef MMU
#if 0
	struct driver *nand_drv=driver_lookup(CFG_ROOT_FS_DEV);

	if (nand_drv) {
		struct device_handle *dh;
		unsigned char buf[4096];
		dh=nand_drv->ops->open(nand_drv->instance,0,0);
		if (dh) {
			memset(buf,9,4096);
			nand_drv->ops->control(dh,
						NAND_READ_RAW,
						buf,
						2048+64);
			dump_page(buf,2048,"Data: ");
			dump_page(buf+2048,64,"OOB Data: ");
		}
	} else {
		sys_printf("did not find : driver nand4\n");

	}
#endif
	if(mount_nand(CFG_ROOT_FS_DEV)<0) {
		sys_printf("could not mount nand\n");
	}

	t=create_user_context();
	if (!t) {
		struct task_create_args tca;

#ifdef GNURP
		memset(&tca,0,sizeof(tca));
		tca.val=CFG_CONSOLE_DEV;
		tca.prio=3;
		tca.fnc=__usr_main;
		tca.name="grrrk";
		t=create_task_sa(&tca);
#else
		while(1);
#endif
	}

	if (!t) {
		sys_printf("no task created\n");
	}

	sys_printf("load_init:\n");
	load_init(t);
#else
	t=(struct task *)get_page();
	memset(t,0,sizeof(struct task));
	if (allocate_task_id(t)<0) {
		sys_printf("proc table full\n");
		put_page(t);
		while(1);
	}
#endif

	t->name=alloc_kheap(t,strlen("sys_mon")+5);
	sys_sprintf(t->name,"%s:%03d","sys_mon",t->asp->id);
	t->state=TASK_STATE_READY;
	t->prio_flags=3;  // Prio 3, not blocked, not scheduled for tslice switch
	stackp=((unsigned long int)t)+4096;
	t->estack=(void *)(((unsigned long int)t)+2048);
	setup_return_stack(t,(void *)stackp,(unsigned long int)usr_init,0, targs, environ);

	disable_interrupts();

	t->asp->fd_tab[0]=fd_con;
	t->asp->fd_tab[1]=fd_con;
	t->asp->fd_tab[2]=fd_con;

	t->asp->next=current->asp->child;
	current->asp->child=t->asp;
	t->asp->parent=current->asp;

	t->next2=troot;
	troot=t;

	if(!ready[GET_PRIO(t)]) {
		ready[GET_PRIO(t)]=t;
	} else {
		ready_last[GET_PRIO(t)]->next=t;
	}
	ready_last[GET_PRIO(t)]=t;
	switch_on_return();  /* will switch in the task on return from next irq */
	clear_all_interrupts();
	while(1) {
		enable_interrupts();
		wait_irq();
	}

//	thread_create(sys_mon,"usart0",0,3,"sys_mon");
////	thread_create(sys_mon,"stterm0",3,"sys_mon");
////	thread_create(sys_mon,"usb_serial0",2,"sys_mon");
}
