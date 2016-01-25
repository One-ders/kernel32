
// CFG register bits

#define	LCDC_CFG_LCDPIN		0x80000000
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
#define LCDC_CFG_PDW_MASK	0x00000030
#define LCDC_CFG_PDW_SHIFT	4
#define LCDC_CFG_MODE_MASK	0x0000000f
#define LCDC_CFG_MODE_SHIFT	0
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

// VSYNC
// Vsync pulse start and end position  (in line clock)
#define LCDC_VSYNC_VPS_MASK		0x07ff0000
#define LCDC_VSYNC_VPS_SHIFT		16
#define LCDC_VSYNC_VPE_MASK		0x000007ff
#define LCDC_VSYNC_VPE_SHIFT		0

// HSYNC
// Hsync pulse start and end position (in dot clock)
#define LCDC_HSYNC_HPS_MASK		0x07ff0000
#define LCDC_HSYNC_HPS_SHIFT		16
#define LCDC_HSYNC_HPE_MASK		0x000007ff
#define LCDC_HSYNC_HPE_SHIFT		0

// VAT
// Virtual Area Setting (in dot clock)
#define LCDC_VAT_HT_MASK		0x07ff0000
#define LCDC_VAT_HT_SHIFT		16
#define LCDC_VAT_VT_MASK		0x000007ff
#define LCDC_VAT_VT_SHIFT		0

// DAV
// Display Area Vertical start/end (in line clock)
#define LCDC_DAV_VDS_MASK		0x07ff0000
#define LCDC_DAV_VDS_SHIFT		16
#define LCDC_DAV_VDE_MASK		0x000007ff
#define LCDC_DAV_VDE_SHIFT		0

// PS
// PS Signal Setting (in dot clock)
#define LCDC_PS_PSS_MASK		0x07ff0000
#define LCDC_PS_PSS_SHIFT		16
#define LCDC_PS_PSE_MASK		0x000007ff
#define LCDC_PS_PSE_SHIFT		0

// CLS
// CLS Signal Setting (in dot clock)
#define LCDC_CLS_CLSS_MASK		0x07ff0000
#define LCDC_CLS_CLSS_SHIFT		16
#define LCDC_CLS_CLSE_MASK		0x000007ff
#define LCDC_CLS_CLSE_SHIFT		0

// SPL
// SPL Signal Setting (in dot clock)
#define LCDC_SPL_SPLS_MASK		0x07ff0000
#define LCDC_SPL_SPLS_SHIFT		16
#define LCDC_SPL_SPLE_MASK		0x000007ff
#define LCDC_SPL_SPLE_SHIFT		0

// REV
// REV Signal Setting (in dot clock)
#define LCDC_REV_REVS_MASK		0x07ff0000
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

#define LCDC_STATE_QD			0x00000080
#define LCDC_STATE_EOF			0x00000020
#define LCDC_STATE_SOF			0x00000010
#define LCDC_STATE_OUF			0x00000008
#define LCDC_STATE_IFU0			0x00000004
#define LCDC_STATE_IFU1			0x00000002
#define LCDC_STATE_LDD			0x00000001

#define LCDC_CMD_SOFINT			0x80000000
#define LCDC_CMD_EOFINT			0x40000000
#define LCDC_CMD_PAL			0x10000000
#define LCDC_CMD_LEN_MASK		0x00ffffff


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
};
