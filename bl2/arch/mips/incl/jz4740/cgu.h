

#define CGU_CPCCR_I2CS_EXCLK	0x00000000
#define CGU_CPCCR_I2CS_PLL	0x80000000

#define CGU_CPCCR_CKOEN_DIS	0x00000000
#define CGU_CPCCR_CKOEN_ENA	0x40000000

#define CGU_CPCCR_UCS_EXCLK	0x00000000
#define CGU_CPCCR_UCS_PLL	0x20000000

// USB clock divider
#define CGU_CPCCR_UDIV_MASK	0x1f800000
#define CGU_CPCCR_UDIV_SHIFT	23

#define CGU_CPCCR_CE		0x00400000
#define CGU_CPCCR_PCS		0x00200000

// LCD clock divider
#define CGU_CPCCR_LDIV_MASK	0x001f0000
#define CGU_CPCCR_LDIV_SHIFT	16

// Memory clock divider
#define CGU_CPCCR_MDIV_MASK	0x0000f000
#define CGU_CPCCR_MDIV_SHIFT	12

// Peripheral clock divider
#define CGU_CPCCR_PDIV_MASK	0x00000f00
#define CGU_CPCCR_PDIV_SHIFT	8

// System clock divider
#define CGU_CPCCR_HDIV_MASK	0x000000f0
#define CGU_CPCCR_HDIV_SHIFT	4

// CPU clock divider
#define CGU_CPCCR_CDIV_MASK	0x0000000f
#define CGU_CPCCR_CDIV_SHIFT	0



#define CGU_I2SCDR_MASK		0x000001ff


#define CGU_LPCDR_LCS_DIVIDER	0x00000000
#define CGU_LPCDR_LCS_PCLK	0x80000000

#define CGU_LPCDR_DIV_MASK	0x000007ff

#define CGU_MSCCDR_DIV_MASK	0x0000001f

#define CGU_UHCCDR_DIV_MASK	0x0000000f

#define CGU_SSICDR_SCS_PLL  	0x80000000
#define CGU_SSICDR_SCS_EXCLK	0x00000000
#define CGU_SSICDR_DIV_MASK	0x0000000f


#define CGU_CPPCR_PLLM_MASK	0xff800000
#define CGU_CPPCR_PLLM_SHIFT	23

#define CGU_CPPCR_PLLN_MASK	0x007c0000
#define CGU_CPPCR_PLLN_SHIFT	18

#define CGU_CPPCR_PLLOD_MASK	0x00030000
#define CGU_CPPCR_PLLOD_SHIFT	16

#define CGU_CPPCR_PLLS		0x00000400
#define CGU_CPPCR_PLLBP		0x00000200
#define	CGU_CPPCR_PLLEN		0x00000100

#define CGU_CPPCR_PLLST_MASK	0x000000ff


struct CGU {
	unsigned int cpccr;	// 0x00
	unsigned int dum1[0x3];	// 0x04-0x0c
	unsigned int cppcr;	// 0x10
	unsigned int dum2[0x13];// 0x14-0x5c
	unsigned int i2scdr;	// 0x60
	unsigned int lpcdr;	// 0x64
	unsigned int msccdr;	// 0x68
	unsigned int uhccdr;	// 0x6c
	unsigned int dum3;	// 0x70
	unsigned int ssicdr;	// 0x74
};

#define __cgu_get_pllm() \
	((CGU->cppcr&CGU_CPPCR_PLLM_MASK)>>CGU_CPPCR_PLLM_SHIFT)

#define __cgu_get_plln() \
	((CGU->cppcr&CGU_CPPCR_PLLN_MASK)>>CGU_CPPCR_PLLN_SHIFT)

#define __cgu_get_pllod() \
	((CGU->cppcr&CGU_CPPCR_PLLOD_MASK)>>CGU_CPPCR_PLLOD_SHIFT)

#define __cgu_get_cdiv() \
	((CGU->cpccr&CGU_CPCCR_CDIV_MASK)>>CGU_CPCCR_CDIV_SHIFT)

#define __cgu_get_hdiv() \
	((CGU->cpccr&CGU_CPCCR_HDIV_MASK)>>CGU_CPCCR_HDIV_SHIFT)

#define __cgu_get_pdiv() \
	((CGU->cpccr&CGU_CPCCR_PDIV_MASK)>>CGU_CPCCR_PDIV_SHIFT)

#define __cgu_get_mdiv() \
	((CGU->cpccr&CGU_CPCCR_MDIV_MASK)>>CGU_CPCCR_MDIV_SHIFT)

#define __cgu_get_ldiv() \
	((CGU->cpccr&CGU_CPCCR_LDIV_MASK)>>CGU_CPCCR_LDIV_SHIFT)

#define __cgu_get_udiv() \
	((CGU->cppcr&CGU_CPCCR_UDIV_MASK)>>CGU_CPCCR_UDIV_SHIFT)

#define __cgu_get_i2sdiv() \
	(CGU->i2scdr&CGU_I2SCDR_MASK)

#define __cgu_get_pixdiv() \
	(CGU->lpcdr&CGU_LPCDR_DIV_MASK)

#define __cgu_get_mscdiv() \
	(CGU->msccdr&CGU_MSCCDR_DIV_MASK)

#define __cgu_get_uhcdiv() \
	(CGU->uhccdr&CGU_UHCCDR_DIV_MASK)

#define __cgu_get_ssidiv() \
	(CGU->ssicdr&CGU_SSICDR_DIV_MASK)


#define __cgu_set_cdiv(v) \
        (CGU->cpccr = (CGU->cpccr & ~CGU_CPCCR_CDIV_MASK) | \
			((v) << (CGU_CPCCR_CDIV_SHIFT)))
#define __cgu_set_hdiv(v) \
        (CGU->cpccr = (CGU->cpccr & ~CGU_CPCCR_HDIV_MASK) | \
			((v) << (CGU_CPCCR_HDIV_SHIFT)))
#define __cgu_set_pdiv(v) \
        (CGU->cpccr = (CGU->cpccr & ~CGU_CPCCR_PDIV_MASK) | \
			((v) << (CGU_CPCCR_PDIV_SHIFT)))
#define __cgu_set_mdiv(v) \
        (CGU->cpccr = (CGU->cpccr & ~CGU_CPCCR_MDIV_MASK) | \
			((v) << (CGU_CPCCR_MDIV_SHIFT)))
#define __cgu_set_ldiv(v) \
        (CGU->cpccr = (CGU->cpccr & ~CGU_CPCCR_LDIV_MASK) | \
			((v) << (CGU_CPCCR_LDIV_SHIFT)))
#define __cgu_set_udiv(v) \
        (CGU->cpccr = (CGU->cpccr & ~CGU_CPCCR_UDIV_MASK) | \
			((v) << (CGU_CPCCR_UDIV_SHIFT)))
#define __cgu_set_i2sdiv(v) \
        (CGU->i2scdr = (CGU->i2scdr & ~CGU_I2SCDR_MASK) | (v))
#define __cgu_set_pixdiv(v) \
        (CGU->lpcdr = (CGU->lpcdr & ~CGU_LPCDR_DIV_MASK) | (v))
#define __cgu_set_mscdiv(v) \
        (CGU->msccdr = (CGU->msccdr & ~CGU_MSCCDR_DIV_MASK) | (v))
#define __cgu_set_uhcdiv(v) \
        (CGU->uhccdr = (CGU->uhccdr & ~CGU_UHCCDR_DIV_MASK) | (v))
#define __cgu_ssiclk_select_exclk() \
        (CGU->ssicdr &= ~CGU_SSICDR_SCS_PLL)
#define __cgu_ssiclk_select_pllout() \
        (CGU->ssicdr |= CGU_SSICDR_SCS_PLL)
#define __cgu_set_ssidiv(v) \
        (CGU->ssicdr = (CGU->ssicdr & ~CGU_SSICDR_DIV_MASK) | (v))

