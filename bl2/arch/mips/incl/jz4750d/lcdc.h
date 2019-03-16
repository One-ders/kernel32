
// CFG register bits

#define	LCDC_CFG_LCDPIN		0x80000000
#define LCDC_CFG_TVEPEH		0x40000000
#define LCDC_CFG_NEWDES		0x10000000
#define LCDC_CFG_PALBP		0x08000000
#define LCDC_CFG_TVEN		0x04000000
#define LCDC_CFG_RECOVER	0x02000000
#define LCDC_CFG_DITHER		0x01000000
#define LCDC_CFG_PSM		0x00800000
#define LCDC_CFG_CLSM		0x00400000
#define LCDC_CFG_SPLM		0x00200000
#define LCDC_CFG_REVM		0x00100000
#define LCDC_CFG_HSYNM		0x00080000
#define LCDC_CFG_PCLKM		0x00040000
#define LCDC_CFG_INVDAT		0x00020000
#define LCDC_CFG_SYNDIR		0x00010000
#define LCDC_CFG_PSP		0x00008000
#define LCDC_CFG_CLSP		0x00004000
#define LCDC_CFG_SPLP		0x00002000
#define LCDC_CFG_REVP		0x00001000
#define LCDC_CFG_HSP		0x00000800
#define LCDC_CFG_PCP		0x00000400
#define LCDC_CFG_DEP		0x00000200
#define LCDC_CFG_VSP		0x00000100
#define LCDC_CFG_18b		0x00000080
#define LCDC_CFG_24		0x00000040
#define LCDC_CFG_PDW_MASK	0x00000030
#define LCDC_CFG_PDW_SHIFT	4
#define LCDC_CFG_MODE_MASK	0x0000000f
#define LCDC_CFG_MODE_SHIFT	0
#define LCDC_CFG_MODE_GENERIC_16_18	0
#define LCDC_CFG_MODE_SPECIAL_TFT1	1
#define LCDC_CFG_MODE_SPECIAL_TFT2	2
#define LCDC_CFG_MODE_SPECIAL_TFT3	3
#define LCDC_CFG_MODE_NI_CCIR656	4
#define LCDC_CFG_MODE_I_CCIR656		6
#define LCDC_CFG_MODE_S_C_STN		8
#define LCDC_CFG_MODE_S_M_STN		9
#define LCDC_CFG_MODE_D_C_STN		10
#define LCDC_CFG_MODE_D_M_STN		11
#define LCDC_CFG_MODE_STFT_8b		12
#define LCDC_CFG_MODE_LCM		13

// VSYNC
// Vsync pulse start and end position  (in line clock)
#define LCDC_VSYNC_VPS_MASK		0x0fff0000
#define LCDC_VSYNC_VPS_SHIFT		16
#define LCDC_VSYNC_VPE_MASK		0x00000fff
#define LCDC_VSYNC_VPE_SHIFT		0

// HSYNC
// Hsync pulse start and end position (in dot clock)
#define LCDC_HSYNC_HPS_MASK		0x0fff0000
#define LCDC_HSYNC_HPS_SHIFT		16
#define LCDC_HSYNC_HPE_MASK		0x00000fff
#define LCDC_HSYNC_HPE_SHIFT		0

// VAT
// Virtual Area Setting (in dot clock)
#define LCDC_VAT_HT_MASK		0x0fff0000
#define LCDC_VAT_HT_SHIFT		16
#define LCDC_VAT_VT_MASK		0x00000fff
#define LCDC_VAT_VT_SHIFT		0

// DAH
// Display Area Horizontal start/end (in line clock)
#define LCDC_DAH_HDS_MASK		0x0fff0000
#define LCDC_DAH_HDS_SHIFT		16
#define LCDC_DAH_HDE_MASK		0x00000fff
#define LCDC_DAH_HDE_SHIFT		0


// DAV
// Display Area Vertical start/end (in line clock)
#define LCDC_DAV_VDS_MASK		0x0fff0000
#define LCDC_DAV_VDS_SHIFT		16
#define LCDC_DAV_VDE_MASK		0x00000fff
#define LCDC_DAV_VDE_SHIFT		0

// PS
// PS Signal Setting (in dot clock)
#define LCDC_PS_PSS_MASK		0x0fff0000
#define LCDC_PS_PSS_SHIFT		16
#define LCDC_PS_PSE_MASK		0x00000fff
#define LCDC_PS_PSE_SHIFT		0

// CLS
// CLS Signal Setting (in dot clock)
#define LCDC_CLS_CLSS_MASK		0x0fff0000
#define LCDC_CLS_CLSS_SHIFT		16
#define LCDC_CLS_CLSE_MASK		0x00000fff
#define LCDC_CLS_CLSE_SHIFT		0

// SPL
// SPL Signal Setting (in dot clock)
#define LCDC_SPL_SPLS_MASK		0x0fff0000
#define LCDC_SPL_SPLS_SHIFT		16
#define LCDC_SPL_SPLE_MASK		0x00000fff
#define LCDC_SPL_SPLE_SHIFT		0

// REV
// REV Signal Setting (in dot clock)
#define LCDC_REV_REVS_MASK		0x0fff0000
#define LCDC_REV_REVS_SHIFT		16

// Control Register
// 0=4 word burst, 1=8 word burst, 2=16 word burst
#define LCDC_CTRL_BST_MASK		0x30000000
#define LCDC_CTRL_BST_SHIFT		28
#define LCDC_CTRL_BST_4			(0<<LCDC_CTRL_BST_SHIFT)
#define LCDC_CTRL_BST_8			(1<<LCDC_CTRL_BST_SHIFT)
#define LCDC_CTRL_BST_16		(2<<LCDC_CTRL_BST_SHIFT)
// 0=RGB565, 1=RGB555
#define LCDC_CTRL_RGB			0x08000000
#define LCDC_CTRL_RGB555		LCDC_CTRL_RGB
#define LCDC_CTRL_OFUP			0x04000000
#define LCDC_CTRL_FRC_MASK		0x03000000
#define LCDC_CTRL_FRC_SHIFT		24
#define LCDC_CTRL_FRC_16		(0<<LCDC_CTRL_FRC_SHIFT)
#define LCDC_CTRL_FRC_4 		(1<<LCDC_CTRL_FRC_SHIFT)
#define LCDC_CTRL_FRC_2 		(2<<LCDC_CTRL_FRC_SHIFT)
#define LCDC_CTRL_PDD_MASK		0x00ff0000
#define LCDC_CTRL_PDD_SHIFT		16
#define LCDC_CTRL_DACTE			0x00004000
#define LCDC_CTRL_EOFM			0x00002000
#define LCDC_CTRL_SOFM			0x00001000
#define LCDC_CTRL_OFUM			0x00000800
#define LCDC_CTRL_IFUM0			0x00000400
#define LCDC_CTRL_IFUM1			0x00000200
#define LCDC_CTRL_LDDM			0x00000100
#define LCDC_CTRL_QDM			0x00000080
#define LCDC_CTRL_BEDN			0x00000040
#define LCDC_CTRL_PEDN			0x00000020
#define LCDC_CTRL_DIS			0x00000010
#define LCDC_CTRL_ENA			0x00000008
// 0=1bpp, 1=2bpp, 2=4bpp, 3=8bpp, 4=15/16 bpp, 5=18/24 bpp
#define LCDC_CTRL_BPP_MASK		0x00000007
#define LCDC_CTRL_BPP_1			0x00000000
#define LCDC_CTRL_BPP_2			0x00000001
#define LCDC_CTRL_BPP_4			0x00000002
#define LCDC_CTRL_BPP_8			0x00000003
#define LCDC_CTRL_BPP_16		0x00000004
#define LCDC_CTRL_BPP_18_24		0x00000005
#define LCDC_CTRL_BPP_24_CMPRS		0x00000006

#define LCDC_STATE_QD			0x00000080
#define LCDC_STATE_EOF			0x00000020
#define LCDC_STATE_SOF			0x00000010
#define LCDC_STATE_OUF			0x00000008
#define LCDC_STATE_IFU0			0x00000004
#define LCDC_STATE_IFU1			0x00000002
#define LCDC_STATE_LDD			0x00000001

#define LCDC_CMD_SOFINT			0x80000000
#define LCDC_CMD_EOFINT			0x40000000
#define LCDC_CMD_CMD			0x20000000
#define LCDC_CMD_PAL			0x10000000
#define LCDC_CMD_LEN_MASK		0x00ffffff

#define LCDC_RGBC_RGBDM			0x00008000
#define LCDC_RGBC_DMM			0x00004000
#define LCDC_RGBC_YCC			0x00000100

#define LCDC_RGBC_OddRGB_MASK		0x00000070
#define LCDC_RGBC_OddRGB_SHIFT		4
#define LCDC_RGBC_OddRGB_RGB		0
#define LCDC_RGBC_OddRGB_RBG		1
#define LCDC_RGBC_OddRGB_GRB		2
#define LCDC_RGBC_OddRGB_GBR		3
#define LCDC_RGBC_OddRGB_BRG		4
#define LCDC_RGBC_OddRGB_BGR		5

#define LCDC_RGBC_EvenRGB_MASK		0x00000007
#define LCDC_RGBC_EvenRGB_SHIFT		0
#define LCDC_RGBC_EvenRGB_RGB		0
#define LCDC_RGBC_EvenRGB_RBG		1
#define LCDC_RGBC_EvenRGB_GRB		2
#define LCDC_RGBC_EvenRGB_GBR		3
#define LCDC_RGBC_EvenRGB_BRG		4
#define LCDC_RGBC_EvenRGB_BGR		5

#define LCDC_OSDC_SOFM1			0x00008000
#define LCDC_OSDC_EOFM1			0x00004000
#define LCDC_OSDC_SOFM0			0x00000800
#define LCDC_OSDC_EOFM0			0x00000400
#define LCDC_OSDC_F1EN			0x00000010
#define LCDC_OSDC_F0EN			0x00000008
#define LCDC_OSDC_ALPHAEN		0x00000004
#define LCDC_OSDC_ALPHAMD		0x00000002
#define LCDC_OSDC_OSDEN			0x00000001

#define LCDC_OSDCTRL_IPU		0x00008000
#define LCDC_OSDCTRL_OSDRGB		0x00000010
#define LCDC_OSDCTRL_CHANGES		0x00000008
#define LCDC_OSDCTRL_OSDBPP_MASK	0x00000007
#define LCDC_OSDCTRL_OSDBPP_SHIFT	0
#define LCDC_OSDCTRL_OSDBPP_15_16	4
#define LCDC_OSDCTRL_OSDBPP_18_24	5
#define LCDC_OSDCTRL_OSDBPP_24_CMPRS	6

#define LCDC_OSDS_SOF1			0x00008000
#define LCDC_OSDS_EOF1			0x00004000
#define LCDC_OSDS_SOF0			0x00000800
#define LCDC_OSDS_EOF0			0x00000400
#define LCDC_OSDS_READY			0x00000001

#define LCDC_BGC_RED_MASK		0x00ff0000
#define LCDC_BGC_RED_SHIFT		16
#define LCDC_BGC_GREEN_MASK		0x0000ff00
#define LCDC_BGC_GREEN_SHIFT		8
#define LCDC_BGC_BLUE_MASK		0x000000ff
#define LCDC_BGC_BLUE_SHIFT		0

#define LCDC_KEY0_KEYEN			0x80000000
#define LCDC_KEY0_KEYMD			0x40000000
#define LCDC_KEY0_RED_MASK		0x00ff0000
#define LCDC_KEY0_RED_SHIFT		16
#define LCDC_KEY0_GREEN_MASK		0x0000ff00
#define LCDC_KEY0_GREEN_SHIFT		8
#define LCDC_KEY0_BLUE_MASK		0x000000ff
#define LCDC_KEY0_BLUE_SHIFT		0

#define LCDC_KEY1_KEYEN			0x80000000
#define LCDC_KEY1_KEYMD			0x40000000
#define LCDC_KEY1_RED_MASK		0x00ff0000
#define LCDC_KEY1_RED_SHIFT		16
#define LCDC_KEY1_GREEN_MASK		0x0000ff00
#define LCDC_KEY1_GREEN_SHIFT		8
#define LCDC_KEY1_BLUE_MASK		0x000000ff
#define LCDC_KEY1_BLUE_SHIFT		0

#define LCDC_IPUR_IPUREN		0x80000000
#define LCDC_IPUR_IPRU_MASK		0x00ffffff

#define LCDC_XYP0_YPOS_MASK		0x0fff0000
#define LCDC_XYP0_YPOS_SHIFT		16
#define LCDC_XYP0_XPOS_MASK		0x00000fff
#define LCDC_XYP0_XPOS_SHIFT		0

#define LCDC_XYP1_YPOS_MASK		0x0fff0000
#define LCDC_XYP1_YPOS_SHIFT		16
#define LCDC_XYP1_XPOS_MASK		0x00000fff
#define LCDC_XYP1_XPOS_SHIFT		0

#define LCDC_SIZE0_HEIGHT_MASK		0x0fff0000
#define LCDC_SIZE0_HEIGHT_SHIFT		16
#define LCDC_SIZE0_WIDTH_MASK		0x00000fff
#define LCDC_SIZE0_WIDTH_SHIFT		0

#define LCDC_SIZE1_HEIGHT_MASK		0x0fff0000
#define LCDC_SIZE1_HEIGHT_SHIFT		16
#define LCDC_SIZE1_WIDTH_MASK		0x00000fff
#define LCDC_SIZE1_WIDTH_SHIFT		0

struct LCDC {
	volatile unsigned int	cfg;	// 0x00
	volatile unsigned int	vsync;	// 0x04
	volatile unsigned int	hsync;	// 0x08
	volatile unsigned int	vat;	// 0x0c
	volatile unsigned int	dah;	// 0x10
	volatile unsigned int	dav;	// 0x14
	volatile unsigned int	ps;	// 0x18
	volatile unsigned int	cls;	// 0x1c
 	volatile unsigned int	spl;	// 0x20
 	volatile unsigned int	rev;	// 0x24
 	unsigned int		pad_0x28;
 	unsigned int		pad_0x2c;
	volatile unsigned int	ctrl;	// 0x30
	volatile unsigned int	state;	// 0x34
 	volatile unsigned int	iid;	// 0x38
 	unsigned int		pad_0x3c;
 	volatile unsigned int	DA0;	// 0x40
 	volatile unsigned int	SA0;	// 0x44
 	volatile unsigned int	FID0;	// 0x48
 	volatile unsigned int	CMD0;	// 0x4c
 	volatile unsigned int	DA1;	// 0x50
 	volatile unsigned int	SA1;	// 0x54
 	volatile unsigned int	FID1;	// 0x58
 	volatile unsigned int	CMD1;	// 0x5c
	volatile unsigned int	OFFS0;	// 0x60
	volatile unsigned int	PW0;	// 0x64
	volatile unsigned int	CNUM0;	// 0x68
	volatile unsigned int	DESSIZE0;// 0x6c
	volatile unsigned int	OFFS1; 	// 0x70
	volatile unsigned int	PW1;	// 0x74
	volatile unsigned int	CNUM1;	// 0x78
	volatile unsigned int	DESSIZE1;// 0x7c
	unsigned int		pad_0x80;// 0x80
	unsigned int		pad_0x84;// 0x84
	unsigned int		pad_0x88;// 0x88
	unsigned int		pad_0x8c;// 0x8c
	volatile unsigned int	RGBC;	// 0x90
	unsigned int		pad_0x94_0xfc[27];// 0x94
	volatile unsigned int	OSDC;	// 0x100
	volatile unsigned int	OSDCTRL;// 0x104
	volatile unsigned int	OSDS;	// 0x108
	volatile unsigned int 	BGC;	// 0x10c
	volatile unsigned int 	KEY0;	// 0x110
	volatile unsigned int 	KEY1;	// 0x114
	volatile unsigned int	ALPHA;	// 0x118
	volatile unsigned int	IPUR;	// 0x11c
	volatile unsigned int	XYP0;	// 0x120
	volatile unsigned int	XYP1;	// 0x124
	volatile unsigned int	SIZE0;	// 0x128
	volatile unsigned int	SIZE1;	// 0x12c
};
