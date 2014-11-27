#include <regdef.h>

struct fullframe {
	unsigned int zero;
	unsigned int at;
	unsigned int v0;
	unsigned int v1;
	unsigned int a0;
	unsigned int a1;
	unsigned int a2;
	unsigned int a3;
	unsigned int t0;
	unsigned int t1;
	unsigned int t2;
	unsigned int t3;
	unsigned int t4;
	unsigned int t5;
	unsigned int t6;
	unsigned int t7;
	unsigned int s0;
	unsigned int s1;
	unsigned int s2;
	unsigned int s3;
	unsigned int s4;
	unsigned int s5;
	unsigned int s6;
	unsigned int s7;
	unsigned int t8;
	unsigned int t9;
	unsigned int k0;
	unsigned int k1;
	unsigned int gp;
	unsigned int sp;
	unsigned int fp;
	unsigned int ra;

	unsigned int cp0_status;
	unsigned int cp0_cause;
	unsigned int cp0_epc;
	unsigned int lo;
	unsigned int hi;
};

void set_c0_compare(unsigned int val) {
	__asm__ __volatile__("mtc0\t%z0, $11\n\t"
			: : "Jr" (val));
}


static unsigned int compare_increment;
static unsigned int compare;

void config_sys_tic(unsigned int ms) {
	compare_increment=1680000;
	compare=read_c0_count();
	compare+=compare_increment;
	set_c0_compare(compare);
}

int irqs=0;
void irq_dispatch(void *sp) {
	struct fullframe *ff=(struct fullframe *)sp;
	unsigned int cause=ff->cp0_cause;

//	sys_printf("in irq: cause %x\n", ff->cp0_cause);

	if (cause&CP0_CAUSE_IP7) {
		if (cause&CP0_CAUSE_TI) {
			irqs++;
#if 0
			if (!(irqs%100)) {
				sys_printf("timer irq: cause %x\n", cause);
			}
#endif
			compare+=compare_increment;
			set_c0_compare(compare);
			SysTick_Handler();
		}
	}
}
