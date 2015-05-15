#include <string.h>

#include "usb_core.h"
#include "usb_device.h"




void usb_dev_init(struct usb_dev_handle *pdev) {
	unsigned int i;
	struct usb_dev_ep *ep;

	pdev->dev.dev_status=USB_DEV_STAT_DEFAULT;
	pdev->dev.dev_addr=0;

	for(i=0;i<pdev->cfg.dev_endpoints;i++) {
		ep=&pdev->dev.in_ep[i];
		ep->is_in=1;
		ep->num=i;
		ep->tx_fifo_num=i;

		ep->type=EP_TYPE_CTRL;
		ep->maxpacket=USB_DEV_MAX_EP0_SIZE;
		ep->xfer_buf=0;
		ep->xfer_len=0;
	}
	
	for(i=0;i<pdev->cfg.dev_endpoints;i++) {
		ep=&pdev->dev.out_ep[i];
		ep->is_in=0;
		ep->num=i;
		ep->tx_fifo_num=i;

		ep->type=EP_TYPE_CTRL;
		ep->maxpacket=USB_DEV_MAX_EP0_SIZE;
		ep->xfer_buf=0;
		ep->xfer_len=0;
	}

	usb_core_disable_gint(pdev->regs);
	usb_core_init_core(pdev->regs,&pdev->cfg);
	usb_core_set_mode(pdev->regs, DEVICE_MODE);
	usb_core_dev_core_init(pdev);
	usb_core_enable_gint(pdev->regs);
}

unsigned int usb_dev_ep_open(struct usb_dev_handle *pdev,
					unsigned char ep_addr,
					unsigned short int ep_max_packet_size,
					unsigned char ep_type) {
	struct usb_dev_ep *ep;

//	sys_printf("usb_dev_ep_open: epnum %d, max packet size %d, ep type %d\n", ep_addr, ep_max_packet_size, ep_type);
	if (ep_addr&0x80) {
		ep=&pdev->dev.in_ep[ep_addr&0x7f];
		ep->is_in=1;
	} else {
		ep=&pdev->dev.out_ep[ep_addr&0x7f];
		ep->is_in=0;
	}
	ep->num=ep_addr&0x7f;
	ep->maxpacket=ep_max_packet_size;
	ep->type=ep_type;
	if(ep->is_in) {
		ep->tx_fifo_num=ep->num;
	}
	if (ep_type==EP_TYPE_BULK) {
		ep->data_pid_start=0;
	}
	
	usb_core_dev_EP_activate(pdev->regs,ep);
	return 0;
}

static unsigned int usb_dev_ep_close(struct usb_dev_handle *pdev,
					unsigned char ep_addr) {
	struct usb_dev_ep *ep;

	if (ep_addr&0x80) {
		ep=&pdev->dev.in_ep[ep_addr&0x7f];
		ep->is_in=1;
	} else {
		ep=&pdev->dev.out_ep[ep_addr&0x7f];
		ep->is_in=0;
	}
	ep->num=ep_addr&0x7f;

	usb_core_dev_EP_deactivate(pdev->regs,ep);
	return 0;
}

unsigned int usb_dev_prepare_rx(struct usb_dev_handle *pdev,
					unsigned char ep_addr,
					unsigned char *buf,
					unsigned short int buf_len) {
	struct usb_dev_ep *ep;

	ep=&pdev->dev.out_ep[ep_addr&0x7f];

	ep->xfer_buf=buf;
	ep->xfer_len=buf_len;
	ep->xfer_count=0;
	ep->is_in=0;
	ep->num=ep_addr&0x7f;

	if (pdev->cfg.dma_enable) {
		ep->dma_addr=(unsigned int)buf;
	}

	if (ep->num==0) {
		usb_core_dev_EP0_start_xfer(pdev,ep);
	} else {
		usb_core_dev_EP_start_xfer(pdev,ep);
	}
	return 0;
}

unsigned int usb_dev_tx(struct usb_dev_handle *pdev,
				unsigned char ep_addr,
				unsigned char *buf,
				unsigned int buf_len) {
	struct usb_dev_ep *ep;

	ep=&pdev->dev.in_ep[ep_addr&0x7f];

	ep->is_in=1;
	ep->num=ep_addr&0x7f;
	ep->xfer_buf=buf;
	ep->dma_addr=(unsigned int)buf;
	ep->xfer_count=0;
	ep->xfer_len=buf_len;

	if (ep->num==0) {
		usb_core_dev_EP0_start_xfer(pdev,ep);
	} else {
		usb_core_dev_EP_start_xfer(pdev,ep);
	}
	return 0;
}

unsigned int usb_dev_ep_stall(struct usb_dev_handle *pdev,
				unsigned char epnum) {
	struct usb_dev_ep *ep;
	if(epnum&0x80) {
		ep=&pdev->dev.in_ep[epnum&0x7f];
	} else {
		ep=&pdev->dev.out_ep[epnum];
	}

	usb_core_dev_EP_set_stall(pdev->regs,ep);
	return 0;
}

unsigned int usb_dev_ep_clr_stall(struct usb_dev_handle *pdev,
					unsigned char epnum) {
	struct usb_dev_ep *ep;
	if(epnum&0x80) {
		ep=&pdev->dev.in_ep[epnum&0x7f];
		ep->is_in=1;
	} else {
		ep=&pdev->dev.out_ep[epnum];
		ep->is_in=0;
	}

	ep->is_stall=0;
	ep->num=epnum&0x7f;

	usb_core_dev_EP_clr_stall(pdev->regs,ep);
	return 0;
}

unsigned int usb_dev_ep_flush(struct usb_dev_handle *pdev,
				unsigned char epnum) {

	if (epnum&0x80) {
		usb_core_flush_tx_fifo(pdev->regs,epnum&0x7f);
	} else {
		usb_core_flush_rx_fifo(pdev->regs);
	}
	return 0;
}

void usb_dev_set_address(struct usb_dev_handle *pdev,
				unsigned char address) {
	usb_core_dev_set_address(pdev->regs,address);
}

static void usb_dev_connect(struct usb_dev_handle *pdev) {
	usb_core_dev_connect(pdev->regs);
}

static void usb_dev_disconnect(struct usb_dev_handle *pdev) {
	usb_core_dev_disconnect(pdev->regs);
}

static unsigned int usb_dev_get_ep_status(struct usb_dev_handle *pdev,
					unsigned char epnum) {
	struct usb_dev_ep *ep;

	if(epnum&0x80) {
		ep=&pdev->dev.in_ep[epnum&0x7f];
	} else {
		ep=&pdev->dev.out_ep[epnum];
	}
	return usb_core_dev_get_ep_status(pdev->regs, ep);
}

static void usb_dev_set_ep_status(struct usb_dev_handle *pdev,
				unsigned char epnum, unsigned int status) {
	struct usb_dev_ep *ep;

	if(epnum&0x80) {
		ep=&pdev->dev.in_ep[epnum&0x7f];
	} else {
		ep=&pdev->dev.out_ep[epnum];
	}
	usb_core_dev_set_ep_status(pdev->regs,ep,status);
}

unsigned int usb_dev_get_string(unsigned char *desc, 
				unsigned char *unicode, 
				unsigned short *len) {
	unsigned char idx=0;
	
	if (desc) {
		*len=(strlen(desc)*2)+2;
		unicode[idx++]=*len;
		unicode[idx++]=USB_DESC_TYPE_STRING;

		while(*desc) {
			unicode[idx++]=*desc++;
			unicode[idx++]=0x00;
		}
	}
	return 0;
}

#if 0
unsigned int usb_dev_ctl_send_data(struct usb_dev_handle *pdev,
				unsigned char *buf,
				unsigned short int len) {

	pdev->dev.in_ep[0].total_data_len=len;
	pdev->dev.in_ep[0].rem_data_len=len;
	pdev->dev.dev_state=USB_DEV_EP0_DATA_IN;

	usb_dev_tx(pdev, 0, buf, len);
	return 0;
}
#endif

unsigned int usb_dev_ctl_prepare_rx(struct usb_dev_handle *pdev,
				unsigned char *buf,
				unsigned short int len) {

	pdev->dev.out_ep[0].total_data_len=len;
	pdev->dev.out_ep[0].rem_data_len=len;
	pdev->dev.dev_state=USB_DEV_EP0_DATA_OUT;

	usb_dev_prepare_rx(pdev,0,buf,len);
	return 0;
}

#if 0
void usb_dev_ctl_error(struct usb_dev_handle *pdev,
			struct usb_setup_req *req) {
	usb_dev_ep_stall(pdev, 0x80);
	usb_dev_ep_stall(pdev, 0x00);
	dev_EP0_out_start(pdev);
}
#endif

