
#include "sys.h"
#include "io.h"

#include "usb_core_drv.h"
#include "usb_serial_drv.h"

/* Exported define -----------------------------------------------------------*/
#define USB_DEVICE_DESCRIPTOR_TYPE              0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE       0x02
#define USB_STRING_DESCRIPTOR_TYPE              0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE           0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE            0x05

#define VIRTUAL_COM_PORT_DATA_SIZE              64
//#define VIRTUAL_COM_PORT_INT_SIZE               8 //FIXME

#define USB_CONFIGURATION_DESC_SIZE             9   // Standard of USB
#define USB_INTERFACE_DESC_SIZE                 9   // Standard of USB
#define USB_ENDPOINT_DESC_SIZE                  7   // Standard of USB

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
 

static struct driver *usb_core;
static int core_fd;


static const unsigned char device_descriptor[] = {
    0x12,   /* bLength */
    USB_DEVICE_DESCRIPTOR_TYPE,     /* bDescriptorType */
    0x00,
    0x02,   /* bcdUSB = 2.00 */
    0xEF,   /* bDeviceClass    (Misc)   */
    0x02,   /* bDeviceSubClass (common) */
    0x01,   /* bDeviceProtocol (IAD)    */
    0x08,   /* bMaxPacketSize0 */        // <-----------
    0x83,
    0x02,   /* idVendor = 0xFF02 */
    0xFF,
    0x01,   /* idProduct = 0x0001 */
    0x00,
    0x02,   /* bcdDevice = 2.00 */
    1,              /* Index of string descriptor describing manufacturer */
    2,              /* Index of string descriptor describing product */
    3,              /* Index of string descriptor describing the device's serial number */
    0x01    /* bNumConfigurations */
};
	

static unsigned char *get_device_descriptor(unsigned char speed, unsigned short *len) {
	io_printf("usb serial: get_device_descriptor, returning descriptor at %x\n", device_descriptor);
	*len=sizeof(device_descriptor);
	return device_descriptor;
}

static unsigned char *get_langIdStr_descriptor(unsigned char speed, unsigned short *len) {
	io_printf("usb_serial: get langId str descriptor\n");
	*len=0;
	return 0;
}

static unsigned char *get_manufacturerStr_descriptor(unsigned char speed, unsigned short *len) {
	io_printf("usb_serial: get manufacturer str descriptor\n");
	*len=0;
	return 0;
}

static unsigned char *get_productStr_descriptor(unsigned char speed, unsigned short *len) {
	io_printf("usb_serial: get product str descriptor\n");
	*len=0;
	return 0;
}

static unsigned char *get_serialStr_descriptor(unsigned char speed, unsigned short *len) {
	io_printf("usb_serial: get serial str descriptor\n");
	*len=0;
	return 0;
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

static struct usbd_device usbd_device = { get_device_descriptor,
					get_langIdStr_descriptor,
					get_manufacturerStr_descriptor,
					get_productStr_descriptor,
					get_serialStr_descriptor,
					get_configStr_descriptor,
					get_interfaceStr_descriptor};


static unsigned char class_init(void *core, unsigned char cfgidx) {
	io_printf("usb_serial: class init cfgidx=%d\n", cfgidx);
	return 0;
}

static unsigned char class_deinit(void *core, unsigned char cfgidx) {
	io_printf("usb_serial: class deinit cfgidx=%d\n", cfgidx);
	return 0;
}

static unsigned char class_setup(void *core, struct usb_setup_req *req) {
	io_printf("usb_serial: setup called\n");
	return 0;
}

static unsigned char class_EP0_tx_sent(void *core) {
	io_printf("usb_serial: EP0_tx_send\n");
	return 0;
}

static unsigned char class_EP0_rx_ready(void *core) {
	io_printf("usb_serial: EP0_rx_ready\n");
	return 0;
}

static unsigned char class_data_in(void *core, unsigned char epnum) {
	io_printf("usb_serial: data in, epnum %d\n", epnum);
	return 0;
}

static unsigned char class_data_out(void *core, unsigned char epnum) {
	io_printf("usb_serial: data_out, epnum %d\n", epnum);
	return 0;
}

static unsigned char class_sof(void *core) {
	io_printf("usb_serial: sof\n");
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
	return 0;
}

static struct usbd_class_cb usbd_class_cb = {
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

static void usr_dev_configured(void) {
	io_printf("usb_serial: usb_dev_configured\n");
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


struct usbd_usr_cb usbd_usr_cb = {
		usr_init,
		usr_dev_reset,
		usr_dev_configured,
		usr_dev_suspended,
		usr_dev_resumed,
		usr_dev_connected,
		usr_dev_disconnected
};

static struct usb_dev usb_dev = {
		&usbd_device,
		&usbd_class_cb,
		&usbd_usr_cb
};


static struct data {
	int users[8];
} usb_serial_data;

static int usb_serial_open(void *instance, DRV_CBH cb, void *dum) {
	return -1;
}

static int usb_serial_close(int kfd) {
	return 0;
}

static int usb_serial_control(int kfd, int cmd, void *arg, int arg_len) {
	return 0;
}

static int usb_serial_init(void *instance) {
	return 0;
}

static int usb_serial_start(void *instance) {
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
	return 0;
}

static struct driver_ops usb_serial_ops = {
	usb_serial_open,
	usb_serial_close,
	usb_serial_control,
	usb_serial_init,
	usb_serial_start,
};

static struct driver usb_serial_drv = {
	"usb_serial",
	&usb_serial_data,
	&usb_serial_ops,
};

void init_usb_serial_drv(void) {
	driver_publish(&usb_serial_drv);
}

INIT_FUNC(init_usb_serial_drv);
