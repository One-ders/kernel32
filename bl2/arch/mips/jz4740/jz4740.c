
#include <config.h>

void flush_cache_all(void);


/* PLL output clock = EXTAL * NF / (NR * NO)
 *
 * NF = FD + 2, NR = RD + 2
 * NO = 1 (if OD = 0), NO = 2 (if OD = 1 or 2), NO = 4 (if OD = 3)
 */
void pll_init(void) {
	register unsigned int cfcr, plcr1;
	int n2FR[33] = {
		0, 0, 1, 2, 3, 0, 4, 0, 5, 0, 0, 0, 6, 0, 0, 0,
		7, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0,
		9
	};
	int div[5] = { 1, 3, 3, 3, 3 };     /* divisors of I:S:P:M:L */
	int nf, pllout2;

	cfcr = CGU_CPCCR_CKOEN |
		(n2FR[div[0]] << CGU_CPCCR_CDIV_SHIFT) |
		(n2FR[div[1]] << CGU_CPCCR_HDIV_SHIFT) |
		(n2FR[div[2]] << CGU_CPCCR_PDIV_SHIFT) |
		(n2FR[div[3]] << CGU_CPCCR_MDIV_SHIFT) |
		(n2FR[div[4]] << CGU_CPCCR_LDIV_SHIFT);

	pllout2 = (cfcr & CGU_CPCCR_PCS) ? CFG_CPU_SPEED : (CFG_CPU_SPEED / 2);

	/* Init USB Host clock, pllout2 must be n*48MHz */
	REG_CGU_UHCCDR = pllout2 / 48000000 - 1;

	nf = CFG_CPU_SPEED * 2 / CFG_EXTAL;
	plcr1 = ((nf - 2) << CGU_CPPCR_PLLM_SHIFT) |  /* FD */
		(0 << CGU_CPPCR_PLLN_SHIFT) |     /* RD=0, NR=2 */
		(0 << CGU_CPPCR_PLLOD_SHIFT) |    /* OD=0, NO=1 */
		(0x20 << CGU_CPPCR_PLLST_SHIFT) | /* PLL stable time */
		CGU_CPPCR_PLLEN;        /* enable PLL */

	/* init PLL */
	REG_CGU_CPCCR = cfcr;
	REG_CGU_CPPCR = plcr1;
}

void sdram_init(void)
{
        register unsigned int dmcr0, dmcr, sdmode, tmp, cpu_clk, mem_clk, ns;

        unsigned int cas_latency_sdmr[2] = {
                SDMR_CASL_2,
                SDMR_CASL_3,
        };

        unsigned int cas_latency_dmcr[2] = {
                DMCR_TCL_2,  /* CAS latency is 2 */
                DMCR_TCL_3   /* CAS latency is 3 */
        };

        int div[] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};

        cpu_clk = CFG_CPU_SPEED;
        mem_clk = cpu_clk * div[__cgu_get_cdiv()] / div[__cgu_get_mdiv()];

        REG_BCR = 0;        /* Disable bus release */
        REG_RTCSR = 0;      /* Disable clock for counting */

        /* Fault DMCR value for mode register setting*/
#define SDRAM_ROW0    11
#define SDRAM_COL0     8
#define SDRAM_BANK40   0

        dmcr0 = ((SDRAM_ROW0-11)<<DMCR_RA_SHIFT) |
                ((SDRAM_COL0-8)<<DMCR_CA_SHIFT) |
                (SDRAM_BANK40<<DMCR_BA_SHIFT) |
                (SDRAM_BW16<<DMCR_BW_SHIFT) |
                DMCR_EPIN |
                cas_latency_dmcr[((SDRAM_CASL == 3) ? 1 : 0)];

        /* Basic DMCR value */
        dmcr = ((SDRAM_ROW-11)<<DMCR_RA_SHIFT) |
                ((SDRAM_COL-8)<<DMCR_CA_SHIFT) |
                (SDRAM_BANK4<<DMCR_BA_SHIFT) |
                (SDRAM_BW16<<DMCR_BW_SHIFT) |
                DMCR_EPIN |
                cas_latency_dmcr[((SDRAM_CASL == 3) ? 1 : 0)];

        /* SDRAM timimg */
        ns = 1000000000 / mem_clk;
        tmp = SDRAM_TRAS/ns;
        if (tmp < 4) tmp = 4;
        if (tmp > 11) tmp = 11;
        dmcr |= ((tmp-4) << DMCR_TRAS_SHIFT);
        tmp = SDRAM_RCD/ns;
        if (tmp > 3) tmp = 3;
        dmcr |= (tmp << DMCR_RCD_SHIFT);
        tmp = SDRAM_TPC/ns;
        if (tmp > 7) tmp = 7;
        dmcr |= (tmp << DMCR_TPC_SHIFT);
        tmp = SDRAM_TRWL/ns;
        if (tmp > 3) tmp = 3;
        dmcr |= (tmp << DMCR_TRWL_SHIFT);
        tmp = (SDRAM_TRAS + SDRAM_TPC)/ns;
        if (tmp > 14) tmp = 14;
        dmcr |= (((tmp + 1) >> 1) << DMCR_TRC_SHIFT);

        /* SDRAM mode value */
        sdmode = SDMR_BT_SEQ |
                 SDMR_OM_NORMAL |
                 SDMR_BL_4 |
                 cas_latency_sdmr[((SDRAM_CASL == 3) ? 1 : 0)];

        /* Stage 1. Precharge all banks by writing SDMR with DMCR.MRSET=0 */
        REG_DMCR = dmcr;
        REG8(SDMR_ADDR|sdmode) = 0;

        /* Wait for precharge, > 200us */
        tmp = (cpu_clk / 1000000) * 1000;
        while (tmp--);

        /* Stage 2. Enable auto-refresh */
        REG_DMCR = dmcr | DMCR_RFSH;

        tmp = SDRAM_TREF/ns;
        tmp = tmp/64 + 1;
        if (tmp > 0xff) tmp = 0xff;
        REG_RTCOR = tmp;
        REG_RTCNT = 0;
        REG_RTCSR = RTCSR_CKS_1_64;       /* Divisor is 64, CKO/64 */

        /* Wait for number of auto-refresh cycles */
        tmp = (cpu_clk / 1000000) * 1000;
        while (tmp--);

        /* Stage 3. Mode Register Set */
        REG_DMCR = dmcr0 | DMCR_RFSH | DMCR_MRSET;
        REG8(SDMR_ADDR|sdmode) = 0;

        /* Set back to basic DMCR value */
        REG_DMCR = dmcr | DMCR_RFSH | DMCR_MRSET;

        /* everything is ok now */
}

void flush_icache_all(void) {
	unsigned int addr, t = 0;

	asm volatile ("mtc0 $0, $28"); /* Clear Taglo */
	asm volatile ("mtc0 $0, $29"); /* Clear TagHi */

	for (addr = K0BASE; addr < K0BASE + CFG_ICACHE_SIZE;
			addr += CFG_CACHELINE_SIZE) {
		asm volatile (
			".set mips3\n\t"
			" cache %0, 0(%1)\n\t"
			".set mips2\n\t"
			: 
			: "I" (Index_Store_Tag_I), "r"(addr));
	}

	/* invalicate btb */
	asm volatile (
		".set mips32\n\t"
		"mfc0 %0, $16, 7\n\t"
		"nop\n\t"
		"ori %0,2\n\t"
		"mtc0 %0, $16, 7\n\t"
		".set mips2\n\t"
		: 
		: "r" (t));
}

void flush_dcache_all(void) {
	unsigned int addr;

	for (addr = K0BASE; addr < K0BASE + CFG_DCACHE_SIZE;
		addr += CFG_CACHELINE_SIZE) {
		asm volatile (
		".set mips3\n\t"
		" cache %0, 0(%1)\n\t"
		".set mips2\n\t"
		:
		: "I" (Index_Writeback_Inv_D), "r"(addr));
	}

	asm volatile ("sync");
}


void flush_cache_all(void) {
	flush_dcache_all();
	flush_icache_all();
}
