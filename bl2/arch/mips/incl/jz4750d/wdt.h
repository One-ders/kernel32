
// Prepare for removal
//
#if 0

#define WDT_TDR_MASK	0xffff

#define WDT_TCER_TCEN	1

#define WDT_TCNT_MASK	0xffff

#define WDT_TCSR_PRESCALE_MASK		0x0038
#define WDT_TCSR_PRESCALE_SHIFT		3
#define WDT_TCSR_PRESCALE_1CLK		0x0000
#define WDT_TCSR_PRESCALE_4CLK		0x0008
#define WDT_TCSR_PRESCALE_16CLK		0x0010
#define WDT_TCSR_PRESCALE_64CLK		0x0018
#define WDT_TCSR_PRESCALE_256CLK	0x0020
#define WDT_TCSR_PRESCALE_1024CLK	0x0028

#define WDT_TCSR_EXT_EN		0x0004
#define WDT_TCSR_RTC_EN		0x0002
#define WDT_TCSR_PCK_EN		0x0001

struct WDT {
	unsigned short int	tdr;
	unsigned short int	__pad0;
	unsigned char		tcer;
	unsigned char		__pad1;
	unsigned char		__pad2;
	unsigned char		__pad3;
	unsigned short int 	tcnt;
	unsigned short int	__pad4;
	unsigned short int	tcsr;
};
#endif
