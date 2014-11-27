
#include "sys_arch.h"
#include <regdef.h>

extern unsigned long __bss_start__, __bss_end__;

void nand_load(int,int,char *);

#if 0
void set_c0_compare(unsigned int val) {
	__asm__ __volatile__("mtc0\t%z0, $11\n\t"
	: : "Jr" (val));
}
#endif

void set_c0_count(unsigned int val) {
	__asm__ __volatile__("mtc0\t%z0, $9\n\t"
		: : "Jr" (val));
}


unsigned int read_c0_compare(void) {
	unsigned int ret=100;
	__asm__ __volatile__("mfc0\t%z0, $11\n\t"
		: "=r" (ret));
	return ret;
}

unsigned int read_c0_count(void) {
	unsigned int ret=300;
	__asm__ __volatile__("mfc0\t%z0, $9\n\t"
		: "=r" (ret));
	return ret;
}


#define UINT(a) ((unsigned long int)(a))

void c_start(void) {
        serial_puts("Copy nand to ram\n");
        nand_load(0x1000, 0x4000,
                (unsigned char *)0x80000000);

	__builtin_memset(&__bss_start__,0,UINT(&__bss_end__)-UINT(&__bss_start__));
	serial_puts("in c_start\n");
	/* shutoff BEV and ERL*/
	asm(	"mfc0	$t0,$12\n\t"
		"lui	$t1,0x44\n\t"
		"ori	$t0,0x44\n\t"
		"xor	$t0,$t1\n\t"
		"mtc0	$t0,$12\n\t"
		:::"t0","t1");
	/* shutoff IV */
	asm(	"mfc0	$t0,$13\n\t"
		"lui	$t1,0x80\n\t"
		"xor	$t0,$t1\n\t"
		"mtc0	$t0,$13\n\t"
		:::"t0","t1");
	
	init_usart_drv();
	driver_init();
	driver_start();

	enable_interrupts();
	start_up();
	while(1) {
		int i,j;
		unsigned int count, compare;
		for(i=0;i<100000;i++) {
			for(j=0;j<3000;j++) {
			}
		}
#if 0
		count=read_c0_count();
		compare=read_c0_compare();
		sys_printf("c_start: loop, count %x, compare %x\n",count,compare);
#endif
	}
}

#if 0
static int irq_cnt=0;
static unsigned int cmp=0x6000000;
void irq_dispatch(void) {
	if (!(irq_cnt%100)) {
		sys_printf("in irq\n");
		cmp+=0x6000000;
		set_c0_compare(cmp);
	}
	irq_cnt++;
}
#endif
