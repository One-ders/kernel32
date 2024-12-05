
#include "uart.h"

struct __attribute__((aligned(4096))) mmu_cfg {
	unsigned long int ttlb_lvl0[512];
	unsigned long int ttlb_lvl1[513];
};

static struct __attribute__((aligned(4096))) mmu_cfg mmu_cfg;

void mmu_init() {
	unsigned long int i,*ttlb_base,r;
	unsigned long int const *lvl1_addr1 = &mmu_cfg.ttlb_lvl1[0];
	unsigned long int const *lvl1_addr2 = &mmu_cfg.ttlb_lvl1[512];

	mmu_cfg.ttlb_lvl0[0] = 0x1UL << 63 | (unsigned long int)(lvl1_addr1)| 0b11;
	mmu_cfg.ttlb_lvl0[1] = 0x1UL << 63 | (unsigned long int)(lvl1_addr2)| 0b11;

	for(i=0;i<504;i++) {
		mmu_cfg.ttlb_lvl1[i]=(unsigned long int)(i*0x200000)|0x710|0b01;
	}

	for(i=504;i<513;i++) {
		mmu_cfg.ttlb_lvl1[i]=(unsigned long int)(i*0x200000)|0x400|0b01;
	}

	r = (0x00<<0) | (0x04<<8) | (0x0c<<16) |
		(0x44<<24) | (0xffUL << 32);
	asm volatile("msr mair_el1, %0" : : "r" (r));

	r = (0b1 << 20) | (0b00LL << 16) | (0b00LL << 14) | (0b11LL << 12) |
		(0b01 << 10) | (0b01LL << 8) | (25LL << 0);
	asm volatile("msr tcr_el1, %0; isb" : : "r" (r));

//	asm volatile("dsb ish; isb; mrs %0, hcr_el2" : "=r" (r));
//	r&=~((1<<0) | (1<<12));
//	asm volatile ("msr hcr_el2, %0; isb" : : "r" (r));

	ttlb_base=(unsigned long int *)(&mmu_cfg.ttlb_lvl0[0]);
	asm volatile ("msr ttbr0_el1, %0" : : "r" (ttlb_base));

	asm volatile ("dsb ish; isb; mrs %0, sctlr_el1" : "=r" (r));
	r&=~((1<<3) | (1<<1));
	r|= ((1<<0)| (1<<12) | (1<<2));
	asm volatile ("msr sctlr_el1, %0; isb" : : "r" (r));

	asm volatile("nop");
	asm volatile("nop");
}

int arch_fixups(unsigned long int x0, unsigned long int x1, unsigned long int atags) {
	serial_init();
	serial_puts("ATAGS val: ");
	serial_put_hexint(atags>>32);
	serial_put_hexint(atags&0xffffffff);
	serial_puts("\n");

	mmu_init();
}
