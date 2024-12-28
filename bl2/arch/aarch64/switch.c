#include "io.h"
#include "sys.h"

extern void serial_puts(const char *);

volatile unsigned int irq_lev=0;
volatile unsigned int switch_flag=0;

#if 0
#define TASK_XPSR(a)    ((unsigned int *)a)[15]

#define MPU_RASR 0xe000eda0


static inline unsigned int  xpsr() {
	register unsigned int r0 asm("r0");
//	__asm__ __volatile__("mrs r0,xpsr"::: "r0");
	return r0;
}
#endif

void init_switcher(void) {
//	NVIC_SetPriority(PendSV_IRQn,0xf);
#if 0
	/* dont align stack on 8 byte, use 4 byte */
        *((unsigned int *)0xe000ed14)&=~0x200;
	/* allow software to return to threadmode always */
        *((unsigned int *)0xe000ed14)|=1;
#endif
}

int task_sleepable(void) {
//	int in_irq=xpsr()&0x1ff;
//	if (in_irq&&(in_irq!=0xb)) return 0;
	return 1;
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
#if 0
	switch_flag=1;
	do_switch();
#endif
}

int decr_address_space_users(struct address_space *asp) {
	return --asp->ref;
}

struct task *create_user_context(void) {
	return 0;
}

void delete_user_context(struct task *t) {
	return;
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
