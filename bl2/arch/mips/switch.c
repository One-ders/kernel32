#include "io.h"
#include "sys.h"
#include "string.h"
#include <mmu.h>

void do_switch(void);

volatile unsigned int irq_lev=0;
volatile unsigned int switch_flag=0;
volatile unsigned int hwr_ulr=0;


struct task *create_user_context(void) {
	struct task *t=(struct task *)get_page();
	unsigned long int *t_pgd=(unsigned long int *)get_page();
	struct address_space *asp=(struct address_space *)(t+1);
	memset(t,0,sizeof(struct task));
	memset(t_pgd,0,P_SIZE);
	memset(asp,0,sizeof(*asp));
	t->sp=(void *)(((char *)t)+P_SIZE);
	t->kheap=(unsigned long int)(asp+1);
//	sys_printf("got page at %x\n",t);
	if (allocate_task_id(t)<0) {
		sys_printf("proc table full\n");
		put_page(t);
		put_page(t_pgd);
		return 0;
	}
	t->asp=asp;
	asp->ref=1;
	asp->id=allocate_as_id();
	asp->mmap_vaddr=0x10000000;
	if (asp->id<0)  {
		put_page(t_pgd);
		put_page(t);
		return 0;
	}
//	set_asid(asp->id);
	asp->pgd=t_pgd;
	return t;
}

int incr_address_space_users(struct address_space *asp) {
	return ++asp->ref;
}

int decr_address_space_users(struct address_space *asp) {
	return --asp->ref;
}


unsigned long int handle_switch(unsigned long int *v_sp) {
	int i=0;
	struct task *t;
	sys_irqs++;
	sys_printf("handle_switch: sp %x\n", v_sp);

//	disable_interrupts();
	while(i<MAX_PRIO) {
		t=ready[i];
		if (t) {
			ready[i]=t->next;
			t->next=0;
			break;
		}
		i++;
	}
	if (t==current) {
		ASSERT(0);
	}
//	enable_interrupts();

	if (!t) {
		ASSERT(0);
	}
	if (t==current) {
		ASSERT(0);
	}

	if (current->blocker.wake) {
		/* IO blocked task, is ready, so    */
		/* fall through and set state ready */
		current->blocker.wake=0;
		current->state=TASK_STATE_RUNNING;
//		sys_printf("in pendsv: readying blocked task\n");
	}
	/* Have next, move away current if still here, 
	   timer and blocker move themselves */
	if (current->state==TASK_STATE_RUNNING) {
		int prio=current->prio_flags&0xf;
		ASSERT(!current->next);
		CLR_TMARK(current);
		if (prio>4) prio=4;
//		disable_interrupts();
		current->state=TASK_STATE_READY;
		if (ready[prio]) {
			ready_last[prio]->next=current;
		} else {
			ready[prio]=current;
		}
		ready_last[prio]=current;
//		enable_interrupts();
	}

        DEBUGP(DSYS_SCHED,DLEV_INFO, "switch in task: %s\n", t->name);

//	sys_printf("in switch: returning %x\n", t);
	return (unsigned long int)t;
}

int task_sleepable(void) {
	return (irq_lev==1?1:0);
}


void switch_on_return(void) {
	switch_flag=1;
}

void switch_now(void) {
	switch_flag=1;
	do_switch();
}

void init_switcher(void) {
//	init_task_list();
}

void UsageFault_Handler(void) {
}

void Error_Handler_c(void *sp);

void HardFault_Handler(void) {
}

void Error_Handler_c(void *sp_v) {
}

void *SVC_Handler_c(unsigned long int *sp);
