
#include <devices.h>

#ifndef JZ_EXTAL
#define JZ_EXTAL	12000000
#endif

#ifndef JZ_EXTAL2
#define JZ_EXTAL2               32768     /* 32.768 KHz */
#endif


static unsigned int cclk;      /* CPU clock */
static unsigned int hclk;      /* System bus clock */
static unsigned int pclk;      /* Peripheral bus clock */
static unsigned int mclk;      /* Flash/SRAM/SDRAM clock */
static unsigned int lcdclk;    /* LCDC module clock */
static unsigned int pixclk;    /* LCD pixel clock */
static unsigned int i2sclk;    /* AIC module clock */
static unsigned int usbclk;    /* USB module clock */
static unsigned int mscclk;    /* MSC module clock */
static unsigned int extalclk;  /* EXTAL clock for UART,I2C,SSI,TCU,USB-PHY */
static unsigned int rtcclk;    /* RTC clock for CPM,INTC,RTC,TCU,WDT */

unsigned int clocks_get_pixclk() {
	return pixclk;
}

unsigned int clocks_set_pixclk(unsigned int clk) {
	pixclk=clk;
	return 0;
}

unsigned int clocks_get_lcdclk() {
	return lcdclk;
}

unsigned int clocks_set_lcdclk(unsigned int clk) {
	lcdclk=clk;
	return 0;
}


unsigned int cgu_get_pllout(void) {
	unsigned long m, n, no, pllout;
	unsigned long cppcr = CGU->cppcr;
	unsigned long od[4] = {1, 2, 2, 4};
	if ((cppcr & CGU_CPPCR_PLLEN) && !(cppcr & CGU_CPPCR_PLLBP)) {
		m = __cgu_get_pllm() + 2;
		n = __cgu_get_plln() + 2;
		no = od[__cgu_get_pllod()];
		pllout = ((JZ_EXTAL) / (n * no)) * m;
	} else {
		pllout = JZ_EXTAL;
	}
	return pllout;
}

unsigned int cgu_get_pllout2(void) {
	if (CGU->cpccr & CGU_CPCCR_PCS) {
		return cgu_get_pllout();
	} else {
		return cgu_get_pllout()/2;
	}
}

unsigned int cgu_get_cclk(void) {
	int div[] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};

        return cgu_get_pllout() / div[__cgu_get_cdiv()];
}

/* AHB system bus clock */
unsigned int cgu_get_hclk(void) {
	int div[] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};

	return cgu_get_pllout() / div[__cgu_get_hdiv()];
}

/* Memory bus clock */
unsigned int cgu_get_mclk(void) {
	int div[] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};

	return cgu_get_pllout() / div[__cgu_get_mdiv()];
}

/* APB peripheral bus clock */
unsigned int cgu_get_pclk(void) {
	int div[] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32};

	return cgu_get_pllout() / div[__cgu_get_pdiv()];
}

/* LCDC module clock */
unsigned int cgu_get_lcdclk(void) {
	return cgu_get_pllout2() / (__cgu_get_ldiv() + 1);
}

/* LCD pixel clock */
unsigned int cgu_get_pixclk(void) {
	return cgu_get_pllout2() / (__cgu_get_pixdiv() + 1);
}

/* I2S clock */
unsigned int cgu_get_i2sclk(void) {
	if (CGU->cpccr & CGU_CPCCR_I2CS_PLL) {
		return cgu_get_pllout2() / (__cgu_get_i2sdiv() + 1);
	} else {
		return JZ_EXTAL;
	}
}

/* USB clock */
unsigned int cgu_get_usbclk(void) {
	if (CGU->cpccr & CGU_CPCCR_UCS_PLL) {
		return cgu_get_pllout2() / (__cgu_get_udiv() + 1);
	} else {
		return JZ_EXTAL;
	}
}

/* MSC clock */
unsigned int cgu_get_mscclk(void) {
	return cgu_get_pllout2() / (__cgu_get_mscdiv() + 1);
}

/* EXTAL clock for UART,I2C,SSI,TCU,USB-PHY */
unsigned int cgu_get_extalclk(void) {
	return JZ_EXTAL;
}

/* RTC clock for CPM,INTC,RTC,TCU,WDT */
unsigned int cgu_get_rtcclk(void) {
	return JZ_EXTAL2;
}

/*
 * Output 24MHz for SD and 16MHz for MMC.
 */
void cgu_select_msc_clk(int sd) {
	unsigned int pllout2 = cgu_get_pllout2();
	unsigned int div = 0;

	if (sd) {
		div = pllout2 / 24000000;
	} else {
		div = pllout2 / 16000000;
	}

        CGU->msccdr = div - 1;
}

