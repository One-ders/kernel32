

#define REG8(A)         (*((volatile unsigned char *)(A)))
#define REG16(A)        (*((volatile unsigned short int *)(A)))
#define REG32(A)        (*((volatile unsigned int *)(A)))

/*
 * Memory segments (32bit kernel mode addresses)
 */
#define KUSEG                   0x00000000
#define KSEG0                   0x80000000
#define KSEG1                   0xa0000000
#define KSEG2                   0xc0000000
#define KSEG3                   0xe0000000


#define K0BASE	KSEG0

/*
 *  * Cache Operations
 *   */
#define Index_Invalidate_I      0x00
#define Index_Writeback_Inv_D   0x01
#define Index_Invalidate_SI     0x02
#define Index_Writeback_Inv_SD  0x03
#define Index_Load_Tag_I        0x04
#define Index_Load_Tag_D        0x05
#define Index_Load_Tag_SI       0x06
#define Index_Load_Tag_SD       0x07
#define Index_Store_Tag_I       0x08
#define Index_Store_Tag_D       0x09
#define Index_Store_Tag_SI      0x0A
#define Index_Store_Tag_SD      0x0B
#define Create_Dirty_Excl_D     0x0d
#define Create_Dirty_Excl_SD    0x0f
#define Hit_Invalidate_I        0x10
#define Hit_Invalidate_D        0x11
#define Hit_Invalidate_SI       0x12
#define Hit_Invalidate_SD       0x13
#define Fill                    0x14
#define Hit_Writeback_Inv_D     0x15
                                        /* 0x16 is unused */
#define Hit_Writeback_Inv_SD    0x17
#define Hit_Writeback_I         0x18
#define Hit_Writeback_D         0x19
                                        /* 0x1a is unused */
#define Hit_Writeback_SD        0x1b
                                        /* 0x1c is unused */
                                        /* 0x1e is unused */
#define Hit_Set_Virtual_SI      0x1e
#define Hit_Set_Virtual_SD      0x1f


#define JZ4740_BOOT_NAND_CFG_3PC8B	0xff
#define JZ4740_BOOT_NAND_CFG_2PC8B	0xf0
#define JZ4740_BOOT_NAND_CFG_3PC16B	0x0f
#define JZ4740_BOOT_NAND_CFG_2PC16B	0xff



#define APB_BUS 0xb0000000
#define AHB_BUS	0xb3000000

#define REG_CGU_CPCCR		*((volatile unsigned int *)(APB_BUS+0x00))
#define REG_CGU_CPPCR		*((volatile unsigned int *)(APB_BUS+0x10))
#define REG_CGU_I2SCDR		*((volatile unsigned int *)(APB_BUS+0x60))
#define REG_CGU_LPCDR		*((volatile unsigned int *)(APB_BUS+0x64))
#define REG_CGU_MSCCDR		*((volatile unsigned int *)(APB_BUS+0x68))
#define REG_CGU_UHCCDR		*((volatile unsigned int *)(APB_BUS+0x6c))
#define REG_CGU_SSICDR		*((volatile unsigned int *)(APB_BUS+0x74))

#define CGU_CPCCR_CDIV_MASK	0x0000000f
#define CGU_CPCCR_CDIV_SHIFT	0
#define CGU_CPCCR_HDIV_MASK	0x000000f0
#define CGU_CPCCR_HDIV_SHIFT	4
#define CGU_CPCCR_PDIV_MASK	0x00000f00
#define CGU_CPCCR_PDIV_SHIFT	8
#define CGU_CPCCR_MDIV_MASK	0x0000f000
#define CGU_CPCCR_MDIV_SHIFT	12
#define CGU_CPCCR_LDIV_MASK	0x001f0000
#define CGU_CPCCR_LDIV_SHIFT	16
#define CGU_CPCCR_PCS		0x00200000
#define CGU_CPCCR_CE		0x00400000
#define CGU_CPCCR_UDIV_MASK	0x1f800000
#define CGU_CPCCR_UDIV_SHIFT	23
#define CGU_CPCCR_UCS		0x20000000
#define CGU_CPCCR_CKOEN		0x40000000
#define CGU_CPCCR_I2CS 		0x80000000

#define CGU_CPPCR_PLLST_MASK	0x000000ff
#define CGU_CPPCR_PLLST_SHIFT   0
#define CGU_CPPCR_PLLEN		0x00000100
#define CGU_CPPCR_PLLBP		0x00000200
#define CGU_CPPCR_PLLS		0x00000400
#define CGU_CPPCR_PLLOD_MASK	0x00030000
#define CGU_CPPCR_PLLOD_SHIFT	16
#define CGU_CPPCR_PLLN_MASK	0x007c0000
#define CGU_CPPCR_PLLN_SHIFT	18
#define CGU_CPPCR_PLLM_MASK	0xff800000
#define CGU_CPPCR_PLLM_SHIFT	23



#define REG_PMC_LCR		*((volatile unsigned int *)(APB_BUS+0x04))
#define REG_PMC_CLKGR		*((volatile unsigned int *)(APB_BUS+0x20))
#define REG_PMC_SCR		*((volatile unsigned int *)(APB_BUS+0x24))

#define PMC_LCR_LPM_MASK	0x00000003
#define PMC_LCR_LPM_SHIFT	0
#define PMC_LCR_DOZE		0x00000004
#define PMC_LCR_DUTY_MASK	0x000000f8
#define PMC_LCR_DUTY_SHIFT	3

#define PMC_CLKGR_UART1         0x00008000
#define PMC_CLKGR_UHC           0x00004000
#define PMC_CLKGR_IPU           0x00002000
#define PMC_CLKGR_DMAC          0x00001000
#define PMC_CLKGR_UDC           0x00000800
#define PMC_CLKGR_LCD           0x00000400
#define PMC_CLKGR_CIM           0x00000200
#define PMC_CLKGR_SADC          0x00000100
#define PMC_CLKGR_MSC           0x00000080
#define PMC_CLKGR_AIC           0x00000040
#define PMC_CLKGR_AIC2          0x00000020
#define PMC_CLKGR_SSI           0x00000010
#define PMC_CLKGR_I2C           0x00000008
#define PMC_CLKGR_RTC           0x00000004
#define PMC_CLKGR_TCU           0x00000002
#define PMC_CLKGR_UART0         0x00000001

#define __pmc_start_tcu()       (REG_PMC_CLKGR &= ~PMC_CLKGR_TCU)
#define __pmc_start_lcd()       (REG_PMC_CLKGR &= ~PMC_CLKGR_LCD)
#define __pmc_stop_lcd()        (REG_PMC_CLKGR |= PMC_CLKGR_LCD)


#define	PMC_SCR_O1SE		0x00000010
#define PMC_SCR_SPENDD		0x00000040
#define PMC_SCR_SPENDH		0x00000080
#define	PMC_SCR_O1ST_MASK	0x0000ff00
#define PMC_SCR_O1ST_SHIFT	8

#define REG_RCM_RSR		*((volatile unsigned int *)(APB_BUS+0x08))

#define RCM_RSR_PR		0x00000001
#define RCM_RSR_WR		0x00000002

#define PORT_A	0x000
#define PORT_B	0x100
#define PORT_C  0x200
#define PORT_D  0x300

#define REG_GPIO_PxPIN(port)	*((volatile unsigned int *)(APB_BUS + port + 0x10000))
#define REG_GPIO_PxDAT(port)	*((volatile unsigned int *)(APB_BUS + port + 0x10010))
#define REG_GPIO_PxDATS(port)	*((volatile unsigned int *)(APB_BUS + port + 0x10014))
#define REG_GPIO_PxDATC(port)	*((volatile unsigned int *)(APB_BUS + port + 0x10018))
#define REG_GPIO_PxIM(port)	*((volatile unsigned int *)(APB_BUS + port + 0x10020))
#define REG_GPIO_PxIMS(port)	*((volatile unsigned int *)(APB_BUS + port + 0x10024))
#define REG_GPIO_PxIMC(port)	*((volatile unsigned int *)(APB_BUS + port + 0x10028))
#define REG_GPIO_PxPE(port)	*((volatile unsigned int *)(APB_BUS + port + 0x10030))
#define REG_GPIO_PxPES(port)	*((volatile unsigned int *)(APB_BUS + port + 0x10034))
#define REG_GPIO_PxPEC(port)	*((volatile unsigned int *)(APB_BUS + port + 0x10038))
#define REG_GPIO_PxFUN(port)	*((volatile unsigned int *)(APB_BUS + port + 0x10040))
#define REG_GPIO_PxFUNS(port)	*((volatile unsigned int *)(APB_BUS + port + 0x10044))
#define REG_GPIO_PxFUNC(port)	*((volatile unsigned int *)(APB_BUS + port + 0x10048))
#define REG_GPIO_PxSEL(port)	*((volatile unsigned int *)(APB_BUS + port + 0x10050))
#define REG_GPIO_PxSELS(port)	*((volatile unsigned int *)(APB_BUS + port + 0x10054))
#define REG_GPIO_PxSELC(port)	*((volatile unsigned int *)(APB_BUS + port + 0x10058))
#define REG_GPIO_PxDIR(port)	*((volatile unsigned int *)(APB_BUS + port + 0x10060))
#define REG_GPIO_PxDIRS(port)	*((volatile unsigned int *)(APB_BUS + port + 0x10064))
#define REG_GPIO_PxDIRC(port)	*((volatile unsigned int *)(APB_BUS + port + 0x10068))
#define REG_GPIO_PxTRG(port)	*((volatile unsigned int *)(APB_BUS + port + 0x10070))
#define REG_GPIO_PxTRGS(port)	*((volatile unsigned int *)(APB_BUS + port + 0x10074))
#define REG_GPIO_PxTRGC(port)	*((volatile unsigned int *)(APB_BUS + port + 0x10078))
#define REG_GPIO_PxFLG(port)	*((volatile unsigned int *)(APB_BUS + port + 0x10080))
#define REG_GPIO_PxFLGC(port)	*((volatile unsigned int *)(APB_BUS + port + 0x10014))


#define __gpio_port_as_output(p, o)             \
do {                                            \
    REG_GPIO_PxFUNC(p) = (1 << (o));            \
    REG_GPIO_PxSELC(p) = (1 << (o));            \
    REG_GPIO_PxDIRS(p) = (1 << (o));            \
} while (0)

#define __gpio_as_output(n)                     \
do {                                            \
        unsigned int p, o;                      \
        p = (n) / 32;                           \
        o = (n) % 32;                           \
        __gpio_port_as_output((p<<8), o);       \
} while (0)

#define __gpio_set_pin(n)                       \
do {                                            \
        unsigned int p, o;                      \
        p = (n) / 32;                           \
        o = (n) % 32;                           \
        REG_GPIO_PxDATS((p<<8)) = (1 << o);     \
} while (0)

#define __gpio_clear_pin(n)                     \
do {                                            \
        unsigned int p, o;                      \
        p = (n) / 32;                           \
        o = (n) % 32;                           \
        REG_GPIO_PxDATC((p<<8)) = (1 << o);     \
} while (0)

/*
 * D0 ~ D31, A0 ~ A16, DCS#, RAS#, CAS#, CKE#, 
 * RDWE#, CKO#, WE0#, WE1#, WE2#, WE3#
 */
#define __gpio_as_sdram_32bit()                 \
do {                                            \
        REG_GPIO_PxFUNS(PORT_A) = 0xffffffff;        \
        REG_GPIO_PxSELC(PORT_A) = 0xffffffff;        \
        REG_GPIO_PxPES(PORT_A) = 0xffffffff;         \
        REG_GPIO_PxFUNS(PORT_B) = 0x81f9ffff;        \
        REG_GPIO_PxSELC(PORT_B) = 0x81f9ffff;        \
        REG_GPIO_PxPES(PORT_B) = 0x81f9ffff;         \
        REG_GPIO_PxFUNS(PORT_C) = 0x07000000;        \
        REG_GPIO_PxSELC(PORT_C) = 0x07000000;        \
        REG_GPIO_PxPES(PORT_C) = 0x07000000;         \
} while (0)

/*
 *  * LCD_D0~LCD_D15, LCD_PCLK, LCD_HSYNC, LCD_VSYNC, LCD_DE
 *   */
#define __gpio_as_lcd_16bit()                   \
do {                                            \
        REG_GPIO_PxFUNS(PORT_C) = 0x003cffff;        \
        REG_GPIO_PxSELC(PORT_C) = 0x003cffff;        \
        REG_GPIO_PxPES(PORT_C) = 0x003cffff;         \
} while (0)

/*
 *  * LCD_D0~LCD_D17, LCD_PCLK, LCD_HSYNC, LCD_VSYNC, LCD_DE
 *   */
#define __gpio_as_lcd_18bit()                   \
do {                                            \
        REG_GPIO_PxFUNS(PORT_C) = 0x003fffff;        \
        REG_GPIO_PxSELC(PORT_C) = 0x003fffff;        \
        REG_GPIO_PxPES(PORT_C) = 0x003fffff;         \
} while (0)

/*
 *  * LCD_PS, LCD_REV, LCD_CLS, LCD_SPL
 *   */
#define __gpio_as_lcd_special()                 \
do {                                            \
        REG_GPIO_PxFUNS(PORT_B) = 0x00060000;        \
        REG_GPIO_PxSELC(PORT_B) = 0x00060000;        \
        REG_GPIO_PxPES(PORT_B)  = 0x00060000;        \
        REG_GPIO_PxFUNS(PORT_C) = 0x00c00000;        \
        REG_GPIO_PxSELC(PORT_C) = 0x00c00000;        \
        REG_GPIO_PxPES(PORT_C)  = 0x00c00000;        \
} while (0)


/*
 * UART0_TxD, UART_RxD0
 */
#define __gpio_as_uart0()                       \
do {                                            \
        REG_GPIO_PxFUNS(PORT_D) = 0x06000000;        \
        REG_GPIO_PxSELS(PORT_D) = 0x06000000;        \
        REG_GPIO_PxPES(PORT_D) = 0x06000000;         \
} while (0)


#define REG_BCR		*((volatile unsigned int *)(AHB_BUS + 0x10000))

#define REG_NFCSR	*((volatile unsigned int *)(AHB_BUS + 0x10050))
#define REG_NFECCR	*((volatile unsigned int *)(AHB_BUS + 0x10100))
#define REG_NFECC	*((volatile unsigned int *)(AHB_BUS + 0x10104))
#define REG_NFPAR0	*((volatile unsigned int *)(AHB_BUS + 0x10108))
#define REG_NFPAR1	*((volatile unsigned int *)(AHB_BUS + 0x1010c))
#define REG_NFPAR2	*((volatile unsigned int *)(AHB_BUS + 0x10110))
#define REG_NFINTS	*((volatile unsigned int *)(AHB_BUS + 0x10114))
#define REG_NFINTE	*((volatile unsigned int *)(AHB_BUS + 0x10118))
#define REG_NFERR0	*((volatile unsigned int *)(AHB_BUS + 0x1011c))
#define REG_NFERR1	*((volatile unsigned int *)(AHB_BUS + 0x10120))
#define REG_NFERR2	*((volatile unsigned int *)(AHB_BUS + 0x10124))
#define REG_NFERR3	*((volatile unsigned int *)(AHB_BUS + 0x10128))

#define REG_DMCR	*((volatile unsigned int *)(AHB_BUS + 0x10080))
#define REG_RTCSR	*((volatile unsigned short int *)(AHB_BUS + 0x10084))
#define REG_RTCNT	*((volatile unsigned short int *)(AHB_BUS + 0x10088))
#define REG_RTCOR	*((volatile unsigned short int *)(AHB_BUS + 0x1008c))
#define REG_DMAR 	*((volatile unsigned int *)(AHB_BUS + 0x10090))
#define REG_SDMR 	*((volatile unsigned int *)(AHB_BUS + 0x1a000))
#define SDMR_ADDR	(AHB_BUS+0x1a000)

/* bit fields for REG_BCR */
#define BCR_ENDIAN	0x00000001
#define BCR_BRE		0x00000002
#define BCR_BT_SEL0	0x40000000
#define BCR_BT_SEL1	0x80000000

/* bit fields for REG_NFCSR */
#define NFCSR_NFE1	0x01
#define NFCSR_NFCE1	0x02
#define NFCSR_NFE2	0x04
#define NFCSR_NFCE2	0x08
#define NFCSR_NFE3	0x10
#define NFCSR_NFCE3	0x20
#define NFCSR_NFE4	0x40
#define NFCSR_NFCE4	0x80

/* bit fields for REG_NFECCR */
#define NFECCR_ECCE	0x01
#define NFECCR_ERST	0x02
#define NFECCR_RSE 	0x04
#define NFECCR_ENCE	0x08
#define NFECCR_PRDY	0x10

/* bit fields for REG_NFECC */
#define NFECC_ECC0_MASK		0xff
#define NFECC_ECC0_SHIFT	0
#define NFECC_ECC1_MASK		0xff00
#define NFECC_ECC1_SHIFT	8
#define NFECC_ECC2_MASK		0xff0000
#define NFECC_ECC2_SHIFT	16

/* bit fields for REG_NFPAR0 */
#define NFPAR0_PAR7_MASK	0x000001ff
#define NFPAR0_PAR7_SHIFT	0
#define NFPAR0_PAR6_MASK	0x0003fe00
#define NFPAR0_PAR6_SHIFT	9
#define NFPAR0_PAR5_MASK	0x07fc0000
#define NFPAR0_PAR5_SHIFT	18
#define NFPAR0_PAR4_MASK	0xf8000000
#define NFPAR0_PAR4_SHIFT	27

/* bit fields for REG_NFPAR1 */
#define NFPAR1_PAR4_MASK	0x0000000f
#define NFPAR1_PAR4_SHIFT	0
#define NFPAR1_PAR3_MASK	0x00001ff0
#define NFPAR1_PAR3_SHIFT	4
#define NFPAR1_PAR2_MASK	0x003fe000
#define NFPAR1_PAR2_SHIFT	13
#define NFPAR1_PAR1_MASK	0x7fc00000
#define NFPAR1_PAR1_SHIFT	22
#define NFPAR1_PAR0_MASK	0x80000000
#define NFPAR1_PAR0_SHIFT	31

/* bit fields for REG_NFPAR2 */
#define NFPAR2_PAR0_MASK	0x000000ff
#define NFPAR2_PAR0_SHIFT	0

/* bit fields for REG_NFINTS */
#define NFINTS_ERRC_MASK	0xe0000000
#define NFINTS_ERRC_SHIFT	29

#define NFINTS_PADF		0x00000010
#define NFINTS_DECF		0x00000008
#define NFINTS_ENCF		0x00000004
#define NFINTS_UNCOR		0x00000002
#define NFINTS_ERR		0x00000001

/* bit fields for REG_NFINTE */
#define NFINTE_PADFE		0x00000010
#define NFINTE_ENCFE		0x00000004
#define NFINTE_UNCORE		0x00000002
#define NFINTE_ERRE		0x00000001

/* bit fields for REG_NFERR0 */
/*		  REG_NFERR1 */
/*		  REG_NFERR2 */
/*		  REG_NFERR3 */
#define NFERR_INDEX_MASK	0x01ff0000
#define NFERR_INDEX_SHIFT	16
#define NFERR_MASK_MASK		0x000001ff
#define NFERR_MASK_SHIFT	0

/* bit fields for REG_DMCR */
#define DMCR_BW			0x80000000
#define DMCR_BW_SHIFT		31
#define DMCR_CA_MASK		0x1c000000
#define DMCR_CA_SHIFT		26
#define DMCR_RMODE		0x02000000
#define DMCR_RFSH		0x01000000
#define	DMCR_MRSET		0x00800000
#define	DMCR_RA_MASK		0x00300000
#define	DMCR_RA_SHIFT		20
#define	DMCR_BA			0x00080000
#define DMCR_BA_SHIFT		19
#define	DMCR_PDM		0x00040000
#define	DMCR_EPIN		0x00020000
#define	DMCR_TRAS_MASK		0x0000e000
#define	DMCR_TRAS_SHIFT		13
#define	DMCR_RCD_MASK		0x00001800
#define	DMCR_RCD_SHIFT		11
#define	DMCR_TPC_MASK		0x00000700
#define	DMCR_TPC_SHIFT		8
#define	DMCR_TRWL_MASK		0x00000060
#define	DMCR_TRWL_SHIFT		5
#define	DMCR_TRC_MASK		0x0000001c
#define	DMCR_TRC_SHIFT		2
#define	DMCR_TCL_MASK		0x00000003
#define	DMCR_TCL_SHIFT		0

#define DMCR_BW_32	DMCR_BW

#define DMCR_CA_8		0
#define DMCR_CA_9		(1<<DMCR_CA_SHIFT)
#define DMCR_CA_10		(2<<DMCR_CA_SHIFT)
#define DMCR_CA_11		(3<<DMCR_CA_SHIFT)
#define DMCR_CA_12		(4<<DMCR_CA_SHIFT)

#define	DMCR_RMODE_SELF		DMCR_RMODE

#define DMCR_RA_11		0
#define DMCR_RA_12		(1<<DMCR_RA_SHIFT)
#define DMCR_RA_13		(2<<DMCR_RA_SHIFT)

#define DMCR_BA_1		0
#define DMCR_BA_2		DMCR_BA

#define	DMCR_TRAS_4		0
#define DMCR_TRAS_5		(1<<DMCR_TRAS_SHIFT)
#define DMCR_TRAS_6		(2<<DMCR_TRAS_SHIFT)
#define DMCR_TRAS_7		(3<<DMCR_TRAS_SHIFT)
#define DMCR_TRAS_8		(4<<DMCR_TRAS_SHIFT)
#define DMCR_TRAS_9		(5<<DMCR_TRAS_SHIFT)
#define DMCR_TRAS_10		(6<<DMCR_TRAS_SHIFT)
#define DMCR_TRAS_11		(7<<DMCR_TRAS_SHIFT)

#define DMCR_RCD_1		0
#define DMCR_RCD_2		(1<<DMCR_RCD_SHIFT)
#define DMCR_RCD_3		(2<<DMCR_RCD_SHIFT)
#define DMCR_RCD_4		(3<<DMCR_RCD_SHIFT)

#define DMCR_TPC_1		0
#define DMCR_TPC_2		(1<<DMCR_TPC_SHIFT)
#define DMCR_TPC_3		(2<<DMCR_TPC_SHIFT)
#define DMCR_TPC_4		(3<<DMCR_TPC_SHIFT)
#define DMCR_TPC_5		(4<<DMCR_TPC_SHIFT)
#define DMCR_TPC_6		(5<<DMCR_TPC_SHIFT)
#define DMCR_TPC_7		(6<<DMCR_TPC_SHIFT)
#define DMCR_TPC_8		(7<<DMCR_TPC_SHIFT)

#define DMCR_TRWL_1		0
#define DMCR_TRWL_2		(1<<DMCR_TRWL_SHIFT)
#define DMCR_TRWL_3		(2<<DMCR_TRWL_SHIFT)
#define DMCR_TRWL_4		(3<<DMCR_TRWL_SHIFT)

#define DMCR_TRC_1		0
#define DMCR_TRC_3		(1<<DMCR_TRC_SHIFT)
#define DMCR_TRC_5		(2<<DMCR_TRC_SHIFT)
#define DMCR_TRC_7		(3<<DMCR_TRC_SHIFT)
#define DMCR_TRC_9		(4<<DMCR_TRC_SHIFT)
#define DMCR_TRC_11		(5<<DMCR_TRC_SHIFT)
#define DMCR_TRC_13		(6<<DMCR_TRC_SHIFT)
#define DMCR_TRC_15		(7<<DMCR_TRC_SHIFT)

#define DMCR_TCL_INIT		0
#define DMCR_TCL_2		(1<<DMCR_TCL_SHIFT)
#define DMCR_TCL_3		(2<<DMCR_TCL_SHIFT)
#define DMCR_TCL_INHIB		(3<<DMCR_TCL_SHIFT)


#define	SDMR_SDRAM_ADDR_MASK	0x7fffe000
#define SDMR_SDRAM_ADDR_SHIFT	13

#define	SDMR_WB_PBL		0
#define SDMR_WB_SLA		0x200

#define SDMR_OM_NORMAL		0
#define SDMR_OM_TEST		0x080

#define SDMR_CASL_1		0x010
#define SDMR_CASL_2		0x020
#define SDMR_CASL_3		0x030

#define SDMR_BT_SEQ		0
#define SDMR_BT_IL		0x008

#define SDMR_BL_1		0
#define SDMR_BL_2		1
#define SDMR_BL_4		2
#define SDMR_BL_8		3



#define	RTCSR_CMF		0x0080
#define	RTCSR_CKS_MASK		0x0007
#define RTCSR_CKS_SHIFT		0

#define RTCSR_CKS_DIS		0
#define RTCSR_CKS_1_4		(1<<RTCSR_CKS_SHIFT)
#define RTCSR_CKS_1_16		(2<<RTCSR_CKS_SHIFT)
#define RTCSR_CKS_1_64		(3<<RTCSR_CKS_SHIFT)
#define RTCSR_CKS_1_256		(4<<RTCSR_CKS_SHIFT)
#define RTCSR_CKS_1_1024	(5<<RTCSR_CKS_SHIFT)
#define RTCSR_CKS_1_2048	(6<<RTCSR_CKS_SHIFT)
#define RTCSR_CKS_1_4096	(7<<RTCSR_CKS_SHIFT)

#define DMAR_BASE_MASK		0xff00
#define DMAR_BASE_SHIFT		8
#define	DMAR_MASK_MASK		0x00ff
#define	DMAR_MASK_SHIFT		0


/*************************************************************************
 * INTC
 *************************************************************************/
#define INTC_BASE	(APB_BUS+0x1000)

#ifndef __ASSEMBLER__

struct INTC_regs {
	volatile unsigned int icsr;
	volatile unsigned int icmr;
	volatile unsigned int icmsr;
	volatile unsigned int icmcr;
	volatile unsigned int icpr;
};

#define INTC ((struct INTC_regs *)INTC_BASE)

#endif


/*************************************************************************
 * UART
 *************************************************************************/
#define	UART0_BASE	(APB_BUS + 0x30000)
#define IRDA_BASE	UART0_BASE
#define UART_BASE	UART0_BASE

#define UART_OFF        0x1000

#ifndef __ASSEMBLER__

struct UART_regs {
	union {
		volatile unsigned char urbr;
		volatile unsigned char uthr;
		volatile unsigned char udllr;
		volatile unsigned int  fill1;
	};
	union {
		volatile unsigned char uier;
		volatile unsigned char udlhr;
		volatile unsigned int  fill2;
	};
	union {
		volatile unsigned char uiir;
		volatile unsigned char ufcr;
		volatile unsigned int  fill3;
	};
	union {
		volatile unsigned char ulcr;
		volatile unsigned int  fill4;
	};
	union {
		volatile unsigned char umcr;
		volatile unsigned int  fill5;
	};
	union {
		volatile unsigned char ulsr;
		volatile unsigned int  fill6;
	};
	union {
		volatile unsigned char umsr;
		volatile unsigned int  fill7;
	};
	union {
		volatile unsigned char uspr;
		volatile unsigned int  fill8;
	};
	union {
		volatile unsigned char isr;
		volatile unsigned int  fill9;
	};
};

#endif

#define OFF_RDR         (0x00)  /* R  8b H'xx */
#define OFF_TDR         (0x00)  /* W  8b H'xx */
#define OFF_DLLR        (0x00)  /* RW 8b H'00 */
#define OFF_DLHR        (0x04)  /* RW 8b H'00 */
#define OFF_IER         (0x04)  /* RW 8b H'00 */
#define OFF_ISR         (0x08)  /* R  8b H'01 */
#define OFF_FCR         (0x08)  /* W  8b H'00 */
#define OFF_LCR         (0x0C)  /* RW 8b H'00 */
#define OFF_MCR         (0x10)  /* RW 8b H'00 */
#define OFF_LSR         (0x14)  /* R  8b H'00 */
#define OFF_MSR         (0x18)  /* R  8b H'00 */
#define OFF_SPR         (0x1C)  /* RW 8b H'00 */
#define OFF_SIRCR       (0x20)  /* RW 8b H'00, UART0 */
#define OFF_UMR         (0x24)  /* RW 8b H'00, UART M Register */
#define OFF_UACR        (0x28)  /* RW 8b H'00, UART Add Cycle Register */



#define UART0_RDR	(UART0_BASE + OFF_RDR)
#define UART0_TDR	(UART0_BASE + OFF_TDR)
#define UART0_DLLR	(UART0_BASE + OFF_DLLR)
#define UART0_DLHR	(UART0_BASE + OFF_DLHR)
#define UART0_IER	(UART0_BASE + OFF_IER)
#define UART0_ISR	(UART0_BASE + OFF_ISR)
#define UART0_FCR	(UART0_BASE + OFF_FCR)
#define UART0_LCR	(UART0_BASE + OFF_LCR)
#define UART0_MCR	(UART0_BASE + OFF_MCR)
#define UART0_LSR	(UART0_BASE + OFF_LSR)
#define UART0_MSR	(UART0_BASE + OFF_MSR)
#define UART0_SPR	(UART0_BASE + OFF_SPR)
#define UART0_SIRCR	(UART0_BASE + OFF_SIRCR)
#define UART0_UMR	(UART0_BASE + OFF_UMR)
#define UART0_UACR	(UART0_BASE + OFF_UACR)

/*
 * Define macros for UART_IER
 * UART Interrupt Enable Register
 */
#define UART_IER_RDRIE    (1 << 0)        /* 0: receive fifo "full" interrupt disable */
#define UART_IER_TDRIE    (1 << 1)        /* 0: transmit fifo "empty" interrupt disable */
#define UART_IER_RLIE   (1 << 2)        /* 0: receive line status interrupt disable */
#define UART_IER_MIE    (1 << 3)        /* 0: modem status interrupt disable */
#define UART_IER_RTIE   (1 << 4)        /* 0: receive timeout interrupt disable */

/*
 * Define macros for UART_ISR
 * UART Interrupt Status Register
 */
#define UART_ISR_IP     (1 << 0)        /* 0: interrupt is pending  1: no interrupt */
#define UART_ISR_IID    (7 << 1)        /* Source of Interrupt */
#define UART_ISR_IID_MSI                (0 << 1)        /* Modem status interrupt */
#define UART_ISR_IID_THRI       (1 << 1)        /* Transmitter holding register empty */
#define UART_ISR_IID_RDI                (2 << 1)        /* Receiver data interrupt */
#define UART_ISR_IID_RLSI       (3 << 1)        /* Receiver line status interrupt */
#define UART_ISR_FFMS   (3 << 6)        /* FIFO mode select, set when UART_FCR.FE is set to 1 */
#define UART_ISR_FFMS_NO_FIFO   (0 << 6)
#define UART_ISR_FFMS_FIFO_MODE (3 << 6)

/*
 * Define macros for UART_FCR
 * UART FIFO Control Register
 */
#define UART_FCR_FME    (1 << 0)        /* 0: non-FIFO mode  1: FIFO mode */
#define UART_FCR_RFRT   (1 << 1)        /* write 1 to flush receive FIFO */
#define UART_FCR_TFRT   (1 << 2)        /* write 1 to flush transmit FIFO */
#define UART_FCR_DME    (1 << 3)        /* 0: disable DMA mode */
#define UART_FCR_UME    (1 << 4)        /* 0: disable UART */
#define UART_FCR_RDTR   (3 << 6)        /* Receive FIFO Data Trigger */
#define UART_FCR_RDTR_1 (0 << 6)
#define UART_FCR_RDTR_4 (1 << 6)
#define UART_FCR_RDTR_8 (2 << 6)
#define UART_FCR_RDTR_15 (3 << 6)

/*
 * Define macros for UART_LCR
 * UART Line Control Register
 */
#define UART_LCR_WLEN   (3 << 0)        /* word length */
#define UART_LCR_WLEN_5 (0 << 0)
#define UART_LCR_WLEN_6 (1 << 0)
#define UART_LCR_WLEN_7 (2 << 0)
#define UART_LCR_WLEN_8 (3 << 0)
#define UART_LCR_STOP   (1 << 2) /* 0: 1 stop bit when word length is 5,6,7,8
                                    1: 1.5 stop bits when 5; 2 stop bits when 6,7,8 */
#define UART_LCR_STOP_1 (0 << 2)  /* 0: 1 stop bit when word length is 5,6,7,8
                                     1: 1.5 stop bits when 5; 2 stop bits when 6,7,8 */
#define UART_LCR_STOP_2 (1 << 2)  /* 0: 1 stop bit when word length is 5,6,7,8
                                     1: 1.5 stop bits when 5; 2 stop bits when 6,7,8 */

#define UART_LCR_PE     (1 << 3)  /* 0: parity disable */
#define UART_LCR_PROE   (1 << 4)  /* 0: even parity  1: odd parity */
#define UART_LCR_SPAR   (1 << 5)  /* 0: sticky parity disable */
#define UART_LCR_SBRK   (1 << 6)  /* write 0 normal, write 1 send break */
#define UART_LCR_DLAB   (1 << 7)  /* 0: access UART_RDR/TDR/IER  1: access UART_DLLR/DLHR */
/*
 * Define macros for UART_LSR
 * UART Line Status Register
 */
#define UART_LSR_DRY    (1 << 0)  /* 0: receive FIFO is empty  1: receive data is ready */
#define UART_LSR_ORER   (1 << 1)  /* 0: no overrun error */
#define UART_LSR_PER    (1 << 2)  /* 0: no parity error */
#define UART_LSR_FER    (1 << 3)  /* 0; no framing error */
#define UART_LSR_BRK    (1 << 4)  /* 0: no break detected  1: receive a break signal */
#define UART_LSR_TDRQ   (1 << 5)  /* 1: transmit FIFO half "empty" */
#define UART_LSR_TEMP   (1 << 6)  /* 1: transmit FIFO and shift registers empty */
#define UART_LSR_RFER   (1 << 7)  /* 0: no receive error  1: receive error in FIFO mode */

/*
 * Define macros for UART_MCR
 * UART Modem Control Register
 */
#define UART_MCR_DTR    (1 << 0)  /* 0: DTR_ ouput high */
#define UART_MCR_RTS    (1 << 1)  /* 0: RTS_ output high */
#define UART_MCR_OUT1   (1 << 2)  /* 0: UART_MSR.RI is set to 0 and RI_ input high */
#define UART_MCR_OUT2   (1 << 3)  /* 0: UART_MSR.DCD is set to 0 and DCD_ input high */
#define UART_MCR_LOOP   (1 << 4)  /* 0: normal  1: loopback mode */
#define UART_MCR_MDCE   (1 << 7)  /* 0: modem function is disable */

/*
 * Define macros for UART_MSR
 * UART Modem Status Register
 */
#define UART_MSR_DCTS   (1 << 0)   /* 0: no change on CTS_ pin since last read of UART_MSR */
#define UART_MSR_DDSR   (1 << 1)   /* 0: no change on DSR_ pin since last read of UART_MSR */
#define UART_MSR_DRI    (1 << 2)   /* 0: no change on RI_ pin since last read of UART_MSR */
#define UART_MSR_DDCD   (1 << 3)   /* 0: no change on DCD_ pin since last read of UART_MSR */
#define UART_MSR_CTS    (1 << 4)   /* 0: CTS_ pin is high */
#define UART_MSR_DSR    (1 << 5)   /* 0: DSR_ pin is high */
#define UART_MSR_RI     (1 << 6)   /* 0: RI_ pin is high */
#define UART_MSR_DCD    (1 << 7)   /* 0: DCD_ pin is high */

/*
 * Define macros for SIRCR
 * Slow IrDA Control Register
 */
#define SIRCR_TSIRE  (1 << 0) /* 0: transmitter is in UART mode  1: IrDA mode */
#define SIRCR_RSIRE  (1 << 1) /* 0: receiver is in UART mode  1: IrDA mode */
#define SIRCR_TPWS   (1 << 2) /* 0: transmit 0 pulse width is 3/16 of bit length
                                 1: 0 pulse width is 1.6us for 115.2Kbps */
#define SIRCR_TXPL   (1 << 3) /* 0: encoder generates a positive pulse for 0 */
#define SIRCR_RXPL   (1 << 4) /* 0: decoder interprets positive pulse as 0 */

