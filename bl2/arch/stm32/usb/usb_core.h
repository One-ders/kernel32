
#define USB_FS_MAX_PACKET_SIZE           64
#define USB_HS_MAX_PACKET_SIZE           512


#define MAX_DATA_LEN	0x200
#define MAX_HOST_TX_FIFOS	15

#define RX_FIFO_FS_SIZE 	128 
#define TX0_FIFO_FS_SIZE	32
#define TX1_FIFO_FS_SIZE	64
#define TX2_FIFO_FS_SIZE	64
#define TX3_FIFO_FS_SIZE	64


#define RX_FIFO_HS_SIZE		512 
#define TX0_FIFO_HS_SIZE	128
#define TX1_FIFO_HS_SIZE	372
#define TX2_FIFO_HS_SIZE	0
#define TX3_FIFO_HS_SIZE	0
#define TX4_FIFO_HS_SIZE	0
#define TX5_FIFO_HS_SIZE	0

#define TXH_NP_FIFO_FS_SIZE	96
#define TXH_P_FIFO_FS_SIZE	96

#define TXH_NP_FIFO_HS_SIZE	96
#define TXH_P_FIFO_HS_SIZE	96

#define HOST_MODE	1
#define DEVICE_MODE	2

#define EP_TYPE_CTRL	0
#define EP_TYPE_ISO	1
#define EP_TYPE_BULK	2
#define EP_TYPE_IRQ	3

#define HPRT_SPEED_HI	0
#define HPRT_SPEED_FULL	1
#define HPRT_SPEED_LOW	2

#define USB_SPEED_UNKNOWN	0
#define USB_SPEED_LOW		1
#define USB_SPEED_FULL		2
#define USB_SPEED_HIGH		3

#define EP_TX_VALID	0
#define EP_TX_NAK	1
#define EP_TX_STALL	2
#define EP_TX_DIS	3

#define EP_RX_VALID	0
#define EP_RX_NAK	1
#define EP_RX_STALL	2
#define EP_RX_DIS	3

#define USB_OTG_ULPI_PHY      1
#define USB_OTG_EMBEDDED_PHY  2
#define USB_OTG_I2C_PHY       3


struct usb_dev_ep {
	unsigned char num;
	unsigned char is_in;
	unsigned char is_stall;
	unsigned char type;
	/***		***/
	unsigned char data_pid_start;
	unsigned char even_odd_frame;
	unsigned short int tx_fifo_num;
	/***		***/
	unsigned int	maxpacket;
	unsigned char	*xfer_buf;
	unsigned int	dma_addr;
	unsigned int	xfer_len;
	unsigned int	xfer_count;
	unsigned int	rem_data_len;
	unsigned int	total_data_len;
	unsigned int	ctl_data_len;
};

#define USB_DEV_EP0_SETUP	0x1
#define USB_DEV_EP0_STATUS_IN	0x2
#define USB_DEV_EP0_STATUS_OUT	0x3
#define USB_DEV_EP0_DATA_IN	0x4
#define USB_DEV_EP0_DATA_OUT	0x5

struct usb_dev {
	unsigned char dev_config;
	unsigned char dev_state;
	unsigned char dev_status;
	unsigned char dev_old_status;

	unsigned char dev_addr;
	unsigned char conn_status;
	unsigned char test_mode;
	unsigned char pad1;

	unsigned int remote_wakeup;
	struct usb_dev_ep in_ep[5];
	struct usb_dev_ep out_ep[5];
	unsigned char setup_packet[8*3];

	struct usb_dev_class	*class_cb;
	struct usb_dev_usr	*usr_cb;
	struct usb_device 	*usr_device;
	unsigned char		*config_descriptor;
};


#define HC_STATUS_IDLE 	0
#define HC_STATUS_XFERC	1
#define HC_STATUS_CHLT	2
#define HC_STATUS_NAK	3
#define HC_STATUS_NYET	4
#define HC_STATUS_STALL 5
#define HC_STATUS_TXERR 6
#define HC_STATUS_BBLERR 7
#define HC_STATUS_DTERR	8

#define URB_STATE_IDLE	0
#define URB_STATE_DONE	1
#define URB_STATE_NOTREADY 2
#define URB_STATE_ERROR	3
#define URB_STATE_STALL 4

struct hc {
	unsigned char	dev_addr;
	unsigned char	ep_num;
	unsigned char 	ep_is_in;
	unsigned char 	speed;
	/**************************/
	unsigned char 	do_ping;
	unsigned char 	ep_type;
	unsigned short int max_pkt_size;
	/**************************/
	unsigned char	data_pid;
	unsigned char   pad1[3];
	/**************************/
	unsigned char	*xfer_buf;
	/**************************/
	unsigned int	xfer_len;
	/**************************/
	unsigned int	xfer_count;
	/**************************/
	unsigned char   toggle_in;
	unsigned char	toggle_out;
	unsigned char	pad2[2];
	/**************************/
	unsigned int	dma_addr;
};

struct host_tx_fifo_data {
	volatile unsigned int	err_cnt;
	volatile unsigned int	xfer_cnt;
	volatile unsigned short int  hc_status;
	volatile unsigned short int  urb_state;
	struct hc		hc;
	unsigned short int	channel;
};

struct usb_host {
	volatile unsigned char 		rx_buf[MAX_DATA_LEN];
	volatile unsigned int 		con_stat;
	struct host_tx_fifo_data	fd[MAX_HOST_TX_FIFOS];
};

#define USB_OTG_HS_CORE_ID 0x00001100

struct usb_cfg {
	unsigned int dma_enable;
	unsigned int sof_output;
	unsigned int host_channels;
//	unsigned int core_id;
	unsigned int dev_endpoints;
	unsigned int low_power;
	unsigned int speed;
	unsigned int maxpacketsize;
	unsigned int total_fifo_size;
	unsigned int phy_itf;
};

struct usb_dev_handle;
struct usb_otg_regs;

unsigned int handle_device_irq(struct usb_dev_handle *pdev);

int usb_config_core(struct usb_dev_handle *pdev);
int usb_core_disable_gint();
int usb_core_enable_gint();
int usb_core_init_core();
int usb_core_set_mode(struct usb_otg_regs *regs, unsigned int mode);
int usb_core_flush_tx_fifo(struct usb_otg_regs *regs, unsigned int fifo);
int usb_core_flush_rx_fifo();

int usb_core_dev_core_init();
int usb_core_dev_EP_activate(struct usb_otg_regs *, struct usb_dev_ep *ep);
int usb_core_dev_EP_deactivate(struct usb_otg_regs *, struct usb_dev_ep *ep);
int usb_core_dev_EP_start_xfer(struct usb_dev_handle *, struct usb_dev_ep *ep);
int usb_core_dev_EP0_start_xfer(struct usb_dev_handle *,struct usb_dev_ep *ep);
int usb_core_dev_EP_set_stall(struct usb_otg_regs *,struct usb_dev_ep *ep);
int usb_core_dev_EP_clr_stall(struct usb_otg_regs *,struct usb_dev_ep *ep);
void usb_core_dev_set_address(struct usb_otg_regs *,unsigned char address);
void usb_core_dev_connect(struct usb_otg_regs *);
void usb_core_dev_disconnect(struct usb_otg_regs *);
unsigned int usb_core_dev_get_ep_status(struct usb_otg_regs *,struct usb_dev_ep *ep);
void usb_core_dev_set_ep_status(struct usb_otg_regs *,struct usb_dev_ep *ep, unsigned int status);


#if 0

struct usb_core_fnc {
	int (*reset_core)();
	int (*write_pkt)(unsigned int cp_num, unsigned char *buf, size_t len);
	int (*read_pkt)(unsigned char *buf, size_t len);
//	int (*init_core)();
//	int (*enable_gint)();
//	int (*disable_gint)();
//	int (*flush_tx_fifo)(unsigned int fifo);
//	int (*flush_rx_fifo)();
//	int (*set_mode)(unsigned int mode);
	unsigned int (*get_mode)();
	unsigned int (*get_core_irq_stat)();
	unsigned int (*get_otg_irq_stat)();
	/****                                     ****/
	int (*host_init_core)();
	int (*host_enable_int)();
	void (*host_stop)();
	int (*isEvenFrame)();
	int (*drive_vbus)(unsigned int state);
	void (*init_fs_ls_p_clksel)(unsigned int freq);
	unsigned int (*read_hprt0)();
	unsigned int (*get_host_all_channel_irq)();
	int (*reset_port)();
	/****                                     ****/
	int (*host_channel_init)(unsigned int host_channel);
	int (*host_channel_start_tx)(unsigned int host_channel);
	int (*host_channel_halt)(unsigned int host_channel);
	int (*host_channel_do_ping)(unsigned int host_channel);
	/****                                    ****/
	void (*dev_speed_init)(unsigned int speed);
//	int (*dev_core_init)();
	int (*dev_enable_int)();
	unsigned int (*dev_get_speed)();
	int (*dev_EP0_activate)();
//	int (*dev_EP_activate)(struct usb_dev_ep *ep);
//	int (*dev_EP_deactivate)(struct usb_dev_ep *ep);
//	int (*dev_EP_start_xfer)(struct usb_dev_ep *ep);
//	int (*dev_EP0_start_xfer)(struct usb_dev_ep *ep);
//	int (*dev_EP_set_stall)(struct usb_dev_ep *ep);
//	int (*dev_EP_clr_stall)(struct usb_dev_ep *ep);
	unsigned int (*dev_read_all_out_ep_irq)();
	unsigned int (*dev_read_out_ep_irq)(unsigned int channel);
	unsigned int (*dev_read_all_in_ep_irq)();
	void (*dev_EP0_out_start)();   // means receive !!!
	void (*dev_remote_wakeup)();
	void (*dev_ungate_clock)();
	void (*dev_stop)();
//	unsigned int (*dev_get_ep_status)(struct usb_dev_ep *ep);
//	void (*dev_set_ep_status)(struct usb_dev_ep *ep, unsigned int status);
};

struct usb_host_handle {
	struct usb_otg_regs	*regs;
	struct usb_cfg		cfg;
	struct usb_host		host;
};
#endif

struct usb_dev_handle {
	struct usb_otg_regs     *regs;
	struct usb_cfg          cfg;
	union {
		struct usb_dev          dev;
		struct usb_host		host;
	};
	void			*uref;
};

