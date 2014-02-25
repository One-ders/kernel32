
#include "sys.h"
#include "io.h"

#include "usbd_core.h"
#include "usbd_usr.h"
#include "usbd_req.h"
#include "usbd_ioreq.h"
#include "usbd_desc.h"
#include "usbd_conf.h"
#include "usb_regs.h"
#include "usbd_cdc_core.h"


USB_OTG_CORE_HANDLE USB_OTG_dev;

/* Exported define -----------------------------------------------------------*/
#define USB_DEVICE_DESCRIPTOR_TYPE              0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE       0x02
#define USB_STRING_DESCRIPTOR_TYPE              0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE           0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE            0x05

#define VIRTUAL_COM_PORT_DATA_SIZE              64
//#define VIRTUAL_COM_PORT_INT_SIZE               8 //FIXME

#define WBVAL(x) (x & 0xFF),((x >> 8) & 0xFF)

#define USB_CONFIGURATION_DESC_SIZE             9   // Standard of USB
#define USB_INTERFACE_DESC_SIZE                 9   // Standard of USB
#define USB_ENDPOINT_DESC_SIZE                  7   // Standard of USB

#define USB_ENDPOINT_TYPE_BULK                  0x02
#define USB_ENDPOINT_TYPE_INTERRUPT             0x03

#define USB_CONFIG_BUS_POWERED                 0x80
//#define USB_CONFIG_SELF_POWERED                0xC0
#define USB_CONFIG_POWER_MA(mA)                ((mA)/2)

#define CDC_V1_10 0x0110
#define USB_ENDPOINT_OUT(addr)                 ((addr) | 0x00)
#define USB_ENDPOINT_IN(addr)                  ((addr) | 0x80)

#define CDC_COMMUNICATION_INTERFACE_CLASS       0x02
#define CDC_DATA_INTERFACE_CLASS                0x0A
#define CDC_ABSTRACT_CONTROL_MODEL              0x02
#define CDC_CS_INTERFACE                        0x24
#define CDC_HEADER                              0x00
#define CDC_CALL_MANAGEMENT                     0x01
#define CDC_ABSTRACT_CONTROL_MANAGEMENT         0x02
#define CDC_UNION                               0x06


#define CDC_IF_DESC_SET_SIZE    ( USB_INTERFACE_DESC_SIZE + 0x05 + 0x05 + 0x04 + \
                                 0x05 + USB_ENDPOINT_DESC_SIZE + \
                                 USB_INTERFACE_DESC_SIZE + 2 * USB_ENDPOINT_DESC_SIZE )
 
#define IAD_CDC_IF_DESC_SET_SIZE    ( 8 + CDC_IF_DESC_SET_SIZE )

#define VIRTUAL_COM_PORT_SIZ_DEVICE_DESC        18      // Standard of USB, not device dependent
//#define VIRTUAL_COM_PORT_SIZ_CONFIG_DESC        67        // FIXME
#define VIRTUAL_COM_PORT_SIZ_CONFIG_DESC        (9+3*IAD_CDC_IF_DESC_SET_SIZE)
#define VIRTUAL_COM_PORT_SIZ_STRING_LANGID      4
#define VIRTUAL_COM_PORT_SIZ_STRING_VENDOR      38      // Ok, for "STMicroelectronics"
#define VIRTUAL_COM_PORT_SIZ_STRING_PRODUCT     50      // Ok, "STM32 Virtual COM Port"
#define VIRTUAL_COM_PORT_SIZ_STRING_SERIAL      26      // FIXME, it is actually 16 for "STM3210"
 
//#define STANDARD_ENDPOINT_DESC_SIZE             0x09  // Funny, this is wrong but unused anyway
 
//#define CDC_DATA_IN_PACKET_SIZE                64
//#define CDC_DATA_OUT_PACKET_SIZE               64

#define USB_SIZ_STRING_LANGID		4
#define USBD_LANGID_STRING            0x409
#define USBD_MANUFACTURER_STRING      ((unsigned char *)"FranzBalanz Electronics")

#define USBD_PRODUCT_FS_STRING		((unsigned char *)"Köszög board v 1.0")
#define USBD_PRODUCT_HS_STRING		((unsigned char *)"HS Njet")

#define USBD_SERIALNUMBER_FS_STRING	((unsigned char *)"00000000011C")
#define USBD_SERIALNUMBER_HS_STRING	((unsigned char *)"00000000011B")

#define USB_MAX_STR_DESC_SIZ       64


static int core_fd;

static unsigned char rx_buf[8][8];

extern unsigned int USBD_OTG_ISR_Handler(USB_OTG_CORE_HANDLE *pdev);
unsigned char CmdBuff[64];
static unsigned int cdcCmd=0xFF;
static unsigned int cdcLen=0;
static volatile unsigned int usbd_cdc_AltSet;

void OTG_FS_IRQHandler(void) {
	io_setpolled(1);
	USBD_OTG_ISR_Handler(&USB_OTG_dev);
	io_setpolled(0);
}


static const unsigned char device_descriptor[] = {
    0x12,   /* bLength */
    USB_DEVICE_DESCRIPTOR_TYPE,     /* bDescriptorType */
    0x00,
    0x02,   /* bcdUSB = 2.00 */
#if 1
    0xEF,   /* bDeviceClass    (Misc)   */
    0x02,   /* bDeviceSubClass (common) */
    0x01,   /* bDeviceProtocol (IAD)    */
#else
	0,
	0,
	0,
#endif
    0x40,   /* bMaxPacketSize0 */        // <-----------
    0xFF,
    0x02,   /* idVendor = 0xFF02 */
    0x00,
    0x01,   /* idProduct = 0x0001 */
    0x00,
    0x02,   /* bcdDevice = 2.00 */
    1,              /* Index of string descriptor describing manufacturer */
    2,              /* Index of string descriptor describing product */
    3,              /* Index of string descriptor describing the device's serial number */
    0x01    /* bNumConfigurations */
};


#if 0
#define CDC_IF_DESC_SET( comIfNum, datIfNum, comInEp, datOutEp, datInEp )   \
/* CDC Communication Interface Descriptor */                            \
    USB_INTERFACE_DESC_SIZE,                /* bLength */               \
    USB_INTERFACE_DESCRIPTOR_TYPE,          /* bDescriptorType */       \
    comIfNum,                               /* bInterfaceNumber */      \
    0x00,                                   /* bAlternateSetting */     \
    0x01,                                   /* bNumEndpoints */         \
    CDC_COMMUNICATION_INTERFACE_CLASS,      /* bInterfaceClass */       \
    CDC_ABSTRACT_CONTROL_MODEL,             /* bInterfaceSubClass */    \
    0x01,                                   /* bInterfaceProtocol */    \
    0x00,                                   /* iInterface */            \
/* Header Functional Descriptor */                                      \
    0x05,                                   /* bLength */               \
    CDC_CS_INTERFACE,                       /* bDescriptorType */       \
    CDC_HEADER,                             /* bDescriptorSubtype */    \
    WBVAL(CDC_V1_10), /* 1.10 */            /* bcdCDC */                \
/* Call Management Functional Descriptor */                             \
    0x05,                                   /* bFunctionLength */       \
    CDC_CS_INTERFACE,                       /* bDescriptorType */       \
    CDC_CALL_MANAGEMENT,                    /* bDescriptorSubtype */    \
    0x03,                                   /* bmCapabilities */        \
    datIfNum,                               /* bDataInterface */        \
/* Abstract Control Management Functional Descriptor */                 \
    0x04,                                   /* bFunctionLength */       \
    CDC_CS_INTERFACE,                       /* bDescriptorType */       \
    CDC_ABSTRACT_CONTROL_MANAGEMENT,        /* bDescriptorSubtype */    \
    0x02,                                   /* bmCapabilities */        \
/* Union Functional Descriptor */                                       \
    0x05,                                   /* bFunctionLength */       \
    CDC_CS_INTERFACE,                       /* bDescriptorType */       \
    CDC_UNION,                              /* bDescriptorSubtype */    \
    comIfNum,                               /* bMasterInterface */      \
    datIfNum,                               /* bSlaveInterface0 */      \
/* Endpoint, Interrupt IN */                /* event notification */    \
    USB_ENDPOINT_DESC_SIZE,                 /* bLength */               \
    USB_ENDPOINT_DESCRIPTOR_TYPE,           /* bDescriptorType */       \
    comInEp,                                /* bEndpointAddress */      \
    USB_ENDPOINT_TYPE_INTERRUPT,            /* bmAttributes */          \
    WBVAL(0x0040),                          /* wMaxPacketSize */        \
    0x01,                                   /* bInterval */             \
                                                                        \
/* CDC Data Interface Descriptor */                                     \
    USB_INTERFACE_DESC_SIZE,                /* bLength */               \
    USB_INTERFACE_DESCRIPTOR_TYPE,          /* bDescriptorType */       \
    datIfNum,                               /* bInterfaceNumber */      \
    0x00,                                   /* bAlternateSetting */     \
    0x02,                                   /* bNumEndpoints */         \
    CDC_DATA_INTERFACE_CLASS,               /* bInterfaceClass */       \
    0x00,                                   /* bInterfaceSubClass */    \
    0x00,                                   /* bInterfaceProtocol */    \
    0x00,                                   /* iInterface */            \
/* Endpoint, Bulk OUT */                                                \
    USB_ENDPOINT_DESC_SIZE,                 /* bLength */               \
    USB_ENDPOINT_DESCRIPTOR_TYPE,           /* bDescriptorType */       \
    datOutEp,                               /* bEndpointAddress */      \
    USB_ENDPOINT_TYPE_BULK,                 /* bmAttributes */          \
    WBVAL(VIRTUAL_COM_PORT_DATA_SIZE),      /* wMaxPacketSize */        \
    0x00,                                   /* bInterval */             \
/* Endpoint, Bulk IN */                                                 \
    USB_ENDPOINT_DESC_SIZE,                 /* bLength */               \
    USB_ENDPOINT_DESCRIPTOR_TYPE,           /* bDescriptorType */       \
    datInEp,                                /* bEndpointAddress */      \
    USB_ENDPOINT_TYPE_BULK,                 /* bmAttributes */          \
    WBVAL(VIRTUAL_COM_PORT_DATA_SIZE),      /* wMaxPacketSize */        \
    0x00                                    /* bInterval */


#define IAD_CDC_IF_DESC_SET_SIZE    ( 8 + CDC_IF_DESC_SET_SIZE )

#if 0
#define IAD_CDC_IF_DESC_SET( comIfNum, datIfNum, comInEp, datOutEp, datInEp )   \
/* Interface Association Descriptor */                                  \
    0x08,                                   /* bLength */               \
    0x0B,                                   /* bDescriptorType */       \
    comIfNum,                               /* bFirstInterface */       \
    0x02,                                   /* bInterfaceCount */       \
    CDC_COMMUNICATION_INTERFACE_CLASS,      /* bFunctionClass */        \
    CDC_ABSTRACT_CONTROL_MODEL,             /* bFunctionSubClass */     \
    0x01,                                   /* bFunctionProcotol */     \
    0x00,                                   /* iInterface */            \
/* CDC Interface descriptor set */                                      \
    CDC_IF_DESC_SET( comIfNum, datIfNum, comInEp, datOutEp, datInEp )
#endif

#define IAD_CDC_IF_DESC_SET( comIfNum, datIfNum, comInEp, datOutEp, datInEp )   \
/* Interface Association Descriptor */                                  \
    0x09,                                   /* bLength */               \
    0x04,                                   /* bDescriptorType */       \
    comIfNum,                               /* bFirstInterface */       \
    0x00
    0x02,                                   /* bInterfaceCount */       \
    0x0a,
    0x00,
    0x00,                                   /* bFunctionProcotol */     \
    0x00,                                   /* iInterface */            \
/* CDC Interface descriptor set */                                      \
    CDC_IF_DESC_SET( comIfNum, datIfNum, comInEp, datOutEp, datInEp )


// Interface numbers
enum {
	USB_CDC_CIF_NUM0,
	USB_CDC_DIF_NUM0,
#if 0
	USB_CDC_CIF_NUM1,
	USB_CDC_DIF_NUM1,
	USB_CDC_CIF_NUM2,
	USB_CDC_DIF_NUM2,
#endif

	USB_NUM_INTERFACES        // number of interfaces
};

static const unsigned char Virtual_Com_Port_ConfigDescriptor[] =  {
	/* Configuration 1 */
	USB_CONFIGURATION_DESC_SIZE,       /* bLength */
	USB_CONFIGURATION_DESCRIPTOR_TYPE, /* bDescriptorType */
	WBVAL(                             /* wTotalLength */
		(USB_CONFIGURATION_DESC_SIZE +
			3 * IAD_CDC_IF_DESC_SET_SIZE)
	),
	USB_NUM_INTERFACES,                /* bNumInterfaces */
	0x01,                              /* bConfigurationValue: 0x01 is used to select this configuration */
	0x00,                              /* iConfiguration: no string to describe this configuration */
	USB_CONFIG_BUS_POWERED /*|*/       /* bmAttributes */
	/*USB_CONFIG_REMOTE_WAKEUP*/,
	USB_CONFIG_POWER_MA(100),          /* bMaxPower, device power consumption is 100 mA */

	IAD_CDC_IF_DESC_SET( USB_CDC_CIF_NUM0, USB_CDC_DIF_NUM0, USB_ENDPOINT_IN(1), USB_ENDPOINT_OUT(2), USB_ENDPOINT_IN(2) ),
#if 0
	IAD_CDC_IF_DESC_SET( USB_CDC_CIF_NUM1, USB_CDC_DIF_NUM1, USB_ENDPOINT_IN(3), USB_ENDPOINT_OUT(4), USB_ENDPOINT_IN(4) ),
	IAD_CDC_IF_DESC_SET( USB_CDC_CIF_NUM2, USB_CDC_DIF_NUM2, USB_ENDPOINT_IN(5), USB_ENDPOINT_OUT(6), USB_ENDPOINT_IN(6) ),
#endif
};

#endif

static const unsigned char Virtual_Com_Port_ConfigDescriptor[] =  {
/*Configuration Descriptor*/
  0x09,   /* bLength: Configuration Descriptor size */
  USB_CONFIGURATION_DESCRIPTOR_TYPE,      /* bDescriptorType: Configuration */
  67,                /* wTotalLength:no of returned bytes */
  0x00,
  0x02,   /* bNumInterfaces: 2 interface */
  0x01,   /* bConfigurationValue: Configuration value */
  0x00,   /* iConfiguration: Index of string descriptor describing the configuration */
  0xC0,   /* bmAttributes: self powered */
  0x32,   /* MaxPower 0 mA */
/*Interface Descriptor */
  0x09,   /* bLength: Interface Descriptor size */
  USB_INTERFACE_DESCRIPTOR_TYPE,  /* bDescriptorType: Interface */
  /* Interface descriptor type */
  0x00,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  0x01,   /* bNumEndpoints: One endpoints used */
  0x02,   /* bInterfaceClass: Communication Interface Class */
  0x02,   /* bInterfaceSubClass: Abstract Control Model */
  0x01,   /* bInterfaceProtocol: Common AT commands */
  0x00,   /* iInterface: */
  /*Header Functional Descriptor*/
  0x05,   /* bLength: Endpoint Descriptor size */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x00,   /* bDescriptorSubtype: Header Func Desc */
  0x10,   /* bcdCDC: spec release number */
  0x01,
  /*Call Management Functional Descriptor*/
  0x05,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x01,   /* bDescriptorSubtype: Call Management Func Desc */
  0x00,   /* bmCapabilities: D0+D1 */
  0x01,   /* bDataInterface: 1 */
  /*ACM Functional Descriptor*/
  0x04,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x02,   /* bDescriptorSubtype: Abstract Control Management desc */
  0x02,   /* bmCapabilities */
  /*Union Functional Descriptor*/
  0x05,   /* bFunctionLength */
  0x24,   /* bDescriptorType: CS_INTERFACE */
  0x06,   /* bDescriptorSubtype: Union func desc */
  0x00,   /* bMasterInterface: Communication class interface */
  0x01,   /* bSlaveInterface0: Data Class Interface */
/*Endpoint 2 Descriptor*/
  0x07,                           /* bLength: Endpoint Descriptor size */
  USB_ENDPOINT_DESCRIPTOR_TYPE,   /* bDescriptorType: Endpoint */
  0x81,                     /* bEndpointAddress */
  0x03,                           /* bmAttributes: Interrupt */
  LOBYTE(0x0a),     /* wMaxPacketSize: */
  HIBYTE(0x00),
  0xFF,                           /* bInterval: */
 /*Data class interface descriptor*/
  0x09,   /* bLength: Endpoint Descriptor size */
  USB_INTERFACE_DESCRIPTOR_TYPE,  /* bDescriptorType: */
  0x01,   /* bInterfaceNumber: Number of Interface */
  0x00,   /* bAlternateSetting: Alternate setting */
  0x02,   /* bNumEndpoints: Two endpoints used */
  0x0A,   /* bInterfaceClass: CDC */
  0x00,   /* bInterfaceSubClass: */
  0x00,   /* bInterfaceProtocol: */
  0x00,   /* iInterface: */
 /*Endpoint OUT Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_ENDPOINT_DESCRIPTOR_TYPE,      /* bDescriptorType: Endpoint */
  0x2,                        /* bEndpointAddress */
  0x02,                              /* bmAttributes: Bulk */
  LOBYTE(0x40),  /* wMaxPacketSize: */
  HIBYTE(0x00),
  0x00,                              /* bInterval: ignore for Bulk transfer */
  /*Endpoint IN Descriptor*/
  0x07,   /* bLength: Endpoint Descriptor size */
  USB_ENDPOINT_DESCRIPTOR_TYPE,      /* bDescriptorType: Endpoint */
  0x82,                         /* bEndpointAddress */
  0x02,                              /* bmAttributes: Bulk */
  LOBYTE(0x40),  /* wMaxPacketSize: */
  HIBYTE(0x00),
  0x00                               /* bInterval: ignore for Bulk transfer */
};


#define TX_BSIZE 64
#define TX_BMASK (TX_BSIZE-1)
#define IX(a) (a&TX_BMASK)

#define RX_BSIZE 16


struct usb_data {
	char tx_buf[TX_BSIZE];
	volatile int tx_in;
	volatile int tx_out;
	int txr;
	char rx_buf[RX_BSIZE];
	volatile int rx_in;
	volatile int rx_out;
	struct sleep_obj rxblocker;
	struct sleep_obj txblocker;
	USB_OTG_CORE_HANDLE *core;
};

static struct usb_data usb_data0 = {
.rxblocker = { "usb_rxb",},
.txblocker = { "usb_txb",}
};


static struct usb_data *udata[8];


static unsigned char *get_device_descriptor(unsigned char speed, unsigned short *len) {
	io_printf("usb serial: get_device_descriptor, returning descriptor at %x, in len %d\n", device_descriptor,*len);
	*len=sizeof(device_descriptor);
	return (unsigned char *)device_descriptor;
}

unsigned char USBD_LangIDDesc[USB_SIZ_STRING_LANGID] = {
	USB_SIZ_STRING_LANGID,
	USB_DESC_TYPE_STRING,
	LOBYTE(USBD_LANGID_STRING),
	HIBYTE(USBD_LANGID_STRING),
};

static unsigned char *get_langIdStr_descriptor(unsigned char speed, unsigned short *len) {
	io_printf("usb_serial: get langId str descriptor\n");
	*len=sizeof(USBD_LangIDDesc);
	return USBD_LangIDDesc;
}

static unsigned char *get_manufacturerStr_descriptor(unsigned char speed, unsigned short *len) {
	io_printf("usb_serial: get manufacturer str descriptor\n");
	USBD_GetString(USBD_MANUFACTURER_STRING, USBD_StrDesc,len);
	return USBD_StrDesc;
}

static unsigned char *get_productStr_descriptor(unsigned char speed, unsigned short *len) {
	io_printf("usb_serial: get product str descriptor\n");
	if(!speed) {
		USBD_GetString(USBD_PRODUCT_HS_STRING,USBD_StrDesc,len);
	} else {
		USBD_GetString(USBD_PRODUCT_FS_STRING,USBD_StrDesc,len);
	}
	return USBD_StrDesc;
}

static unsigned char *get_serialStr_descriptor(unsigned char speed, unsigned short *len) {
	io_printf("usb_serial: get serial str descriptor\n");
	if (!speed) {
		USBD_GetString(USBD_SERIALNUMBER_HS_STRING,USBD_StrDesc,len);
	} else {
		USBD_GetString(USBD_SERIALNUMBER_FS_STRING,USBD_StrDesc,len);
	}
	return USBD_StrDesc;
}

static unsigned char *get_configStr_descriptor(unsigned char speed, unsigned short *len) {
	io_printf("usb_serial: get config str descriptor\n");
	*len=0;
	return 0;
}

static unsigned char *get_interfaceStr_descriptor(unsigned char speed, unsigned short *len) {
	io_printf("usb_serial: get interface str descriptor\n");
	*len=0;
	return 0;
}

USBD_DEVICE usbd_device = { get_device_descriptor,
				get_langIdStr_descriptor,
				get_manufacturerStr_descriptor,
				get_productStr_descriptor,
				get_serialStr_descriptor,
				get_configStr_descriptor,
				get_interfaceStr_descriptor};


static unsigned char class_init(void *core, unsigned char cfgidx) {
	io_printf("usb_serial: class init cfgidx=%d\n", cfgidx);


	DCD_EP_Open(core,
			USB_ENDPOINT_IN(1),
			64,
			USB_OTG_EP_INT);

	DCD_EP_Open(core,
			USB_ENDPOINT_IN(2),
			CDC_DATA_IN_PACKET_SIZE,
			USB_OTG_EP_BULK);

	DCD_EP_Open(core,
			USB_ENDPOINT_OUT(2),
			CDC_DATA_OUT_PACKET_SIZE,
			USB_OTG_EP_BULK);

	DCD_EP_Flush(core,0x82);
	DCD_EP_PrepareRx(core,
				USB_ENDPOINT_OUT(2),
				rx_buf[2],
				8);

	return 0;
}

static unsigned char class_deinit(void *core, unsigned char cfgidx) {
	io_printf("usb_serial: class deinit cfgidx=%d\n", cfgidx);
	return 0;
}

static unsigned char class_setup(void *core, struct usb_setup_req *req) {
	unsigned short int len=0;
	unsigned char *pbuf;

	io_printf("usb_serial: setup called\n");
	io_printf("bmRequest=%x, bRequest=%x\n", 
			req->bmRequest, req->bRequest);
	io_printf("wValue=%x, wIndex=%x, wLength=%x\n",
		req->wValue,req->wIndex,req->wLength);

	switch(req->bmRequest&USB_REQ_TYPE_MASK) {
		case USB_REQ_TYPE_CLASS:
			if (req->wLength) {
				if (req->bmRequest&0x80) {
					switch (req->bRequest) {
						case SEND_ENCAPSULATED_COMMAND:
							io_printf("got SEND_ENCAPSULATED_COMMAND\n");
							break;
						case GET_ENCAPSULATED_RESPONSE:
							io_printf("got GET_ENCAPSULATED_RESPONSE\n");
							break;
						case SET_COMM_FEATURE:
							io_printf("got SET_COMM_FEATURE\n");
							break;
						case GET_COMM_FEATURE:
							io_printf("got GET_COMM_FEATURE\n");
							break;
						case SET_LINE_CODING:
							io_printf("got SET_LINE_CODING\n");
							break;
						case GET_LINE_CODING:
							io_printf("got GET_LINE_CODING\n");
							break;
						case SET_CONTROL_LINE_STATE:
							io_printf("got SET_CONTROL_LINE_STATE\n");
							break;
						case SEND_BREAK:
							io_printf("got SEND_BREAK\n");
							break;
						default:
							io_printf("got unknown cmd %x\n", req->bRequest);
							break;
					}
					USBD_CtlSendData(core, CmdBuff, req->wLength);

					return 0;

				} else {
					cdcCmd=req->bRequest;
					cdcLen=req->wLength;
					USBD_CtlPrepareRx(core, CmdBuff, req->wLength);
				}
			} else { /* no data request */
			}
			return 0;
		default:
			USBD_CtlError (core, req);
			return -1;
		/* Standard Requests --------------------------------------*/
		case USB_REQ_TYPE_STANDARD:
			switch (req->bRequest) {
				case USB_REQ_GET_DESCRIPTOR:
					if ((req->wValue>>8)==CDC_DESCRIPTOR_TYPE) {
						pbuf = Virtual_Com_Port_ConfigDescriptor + 9 + (9 * USBD_ITF_MAX_NUM);
						len = MIN(USB_CDC_DESC_SIZ , req->wLength);
					}
					USBD_CtlSendData (core,
							pbuf,
							len);
					break;

				case USB_REQ_GET_INTERFACE:
					USBD_CtlSendData (core,
							(uint8_t *)&usbd_cdc_AltSet,
							1);
					break;

				case USB_REQ_SET_INTERFACE:
					if ((uint8_t)(req->wValue) < USBD_ITF_MAX_NUM) {
						usbd_cdc_AltSet = (uint8_t)(req->wValue);
					} else {
						USBD_CtlError (core, req);
					}
			}
	}
	return 0;
}

static unsigned char class_EP0_tx_sent(void *core) {
	io_printf("usb_serial: EP0_tx_send\n");
	return 0;
}

static unsigned char class_EP0_rx_ready(void *core) {
	io_printf("usb_serial: EP0_rx_ready\n");
	if (cdcCmd!=NO_CMD) {
		cdcCmd=NO_CMD;
	}
	return 0;
}

static unsigned char class_data_in(void *core, unsigned char epnum) {
	struct usb_data *ud=&usb_data0;
	if (ud->tx_in) {
		int len=MIN(ud->tx_in,64);
		DCD_EP_Tx(core,0x82,((unsigned char *)ud->tx_buf),len);
		ud->tx_in=ud->tx_out=0;
	} else {
	   ud->txr=0;
	}
	sys_wakeup(&usb_data0.txblocker,0,0);
	return 0;
}

static int usb_serial_putc(struct usb_data *ud, int c);

static unsigned char class_data_out(void *core, unsigned char epnum) {
	struct usb_data *ud=&usb_data0;
	unsigned int rx_cnt;
	int i;
//	rx_cnt=get_rx_cnt(core,epnum);
	rx_cnt=((USB_OTG_CORE_HANDLE*)core)->dev.out_ep[epnum].xfer_count;
	rx_buf[epnum][rx_cnt]=0;
	for(i=0;i<rx_cnt;i++) {
		usb_data0.rx_buf[usb_data0.rx_in%RX_BSIZE]=rx_buf[epnum][i];
		usb_data0.rx_in++;
		usb_serial_putc(ud,rx_buf[epnum][i]);
	}
	sys_wakeup(&usb_data0.rxblocker,0,0);


	DCD_EP_PrepareRx(core,epnum,
				rx_buf[epnum],8);

	return 0;
}

static unsigned char class_sof(void *core) {
//	io_printf("usb_serial: sof\n");
	return 0;
}

static unsigned char class_iso_in_incomplete(void *core) {
	io_printf("usb_serial: iso_in_incomplete\n");
	return 0;
}

static unsigned char class_iso_out_incomplete(void *core) {
	io_printf("usb_serial: iso_out_incomplete\n");
	return 0;
}

static unsigned char *class_get_config_descriptor(unsigned char speed, unsigned short *len) {
	io_printf("usb_serial: get_config_descriptor\n");
	*len=sizeof(Virtual_Com_Port_ConfigDescriptor);
	return ((unsigned char *)Virtual_Com_Port_ConfigDescriptor);
}

USBD_Class_cb_TypeDef usbd_class_cb = {
			class_init,
			class_deinit,
			class_setup,
			class_EP0_tx_sent,
			class_EP0_rx_ready,
			class_data_in,
			class_data_out,
			class_sof,
			class_iso_in_incomplete,
			class_iso_out_incomplete,
			class_get_config_descriptor
};


static void usr_init(void) {
	io_printf("usb_serial: usr_init\n");
}

static void usr_dev_reset(unsigned char speed) {
	io_printf("usb_serial: usr_dev_reset\n");
}

unsigned int tx_started;
static void usr_dev_configured(void) {
	io_printf("usb_serial: usb_dev_configured\n");
	tx_started=1;
	class_data_in(usb_data0.core, 0x82);
}

static void usr_dev_suspended(void) {
	io_printf("usb_serial: usb_dev_suspended\n");
}

static void usr_dev_resumed(void) {
	io_printf("usb_serial: usb_dev_resumed\n");
}

static void usr_dev_connected(void) {
	io_printf("usb_serial: usb_dev_connected\n");
}

static void usr_dev_disconnected(void) {
	io_printf("usb_serial: usb_dev_disconnected\n");
}


USBD_Usr_cb_TypeDef usbd_usr_cb = {
		usr_init,
		usr_dev_reset,
		usr_dev_configured,
		usr_dev_suspended,
		usr_dev_resumed,
		usr_dev_connected,
		usr_dev_disconnected
};

#if 0
static struct usb_dev usb_dev = {
		&usbd_device,
		&usbd_class_cb,
		&usbd_usr_cb
};
#endif

void USB_OTG_BSP_uDelay(unsigned int usec) {
	unsigned int count=0;
	unsigned int utime=(120*usec/7);
	do {
		if (++count>utime) {
			return;
		}
	} while(1);
}

void USB_OTG_BSP_mDelay(unsigned int msec) {
	USB_OTG_BSP_uDelay(msec*1000);
}

void USB_OTG_BSP_EnableInterrupt(USB_OTG_CORE_HANDLE *pdev) {
	NVIC_SetPriority(OTG_FS_IRQn,0xc);
	NVIC_EnableIRQ(OTG_FS_IRQn);
}

void USB_OTG_BSP_Init(USB_OTG_CORE_HANDLE *pdev) {
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

	GPIOA->MODER |= (0x2<<16);      /* Pin 8 to AF */
	GPIOA->OSPEEDR |= (0x3 << 16);  /* Pin 8 to High speed */
	GPIOA->OTYPER &= ~(1<<8);       /* Pin 8 to PP */
	GPIOA->PUPDR  &= ~(0x3<<16);    /* Pin 8 to NoPull */

	GPIOA->MODER |= (0x2<<18);      /* Pin 9 to AF */
	GPIOA->OSPEEDR |= (0x3 << 18);  /* Pin 9 to High speed */
	GPIOA->OTYPER &= ~(1<<9);       /* Pin 9 to PP */
	GPIOA->PUPDR  &= ~(0x3<<18);    /* Pin 9 to NoPull */

	GPIOA->MODER |= (0x2<<22);      /* Pin 11 to AF */
	GPIOA->OSPEEDR |= (0x3 << 22);  /* Pin 11 to High speed */
	GPIOA->OTYPER &= ~(1<<11);      /* Pin 11 to PP */
	GPIOA->PUPDR  &= ~(0x3<<22);    /* Pin 11 to NoPull */

	GPIOA->MODER |= (0x2<<24);      /* Pin 12 to AF */
	GPIOA->OSPEEDR |= (0x3 << 24);  /* Pin 12 to High speed */
	GPIOA->OTYPER &= ~(1<<12);      /* Pin 12 to PP */
	GPIOA->PUPDR  &= ~(0x3<<24);    /* Pin 12 to NoPull */

	GPIOA->AFR[1] |= 0x0000000a;    /* Pin 8 to AF_OTG_FS */
	GPIOA->AFR[1] |= 0x000000a0;    /* Pin 9 to AF_OTG_FS */
	GPIOA->AFR[1] |= 0x0000a000;    /* Pin 11 to AF_OTG_FS */
	GPIOA->AFR[1] |= 0x000a0000;    /* Pin 12 to AF_OTG_FS */

	GPIOA->MODER &= ~(0x3<<20);     /* Pin 10 to input */
	GPIOA->PUPDR  |= (0x1<<20);     /* Pin 10 to PullUp */
	GPIOA->AFR[1] |= 0x00000a00;    /* Pin 10 to AF_OTG_FS */


	RCC->APB2ENR|=RCC_APB2ENR_SYSCFGEN;
	RCC->AHB2ENR|=RCC_AHB2ENR_OTGFSEN;

	/* enable the PWR clock */
	RCC->APB1RSTR|=RCC_APB1RSTR_PWRRST;
}


static int usb_serial_read(struct usb_data *ud, char *buf, int len) {
	int i=0;
	while(i<len) {
		int ix=ud->rx_out%RX_BSIZE;
		int ch;
		disable_interrupt();
		if (!(ud->rx_in-ud->rx_out)) {
			sys_sleepon(&ud->rxblocker,0,0);
		}
		enable_interrupt();
		ud->rx_out++;
		ch=buf[i++]=ud->rx_buf[ix];
		if(ch==0x0d) {
			return i-1;
		}
	}
	return i;
}

static int usb_serial_putc(struct usb_data *ud, int c) {
	disable_interrupt();
	if ((ud->tx_in-ud->tx_out)>=TX_BSIZE) {
		sys_sleepon(&ud->txblocker,0,0);
	}
	ud->tx_buf[IX(ud->tx_in)]=c;
	ud->tx_in++;
	enable_interrupt();
	if (!tx_started) return 1;
	if(!ud->txr) {
		int len=ud->tx_in;
		ud->tx_in=0;
		ud->txr=1;
		DCD_EP_Tx(ud->core,0x82,((unsigned char *)ud->tx_buf),len);
	}
	return 1;
}

static int usb_serial_write(struct usb_data *ud, char *buf, int len) {
	int i;
	for(i=0;i<len;i++) {
		usb_serial_putc(ud,buf[i]);
		if (buf[i]=='\n') {
			usb_serial_putc(ud,'\r');
		}
	}
	return len;
}


static int usb_serial_open(void *instance, DRV_CBH cb, void *dum) {
	int i=0;
	for(i=0;i<(sizeof(udata)/sizeof(udata[0]));i++) {
		if (udata[i]==0) break;
	}
	if (i==sizeof(udata)) return -1;
	udata[i]=instance;
	return i;
}

static int usb_serial_close(int kfd) {
	return 0;
}

static int usb_serial_control(int kfd, int cmd, void *arg, int arg_len) {
	struct usb_data *ud=udata[kfd];
	switch(cmd) {
		case RD_CHAR:
			return usb_serial_read(ud,arg,arg_len);
		case WR_CHAR:
			return usb_serial_write(ud,arg,arg_len);
		default:
			return -1;
	}
	return 0;
}

static int usb_serial_init(void *instance) {
	return 0;
}

static int usb_serial_start(void *instance) {
	struct usb_data *ud=(struct usb_data *)instance;
#if 0
	usb_core=driver_lookup("usb_core");
	if (!usb_core) {
		io_printf("usb_serial: failed to lookup usb_core\n");
		return -1;
	}
	core_fd=usb_core->ops->open(usb_core->instance,0,&usb_dev);
	if (core_fd==-1) {
		io_printf("usb_serial: failed to open usb_cure\n");
		return -1;
	}
#endif
	ud->core=&USB_OTG_dev;
	USBD_Init(&USB_OTG_dev,
			USB_OTG_FS_CORE_ID,
			&usbd_device,
			&usbd_class_cb,
			&usbd_usr_cb);
	return 0;
}

static struct driver_ops usb_serial_ops = {
	usb_serial_open,
	usb_serial_close,
	usb_serial_control,
	usb_serial_init,
	usb_serial_start,
};

static struct driver usb_serial_drv0 = {
	"usb_serial0",
	&usb_data0,
	&usb_serial_ops,
};

void init_usb_serial_drv(void) {
	driver_publish(&usb_serial_drv0);
}

INIT_FUNC(init_usb_serial_drv);
