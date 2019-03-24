#include "sys.h"
#include "io.h"
#include "irq.h"


//static	unsigned int divisor=1000/10;
//static	unsigned int cmp_val=378000000/100;
static	unsigned int cmp_val=120000000/100;

static int tcu0_irq_handler(int irq_num, void *hdata) {
	unsigned long int cpu_flags;

	__tcu_clear_ost_match_flag();

//	sys_printf("wait_irq: TER %x, TFR %x, TMR %x, OS count %x, OS data %x\n",
//                __tcu_read_ter(), __tcu_read_tfr(), __tcu_read_tmr(),
//                __ost_get_count(), __ost_get_data());

//	sys_printf("got tcu0 irq\n");
	cpu_flags=save_cpu_flags();
//	enable_interrupts();
	SysTick_Handler();
//	restore_cpu_flags(cpu_flags);

	return 0;
}


void config_sys_tic(unsigned int ms) {

	sys_printf("setting up timer\n");
        install_irq_handler(IRQ_TCU0, tcu0_irq_handler, 0);

	__ost_set_shutdown_abrupt(); //	TCSR.SD=1;
	__ost_select_clk_div1();  // OSTCSR.PRESCALE=0;
	__ost_set_count(0);       // OSTCNT=0;
	__ost_set_data(cmp_val); // OSTDR=cmp_val;
	__ost_select_pclk();     // OSTCSR.PCK_EN=1
	REG_TCU_TMCR = TCU_TMCR_OSTMCL;
	__tcu_start_counter(15); // TESR.OSTCST=1;
}
