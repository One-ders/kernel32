
#include <devices.h>
#include <stm32f407.h>
#include <sys.h>
#include <io.h>
#include <string.h>

#include <usb_core.h>
#include <usb_device.h>


static int reset_core(struct usb_otg_regs *);
static int write_pkt(struct usb_dev_handle *,unsigned int cp_num, unsigned char *buf, size_t len);
static int read_pkt(struct usb_dev_handle *, unsigned char *buf, size_t len);
static unsigned int get_mode(struct usb_otg_regs *);
static unsigned int get_core_irq_stat(struct usb_otg_regs *);
static unsigned int get_otg_irq_stat();
        /****                                     ****/
static int host_init_core();
static int host_enable_int(struct usb_dev_handle *);
static void host_stop();
static int isEvenFrame(struct usb_otg_regs *);
static void vbus_drive(struct usb_otg_regs *regs, unsigned int state);
static void init_fs_ls_p_clksel(struct usb_otg_regs *,unsigned int freq);
static unsigned int read_hprt0(struct usb_otg_regs *);
static unsigned int get_host_all_channel_irq(struct usb_otg_regs *);
static int reset_port();
        /****                                     ****/
static int host_channel_init(struct usb_dev_handle *,unsigned int host_channel);
static int host_channel_start_tx(struct usb_dev_handle *, unsigned int host_channel);
static int host_channel_halt(struct usb_otg_regs *,unsigned int host_channel);
static int host_channel_do_ping(struct usb_otg_regs *,unsigned int host_channel);
        /****                                    ****/
static void dev_speed_init(struct usb_otg_regs *, unsigned int speed);
static int dev_enable_int();
static unsigned int dev_get_speed(struct usb_otg_regs *);
static int dev_EP0_activate(struct usb_otg_regs *);
static unsigned int dev_read_all_out_ep_irq();
static unsigned int dev_read_out_ep_irq(struct usb_otg_regs *,unsigned int channel);
static unsigned int dev_read_all_in_ep_irq();
void dev_remote_wakeup();
static void dev_ungate_clock();
static void dev_stop();

struct usb_host *usb_host;


/*****************************************************/

static int config_vbus(void) {
	sys_printf("config vbus\n");
	return 0;
}

static int drive_vbus(unsigned int state) {
	sys_printf("drive vbus %d\n", state);
	return 0;
}

/*****************************************************/

static void common_irq_enable(struct usb_otg_regs *regs) {

	regs->core.g_otg_gint=0xffffffff;
	regs->core.g_int_sts= 0xbfffffff;

	regs->core.g_int_msk=(G_INT_STS_WKUIN|G_INT_STS_USBSUSP|
				G_INT_STS_OTGINT|G_INT_STS_SRQINT|
                                G_INT_STS_CIDSCHG);
}

static int reset_core(struct usb_otg_regs *regs) {
	int count=20000;
	while(!(regs->core.g_rst_ctl&G_RST_CFG_AHBIDL)) {
                sys_udelay(10);  
		count--;
		if (!count) return -1;
        }
        regs->core.g_rst_ctl|=G_RST_CFG_CSRST;
        while(regs->core.g_rst_ctl&G_RST_CFG_CSRST);
        sys_udelay(3);
	return 0;
}

static int write_pkt(struct usb_dev_handle *pdev,
			unsigned int ep_num, 
			unsigned char *buf, 
			size_t len) {

	if (!pdev->cfg.dma_enable) {
		unsigned int i;
		unsigned int wcount=(len+3)>>2;
		volatile unsigned int *fifo;
		
		fifo=pdev->regs->dfifo[ep_num];
		for(i=0;i<wcount;i++) {
			*fifo=((unsigned int *)buf)[i];
		}
	}
	return 0;
}

static int read_pkt(struct usb_dev_handle *pdev, 
			unsigned char *buf, 
			size_t len) {
	unsigned int i;
	unsigned int wcount=(len+3)>>2;
	volatile unsigned int *fifo=pdev->regs->dfifo[0];
	
	for(i=0;i<wcount;i++) {
		((unsigned int *)buf)[i]=*fifo;
	}
	return 0;
}

int usb_config_core(struct usb_dev_handle *pdev) {
	pdev->cfg.dma_enable=0;
	pdev->cfg.speed=USB_SPEED_FULL;
	pdev->cfg.maxpacketsize=USB_FS_MAX_PACKET_SIZE;

#if defined(MB1075B)
	pdev->regs=(struct usb_otg_regs *)USB_OTG_HS_ADDR;
	pdev->cfg.host_channels         = 12;
	pdev->cfg.dev_endpoints         = 6;
	pdev->cfg.total_fifo_size       = 1280; /* ints */
	pdev->cfg.phy_itf               = USB_OTG_EMBEDDED_PHY;
	pdev->cfg.dma_enable            = 0;

#ifdef USB_SOF_ENABLED
	pdev->cfg.sof_output            = 1;
#endif

#ifdef USB_LOW_PWR_MGMT_SUPPORT
	pdev->cfg.low_power             = 1;
#endif

#elif defined(MB997C)
	pdev->regs=(struct usb_otg_regs *)USB_OTG_FS_ADDR;
	pdev->cfg.host_channels         = 8;
	pdev->cfg.dev_endpoints         = 4;
	pdev->cfg.total_fifo_size       = 320; /* ints */
	pdev->cfg.phy_itf               = USB_OTG_EMBEDDED_PHY;

#ifdef USB_SOF_ENABLED
	pdev->cfg.sof_output            = 1;
#endif

#ifdef USB_LOW_PWR_MGMT_SUPPORT
	pdev->cfg.low_power             = 1;
#endif

#else
	return -1;
#endif
	return 0;
}

int usb_core_init_core(struct usb_otg_regs *regs, struct usb_cfg *usb_cfg) {
	/* Use the embedded FS interface */
	regs->core.g_usb_cfg|=G_USB_CFG_PHSEL;
	reset_core(regs);
	if (usb_cfg->sof_output) {
		regs->core.g_c_cfg=G_C_CFG_SOFOUTEN|
				G_C_CFG_VBUSBSEN|
				G_C_CFG_VBUSASEN|
				G_C_CFG_PWRDWN;
		
	} else {
		regs->core.g_c_cfg=G_C_CFG_VBUSBSEN|
				G_C_CFG_VBUSASEN|
				G_C_CFG_PWRDWN;
	}
	sys_mdelay(20);

	if (usb_cfg->dma_enable) {
		regs->core.g_ahb_cfg|=(G_AHB_CFG_DMAEN|
				(5<<G_AHB_CFG_HBSTLEN_SHIFT));
	}

	regs->core.g_usb_cfg|=(G_USB_CFG_HNPCAP|G_USB_CFG_SRPCAP);	
	
	common_irq_enable(regs);
	return 0;
}

int usb_core_enable_gint(struct usb_otg_regs *regs) {
	regs->core.g_ahb_cfg|=G_AHB_GINT;
	return 0;
}

int usb_core_disable_gint(struct usb_otg_regs *regs) {
	regs->core.g_ahb_cfg&=~G_AHB_GINT;
	return 0;
}

int usb_core_flush_tx_fifo(struct usb_otg_regs *regs, unsigned int fifo) {
	unsigned int count=200000;

	regs->core.g_rst_ctl=G_RST_CFG_TXFFLSH|
				(fifo<<G_RST_CFG_TXFNUM_SHIFT);
	while(regs->core.g_rst_ctl&G_RST_CFG_TXFFLSH) {
		count--;
		if (!count) return -1;
	}
	sys_udelay(3);
	return 0;
}

int usb_core_flush_rx_fifo(struct usb_otg_regs *regs) {
	unsigned int count=200000;

	regs->core.g_rst_ctl=G_RST_CFG_RXFFLSH;
	while(regs->core.g_rst_ctl&G_RST_CFG_RXFFLSH) {
		count--;
		if (!count) return -1;
	}
	sys_udelay(3);
	return 0;
}

int usb_core_set_mode(struct usb_otg_regs *regs, unsigned int mode) {
	
	unsigned int usbcfg=regs->core.g_usb_cfg;
	usbcfg&=~(G_USB_CFG_FHMOD|G_USB_CFG_FDMOD);
	if (mode==HOST_MODE) {
		usbcfg|=G_USB_CFG_FHMOD;
	} else if (mode==DEVICE_MODE) {
		usbcfg|=G_USB_CFG_FDMOD;
	} else {
		return -1;
	}
	regs->core.g_usb_cfg=usbcfg;
	sys_mdelay(50);
	return 0;
}

static unsigned int get_mode(struct usb_otg_regs *regs) {
	return regs->core.g_int_sts&G_INT_STS_CMOD;
}

static unsigned int get_core_irq_stat(struct usb_otg_regs *regs) {
	unsigned int i=regs->core.g_int_sts;
	i&=regs->core.g_int_msk;
	return i;
}

static unsigned int get_otg_irq_stat(struct usb_otg_regs *regs) {
	return regs->core.g_otg_gint;
}

static int host_init_core(struct usb_dev_handle *pdev) {
	int i;	
	config_vbus();
	
	pdev->regs->pwrclk=0;
	init_fs_ls_p_clksel(pdev->regs, H_CFG_FSLSPC_48);
	reset_port();
	
	pdev->regs->host.h_cfg&=~H_CFG_FSLSS;
	
	if (pdev->regs==USB_OTG_HS_ADDR) {
		pdev->regs->core.g_rx_fsiz=RX_FIFO_HS_SIZE; // 512
		pdev->regs->core.g_nptxfcnf=(TXH_NP_FIFO_HS_SIZE<<G_NPTX_FCNF_D_SHIFT)| // 96
				(RX_FIFO_HS_SIZE<<G_NPTX_FCNF_A_SHIFT);  // 512

		pdev->regs->core.hptx_fcnf=(TXH_P_FIFO_HS_SIZE<<PTX_FCNF_D_SHIFT) |
			((RX_FIFO_HS_SIZE+TXH_NP_FIFO_HS_SIZE)<<PTX_FCNF_A_SHIFT);
	} else {
		pdev->regs->core.g_rx_fsiz=RX_FIFO_FS_SIZE; // 512
		pdev->regs->core.g_nptxfcnf=(TXH_NP_FIFO_FS_SIZE<<G_NPTX_FCNF_D_SHIFT)| // 96
				(RX_FIFO_FS_SIZE<<G_NPTX_FCNF_A_SHIFT);  // 512

		pdev->regs->core.hptx_fcnf=(TXH_P_FIFO_FS_SIZE<<PTX_FCNF_D_SHIFT) |
			((RX_FIFO_FS_SIZE+TXH_NP_FIFO_FS_SIZE)<<PTX_FCNF_A_SHIFT);
	}

	pdev->regs->core.g_otg_ctl&=~G_OTG_CTL_HSHNPEN;

	usb_core_flush_tx_fifo(pdev->regs,0x10);
	usb_core_flush_rx_fifo(pdev->regs);

	for(i=0;i<pdev->cfg.host_channels;i++) {
		pdev->regs->hc[i].hc_int=0xffffffff;
		pdev->regs->hc[i].hc_intmsk=0;
	}
	
	vbus_drive(pdev->regs,1);

	host_enable_int(pdev);

	return 0;	
}

static int isEvenFrame(struct usb_otg_regs *regs) {
	return !(regs->host.h_fnum&1);
}

static void vbus_drive(struct usb_otg_regs *regs, unsigned int state) {
	int prtpwr;

	drive_vbus(state);
	
	prtpwr=read_hprt0(regs)&H_PRT_PPWR;
	if (state && !prtpwr) {
		regs->host.h_prt|=H_PRT_PPWR;
	} else {
		regs->host.h_prt&=~H_PRT_PPWR;
	}
	
	sys_mdelay(200);
}

static int host_enable_int(struct usb_dev_handle *pdev) {

	pdev->regs->core.g_int_msk=0;
	pdev->regs->core.g_int_sts= 0xffffffff;

	common_irq_enable(pdev->regs);

	if (!pdev->cfg.dma_enable) {
		pdev->regs->core.g_int_msk|= (G_INT_STS_RXFLVL|
						G_INT_STS_HPRTINT|
						G_INT_STS_HCINT|
						G_INT_STS_DISCINT|
						G_INT_STS_SOF|
						G_INT_STS_INCOMPISOOUT);
	} else {
		pdev->regs->core.g_int_msk|= (G_INT_STS_HPRTINT|
						G_INT_STS_HCINT|
						G_INT_STS_DISCINT|
						G_INT_STS_SOF|
						G_INT_STS_INCOMPISOOUT);
	}
	return 0;
}


static void init_fs_ls_p_clksel(struct usb_otg_regs *regs, unsigned int freq) {
	regs->host.h_cfg|=freq;
}

static unsigned int read_hprt0(struct usb_otg_regs *regs) {
	unsigned int hprt0=regs->host.h_prt;
	hprt0&=~(H_PRT_PENA|
		H_PRT_PCDET|
		H_PRT_PENCHNG|
		H_PRT_POCCHNG);
	return hprt0;
}

static unsigned int get_host_all_channel_irq(struct usb_otg_regs *regs) {
	return regs->host.h_aint;
}

static int reset_port(struct usb_otg_regs *regs) {
	unsigned int hprt0=read_hprt0(regs);

	regs->host.h_prt=hprt0|H_PRT_PRST;
	sys_mdelay(10);
	regs->host.h_prt=hprt0;
	sys_mdelay(29);
	return 1;
}

static int host_channel_init(struct usb_dev_handle *pdev, 
				unsigned int channel) {
	unsigned int intmask=0;
	unsigned int hcchar;

	pdev->regs->hc[channel].hc_int=0xffffffff;
	if (pdev->cfg.dma_enable) {
		intmask=HC_INT_AHBERR;
	}
	switch(pdev->host.fd[channel].hc.ep_type) {
		case EP_TYPE_CTRL:
		case EP_TYPE_BULK:
			intmask|=(HC_INT_XFERC|HC_INT_STALL|
				HC_INT_TXERR|HC_INT_DTERR|
				HC_INT_NAK);
			if (pdev->host.fd[channel].hc.ep_is_in) {
				intmask|=HC_INT_BBLERR;
			} else {
				intmask|=HC_INT_NYET;
				if (pdev->host.fd[channel].hc.do_ping) {
					intmask|=HC_INT_ACK;
				}
			}
			break;
		case EP_TYPE_IRQ:
			intmask|=(HC_INT_XFERC|HC_INT_NAK|
				HC_INT_STALL|HC_INT_TXERR|
				HC_INT_DTERR|HC_INT_FROVR);
			if (pdev->host.fd[channel].hc.ep_is_in) {
				intmask|=HC_INT_BBLERR;
			}
			break;
		case EP_TYPE_ISO:
			intmask|=(HC_INT_XFERC|HC_INT_FROVR|
				HC_INT_ACK);
			if(pdev->host.fd[channel].hc.ep_is_in) {
				intmask|=(HC_INT_TXERR|HC_INT_BBLERR);
			}
			break;
	}
	pdev->regs->hc[channel].hc_intmsk=intmask;
	pdev->regs->host.h_aint_msk|=(1<<channel);
	pdev->regs->core.g_int_msk|=G_INT_STS_HCINT;

	hcchar = (pdev->host.fd[channel].hc.dev_addr<<HC_CHAR_DEVADDR_SHIFT)|
		(pdev->host.fd[channel].hc.ep_num<<HC_CHAR_EPNUM_SHIFT)|
		(pdev->host.fd[channel].hc.ep_is_in?HC_CHAR_EPDIR_IN:0)|
		(pdev->host.fd[channel].hc.speed==HPRT_SPEED_LOW?HC_CHAR_LSDEV:0)|
		(pdev->host.fd[channel].hc.ep_type<<HC_CHAR_ETYPE_SHIFT)|
		(pdev->host.fd[channel].hc.max_pkt_size<<HC_CHAR_MPSIZ_SHIFT);

	if (pdev->host.fd[channel].hc.ep_type==EP_TYPE_IRQ) {
		hcchar|=HC_CHAR_ODDFRM;
	}
	pdev->regs->hc[channel].hc_char=hcchar;
	return 0;
}


static int host_channel_start_tx(struct usb_dev_handle *pdev,
				unsigned int channel) {
	unsigned int len_w=0;
	unsigned int num_packets;
	unsigned int max_pkt_count=256;
	unsigned long int hcchar;

	if (pdev->host.fd[channel].hc.xfer_len>0) {
		num_packets=(pdev->host.fd[channel].hc.xfer_len /
			pdev->host.fd[channel].hc.max_pkt_size)+1; 

		if (num_packets > max_pkt_count) {
			num_packets=max_pkt_count;
			pdev->host.fd[channel].hc.xfer_len = num_packets *
				pdev->host.fd[channel].hc.max_pkt_size;
		}
	} else {
		num_packets = 1;
	}

	pdev->regs->hc[channel].hc_tsiz=
		(pdev->host.fd[channel].hc.xfer_len<<HC_TSIZ_XFRSIZ_SHIFT)|
		(num_packets<<HC_TSIZ_PKTCNT_SHIFT)|
		(pdev->host.fd[channel].hc.data_pid<<HC_TSIZ_DPID_SHIFT);

	if (pdev->cfg.dma_enable) {
		pdev->regs->hc[channel].hc_dma=
		(unsigned long int)pdev->host.fd[channel].hc.xfer_buf;
	}

	hcchar=pdev->regs->hc[channel].hc_char;
	
	if (isEvenFrame(pdev->regs)) {
		hcchar|=HC_CHAR_ODDFRM;
	} else {
		hcchar&=~HC_CHAR_ODDFRM;
	}
	hcchar&=~HC_CHAR_CHDIS;
	hcchar|=HC_CHAR_CHENA;
	pdev->regs->hc[channel].hc_char=hcchar;

	if (!pdev->cfg.dma_enable) {
		if ((!pdev->host.fd[channel].hc.ep_is_in) &&
			(pdev->host.fd[channel].hc.xfer_len>0)) {
			switch(pdev->host.fd[channel].hc.ep_type) {
				case EP_TYPE_CTRL:
				case EP_TYPE_BULK: {
					unsigned int hnptx_stat=pdev->regs->core.g_nptxsts;
					len_w=(pdev->host.fd[channel].hc.xfer_len+3)/4;

					if (len_w>(hnptx_stat&G_NPTXSTAT_FSAV_MSK)) {
						pdev->regs->core.g_int_msk|=G_INT_STS_NPTXFE;
					}
					break;
				}
				case EP_TYPE_IRQ:
				case EP_TYPE_ISO: {
					unsigned int hptxstat=pdev->regs->host.h_ptxsts;
					len_w=(pdev->host.fd[channel].hc.xfer_len+3)/4;
					if (len_w>(hptxstat&H_PTXSTS_FSAV_MSK)) {
						pdev->regs->core.g_int_msk|=G_INT_STS_PTXFE;
					}
					break;
				}
				default:
					break;
			}
			write_pkt(pdev,channel,pdev->host.fd[channel].hc.xfer_buf,
					pdev->host.fd[channel].hc.xfer_len);
		}
	}
	return 0;
}

static int host_channel_halt(struct usb_otg_regs *regs, unsigned int channel) {
	unsigned int hcchar;

	hcchar=regs->hc[channel].hc_char;
	if ((hcchar&HC_CHAR_ETYPE_CTRL)||
		(hcchar&HC_CHAR_ETYPE_BULK)) {
		if (!(regs->core.g_nptxsts&G_NPTXSTAT_QSAV_MSK)) {
			regs->hc[channel].hc_char&=~HC_CHAR_CHENA;
		}
	} else {
		if(!(regs->host.h_ptxsts&H_PTXSTS_QSAV_MSK)) {
			regs->hc[channel].hc_char&=~HC_CHAR_CHENA;
		}
	}
	return 0;
}

static int host_channel_do_ping(struct usb_otg_regs *regs,unsigned int channel) {
	unsigned int hcchar;

	regs->hc[channel].hc_tsiz=(HC_TSIZ_DOPING|(1<<HC_TSIZ_PKTCNT_SHIFT));
	hcchar=regs->hc[channel].hc_char;
	hcchar|=HC_CHAR_CHENA;
	hcchar&=~HC_CHAR_CHDIS;
	regs->hc[channel].hc_tsiz=hcchar;
	return 0;
}

static void host_stop(struct usb_dev_handle *pdev) {
	unsigned int i;

	pdev->regs->host.h_aint_msk=0;
	pdev->regs->host.h_aint=0xffffffff;

	for(i=0;i<pdev->cfg.host_channels;i++) {
		unsigned int hcchar;
		hcchar=pdev->regs->hc[i].hc_char;
		hcchar&=~(HC_CHAR_CHENA|HC_CHAR_EPDIR_IN);
		hcchar|=HC_CHAR_CHDIS;
		pdev->regs->hc[i].hc_char=hcchar;
	}
	usb_core_flush_rx_fifo(pdev->regs);
	usb_core_flush_tx_fifo(pdev->regs, 0x10);
}


/*************** as Device  ***********************/


static void dev_speed_init(struct usb_otg_regs *regs, unsigned int speed) {
	unsigned int dcfg;

	dcfg=regs->dev.d_cfg&~DCFG_DEV_SPEED_MSK;
	regs->dev.d_cfg=dcfg|speed;
}

int usb_core_dev_core_init(struct usb_dev_handle *pdev) {
	struct usb_otg_regs *regs=pdev->regs;
	struct usb_cfg	*cfg=&pdev->cfg;
	unsigned int i;
	unsigned int dcfg;
	unsigned int tx_fifo_addr;
	
	regs->pwrclk=0;
	dcfg=regs->dev.d_cfg&~DCFG_FINTRVL_MSK;
	regs->dev.d_cfg=dcfg|DCFG_FINTRVL_80;

	if (regs==USB_OTG_FS_ADDR) {
		unsigned int tx_fifo_size[] = {TX1_FIFO_FS_SIZE,
					TX2_FIFO_FS_SIZE,
					TX3_FIFO_FS_SIZE};

		dev_speed_init(regs, DCFG_DEV_SPEED_FS_FULL);
		regs->core.g_rx_fsiz=RX_FIFO_FS_SIZE;

		regs->core.g_nptxfcnf=
			(TX0_FIFO_FS_SIZE<<G_NPTX_FCNF_D_SHIFT)|
			(RX_FIFO_FS_SIZE<<G_NPTX_FCNF_A_SHIFT);

			tx_fifo_addr=RX_FIFO_FS_SIZE+TX0_FIFO_FS_SIZE;
	
		for(i=0;i<3;i++) {
			unsigned int txfifo_data=
				(tx_fifo_size[i]<<PTX_FCNF_D_SHIFT)|
				   	tx_fifo_addr;
			tx_fifo_addr+=tx_fifo_size[i];
			regs->core.dieptxf[i]=txfifo_data;
		}

	} else if (regs==USB_OTG_HS_ADDR) {
		unsigned int tx_fifo_size[] = {TX1_FIFO_HS_SIZE,
					TX2_FIFO_HS_SIZE,
					TX3_FIFO_HS_SIZE,
					TX4_FIFO_HS_SIZE,
					TX5_FIFO_HS_SIZE};
		dev_speed_init(regs, DCFG_DEV_SPEED_FULL_IPHY);
	
		regs->core.g_rx_fsiz=RX_FIFO_HS_SIZE;

		regs->core.g_nptxfcnf=
			(TX0_FIFO_HS_SIZE<<G_NPTX_FCNF_D_SHIFT)|
			(RX_FIFO_HS_SIZE<<G_NPTX_FCNF_A_SHIFT);

		tx_fifo_addr=RX_FIFO_HS_SIZE+TX0_FIFO_HS_SIZE;
	
		for(i=0;i<5;i++) {
			unsigned int txfifo_data=
				(tx_fifo_size[i]<<PTX_FCNF_D_SHIFT)|
				   	tx_fifo_addr;
			tx_fifo_addr+=tx_fifo_size[i];
			regs->core.dieptxf[i]=txfifo_data;
		}
	}

	usb_core_flush_tx_fifo(regs, 0x10);
	usb_core_flush_rx_fifo(regs);

	regs->dev.d_diep_msk=0;
	regs->dev.d_doep_msk=0;
	regs->dev.d_aint=0xffffffff;
	regs->dev.d_aintmsk=0;

	for(i=0;i<cfg->dev_endpoints;i++) {
		unsigned int depctl;
		depctl=regs->dev.d_ep_in[i].ctl;
		if (depctl&EP_CTL_ENA) {
			depctl=EP_CTL_DIS|EP_CTL_SNAK;
		} else {
			depctl=0;
		}
		regs->dev.d_ep_in[i].ctl=depctl;
		regs->dev.d_ep_in[i].tsiz=0;
		regs->dev.d_ep_in[i].ints=0xff;
	}

	for(i=0;i<cfg->dev_endpoints;i++) {
		unsigned int depctl;
		depctl=regs->dev.d_ep_out[i].ctl;
		if (depctl&EP_CTL_ENA) {
			depctl=EP_CTL_DIS|EP_CTL_SNAK;
		} else {
			depctl=0;
		}
		regs->dev.d_ep_out[i].ctl=depctl;
		regs->dev.d_ep_out[i].tsiz=0;
		regs->dev.d_ep_out[i].ints=0xff;
	}

	regs->dev.d_diep_msk|=DDIEPMSK_TXFUR;

	if (cfg->dma_enable) {
		unsigned int dthrctl=
			(DTHRCTL_NONISOTHREN|
			DTHRCTL_ISOTHREN|
			(64<<DTHRCTL_TXTHRLEN_SHIFT)|
			DTHRCTL_RXTHREN|
			(64<<DTHRCTL_RXTHRLEN_SHIFT));
		regs->dev.d_dthr_ctl=dthrctl;
	}
	dev_enable_int(pdev);
	return 0;
}

static int dev_enable_int(struct usb_dev_handle *pdev) {
	struct usb_otg_regs *regs=pdev->regs;
	struct usb_cfg	*cfg=&pdev->cfg;	
	unsigned int intmsk=0;

	regs->core.g_int_msk=0;
	regs->core.g_int_sts=0xbfffffff;

	common_irq_enable(regs);

	if (!cfg->dma_enable) {
		intmsk=G_INT_STS_RXFLVL;
	}

	intmsk|=(G_INT_STS_USBSUSP|G_INT_STS_USBRST|
		G_INT_STS_ENUMDNE|G_INT_STS_IEPINT|
		G_INT_STS_OEPINT|G_INT_STS_SOF|
		G_INT_STS_IISOIXFR|G_INT_STS_INCOMPISOOUT|
		G_INT_STS_SRQINT|G_INT_STS_OTGINT);

	regs->core.g_int_msk|=intmsk;
	return 0;
}

static unsigned int dev_get_speed(struct usb_otg_regs *regs) {
	unsigned int enumspd;
	unsigned int speed=USB_SPEED_UNKNOWN;

	enumspd=(regs->dev.d_sts&DSTS_ENUMSPD_MSK)>>DSTS_ENUMSPD_SHIFT;
	
	switch(enumspd) {
		case DSTS_ENUMSPD_HI:
			speed=USB_SPEED_HIGH;
			break;
		case DSTS_ENUMSPD_FULL:
			speed=USB_SPEED_FULL;
			break;
	}
	return speed;
}

static int dev_EP0_activate(struct usb_otg_regs *regs) {
//	unsigned int enumspd;
	unsigned int diepctl;

#if 0
	enumspd=(usb_regs->dev.d_sts&DSTS_ENUMSPD_MSK)>>DSTS_ENUMSPD_SHIFT;
	
	switch(enumspd) {
		case DSTS_ENUMSPD_HI:
	
#endif

	diepctl=regs->dev.d_ep_in[0].ctl;
	diepctl&=~EP_CTL_MPSIZ_MSK;
	/* Set Max packet size to 0 ?? */
	regs->dev.d_ep_in[0].ctl=diepctl;

	regs->dev.d_ctl|=DCTL_CGINAK;
	return 0;
}

int usb_core_dev_EP_activate(struct usb_otg_regs *regs, struct usb_dev_ep *ep) {
	volatile unsigned int *addr;
	unsigned int daintmsk=0;
	unsigned int depctl;

	if (ep->is_in) {
		addr=&regs->dev.d_ep_in[ep->num].ctl;
		daintmsk=1<<ep->num;
	} else {
		addr=&regs->dev.d_ep_out[ep->num].ctl;
		daintmsk=0x10000<<ep->num;
	}

	depctl=*addr;
	if (!(depctl&EP_CTL_USBAEP)) {
		depctl&=~(EP_CTL_MPSIZ_MSK|EP_CTL_EPTYPE_MSK|
			EP_CTL_TXFNUM_MSK);
		depctl|=((ep->maxpacket<<EP_CTL_MPSIZ_SHIFT)|
			(ep->type<<EP_CTL_EPTYPE_SHIFT)|
			(ep->tx_fifo_num<<EP_CTL_TXFNUM_SHIFT)|
			EP_CTL_SD0PID|
			EP_CTL_USBAEP);
		*addr=depctl;
	}

	regs->dev.d_aintmsk|=daintmsk;
	return 0;
}

int usb_core_dev_EP_deactivate(struct usb_otg_regs *regs, struct usb_dev_ep *ep) {
	volatile unsigned int *addr;
	unsigned int daintmsk=0;

	if (ep->is_in) {
		addr=&regs->dev.d_ep_in[ep->num].ctl;
		daintmsk=1<<ep->num;
	} else {
		addr=&regs->dev.d_ep_out[ep->num].ctl;
		daintmsk=0x10000<<ep->num;
	}

	(*addr)&=~EP_CTL_USBAEP;

	regs->dev.d_aintmsk&=~daintmsk;
	return 0;
}

int usb_core_dev_EP_start_xfer(struct usb_dev_handle *pdev, struct usb_dev_ep *ep) {
	unsigned int depctl;
	unsigned int deptsiz;
	unsigned int dsts;

	if (ep->is_in) {
		depctl=pdev->regs->dev.d_ep_in[ep->num].ctl;
		deptsiz=pdev->regs->dev.d_ep_in[ep->num].tsiz;
		
		deptsiz&=~(EP_TSIZ_PKTCNT_MSK|EP_TSIZ_XFRSIZ_MSK);
		if (!ep->xfer_len) {
			deptsiz|=(1<<EP_TSIZ_PKTCNT_SHIFT);
		} else {
			deptsiz|=((ep->xfer_len<<EP_TSIZ_XFRSIZ_SHIFT)|
			(((ep->xfer_len-1+ep->maxpacket)/ep->maxpacket)<<EP_TSIZ_PKTCNT_SHIFT));

			if (ep->type==EP_TYPE_ISO) {
				deptsiz&=~EP_TSIZ_MCNT_MSK;
				deptsiz|=(1<<EP_TSIZ_MCNT_SHIFT);
			}
		}
		pdev->regs->dev.d_ep_in[ep->num].tsiz=deptsiz;

		if (pdev->cfg.dma_enable) {
			pdev->regs->dev.d_ep_in[ep->num].dma=ep->dma_addr;
		} else {
			if (ep->type!=EP_TYPE_ISO) {
				if (ep->xfer_len>0) {
					pdev->regs->dev.d_iep_empty_msk|=(1<<ep->num);
				}
			}
		}

		if (ep->type==EP_TYPE_ISO) {
			dsts=pdev->regs->dev.d_sts;
			if ((dsts>>DSTS_FNSOF_SHIFT)&1) {
				depctl|=EP_CTL_SD0PID;
			} else {
				depctl|=EP_CTL_SD1PID;
			}
		}

		depctl|=(EP_CTL_CNAK|EP_CTL_ENA);
		
		pdev->regs->dev.d_ep_in[ep->num].ctl=depctl;

		if (ep->type==EP_TYPE_ISO) {
			write_pkt(pdev, ep->num, ep->xfer_buf, ep->xfer_len);
		}
	} else {  /* out end point */
		depctl=pdev->regs->dev.d_ep_out[ep->num].ctl;
		deptsiz=pdev->regs->dev.d_ep_out[ep->num].tsiz;
		
		deptsiz&=~(EP_TSIZ_PKTCNT_MSK|EP_TSIZ_XFRSIZ_MSK);
		if (!ep->xfer_len) {
			deptsiz|=((ep->maxpacket<<EP_TSIZ_XFRSIZ_SHIFT)|
				(1<<EP_TSIZ_PKTCNT_SHIFT));
		} else {
			unsigned int pcnt=(ep->xfer_len+(ep->maxpacket-1))/
					ep->maxpacket;
			deptsiz|=(((pcnt*ep->maxpacket)<<EP_TSIZ_XFRSIZ_SHIFT)|
					(pcnt<<EP_TSIZ_PKTCNT_SHIFT));
		}
		pdev->regs->dev.d_ep_out[ep->num].tsiz=deptsiz;

		if (pdev->cfg.dma_enable) {
			pdev->regs->dev.d_ep_out[ep->num].dma=ep->dma_addr;
		}

		if (ep->type==EP_TYPE_ISO) {
			if (ep->even_odd_frame) {
				depctl|=EP_CTL_SD1PID;
			} else {
				depctl|=EP_CTL_SD0PID;
			}
		}

		depctl|=(EP_CTL_CNAK|EP_CTL_ENA);
		pdev->regs->dev.d_ep_out[ep->num].ctl=depctl;
	}
	return 0;
}

int usb_core_dev_EP0_start_xfer(struct usb_dev_handle *pdev, struct usb_dev_ep *ep) {
	unsigned int depctl;
	unsigned int deptsiz;

	if (ep->is_in) {
		struct d_ep_in *in_ep_regs=&pdev->regs->dev.d_ep_in[0];
		depctl=in_ep_regs->ctl;
		deptsiz=in_ep_regs->tsiz;
		
		deptsiz&=~(EP_TSIZ_PKTCNT_MSK|EP_TSIZ_XFRSIZ_MSK);
		if (!ep->xfer_len) {
			deptsiz|=(1<<EP_TSIZ_PKTCNT_SHIFT);
		} else {
			if (ep->xfer_len>ep->maxpacket) {
				ep->xfer_len=ep->maxpacket;
			}
			deptsiz|=((ep->xfer_len<<EP_TSIZ_XFRSIZ_SHIFT)|
				(1<<EP_TSIZ_PKTCNT_SHIFT));
		}
		in_ep_regs->tsiz=deptsiz;
		
		if (pdev->cfg.dma_enable) {
			pdev->regs->dev.d_ep_in[ep->num].dma=ep->dma_addr;
//			in_ep_regs->dma=ep->dma_addr;
		}

		depctl|=(EP_CTL_CNAK|EP_CTL_ENA);
		
		in_ep_regs->ctl=depctl;

		if (!pdev->cfg.dma_enable) {
			if (ep->xfer_len>0) {
				pdev->regs->dev.d_iep_empty_msk|=(1<<ep->num);
			}
		}
	} else {  /* out end point */
		struct d_ep_out *ep_out_regs=&pdev->regs->dev.d_ep_out[ep->num];
		depctl=ep_out_regs->ctl;
		deptsiz=ep_out_regs->tsiz;
		
		deptsiz&=~(EP_TSIZ_PKTCNT_MSK|EP_TSIZ_XFRSIZ_MSK);
		if (ep->xfer_len) {
			ep->xfer_len=ep->maxpacket;
		}
		deptsiz|=((ep->maxpacket<<EP_TSIZ_XFRSIZ_SHIFT)|
					(1<<EP_TSIZ_PKTCNT_SHIFT));

		ep_out_regs->tsiz=deptsiz;

		if (pdev->cfg.dma_enable) {
			ep_out_regs->dma=ep->dma_addr;
		}

		depctl|=(EP_CTL_CNAK|EP_CTL_ENA);
		ep_out_regs->ctl=depctl;
	}
	return 0;
}

int usb_core_dev_EP_set_stall(struct usb_otg_regs *regs,struct usb_dev_ep *ep) {
	
	if (ep->is_in) {
		struct d_ep_in *ep_in_regs=&regs->dev.d_ep_in[ep->num];
		unsigned int depctl=ep_in_regs->ctl;
	
		if (depctl&EP_CTL_ENA) {
			depctl|=EP_CTL_DIS;
		}
		depctl|=EP_CTL_STALL;
		ep_in_regs->ctl=depctl;
	} else {
		struct d_ep_out *ep_out_regs=&regs->dev.d_ep_out[ep->num];
		unsigned int depctl=ep_out_regs->ctl;
		depctl|=EP_CTL_STALL;
		ep_out_regs->ctl=depctl;
	}
	return 0;
}

int usb_core_dev_EP_clr_stall(struct usb_otg_regs *regs,struct usb_dev_ep *ep) {
	volatile unsigned int *ctl;
	unsigned int depctl;

	if (ep->is_in) {
		struct d_ep_in *ep_in_regs=&regs->dev.d_ep_in[ep->num];
		ctl=&ep_in_regs->ctl;
	} else {
		struct d_ep_out *ep_out_regs=&regs->dev.d_ep_out[ep->num];
		ctl=&ep_out_regs->ctl;
	}
	depctl=*ctl;

	depctl&=~EP_CTL_STALL;

	if (ep->type==EP_TYPE_IRQ||ep->type==EP_TYPE_BULK) {
		depctl|=EP_CTL_SD0PID;
	}

	*ctl=depctl;

	return 0;
}

static unsigned int dev_read_all_out_ep_irq(struct usb_otg_regs *regs) {
	unsigned int irqs;
	irqs=regs->dev.d_aint;
	irqs&=regs->dev.d_aintmsk;
	return (irqs>>16);
}

static unsigned int dev_read_out_ep_irq(struct usb_otg_regs *regs, unsigned int channel) {
	unsigned int irqs;
	irqs=regs->dev.d_ep_out[channel].ints;
	irqs&=regs->dev.d_doep_msk;
	return irqs;
}	

static unsigned int dev_read_all_in_ep_irq(struct usb_otg_regs *regs) {
	unsigned int irqs;
	irqs=regs->dev.d_aint;
	irqs&=regs->dev.d_aintmsk;
	return (irqs&0xffff);
}

static unsigned int dev_read_in_ep_irq(struct usb_otg_regs *regs,unsigned int channel) {
	unsigned int mask=regs->dev.d_diep_msk;
	unsigned int emptymask=regs->dev.d_iep_empty_msk;
	mask|= ((emptymask>>channel)&1)<<7;
	return regs->dev.d_ep_in[channel].ints&mask;
}

void dev_EP0_out_start(struct usb_dev_handle *pdev) {
	unsigned int doeptsiz= (3<<EP_TSIZ_SUPCNT_SHIFT)|
				(1<<EP_TSIZ_PKTCNT_SHIFT)|
				((8*3)<<EP_TSIZ_XFRSIZ_SHIFT);
	pdev->regs->dev.d_ep_out[0].tsiz=doeptsiz;
	
	if (pdev->cfg.dma_enable) {
		unsigned int doepctl;
		pdev->regs->dev.d_ep_out[0].dma=(unsigned int)
						pdev->dev.setup_packet;

		doepctl=EP_CTL_ENA|EP_CTL_USBAEP;
//		pdev->regs->dev.d_ep_out[0].ctl|=doepctl;
		pdev->regs->dev.d_ep_out[0].ctl=doepctl;
	}
}

void dev_remote_wakeup(struct usb_dev_handle *pdev) {
	pdev->dev.remote_wakeup=1;  // FixME
	if (pdev->dev.remote_wakeup) {
		unsigned int dsts = pdev->regs->dev.d_sts;
		if (dsts&DSTS_SUSP) {
			if (pdev->cfg.low_power) {
				unsigned int pwrclk=pdev->regs->pwrclk;
				pwrclk=~(PWRCLK_GATECLK|PWRCLK_STOPPCLK);
				pdev->regs->pwrclk=pwrclk;
			}
			pdev->regs->dev.d_ctl|=DCTL_RWUSIG;
			sys_mdelay(2);
			pdev->regs->dev.d_ctl&=~DCTL_RWUSIG;
		}
	}
}

static void dev_ungate_clock(struct usb_dev_handle *pdev) {
	if (pdev->cfg.low_power) {
		if (pdev->regs->dev.d_sts&DSTS_SUSP) {
			unsigned int pwrclk=pdev->regs->pwrclk;
			pwrclk=~(PWRCLK_GATECLK|PWRCLK_STOPPCLK);
			pdev->regs->pwrclk=pwrclk;
		}
	}
}

static void dev_stop(struct usb_dev_handle *pdev) {
	unsigned int i;
	pdev->dev.dev_status=1;	

	for(i=0;i<pdev->cfg.dev_endpoints;i++) {
		pdev->regs->dev.d_ep_in[i].ints=0xff;
		pdev->regs->dev.d_ep_out[i].ints=0xff;
	}

	pdev->regs->dev.d_diep_msk=0;
	pdev->regs->dev.d_doep_msk=0;
	pdev->regs->dev.d_aintmsk=0;
	pdev->regs->dev.d_aint=0xffffffff;

	usb_core_flush_rx_fifo(pdev->regs);
	usb_core_flush_tx_fifo(pdev->regs,0x10);
}

unsigned int usb_core_dev_get_ep_status(struct usb_otg_regs *regs, struct usb_dev_ep *ep) {
	unsigned int status=0;
	
	if (ep->is_in) {
		unsigned int depctl=regs->dev.d_ep_in[ep->num].ctl;
		if (depctl&EP_CTL_STALL) {
			status=EP_TX_STALL;
		} else if (depctl&EP_CTL_NAKSTS) {
			status=EP_TX_NAK;
		} else {
			status=EP_TX_VALID;
		}
	} else {
		unsigned int depctl=regs->dev.d_ep_out[ep->num].ctl;
		if (depctl&EP_CTL_STALL) {
			status=EP_RX_STALL;
		} else  if (depctl&EP_CTL_NAKSTS) {
			status=EP_RX_NAK;
		} else {
			status=EP_RX_VALID;
		}
	}
	return status;
}

void usb_core_dev_set_ep_status(struct usb_otg_regs *regs,
			struct usb_dev_ep *ep, unsigned int status) {
	volatile unsigned int *depctl_ptr=0;
	unsigned int depctl;

	if (ep->is_in) {
		depctl_ptr=&regs->dev.d_ep_in[ep->num].ctl;
		depctl=*depctl_ptr;
		if (status==EP_TX_STALL) {
			usb_core_dev_EP_set_stall(regs,ep);
			return;
		} else if (status==EP_TX_NAK) {
			regs->dev.d_ep_in[ep->num].ctl|=EP_CTL_SNAK;
		} else if (status==EP_TX_VALID) {
			if (depctl&EP_CTL_STALL) {
				ep->even_odd_frame=0;
				usb_core_dev_EP_clr_stall(regs,ep);
				return;
			}
			depctl|=(EP_CTL_CNAK|EP_CTL_USBAEP|EP_CTL_ENA);
		} else if (status==EP_TX_DIS) {
			depctl&=~EP_CTL_USBAEP;
		}
	} else {
		depctl_ptr=&regs->dev.d_ep_out[ep->num].ctl;
		depctl=*depctl_ptr;

		if (status==EP_RX_STALL) {
			depctl|=EP_CTL_STALL;
		} else if (status==EP_RX_NAK) {
			depctl|=EP_CTL_SNAK;
		} else if (status==EP_RX_VALID) {
			if (depctl&EP_CTL_STALL) {
				ep->even_odd_frame=0;
				usb_core_dev_EP_clr_stall(regs,ep);
				return;
			}
			depctl|=(EP_CTL_CNAK|EP_CTL_USBAEP|EP_CTL_ENA);
		} else if (status==EP_RX_DIS) {
			depctl&=~EP_CTL_USBAEP;
		}
	}
	(*depctl_ptr)=depctl;
}

void usb_core_dev_set_address(struct usb_otg_regs *regs, unsigned char address) {
	unsigned int dcfg=regs->dev.d_cfg;
	dcfg&=~DCFG_DADDR_MSK;
	dcfg|=(address<<DCFG_DADDR_SHIFT);
	regs->dev.d_cfg=dcfg;
}

void usb_core_dev_connect(struct usb_otg_regs *regs) {
	regs->dev.d_ctl&=~DCTL_SDIS;
	sys_mdelay(2);
}

void usb_core_dev_disconnect(struct usb_otg_regs *regs) {
	regs->dev.d_ctl|=DCTL_SDIS;
	sys_mdelay(2);
}


/*********************************************************** IRQ **********************************/

static unsigned int dev_write_empty_tx_fifo(struct usb_dev_handle *pdev,
						unsigned int epnum) {
	struct usb_dev_ep *ep;
	unsigned int len,wlen;
	unsigned int space_avail;

	ep=&pdev->dev.in_ep[epnum];
	len=ep->xfer_len-ep->xfer_count;

	if (len>ep->maxpacket) {
		len=ep->maxpacket;
	}

	wlen=(len+3)/4;
	space_avail=pdev->regs->dev.d_ep_in[epnum].txf_sts&EP_IN_TXF_STS_INEPTFSAV_MSK;

	while ((space_avail>wlen) &&
		ep->xfer_count<ep->xfer_len &&
		(ep->xfer_len!=0)) {

		len=ep->xfer_len-ep->xfer_count;
		if (len>ep->maxpacket) {
			len=ep->maxpacket;
		}

		wlen=(len+3)/4;

		write_pkt(pdev, epnum,ep->xfer_buf,len);
		ep->xfer_buf+=len;
		ep->xfer_count+=len;

//		if (ep->xfer_count>=ep->xfer_len) {
//			/* mask out ep_empty irq, we dont need it for now*/
//			pdev->regs->dev.d_iep_empty_msk&=~(1<<ep->num);
//		}
		
		space_avail=pdev->regs->dev.d_ep_in[epnum].txf_sts&EP_IN_TXF_STS_INEPTFSAV_MSK;
	}
	return 1;
}

#ifdef DEDICATED_EP1

unsigned int usb_device_EP1_out_handler() {
	unsigned int d_doepint;

	d_doepint=usb_regs->dev.d_ep_out[1].ints;
	d_doepint&=usb_regs->dev.d_oep_each_msk1;

	if (d_doepint&EP_OINT_XFER_DONE) {
		usb_regs->dev.d_ep_out[1].ints=EP_OINT_XFER_DONE;
		if (usb_cfg->dma_enable) {
			unsigned int xfersize=
				usb_regs->dev.d_ep_out[1].tsiz&
					EP_TSIZ_XFRSIZ_MSK;
			usb_dev->out_ep[1].xfer_count=
				usb_dev->out_ep[1].maxpacket - xfersize;
		}
		usb_device_cb->data_out(1);
	}

	if (d_doepint&EP_OINT_DISABLED) {
		usb_regs->dev.d_ep_out[1].ints=EP_OINT_DISABLED;
	}
	return 1;
}


unsigned int usb_device_EP1_in_handler() {
	unsigned int d_diepint;
	unsigned int fifoemptymsk, msk, empty;
	
	msk=usb_regs->dev.d_iep_each_msk1;
	empty=usb_regs->dev.d_iep_empty_msk;
	msk|=(((empty>>1)&1)<<7); // use reserved bit to indicate empty 
	d_diepint=usb_regs->dev.d_ep_in[1].ints & msk;

	if (d_diepint&EP_IINT_XFER_DONE) {
		fifoemptymsk=0x1<<1;
		usb_regs->dev.d_iep_empty_msk&=~fifoemptymsk;
		usb_regs->dev.d_ep_in[1].ints=EP_IINT_XFER_DONE;
		usb_device_cb->data_in(1);
	}

	if (d_diepint&EP_IINT_DISABLED) {
		usb_regs->dev.d_ep_in[1].ints=EP_IINT_DISABLED;
	}

	if (d_diepint&EP_IINT_TIMEOUT) {
		usb_regs->dev.d_ep_in[1].ints=EP_IINT_TIMEOUT;
	}

	if (d_diepint&EP_IINT_ITTXFE) {
		usb_regs->dev.d_ep_in[1].ints=EP_IINT_ITTXFE;
	}

	if (d_diepint&EP_IINT_INEPNE) {
		usb_regs->dev.d_ep_in[1].ints=EP_IINT_INEPNE;
	}

	if (d_diepint&EP_IINT_TXFE) {
		dev_write_empty_tx_fifo(1);
		usb_regs->dev.d_ep_in[1].ints=EP_IINT_TXFE;
	}
	return 1;
}

#endif

static unsigned int device_handle_resume_irq(struct usb_dev_handle *pdev) {

	if (pdev->cfg.low_power) {
		unsigned int pwrclk=pdev->regs->pwrclk;
		pwrclk=~(PWRCLK_GATECLK|PWRCLK_STOPPCLK);
		pdev->regs->pwrclk=pwrclk;
	}
	pdev->regs->dev.d_ctl&=~DCTL_RWUSIG;

	usbd_resume(pdev);

	pdev->regs->core.g_int_sts=G_INT_STS_WKUIN;

	return 1;
}

static unsigned int device_handle_usb_suspend_irq(struct usb_dev_handle *pdev) {
	unsigned int prev_stat;
	unsigned int dev_stat;

	prev_stat=pdev->dev.dev_status;
	usbd_suspend(pdev);

	dev_stat=pdev->regs->dev.d_sts;
	
	pdev->regs->core.g_int_sts=G_INT_STS_USBSUSP;

	if (pdev->cfg.low_power&&(dev_stat&DSTS_SUSP)&&
		(pdev->dev.conn_status==1) &&
		(prev_stat == USB_DEV_STAT_CONFIGURED)) {
		pdev->regs->pwrclk|=PWRCLK_STOPPCLK;
		pdev->regs->pwrclk|=PWRCLK_GATECLK;
		// ask OS to bring cpu to sleep
	}
	return 1;
}

static unsigned int device_handle_inep_irq(struct usb_dev_handle *pdev) {
	unsigned int ep_intr;
	unsigned int epnum=0;
	unsigned int fifoemptymsk=0;

	ep_intr=dev_read_all_in_ep_irq(pdev->regs);
	while(ep_intr) {
		if (ep_intr&0x1) {
			unsigned int diepint=dev_read_in_ep_irq(pdev->regs,epnum);
			if (diepint&EP_IINT_XFER_DONE) {
//				sys_printf("in ep irq ep %d, XFER_DONE\n",epnum);
				fifoemptymsk=1<<epnum;
				pdev->regs->dev.d_iep_empty_msk&=~fifoemptymsk;
				pdev->regs->dev.d_ep_in[epnum].ints=EP_IINT_XFER_DONE;
				usbd_data_in(pdev,epnum);

				if (pdev->cfg.dma_enable) {
					if ((epnum==0)&&(pdev->dev.dev_state==USB_DEV_EP0_STATUS_IN)) {
						dev_EP0_out_start(pdev);
					}
				}
			}

			if (diepint&EP_IINT_TIMEOUT) {
				sys_printf("iep timeout ep %d\n",epnum);
				pdev->regs->dev.d_ep_in[epnum].ints=EP_IINT_TIMEOUT;
			}

			if (diepint&EP_IINT_ITTXFE) {
				sys_printf("iep ittxfe ep %d\n",epnum);
				pdev->regs->dev.d_ep_in[epnum].ints=EP_IINT_ITTXFE;
			}

			if (diepint&EP_IINT_INEPNE) {
				sys_printf("iep inepne ep %d\n",epnum);
				pdev->regs->dev.d_ep_in[epnum].ints=EP_IINT_INEPNE;
			}

			if (diepint&EP_IINT_DISABLED) {
				sys_printf("iep disable ep %d\n",epnum);
				pdev->regs->dev.d_ep_in[epnum].ints=EP_IINT_DISABLED;
			}

			if (diepint&EP_IINT_TXFE) {
//				sys_printf("in ep irq ep %d, TXFE\n",epnum);
				dev_write_empty_tx_fifo(pdev, epnum);
				pdev->regs->dev.d_ep_in[epnum].ints=EP_IINT_TXFE;
			}
		}
		epnum++;
		ep_intr >>= 1;
	}
	return 1;
}

static unsigned int device_handle_outep_irq(struct usb_dev_handle *pdev) {
	unsigned int ep_intr;
	unsigned int epnum=0;

	ep_intr=dev_read_all_out_ep_irq(pdev->regs);
	
	while(ep_intr) {
		if (ep_intr&0x1) {
			unsigned int doepint=dev_read_out_ep_irq(pdev->regs,epnum);
			if (doepint&EP_OINT_XFER_DONE) {
//				sys_printf("out ep irq ep %d, XFER_DONE\n",epnum);
				pdev->regs->dev.d_ep_out[epnum].ints=EP_OINT_XFER_DONE;
				if (pdev->cfg.dma_enable) {
					unsigned int deptsiz;
					deptsiz=pdev->regs->dev.d_ep_out[epnum].tsiz;
					pdev->dev.out_ep[epnum].xfer_count = 
						pdev->dev.out_ep[epnum].maxpacket -
						(deptsiz&EP_TSIZ_XFRSIZ_MSK);
				}
				usbd_data_out(pdev,epnum);
				if (pdev->cfg.dma_enable) {
					if ((epnum==0)&&(pdev->dev.dev_state==USB_DEV_EP0_STATUS_OUT)) {
						dev_EP0_out_start(pdev);
					}
				}
			}
			
			if (doepint&EP_OINT_DISABLED) {
				pdev->regs->dev.d_ep_out[epnum].ints=EP_OINT_DISABLED;
//				sys_printf("out ep irq ep %d, DISABLED\n",epnum);
			}

			if (doepint&EP_OINT_SETUP_DONE) {
				usbd_setup(pdev);
				pdev->regs->dev.d_ep_out[epnum].ints=EP_OINT_SETUP_DONE;
//				sys_printf("out ep irq ep %d, SETUP_DONE\n",epnum);
			}
		}
		epnum++;
		ep_intr>>=1;
	}
	return 1;
}

static unsigned int device_handle_sof_irq(struct usb_dev_handle *pdev) {
	usbd_sof(pdev);
	pdev->regs->core.g_int_sts=G_INT_STS_SOF;
	return 1;
}

static unsigned int device_handle_rxfifo_irq(struct usb_dev_handle *pdev) {
	unsigned int status;
	struct usb_dev_ep *ep;

	pdev->regs->core.g_int_msk&=~G_INT_STS_RXFLVL;
	status=pdev->regs->core.g_rx_sts_p;

	ep=&pdev->dev.out_ep[status&G_RX_STS_EPNUM_MSK];

	switch((status&G_RX_STS_PKTSTAT_MSK)>>G_RX_STS_PKTSTAT_SHIFT) {
		case G_RX_STS_PKTSTAT_GOUT_NAK:
			break;
		case G_RX_STS_PKTSTAT_ODATA_RECEIVED: {
			unsigned int bcnt=(status&G_RX_STS_BCNT_MSK);
			if (bcnt) {
				bcnt>>=G_RX_STS_BCNT_SHIFT;
				read_pkt(pdev,ep->xfer_buf,bcnt);
				ep->xfer_buf+=bcnt;
				ep->xfer_count+=bcnt;
			}
			break;
		}
		case G_RX_STS_PKTSTAT_OXFER_DONE:
			break;
		case G_RX_STS_PKTSTAT_SXFER_DONE:
			break;
		case G_RX_STS_PKTSTAT_SDATA_RECEIVED: {
			unsigned int bcnt=(status&G_RX_STS_BCNT_MSK)>>G_RX_STS_BCNT_SHIFT;
			read_pkt(pdev,pdev->dev.setup_packet, 8);
			ep->xfer_count += bcnt;
			break;
		}
		default:
			break;
	}
	pdev->regs->core.g_int_msk|=G_INT_STS_RXFLVL;
	return 1;
}

unsigned int device_handle_usb_reset_irq(struct usb_dev_handle *pdev) {
	unsigned int i;

	pdev->regs->dev.d_ctl&=~DCTL_RWUSIG;

	usb_core_flush_tx_fifo(pdev->regs,0);

	for(i=0;i<pdev->cfg.dev_endpoints;i++) {
		pdev->regs->dev.d_ep_in[i].ints=0xff;
		pdev->regs->dev.d_ep_out[i].ints=0xff;
	}

	pdev->regs->dev.d_aint=0xffffffff;
	pdev->regs->dev.d_aintmsk=(1<<16)|(1<<0);

	pdev->regs->dev.d_doep_msk=
		DDOEPMSK_SETUP|DDOEPMSK_XFER_DONE|DDOEPMSK_EPDISABLED;
#ifdef DEDICATED_EP1
	pdev->regs->dev.d_doep_each_msk1=
		DDOEPMSK_SETUP|DDOEPMSK_XFER_DONE|DDOEPMSK_EPDISABLED;
//		DDOEPMSK_XFER_DONE|DDOEPMSK_EPDISABLED;
#endif

	pdev->regs->dev.d_diep_msk=
		DDIEPMSK_XFRC|DDIEPMSK_TO|DDIEPMSK_EPD;
#ifdef DEDICATED_EP1
	pdev->regs->dev.d_diep_each_msk1=
		DDIEPMSK_XFRC|DDIEPMSK_TO|DDIEPMSK_EPD;
#endif

	pdev->regs->dev.d_cfg&=~DCFG_DADDR_MSK;

	dev_EP0_out_start(pdev);

	pdev->regs->core.g_int_sts=G_INT_STS_USBRST;

	usbd_reset(pdev);

	return 1;
}

static unsigned int device_handle_enum_done_irq(struct usb_dev_handle *pdev) {
	unsigned int usbcfg;
	dev_EP0_activate(pdev->regs);

	usbcfg=pdev->regs->core.g_usb_cfg;

	if (dev_get_speed(pdev->regs)==USB_SPEED_HIGH) {
		pdev->cfg.speed=USB_SPEED_HIGH;
		pdev->cfg.maxpacketsize=USB_HS_MAX_PACKET_SIZE;
		usbcfg&=~G_USB_CFG_TRDT_MSK;
		usbcfg|=(9<<G_USB_CFG_TRDT_SHIFT);
	} else {
		pdev->cfg.speed=USB_SPEED_FULL;
		pdev->cfg.maxpacketsize=USB_FS_MAX_PACKET_SIZE;
		usbcfg&=~G_USB_CFG_TRDT_MSK;
		usbcfg|=(5<<G_USB_CFG_TRDT_SHIFT);
	}

	pdev->regs->core.g_usb_cfg=usbcfg;
	pdev->regs->core.g_int_sts=G_INT_STS_ENUMDNE;
	return 1;
}

static unsigned int device_handle_incomplete_iso_in_irq(struct usb_dev_handle *pdev) {

	usbd_iso_in_incomplete(pdev);
	pdev->regs->core.g_int_sts=G_INT_STS_IISOIXFR;
	return 1;
}

static unsigned int device_handle_incomplete_iso_out_irq(struct usb_dev_handle *pdev) {

	usbd_iso_out_incomplete(pdev);
	pdev->regs->core.g_int_sts=G_INT_STS_INCOMPISOOUT;
	return 1;
}

static unsigned int device_handle_session_req_irq(struct usb_dev_handle *pdev) {
//	usbd_dev_connected(pdev);
	pdev->regs->core.g_int_sts=G_INT_STS_SRQINT;
	return 1;
}

static unsigned int device_handle_otg_irq(struct usb_dev_handle *pdev) {
	unsigned int gotgint=pdev->regs->core.g_otg_gint;

	if (gotgint&G_OTG_INT_SEDET) {
//		usbd_dev_disconnected(pdev);
	}

	pdev->regs->core.g_otg_gint=gotgint;
	return 1;
}

unsigned int handle_device_irq(struct usb_dev_handle *pdev) {
	unsigned int rc=0;

	if (get_mode(pdev->regs)!=HOST_MODE) {
		unsigned int gint_status=get_core_irq_stat(pdev->regs);
		if (!gint_status) {
			sys_printf("handle_device_irq: spurious\n");
			return 0;
		}

//		if ((gint_status!=0x40000)&&(gint_status!=0x80000)&&
//			(gint_status!=0x10)) {
//			sys_printf("usb_irq: gint_stat=%x\n",gint_status);
//		}
//		if (gint_status&G_INT_STS_RXFLVL) {
//			gint_status&=~G_INT_STS_RXFLVL;
//			rc|=device_handle_rxfifo_irq(pdev);
//		}


		if (gint_status&G_INT_STS_OEPINT) {
			gint_status&=~G_INT_STS_OEPINT;
			rc|=device_handle_outep_irq(pdev);
		}

		if (gint_status&G_INT_STS_IEPINT) {
			gint_status&=~G_INT_STS_IEPINT;
			rc|=device_handle_inep_irq(pdev);
		}

		if (gint_status&G_INT_STS_MMIS) {
			gint_status&=~G_INT_STS_MMIS;
			pdev->regs->core.g_int_sts=G_INT_STS_MMIS;
		}

		if (gint_status&G_INT_STS_WKUIN) {
			gint_status&=~G_INT_STS_WKUIN;
			rc|=device_handle_resume_irq(pdev);
		}

		if (gint_status&G_INT_STS_USBSUSP) {
			gint_status&=~G_INT_STS_USBSUSP;
			rc|=device_handle_usb_suspend_irq(pdev);
		}

		if (gint_status&G_INT_STS_SOF) {
			gint_status&=~G_INT_STS_SOF;
			rc|=device_handle_sof_irq(pdev);
		}

		if (gint_status&G_INT_STS_RXFLVL) {
			gint_status&=~G_INT_STS_RXFLVL;
			rc|=device_handle_rxfifo_irq(pdev);
		}

		if (gint_status&G_INT_STS_USBRST) {
			gint_status&=~G_INT_STS_USBRST;
			rc|=device_handle_usb_reset_irq(pdev);
		}

		if (gint_status&G_INT_STS_ENUMDNE) {
			gint_status&=~G_INT_STS_ENUMDNE;
			rc|=device_handle_enum_done_irq(pdev);
		}

		if (gint_status&G_INT_STS_IISOIXFR) {
			gint_status&=~G_INT_STS_IISOIXFR;
			rc|=device_handle_incomplete_iso_in_irq(pdev);
		}

		if (gint_status&G_INT_STS_INCOMPISOOUT) {
			gint_status&=~G_INT_STS_INCOMPISOOUT;
			rc|=device_handle_incomplete_iso_out_irq(pdev);
		}

		if (gint_status&G_INT_STS_SRQINT) {
			gint_status&=~G_INT_STS_SRQINT;
			rc|=device_handle_session_req_irq(pdev);
		}

		if (gint_status&G_INT_STS_OTGINT) {
			gint_status&=~G_INT_STS_OTGINT;
			rc|=device_handle_otg_irq(pdev);
		}

		if (gint_status) {
			sys_printf("handle_device_irq: unhandled irq %x\n",
					gint_status);
		}
	}
	return rc;
}

/***************************************************************************/
