
//  Control and status register bits

// B-session valid
#define G_OTG_CTL_BSVLD		0x00080000
// A-session valid
#define G_OTG_CTL_ASVLD		0x00040000
// Debounce time 0=long, 1=short
#define G_OTG_CTL_DBCT		0x00020000
// Connector ID status
#define G_OTG_CTL_CIDSTS	0x00010000
// Device HNP enable
#define G_OTG_CTL_DHNPEN	0x00000800
// Host set HNP enable
#define G_OTG_CTL_HSHNPEN	0x00000400
// HNP request
#define G_OTG_CTL_HNPRQ		0x00000200
// Host negotiation succuess
#define G_OTG_CTL_HNGSCS	0x00000100
// Session request
#define G_OTG_CTL_SRQ		0x00000002
// Session request success
#define G_OTG_CTL_SRQSCSC	0x00000001

// Interrupt(ack) register
// Debounce done
#define G_OTG_INT_DBCDNE	0x00080000
// A-device timeout change
#define G_OTG_INT_ADTOCHG	0x00040000
// Host negotiation detected
#define G_OTG_INT_HNGDET	0x00020000
// Host negotiation success status change
#define G_OTG_INT_HNSSCHG	0x00000200
// Session request success status change
#define G_OTG_INT_SRSSCHG	0x00000100
// Session end detected
#define G_OTG_INT_SEDET		0x00000004



// AHB configuration register

// (host mode)
#define G_AHB_CFG_PTXFELVL	0x100		// Periodic TxFIFO Empty level,
						//	Not Set= irq at half e.
						//	Set= irq at empty
// (dev mode)
#define G_AHB_CFG_TXFELVL	0x80		// TxFifo Empty Level
						// not Set: irq at half
						// Set: irq at empty
#define G_AHB_CFG_DMAEN		0x20
// HTBSTLEN (BURST LENGTH/TYPE)
// 0 == Single
// 1 == INCR
// 3 == INCR4
// 5 == INCR8
// 7 == INCR16
#define	G_AHB_CFG_HBSTLEN_MASK	0x1e
#define G_AHB_CFG_HBSTLEN_SHIFT	1
// Global interrupt mask: 0=mask the interrupt, 1=Unmask the interrupt
#define G_AHB_GINT		1


// USB configuration register
//
// Corrupt Tx packet (for debug)
#define G_USB_CFG_CTXPKT	0x80000000
// Forced Peripheral mode
#define G_USB_CFG_FDMOD		0x40000000
// Forced Host mode
#define G_USB_CFG_FHMOD		0x20000000
// ULPI interface protect disable
#define G_USB_CFG_ULPIIPD	0x02000000
// Indicator pass through
#define G_USB_CFG_PTCI		0x01000000
// Indicator complement
#define G_USB_CFG_PCCI		0x00800000
// (UTMI) TermSel DLine pulsing selection
#define G_USB_CFG_TSDPS		0x00400000
// ULPI External VBUS indicator
#define G_USB_CFG_ULPIEVBUSI	0x00200000
// ULPI External VBUS Drive
#define G_USB_CFG_ULPIEVBUSD	0x00100000
// ULPI Clock SuspendM
#define G_USB_CFG_ULPICSM	0x00080000
// ULPI Auto-resume
#define G_USB_CFG_ULPIAR	0x00040000
// ULPI FS/LS select
#define G_USB_CFG_ULPIFSLS	0x00020000
// PHY low-power clock
#define G_USB_CFG_PHYLPCS	0x00008000
// USB turnaround time
// =9 for AHB 72 Mhz, 5 for AHB 48 Mhz
#define G_USB_CFG_TRDT_MSK	0x00003c00
#define G_USB_CFG_TRDT_SHIFT	10
// HNP-capable
#define G_USB_CFG_HNPCAP	0x00000200
// SRP-capable
#define G_USB_CFG_SRPCAP	0x00000100
// PHSEL 0=usb2.0 hs ULPI PHY, 1=usb1.1 full-speed serial transeiver
#define G_USB_CFG_PHSEL		0x00000040
// FS timeout calibration
#define G_USB_CFG_TOCAL_MASK	0x00000007
#define G_USB_CFG_TOCAL_SHIFT	0

// Reset register
//
// AHB master idle (r)
#define G_RST_CFG_AHBIDL	0x80000000
// DMA requeset signal (debug) (r)
#define G_RST_CFG_DMAREQ	0x40000000
// TxFifo number for flush
#define G_RST_CFG_TXFNUM_MASK	0x000007c0
#define G_RST_CFG_TXFNUM_SHIFT	6
// TxFifo flush
#define G_RST_CFG_TXFFLSH	0x00000020
// RxFifo flush
#define G_RST_CFG_RXFFLSH	0x00000010
// Host frame counter reset
#define G_RST_CFG_FCRST		0x00000004
// HCLK soft reset
#define G_RST_CFG_HSRST		0x00000002
// Core soft reset
#define G_RST_CFG_CSRST		0x00000001

// core interrupt register (status)
//
// Resume/remote wakeup detected
#define G_INT_STS_WKUIN		0x80000000
// Session request/new session detected
#define G_INT_STS_SRQINT	0x40000000
// Disconnect detected
#define G_INT_STS_DISCINT	0x20000000
// Connector ID status change
#define G_INT_STS_CIDSCHG	0x10000000
// Periodic TxFifo empty
#define G_INT_STS_PTXFE		0x04000000
// Host channels interrupt
#define G_INT_STS_HCINT		0x02000000
// Host Port interrupt
#define G_INT_STS_HPRTINT	0x01000000
// Data fetch suspended
#define G_INT_STS_DATAFSUSP	0x00400000
// Incomplete periodic transfer
#define G_INT_STS_IPXFR		0x00200000
// Incomplete isochronous out transfer
#define G_INT_STS_INCOMPISOOUT	0x00200000
// Incomplete isochronous IN transfer
#define G_INT_STS_IISOIXFR	0x00100000
// OUT endpoint interrupt
#define G_INT_STS_OEPINT	0x00080000
// IN endpoint interrupt
#define G_INT_STS_IEPINT	0x00040000
// End of periodic frame interrupt
#define G_INT_STS_EOPF		0x00008000
// Isochronous OUT packet dropped interrupt
#define G_INT_STS_ISOODRP	0x00004000
// Enumeration done
#define G_INT_STS_ENUMDNE	0x00002000
// USB reset
#define G_INT_STS_USBRST	0x00001000
// USB suspend
#define G_INT_STS_USBSUSP	0x00000800
// Early suspend
#define G_INT_STS_ESUSP		0x00000400
// Global OUT NAK effective
#define G_INT_STS_GONAKEFF	0x00000080
// Global IN nonperiodic NAK effective
#define G_INT_STS_GINAKEFF	0x00000040
// Nonperiodic TxFIFO empty
#define G_INT_STS_NPTXFE	0x00000020
// RxFifo nonempty
#define G_INT_STS_RXFLVL	0x00000010
// Start of frame
#define G_INT_STS_SOF		0x00000008
// OTG interrupt
#define G_INT_STS_OTGINT	0x00000004
// Mode mismatch interrupt
#define G_INT_STS_MMIS		0x00000002
// Current mode of operation
#define G_INT_STS_CMOD		0x00000001

#define G_RX_STS_FRMNUM_MSK	0x01e00000
#define G_RX_STS_FRMNUM_SHIFT	21
#define G_RX_STS_PKTSTAT_MSK	0x001e0000
#define G_RX_STS_PKTSTAT_SHIFT	17
#define G_RX_STS_PKTSTAT_GOUT_NAK 1
#define G_RX_STS_PKTSTAT_ODATA_RECEIVED 2
#define G_RX_STS_PKTSTAT_OXFER_DONE	3
#define G_RX_STS_PKTSTAT_SXFER_DONE	4
#define G_RX_STS_PKTSTAT_SDATA_RECEIVED 6
#define G_RX_STS_DPID_MSK	0x00018000
#define G_RX_STS_DPID_SHIFT	15
#define G_RX_STS_DPID_DATA0	0
#define G_RX_STS_DPID_DATA1	2
#define G_RX_STS_DPID_DATA2	1
#define G_RX_STS_DPID_MDATA	3
#define G_RX_STS_BCNT_MSK	0x00007ff0
#define G_RX_STS_BCNT_SHIFT	4
#define G_RX_STS_EPNUM_MSK	0x0000000f
#define G_RX_STS_EPNUM_SHIFT	0


// g_c_cfg

#define G_C_CFG_NOVBUSSENS	0x00200000
#define G_C_CFG_SOFOUTEN	0x00100000
#define G_C_CFG_VBUSBSEN	0x00080000
#define G_C_CFG_VBUSASEN	0x00040000
#define G_C_CFG_I2CPADEN	0x00020000
// Power down 0=Power down active, 1=Not power down
#define G_C_CFG_PWRDWN		0x00010000

#define G_NPTX_FCNF_D_MASK	0xffff0000
#define G_NPTX_FCNF_D_SHIFT 16
#define G_NPTX_FCNF_A_MASK	0x0000ffff
#define G_NPTX_FCNF_A_SHIFT 0

#define G_NPTXSTAT_QTOP_MSK 	0x7f000000
#define G_NPTXSTAT_QTOP_SHIFT	24
#define G_NPTXSTAT_QSAV_MSK	0x00ff0000
#define G_NPTXSTAT_QSAV_SHIFT	16
#define G_NPTXSTAT_FSAV_MSK	0x0000ffff
#define G_NPTXSTAT_FSAV_SHIFT	0

#define PTX_FCNF_D_MASK		0xffff0000
#define PTX_FCNF_D_SHIFT	16
#define PTX_FCNF_A_MASK		0x0000ffff
#define PTX_FCNF_A_SHIFT	0

struct core_regs {
	volatile unsigned int g_otg_ctl;	// 0x000
	volatile unsigned int g_otg_gint;	// 0x004
	volatile unsigned int g_ahb_cfg;	// 0x008
	volatile unsigned int g_usb_cfg;	// 0x00c
	volatile unsigned int g_rst_ctl;	// 0x010
	volatile unsigned int g_int_sts;	// 0x014
	volatile unsigned int g_int_msk;	// 0x018
	volatile unsigned int g_rx_sts_r;	// 0x01c
	volatile unsigned int g_rx_sts_p;	// 0x020
	volatile unsigned int g_rx_fsiz;	// 0x024
	volatile unsigned int g_nptxfcnf;	// 0x028
	volatile unsigned int g_nptxsts;	// 0x02c
	volatile unsigned int g_i2c_ctl;	// 0x030
	volatile unsigned int pad1;		// 0x034
	volatile unsigned int g_c_cfg;		// 0x038
	volatile unsigned int cid;		// 0x03c
	volatile unsigned char pad2[192];	// 0x040 
	volatile unsigned int hptx_fcnf;	// 0x100
	volatile unsigned int dieptxf[4];   /* index 1..4 */ // 0x104
};


#define H_CFG_FSLSS		0x00000004
#define H_CFG_FSLSPC_48		0x00000001
#define H_CFG_FSLSPC_6		0x00000002
#define H_CFG_FSLSPC_MASK	0x00000003

#define H_PRT_SPEED_MASK	0x00060000
#define H_PRT_SPEED_HIGH	0x00000000
#define H_PRT_SPEED_FULL	0x00020000
#define H_PRT_SPEED_LOW		0x00040000

#define H_PRT_PTCTL_MASK	0x0001e000
#define H_PRT_PTCTL_OFF		0x00000000
#define H_PRT_PTCTL_J_MODE	0x00002000
#define H_PRT_PTCTL_K_MODE	0x00004000
#define H_PRT_PTCTL_SE0NAK	0x00006000
#define H_PRT_PTCTL_PACKET	0x00008000
#define H_PRT_PTCTL_FE		0x0000c000

#define H_PRT_PPWR		0x00001000
#define H_PRT_PLSTS_DM		0x00000800
#define H_PRT_PLSTS_DP		0x00000400
#define H_PRT_PRST		0x00000100
#define H_PRT_PSUSP		0x00000080
#define H_PRT_PRES		0x00000040
#define H_PRT_POCCHNG		0x00000020
#define H_PRT_POCA		0x00000010
#define H_PRT_PENCHNG		0x00000008
#define H_PRT_PENA		0x00000004
#define H_PRT_PCDET		0x00000002
#define H_PRT_PCSTS		0x00000001

#define H_PTXSTS_QTOP_MSK	0xff000000
#define H_PTXSTS_QTOP_SHIFT	24
#define H_PTXSTS_QSAV_MSK	0x00ff0000
#define H_PTXSTS_QSAV_SHIFT	16
#define H_PTXSTS_FSAV_MSK	0x0000ffff
#define H_PTXSTS_FSAV_SHIFT	0

struct hs_regs {
	unsigned int h_cfg;		// 0x00
	unsigned int h_fir;		// 0x04
	unsigned int h_fnum;		// 0x08
	unsigned int h_pad1;		// 0x0c
	unsigned int h_ptxsts;		// 0x10
	unsigned int h_aint;		// 0x14
	unsigned int h_aint_msk;	// 0x18
	unsigned int h_pad2[9];		// 0x1c-0x3c
	unsigned int h_prt;		// 0x40
};

#define HC_CHAR_CHENA		0x80000000
#define HC_CHAR_CHDIS		0x40000000
#define HC_CHAR_ODDFRM		0x20000000
#define HC_CHAR_DEVADDR_MSK	0x1fc00000
#define HC_CHAR_DEVADDR_SHIFT	22
#define HC_CHAR_MC_MSK		0x00300000
#define HC_CHAR_MC_SHIFT	20
#define HC_CHAR_ETYPE_MSK	0x000c0000
#define HC_CHAR_ETYPE_SHIFT	18
#define HC_CHAR_ETYPE_CTRL	0x00000000
#define HC_CHAR_ETYPE_ISO	0x00040000
#define HC_CHAR_ETYPE_BULK	0x00080000
#define HC_CHAR_ETYPE_IRQ	0x000c0000
#define HC_CHAR_LSDEV		0x00020000
#define HC_CHAR_EPDIR_IN	0x00008000
#define HC_CHAR_EPNUM_MSK	0x00007800
#define HC_CHAR_EPNUM_SHIFT	11
#define HC_CHAR_MPSIZ_MSK	0x000007ff
#define HC_CHAR_MPSIZ_SHIFT	0

#define HC_INT_DTERR		0x00000400
#define HC_INT_FROVR		0x00000200
#define HC_INT_BBLERR		0x00000100
#define HC_INT_TXERR		0x00000080
#define HC_INT_NYET		0x00000040
#define HC_INT_ACK		0x00000020
#define HC_INT_NAK		0x00000010
#define HC_INT_STALL		0x00000008
#define HC_INT_AHBERR		0x00000004
#define HC_INT_CHLT		0x00000002
#define HC_INT_XFERC		0x00000001

#define HC_TSIZ_DOPING		0x80000000
#define HC_TSIZ_DPID_MSK	0x60000000
#define HC_TSIZ_DPID_SHIFT	29
#define HC_TSIZ_PKTCNT_MSK	0x1ff80000
#define HC_TSIZ_PKTCNT_SHIFT	19
#define HC_TSIZ_XFRSIZ_MSK	0x0007ffff
#define HC_TSIZ_XFRSIZ_SHIFT	0

struct hc_regs {
	volatile unsigned int hc_char;	// 0x00
	volatile unsigned int hc_splt;	// 0x04
	volatile unsigned int hc_int;	// 0x08
	volatile unsigned int hc_intmsk;// 0x0c
	volatile unsigned int hc_tsiz;	// 0x10
	volatile unsigned int hc_dma;	// 0x14
	volatile unsigned int hc_pad1;	// 0x18
	volatile unsigned int hc_pad2;	// 0x1c
};

#define EP_CTL_ENA		0x80000000
#define EP_CTL_DIS		0x40000000
#define EP_CTL_SD1PID 		0x20000000
#define EP_CTL_SODDFRM		0x20000000
#define EP_CTL_SD0PID		0x10000000
#define EP_CTL_SEVNFRM		0x10000000
#define EP_CTL_SNAK		0x08000000
#define EP_CTL_CNAK		0x04000000
#define EP_CTL_TXFNUM_MSK	0x03c00000
#define EP_CTL_TXFNUM_SHIFT	22
#define EP_CTL_STALL		0x00200000
#define EP_CTL_EPTYPE_MSK	0x000c0000
#define EP_CTL_EPTYPE_SHIFT	18
#define EP_CTL_NAKSTS		0x00020000
#define EP_CTL_EONUM		0x00010000
#define EP_CTL_DPID		0x00010000
#define EP_CTL_USBAEP		0x00008000
#define EP_CTL_MPSIZ_MSK	0x000007ff
#define EP_CTL_MPSIZ_SHIFT	0

#define EP_TSIZ_MCNT_MSK	0x60000000
#define EP_TSIZ_MCNT_SHIFT	29
#define EP_TSIZ_SUPCNT_MSK	0x60000000
#define EP_TSIZ_SUPCNT_SHIFT	29
#define EP_TSIZ_PKTCNT_MSK	0x1ff80000
#define EP_TSIZ_PKTCNT_SHIFT	19
#define EP_TSIZ_XFRSIZ_MSK	0x0007ffff
#define EP_TSIZ_XFRSIZ_SHIFT	0

#define EP_IINT_NAK		0x00002000
#define EP_IINT_BERR		0x00001000
#define EP_IINT_PKTDRPSTS	0x00000800
#define EP_IINT_BNA		0x00000200
#define EP_IINT_TXFUR		0x00000100
#define EP_IINT_TXFE		0x00000080
#define EP_IINT_INEPNE		0x00000040
#define EP_IINT_ITTXFE		0x00000010
#define EP_IINT_TIMEOUT		0x00000008
#define	EP_IINT_DISABLED	0x00000002
#define EP_IINT_XFER_DONE	0x00000001

#define EP_OINT_NYET		0x00004000
#define EP_OINT_B2B_SETUP	0x00000040
#define EP_OINT_OTEPDIS		0x00000010
#define EP_OINT_SETUP_DONE	0x00000008
#define EP_OINT_DISABLED	0x00000002
#define EP_OINT_XFER_DONE	0x00000001

#define EP_IN_TXF_STS_INEPTFSAV_MSK 0x0000ffff

struct d_ep_in {
	volatile unsigned int ctl;	// 0x00
	volatile unsigned int pad1;	// 0x04
	volatile unsigned int ints;	// 0x08
	volatile unsigned int pad2;	// 0x0c
	volatile unsigned int tsiz;	// 0x10
	volatile unsigned int dma;	// 0x14
	volatile unsigned int txf_sts;	// 0x18
	volatile unsigned int dmab;      // 0x1c
};


struct d_ep_out {
	volatile unsigned int ctl;	// 0x00
	volatile unsigned int pad1;	// 0x04
	volatile unsigned int ints;	// 0x08
	volatile unsigned int pad2;	// 0x0c
	volatile unsigned int tsiz;	// 0x10
	volatile unsigned int dma;	// 0x14
	volatile unsigned int pad3;	// 0x18
	volatile unsigned int dmab;	// 0x1c
};

#define DCFG_INTRVL_MSK		0x03000000
#define DCFG_INTRVL_SHIFT	24

#define DCFG_FINTRVL_MSK	0x00001800
#define DCFG_FINTRVL_SHIFT	11
#define DCFG_FINTRVL_80		0x00000000
#define DCFG_FINTRVL_85		0x00000800
#define DCFG_FINTRVL_90		0x00001000
#define DCFG_FINTRVL_95		0x00001800

#define DCFG_DADDR_MSK		0x000007f0
#define DCFG_DADDR_SHIFT	4
#define DCFG_STALL_HSK		0x00000004
#define DCFG_DEV_SPEED_MSK	0x00000003
#define DCFG_DEV_SPEED_SHIFT	0

#define DCFG_DEV_SPEED_HI		0
#define DCFG_DEV_SPEED_FS_FULL		1
#define DCFG_DEV_SPEED_FULL_EPHY	1
#define DCFG_DEV_SPEED_FULL_IPHY	3

#define DCTL_POPRGDNE		0x00000800
#define DCTL_CGONAK		0x00000400
#define DCTL_SGONAK		0x00000200
#define DCTL_CGINAK		0x00000100
#define DCTL_SGINAK		0x00000080
#define DCTL_TCTL_MSK		0x00000070
#define DCTL_TCTL_SHIFT		4
#define DCTL_GONSTS		0x00000008
#define DCTL_GINSTS		0x00000004
#define DCTL_SDIS		0x00000002
#define DCTL_RWUSIG		0x00000001

#define DSTS_FNSOF_MSK		0x003fff00
#define DSTS_FNSOF_SHIFT	8
#define DSTS_EERR		0x00000008
#define DSTS_ENUMSPD_MSK	0x00000006
#define DSTS_ENUMSPD_SHIFT	1
#define DSTS_ENUMSPD_HI		0
#define DSTS_ENUMSPD_FULL	3
#define DSTS_SUSP		0x00000001

#define DDIEPMSK_BI		0x00000200
#define DDIEPMSK_TXFUR		0x00000100
#define DDIEPMSK_INEPNE		0x00000040
#define DDIEPMSK_INEPNM		0x00000020
#define DDIEPMSK_ITTXFE		0x00000010
#define DDIEPMSK_TO		0x00000008
#define DDIEPMSK_EPD		0x00000002
#define DDIEPMSK_XFRC		0x00000001

#define DDOEPMSK_BNA		0x00000200
#define DDOEPMSK_OPERR		0x00000100
#define DDOEPMSK_B2BSETUP	0x00000040
#define DDOEPMSK_OTEPDIS	0x00000010
#define DDOEPMSK_SETUP		0x00000008
#define DDOEPMSK_EPDISABLED	0x00000002
#define DDOEPMSK_XFER_DONE	0x00000001

#define DTHRCTL_ARPEN		0x08000000
#define DTHRCTL_RXTHRLEN_MSK	0x03fe0000
#define DTHRCTL_RXTHRLEN_SHIFT	17
#define DTHRCTL_RXTHREN		0x00010000
#define DTHRCTL_TXTHRLEN_MSK	0x000007fc
#define DTHRCTL_TXTHRLEN_SHIFT	2
#define DTHRCTL_ISOTHREN	2
#define DTHRCTL_NONISOTHREN	1

struct dev_regs {
	volatile unsigned int d_cfg;		// 0x000
	volatile unsigned int d_ctl;		// 0x004
	volatile unsigned int d_sts;		// 0x008
	volatile unsigned int d_pad1;		// 0x00c
	volatile unsigned int d_diep_msk;	// 0x010
	volatile unsigned int d_doep_msk;	// 0x014
	volatile unsigned int d_aint;		// 0x018
	volatile unsigned int d_aintmsk;	// 0x01c
	volatile unsigned int d_pad2[2];	// 0x020-0x024
	volatile unsigned int d_vbus_dis;	// 0x028
	volatile unsigned int d_vbus_pulse;	// 0x02c
	volatile unsigned int d_dthr_ctl;	// 0x030
	volatile unsigned int d_iep_empty_msk;	// 0x034
	volatile unsigned int d_each_int;	// 0x038
	volatile unsigned int d_each_int_msk;	// 0x03c
	volatile unsigned int d_pad3;		// 0x040
	volatile unsigned int d_iep_each_msk1;	// 0x044
	volatile unsigned char d_pad4[60];	// 0x048
	volatile unsigned int d_oep_each_msk1;	// 0x084
	volatile unsigned char d_pad5[120];	// 0x088
	struct d_ep_in	d_ep_in[16];		// 0x100
	struct d_ep_out	d_ep_out[16];		// 0x300
};

#define NUM_EPS_HOST	0xB
#define NUM_EPS_DEV	0x5

#define PWRCLK_PHYSUSP	0x00000010
#define PWRCLK_GATECLK	0x00000002
#define PWRCLK_STOPPCLK	0x00000001

struct usb_otg_regs {
	union {				// 0x000
		struct core_regs core;
		unsigned char core_regs_space[0x400];
	};
	union {				// 0x400
		struct hs_regs	 host;
		unsigned char host_regs_space[0x100];
	};
	union {
		struct hc_regs	hc[12];	// 0x500
		unsigned char host_channel_regs_space[0x300];
	};
	union {				// 0x800
		struct dev_regs dev;
		unsigned char dev_regs_space[0x500];
	};
	unsigned char pad1[0x100];	// 0xd00
	volatile unsigned int  pwrclk; // 0xe00
	unsigned char pad2[0x200-4];
	volatile unsigned int  dfifo[11][0x400];
};
