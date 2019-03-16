#include <config.h>
//#include <jz4750d.h>
#include <devices.h>

typedef struct  global_data {
//        bd_t            *bd;
        unsigned long   flags;
#if defined(CONFIG_JZSOC)
        /* There are other clocks in the Jz47xx or Jz5730*/
        unsigned long   cpu_clk;        /* CPU core clock */
        unsigned long   sys_clk;        /* System bus clock */
        unsigned long   per_clk;        /* Peripheral bus clock */
        unsigned long   mem_clk;        /* Memory bus clock */
        unsigned long   dev_clk;        /* Device clock */
        unsigned long   fb_base;        /* base address of framebuffer */
#endif
        unsigned long   baudrate;
        unsigned long   have_console;   /* serial_init() was called */
        unsigned long   ram_size;       /* RAM size */
        unsigned long   reloc_off;      /* Relocation Offset */
        unsigned long   env_addr;       /* Address  of Environment struct */
        unsigned long   env_valid;      /* Checksum of Environment valid? */
        void            **jt;           /* jump table */
} gd_t;

/*
 * Global Data Flags
 */
#define GD_FLG_RELOC    0x00001         /* Code was relocated to RAM     */
#define GD_FLG_DEVINIT  0x00002         /* Devices have been initialized */
#define GD_FLG_SILENT   0x00004         /* Silent mode                   */

#define DECLARE_GLOBAL_DATA_PTR     register volatile gd_t *gd asm ("k0")



inline int get_cpu_speed(void)
{
        unsigned int speed, cfg;

        /* set gpio as input??? */
        cfg = (REG_GPIO_PXPIN(2) >> 11) & 0x7; /* read GPC11,GPC12,GPC13 */
        switch (cfg) {
        case 0:
                speed = 336000000;
                break;
        case 1:
                speed = 392000000;
                break;
        case 2:
                speed = 400000000;
                break;
        case 3:
                speed = 180000000;
                break;
        case 4:
                speed = 410000000;
                break;
        default:
                speed = 420000000; /* default speed */
                break;
        }

        return speed;
}

/* PLL output clock = EXTAL * NF / (NR * NO)
 *
 * NF = FD + 2, NR = RD + 2
 * NO = 1 (if OD = 0), NO = 2 (if OD = 1 or 2), NO = 4 (if OD = 3)
 */
static int hs = 6;
void pll_init(void) {
        register unsigned int cfcr, plcr1;
        int n2FR[33] = { 
                0, 0, 1, 2, 3, 0, 4, 0, 5, 0, 0, 0, 6, 0, 0, 0,
                7, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0,
                9
        };
        int div[5] = {1, hs, hs, hs, hs}; /* divisors of I:S:P:L:M */
        int nf, pllout2;

        cfcr =  CPM_CPCCR_PCS |
                (n2FR[div[0]] << CPM_CPCCR_CDIV_BIT) |
                (n2FR[div[1]] << CPM_CPCCR_HDIV_BIT) |
                (n2FR[div[2]] << CPM_CPCCR_PDIV_BIT) |
                (n2FR[div[3]] << CPM_CPCCR_MDIV_BIT) |
                (n2FR[div[4]] << CPM_CPCCR_H1DIV_BIT);

        if (CFG_EXTAL > 16000000)
                cfcr |= CPM_CPCCR_ECS;
        else
                cfcr &= ~CPM_CPCCR_ECS;

        pllout2 = (cfcr & CPM_CPCCR_PCS) ? get_cpu_speed() : (get_cpu_speed() / 2);

        nf = get_cpu_speed()  / 1000000;

//      nf = get_cpu_speed() * 2 / CFG_EXTAL;
        plcr1 = ((nf - 2) << CPM_CPPCR_PLLM_BIT) | /* FD */
                (22 << CPM_CPPCR_PLLN_BIT) |    /* RD=0, NR=2 */
                (0 << CPM_CPPCR_PLLOD_BIT) |    /* OD=0, NO=1 */
                (0x20 << CPM_CPPCR_PLLST_BIT) | /* PLL stable time */
                CPM_CPPCR_PLLEN;                /* enable PLL */

        /* init PLL */
        REG_CPM_CPCCR = cfcr;
        REG_CPM_CPPCR = plcr1;
}




static void gpio_init(void)
{
        /*
         * Initialize SDRAM pins
         */
#if CONFIG_NR_DRAM_BANKS == 2   /*Use Two Banks SDRAM*/
        __gpio_as_sdram_x2_32bit();
#else
        __gpio_as_sdram_32bit();
#endif

        /*
         * Initialize lcd pins
         */
        __gpio_as_lcd_18bit();

        /*
         * Initialize UART1 pins
         */
        __gpio_as_uart1();
        __gpio_as_i2c();
        __cpm_start_i2c();
}

static inline unsigned int get_ram_size_per_bank(void)
{
        u32 dmcr; 
        u32 rows, cols, dw, banks;
        unsigned long size;

        dmcr = REG_EMC_DMCR; 
        rows = 11 + ((dmcr & EMC_DMCR_RA_MASK) >> EMC_DMCR_RA_BIT);
        cols = 8 + ((dmcr & EMC_DMCR_CA_MASK) >> EMC_DMCR_CA_BIT);
        dw = (dmcr & EMC_DMCR_BW) ? 2 : 4; 
        banks = (dmcr & EMC_DMCR_BA) ? 4 : 2;

        size = (1 << (rows + cols)) * dw * banks;
//      size *= CONFIG_NR_DRAM_BANKS;

        return size;

}


void sdram_init(void) {
	register unsigned int dmcr, sdmode, tmp, cpu_clk, mem_clk, ns;

#ifdef CONFIG_MOBILE_SDRAM
	register unsigned int sdemode; /*SDRAM Extended Mode*/
#endif
	unsigned int cas_latency_sdmr[2] = {
		EMC_SDMR_CAS_2,
		EMC_SDMR_CAS_3,
	};

	unsigned int cas_latency_dmcr[2] = {
		1 << EMC_DMCR_TCL_BIT,  /* CAS latency is 2 */
		2 << EMC_DMCR_TCL_BIT   /* CAS latency is 3 */
	};

	int div[] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};

	cpu_clk = CFG_CPU_SPEED;
#if defined(CONFIG_FPGA)
	mem_clk = CFG_EXTAL / CFG_DIV;
#else
	mem_clk = cpu_clk * div[__cpm_get_cdiv()] / div[__cpm_get_mdiv()];
#endif

	REG_EMC_BCR &= ~EMC_BCR_BRE;    /* Disable bus release */
	REG_EMC_RTCSR = 0;      /* Disable clock for counting */

	/* Basic DMCR value */
	dmcr = ((SDRAM_ROW-11)<<EMC_DMCR_RA_BIT) |
		((SDRAM_COL-8)<<EMC_DMCR_CA_BIT) |
		(SDRAM_BANK4<<EMC_DMCR_BA_BIT) |
		(SDRAM_BW16<<EMC_DMCR_BW_BIT) |
		EMC_DMCR_EPIN |
		cas_latency_dmcr[((SDRAM_CASL == 3) ? 1 : 0)];

	/* SDRAM timimg */
	ns = 1000000000 / mem_clk;
	tmp = SDRAM_TRAS/ns;
	if (tmp < 4) tmp = 4;
	if (tmp > 11) tmp = 11;
	dmcr |= ((tmp-4) << EMC_DMCR_TRAS_BIT);
	tmp = SDRAM_RCD/ns;
	if (tmp > 3) tmp = 3;
	dmcr |= (tmp << EMC_DMCR_RCD_BIT);
	tmp = SDRAM_TPC/ns;
	if (tmp > 7) tmp = 7;
	dmcr |= (tmp << EMC_DMCR_TPC_BIT);
	tmp = SDRAM_TRWL/ns;
	if (tmp > 3) tmp = 3;
	dmcr |= (tmp << EMC_DMCR_TRWL_BIT);
	tmp = (SDRAM_TRAS + SDRAM_TPC)/ns;
	if (tmp > 14) tmp = 14;
	dmcr |= (((tmp + 1) >> 1) << EMC_DMCR_TRC_BIT);

	/* SDRAM mode value */
	sdmode = EMC_SDMR_BT_SEQ |
		EMC_SDMR_OM_NORMAL |
		EMC_SDMR_BL_4 |
		cas_latency_sdmr[((SDRAM_CASL == 3) ? 1 : 0)];

        /* Stage 1. Precharge all banks by writing SDMR with DMCR.MRSET=0 */
	REG_EMC_DMCR = dmcr;
	REG8(EMC_SDMR0|sdmode) = 0;

	/* Precharge Bank1 SDRAM */
#if CONFIG_NR_DRAM_BANKS == 2   
	REG_EMC_DMCR = dmcr | EMC_DMCR_MBSEL_B1;
	REG8(EMC_SDMR0|sdmode) = 0;
#endif

#ifdef CONFIG_MOBILE_SDRAM
        /* Mobile SDRAM Extended Mode Register */
	sdemode = EMC_SDMR_SET_BA1 | EMC_SDMR_DS_HALF | EMC_SDMR_PRSR_ALL;
#endif

	/* Wait for precharge, > 200us */
	tmp = (cpu_clk / 1000000) * 1000;
	while (tmp--);

	/* Stage 2. Enable auto-refresh */
	REG_EMC_DMCR = dmcr | EMC_DMCR_RFSH;

	tmp = SDRAM_TREF/ns;
	tmp = tmp/64 + 1;
	if (tmp > 0xff) tmp = 0xff;
	REG_EMC_RTCOR = tmp;
	REG_EMC_RTCNT = 0;
	REG_EMC_RTCSR = EMC_RTCSR_CKS_64;       /* Divisor is 64, CKO/64 */

	/* Wait for number of auto-refresh cycles */
	tmp = (cpu_clk / 1000000) * 1000;
	while (tmp--);

	/* Stage 3. Mode Register Set */
	REG_EMC_DMCR = dmcr | EMC_DMCR_RFSH | EMC_DMCR_MRSET | EMC_DMCR_MBSEL_B0;
	REG8(EMC_SDMR0|sdmode) = 0;


#ifdef CONFIG_MOBILE_SDRAM
	REG8(EMC_SDMR0|sdemode) = 0;    /* Set Mobile SDRAM Extended Mode Register */
#endif

#if CONFIG_NR_DRAM_BANKS == 2
	REG_EMC_DMCR = dmcr | EMC_DMCR_RFSH | EMC_DMCR_MRSET | EMC_DMCR_MBSEL_B1;
	REG8(EMC_SDMR0|sdmode) = 0;     /* Set Bank1 SDRAM Register */


#ifdef CONFIG_MOBILE_SDRAM
	REG8(EMC_SDMR0|sdemode) = 0;    /* Set Mobile SDRAM Extended Mode Register */
#endif

#endif   /*CONFIG_NR_DRAM_BANKS == 2*/

	/* Set back to basic DMCR value */
	REG_EMC_DMCR = dmcr | EMC_DMCR_RFSH | EMC_DMCR_MRSET;

	/* bank_size: 32M 64M 128M ... */
	unsigned int bank_size = get_ram_size_per_bank();
	unsigned int mem_base0, mem_base1, mem_mask;

	mem_base0 = EMC_MEM_PHY_BASE >> EMC_MEM_PHY_BASE_SHIFT;
	mem_base1 = ((EMC_MEM_PHY_BASE + bank_size) >> EMC_MEM_PHY_BASE_SHIFT);
	mem_mask = EMC_DMAR_MASK_MASK &
		(~(((bank_size) >> EMC_MEM_PHY_BASE_SHIFT)-1)&EMC_DMAR_MASK_MASK);

	REG_EMC_DMAR0 = (mem_base0 << EMC_DMAR_BASE_BIT) | mem_mask;
	REG_EMC_DMAR1 = (mem_base1 << EMC_DMAR_BASE_BIT) | mem_mask;

	/* everything is ok now */
}


//----------------------------------------------------------------------
// board early init routine

void board_early_init(void)
{
        gpio_init();
//      pll_init();
}

//----------------------------------------------------------------------
// U-Boot common routines

int checkboard (void)
{
        DECLARE_GLOBAL_DATA_PTR;
        sys_printf("Board: Ingenic CETUS (CPU Speed %d MHz)\n",
               gd->cpu_clk/1000000);

        return 0; /* success */
}
             
