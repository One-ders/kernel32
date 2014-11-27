#include "io.h"
#include "sys.h"


void set_asid(unsigned int asid) {
	asm volatile ("mtc0 $4, $10");
}


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
			set_asid(i);
			return i;
		}
	}
	return -1;
}

struct task *create_user_context(void) {
	struct task *t=(struct task *)get_page();
	memset(t,0,sizeof(struct task));
	t->sp=(void *)0x80000000;
	sys_printf("got page at %x\n",t);
	if (allocate_task_id(t)<0) {
		sys_printf("proc table full\n");
		put_page(t);
		return 0;
	}
	return t;
}

int task_sleepable(void) {
	return 1;
}


void switch_on_return(void) {
}

void switch_now(void) {
}

void init_switcher(void) {
	init_task_list();
}

void UsageFault_Handler(void) {
}

void Error_Handler_c(void *sp);

void HardFault_Handler(void) {
}

void Error_Handler_c(void *sp_v) {
}

void *SVC_Handler_c(unsigned long int *sp);
