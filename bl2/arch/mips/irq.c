#include <regdef.h>
#include <frame.h>
#include <irq.h>
#include <devices.h>
#include <io.h>
#include <sys.h>

unsigned int read_c0_count(void);

struct IH_DATA {
	IRQ_HANDLER handler;
	void		*h_data;
};

static struct IH_DATA handlers[32];

static unsigned int installed_mask;

int install_irq_handler(int irq_num,
			IRQ_HANDLER irq_handler,
			void *handler_data) {

	if (!((1<<irq_num)&VALID_IRQS)) {
		sys_printf("someone installing invalid irq handler\n");
		return -1;
	}
	if (irq_handler) {
		handlers[irq_num].handler=irq_handler;
		handlers[irq_num].h_data=handler_data;
		installed_mask|=(1<<irq_num);
		INTC->icmcr=(1<<irq_num);
	} else {
		INTC->icmsr=(1<<irq_num);
		installed_mask&=~(1<<irq_num);
		handlers[irq_num].handler=0;
	}
//	INTC->icmr=~installed_mask;
	return 0;
}

#if 0
void set_c0_compare(unsigned int val) {
	__asm__ __volatile__("mtc0\t%z0, $11\n\t"
			: : "Jr" (val));
}
#endif

void set_c0_compare(unsigned int val);

unsigned long int get_c0_status() {
	unsigned long int ret=0;
	__asm__ __volatile__("mfc0\t%z0, $12\n\t"
			: "=r" (ret));
	return ret;
}

void set_c0_status(unsigned int val) {
	__asm__ __volatile__("mtc0\t%z0, $12\n\t"
			: : "Jr" (val));
}

unsigned long int get_c0_cause() {
	unsigned long int ret=0;
	__asm__ __volatile__("mfc0\t%z0, $13\n\t"
			: "=r" (ret));
	return ret;
}


unsigned int compare_increment;
#if 0
static unsigned int compare;

void config_sys_tic(unsigned int ms) {
	compare_increment=1680000;
	compare=read_c0_count();
	compare+=compare_increment;
	set_c0_compare(compare);
	set_c0_status(get_c0_status()|CP0_CAUSE_IP7);
	sys_printf("config_sys_tic: status %x, count %x, compare %x\n",
		get_c0_status(), read_c0_count(), read_c0_compare());
}
#endif

void irq_dispatch(void *sp) {
	struct fullframe *ff=(struct fullframe *)sp;
//	unsigned int cause=ff->cp0_cause;
	unsigned int cause=get_c0_cause();
	volatile static int fot=0;
	unsigned int cirq=INTC->icpr;

//	sys_printf("in irq: cause %x, irq_lev %x, current cause %x\n", cause, irq_lev, get_c0_cause());

	INTC->icmr=cirq;
// jz-qemu bug
#ifdef QEMU
	INTC->icpr=cirq;
#endif

#if 1
	if (cause&CP0_CAUSE_IP7) {
//		set_c0_status(get_c0_status()&~CP0_CAUSE_IP7);
		if (cause&CP0_CAUSE_TI) {
			unsigned long int cpu_flags;
			unsigned long int compare;
			compare=read_c0_count()+compare_increment;
			set_c0_compare(compare);
			cpu_flags=save_cpu_flags();
			enable_interrupts();
			SysTick_Handler();
			restore_cpu_flags(cpu_flags);
			
		}
//		set_c0_status(get_c0_status()|CP0_CAUSE_IP7);
	}
#endif

	if (cause&CP0_CAUSE_IP2) {
//		set_c0_status(get_c0_status()&~CP0_CAUSE_IP2);
		if (cirq&=installed_mask) {
			int i;
			for(i=0;i<32;i++) {
				if (cirq&(1<<i)) {
					INTC->icmcr=cirq&(1<<i);
					handlers[i].handler(i,handlers[i].h_data);
				}
			}
		}
//		set_c0_status(get_c0_status()|CP0_CAUSE_IP2);
	}
//	INTC->icmcr=cirq;
}

int gruux=0;
void wait_irq(void) {
#if 1
	set_c0_status(get_c0_status()|0x1c00);
	asm volatile ("wait\t\n\t"
			: : );
#endif
#if 0
	if ((gruux%10000000)==0) {
		sys_printf("wait irq: c0_status %x, c0_cause %x, INTC.icmr %x\n",
			get_c0_status(), get_c0_cause(), INTC->icmr);
	        set_c0_status(get_c0_status()|0x1c00);
	}
	gruux++;
#endif
}

