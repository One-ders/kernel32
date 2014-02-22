
#include "sys.h"
#include "io.h"
#include "stm32f4xx.h"

#include "usb_core_drv.h"


 #define RX_FIFO_FS_SIZE                          128
 #define TX0_FIFO_FS_SIZE                          64
 #define TX1_FIFO_FS_SIZE                         128
 #define TX2_FIFO_FS_SIZE                          0
 #define TX3_FIFO_FS_SIZE                          0
 #define TXH_NP_HS_FIFOSIZ                         96
 #define TXH_P_HS_FIFOSIZ                          96


/* OnTheGo Defines */
#define MODE_HNP_SRP_CAPABLE                   0
#define MODE_SRP_ONLY_CAPABLE                  1
#define MODE_NO_HNP_SRP_CAPABLE                2
#define MODE_SRP_CAPABLE_DEVICE                3
#define MODE_NO_SRP_CAPABLE_DEVICE             4
#define MODE_SRP_CAPABLE_HOST                  5
#define MODE_NO_SRP_CAPABLE_HOST               6
#define A_HOST                                 1
#define A_SUSPEND                              2
#define A_PERIPHERAL                           3
#define B_PERIPHERAL                           4
#define B_HOST                                 5
#define DEVICE_MODE                            0
#define HOST_MODE                              1
#define OTG_MODE                               2


#define USB_OTG_HS_CORE_ID	0
#define USB_OTG_FS_CORE_ID	1


/* address offsets */
#define USB_OTG_HS_BASE_ADDR	0x40040000
#define USB_OTG_FS_BASE_ADDR	0x50000000

#define GREGS_OFFSET		0x000
#define DREGS_OFFSET		0x800
#define IN_EP_REGS_OFFSET	0x900
#define EP_REGS_SIZE		0x20
#define OUT_EP_REGS_OFFSET	0xB00
#define HOST_GLOBAL_REGS_OFFSET	0x400
#define HOST_PORT_REGS_OFFSET	0x440
#define HOST_CHAN_REGS_OFFSET	0x500
#define HOST_CHAN_REGS_SIZE	0x20
#define PCGCCTL_OFFSET		0xE00
#define DATA_FIFO_OFFSET	0x1000
#define DATA_FIFO_SIZE		0x1000


#define USB_SPEED_UNKNOWN	0
#define USB_SPEED_LOW		1
#define USB_SPEED_FULL		2
#define USB_SPEED_HIGH		3

#define USB_OTG_MAX_TX_FIFOS                 15

#define USB_OTG_HS_MAX_PACKET_SIZE           512
#define USB_OTG_FS_MAX_PACKET_SIZE           64
#define USB_OTG_MAX_EP0_SIZE                 64

#define USB_OTG_SPEED_PARAM_HIGH 0
#define USB_OTG_SPEED_PARAM_HIGH_IN_FULL 1
#define USB_OTG_SPEED_PARAM_FULL 3

#define USB_OTG_ULPI_PHY	1
#define USB_OTG_EMBEDDED_PHY	2
#define USB_OTG_I2C_PHY		3


#define USB_OTG_EP0_IDLE                          0
#define USB_OTG_EP0_SETUP                         1
#define USB_OTG_EP0_DATA_IN                       2
#define USB_OTG_EP0_DATA_OUT                      3
#define USB_OTG_EP0_STATUS_IN                     4
#define USB_OTG_EP0_STATUS_OUT                    5
#define USB_OTG_EP0_STALL                         6

#define USB_CONFIG_REMOTE_WAKEUP                           2
#define USB_CONFIG_SELF_POWERED                            1

#define USB_FEATURE_EP_HALT                                0
#define USB_FEATURE_REMOTE_WAKEUP                          1
#define USB_FEATURE_TEST_MODE                              2



struct usb_core_cfg {
	volatile unsigned char 	host_channels;
	volatile unsigned char 	dev_endpts;
	volatile unsigned char	speed;
	volatile unsigned char 	dma_enable;
	volatile unsigned short int mps;
	volatile unsigned short int total_fifo_size;
	volatile unsigned char 	phy;
	volatile unsigned char	sof_out;
	volatile unsigned char	low_power;
	volatile unsigned char	coreID;
};

#define STS_GOUT_NAK                           1
#define STS_DATA_UPDT                          2
#define STS_XFER_COMP                          3
#define STS_SETUP_COMP                         4
#define STS_SETUP_UPDT                         6


struct usb_core_gregs {
	volatile unsigned int g_otgctl;
#define OTGINT_RES0_1		0x00000003
#define OTGINT_SESENDDET	0x00000004
#define OTGINT_RES3_7		0x000000f8
#define OTGINT_SESREQUCSTSCHNG	0x00000100
#define OTGINT_HSTNEGSUCSTSCHNG	0x00000200
#define OTGINT_RES10_16		0x0001fc00
#define OTGINT_HSTNEGDEG	0x00020000
#define OTGINT_ADEVTOUTCHNG	0x00040000
#define OTGINT_DEBDON		0x00080000
	volatile unsigned int g_otgint;
#define AHBCFG_GLBL_IRQ_MASK	1
#define AHBCFG_HBURSTLEN_MASK	0x1e
#define AHBCFG_HBURSTLEN_SHIFT	1
#define AHBCFG_DMA_ENABLE	0x20
#define AHBCFG_TXFEMPLVL	0x80
#define AHBCFG_PTXFEMPLVL	0x100
	volatile unsigned int g_ahbcfg;

#define USBCFG_TOUTCAL_MASK	0x00000007
#define USBCFG_PHYIF		0x00000008
#define USBCFG_ULPI_ULMI_SEL	0x00000010
#define USBCFG_FSINTF		0x00000020
#define USBCFG_PHYSEL		0x00000040
#define USBCFG_DDRSEL		0x00000080
#define USBCFG_SRPCAP		0x00000100
#define USBCFG_HNPCAP		0x00000200
#define USBCFG_USBTRDTIM_MASK	0x00003C00
#define USBCFG_USBTRDTIM_SHIFT	10
#define USBCFG_NPTXFRWNDEN	0x00004000
#define USBCFG_PHYLPWRCLKSEL	0x00008000
#define USBCFG_OTGUTMIFSSEL	0x00010000
#define USBCFG_ULPI_FSLS	0x00020000
#define USBCFG_ULPI_AUTO_RES	0x00040000
#define USBCFG_ULPI_CLK_SUS_M	0x00080000
#define USBCFG_ULPI_EXT_VBUS_DRV 0x00100000
#define USBCFG_ULPI_INT_VBUS_IND 0x00200000
#define USBCFG_TERM_SEL_DL_PULSE 0x00400000
#define USBCFG_FORCE_HOST	0x20000000
#define USBCFG_FORCE_DEV	0x40000000
#define USBCFG_CORRUPT_TX	0x80000000

	volatile unsigned int g_usbcfg;

#define RSTCTL_CSFTRST		0x00000001
#define RSTCTL_HSFTRST		0x00000002
#define RSTCTL_HSTFRM		0x00000004
#define RSTCTL_INTKNQFLSH	0x00000008
#define RSTCTL_RXFFLSH		0x00000010
#define RSTCTL_TXFFLSH		0x00000020
#define RSTCTL_TXFNUM_MASK	0x000007c0
#define RSTCTL_TXFNUM_SHIFT	6
#define RSTCTL_RES11_26		0x3ffff800
#define RSTCTL_DMAREQ		0x40000000
#define RSTCTL_AHBIDLE		0x80000000
	volatile unsigned int g_rstctl;
	volatile unsigned int g_intsts;

#define INTMSK_RES0		0x00000001
#define INTMSK_MODEMISMATCH	0x00000002
#define INTMSK_OTGINTR		0x00000004
#define INTMSK_SOFINTR		0x00000008
#define INTMSK_RXSTSQLVL	0x00000010
#define INTMSK_NPTXFEMPTY	0x00000020
#define INTMSK_GINNAKEFF	0x00000040
#define INTMSK_GOUTNAKEFF	0x00000080
#define INTMSK_RES8		0x00000100
#define INTMSK_I2CINTR		0x00000200
#define INTMSK_ERLYSUSPEND	0x00000400
#define INTMSK_USBSUSPEND	0x00000800
#define INTMSK_USBRESET		0x00001000
#define INTMSK_ENUMDONE		0x00002000
#define INTMSK_ISOOUTDROP	0x00004000
#define INTMSK_EOPFRAME		0x00008000
#define INTMSK_RES16		0x00010000
#define INTMSK_EPMISMATCH	0x00020000
#define INTMSK_INEPINTR		0x00040000
#define INTMSK_OUTEPINTR	0x00080000
#define INTMSK_INCOMPLISOIN	0x00100000
#define INTMSK_INCOMPLISOOUT	0x00200000
#define INTMSK_RES22_23		0x00c00000
#define INTMSK_PORTINTR		0x01000000
#define INTMSK_HCINTR		0x02000000
#define INTMSK_PTXFEMPTY	0x04000000
#define INTMSK_RES27		0x08000000
#define INTMSK_CONIDSTSCHNG	0x10000000
#define INTMSK_DISCONNECT	0x20000000
#define INTMSK_SESSREQINTR	0x40000000
#define INTMSK_WKUPINTR		0x80000000
	volatile unsigned int g_intmsk;
	volatile unsigned int g_rxstsr;

#define RXSTS_EPNUM_MASK	0x0000000f
#define RXSTS_EPNUM_SHIFT	0
#define RXSTS_BCNT_MASK		0x00007ff0
#define RXSTS_BCNT_SHIFT	4
#define RXSTS_DPID_MASK		0x00018000
#define RXSTS_DPID_SHIFT	15
#define RXSTS_PKTSTS_MASK	0x001e0000
#define RXSTS_PKTSTS_SHIFT	17
#define RXSTS_FN_MASK		0x01e00000
#define RXSTS_FN_SHIFT
	volatile unsigned int g_rxstsp;

#define FSIZE_START_MASK 0xffff
#define FSIZE_START_SHIFT 0
#define FSIZE_DEPTH_MASK 0xffff0000
#define FSIZE_DEPTH_SHIFT 16
	volatile unsigned int g_rxfsize;
	volatile unsigned int g_dieptxfo_hnptxfsize;
	volatile unsigned int g_hnptx_sts;

#define I2CCTL_RWDATA	0x000000ff
#define I2CCTL_REGADDR  0x0000ff00
#define I2CCTL_ADDR_MASK	0x007f0000
#define I2CCTL_ADDR_SHIFT	16
#define I2CCTL_I2CEN	0x00800000
#define I2CCTL_ACK	0x01000000
#define I2CCTL_I2CSUSP	0x02000000
#define I2CCTL_I2CDADDR_MASK	0x0C000000
#define I2CCTL_I2CDADDR_SHIFT	26
#define I2CCTL_DAT_SE0	0x10000000
#define I2CCTL_RW	0x40000000
#define I2CCTL_BSYDNE	0x80000000

	volatile unsigned int g_i2cctl;
	volatile unsigned int res_0x34;

#define CCFG_PWDN	0x00010000
#define CCFG_I2CIFEN	0x00020000
#define CCFG_VBUSSENSA	0x00040000
#define CCFG_VBUSSENSB  0x00080000
#define CCFG_SOFOUTEN	0X00100000
#define CCFG_DIS_BUSSENSE 0x00200000

	volatile unsigned int g_ccfg;
	volatile unsigned int g_cid;
	unsigned int res_0x40[48];
	volatile unsigned int g_hptxfsize;
	volatile unsigned int g_diep_txf[USB_OTG_MAX_TX_FIFOS];
};

#define DCFG_FRAME_INTERVAL_80                 0
#define DCFG_FRAME_INTERVAL_85                 1
#define DCFG_FRAME_INTERVAL_90                 2
#define DCFG_FRAME_INTERVAL_95                 3

struct usb_core_dregs {
#define DEVCFG_SPEED_MASK	0x00000003
#define DEVCFG_SPEED_SHIFT	0
#define DEVCFG_NZSTSOUTHSHK	0x00000004
#define DEVCFG_RES3		0x00000008
#define DEVCFG_ADDR_MASK	0x000007f0
#define DEVCFG_ADDR_SHIFT	4
#define DEVCFG_PERFRINT_MASK	0x00001800
#define DEVCFG_PERFRINT_SHIFT	11
#define DEVCFG_RES13_17		0x0003e000
#define DEVCFG_EPMSCNT_MASK	0x003c0000
#define DEVCFG_EPMSCNT_SHIFT	18
	volatile unsigned int d_cfg;

#define DEVCTL_RMTWKUPSIG	0x00000001
#define DEVCTL_SFTDISCON	0x00000002
#define DEVCTL_GNPINNAKSTS	0x00000004
#define DEVCTL_GOUTNAKSTS	0x00000008
#define DEVCTL_TSTCTL_MASK	0x00000070
#define DEVCTL_TSTCTL_SHIFT	4
#define DEVCTL_SGNPINNAK	0x00000080
#define DEVCTL_CGNPINNAK	0x00000100
#define DEVCTL_SGOUTNAK		0x00000200
#define DEVCTL_CGOUTNAK		0x00000400
	volatile unsigned int d_ctl;

#define STS_SUSPSTS		0x00000001
#define STS_ENUMSPD_MASK	0x00000006
#define STS_ENUMSPD_SHIFT	1
#define STS_ERRTICERR		0x00000008
#define STS_RES4_7		0x000000f0
#define STS_SOFFN_MASK		0x003fff00
#define STS_SOFFN_SHIFT		8

	volatile unsigned int d_sts;
	volatile unsigned int res_0x0c;

#define DIEPINT_XFERCOMPL	0x00000001
#define DIEPINT_EPDISABLED	0x00000002
#define DIEPINT_AHBERR		0x00000004
#define DIEPINT_TIMEOUT		0x00000008
#define DIEPINT_INTKTXFEMP	0x00000010
#define DIEPINT_INTKNEPMIS	0x00000020
#define DIEPINT_INEPNAKEFF	0x00000040
#define DIEPINT_EMPTYINTR	0x00000080
#define DIEPINT_TXFIFOUNDRN	0x00000100

	volatile unsigned int d_in_ep_msk;

#define DOEPINT_XFERCOMPL	0x00000001
#define DOEPINT_EPDISABLED	0x00000002
#define DOEPINT_AHBERR		0x00000004
#define DOEPINT_SETUP		0x00000008
	volatile unsigned int d_out_ep_msk;

#define DALLIRQ_IN_MASK		0x0000ffff
#define DALLIRQ_IN_SHIFT	0
#define DALLIRQ_OUT_MASK	0xffff0000
#define DALLIRQ_OUT_SHIFT	16
	volatile unsigned int d_all_irq;
	volatile unsigned int d_all_irq_msk;
	volatile unsigned int res_0x20;
	volatile unsigned int res_0x24;
	volatile unsigned int dev_vbus_discharge;
	volatile unsigned int dev_vbus_pulse;
#define DTHRCTL_NISOTHR_EN	0x00000001
#define DTHRCTL_ISOTHR_EN	0x00000002
#define DTHRCTL_TXTHRLEN_MASK	0x000007fc
#define DTHRCTL_TXTHRLEN_SHIFT	2
#define DTHRCTL_RES11_15	0x0000f800
#define DTHRCTL_RXTHR_EN	0x00010000
#define DTHRCTL_RXTHRLEN_MASK	0x03fe0000
#define DTHRCTL_RXTHRLEN_SHIFT	17
#define DTHRCTL_RES26_31	0xfc000000
	volatile unsigned int d_thrctl;
	volatile unsigned int d_in_ep_empty_msk;
	volatile unsigned int d_each_irq;
	volatile unsigned int d_each_msk;
	unsigned int res_0x40;
	volatile unsigned int dev_in_ep_1msk;
	unsigned int res_0x44[15];
	volatile unsigned int dev_out_ep_1msk;
};

struct usb_core_hregs {
	volatile unsigned int host_cfg;
	volatile unsigned int host_fir;
	volatile unsigned int host_fnum;
	unsigned int res_0x40c;
	volatile unsigned int host_p_tx_stat;
	volatile unsigned int host_all_chan_irq;
	volatile unsigned int host_all_chan_irq_msg;
};

#define DEPCTL_MPS_MASK 	0x000007ff
#define DEPCTL_MPS_SHIFT 	0
#define DEPCTL_USBACTEP 	0x00008000
#define DEPCTL_DPID		0x00010000
#define DEPCTL_NAKSTST		0x00020000
#define DEPCTL_EPTYPE_MASK 	0x000c0000
#define DEPCTL_EPTYPE_SHIFT 	18
#define DEPCTL_SNP		0x00100000
#define DEPCTL_STALL		0x00200000
#define DEPCTL_TXFNUM_MASK	0x03c00000
#define DEPCTL_TXFNUM_SHIFT	22
#define DEPCTL_CNAK		0x04000000
#define DEPCTL_SNAK		0x08000000
#define DEPCTL_SETD0PID		0x10000000
#define DEPCTL_SETD1PID		0x20000000
#define DEPCTL_EPDIS		0x40000000
#define DEPCTL_EPENA		0x80000000

#define DSTS_ENUMSPD_HS_PHY_30MHZ_OR_60MHZ     0
#define DSTS_ENUMSPD_FS_PHY_30MHZ_OR_60MHZ     1
#define DSTS_ENUMSPD_LS_PHY_6MHZ               2
#define DSTS_ENUMSPD_FS_PHY_48MHZ              3

#define DEP0CTL_MPS_64                         0
#define DEP0CTL_MPS_32                         1
#define DEP0CTL_MPS_16                         2
#define DEP0CTL_MPS_8                          3

#define DEP0XFERSIZE_XFERSIZE_MASK	0x0000007f
#define DEP0XFERSIZE_XFERSIZE_SHIFT	0
#define DEP0XFERSIZE_RES7_18		0x0007ff80
#define DEP0XFERSIZE_PKTCNT_MASK		0x00180000
#define DEP0XFERSIZE_PKTCNT_SHIFT	19
#define DEP0XFERSIZE_RES_20_28		0x1ff00000
#define DEP0XFERSIZE_SUPCNT_MASK		0x60000000
#define DEP0XFERSIZE_SUPCNT_SHIFT	29

#define DEPXFERSIZE_XFERSIZE_MASK	0x0007ffff
#define DEPXFERSIZE_XFERSIZE_SHIFT	0
#define DEPXFERSIZE_PKTCNT_MASK		0x1ff80000
#define DEPXFERSIZE_PKTCNT_SHIFT	19
#define DEPXFERSIZE MC_MASK		0x60000000
#define DEPXFERSIZE_MC_SHIFT		29

#define DTXFSTS_TXFSPCAVAIL_MASK	0x0000ffff
#define DTXFSTS_TXFSPCAVAIL_SHIFT	0

struct usb_core_in_epregs {
	volatile unsigned int d_iepctl;
	unsigned int res_0x04;
	volatile unsigned int d_in_ep_irq;
	unsigned int res_0x0c;
	volatile unsigned int d_ieptsiz;
	volatile unsigned int d_iepdma;
	volatile unsigned int d_txfsts;
	unsigned int res_0x18;
};

struct usb_core_out_epregs {
	volatile unsigned int d_oepctl;
	volatile unsigned int dev_out_ep_frame;
	volatile unsigned int d_out_ep_irq;
	unsigned int res_0x0c;
	volatile unsigned int d_oeptsiz;
	volatile unsigned int d_oepdma;
	unsigned int res_0x18[2];
};

struct usb_core_hc_regs {
	volatile unsigned int host_channel_char;
	volatile unsigned int host_channel_splt;
	volatile unsigned int host_channel_irq;
	volatile unsigned int host_channel_irq_msk;
	volatile unsigned int host_channel_tsize;
	volatile unsigned int host_channel_dma;
	unsigned int res[2];
};

struct usb_core_regs {
	struct usb_core_gregs	*gregs;
	struct usb_core_dregs	*dregs;
	struct usb_core_hregs	*hregs;
	struct usb_core_in_epregs	*in_epregs [USB_OTG_MAX_TX_FIFOS];
	struct usb_core_out_epregs	*out_epregs[USB_OTG_MAX_TX_FIFOS];
	struct usb_core_hcregs		*hcregs    [USB_OTG_MAX_TX_FIFOS];
	volatile unsigned int	*hprt0;
	volatile unsigned int	*dfifo             [USB_OTG_MAX_TX_FIFOS];

#define PCGCCTL_STOPPCLK	0x00000001
#define	PCGCCTL_GATEHCLK	0x00000002
	volatile unsigned int	*pcgcctl;
};

#define EP_SPEED_LOW                           0
#define EP_SPEED_FULL                          1
#define EP_SPEED_HIGH                          2

#define EP_TYPE_CTRL                           0
#define EP_TYPE_ISOC                           1
#define EP_TYPE_BULK                           2
#define EP_TYPE_INTR                           3
#define EP_TYPE_MSK                            3

struct usb_otg_ep {
	unsigned char num;
	unsigned char is_in;
	unsigned char is_stall;
	unsigned char type;
	unsigned char data_pid_start;
	unsigned char even_odd_frame;
	unsigned short int tx_fifo_num;
	unsigned int max_packet;
	unsigned char *xfer_buf;
	unsigned int dma_addr;
	unsigned int xfer_len;
	unsigned int xfer_count;
	unsigned int rem_data_len;
	unsigned int total_data_len;
	unsigned int ctl_data_len;
};

/*  Device Status */
#define USB_OTG_DEFAULT                          1
#define USB_OTG_ADDRESSED                        2
#define USB_OTG_CONFIGURED                       3
#define USB_OTG_SUSPENDED                        4


struct usb_dcd_dev {

	unsigned char dev_config;


	unsigned char dev_state;

#define DSTS_ENUMSPD_HS_PHY_30MHZ_OR_60MHZ     0
#define DSTS_ENUMSPD_FS_PHY_30MHZ_OR_60MHZ     1
#define DSTS_ENUMSPD_LS_PHY_6MHZ               2
#define DSTS_ENUMSPD_FS_PHY_48MHZ              3
	unsigned char dev_status;
	unsigned char dev_address;
	unsigned int  dev_remote_wakeup;
	struct usb_otg_ep	in_ep [USB_OTG_MAX_TX_FIFOS];
	struct usb_otg_ep	out_ep[USB_OTG_MAX_TX_FIFOS];
	unsigned char setup_packet[3*8];
	struct usbd_class_cb	*class_cb;
	struct usbd_usr_cb	*usr_cb;
	struct usbd_device	*usr_dev;
	unsigned char 		*pConfig_desc;
};

static struct usb_core_data {
	struct usb_core_cfg	cfg;
	struct usb_core_regs	regs;
	struct usb_dcd_dev	dev;
} usb_core_data;

#define READ_REG32(reg) (*(volatile unsigned int *)reg)
#define WRITE_REG32(reg,value) (*(volatile unsigned int *)reg = value)
#define MOD_REG32(reg,clear_mask,set_mask) \
	WRITE_REG32(reg,(((READ_REG32(reg)) & ~clear_mask) | set_mask ))
#define MIN(a,b) (a<b)?(a):(b)


static int usb_get_dev_speed(struct usb_core_data *core);
static int usb_otg_ep_set_stall(struct usb_core_data *core, struct usb_otg_ep *ep);
static int usb_otg_ep_clr_stall(struct usb_core_data *core, struct usb_otg_ep *ep);
static int usb_otg_write_packet(struct usb_core_data *core, unsigned char *src,
				unsigned char ep_num, unsigned short int len);
static int usb_otg_ep_activate(struct usb_core_data *core, struct usb_otg_ep *ep);
static int usb_otg_ep0_start_xfer(struct usb_core_data *core, struct usb_otg_ep *ep);
static int usb_otg_ep_start_xfer(struct usb_core_data *core, struct usb_otg_ep *ep);

static unsigned char usbd_default_cfg=0;

static void usb_udelay(unsigned int usec) {
	unsigned int count=0;
	unsigned int utime=(120*usec/7);
	do {
		if (++count>utime) {
			return;
		}
	} while (1);
}

static void usb_mdelay(unsigned int msec) {
	usb_udelay(msec*1000);
}


static unsigned char usbd_get_len(char *buf) {
	unsigned char len=0;
	while(*buf) {
		len++;
		buf++;
	}
	return len;
}


void usbd_get_string(char *desc, unsigned char *unicode, unsigned short *len) {
	unsigned char idx=0;
	if(desc) {
		*len=usbd_get_len(desc)*2+2;
		unicode[idx++]=*len;
		unicode[idx++]=USB_DESC_TYPE_STRING;

		while(*desc) {
			unicode[idx++]=*desc++;
			unicode[idx++]=0x00;
		}
	}
}

/**********************************************************************
 *  dcd functions
 **********************************************************************/
static int dcd_ep_stall(struct usb_core_data *core, unsigned char epnum) {
	struct usb_otg_ep *ep;
	io_printf("dcd_ep_stall epnum%x\n",epnum);
	if (epnum&0x80) {
		ep=&core->dev.in_ep[epnum&0x7f];
	} else {
		ep=&core->dev.out_ep[epnum];
	}

	ep->is_stall=1;
	ep->num=epnum&0x7f;
	ep->is_in=((epnum&0x80)!=0);
	
	usb_otg_ep_set_stall(core,ep);
	return 0;
}

static int dcd_ep_clr_stall(struct usb_core_data *core, unsigned char epnum) {
	struct usb_otg_ep *ep;
	if (epnum&0x80) {
		ep=&core->dev.in_ep[epnum&0x7f];
	} else {
		ep=&core->dev.out_ep[epnum];
	}

	ep->is_stall=0;
	ep->num=epnum&0x7f;
	ep->is_in=((epnum&0x80)==0x80);

	usb_otg_ep_clr_stall(core,ep);
	return 9;
}

int dcd_ep_tx(void *vcore, unsigned char ep_addr, unsigned char *pbuf, unsigned int len) {
	struct usb_core_data *core=(struct usb_core_data *)vcore;
	struct usb_otg_ep *ep;

	ep=&core->dev.in_ep[ep_addr&0x7f];

	ep->is_in=1;
	ep->num=ep_addr&0x7f;
	ep->xfer_buf=pbuf;
	ep->dma_addr=(unsigned int)pbuf;
	ep->xfer_count=0;
	ep->xfer_len=len;

	if (!ep->num) {
		usb_otg_ep0_start_xfer(core,ep);
	} else {
		usb_otg_ep_start_xfer(core,ep);
	}
	return 0;
}

int dcd_ep_open(void *vcore, unsigned char ep_addr,
			unsigned short int ep_mps, unsigned char ep_type) {
	struct usb_core_data *core=(struct usb_core_data *)vcore;
	struct usb_otg_ep *ep;

	if (ep_addr&0x80) {
		ep=&core->dev.in_ep[ep_addr&0x7f];
	} else {
		ep=&core->dev.out_ep[ep_addr&0x7f];
	}
	ep->num=ep_addr&0x7f;

	ep->is_in=(0x80&ep_addr)!=0;
	ep->max_packet=ep_mps;
	ep->type=ep_type;
	
	if (ep->is_in) {
		ep->tx_fifo_num=ep->num;
	}

	if (ep_type==USB_OTG_EP_BULK) {
		ep->data_pid_start=0;
	}
	usb_otg_ep_activate(core,ep);
	return 0;
}

unsigned short int get_rx_cnt(void *vcore, int epnum) {
	struct usb_core_data *core=(struct usb_core_data *)vcore;
	return core->dev.out_ep[epnum].xfer_count;
}

int dcd_ep_prepare_rx(void *vcore, unsigned char ep_addr,
				unsigned char *pbuf, unsigned short int len) {
	struct usb_core_data *core=(struct usb_core_data *)vcore;
	struct usb_otg_ep *ep;
	ep=&core->dev.out_ep[ep_addr&0x7f];

	ep->xfer_buf=pbuf;
	ep->xfer_len=len;
	ep->xfer_count=0;
	ep->is_in=0;
	ep->num=ep_addr&0x7f;

	if (core->cfg.dma_enable) {
		ep->dma_addr=(unsigned int)pbuf;
	}

	if (!ep->num) {
		usb_otg_ep0_start_xfer(core,ep);
	} else {
		usb_otg_ep_start_xfer(core,ep);
	}
	return 0;
}

static void dcd_ep_set_address(struct usb_core_data *core, unsigned char address) {
	unsigned int dcfg;
	dcfg=address<<DEVCFG_ADDR_SHIFT;
	MOD_REG32(&core->regs.dregs->d_cfg,0,dcfg);
}

static int dcd_set_cfg(struct usb_core_data *core, unsigned char cfgidx) {
	core->dev.class_cb->init(core,cfgidx);
	core->dev.usr_cb->dev_configured();
	return 0;
}

static int dcd_clr_cfg(struct usb_core_data *core, unsigned char cfgidx) {
	core->dev.class_cb->deinit(core,cfgidx);
	return 0;
}


static void usb_ep0_out_start(struct usb_core_data *core);

static int usbd_ctl_cont_rx(struct usb_core_data *core, unsigned char *pbuf, unsigned short int len) {
	dcd_ep_prepare_rx(core,0,pbuf,len);
	return 0;
}

static int usbd_ctl_send_status(struct usb_core_data *core) {
	core->dev.dev_state=USB_OTG_EP0_STATUS_IN;
	
	dcd_ep_tx(core,0,0,0);
	usb_ep0_out_start(core);
	return 0;
}

static int usbd_ctl_send_data(struct usb_core_data *core, unsigned char *pbuf, unsigned short int len) {
	core->dev.in_ep[0].total_data_len=len;
	core->dev.in_ep[0].rem_data_len=len;
	core->dev.dev_state=USB_OTG_EP0_DATA_IN;
	dcd_ep_tx(core,0,pbuf,len);
	return 0;
}

static int usbd_ctl_continue_send_data(struct usb_core_data *core, unsigned char *pbuf, unsigned short int len) {
	dcd_ep_tx(core,0,pbuf,len);
	return 0;
}

static int usbd_ctl_receive_status(struct usb_core_data *core) {
	core->dev.dev_state=USB_OTG_EP0_STATUS_OUT;
	dcd_ep_prepare_rx(core,0,0,0);
	usb_ep0_out_start(core);
	return 0;
}

int usbd_ctl_error(void *vcore, struct usb_setup_req *req) {
	struct usb_core_data *core=(struct usb_core_data *)vcore;

	if (req->bmRequest&0x80) {
		dcd_ep_stall(core,0x80);
	} else {
		if (!req->wLength) {
			dcd_ep_stall(core,0x80);
		} else {
			dcd_ep_stall(core,0);
		}
	}
	usb_ep0_out_start(core);
	return 0;
}

static int usbd_get_descriptor(struct usb_core_data *core, struct usb_setup_req *req) {
	unsigned short int len;
	unsigned char *pbuf;
	switch(req->wValue>>8) {
		case USB_DESC_TYPE_DEVICE:
			pbuf=core->dev.usr_dev->get_device_descriptor(core->cfg.speed,&len);
			if ((req->wLength==64)||(core->dev.dev_status==USB_OTG_DEFAULT)) {
				len=8;
			}
			io_printf("get device_descriptor, len=%d\n",len);
			break;
		case USB_DESC_TYPE_CONFIGURATION:
			pbuf=(unsigned char *)core->dev.class_cb->get_config_descriptor(core->cfg.speed,&len);
#if 0
  HS code here
#endif
			pbuf[1]=USB_DESC_TYPE_CONFIGURATION;
			core->dev.pConfig_desc=pbuf;
			break;
		case USB_DESC_TYPE_STRING:
			switch((unsigned char)(req->wValue)) {
				case USBD_IDX_LANGID_STR:
					pbuf=core->dev.usr_dev->get_langIdStr_descriptor(core->cfg.speed,&len);
					break;
				case USBD_IDX_MFC_STR:
					pbuf=core->dev.usr_dev->get_manufacturerStr_descriptor(core->cfg.speed,&len);
					break;
				case USBD_IDX_PRODUCT_STR:
					pbuf=core->dev.usr_dev->get_productStr_descriptor(core->cfg.speed,&len);
					break;
				case USBD_IDX_SERIAL_STR:
					pbuf=core->dev.usr_dev->get_serialStr_descriptor(core->cfg.speed,&len);
					break;
				case USBD_IDX_CONFIG_STR:
					pbuf=core->dev.usr_dev->get_configStr_descriptor(core->cfg.speed,&len);
					break;
				case USBD_IDX_INTERFACE_STR:
					pbuf=core->dev.usr_dev->get_interfaceStr_descriptor(core->cfg.speed,&len);
					break;
				default:
#ifdef USB_SUPPORT_USER_STRING_DESC
					pbuf=core->dev.class_cb->getUsrStr_descriptor(core->cfg.speed,req->wValue,&len);					
					break;
#else
					usbd_ctl_error(core,req);
					return 0;
					break;
#endif
			}
			break;
		case USB_DESC_TYPE_DEVICE_QUALIFIER:
#if 0
   HS code here
#else
			usbd_ctl_error(core,req);
#endif
			return 0;
			break;
		case USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION:
#if 0
	HS code here
#else
			usbd_ctl_error(core,req);
			return 0;
			break;
#endif
		default:
			usbd_ctl_error(core,req);
			return 0;
	}

	if (len&&req->wLength) {
		len=MIN(len,req->wLength);
		io_printf("ctl_send_data: pbuf@%x, len=%d\n",pbuf,len);
		usbd_ctl_send_data(core,pbuf,len);
	}
	return 0;
}

static int usbd_set_address(struct usb_core_data *core, struct usb_setup_req *req) {
	unsigned char dev_addr;

	io_printf("usbd_set_address\n");
	if ((!req->wIndex)&&(!req->wLength)) {
		dev_addr=(unsigned char)req->wValue&0x7f;

		if (core->dev.dev_status==USB_OTG_CONFIGURED) {
			io_printf("usbd_set_address, bad state\n");
			usbd_ctl_error(core,req);
		} else {
			core->dev.dev_address=dev_addr;
			dcd_ep_set_address(core,dev_addr);
			usbd_ctl_send_status(core);

			if (dev_addr) {
				io_printf("dev_status->OTG_ADDRESSED addr=%d\n",dev_addr);
				core->dev.dev_status=USB_OTG_ADDRESSED;
			} else {
				io_printf("dev_status->OTG_DEFAULT\n");
				core->dev.dev_status=USB_OTG_DEFAULT;
			}
		}
	} else {
		io_printf("usbd_set_address, bad req data\n");
		usbd_ctl_error(core,req);
	}
	return 0;
}

static int usbd_set_config(struct usb_core_data *core, struct usb_setup_req *req) {
	static unsigned char cfgidx;

	cfgidx=(unsigned char)(req->wValue);
	if (cfgidx>USBD_CFG_MAX_NUM) { 
		usbd_ctl_error(core,req);
	} else {
		switch(core->dev.dev_status) {
			case USB_OTG_ADDRESSED:
				if (cfgidx) {
					core->dev.dev_config=cfgidx;
					core->dev.dev_status=USB_OTG_CONFIGURED;
					dcd_set_cfg(core,cfgidx);
					usbd_ctl_send_status(core);
				} else {
					usbd_ctl_send_status(core);
				}
				break;
			case USB_OTG_CONFIGURED:
				if (!cfgidx) {
					core->dev.dev_status=USB_OTG_ADDRESSED;
					core->dev.dev_config=cfgidx;
					dcd_clr_cfg(core,cfgidx);
					usbd_ctl_send_status(core);
				} else if (cfgidx!=core->dev.dev_config) {
					dcd_clr_cfg(core,core->dev.dev_config);
					core->dev.dev_config=cfgidx;
					dcd_set_cfg(core,cfgidx);
					usbd_ctl_send_status(core);
				} else {
					usbd_ctl_send_status(core);
				}
				break;
			default:
				usbd_ctl_error(core,req);
				break;
		}
	}
	return 0;
}

static int usbd_get_config(struct usb_core_data *core, struct usb_setup_req *req) {
	io_printf("usbd_get_config\n");
	if(req->wLength!=1) {
		usbd_ctl_error(core,req);
	} else {
		switch(core->dev.dev_status) {
			case USB_OTG_ADDRESSED:
				usbd_ctl_send_data(core,&usbd_default_cfg,1);
				break;
			case USB_OTG_CONFIGURED:
				usbd_ctl_send_data(core,&core->dev.dev_config,1);
				break;
			default:
				usbd_ctl_error(core,req);
				break;
		}
	}
	return 0;
}

static int usbd_get_status(struct usb_core_data *core, struct usb_setup_req *req) {
	io_printf("usbd_get_status\n");
	unsigned char usbd_cfg_status;
	switch(core->dev.dev_status) {
		case USB_OTG_ADDRESSED:
		case USB_OTG_CONFIGURED:
			if (core->dev.dev_remote_wakeup) {
				usbd_cfg_status=USB_CONFIG_SELF_POWERED | USB_CONFIG_REMOTE_WAKEUP;
			} else {
				usbd_cfg_status=USB_CONFIG_SELF_POWERED;
			}
			usbd_ctl_send_data(core,&usbd_cfg_status,1);
			break;
		default:
			usbd_ctl_error(core,req);
			break;
	}
	return 0;
}

static int usbd_set_feature(struct usb_core_data *core, struct usb_setup_req *req) {

	unsigned int dctl;
	unsigned char test_mode=0;
	
	if (req->wValue==USB_FEATURE_REMOTE_WAKEUP) {
		core->dev.dev_remote_wakeup=1;
		core->dev.class_cb->setup(core,req);
		usbd_ctl_send_status(core);
	} else if ((req->wValue==USB_FEATURE_TEST_MODE) &&
		   (!(req->wIndex&0xff))) {
		dctl=READ_REG32(&core->regs.dregs->d_ctl);
		test_mode=req->wIndex<<8;
		switch(test_mode) {
			case 1: // TEST_J
				dctl=(1<<DEVCTL_TSTCTL_SHIFT);
				break;
			case 2: // TEST_K
				dctl=(2<<DEVCTL_TSTCTL_SHIFT);
				break;
			case 3: // TEST_SE0_NAK
				dctl=(3<<DEVCTL_TSTCTL_SHIFT);
				break;
			case 4: // TEST_PACKET
				dctl=(4<<DEVCTL_TSTCTL_SHIFT);
				break;
			case 5: // TEST_FORCE_ENABLE
				dctl=(5<<DEVCTL_TSTCTL_SHIFT);
				break;
		}
		WRITE_REG32(&core->regs.dregs->d_ctl,dctl);
		usbd_ctl_send_status(core);
	}
	return 0;
}

static int usbd_clr_feature(struct usb_core_data *core, struct usb_setup_req *req) {
	switch(core->dev.dev_status) {
		case USB_OTG_ADDRESSED:
		case USB_OTG_CONFIGURED:
			if (req->wValue==USB_FEATURE_REMOTE_WAKEUP) {
				core->dev.dev_remote_wakeup=0;
				core->dev.class_cb->setup(core,req);
				usbd_ctl_send_status(core);
			}
			break;
		default:
			usbd_ctl_error(core,req);
			break;
	}
	return 0;
}


/***************************************************************************************
 * setup packet received
 *
 ***************************************************************************************/

static int usbd_std_dev_req(struct usb_core_data *core, struct usb_setup_req *req) {
	switch(req->bRequest) {
		case USB_REQ_GET_DESCRIPTOR:
			usbd_get_descriptor(core,req);
			break;
		case USB_REQ_SET_ADDRESS:
			usbd_set_address(core,req);
			break;
		case USB_REQ_SET_CONFIGURATION:
			usbd_set_config(core,req);
			break;
		case USB_REQ_GET_CONFIGURATION:
			usbd_get_config(core,req);
			break;
		case USB_REQ_GET_STATUS:
			usbd_get_status(core,req);
			break;
		case USB_REQ_SET_FEATURE:
			usbd_set_feature(core,req);
			break;
		case USB_REQ_CLEAR_FEATURE:
			usbd_clr_feature(core,req);
			break;
		default:
			usbd_ctl_error(core,req);
			break;
	}
	return 0;
}

static int usbd_std_itf_req(struct usb_core_data *core, struct usb_setup_req *req) {
	int rc=0;
	switch(core->dev.dev_status) {
		case USB_OTG_CONFIGURED:
			if (LOBYTE(req->wIndex)<=USBD_ITF_MAX_NUM) {
				rc=core->dev.class_cb->setup(core,req);
				if ((!req->wLength)&&!rc) {
					usbd_ctl_send_status(core);
				}
			} else {
				usbd_ctl_error(core,req);
			}
			break;
		default:
			usbd_ctl_error(core,req);
			break;
	}
	return rc;
}

static int usbd_std_ep_req(struct usb_core_data *core, struct usb_setup_req *req) {
	unsigned char ep_addr=LOBYTE(req->wIndex);
	unsigned int usbd_ep_status;

	switch(req->bRequest) {
		case USB_REQ_SET_FEATURE:
			switch(core->dev.dev_status) {
				case USB_OTG_ADDRESSED:
					if (ep_addr&&(ep_addr!=0x80)) {
						dcd_ep_stall(core,ep_addr);
					}
					break;
				case USB_OTG_CONFIGURED:
					if (req->wValue==USB_FEATURE_EP_HALT) {
						if (ep_addr&&(ep_addr!=0x80)) {
							dcd_ep_stall(core,ep_addr);
						}
					}
					core->dev.class_cb->setup(core,req);
					usbd_ctl_send_status(core);
					break;
				default:
					usbd_ctl_error(core,req);
					break;
			}
			break;
		case USB_REQ_CLEAR_FEATURE:
			switch(core->dev.dev_status) {
				case USB_OTG_ADDRESSED:
					if(ep_addr&&(ep_addr!=0x80)) {
						dcd_ep_stall(core,ep_addr);
					}
					break;
				case USB_OTG_CONFIGURED:
					if (req->wValue==USB_FEATURE_EP_HALT) {
						if (ep_addr&&(ep_addr!=0x80)) {
							dcd_ep_clr_stall(core,ep_addr);
							core->dev.class_cb->setup(core,req);
						}
						usbd_ctl_send_status(core);
					}
					break;
				default:
					usbd_ctl_error(core,req);
					break;
			}
			break;
		case USB_REQ_GET_STATUS:
			switch(core->dev.dev_status) {
				case USB_OTG_ADDRESSED:
					if (ep_addr&&(ep_addr&0x80)) {
						dcd_ep_stall(core,ep_addr);
					}
					break;
				case USB_OTG_CONFIGURED:
					if (ep_addr&0x80) {
						if (core->dev.in_ep[ep_addr&0x7f].is_stall) {
							usbd_ep_status=1;
						} else {
							usbd_ep_status=0;
						}
					} else {
						if(core->dev.out_ep[ep_addr].is_stall) {
							usbd_ep_status=1;
						} else {
							usbd_ep_status=0;
						}
					}
					usbd_ctl_send_data(core,(unsigned char *)&usbd_ep_status,2);
					break;
				default:
					usbd_ctl_error(core,req);
					break;
			}
			break;
		default:
			break;
	}
	return 0;
}

static int usbd_parse_setup_req(struct usb_core_data *core,struct usb_setup_req *req) {
	req->bmRequest	= *(unsigned char *)(core->dev.setup_packet);
	req->bRequest	= *(unsigned char *)(core->dev.setup_packet+1);
	req->wValue	= SWAPBYTE(core->dev.setup_packet+2);
	req->wIndex	= SWAPBYTE(core->dev.setup_packet+4);
	req->wLength	= SWAPBYTE(core->dev.setup_packet+6);

	core->dev.in_ep[0].ctl_data_len=req->wLength;
	core->dev.dev_state=USB_OTG_EP0_SETUP;
	return 0;
}

static void *usb_otg_read_packet(struct usb_core_data *core, unsigned char *dst, unsigned int len) {
	unsigned int i;
	unsigned int count32b = (len+3)/4;
	volatile unsigned int *fifo=core->regs.dfifo[0];

	for(i=0;i<count32b;i++,dst+=4) {
		*(unsigned int *)dst=READ_REG32(fifo);
	}
	return ((void *)dst);
}


static int usb_enable_irq(struct usb_core_data *core) {
	unsigned int ahbcfg;
	ahbcfg=AHBCFG_GLBL_IRQ_MASK;
	MOD_REG32(&core->regs.gregs->g_ahbcfg,0,ahbcfg);
	return 0;
}

static int usb_disable_irq(struct usb_core_data *core) {
	unsigned int ahbcfg;
	ahbcfg=AHBCFG_GLBL_IRQ_MASK;
	MOD_REG32(&core->regs.gregs->g_ahbcfg,ahbcfg,0);
	return 0;
}

static int usb_setmode(struct usb_core_data *core, unsigned int mode) {
	unsigned int usbcfg;
	
	usbcfg=READ_REG32(&core->regs.gregs->g_usbcfg);

	usbcfg&=~(USBCFG_FORCE_HOST|USBCFG_FORCE_DEV);

	if (mode==HOST_MODE) {
		usbcfg|=USBCFG_FORCE_HOST;
	} else if (mode==DEVICE_MODE) {
		usbcfg|=USBCFG_FORCE_DEV;
	} 

	WRITE_REG32(&core->regs.gregs->g_usbcfg, usbcfg);
	usb_mdelay(50);
	return 0;
}

static int usb_core_inDevMode(struct usb_core_data *core) {
	unsigned int mode = READ_REG32(&core->regs.gregs->g_intsts) & 0x1;
	return (mode!=HOST_MODE);
}

static void usb_ep0_out_start(struct usb_core_data *core) {
	unsigned int doeptsize0;
	doeptsize0=(3<<DEP0XFERSIZE_SUPCNT_SHIFT) |
		   (1<<DEP0XFERSIZE_PKTCNT_SHIFT) |
		   ((8*3)<<DEP0XFERSIZE_XFERSIZE_SHIFT);
	WRITE_REG32(&core->regs.out_epregs[0]->d_oeptsiz, doeptsize0);

	if (core->cfg.dma_enable) {
		unsigned int doepctl;
		WRITE_REG32(&core->regs.out_epregs[0]->d_oepctl,
			(unsigned int)&core->dev.setup_packet);

		doepctl=READ_REG32(&core->regs.out_epregs[0]->d_oepctl);
		doepctl|=DEPCTL_EPENA;
		WRITE_REG32(&core->regs.out_epregs[0]->d_oepctl, doepctl);
	}
}

static int usb_otg_ep0_activate(struct usb_core_data *core) {
	unsigned int dsts;
	unsigned int diepctl;
	unsigned int dctl;

	dsts=READ_REG32(&core->regs.dregs->d_sts);
	diepctl=READ_REG32(&core->regs.in_epregs[0]->d_iepctl);
	
	switch((dsts&STS_ENUMSPD_MASK)>>STS_ENUMSPD_SHIFT) {
		case DSTS_ENUMSPD_HS_PHY_30MHZ_OR_60MHZ:
		case DSTS_ENUMSPD_FS_PHY_30MHZ_OR_60MHZ:
		case DSTS_ENUMSPD_FS_PHY_48MHZ:
			diepctl&=~DEPCTL_MPS_MASK;	
			diepctl|=DEP0CTL_MPS_64;
			break;
		case DSTS_ENUMSPD_LS_PHY_6MHZ:
			diepctl&=~DEPCTL_MPS_MASK;	
			diepctl|=DEP0CTL_MPS_8;
			break;
	}

	WRITE_REG32(&core->regs.in_epregs[0]->d_iepctl,diepctl);
	dctl=DEVCTL_CGNPINNAK;
	MOD_REG32(&core->regs.dregs->d_ctl,dctl,dctl);

	return 0;
}


static int usb_flush_tx_fifo(struct usb_core_data *core, unsigned int num) {
	volatile unsigned int grstctl;
	unsigned int count=0;
	grstctl=(RSTCTL_TXFFLSH|
		((num<<RSTCTL_TXFNUM_SHIFT)&RSTCTL_TXFNUM_MASK));
	WRITE_REG32(&core->regs.gregs->g_rstctl,grstctl);
	do {
		grstctl=READ_REG32(&core->regs.gregs->g_rstctl);
		if (++count>200000) {
			break;
		}
	} while (grstctl&RSTCTL_TXFFLSH);
	usb_udelay(3);
	return 0;
}

static int usb_flush_rx_fifo(struct usb_core_data *core) {
	volatile unsigned int grstctl;
	unsigned int count=0;
	grstctl=RSTCTL_RXFFLSH;
	WRITE_REG32(&core->regs.gregs->g_rstctl,grstctl);
	do {
		grstctl=READ_REG32(&core->regs.gregs->g_rstctl);
		if (++count>200000) {
			break;
		}
	} while (grstctl&RSTCTL_RXFFLSH);
	usb_udelay(3);
	return 0;
}



/******************************************************************************************
 * DataOutStage
 * DataInStage
 * SetupStage
 * SOF
 * Reset
 * Suspend
 * Resume
 * IsoINIncomplete
 * IsoOUTIncomplete
 * DevConnected
 * DevDisconnected
 *
 ******************************************************************************************/

static int dcd_int_data_out_stage(struct usb_core_data *core, unsigned int epnum) {
	struct usb_otg_ep *ep;
	
	if (epnum==0) {
		ep=&core->dev.out_ep[0];
		if (core->dev.dev_state==USB_OTG_EP0_DATA_OUT) {
			if (ep->rem_data_len > ep->max_packet) {
				ep->rem_data_len-=ep->max_packet;
				if (core->cfg.dma_enable==1) {
					ep->xfer_buf += ep->max_packet;
				}
				usbd_ctl_cont_rx(core,ep->xfer_buf, MIN(ep->rem_data_len,ep->max_packet));
			} else {
				if ((core->dev.class_cb->EP0_rx_ready)&&
					(core->dev.dev_status==USB_OTG_CONFIGURED)) {
					core->dev.class_cb->EP0_rx_ready(core);
				}
				usbd_ctl_send_status(core);
			}
		}
	} else if (core->dev.class_cb->data_out&&
			core->dev.dev_status==USB_OTG_CONFIGURED) {
		core->dev.class_cb->data_out(core,epnum);
	}	
	return 0;
}

static int dcd_int_data_in_stage(struct usb_core_data *core, unsigned int epnum) {
	struct usb_otg_ep *ep;
	
	if(!epnum) {
		ep=&core->dev.in_ep[0];
		if (core->dev.dev_state==USB_OTG_EP0_DATA_IN) {
			if (ep->rem_data_len>ep->max_packet) {
				ep->rem_data_len-=ep->max_packet;
				if (core->cfg.dma_enable) {
					ep->xfer_buf+=ep->max_packet;
				}
				usbd_ctl_continue_send_data(core,ep->xfer_buf,ep->rem_data_len);
			} else {
				if(((ep->total_data_len%ep->max_packet)==0) &&
				   (ep->total_data_len>=ep->max_packet) &&
				   (ep->total_data_len<ep->ctl_data_len)) {
					usbd_ctl_continue_send_data(core,0,0);
				} else {
					if (core->dev.class_cb->EP0_tx_sent &&
						(core->dev.dev_status==USB_OTG_CONFIGURED)) {
						core->dev.class_cb->EP0_tx_sent(core);
					}
					usbd_ctl_receive_status(core);
				}
			}
		}
	} else if(core->dev.class_cb->data_in&&
			(core->dev.dev_status==USB_OTG_CONFIGURED)) {
		core->dev.class_cb->data_in(core,epnum);
	}
	return 0;
}


static int dcd_int_setup_stage(struct usb_core_data *core) {
	struct usb_setup_req req;
	usbd_parse_setup_req(core,&req);
	switch(req.bmRequest&0x1f) {
		case USB_REQ_RECIPIENT_DEVICE:
			usbd_std_dev_req(core,&req);
			break;
		case USB_REQ_RECIPIENT_INTERFACE:
			usbd_std_itf_req(core,&req);
			break;
		case USB_REQ_RECIPIENT_ENDPOINT:
			usbd_std_ep_req(core,&req);
			break;
		default:
			dcd_ep_stall(core,req.bmRequest&0x80);
			break;
	}
	return 0;
}

static int dcd_int_sof(struct usb_core_data *core) {
	if (core->dev.class_cb->sof) {
		core->dev.class_cb->sof(core);
	}
	return 0;
}

static int dcd_int_reset(struct usb_core_data *core) {
	dcd_ep_open(core,0,USB_OTG_MAX_EP0_SIZE,EP_TYPE_CTRL);
	dcd_ep_open(core,0x80,USB_OTG_MAX_EP0_SIZE,EP_TYPE_CTRL);

	core->dev.dev_status=USB_OTG_DEFAULT;
	core->dev.usr_cb->dev_reset(core->cfg.speed);
	return 0;
}

static int dcd_int_suspend(struct usb_core_data *core) {
	core->dev.dev_status=USB_OTG_SUSPENDED;
	core->dev.usr_cb->dev_suspended();
	return 0;
}

static int dcd_int_resume(struct usb_core_data *core) {
	core->dev.usr_cb->dev_resumed();
	core->dev.dev_status=USB_OTG_CONFIGURED;
	return 0;
}

static int dcd_int_iso_in_incomplete(struct usb_core_data *core) {
	core->dev.class_cb->iso_in_incomplete(core);
	return 0;
}

static int dcd_int_iso_out_incomplete(struct usb_core_data *core) {
	core->dev.class_cb->iso_out_incomplete(core);
	return 0;
}

static int dcd_int_dev_connected(struct usb_core_data *core) {
	core->dev.usr_cb->dev_connected();
	return 0;
}

static int dcd_int_dev_disconnected(struct usb_core_data *core) {
	core->dev.usr_cb->dev_disconnected();
	core->dev.class_cb->deinit(core,0);
	return 0;
}

/**********************************************************************************
 *   Irq handlers
 **********************************************************************************/

static void handle_usb_reset(struct usb_core_data *core) {
	unsigned int daintmsk;
	unsigned int doepmsk;
	unsigned int diepmsk;
	unsigned int dcfg;
	unsigned int dctl;
	unsigned int gintsts;
	unsigned int i;

	dctl=DEVCTL_RMTWKUPSIG;
	MOD_REG32(&core->regs.dregs->d_ctl, dctl,0);

	usb_flush_tx_fifo(core,0);
	
	for(i=0;i<core->cfg.dev_endpts;i++) {
		WRITE_REG32(&core->regs.in_epregs[i]->d_in_ep_irq, 0xff);
		WRITE_REG32(&core->regs.out_epregs[i]->d_out_ep_irq, 0xff);
	}

	WRITE_REG32(&core->regs.dregs->d_all_irq, 0xffffffff);

	daintmsk=((1<<DALLIRQ_IN_SHIFT) |
		 (1<<DALLIRQ_OUT_SHIFT));
	WRITE_REG32(&core->regs.dregs->d_all_irq_msk,daintmsk);
	
	doepmsk=DOEPINT_SETUP|DOEPINT_XFERCOMPL|DOEPINT_AHBERR|DOEPINT_EPDISABLED;
	WRITE_REG32(&core->regs.dregs->d_out_ep_msk, doepmsk);

	diepmsk=DIEPINT_XFERCOMPL|DIEPINT_TIMEOUT|DIEPINT_EPDISABLED|DIEPINT_AHBERR|DIEPINT_INTKNEPMIS;
	WRITE_REG32(&core->regs.dregs->d_in_ep_msk,diepmsk);

	dcfg=READ_REG32(&core->regs.dregs->d_cfg);
	dcfg&=~DEVCFG_ADDR_MASK;
	WRITE_REG32(&core->regs.dregs->d_cfg,dcfg);

	usb_ep0_out_start(core);

	gintsts=INTMSK_USBRESET;
	WRITE_REG32(&core->regs.gregs->g_intsts,gintsts);	


	dcd_int_reset(core);
#if 0
	usbd_dcd_int_fops->reset(core);
#endif

}

static void handle_session_req(struct usb_core_data *core) {
	unsigned int gintsts;

	dcd_int_dev_connected(core);
#if 0
	usbd_dcd_int_fops->dev_connected(core);
#endif
	gintsts=INTMSK_SESSREQINTR;
	WRITE_REG32(&core->regs.gregs->g_intsts,gintsts);
}

static void handle_otg(struct usb_core_data *core) {
	unsigned int gotgint;

	gotgint=READ_REG32(&core->regs.gregs->g_otgint);
	if (gotgint&OTGINT_SESENDDET) {
		dcd_int_dev_disconnected(core);
#if 0
	usbd_dcd_int_fops->dev_disconnected(core);
#endif
	}
	WRITE_REG32(&core->regs.gregs->g_otgint,gotgint);
}

static int handle_usb_resume(struct usb_core_data *core) {
	unsigned int gintsts;
	unsigned int devctl;
	unsigned int pcgcctl;
	
	if (core->cfg.low_power) {
		pcgcctl=READ_REG32(core->regs.pcgcctl);
		pcgcctl&=~(PCGCCTL_STOPPCLK|PCGCCTL_GATEHCLK);
		WRITE_REG32(core->regs.pcgcctl,pcgcctl);
	}

	devctl=DEVCTL_RMTWKUPSIG;
	MOD_REG32(&core->regs.dregs->d_ctl,devctl,0);

	dcd_int_resume(core);
#if 0
	usbd_dcd_int_fops->resume(core);
#endif

	gintsts=INTMSK_WKUPINTR;
	WRITE_REG32(&core->regs.gregs->g_intsts,gintsts);
	return 1;
}

static void handle_usb_suspend(struct usb_core_data *core) {
	unsigned int gintsts;
	unsigned int pcgcctl;
	unsigned int dsts;

	dcd_int_suspend(core);
#if 0
	usbd_dcd_int_fops->suspend(core);
#endif

	dsts=READ_REG32(&core->regs.dregs->d_sts);

	gintsts=INTMSK_USBSUSPEND;
	WRITE_REG32(&core->regs.gregs->g_intsts,gintsts);
	
	if (core->cfg.low_power&&(dsts&STS_SUSPSTS)) {
		pcgcctl=PCGCCTL_STOPPCLK;
		MOD_REG32(core->regs.pcgcctl,0,pcgcctl);

		pcgcctl|=PCGCCTL_GATEHCLK;
		MOD_REG32(core->regs.pcgcctl,0,pcgcctl);
#if 0
		PUT CPU TO SLEEP
#endif
	}
}

static void handle_rx_status_queue_level(struct usb_core_data *core) {
	unsigned int gintmsk;
	unsigned int drxsts;
	struct usb_otg_ep *ep;

	gintmsk=INTMSK_RXSTSQLVL;
	MOD_REG32(&core->regs.gregs->g_intmsk,gintmsk,0);

	drxsts=READ_REG32(&core->regs.gregs->g_rxstsp);

	ep=&core->dev.out_ep[(drxsts&RXSTS_EPNUM_MASK)>>RXSTS_EPNUM_SHIFT];

	switch((drxsts&RXSTS_PKTSTS_MASK)>>RXSTS_PKTSTS_SHIFT) {
		case	STS_GOUT_NAK:
			break;
		case	STS_DATA_UPDT:
			if (drxsts&RXSTS_BCNT_MASK) {
				unsigned int bcnt=((drxsts&RXSTS_BCNT_MASK)>>RXSTS_BCNT_SHIFT);
				usb_otg_read_packet(core,ep->xfer_buf,bcnt);
				ep->xfer_buf+=bcnt;
				ep->xfer_count+=bcnt;
			}
			break;
		case	STS_XFER_COMP:
			break;
		case	STS_SETUP_COMP:
			break;
		case	STS_SETUP_UPDT: {
			unsigned int bcnt=((drxsts&RXSTS_BCNT_MASK)>>RXSTS_BCNT_SHIFT);
			usb_otg_read_packet(core,core->dev.setup_packet,8);
			ep->xfer_count += bcnt;
			break;
		}
		default:
			break;
	}
	MOD_REG32(&core->regs.gregs->g_intmsk,0,gintmsk);
}

static void handle_enumdone(struct usb_core_data *core) {
	unsigned int gintsts;
	unsigned int gusbcfg;

	usb_otg_ep0_activate(core);

	gusbcfg=READ_REG32(&core->regs.gregs->g_usbcfg);

	if (usb_get_dev_speed(core)==USB_SPEED_HIGH) {
		core->cfg.speed=USB_OTG_SPEED_HIGH;
		core->cfg.mps=USB_OTG_HS_MAX_PACKET_SIZE;
		gusbcfg&=~USBCFG_USBTRDTIM_MASK;
		gusbcfg|=(9<<USBCFG_USBTRDTIM_SHIFT);
	} else {
		core->cfg.speed=USB_OTG_SPEED_FULL;
		core->cfg.mps=USB_OTG_FS_MAX_PACKET_SIZE;
		gusbcfg&=~USBCFG_USBTRDTIM_MASK;
		gusbcfg|=(5<<USBCFG_USBTRDTIM_SHIFT);
	}

	WRITE_REG32(&core->regs.gregs->g_usbcfg,gusbcfg);

	gintsts=INTMSK_ENUMDONE;
	WRITE_REG32(&core->regs.gregs->g_intsts,gintsts);
}

static unsigned int usb_read_all_out_ep_irq(struct usb_core_data *core) {
	unsigned int v;
	v=READ_REG32(&core->regs.dregs->d_all_irq);
	v&=READ_REG32(&core->regs.dregs->d_all_irq_msk);
	return ((v>>16)&0xffff);
}

static unsigned int usb_read_out_ep_irq(struct usb_core_data *core, unsigned char epnum) {
	unsigned int v;
	v=READ_REG32(&core->regs.out_epregs[epnum]->d_out_ep_irq);
	v&=READ_REG32(&core->regs.dregs->d_out_ep_msk);
	return v;
}

static void clear_out_ep_irq(struct usb_core_data *core, unsigned int epnum, unsigned int irq_flag) {
	WRITE_REG32(&core->regs.out_epregs[epnum]->d_out_ep_irq,irq_flag);
}

static void usb_handle_out_ep_irq(struct usb_core_data *core) {
	unsigned int ep_intr;
	unsigned int doepint;
	unsigned int deptsiz;
	unsigned int epnum=0;

	ep_intr=usb_read_all_out_ep_irq(core);

	while(ep_intr) {
		if(ep_intr&0x1) {
			doepint=usb_read_out_ep_irq(core,epnum);
			if (doepint&DOEPINT_XFERCOMPL) {
				clear_out_ep_irq(core,epnum,DOEPINT_XFERCOMPL);
				if (core->cfg.dma_enable) {
					deptsiz=READ_REG32(&core->regs.out_epregs[epnum]->d_oeptsiz);
					core->dev.out_ep[epnum].xfer_count=
						core->dev.out_ep[epnum].max_packet -
							((deptsiz&DEPXFERSIZE_XFERSIZE_MASK)>>
							   DEPXFERSIZE_XFERSIZE_SHIFT);
				}

				dcd_int_data_out_stage(core,epnum);
#if 0
				usbd_dcd_int_fops->data_out_stage(core,epnun);
#endif

				if (core->cfg.dma_enable) {
					if ((!epnum)&&(core->dev.dev_state==USB_OTG_EP0_STATUS_OUT)) {
						usb_ep0_out_start(core);
					}
				}
			}

			if (doepint&DOEPINT_EPDISABLED) {
				clear_out_ep_irq(core,epnum, DOEPINT_EPDISABLED);
			}

			if (doepint&DOEPINT_AHBERR) {
				clear_out_ep_irq(core,epnum, DOEPINT_AHBERR);
			}
			
			if (doepint&DOEPINT_SETUP) {
				dcd_int_setup_stage(core);
#if 0
			usbd_dcd_int_fops->setup_stage(core);
#endif
				clear_out_ep_irq(core, epnum, DOEPINT_SETUP);
			}
		}
		epnum++;
		ep_intr>>=1;
	}
}

static unsigned int usb_read_all_in_ep_irq(struct usb_core_data *core) {
	unsigned int v;
	v=READ_REG32(&core->regs.dregs->d_all_irq);
	v&=READ_REG32(&core->regs.dregs->d_all_irq_msk);
	return (v&0xffff);
}

static unsigned int usb_read_in_ep_irq(struct usb_core_data *core, unsigned char epnum) {
	unsigned int v,msk,empty;
	msk=READ_REG32(&core->regs.dregs->d_in_ep_msk);
	empty=READ_REG32(&core->regs.dregs->d_in_ep_empty_msk);
	v=READ_REG32(&core->regs.in_epregs[epnum]->d_in_ep_irq);
	io_printf("usb_read_in_ep_irq: msk %x, empty %x, irq %x\n", msk, empty, v);
	msk |= ((empty>>epnum)&0x1)<<7;
	v=v&msk;
	return v;
}

static void clear_in_ep_irq(struct usb_core_data *core, unsigned int epnum, unsigned int irq_flag) {
	WRITE_REG32(&core->regs.in_epregs[epnum]->d_in_ep_irq,irq_flag);
}


static int dcd_write_empty_tx_fifo(struct usb_core_data *core, unsigned int epnum) {
	unsigned int txstatus=0;
	struct usb_otg_ep *ep;
	unsigned int len=0;
	unsigned int len32b;

	ep=&core->dev.in_ep[epnum];
	len=ep->xfer_len-ep->xfer_count;

	if (len>ep->max_packet) {
		len=ep->max_packet;
	}

	len32b=(len+3)/4;
	txstatus=READ_REG32(&core->regs.in_epregs[epnum]->d_txfsts);

	io_printf("txstatus=%x, epnum=%x\n",txstatus,epnum);

	while((txstatus&DTXFSTS_TXFSPCAVAIL_MASK)>len32b &&
		ep->xfer_count < ep->xfer_len &&
		ep->xfer_len!=0) {
		len=ep->xfer_len-ep->xfer_count;
		if (len>ep->max_packet) {
			len=ep->max_packet;
		}
		len32b=(len+3)/4;

		usb_otg_write_packet(core,ep->xfer_buf,epnum,len);
	
		ep->xfer_buf+=len;
		ep->xfer_count+=len;

		txstatus=READ_REG32(&core->regs.in_epregs[epnum]->d_txfsts);
	}
	return 1;
}


static unsigned int usb_handle_in_ep_irq(struct usb_core_data *core) {
	unsigned int diepint=0;
	unsigned int ep_intr;
	unsigned int epnum=0;
	unsigned int fifoemptymsk;

	ep_intr=usb_read_all_in_ep_irq(core);
	io_printf("in_ep_irq: ep_intr=%x\n",ep_intr);

	while(ep_intr) {
		if (ep_intr&0x1) {
			diepint=usb_read_in_ep_irq(core,epnum);
			io_printf("diepint: %x for epnum %d\n", diepint, epnum);
			if (diepint&DIEPINT_XFERCOMPL) {
				fifoemptymsk=1<<epnum;
				MOD_REG32(&core->regs.dregs->d_in_ep_empty_msk,fifoemptymsk,0);
				clear_in_ep_irq(core,epnum,DIEPINT_XFERCOMPL);

				dcd_int_data_in_stage(core,epnum);
#if 0
			USBD_DCD_INT_fops->DataInStage(core,epnum);
#endif
				if (core->cfg.dma_enable) {
					if ((!epnum)&&(core->dev.dev_state==USB_OTG_EP0_STATUS_IN)) {
						usb_ep0_out_start(core);
					}
				}
			}
			if (diepint&DIEPINT_AHBERR) {
				clear_in_ep_irq(core,epnum,DIEPINT_AHBERR);
			}
			if (diepint&DIEPINT_TIMEOUT) {
				clear_in_ep_irq(core,epnum,DIEPINT_TIMEOUT);
			}
			if (diepint&DIEPINT_INTKTXFEMP) {
				clear_in_ep_irq(core,epnum,DIEPINT_INTKTXFEMP);
			}
			if (diepint&DIEPINT_INTKNEPMIS) {
				clear_in_ep_irq(core,epnum,DIEPINT_INTKNEPMIS);
			}
			if (diepint&DIEPINT_INEPNAKEFF) {
				clear_in_ep_irq(core,epnum,DIEPINT_INEPNAKEFF);
			}
			if (diepint&DIEPINT_EPDISABLED) {
				clear_in_ep_irq(core,epnum,DIEPINT_EPDISABLED);
			}
			if (diepint&DIEPINT_EMPTYINTR) {
				dcd_write_empty_tx_fifo(core,epnum);
				clear_in_ep_irq(core,epnum,DIEPINT_EMPTYINTR);
			}
		}
		epnum++;
		ep_intr >>= 1;
	}
	return 1;
}


static void handle_sof_irq(struct usb_core_data *core) {
	unsigned int gintsts;
	
	dcd_int_sof(core);
#if 0
	usbd_dcd_int_fops->sof(core);
#endif
	gintsts=INTMSK_SOFINTR;
	WRITE_REG32(&core->regs.gregs->g_intsts,gintsts);
}

void OTG_FS_IRQHandler(void) {
	struct usb_core_data *core=&usb_core_data;
	
	io_setpolled(1);
	if (usb_core_inDevMode(core)) {
		unsigned int v=READ_REG32(&core->regs.gregs->g_intsts);
		v &=READ_REG32(&core->regs.gregs->g_intmsk);
		if (!v) {
			io_setpolled(0);
			return;
		}
		
		if (v&INTMSK_OUTEPINTR) {
			io_printf("USB out ep irq\n");
			usb_handle_out_ep_irq(core);
		}

		if (v&INTMSK_INEPINTR) {
			io_printf("USB in ep irq\n");
			usb_handle_in_ep_irq(core);
		}

		if (v&INTMSK_MODEMISMATCH) {
			unsigned int r=INTMSK_MODEMISMATCH;
			io_printf("USB mode missmatch irq\n");
			WRITE_REG32(&core->regs.gregs->g_intsts,r);
		}
		
		if (v&INTMSK_WKUPINTR) {
			io_printf("USB wakeup irq\n");
			handle_usb_resume(core);
		}

		if (v&INTMSK_USBSUSPEND) {
			io_printf("USB suspend irq\n");
			handle_usb_suspend(core);
			
		}
	
		if (v&INTMSK_RXSTSQLVL) {
			io_printf("USB RXSTSQLVL irq\n");
			handle_rx_status_queue_level(core);
		}

		if (v&INTMSK_USBRESET) {
			io_printf("USB reset irq\n");
			handle_usb_reset(core);
		}

		if (v&INTMSK_ENUMDONE) {
			io_printf("USB enum done irq\n");
			handle_enumdone(core);
		}

		if (v&INTMSK_INCOMPLISOIN) {
			io_printf("USB IsoInIncomplete irq\n");
//			handle_iso_in_incomplete(core);
		}

		if (v&INTMSK_INCOMPLISOOUT) {
			io_printf("USB IsoOutIncomplete irq\n");
		}

		if (v&INTMSK_SESSREQINTR) {
			io_printf("USB session request irq\n");
			handle_session_req(core);
		}	
		
		if (v&INTMSK_OTGINTR) {
			io_printf("USB otg  irq\n");
			handle_otg(core);
		}
		if (v&INTMSK_SOFINTR) {
//			io_printf("USB SOF irq\n");
			handle_sof_irq(core);
		}
	}
	io_setpolled(0);
}

/*************************************************************************
 * ***********************************************************************/


static int usb_otg_ep_set_stall(struct usb_core_data *core, struct usb_otg_ep *ep) {
	unsigned int depctl;
	volatile unsigned int *depctl_addr;

	if (ep->is_in==1) {
		depctl_addr=&(core->regs.in_epregs[ep->num]->d_iepctl);
		depctl=READ_REG32(depctl_addr);
		if (depctl&DEPCTL_EPENA) {
			depctl|=DEPCTL_EPDIS;
		}
		depctl|=DEPCTL_STALL;
		WRITE_REG32(depctl_addr,depctl);
	} else {
		depctl_addr=&(core->regs.out_epregs[ep->num]->d_oepctl);
		depctl=READ_REG32(depctl_addr);
		depctl|=DEPCTL_STALL;
		WRITE_REG32(depctl_addr,depctl);
	}
	return 0;
}

static int usb_otg_ep_clr_stall(struct usb_core_data *core, struct usb_otg_ep *ep) {
	unsigned int depctl;
	volatile unsigned int *depctl_addr;

	if (ep->is_in) {
		depctl_addr=&core->regs.in_epregs[ep->num]->d_iepctl;
	} else {
		depctl_addr=&core->regs.out_epregs[ep->num]->d_oepctl;
	}

	depctl=READ_REG32(depctl_addr);
	depctl&=~DEPCTL_STALL;

	if (ep->type==EP_TYPE_INTR||ep->type==EP_TYPE_BULK) {
		depctl|=DEPCTL_SETD0PID;
	}
	WRITE_REG32(depctl_addr,depctl);
	return 0;
}

static int usb_otg_write_packet(struct usb_core_data *core, unsigned char *src,
				unsigned char ep_num, unsigned short int len) {
	if (!core->cfg.dma_enable) {
		unsigned int count32b;
		unsigned int i;
		volatile unsigned int *fifo;

		count32b=(len+3)/4;
		fifo=core->regs.dfifo[ep_num];
		io_printf("write_packet: ");
		for(i=0;i<count32b;i++,src+=4) {
			io_printf("%08x.", *((unsigned int *)src));
			WRITE_REG32(fifo,*((unsigned int *)src));
		}
		io_printf("\n");
	}
	return 0;
}

static int usb_otg_ep_activate(struct usb_core_data *core, struct usb_otg_ep *ep) {
	unsigned int depctl;
	unsigned int daintmsk;
	volatile unsigned int *addr;

	if(ep->is_in) {
		addr=&core->regs.in_epregs[ep->num]->d_iepctl;
		daintmsk=(1<<(ep->num+DALLIRQ_IN_SHIFT));
	} else {
		addr=&core->regs.out_epregs[ep->num]->d_oepctl;
		daintmsk=(1<<(ep->num+DALLIRQ_OUT_SHIFT));
	}

	depctl=READ_REG32(addr);
	if (!(depctl&DEPCTL_USBACTEP)) {
		depctl&=~(DEPCTL_MPS_MASK|DEPCTL_EPTYPE_MASK|DEPCTL_TXFNUM_MASK);
		depctl|=((ep->max_packet<<DEPCTL_MPS_SHIFT)|
			 (ep->type<<DEPCTL_EPTYPE_SHIFT)|
			 (ep->tx_fifo_num<<DEPCTL_TXFNUM_SHIFT)|
			 DEPCTL_SETD0PID|DEPCTL_USBACTEP);
		WRITE_REG32(addr,depctl);
	}
#if 0
	Some HS code
#endif

	MOD_REG32(&core->regs.dregs->d_all_irq_msk,0,daintmsk);
	return 0;
}


static int usb_otg_ep_start_xfer(struct usb_core_data *core, struct usb_otg_ep *ep) {
	unsigned int depctl=0;
	unsigned int deptsiz=0;
	unsigned int dsts;
	unsigned int fifoemptymsk=0;

	if (ep->is_in) {
		depctl = READ_REG32(&core->regs.in_epregs[ep->num]->d_iepctl);
		deptsiz = READ_REG32(&core->regs.in_epregs[ep->num]->d_ieptsiz);

		if (!ep->xfer_len) {
			deptsiz&=~(DEPXFERSIZE_XFERSIZE_MASK|DEPXFERSIZE_PKTCNT_MASK);
			deptsiz|=(1<<DEPXFERSIZE_PKTCNT_SHIFT);	
		} else {
			deptsiz&=~(DEPXFERSIZE_XFERSIZE_MASK|DEPXFERSIZE_PKTCNT_MASK);
			deptsiz|=((ep->xfer_len<<DEPXFERSIZE_XFERSIZE_SHIFT)|
				(((ep->xfer_len - 1 + ep->max_packet) / ep->max_packet)<<DEPXFERSIZE_PKTCNT_SHIFT));	
			if (ep->type==EP_TYPE_ISOC) {
				deptsiz|=(1<<DEPXFERSIZE_MC_SHIFT);
			}
		}
		io_printf("sending %x bytes and packets\n", deptsiz);
		WRITE_REG32(&core->regs.in_epregs[ep->num]->d_ieptsiz,deptsiz);

		if (core->cfg.dma_enable) {
			WRITE_REG32(&core->regs.in_epregs[ep->num]->d_iepdma,ep->dma_addr);
		} else {
			if (ep->type != EP_TYPE_ISOC) {
				if (ep->xfer_len) {
					fifoemptymsk=1<<ep->num;
					MOD_REG32(&core->regs.dregs->d_in_ep_empty_msk, 0, fifoemptymsk);
				}
			}
		}

		if (ep->type==EP_TYPE_ISOC) {
			dsts=READ_REG32(&core->regs.dregs->d_sts);
			if (!(((dsts&STS_SOFFN_MASK)>>STS_SOFFN_SHIFT)&0x1)) {
				depctl|=DEPCTL_SETD1PID;
			} else {
				depctl|=DEPCTL_SETD0PID;
			}
		}

		depctl|=(DEPCTL_CNAK|DEPCTL_EPENA);
		WRITE_REG32(&core->regs.in_epregs[ep->num]->d_iepctl,depctl);

		if (ep->type==EP_TYPE_ISOC) {
			usb_otg_write_packet(core,ep->xfer_buf,ep->num,ep->xfer_len);
		}
	} else {
		depctl=READ_REG32(&core->regs.out_epregs[ep->num]->d_oepctl);
		deptsiz=READ_REG32(&core->regs.out_epregs[ep->num]->d_oeptsiz);

		if (!ep->xfer_len) {
			deptsiz&=~(DEPXFERSIZE_XFERSIZE_MASK|DEPXFERSIZE_PKTCNT_MASK);
			deptsiz|=((ep->max_packet<<DEPXFERSIZE_XFERSIZE_SHIFT)|
				(1<<DEPXFERSIZE_PKTCNT_SHIFT));
		} else {
			unsigned int pktcnt=(ep->xfer_len + (ep->max_packet - 1)) / ep->max_packet;
			deptsiz&=~(DEPXFERSIZE_XFERSIZE_MASK|DEPXFERSIZE_PKTCNT_MASK);
			deptsiz|=(((pktcnt*ep->max_packet)<<DEPXFERSIZE_XFERSIZE_SHIFT)|
				(pktcnt<<DEPXFERSIZE_PKTCNT_SHIFT));	
		}

		WRITE_REG32(&core->regs.out_epregs[ep->num]->d_oeptsiz,deptsiz);

		if (core->cfg.dma_enable) {
			WRITE_REG32(&core->regs.out_epregs[ep->num]->d_oepdma,ep->dma_addr);
		}
		
		if (ep->type==EP_TYPE_ISOC) {
			if (ep->even_odd_frame) {
				depctl|=DEPCTL_SETD1PID;
			} else {
				depctl|=DEPCTL_SETD0PID;
			}
		}

		depctl|=(DEPCTL_EPENA|DEPCTL_CNAK);
		WRITE_REG32(&core->regs.out_epregs[ep->num]->d_oepctl,depctl);
	}
	return 0;
}


static int usb_otg_ep0_start_xfer(struct usb_core_data *core, struct usb_otg_ep *ep) {
	unsigned int depctl=0;
	unsigned int deptsiz=0;
	struct  usb_core_in_epregs	*in_regs;
	unsigned int fifoemptymsk=0;

	if (ep->is_in) {
		in_regs=core->regs.in_epregs[0];
		depctl = READ_REG32(&in_regs->d_iepctl);
		deptsiz = READ_REG32(&in_regs->d_ieptsiz);

		if (!ep->xfer_len) {
			deptsiz&=~(DEP0XFERSIZE_XFERSIZE_MASK|DEP0XFERSIZE_PKTCNT_MASK);
			deptsiz|=(1<<DEP0XFERSIZE_PKTCNT_SHIFT);	
		} else {
			deptsiz&=~(DEP0XFERSIZE_XFERSIZE_MASK|DEP0XFERSIZE_PKTCNT_MASK);
			if (ep->xfer_len>ep->max_packet) {
				ep->xfer_len=ep->max_packet;
				deptsiz|=(ep->max_packet<<DEP0XFERSIZE_XFERSIZE_SHIFT);
			} else {
				deptsiz|=(ep->xfer_len<<DEP0XFERSIZE_XFERSIZE_SHIFT);
			}
			deptsiz|=(1<<DEP0XFERSIZE_PKTCNT_SHIFT);
		}
		WRITE_REG32(&in_regs->d_ieptsiz,deptsiz);

		if (core->cfg.dma_enable) {
			WRITE_REG32(&core->regs.in_epregs[ep->num]->d_iepdma,ep->dma_addr);
		}

		depctl|=(DEPCTL_CNAK|DEPCTL_EPENA);
		WRITE_REG32(&in_regs->d_iepctl,depctl);

		if (!core->cfg.dma_enable) {
			if (ep->xfer_len) {
				fifoemptymsk|=1<<ep->num;
				MOD_REG32(&core->regs.dregs->d_in_ep_empty_msk,0,fifoemptymsk);
			}
		}
	} else {
		depctl=READ_REG32(&core->regs.out_epregs[ep->num]->d_oepctl);
		deptsiz=READ_REG32(&core->regs.out_epregs[ep->num]->d_oeptsiz);

		if (!ep->xfer_len) {
			deptsiz&=~(DEP0XFERSIZE_XFERSIZE_MASK|DEP0XFERSIZE_PKTCNT_MASK);
			deptsiz|=((ep->max_packet<<DEP0XFERSIZE_XFERSIZE_SHIFT)|
				(1<<DEP0XFERSIZE_PKTCNT_SHIFT));
		} else {
			ep->xfer_len=ep->max_packet;
			deptsiz&=~(DEP0XFERSIZE_XFERSIZE_MASK|DEP0XFERSIZE_PKTCNT_MASK);
			deptsiz|=((ep->max_packet<<DEP0XFERSIZE_XFERSIZE_SHIFT)|
				(1<<DEP0XFERSIZE_PKTCNT_SHIFT));
		}
		WRITE_REG32(&core->regs.out_epregs[ep->num]->d_oeptsiz, deptsiz);

		if (core->cfg.dma_enable) {
			WRITE_REG32(&core->regs.out_epregs[ep->num]->d_oepdma,ep->dma_addr);
		}
		depctl|=(DEPCTL_CNAK|DEPCTL_EPENA);
		WRITE_REG32(&core->regs.out_epregs[ep->num]->d_oepctl,depctl);
	}
	return 0;
}

		
static unsigned int usb_core_reset(struct usb_core_data *core) {
	volatile unsigned int grstctl=0;
	unsigned int count=0;

	do {
		usb_udelay(3);
		grstctl=READ_REG32(&core->regs.gregs->g_rstctl);
		if (++count > 200000) {
			return 0;
		}
	} while (!(grstctl&RSTCTL_AHBIDLE));

	count=0;
	grstctl|=RSTCTL_CSFTRST;
	WRITE_REG32(&core->regs.gregs->g_rstctl,grstctl);
	do {
		grstctl=READ_REG32(&core->regs.gregs->g_rstctl);
		if (++count>200000) {
			break;
		}
	} while (grstctl&RSTCTL_CSFTRST);
	usb_udelay(3);
	return 0;
}

static int usb_core_init(struct usb_core_data *core) {
	unsigned int usbcfg;
	unsigned int gccfg;
	unsigned int i2cctl;
	unsigned int ahbcfg;

	usbcfg=0;
	gccfg=0;
	ahbcfg=0;

	if (core->cfg.phy==USB_OTG_ULPI_PHY) {
		gccfg=READ_REG32(&core->regs.gregs->g_ccfg);
		gccfg&=~CCFG_PWDN;
		
		if (core->cfg.sof_out) {
			gccfg|=CCFG_SOFOUTEN;
		}
		WRITE_REG32(&core->regs.gregs->g_ccfg,gccfg);

		usbcfg=0;
		usbcfg=READ_REG32(&core->regs.gregs->g_usbcfg);

		usbcfg	&= ~USBCFG_TERM_SEL_DL_PULSE;
		usbcfg |= USBCFG_ULPI_ULMI_SEL;
		usbcfg &= ~(USBCFG_PHYIF|USBCFG_DDRSEL|USBCFG_FSINTF|USBCFG_ULPI_CLK_SUS_M);

		WRITE_REG32(&core->regs.gregs->g_usbcfg,usbcfg);

		usb_core_reset(core);

		if (core->cfg.dma_enable==1) {
			ahbcfg |= ((5<<AHBCFG_HBURSTLEN_SHIFT)&AHBCFG_HBURSTLEN_MASK);
			ahbcfg |= AHBCFG_DMA_ENABLE;
			WRITE_REG32(&core->regs.gregs->g_ahbcfg, ahbcfg);
		}
	} else {
		usbcfg = READ_REG32(&core->regs.gregs->g_usbcfg);
		usbcfg |= USBCFG_PHYSEL;
		WRITE_REG32(&core->regs.gregs->g_usbcfg, usbcfg);
		usb_core_reset(core);
		gccfg=0;
		gccfg|=CCFG_PWDN;

		if (core->cfg.phy==USB_OTG_I2C_PHY) {
			gccfg|=CCFG_I2CIFEN;

		}
		gccfg|=(CCFG_VBUSSENSA|CCFG_VBUSSENSB);

		if (core->cfg.sof_out) {
			gccfg|=CCFG_SOFOUTEN;
		}
		WRITE_REG32(&core->regs.gregs->g_ccfg,gccfg);

		usb_mdelay(20);

		usbcfg=READ_REG32(&core->regs.gregs->g_usbcfg);
		if (core->cfg.phy==USB_OTG_I2C_PHY) {
			usbcfg|=USBCFG_OTGUTMIFSSEL;
		}
		WRITE_REG32(&core->regs.gregs->g_usbcfg,usbcfg);

		if (core->cfg.phy==USB_OTG_I2C_PHY) {
			i2cctl=READ_REG32(&core->regs.gregs->g_i2cctl);
			i2cctl|=((1<<I2CCTL_I2CDADDR_SHIFT)&I2CCTL_I2CDADDR_MASK);
			i2cctl&=~(I2CCTL_I2CEN);
			i2cctl|=I2CCTL_DAT_SE0;
			i2cctl|=((0x2d<<I2CCTL_ADDR_SHIFT)&I2CCTL_ADDR_MASK);
			WRITE_REG32(&core->regs.gregs->g_i2cctl,i2cctl);
			usb_mdelay(200);
		
			i2cctl|=I2CCTL_I2CEN;
			WRITE_REG32(&core->regs.gregs->g_i2cctl,i2cctl);
			usb_mdelay(200);
		}
	}
	if (core->cfg.dma_enable==1) {
		ahbcfg=READ_REG32(&core->regs.gregs->g_ahbcfg);
		ahbcfg&=~AHBCFG_HBURSTLEN_MASK;
		ahbcfg|=(5<AHBCFG_HBURSTLEN_SHIFT);
		ahbcfg|=AHBCFG_DMA_ENABLE;
		WRITE_REG32(&core->regs.gregs->g_ahbcfg,ahbcfg);
	}
	return 0;
}

static int usb_select_core(struct usb_core_data *core, unsigned int coreId) {
	unsigned int 		base=USB_OTG_FS_BASE_ADDR;
	unsigned int 		i;

	core->cfg.dma_enable	= 0;
	core->cfg.speed		= USB_OTG_SPEED_FULL;
	core->cfg.mps		= USB_OTG_FS_MAX_PACKET_SIZE;

	core->cfg.coreID	= coreId;
	core->cfg.host_channels	= 8;
	core->cfg.dev_endpts	= 4;
	core->cfg.total_fifo_size = 320;
	core->cfg.phy		= USB_OTG_EMBEDDED_PHY;

	core->regs.gregs = (struct usb_core_gregs *) (base+GREGS_OFFSET);
	core->regs.dregs = (struct usb_core_dregs *) (base+DREGS_OFFSET);
	
	for(i=0;i<core->cfg.dev_endpts;i++) {
		core->regs.in_epregs[i]=
			(struct usb_core_in_epregs *) (base +
				IN_EP_REGS_OFFSET +
				(i*EP_REGS_SIZE));
		core->regs.out_epregs[i]=
			(struct usb_core_out_epregs *) (base +
				OUT_EP_REGS_OFFSET +
				(i*EP_REGS_SIZE));
	}

	core->regs.hregs = (struct usb_core_hregs *) (base +
				HOST_GLOBAL_REGS_OFFSET);
	core->regs.hprt0 = (unsigned int *) (base +
				HOST_PORT_REGS_OFFSET);

	for(i=0;i<core->cfg.host_channels;i++) {
		core->regs.hcregs[i]=(struct usb_core_hcregs *)(base +
			HOST_CHAN_REGS_OFFSET +
			(i*HOST_CHAN_REGS_SIZE));
	}
	
	for(i=0;i<core->cfg.host_channels;i++) {
		core->regs.dfifo[i]=(unsigned int *)(base +
			DATA_FIFO_OFFSET +
			(i*DATA_FIFO_SIZE));
	}

	core->regs.pcgcctl=(unsigned int *)(base+PCGCCTL_OFFSET);
	return 0;

}

static void usb_init_dev_speed(struct usb_core_data *core, unsigned int speed) {
	unsigned int dcfg;
	dcfg=READ_REG32(&core->regs.dregs->d_cfg);
	dcfg&=~DEVCFG_SPEED_MASK;
	dcfg|=((speed<<DEVCFG_SPEED_SHIFT)&DEVCFG_SPEED_MASK);
	WRITE_REG32(&core->regs.dregs->d_cfg,dcfg);
}

static int usb_get_dev_speed(struct usb_core_data *core) {
	unsigned int dsts;
	unsigned int speed=0;

	dsts=READ_REG32(&core->regs.dregs->d_sts);

	switch((dsts&STS_ENUMSPD_MASK)>>STS_ENUMSPD_SHIFT) {
		case DSTS_ENUMSPD_HS_PHY_30MHZ_OR_60MHZ:
			speed=USB_SPEED_HIGH;
			break;
		case DSTS_ENUMSPD_FS_PHY_30MHZ_OR_60MHZ:
		case DSTS_ENUMSPD_FS_PHY_48MHZ:
			speed = USB_SPEED_FULL;
			break;
		case DSTS_ENUMSPD_LS_PHY_6MHZ:
			speed = USB_SPEED_LOW;
			break;
	}
	return speed;
}

static unsigned int usb_core_enable_common_irq(struct usb_core_data *core) {
	unsigned int intmask;
	
	WRITE_REG32(&core->regs.gregs->g_otgint, 0xffffffff);
	WRITE_REG32(&core->regs.gregs->g_intsts, 0xffffffff);

	intmask=INTMSK_WKUPINTR| INTMSK_USBSUSPEND |
		INTMSK_OTGINTR| INTMSK_SESSREQINTR |
		INTMSK_CONIDSTSCHNG;

	WRITE_REG32(&core->regs.gregs->g_intmsk, intmask);

	return 0;
}

static unsigned int usb_enable_dev_irq(struct usb_core_data *core) {
	unsigned int intmask;
	WRITE_REG32(&core->regs.gregs->g_intmsk,0);
	WRITE_REG32(&core->regs.gregs->g_intsts,0xffffffff);
	usb_core_enable_common_irq(core);
	
	intmask= INTMSK_USBSUSPEND | INTMSK_USBRESET |
		INTMSK_ENUMDONE | INTMSK_INEPINTR |
		INTMSK_OUTEPINTR | INTMSK_SOFINTR |
		INTMSK_INCOMPLISOIN | INTMSK_INCOMPLISOOUT;

	if (!core->cfg.dma_enable) {
		intmask|=INTMSK_RXSTSQLVL;
	}
	
	MOD_REG32(&core->regs.gregs->g_intmsk, intmask, intmask);
	return 0;
}

static int usb_core_init_dev(struct usb_core_data *core) {
	unsigned int i;
	unsigned int depctl=0;
	unsigned int dcfg;
	unsigned int nptx_fifo_size=0;
	unsigned int tx_fifo_size=0;
	unsigned int mask=0;
	unsigned int dthrctl;

	WRITE_REG32(core->regs.pcgcctl,0);
	dcfg=READ_REG32(&core->regs.dregs->d_cfg);
	dcfg&=~DEVCFG_PERFRINT_MASK;
	dcfg|=(DCFG_FRAME_INTERVAL_80<<DEVCFG_PERFRINT_SHIFT);
	WRITE_REG32(&core->regs.dregs->d_cfg, dcfg);

	if (core->cfg.coreID==USB_OTG_FS_CORE_ID) {
		unsigned int start=0;
		unsigned int depth=0;
		usb_init_dev_speed(core,USB_OTG_SPEED_PARAM_FULL);

		WRITE_REG32(&core->regs.gregs->g_rxfsize, RX_FIFO_FS_SIZE);

		start=RX_FIFO_FS_SIZE;
		depth=TX0_FIFO_FS_SIZE;
		nptx_fifo_size=
		((depth<<FSIZE_DEPTH_SHIFT)&FSIZE_DEPTH_MASK)|
		((start<<FSIZE_START_SHIFT)&FSIZE_START_MASK);
		WRITE_REG32(&core->regs.gregs->g_dieptxfo_hnptxfsize,nptx_fifo_size);

		start+=depth;
		depth=TX1_FIFO_FS_SIZE;
		tx_fifo_size=
		((depth<<FSIZE_DEPTH_SHIFT)&FSIZE_DEPTH_MASK)|
		((start<<FSIZE_START_SHIFT)&FSIZE_START_MASK);
		WRITE_REG32(&core->regs.gregs->g_diep_txf[0],tx_fifo_size);

		start+=depth;
		depth=TX2_FIFO_FS_SIZE;
		tx_fifo_size=
		((depth<<FSIZE_DEPTH_SHIFT)&FSIZE_DEPTH_MASK)|
		((start<<FSIZE_START_SHIFT)&FSIZE_START_MASK);
		WRITE_REG32(&core->regs.gregs->g_diep_txf[1],tx_fifo_size);

		start+=depth;
		depth=TX3_FIFO_FS_SIZE;
		tx_fifo_size=
		((depth<<FSIZE_DEPTH_SHIFT)&FSIZE_DEPTH_MASK)|
		((start<<FSIZE_START_SHIFT)&FSIZE_START_MASK);
		WRITE_REG32(&core->regs.gregs->g_diep_txf[2],tx_fifo_size);
	}

	usb_flush_tx_fifo(core,0x10);
	usb_flush_rx_fifo(core);

	WRITE_REG32(&core->regs.dregs->d_in_ep_msk,0);
	WRITE_REG32(&core->regs.dregs->d_out_ep_msk,0);
	WRITE_REG32(&core->regs.dregs->d_all_irq,0xffffffff);
	WRITE_REG32(&core->regs.dregs->d_all_irq_msk, 0);

	for(i=0;i<core->cfg.dev_endpts;i++) {
		depctl=READ_REG32(&core->regs.in_epregs[i]->d_iepctl);
		if (depctl&DEPCTL_EPENA) {
			depctl=DEPCTL_EPDIS|DEPCTL_SNAK;
		} else {
			depctl=0;
		}
		WRITE_REG32(&core->regs.in_epregs[i]->d_iepctl,depctl);
		WRITE_REG32(&core->regs.in_epregs[i]->d_ieptsiz,0);
		WRITE_REG32(&core->regs.in_epregs[i]->d_in_ep_irq,0xff);
	}

	for(i=0;i<core->cfg.dev_endpts;i++) {
		depctl=READ_REG32(&core->regs.out_epregs[i]->d_oepctl);
		if (depctl&DEPCTL_EPENA) {
			depctl=DEPCTL_EPDIS|DEPCTL_SNAK;
		} else {
			depctl=0;
		}
		WRITE_REG32(&core->regs.out_epregs[i]->d_oepctl,depctl);
		WRITE_REG32(&core->regs.out_epregs[i]->d_oeptsiz,0);
		WRITE_REG32(&core->regs.out_epregs[i]->d_out_ep_irq,0xff);
	}
	
	mask=DIEPINT_TXFIFOUNDRN;
	MOD_REG32(&core->regs.dregs->d_in_ep_msk,mask,mask);

	if (core->cfg.dma_enable==1) {
		dthrctl=DTHRCTL_NISOTHR_EN|DTHRCTL_ISOTHR_EN|
		((64<<DTHRCTL_TXTHRLEN_SHIFT)&DTHRCTL_TXTHRLEN_MASK)|
			DTHRCTL_RXTHR_EN|
		((64<<DTHRCTL_RXTHRLEN_SHIFT)&DTHRCTL_RXTHRLEN_MASK);
		WRITE_REG32(&core->regs.dregs->d_thrctl,dthrctl);
	}
	
	usb_enable_dev_irq(core);

	return 0;
}

void DCD_Init(struct usb_core_data *core, unsigned int coreId) {
	unsigned int i;
	struct usb_otg_ep *ep;

	usb_select_core(core, coreId);

	core->dev.dev_status = USB_OTG_DEFAULT;
	core->dev.dev_address=0;

	for(i=0;i<core->cfg.dev_endpts;i++) {
		ep=&core->dev.in_ep[i];
		ep->is_in=1;
		ep->num=i;
		ep->tx_fifo_num=i;
		ep->type=EP_TYPE_CTRL;
		ep->max_packet=USB_OTG_MAX_EP0_SIZE;
		ep->xfer_buf=0;
		ep->xfer_len=0;
	}

	for(i=0;i<core->cfg.dev_endpts;i++) {
		ep=&core->dev.out_ep[i];
		ep->is_in=0;
		ep->num=i;
		ep->tx_fifo_num=i;
		ep->type=EP_TYPE_CTRL;
		ep->max_packet=USB_OTG_MAX_EP0_SIZE;
		ep->xfer_buf=0;
		ep->xfer_len=0;
	}

	usb_disable_irq(core);
	usb_core_init(core);
	usb_setmode(core,DEVICE_MODE);
	usb_core_init_dev(core);
	usb_enable_irq(core);
}

void usb_bsp_enable_interrupt(struct usb_core_data *core) {
	NVIC_SetPriority(OTG_FS_IRQn,0xc);
	NVIC_EnableIRQ(OTG_FS_IRQn);
}

/*===================================================================*/

static int usb_core_open(void *instance, DRV_CBH cb_handler, void *dev_data) {
	struct usb_core_data *core=(struct usb_core_data *)instance;
	struct usb_dev *usb_dev=(struct usb_dev *)dev_data;

	io_printf("usb_core_open: bububuu\n");

	core->dev.class_cb=usb_dev->class_cb;
	core->dev.usr_cb=usb_dev->usr_cb;
	core->dev.usr_dev=usb_dev->device;

	DCD_Init(core, USB_OTG_FS_CORE_ID);

	usb_bsp_enable_interrupt(core);

	return 0;
}

static int usb_core_close(int driver_fd) {
	return 0;
}

static int usb_core_control(int driver_fd, int cmd, void *arg1, int arg2) {
	return 0;
}



static int usb_core_init_drv(void *instance) {

	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
	
	GPIOA->MODER |= (0x2<<16);	/* Pin 8 to AF */
	GPIOA->OSPEEDR |= (0x3 << 16);	/* Pin 8 to High speed */
	GPIOA->OTYPER &= ~(1<<8);	/* Pin 8 to PP */
	GPIOA->PUPDR  &= ~(0x3<<16);	/* Pin 8 to NoPull */

	GPIOA->MODER |= (0x2<<18);	/* Pin 9 to AF */
	GPIOA->OSPEEDR |= (0x3 << 18);	/* Pin 9 to High speed */
	GPIOA->OTYPER &= ~(1<<9);	/* Pin 9 to PP */
	GPIOA->PUPDR  &= ~(0x3<<18);	/* Pin 9 to NoPull */

	GPIOA->MODER |= (0x2<<22);	/* Pin 11 to AF */
	GPIOA->OSPEEDR |= (0x3 << 22);	/* Pin 11 to High speed */
	GPIOA->OTYPER &= ~(1<<11);	/* Pin 11 to PP */
	GPIOA->PUPDR  &= ~(0x3<<22);	/* Pin 11 to NoPull */

	GPIOA->MODER |= (0x2<<24);	/* Pin 12 to AF */
	GPIOA->OSPEEDR |= (0x3 << 24);	/* Pin 12 to High speed */
	GPIOA->OTYPER &= ~(1<<12);	/* Pin 12 to PP */
	GPIOA->PUPDR  &= ~(0x3<<24);	/* Pin 12 to NoPull */

	GPIOA->AFR[1] |= 0x0000000a;    /* Pin 8 to AF_OTG_FS */
	GPIOA->AFR[1] |= 0x000000a0;    /* Pin 9 to AF_OTG_FS */
	GPIOA->AFR[1] |= 0x0000a000;    /* Pin 11 to AF_OTG_FS */
	GPIOA->AFR[1] |= 0x000a0000;    /* Pin 12 to AF_OTG_FS */

	GPIOA->MODER &= ~(0x3<<20);	/* Pin 10 to input */
	GPIOA->PUPDR  |= (0x1<<20);	/* Pin 10 to PullUp */
	GPIOA->AFR[1] |= 0x00000a00;    /* Pin 10 to AF_OTG_FS */


	RCC->APB2ENR|=RCC_APB2ENR_SYSCFGEN;
	RCC->AHB2ENR|=RCC_AHB2ENR_OTGFSEN;

	 /* enable the PWR clock */
	RCC->APB1RSTR|=RCC_APB1RSTR_PWRRST;
	return 0;
};


static int usb_core_start(void *instance) {
	return 0;
};


static struct driver_ops usb_core_ops = {
	usb_core_open,
	usb_core_close,
	usb_core_control,
	usb_core_init_drv,
	usb_core_start,
};


static struct driver usb_core_drv = {
	"usb_core",
	&usb_core_data,
	&usb_core_ops,
};


void init_usb_core_drv(void) {
	driver_publish(&usb_core_drv);
}


INIT_FUNC(init_usb_core_drv);
