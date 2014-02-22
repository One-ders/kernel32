

/* Table 9-5. Descriptor Types of USB Specifications */
#define  USB_DESC_TYPE_DEVICE                              1
#define  USB_DESC_TYPE_CONFIGURATION                       2
#define  USB_DESC_TYPE_STRING                              3
#define  USB_DESC_TYPE_INTERFACE                           4
#define  USB_DESC_TYPE_ENDPOINT                            5
#define  USB_DESC_TYPE_DEVICE_QUALIFIER                    6
#define  USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION           7
#define  USB_DESC_TYPE_INTERFACE_POWER                     8
#define  USB_DESC_TYPE_HID                                 0x21
#define  USB_DESC_TYPE_HID_REPORT                          0x22

#define  USBD_IDX_LANGID_STR                            0x00 
#define  USBD_IDX_MFC_STR                               0x01 
#define  USBD_IDX_PRODUCT_STR                           0x02
#define  USBD_IDX_SERIAL_STR                            0x03 
#define  USBD_IDX_CONFIG_STR                            0x04 
#define  USBD_IDX_INTERFACE_STR                         0x05 



#define USBD_ITF_MAX_NUM           1
#define USBD_CFG_MAX_NUM           1


#define  USB_REQ_TYPE_STANDARD                          0x00
#define  USB_REQ_TYPE_CLASS                             0x20
#define  USB_REQ_TYPE_VENDOR                            0x40
#define  USB_REQ_TYPE_MASK                              0x60

#define  USB_REQ_RECIPIENT_DEVICE                       0x00
#define  USB_REQ_RECIPIENT_INTERFACE                    0x01
#define  USB_REQ_RECIPIENT_ENDPOINT                     0x02
#define  USB_REQ_RECIPIENT_MASK                         0x03

#define  USB_REQ_GET_STATUS                             0x00
#define  USB_REQ_CLEAR_FEATURE                          0x01
#define  USB_REQ_SET_FEATURE                            0x03
#define  USB_REQ_SET_ADDRESS                            0x05
#define  USB_REQ_GET_DESCRIPTOR                         0x06
#define  USB_REQ_SET_DESCRIPTOR                         0x07
#define  USB_REQ_GET_CONFIGURATION                      0x08
#define  USB_REQ_SET_CONFIGURATION                      0x09
#define  USB_REQ_GET_INTERFACE                          0x0A
#define  USB_REQ_SET_INTERFACE                          0x0B
#define  USB_REQ_SYNCH_FRAME                            0x0C

#define USB_OTG_EP_CONTROL			0
#define USB_OTG_EP_ISOC				1
#define USB_OTG_EP_BULK				2
#define USB_OTG_EP_INT				3
#define USB_OTG_EP_MASK				3

#define USB_OTG_SPEED_HIGH			0
#define USB_OTG_SPEED_FULL			1


#define  SWAPBYTE(addr)        (((unsigned short int)(*((unsigned char *)(addr)))) + \
                               (((unsigned short int)(*(((unsigned char *)(addr)) + 1))) << 8))
#define LOBYTE(x)  ((unsigned char)(x & 0x00FF))
#define HIBYTE(x)  ((unsigned char)((x & 0xFF00) >>8))




struct usb_setup_req {
	unsigned char   bmRequest;
	unsigned char   bRequest;
	unsigned short int wValue;
	unsigned short int wIndex;
	unsigned short int wLength;
};


struct usbd_class_cb {
	unsigned char  (*init)(void *core, unsigned char cfgidx);
	unsigned char  (*deinit)(void *core, unsigned char cfgidx);
	unsigned char  (*setup)(void *core, struct usb_setup_req *req);
	unsigned char  (*EP0_tx_sent)(void *core);
	unsigned char  (*EP0_rx_ready)(void *core);
	unsigned char  (*data_in)(void *core, unsigned char epnum);
	unsigned char  (*data_out)(void *core, unsigned char epnum);
	unsigned char  (*sof)(void *core);
	unsigned char  (*iso_in_incomplete)(void *core);
	unsigned char  (*iso_out_incomplete)(void *core);
	unsigned char  *(*get_config_descriptor)(unsigned char speed, unsigned short *len);
};

struct usbd_device {
	unsigned char *(*get_device_descriptor)(unsigned char speed, unsigned short *len);
	unsigned char *(*get_langIdStr_descriptor)(unsigned char speed, unsigned short *len);
	unsigned char *(*get_manufacturerStr_descriptor)(unsigned char speed, unsigned short *len);
	unsigned char *(*get_productStr_descriptor)(unsigned char speed, unsigned short *len);
	unsigned char *(*get_serialStr_descriptor)(unsigned char speed, unsigned short *len);
	unsigned char *(*get_configStr_descriptor)(unsigned char speed, unsigned short *len);
	unsigned char *(*get_interfaceStr_descriptor)(unsigned char speed, unsigned short *len);
};

struct usbd_usr_cb {
	void (*init)(void);
	void (*dev_reset)(unsigned char speed);
	void (*dev_configured)(void);
	void (*dev_suspended)(void);
	void (*dev_resumed)(void);
	void (*dev_connected)(void);
	void (*dev_disconnected)(void);
};

struct usb_dev {
	struct usbd_device	*device;
	struct usbd_class_cb	*class_cb;
	struct usbd_usr_cb	*usr_cb;
};

void usbd_get_string(char *desc, unsigned char *unicode, unsigned short *len);
int dcd_ep_open(void *core, unsigned char ep_addr,
                        unsigned short int ep_mps, unsigned char ep_type);
int dcd_ep_prepare_rx(void *core, unsigned char ep_addr,
                                unsigned char *pbuf, unsigned short int len);
unsigned short int get_rx_cnt(void *core, int epnum);
int usbd_ctl_error(void *vcore, struct usb_setup_req *req);
int dcd_ep_tx(void *core, unsigned char ep_addr, unsigned char *pbuf, unsigned int len);
