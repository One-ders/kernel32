#include "stm32/stm32f407.h"
#include "io.h"
#include "sys.h"

#define TASK_XPSR(a)    ((unsigned int *)a)[15]

#define MPU_RASR 0xe000eda0

static inline unsigned int  xpsr() {
	register unsigned int r0 asm("r0");
	__asm__ __volatile__("mrs r0,xpsr"::: "r0");
	return r0;
}

int task_sleepable(void) {
	int in_irq=xpsr()&0x1ff;
	if (in_irq&&(in_irq!=0xb)) return 0;
	return 1;
}


/*
 * PendSV handler
 */
void *PendSV_Handler_c(unsigned long int *svc_args);

void __attribute__ (( naked )) PendSV_Handler(void) {
	asm volatile (
		"tst lr,#4\n\t"
		"ite eq\n\t"
		"mrseq r0,msp\n\t"
		"mrsne r0,psp\n\t"
		"mov r1,lr\n\t"
		"mov r2,r0\n\t"
		"push {r1-r2}\n\t"
		"cpsie i\n\t"
		"bl PendSV_Handler_c\n\t"
		"pop {r1-r2}\n\t"
		"mov lr,r1\n\t"
		"cmp    r0,#0\n\t"
		"bne.n  1f\n\t"
		"bx lr\n\t"
		"1:\n\t"
		"cpsid i\n\t"
		"stmfd r2!,{r4-r11}\n\t"
		"ldr r1,.L1\n\t"
		"ldr r1,[r1]\n\t"
		"str r2,[r1,#4]\n\t"		/* save sp on prev task */
		"ldr r1,.L2\n\t"
		"ldr r2,[r1,#0]\n\t"
		"bic r2,r2,0x03000000\n\t"
		"str r2,[r1,#0]\n\t"		/* map out prev task mem */
		"ldr r1,.L1\n\t"
		"str r0,[r1,#0]\n\t"		/* store new task as current */
		"movs r2,#1\n\t"
		"str r2,[r0,#16]\n\t"		/* set current state running */
		"ldr r0,[r0,#4]\n\t"
		"mov sp,r0\n\t"			/* switch stack		*/
		"ldmfd sp!,{r4-r11}\n\t"
		"cpsie i\n\t"
		"bx lr\n\t"
		".L1: .word current\n\t"
		".L2: .word 0xe000eda0\n\t"
		:
		:
		:
	);
}

void *PendSV_Handler_c(unsigned long int *save_sp) {
	int i=0;
	struct task *t;
	sys_irqs++;

	disable_interrupt();
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
	enable_interrupt();

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
		disable_interrupt();
		current->state=TASK_STATE_READY;
		if (ready[prio]) {
			ready_last[prio]->next=current;
		} else {
			ready[prio]=current;
		}
		ready_last[prio]=current;
		enable_interrupt();
	}

	map_next_stack_page((unsigned long int)t->estack,
				(unsigned long int)current->estack);
	if ((TASK_XPSR(t->sp)&0x1ff)!=0) {
		*(save_sp-2)=0xfffffff1;
	} else {
		*(save_sp-2)=0xfffffff9;
	}
	return t;
}

void switch_on_return(void) {
	/* Trigger a pendsv trap */
	 *((unsigned int volatile *)0xE000ED04) = 0x10000000;
}

__attribute__ ((naked)) int fake_pendSV(void) {
	asm volatile (
		"mov  r0,sp\n\t"
		"ands r0,r0,0x7\n\t"
		"beq aligned\n\t"
		"mov  r0,sp\n\t"
		"and  r0,r0,0xfffffff8\n\t"
		"mov  sp,r0\n\t"
		"mrs r0,psr\n\t"
		"orr r0,r0,0x01000000\n\t"
		"orr r0,r0,0x00000200\n\t"
		"b  saveit\n\t"
		"aligned:\n\t"
		"mrs r0,psr\n\t"
		"orr r0,r0,0x01000000\n\t"
		"saveit:\n\t"
		"stmfd sp!,{r0}\n\t"
		"push {lr}\n\t"
		"stmfd sp!,{r0-r3,r12,lr}\n\t"
		"mvns lr,#0x0e\n\t"
		"b      PendSV_Handler\n\t"
		:::);
	return 1;
}

void switch_now(void) {
	fake_pendSV();
	if ((xpsr()&0x1ff)!=0x0b) {
		ASSERT(0);
	}
        *((volatile unsigned int *)0xe000ed24)|=0x80; /* set cpu state to
                                                        SVC call active */
}

void init_switcher(void) {
	NVIC_SetPriority(PendSV_IRQn,0xf);
#if 0
	/* dont align stack on 8 byte, use 4 byte */
        *((unsigned int *)0xe000ed14)&=~0x200;
	/* allow software to return to threadmode always */
        *((unsigned int *)0xe000ed14)|=1;
#endif
}
