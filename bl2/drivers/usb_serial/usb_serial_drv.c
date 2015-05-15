/* $Leanaux: , v1.1 2014/04/07 21:44:00 anders Exp $ */

/*
 * Copyright (c) 2014, Anders Franzen.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @(#)usb_serial_drv.c
 */
#include <sys.h>
#include <io.h>
#include <string.h>

#include <gpio_drv.h>
#include <devices.h>

#include <usb_core.h>
#include <usb_device.h>

#include "usbd_cdc_core.h"

#include "usb_serial_drv.h"

#define MIN(a,b) (a<b?a:b)

#define USB_DEV_ITF_MAX_NUM 1

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

#define USBD_PRODUCT_FS_STRING		((unsigned char *)"Lionel Groulx v 1.0")
#define USBD_PRODUCT_HS_STRING		((unsigned char *)"HS Njet")

#define USBD_SERIALNUMBER_FS_STRING	((unsigned char *)"00000000011C")
#define USBD_SERIALNUMBER_HS_STRING	((unsigned char *)"00000000011B")

#define USB_MAX_STR_DESC_SIZ       64


//static int core_fd;

static unsigned char rx_buf[8][8];

unsigned char CmdBuff[64];
static unsigned int cdcCmd=0xFF;
static unsigned int cdcLen=0;
static volatile unsigned int usbd_cdc_AltSet;

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
    0x48,
    0x25,   /* idVendor = 0x2548 */
    0x01,
    0x10,   /* idProduct = 0x1001 */
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
  USB_CONFIG_BUS_POWERED|USB_CONFIG_REMOTE_WAKEUP,   /* bmAttributes: self powered */
  USB_CONFIG_POWER_MA(100),   /* MaxPower 0 mA */
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

#define RX_BSIZE 512


struct usb_data {
	char tx_buf[TX_BSIZE];
	volatile int tx_in;
	volatile int tx_out;
	int txr;
	char rx_buf[RX_BSIZE];
	volatile int rx_in;
	volatile int rx_out;
	unsigned char str_buf[USB_MAX_STR_DESC_SIZ];
	struct usb_dev_handle *core;
};

static struct usb_data usb_data0;

struct user_data {
	struct device_handle dh;
	struct usb_data *usb_data;
	void 		*userdata;
	DRV_CBH		callback;
	int		ev_flags;
};


#define MAX_USERS 4
static struct user_data udata[MAX_USERS];

static struct user_data *get_user_data() {
	int i;
	for(i=0;i<MAX_USERS;i++) {
		if (!udata[i].usb_data) {
			return &udata[i];
		}
	}
	return 0;
}

static void wakeup_users(unsigned int ev) {
	int i;

	for(i=0;i<MAX_USERS;i++) {
		unsigned int rev;
		if ((udata[i].usb_data)&&
			(udata[i].callback) &&
			(rev=udata[i].ev_flags&ev)) {
			udata[i].ev_flags&=~ev;
			udata[i].callback(&udata[i].dh,rev,udata[i].userdata);
		}
	}
}


static unsigned char *get_device_descriptor(void *uref, unsigned char speed, unsigned short *len) {
//	sys_printf("usb serial: get_device_descriptor, returning descriptor at %x, in len %d\n", device_descriptor,*len);
	*len=sizeof(device_descriptor);
	return (unsigned char *)device_descriptor;
}

unsigned char usb_dev_lang_id_desc[USB_SIZ_STRING_LANGID] = {
	USB_SIZ_STRING_LANGID,
	USB_DESC_TYPE_STRING,
	LOBYTE(USBD_LANGID_STRING),
	HIBYTE(USBD_LANGID_STRING),
};

static unsigned char *get_langIdStr_descriptor(void *ud, unsigned char speed, unsigned short *len) {
//	sys_printf("usb_serial: get langId str descriptor\n");
	*len=sizeof(usb_dev_lang_id_desc);
	return usb_dev_lang_id_desc;
}

static unsigned char *get_manufacturerStr_descriptor(void *uref, unsigned char speed, unsigned short *len) {
	struct usb_data *ud=(struct usb_data *)uref;
//	sys_printf("usb_serial: get manufacturer str descriptor\n");
	usb_dev_get_string(USBD_MANUFACTURER_STRING, ud->str_buf,len);
	return ud->str_buf;
}

static unsigned char *get_productStr_descriptor(void *uref, unsigned char speed, unsigned short *len) {
	struct usb_data *ud=(struct usb_data *)uref;
//	sys_printf("usb_serial: get product str descriptor\n");
	if(!speed) {
		usb_dev_get_string(USBD_PRODUCT_HS_STRING,ud->str_buf,len);
	} else {
		usb_dev_get_string(USBD_PRODUCT_FS_STRING,ud->str_buf,len);
	}
	return ud->str_buf;
}

static unsigned char *get_serialStr_descriptor(void *uref, unsigned char speed, unsigned short *len) {
	struct usb_data *ud=(struct usb_data *)uref;
//	sys_printf("usb_serial: get serial str descriptor\n");
	if (!speed) {
		usb_dev_get_string(USBD_SERIALNUMBER_HS_STRING,ud->str_buf,len);
	} else {
		usb_dev_get_string(USBD_SERIALNUMBER_FS_STRING,ud->str_buf,len);
	}
	return ud->str_buf;
}

static unsigned char *get_configStr_descriptor(void *uref, unsigned char speed, unsigned short *len) {
//	sys_printf("usb_serial: get config str descriptor\n");
	*len=0;
	return 0;
}

static unsigned char *get_interfaceStr_descriptor(void *uref, unsigned char speed, unsigned short *len) {
//	sys_printf("usb_serial: get interface str descriptor\n");
	*len=0;
	return 0;
}

struct usb_device usb_device = { get_device_descriptor,
				get_langIdStr_descriptor,
				get_manufacturerStr_descriptor,
				get_productStr_descriptor,
				get_serialStr_descriptor,
				get_configStr_descriptor,
				get_interfaceStr_descriptor};


static unsigned int class_init(void *ud_v, unsigned char cfgidx) {
	struct usb_data *ud=(struct usb_data *)ud_v;

//	sys_printf("usb_serial: class init cfgidx=%d\n", cfgidx);


	usb_dev_ep_open(ud->core,
			USB_ENDPOINT_IN(1),
			64,
			EP_TYPE_IRQ);

	usb_dev_ep_open(ud->core,
			USB_ENDPOINT_IN(2),
			CDC_DATA_IN_PACKET_SIZE(ud->core),
			EP_TYPE_BULK);

	usb_dev_ep_open(ud->core,
			USB_ENDPOINT_OUT(2),
			CDC_DATA_OUT_PACKET_SIZE(ud->core),
			EP_TYPE_BULK);

	usb_dev_ep_flush(ud->core,0x82);
	usb_dev_prepare_rx(ud->core,
				USB_ENDPOINT_OUT(2),
				rx_buf[2],
				8);

	return 0;
}

static unsigned int class_deinit(void *ud, unsigned char cfgidx) {
//	sys_printf("usb_serial: class deinit cfgidx=%d\n", cfgidx);
	return 0;
}

static unsigned int class_setup(void *ud_v, struct usb_setup_req *req) {
	struct usb_data *ud=(struct usb_data *)ud_v;
	unsigned short int len=0;
	unsigned char *pbuf;

//	sys_printf("usb_serial: setup called\n");

	switch(req->bmRequest&USB_REQ_TYPE_MASK) {
		case USB_REQ_TYPE_CLASS:
			if (req->wLength) {
				if (req->bmRequest&0x80) {
					switch (req->bRequest) {
						case SEND_ENCAPSULATED_COMMAND:
//							sys_printf("got SEND_ENCAPSULATED_COMMAND\n");
							break;
						case GET_ENCAPSULATED_RESPONSE:
//							sys_printf("got GET_ENCAPSULATED_RESPONSE\n");
							break;
						case SET_COMM_FEATURE:
//							sys_printf("got SET_COMM_FEATURE\n");
							break;
						case GET_COMM_FEATURE:
//							sys_printf("got GET_COMM_FEATURE\n");
							break;
						case SET_LINE_CODING:
//							sys_printf("got SET_LINE_CODING\n");
							break;
						case GET_LINE_CODING:
//							sys_printf("got GET_LINE_CODING\n");
							break;
						case SET_CONTROL_LINE_STATE:
//							sys_printf("got SET_CONTROL_LINE_STATE\n");
							break;
						case SEND_BREAK:
//							sys_printf("got SEND_BREAK\n");
							break;
						default:
							sys_printf("got unknown cmd %x\n", req->bRequest);
							break;
					}
					usbd_ctl_send_data(ud->core, CmdBuff, req->wLength);

					return 0;

				} else {
					cdcCmd=req->bRequest;
					cdcLen=req->wLength;
					usb_dev_ctl_prepare_rx(ud->core, CmdBuff, req->wLength);
				}
			} else { /* no data request */
			}
			return 0;
		default:
			usbd_ctl_error (ud->core, req);
			return -1;
		/* Standard Requests --------------------------------------*/
		case USB_REQ_TYPE_STANDARD:
			switch (req->bRequest) {
				case USB_REQ_GET_DESCRIPTOR:
					if ((req->wValue>>8)==CDC_DESCRIPTOR_TYPE) {
						pbuf = (unsigned char *)Virtual_Com_Port_ConfigDescriptor + 9 + (9 * USB_DEV_ITF_MAX_NUM);
						len = MIN(USB_CDC_DESC_SIZ , req->wLength);
					}
					usbd_ctl_send_data(ud->core,
							pbuf,
							len);
					break;

				case USB_REQ_GET_INTERFACE:
					usbd_ctl_send_data(ud->core,
							(unsigned char  *)&usbd_cdc_AltSet,
							1);
					break;

				case USB_REQ_SET_INTERFACE:
					if ((unsigned char)(req->wValue) < USB_DEV_ITF_MAX_NUM) {
						usbd_cdc_AltSet = (unsigned char)(req->wValue);
					} else {
						usbd_ctl_error (ud->core, req);
					}
			}
	}
	return 0;
}

static unsigned int class_EP0_tx_sent(void *ud_v) {
//	sys_printf("usb_serial: EP0_tx_send\n");
	return 0;
}

static unsigned int class_EP0_rx_ready(void *ud_v) {
//	sys_printf("usb_serial: EP0_rx_ready\n");
	if (cdcCmd!=NO_CMD) {
		cdcCmd=NO_CMD;
	}
	return 0;
}

static unsigned int class_data_in(void *ud_v, unsigned char epnum) {
	struct usb_data *ud=(struct usb_data *)ud_v;
	if (ud->tx_in) {
		int len=MIN(ud->tx_in,64);
		usb_dev_tx(ud->core,0x82,((unsigned char *)ud->tx_buf),len);
		ud->tx_in=ud->tx_out=0;
	} else {
	   ud->txr=0;
	}
	wakeup_users(EV_WRITE);
	return 0;
}

static int usb_serial_putc(struct user_data *ud, int c);

static unsigned int class_data_out(void *ud_v, unsigned char epnum) {
	struct usb_data *ud=(struct usb_data *)ud_v;
	unsigned int rx_cnt;
	int i;
	rx_cnt=ud->core->dev.out_ep[epnum].xfer_count;
	rx_buf[epnum][rx_cnt]=0;

	for(i=0;i<rx_cnt;i++) {
		if ((usb_data0.rx_in-usb_data0.rx_out)>=RX_BSIZE) {
			sys_printf("usb_serial: rx overrun\n");
		}

		usb_data0.rx_buf[usb_data0.rx_in%RX_BSIZE]=rx_buf[epnum][i];
		usb_data0.rx_in++;
	}


	usb_dev_prepare_rx(ud->core,epnum,
				rx_buf[epnum],8);

	wakeup_users(EV_READ);
	return 0;
}

static unsigned int class_sof(void *ud_v) {
	struct usb_data *ud=(struct usb_data *)ud_v;
//	USB_OTG_GINTMSK_TypeDef int_mask;
	ud->core->regs->core.g_int_msk&=~G_INT_STS_SOF;
//	int_mask.d32=0;
//	int_mask.b.sofintr=1;
//	USB_OTG_MODIFY_REG32(&ud->core->regs.GREGS->GINTMSK,int_mask.d32,0);
	return 0;
}

static unsigned int class_iso_in_incomplete(void *ud_v) {
//	sys_printf("usb_serial: iso_in_incomplete\n");
	return 0;
}

static unsigned int class_iso_out_incomplete(void *ud_v) {
//	sys_printf("usb_serial: iso_out_incomplete\n");
	return 0;
}

static unsigned char *class_get_config_descriptor(void *ud_v, unsigned char speed, unsigned short *len) {
//	sys_printf("usb_serial: get_config_descriptor\n");
	*len=sizeof(Virtual_Com_Port_ConfigDescriptor);
	return ((unsigned char *)Virtual_Com_Port_ConfigDescriptor);
}

struct usb_dev_class usbd_class_cb = {
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


static void usr_dev_init(void) {
//	sys_printf("usb_serial: usr_dev_init\n");
}

static void usr_dev_reset(unsigned char speed) {
//	sys_printf("usb_serial: usr_dev_reset\n");
}

unsigned int tx_started;
static void usr_dev_configured(void) {
//	sys_printf("usb_serial: usb_dev_configured\n");
	tx_started=1;
	class_data_in(&usb_data0, 0x82);
}

static void usr_dev_suspended(void) {
//	sys_printf("usb_serial: usb_dev_suspended\n");
}

static void usr_dev_resumed(void) {
//	sys_printf("usb_serial: usb_dev_resumed\n");
}

static void usr_dev_connected(void) {
//	sys_printf("usb_serial: usb_dev_connected\n");
}

static void usr_dev_disconnected(void) {
//	sys_printf("usb_serial: usb_dev_disconnected\n");
}


struct usb_dev_usr usb_dev_usr = {
		usr_dev_init,
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

#if 0
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
#endif

#if 0
void USB_OTG_BSP_Init(USB_OTG_CORE_HANDLE *pdev) {
	struct driver *pindrv=driver_lookup(GPIO_DRV);
	struct device_handle *pin_dh;
	struct pin_spec ps[4];
	unsigned int flags;

	if (!pindrv) return;
	pin_dh=pindrv->ops->open(pindrv->instance,0,0);
	if(!pin_dh) return;

//	ps[0].pin=USB_VBUS;
	ps[0].pin=USB_ID;
	flags=GPIO_DIR(0,GPIO_ALTFN_PIN);
//	flags=GPIO_DIR(0,GPIO_INPUT);
	flags=GPIO_SPEED(flags,GPIO_SPEED_HIGH);
	flags=GPIO_DRIVE(flags,GPIO_PUSHPULL);
	flags=GPIO_ALTFN(flags,0xa);
	ps[0].flags=flags;
//	ps[1].pin=USB_ID;
	ps[1].pin=USB_VBUS;
	flags=GPIO_DIR(0,GPIO_INPUT);
//	flags=GPIO_DRIVE(flags,GPIO_PULLUP);
//	flags=GPIO_SPEED(flags,GPIO_SPEED_HIGH);
//	flags=GPIO_ALTFN(flags,0xa);
	ps[1].flags=flags;
	ps[2].pin=USB_DM;
	flags=GPIO_DIR(0,GPIO_ALTFN_PIN);
	flags=GPIO_SPEED(flags,GPIO_SPEED_HIGH);
	flags=GPIO_DRIVE(flags,GPIO_PUSHPULL);
	flags=GPIO_ALTFN(flags,0xa);
	ps[2].flags=flags;
	ps[3].pin=USB_DP;
	flags=GPIO_DIR(0,GPIO_ALTFN_PIN);
	flags=GPIO_SPEED(flags,GPIO_SPEED_HIGH);
	flags=GPIO_DRIVE(flags,GPIO_PUSHPULL);
	flags=GPIO_ALTFN(flags,0xa);
	ps[3].flags=flags;

	pindrv->ops->control(pin_dh,GPIO_BUS_ASSIGN_PINS,ps,sizeof(ps));
	
#if 0
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

#endif

//	RCC->APB2ENR|=RCC_APB2ENR_SYSCFGEN;
/*	RCC->AHB2ENR|=RCC_AHB2ENR_OTGFSEN; */

	/* enable the PWR clock */
//	RCC->APB1RSTR|=RCC_APB1RSTR_PWRRST;
}
#endif


static int usb_serial_read(struct user_data *ud, char *buf, int len) {
	struct usb_data *usb_data=ud->usb_data;
	int i=0;
	while(i<len) {
		int ix=usb_data->rx_out%RX_BSIZE;
		if (!(usb_data->rx_in-usb_data->rx_out)) {
			if (!i) {
				ud->ev_flags|=EV_READ;
				return -DRV_AGAIN;
			} else {
				return i;
			}
		}
		buf[i++]=usb_data->rx_buf[ix];
		usb_data->rx_out++;
	}
	return i;
}

static int usb_serial_putc(struct user_data *ud, int c) {
	struct usb_data *usb_data=ud->usb_data;
	unsigned long int cpu_flags=disable_interrupts();
	if ((usb_data->tx_in-usb_data->tx_out)>=TX_BSIZE) {
		ud->ev_flags|=EV_WRITE;
		restore_cpu_flags(cpu_flags);
		return -DRV_AGAIN;
	}
	usb_data->tx_buf[IX(usb_data->tx_in)]=c;
	usb_data->tx_in++;
	restore_cpu_flags(cpu_flags);
	if (!tx_started) return 1;
	if(!usb_data->txr) {
		int len=usb_data->tx_in;
		usb_data->tx_in=0;
		usb_data->txr=1;
		usb_dev_tx(usb_data->core,0x82,((unsigned char *)usb_data->tx_buf),len);
	}
	return 1;
}

static int usb_serial_write(struct user_data *ud, char *buf, int len) {
	int i;
	for(i=0;i<len;i++) {
		if (usb_serial_putc(ud,buf[i])<0) return i;
	}
	return len;
}


static struct device_handle *usb_serial_open(void *instance, DRV_CBH cb, void *dum) {
	struct user_data *ud=get_user_data();
	if (!ud) return 0;

	ud->usb_data=instance;
	ud->callback=cb;
	ud->userdata=dum;
	ud->ev_flags=0;
	return &ud->dh;
}

static int usb_serial_close(struct device_handle *dh) {
	struct user_data *ud=(struct user_data *)dh;
	ud->usb_data=0;
	return 0;
}

static int usb_serial_control(struct device_handle *dh, int cmd, void *arg, int arg_len) {
	struct user_data *ud=(struct user_data *)dh;
	struct usb_data *usb_data=ud->usb_data;
	if (ud->usb_data!=&usb_data0) {
		ASSERT(0);
	}
	switch(cmd) {
		case RD_CHAR: {
			int rc=usb_serial_read(ud,arg,arg_len);
			return rc;
		}
		case WR_CHAR: {
			int rc=usb_serial_write(ud,arg,arg_len);
			return rc;
		}
		case IO_POLL: {
			unsigned int events=(unsigned int)arg;
			unsigned int revents=0;
			if (events&EV_WRITE) {
				if ((usb_data->tx_in-usb_data->tx_out)<TX_BSIZE) {
					revents|=EV_WRITE;
					ud->ev_flags&=~EV_WRITE;
				} else {
					ud->ev_flags|=EV_WRITE;
				}
			}
			if (events&EV_READ) {
				if ((usb_data->rx_in-usb_data->rx_out)) {
					revents|=EV_READ;
					ud->ev_flags&=~EV_READ;
				} else {
					ud->ev_flags|=EV_READ;
				}
			}
			return revents;
		}
		case USB_REMOTE_WAKEUP: {
			dev_remote_wakeup(usb_data->core);
			return 0;
			break;
		}
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

//	ud->core=&USB_OTG_dev;
	usbd_init(&ud->core,
			ud,
			&usb_device,
			&usbd_class_cb,
			&usb_dev_usr);
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
