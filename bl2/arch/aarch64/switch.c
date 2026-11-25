#include "io.h"
#include "sys.h"
#include "string.h"

extern void serial_puts(const char *);
void do_switch(void);

volatile unsigned int irq_lev=0;
volatile unsigned int long switch_flag=0;

void *alloc_kheap(struct task *t, unsigned int size) {
	unsigned long int a=t->kheap;

//                           update to 4096 the kheap has a full page.
	if (a+size>(((unsigned long int)t->kheap & PAGE_MASK) + 2048)) {
		return 0;
        }
	t->kheap=a+size;
	return (void *)a;
}


/* The address space struct is located on the page of the first
 * task for a context. Tasks created in the same adderess space, refers to
 * this task.
 */
struct task *create_user_context(void) {
	struct task *t=(struct task *)get_page();
	struct address_space *asp=(struct address_space *)(t+1);
	unsigned long int *t_pgd=(unsigned long int *)get_page();
	memset(t,0,sizeof(struct task));
	memset(t_pgd,0,P_SIZE);
	memset(asp,0,sizeof(*asp));
	t->kheap=(unsigned long int)get_page();
	t->sp=(void *)(((char *)t->kheap)+P_SIZE);
	t->estack=(void *)(t->kheap+2048);
//	sys_printf("got page at %x\n",t);
	if (allocate_task_id(t)<0) {
		sys_printf("proc table full\n");
		put_page((void *)t->kheap);
		put_page(t);
		put_page(t_pgd);
		return 0;
	}
	t->asp=asp;
	asp->ref=1;
	asp->id=allocate_as_id();
	asp->mmap_vaddr=0x1000000;
	asp->brk=0x1000000;

	if (asp->id<0)  {
		dealloc_task_id(t->id);
		put_page((void *)t->kheap);
		put_page(t_pgd);
		put_page(t);
		return 0;
	}
//      set_asid(asp->id);
        asp->pgd=t_pgd;
        return t;
}

void delete_user_context(struct task *t) {
        struct address_space *asp=t->asp;
        dealloc_as_id(asp->id);
        dealloc_task_id(t->id);
        put_page(asp->pgd);
	put_page((void*)(t->kheap&PAGE_MASK));
        put_page(t);
}

int incr_address_space_users(struct address_space *asp) {
        return ++asp->ref;
}

int decr_address_space_users(struct address_space *asp) {
        return --asp->ref;
}

// create task from user mode, tca holds argument from calling process
int create_task(unsigned long int *svc_sp) {
	struct task_create_args *tca=
		(struct task_create_args *)get_svc_arg(svc_sp,0);
	struct task *t=(struct task *)get_page();
	unsigned long int kheap=(unsigned long int)get_page();
	unsigned char *estack=((unsigned char *)kheap)+2048;
	unsigned long int *stackp=(unsigned long int *)(estack+2048);
	unsigned char *uval=tca->val;
	char *targs[]={uval,(char *)0x0};
	char *environ[] = { (char *)0};

	if (!t) {
		set_svc_ret(svc_sp,-1);
		return 0;
	}

	memset(t,0,sizeof(struct task));

	t->kheap=(unsigned long int)kheap;

	if (allocate_task_id(t)<0) {
		sys_printf("proc table full\n");
		put_page((void *)kheap);
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
	setup_return_stack(t,(void *)stackp,(unsigned long int)tca->fnc,0,0x80000000,targs,environ);

	t->state=TASK_STATE_READY;
sys_printf("alloc_kheap from create_task\n");
	t->name=alloc_kheap(t,strlen(tca->name)+5);
	sys_sprintf(t->name,"%s:%03d:%03d",tca->name,t->asp->id,t->id);
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


void init_switcher(void) {
}

int task_sleepable(void) {
	return irq_lev==1?1:0;
}

/*
 * Called from assembly context restore after interrupts/traps.
 * Interrupts are disabled
 *
 */
unsigned long int handle_switch(unsigned long int *v_sp) {
	int i=0;
	struct task *t;
//      sys_irqs++;
//      sys_printf("handle_switch: sp %x\n", v_sp);

	while(i<=MAX_PRIO) {
		t=ready[i];
		if (t) {
			ready[i]=t->next;
			t->next=0;
			break;
		}
		i++;
	}

	if (t==current) {
		sys_printf("t==current==%s\n", current->name);
		ASSERT(0);
	}

	if (!t) {
		t=&idle_task;
#if 0
		sys_printf("no ready proc\n");
		ASSERT(0);
#endif
	} else {
		if (t==&idle_task) {
			sys_printf("idle task in job queue\n");
			ASSERT(0);
		}
	}

	if (t==current) {
		sys_printf("t==current==%s, again\n", current->name);
		ASSERT(0);
	}

	if (current->blocker.wake) {
		/* IO blocked task, is ready, so    */
		/* fall through and set state ready */
		current->blocker.wake=0;
		current->state=TASK_STATE_RUNNING;
	}
	/* Should not have next, move away current if still here,
		timer and blocker move themselves */
	if (current->state==TASK_STATE_RUNNING) {
		int prio=GET_PRIO(current);
		ASSERT(!current->next);
		CLR_TMARK(current);
		if ((!GET_BLCK_STATE(current))&&(current!=&idle_task)) {
			if (ready[prio]) {
				ready_last[prio]->next=current;
			} else {
				ready[prio]=current;
			}
			ready_last[prio]=current;
		}
		current->state=TASK_STATE_READY;
	}

	DEBUGP(DSYS_SCHED,DLEV_INFO, "switch in task: %s\n", t->name);

	return (unsigned long int)t;
}

void switch_on_return(void) {
	switch_flag=1;
}

void switch_now(void) {
	switch_flag=1;
	do_switch();
}

#define POLLPRINT(a) {io_setpolled(1);sys_printf(a);io_setpolled(0);}


#if 0
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
#endif

void *handle_syscall(unsigned long int *sp);
