

// Bank control register
#define FMC_BCR_CCLKEN		0x00100000
#define FMC_BCR_CBURSTRW	0x00080000
#define FMC_BCR_ASYNCWAIT	0x00008000
#define	FMC_BCR_EXTMOD		0x00004000
#define FMC_BCR_WAITEN		0x00002000
#define FMC_BCR_WREN		0x00001000
#define FMC_BCR_WAITCFG		0x00000800
#define FMC_BCR_WRAPMOD		0x00000400
#define FMC_BCR_WAITPOL		0x00000200
#define FMC_BCR_BURSTEN		0x00000100
#define FMC_BCR_FACCEN		0x00000040
#define FMC_BCR_MWID_MASK	0x00000030
#define FMC_BCR_MWID_SHIFT	4
#define FMC_BCR_MTYP_MASK	0x0000000c
#define FMC_BCR_MTYP_SHIFT	2
#define FMC_BCR_MUXEN		0x00000002
#define FMC_BCR_MBKEN		0x00000001

// Bank timing register
#define	FMC_BTR_ACCMOD_MASK	0x30000000
#define FMC_BTR_ACCMOD_SHIFT	28
#define FMC_BTR_DATLAT_MASK	0x0f000000
#define FMC_BTR_DATLAT_SHIFT	24
#define FMC_BTR_CLKDIV_MASK	0x00f00000
#define FMC_BTR_CLKDIV_SHIFT	20
#define FMC_BTR_BUSTURN_MASK	0x000f0000
#define FMC_BTR_BUSTURN_SHIFT	16
#define FMC_BTR_DATAST_MASK	0x0000ff00
#define FMC_BTR_DATAST_SHIFT	8
#define FMC_BTR_ADDHLD_MASK	0x000000f0
#define FMC_BTR_ADDHLD_SHIFT	4
#define FMC_BTR_ADDSET_MASK	0x0000000f
#define FMC_BTR_ADDSET_SHIFT	0

// Bank write timing register
#define	FMC_BWTR_ACCMOD_MASK	0x30000000
#define FMC_BWTR_ACCMOD_SHIFT	28
#define FMC_BWTR_DATLAT_MASK	0x0f000000
#define FMC_BWTR_DATLAT_SHIFT	24
#define FMC_BWTR_CLKDIV_MASK	0x00f00000
#define FMC_BWTR_CLKDIV_SHIFT	20
#define FMC_BWTR_BUSTURN_MASK	0x000f0000
#define FMC_BWTR_BUSTURN_SHIFT	16
#define FMC_BWTR_DATAST_MASK	0x0000ff00
#define FMC_BWTR_DATAST_SHIFT	8
#define FMC_BWTR_ADDHLD_MASK	0x000000f0
#define FMC_BWTR_ADDHLD_SHIFT	4
#define FMC_BWTR_ADDSET_MASK	0x0000000f
#define FMC_BWTR_ADDSET_SHIFT	0

// PC card controller registers
#define FMC_PCR_ECCPS_MASK	0x000e0000
#define FMC_PCR_ECCPS_SHIFT	17
#define FMC_PCR_TAR_MASK	0x0001e000
#define FMC_PCR_TAR_SHIFT	13
#define FMC_PCR_TCLR_MASK	0x00001e00
#define FMC_PCR_TCLR_SHIFT	9
#define FMC_PCR_ECCEN		0x00000040
#define FMC_PCR_PWID_MASK	0x00000030
#define FMC_PCR_PWID_SHIFT	4
#define FMC_PCR_PTYP		0x00000008
#define FMC_PCR_PBKEN		0x00000004
#define FMC_PCR_PWAITEN		0x00000002

// FIFO status and interrupt registers
#define FMC_SR_FEMPT		0x00000040
#define FMC_SR_IFEN		0x00000020
#define FMC_SR_ILEN		0x00000010
#define FMC_SR_IREN		0x00000008
#define FMC_SR_IFS		0x00000004
#define FMC_SR_ILS		0x00000002
#define FMC_SR_IRS		0x00000001

// Common memory space timing registers
#define FMC_PMEM_MEMHIZ_MASK	0xff000000
#define FMC_PMEM_MEMHIZ_SHIFT	24
#define FMC_PMEM_MEMHOLD_MASK	0x00ff0000
#define FMC_PMEM_MEMHOLD_SHIFT	16
#define FMC_PMEM_MEMWAIT_MASK	0x0000ff00
#define FMC_PMEM_MEMWAIT_SHIFT	8
#define FMC_PMEM_MEMSET_MASK	0x000000ff
#define FMC_PMEM_MEMSET_SHIFT	0

// Attribute memory space timing registers
#define FMC_PATT_ATTHIZ_MASK	0xff000000
#define FMC_PATT_ATTHIZ_SHIFT	24
#define FMC_PATT_ATTHOLD_MASK	0x00ff0000
#define FMC_PATT_ATTHOLD_SHIFT	16
#define FMC_PATT_ATTWAIT_MASK	0x0000ff00
#define FMC_PATT_ATTWAIT_SHIFT	8
#define FMC_PATT_ATTSET_MASK	0x000000ff
#define FMC_PATT_ATTSET_SHIFT	0

// IO space timing registers
#define FMC_PIO_IOHIZ_MASK	0xff000000
#define FMC_PIO_IOHIZ_SHIFT	24
#define FMC_PIO_IOHOLD_MASK	0x00ff0000
#define FMC_PIO_IOHOLD_SHIFT	16
#define FMC_PIO_IOWAIT_MASK	0x0000ff00
#define FMC_PIO_IOWAIT_SHIFT	8
#define FMC_PIO_IOSET_MASK	0x000000ff
#define FMC_PIO_IOSET_SHIFT	0

// SDRAM controller registers
#define FMC_SDCR_RPIPE_MASK	0x00006000
#define FMC_SDCR_RPIPE_SHIFT	13
#define FMC_SDCR_RBURST		0x00001000
#define FMC_SDCR_SDCLK_MASK	0x00000c00
#define FMC_SDCR_SDCLK_SHIFT	10
#define FMC_SDCR_WP		0x00000200
#define FMC_SDCR_CAS_MASK	0x00000180
#define FMC_SDCR_CAS_SHIFT	7
#define FMC_SDCR_NB		0x00000040
#define FMC_SDCR_NB_SHIFT	6
#define FMC_SDCR_MWID_MASK	0x00000030
#define FMC_SDCR_MWID_SHIFT	4
#define FMC_SDCR_NR_MASK	0x0000000c
#define FMC_SDCR_NR_SHIFT	2
#define FMC_SDCR_NC_MASK	0x00000003
#define FMC_SDCR_NC_SHIFT	0

// SDRAM Timing registers
#define FMC_SDTR_TRCD_MASK	0x0f000000
#define FMC_SDTR_TRCD_SHIFT	24
#define FMC_SDTR_TRP_MASK	0x00f00000
#define FMC_SDTR_TRP_SHIFT	20
#define FMC_SDTR_TWR_MASK	0x000f0000
#define FMC_SDTR_TWR_SHIFT	16
#define FMC_SDTR_TRC_MASK	0x0000f000
#define FMC_SDTR_TRC_SHIFT	12
#define FMC_SDTR_TRAS_MASK	0x00000f00
#define FMC_SDTR_TRAS_SHIFT	8
#define FMC_SDTR_TXSR_MASK	0x000000f0
#define FMC_SDTR_TXSR_SHIFT	4
#define FMC_SDTR_TMRD_MASK	0x0000000f
#define FMC_SDTR_TMRD_SHIFT	0

// SDRAM Command Mode register
#define FMC_SDCMR_MRD_MASK	0x003ffe00
#define FMC_SDCMR_MRD_SHIFT	9
#define FMC_SDCMR_NRFS_MASK	0x000001e0
#define FMC_SDCMR_NRFS_SHIFT	5
#define FMC_SDCMR_CTB1		0x00000010
#define FMC_SDCMR_CTB2		0x00000008
#define FMC_SDCMR_MODE_MASK	0x00000007
#define FMC_SDCMR_MODE_SHIFT	0

// SDRAM Refresh Timer register
#define FMC_SDRTR_REIE		0x00004000
#define FMC_SDRTR_COUNT_MASK	0x00003ffe
#define FMC_SDRTR_COUNT_SHIFT	1
#define FMC_SDRTR_CRE		1

// SDRAM Status register
#define FMC_SDSR_BUSY		0x00000020
#define FMC_SDSR_MODES2_MASK	0x00000018
#define FMC_SDSR_MODES2_SHIFT	3
#define FMC_SDSR_MODES1_MASK	0x00000006
#define FMC_SDSR_MODES1_SHIFT	1
#define FMC_SDSR_RE		1







struct FMC {
	volatile unsigned int	BCR1;		// 0x00
	volatile unsigned int	BTR1;		// 0x04
	volatile unsigned int	BCR2;		// 0x08
	volatile unsigned int	BTR2;		// 0x0c
	volatile unsigned int 	BCR3;		// 0x10
	volatile unsigned int	BTR3;		// 0x14
	volatile unsigned int 	BCR4;		// 0x18
	volatile unsigned int	BTR4;		// 0x1c
	unsigned char		padOx20[0x40];	// 0x20-0x5c
	volatile unsigned int	PCR2;		// 0x60
	volatile unsigned int	SR2;		// 0x64
	volatile unsigned int	PMEM2;		// 0x68
	volatile unsigned int	PATT2;		// 0x6c
	unsigned char		padOx70[4];	// 0x70
	volatile unsigned int	ECCR2;		// 0x74
	unsigned char 		padOx78[8];	// 0x78
	volatile unsigned int	PCR3;		// 0x80
	volatile unsigned int	SR3;		// 0x84
	volatile unsigned int	PMEM3;		// 0x88
	volatile unsigned int	PATT3;		// 0x8c
	unsigned char		padOx90[4];	// 0x90
	volatile unsigned int	ECCR3;		// 0x94
	unsigned char		padOx98[8];	// 0x98
	volatile unsigned int	PCR4;		// 0xa0
	volatile unsigned int	SR4;		// 0xa4
	volatile unsigned int	PMEM4;		// 0xa8
	volatile unsigned int	PATT4;		// 0xac
	volatile unsigned int	PIO4;		// 0xb0
	unsigned char		padOxb4[0x50];	// 0xb4
	volatile unsigned int	BWTR1;		// 0x104
	unsigned char		pad0x108[4];	// 0x108
	volatile unsigned int	BWTR2;		// 0x10c
	unsigned char		padOx110[4];	// 0x110
	volatile unsigned int	BWTR3;		// 0x114
	unsigned char		pad0x118[4];	// 0x118
	volatile unsigned int	BWTR4;		// 0x11c
	unsigned char		padOx[0x20];	// 0x120
	volatile unsigned int	SDCR_1;		// 0x140
	volatile unsigned int	SDCR_2;		// 0x144
	volatile unsigned int	SDTR1;		// 0x148
	volatile unsigned int	SDTR2;		// 0x14c
	volatile unsigned int	SDCMR;		// 0x150
	volatile unsigned int	SDRTR;		// 0x154
	volatile unsigned int	SDSR;		// 0x158
};
