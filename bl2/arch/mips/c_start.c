
#include "sys.h"
#include <regdef.h>

extern unsigned long __bss_start__, __bss_end__;

void nand_load(int,int,char *);

void set_c0_compare(unsigned int val) {
	__asm__ __volatile__("mtc0\t%z0, $11\n\t"
	: : "Jr" (val));
}

#if 0
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

#endif
unsigned int read_c0_count(void) {
	unsigned int ret=300;
	__asm__ __volatile__("mfc0\t%z0, $9\n\t"
		: "=r" (ret));
	return ret;
}

#define UINT(a) ((unsigned int)(a))
#define ULINT(a) ((unsigned long int)(a))



struct user_data {
        struct device_handle dh;
        struct usart_data *drv_data;
        DRV_CBH callback;
        void    *userdata;
        int     events;
};

extern struct user_data udata[];

typedef int (*P_STR)(char *);
typedef int (*P_CHAR)(char );


P_STR p_str;
P_CHAR p_char;


#ifndef UBOOT
void c_start(void *bsp_funcs) {
//        nand_load(0x1000, 0x4000,
//                (unsigned char *)0x80000000);

	nand_load(0x1000, 0x500, (unsigned char *)0x80000000);
	nand_load(0x2000, 0x3000,(unsigned char *)0x80001000);

	__builtin_memset(&__bss_start__,0,
		ULINT(&__bss_end__)-ULINT(&__bss_start__));

	if (bsp_funcs) {
		p_char=*((P_CHAR *)bsp_funcs);
		p_str=*(((P_STR *)bsp_funcs)+1);
	}

//	sys_printf("in c_start, udata0=%x\n",udata[0].drv_data);
//	sys_printf("bsp_funcs at: %x, p_char at %x, p_str at %x\n",
//			bsp_funcs, p_char, p_str);

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
	
//	init_usart_drv();
//	driver_init();
//	driver_start();

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

#else 

void uboot_start(void *bsp_funcs) {
//        nand_load(0x1000, 0x4000,
//                (unsigned char *)0x80000000);

//	nand_load(0x1000, 0x500, (unsigned char *)0x80000000);
//	nand_load(0x2000, 0x3000,(unsigned char *)0x80001000);

	__builtin_memset(&__bss_start__,0,
		ULINT(&__bss_end__)-ULINT(&__bss_start__));

#if 0
	if (bsp_funcs) {
		p_char=*((P_CHAR *)bsp_funcs);
		p_str=*(((P_STR *)bsp_funcs)+1);
	}
#endif

//	sys_printf("in c_start, udata0=%x\n",udata[0].drv_data);
//	sys_printf("bsp_funcs at: %x, p_char at %x, p_str at %x\n",
//			bsp_funcs, p_char, p_str);

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

	sdram_init();
	pll_init();

//	init_usart_drv();
//	driver_init();
//	driver_start();

	enable_interrupts();
	start_up();

	while(1) {
		int i,j;
		unsigned int count, compare;
		for(i=0;i<100000;i++) {
			for(j=0;j<3000;j++) {
			}
		}
	}
}

#endif
