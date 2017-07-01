

/** USB device implementor interface **/
#define USBD_CFG_MAX_NUM		1
#define USBD_ITF_MAX_NUM		1

#define USB_DEV_STAT_DEFAULT		1
#define USB_DEV_STAT_ADDRESSED		2
#define USB_DEV_STAT_CONFIGURED		3
#define USB_DEV_STAT_SUSPENDED		4

#define USB_DEV_MAX_EP0_SIZE		64


#define  USB_LEN_DEV_QUALIFIER_DESC                     0x0A
#define  USB_LEN_DEV_DESC                               0x12
#define  USB_LEN_CFG_DESC                               0x09
#define  USB_LEN_IF_DESC                                0x09
#define  USB_LEN_EP_DESC                                0x07
#define  USB_LEN_OTG_DESC                               0x03

#define  USBD_IDX_LANGID_STR                            0x00 
#define  USBD_IDX_MFC_STR                               0x01 
#define  USBD_IDX_PRODUCT_STR                           0x02
#define  USBD_IDX_SERIAL_STR                            0x03 
#define  USBD_IDX_CONFIG_STR                            0x04 
#define  USBD_IDX_INTERFACE_STR                         0x05 

#define  USB_REQ_TYPE_STANDARD                          0x00
#define  USB_REQ_TYPE_CLASS                             0x20
#define  USB_REQ_TYPE_VENDOR                            0x40
#define  USB_REQ_TYPE_MASK                              0x60

#define  USB_REQ_RECIPIENT_DEVICE                       0x00
#define  USB_REQ_RECIPIENT_INTERFACE                    0x01
#define  USB_REQ_RECIPIENT_ENDPOINT                     0x02
#define  USB_REQ_RECIPIENT_MASK                         0x03

#define  USB_REQ_GET_STATUS                             0x00
#define  USB_REQ_CLR_FEATURE				0x01
#define  USB_REQ_SET_FEATURE                            0x03
#define  USB_REQ_SET_ADDRESS                            0x05
#define  USB_REQ_GET_DESCRIPTOR                         0x06
#define  USB_REQ_SET_DESCRIPTOR                         0x07
#define  USB_REQ_GET_CONFIGURATION                      0x08
#define  USB_REQ_SET_CONFIGURATION                      0x09
#define  USB_REQ_GET_INTERFACE                          0x0A
#define  USB_REQ_SET_INTERFACE                          0x0B
#define  USB_REQ_SYNCH_FRAME                            0x0C

#define  USB_DESC_TYPE_DEVICE                              1
#define  USB_DESC_TYPE_CONFIGURATION                       2
#define  USB_DESC_TYPE_STRING                              3
#define  USB_DESC_TYPE_INTERFACE                           4
#define  USB_DESC_TYPE_ENDPOINT                            5
#define  USB_DESC_TYPE_DEVICE_QUALIFIER                    6
#define  USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION           7


#define USB_CONFIG_REMOTE_WAKEUP                           2
#define USB_CONFIG_SELF_POWERED                            1

#define USB_FEATURE_EP_HALT                                0
#define USB_FEATURE_REMOTE_WAKEUP                          1
#define USB_FEATURE_TEST_MODE                              2


#define  SWAPBYTE(addr)        (((unsigned short int)(*((unsigned char *)(addr)))) + \
                               (((unsigned short int)(*(((uint8_t *)(addr)) + 1))) << 8))

#define LOBYTE(x)  ((unsigned char)(x & 0x00FF))
#define HIBYTE(x)  ((unsigned char)((x & 0xFF00) >>8))


unsigned int usb_dev_ep_open(struct usb_dev_handle *pdev,
                                        unsigned char ep_addr,
                                        unsigned short int ep_max_packet_size,
                                        unsigned char ep_type);
unsigned int usb_dev_ep_flush(struct usb_dev_handle *pdev,
				unsigned char epnum);
unsigned int usb_dev_prepare_rx(struct usb_dev_handle *pdev,
					unsigned char ep_addr,
					unsigned char *buf,
					unsigned short int blen);

void dev_EP0_out_start(struct usb_dev_handle *pdev);

void dev_remote_wakeup(struct usb_dev_handle *pdev);
void usb_dev_init(struct usb_dev_handle *pdev);
unsigned int usb_dev_ep_stall(struct usb_dev_handle *pdev,
				unsigned char epnum);
unsigned int usb_dev_ep_clr_stall(struct usb_dev_handle *pdev,
				unsigned char epnum);
void usb_dev_set_address(struct usb_dev_handle *pdev,
                                unsigned char address);

#if 0
struct usb_device_funcs {
	void (*usb_dev_init)(unsigned int coreId);
	unsigned int (*usb_dev_ep_open)(unsigned char ep_addr,
					unsigned short int ep_max_pkt_size,
					unsigned char ep_type);
	unsigned int (*usb_dev_ep_close)(unsigned char ep_addr);
	unsigned int (*usb_dev_prepare_rx)(unsigned char ep_addr,
					unsigned char *buf,
					unsigned short int blen);
	unsigned int (*usb_dev_tx)(unsigned char ep_addr,
				unsigned char *buf,
				unsigned int blen);
	unsigned int (*usb_dev_ep_stall)(unsigned char epnum);
	unsigned int (*usb_dev_ep_clr_stall)(unsigned char epnum);
	unsigned int (*usb_dev_ep_flush)(unsigned char epnum);
	void (*usb_dev_set_address)(unsigned char address);
	void (*usb_dev_connect)(void);
	void (*usb_dev_disconnect)(void);
	unsigned int (*usb_dev_get_ep_status)(unsigned char epnum);
	void (*usb_dev_set_ep_status)(unsigned char epnum, 
						unsigned int status);
};

#endif

unsigned int usb_dev_get_string(unsigned char *desc, unsigned char *unicode, unsigned short *len);

struct usb_device_cb {
	void (*data_out)(unsigned int ep);
	void (*data_in)(unsigned int ep);
	void (*setup_done)(unsigned int ep);
	void (*handle_resume)(void);
	void (*handle_suspend)(void);
	void (*sof)(void);
	void (*reset)(void);
	void (*iso_in_incomplete)(void);
	void (*iso_out_incomplete)(void);
	void (*dev_connected)(void);
	void (*dev_disconnected)(void);
};

struct usb_device {
	unsigned char *(*get_device_descriptor)(void *uref, 
				unsigned char speed, unsigned short int *len);
	unsigned char *(*get_lang_id_descriptor)(void *uref, 
				unsigned char speed, unsigned short int *len);
	unsigned char *(*get_mfc_descriptor)(void *uref, 
				unsigned char speed, unsigned short int *len);
	unsigned char *(*get_prod_descriptor)(void *uref, 
				unsigned char speed, unsigned short int *len);
	unsigned char *(*get_serial_descriptor)(void *uref, 
				unsigned char speed, unsigned short int *len);
	unsigned char *(*get_config_descriptor)(void *uref, 
				unsigned char speed, unsigned short int *len);
	unsigned char *(*get_itf_descriptor)(void *uref, 
				unsigned char speed, unsigned short int *len);
};

struct usb_setup_req {
	unsigned char bmRequest;
	unsigned char bRequest;
	unsigned short int wValue;
	unsigned short int wIndex;
	unsigned short int wLength;
};

struct usb_dev_class {
	unsigned int (*init)(void *uref, unsigned char cfgidx);
	unsigned int (*deinit)(void *uref, unsigned char cfgidx);
	unsigned int (*setup)(void *uref, struct usb_setup_req *req);
	unsigned int (*ep0_tx_sent)(void *uref);
	unsigned int (*ep0_rx_ready)(void *uref);
	unsigned int (*data_in)(void *uref, unsigned char epnum);
	unsigned int (*data_out)(void *uref, unsigned char epnum);
	unsigned int (*sof)(void *uref);
	unsigned int (*iso_in_incomplete)(void *uref);
	unsigned int (*iso_out_incomplete)(void *uref);
	unsigned char *(*get_config_descriptor)(void *uref,
			unsigned char speed, unsigned short int *length);		
	unsigned char *(*get_other_cf_descriptor)(void *uref,
			unsigned char speed, unsigned short int *length);		
	unsigned char *(*get_usr_str_descriptor)(void *uref,
			unsigned char speed, unsigned char index,
			unsigned short int *length);		
};

struct usb_dev_usr {
	void (*init)(void);
	void (*dev_reset)(unsigned char speed);
	void (*dev_configured)(void);
	void (*dev_suspended)(void);
	void (*dev_resumed)(void);
	void (*dev_connected)(void);
	void (*dev_disconnected)(void);
};

void usbd_init(struct usb_dev_handle    **pdev,
		void  			*uref,
                struct usb_device	*usb_dev,
                struct usb_dev_class 	*usb_class,
                struct usb_dev_usr      *usb_dev_usr);

void usbd_deinit(struct usb_dev_handle *pdev);
int usbd_setup(struct usb_dev_handle *pdev);
int usbd_data_out(struct usb_dev_handle *pdev, unsigned char epnum);
int usbd_data_in(struct usb_dev_handle *pdev, unsigned char epnum);
int usbd_run_test_mode(struct usb_dev_handle *pdev);
int usbd_reset(struct usb_dev_handle *pdev);
int usbd_resume(struct usb_dev_handle *pdev);
int usbd_suspend(struct usb_dev_handle *pdev);
int usbd_sof(struct usb_dev_handle *pdev);
int usbd_set_cfg(struct usb_dev_handle *pdev, unsigned char cfgidx);
int usbd_clr_cfg(struct usb_dev_handle *pdev, unsigned char cfgidx);
int usbd_iso_in_incomplete(struct usb_dev_handle *pdev);
int usbd_iso_out_incomplete(struct usb_dev_handle *pdev);
int usbd_dev_connected(struct usb_dev_handle *pdev);
int usbd_dev_disconnected(struct usb_dev_handle *pdev);



unsigned int usb_dev_tx(struct usb_dev_handle *pdev,
                                unsigned char ep_addr,
                                unsigned char *buf,
                                unsigned int buf_len);

unsigned int usb_dev_get_string(unsigned char *desc,
                                unsigned char *unicode,
                                unsigned short *len);

int usbd_ctl_send_data(struct usb_dev_handle *pdev,
                                void *buf,
                                unsigned short int len);

unsigned int usb_dev_ctl_prepare_rx(struct usb_dev_handle *pdev,
                                unsigned char *buf,
                                unsigned short int len);

void usbd_ctl_error(struct usb_dev_handle *pdev,
                        struct usb_setup_req *req);

int usbd_ctl_continue_rx(struct usb_dev_handle *pdev,
                        void *buf,
                        unsigned short int len);

int usbd_ctl_continue_tx(struct usb_dev_handle *pdev,
                        void *pbuf,
                        unsigned short int len);

int usbd_ctl_receive_status(struct usb_dev_handle *pdev);
