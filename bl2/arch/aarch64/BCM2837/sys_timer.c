#include <sys.h>
#include <devices.h>
#include <irq.h>

static unsigned int us2next;

static int sys_timer1_irq_handler(int irq_num, void *hdata) {
	unsigned long int cpu_flags;
        struct SYS_TIMER *sys_timer=SYS_TIMER_BASE;
        sys_timer->cs|=SYS_TIMER_CS_M1;		// clear interrupt
        sys_timer->c1=sys_timer->clo+us2next;	// set next timeout

//	cpu_flags=save_cpu_flags();
//	enable_interrupts();
	SysTick_Handler();
//	restore_cpu_flags(cpu_flags);
	return 0;
}

void config_sys_tic(unsigned int ms) {
        struct SYS_TIMER *sys_timer=SYS_TIMER_BASE;

	us2next=ms*1000;         // cycle time for timer cnt is 1 uS

	install_irq_handler(SYS_TIMER1_MATCH, sys_timer1_irq_handler,0);

        sys_timer->cs|=SYS_TIMER_CS_M1;
        sys_timer->c1=sys_timer->clo+us2next;

#if 0
        while(1) {
                if ((sys_timer->cs&SYS_TIMER_CS_M1)!=0) {
                        sys_timer->c1=sys_timer->clo+(1024*1024);
                        sys_timer->cs|=SYS_TIMER_CS_M1;
                        sys_printf("timer is %x\n", sys_timer->clo);
                }
        }
#endif
}
