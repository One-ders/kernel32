#include <devices.h>
#include <usb_core.h>
#include <usb_device.h>
#include <bsp_usb_otg.h>

#include <string.h>

struct usb_dev_handle usb_dev;

static unsigned int test_mode_data;

#define MIN(a,b) (a<b?a:b)

unsigned short ntohs(unsigned short a) {
	return (a<<8)|(a>>8);
}

void usbd_init(struct usb_dev_handle	**pdev,
		void			*uref,
		struct usb_device	*usb_udev,
		struct usb_dev_class	*class_cb,
		struct usb_dev_usr	*usr_cb) {

	*pdev=&usb_dev;
	(*pdev)->uref=uref;
	usb_config_core(*pdev);
	
	bsp_usb_init(*pdev);
	usbd_deinit(*pdev);

	(*pdev)->dev.class_cb=class_cb;
	(*pdev)->dev.usr_cb=usr_cb;
	(*pdev)->dev.usr_device=usb_udev;

	usb_dev_init(*pdev);

	(*pdev)->dev.usr_cb->init();

	bsp_usb_enable_interrupt(*pdev);
}

void usbd_deinit(struct usb_dev_handle *pdev) {
	return ;
}

int usbd_ctl_send_data(struct usb_dev_handle *pdev,
			void *buf,
			unsigned short int len) {
	pdev->dev.in_ep[0].total_data_len=len;
	pdev->dev.in_ep[0].rem_data_len=len;
	pdev->dev.dev_state=USB_DEV_EP0_DATA_IN;

	usb_dev_tx(pdev,0,buf,len);
	return 0;
}

int usbd_ctl_continue_rx(struct usb_dev_handle *pdev,
			void *buf,
			unsigned short int len) {
	usb_dev_prepare_rx(pdev,0,buf,len);
	return 0;
}

int usbd_ctl_continue_tx(struct usb_dev_handle *pdev,
			void *buf,
			unsigned short int len) {
	usb_dev_tx(pdev,0,buf,len);
	return 0;
}

int usbd_ctl_send_status(struct usb_dev_handle *pdev) {
	pdev->dev.dev_state=USB_DEV_EP0_STATUS_IN;
	usb_dev_tx(pdev,0,0,0);
	dev_EP0_out_start(pdev);
	return 0;
}

int usbd_ctl_receive_status(struct usb_dev_handle *pdev) {
	pdev->dev.dev_state = USB_DEV_EP0_STATUS_OUT;
	usb_dev_prepare_rx(pdev,0,0,0);
	dev_EP0_out_start(pdev);
	return 0;
}

void usbd_ctl_error(struct usb_dev_handle *pdev,
			struct usb_setup_req *req) {
	usb_dev_ep_stall(pdev,0x80);
	usb_dev_ep_stall(pdev,0);
	dev_EP0_out_start(pdev);
}

void usbd_get_descriptor(struct usb_dev_handle *pdev,
			struct usb_setup_req *req) {
	unsigned short int len;
	unsigned char *buf;

	switch(req->wValue>>8) {
		case USB_DESC_TYPE_DEVICE:
			buf=pdev->dev.usr_device->get_device_descriptor(pdev->uref, pdev->cfg.speed, &len);
			if ((req->wLength==64)||
				(pdev->dev.dev_status==USB_DEV_STAT_DEFAULT)) {
				len=8;
			}
			break;
		case USB_DESC_TYPE_CONFIGURATION:
			buf=pdev->dev.class_cb->get_config_descriptor(pdev->uref,
						pdev->cfg.speed,&len);
			buf[1]=USB_DESC_TYPE_CONFIGURATION;
			pdev->dev.config_descriptor=buf;
			break;
		case USB_DESC_TYPE_STRING:
			switch((unsigned char)req->wValue) {
				case USBD_IDX_LANGID_STR:
					buf=pdev->dev.usr_device->get_lang_id_descriptor(pdev->uref,pdev->cfg.speed,&len);
					break;
				case USBD_IDX_MFC_STR:
					buf=pdev->dev.usr_device->get_mfc_descriptor(pdev->uref,pdev->cfg.speed,&len);
					break;
				case USBD_IDX_PRODUCT_STR:
					buf=pdev->dev.usr_device->get_prod_descriptor(pdev->uref,pdev->cfg.speed,&len);
					break;
				case USBD_IDX_SERIAL_STR:
					buf=pdev->dev.usr_device->get_serial_descriptor(pdev->uref,pdev->cfg.speed,&len);
					break;
				case USBD_IDX_CONFIG_STR:
					buf=pdev->dev.usr_device->get_config_descriptor(pdev->uref,pdev->cfg.speed,&len);
					break;
				case USBD_IDX_INTERFACE_STR:
					buf=pdev->dev.usr_device->get_itf_descriptor(pdev->uref,pdev->cfg.speed,&len);
					break;
				default:
					usbd_ctl_error(pdev,req);
					return;
			}
			break;
		case USB_DESC_TYPE_DEVICE_QUALIFIER:
			usbd_ctl_error(pdev,req);
			return;
		case USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION:
			usbd_ctl_error(pdev,req);
			return;
		default:
			usbd_ctl_error(pdev,req);
			return;
	}

	if (len&&req->wLength) {
		len=MIN(len,req->wLength);
		usbd_ctl_send_data(pdev,buf,len);
	}
}

void usbd_set_address(struct usb_dev_handle *pdev,
			struct usb_setup_req *req) {
	unsigned char dev_addr;

	if ((!req->wIndex)&&(!req->wLength)) {
		dev_addr=req->wValue&0x7f;
		if (pdev->dev.dev_status==USB_DEV_STAT_CONFIGURED) {
			usbd_ctl_error(pdev,req);
		} else {
			pdev->dev.dev_addr=dev_addr;
			usb_dev_set_address(pdev,dev_addr);
			usbd_ctl_send_status(pdev);

			if (dev_addr) {
				pdev->dev.dev_status=USB_DEV_STAT_ADDRESSED;
			} else {
				pdev->dev.dev_status=USB_DEV_STAT_DEFAULT;
			}
		}
	} else {
		usbd_ctl_error(pdev,req);
	}
}

void usbd_set_config(struct usb_dev_handle *pdev,
			struct usb_setup_req *req) {
	unsigned char cfgidx=req->wValue;
	
	if (cfgidx>USBD_CFG_MAX_NUM) {
//		sys_printf("set_config: cfgidx %d to high\n",cfgidx);
		usbd_ctl_error(pdev,req);
	} else {
		switch(pdev->dev.dev_status) {
			case USB_DEV_STAT_ADDRESSED:
				if(cfgidx) {
					pdev->dev.dev_config=cfgidx;
					pdev->dev.dev_status=USB_DEV_STAT_CONFIGURED;
					usbd_set_cfg(pdev,cfgidx);
					usbd_ctl_send_status(pdev);
				} else {
					usbd_ctl_send_status(pdev);
				}
				break;
			case USB_DEV_STAT_CONFIGURED:
//		sys_printf("set_config: cfgidx %d STAT_CONFIGURED\n",cfgidx);
				if (!cfgidx) {
					pdev->dev.dev_status=USB_DEV_STAT_ADDRESSED;
					pdev->dev.dev_config=cfgidx;
					usbd_clr_cfg(pdev,cfgidx);
					usbd_ctl_send_status(pdev);
				} if (cfgidx!=pdev->dev.dev_config) {
					usbd_clr_cfg(pdev,pdev->dev.dev_config);
					pdev->dev.dev_config=cfgidx;
					usbd_set_cfg(pdev,cfgidx);
					usbd_ctl_send_status(pdev);
				} else {
					usbd_ctl_send_status(pdev);
				}
				break;
			default:
//		sys_printf("set_config: cfgidx %d default err\n",cfgidx);
				usbd_ctl_error(pdev,req);
				break;
		}
	}
}

void usbd_get_config(struct usb_dev_handle *pdev,
			struct usb_setup_req *req) {

	unsigned char usbd_default_cfg=0;

	if (req->wLength!=1) {
		usbd_ctl_error(pdev,req);
	} else {
		switch(pdev->dev.dev_status) {
			case USB_DEV_STAT_ADDRESSED:
				usbd_ctl_send_data(pdev,
							&usbd_default_cfg,
							1);
				break;
			case USB_DEV_STAT_CONFIGURED:
				usbd_ctl_send_data(pdev,
							&pdev->dev.dev_config,
							1);
				break;
			default:
				usbd_ctl_error(pdev,req);
				break;
		}
	}
}

void usbd_get_status(struct usb_dev_handle *pdev,
			struct usb_setup_req *req) {

	unsigned short usbd_cfg_status=0;
	
	switch(pdev->dev.dev_status) {
		case USB_DEV_STAT_ADDRESSED:
		case USB_DEV_STAT_CONFIGURED:
			usbd_cfg_status=0;

			if(pdev->dev.remote_wakeup) {
				usbd_cfg_status|=USB_CONFIG_REMOTE_WAKEUP;
			}
			
			usbd_ctl_send_data(pdev,&usbd_cfg_status,2);
			break;
		default:
			usbd_ctl_error(pdev,req);
			break;
	}
}

static void usbd_set_feature(struct usb_dev_handle *pdev,
				struct usb_setup_req *req) {
	unsigned int dctl;
	unsigned char test_mode=0;

	if (req->wValue==USB_FEATURE_REMOTE_WAKEUP) {
		pdev->dev.remote_wakeup=1;
		pdev->dev.class_cb->setup(pdev->uref,req);
		usbd_ctl_send_status(pdev);
	} else if ((req->wValue==USB_FEATURE_TEST_MODE)&&
			(!(req->wIndex&0xff))) {
		dctl=pdev->regs->dev.d_ctl&~DCTL_TCTL_MSK;
		test_mode=req->wIndex>>8;
		switch(test_mode) {
			case 1:
				dctl|=(1<<DCTL_TCTL_SHIFT);
				break;
			case 2:
				dctl|=(2<<DCTL_TCTL_SHIFT);
				break;
			case 3:
				dctl|=(3<<DCTL_TCTL_SHIFT);
				break;
			case 4:
				dctl|=(4<<DCTL_TCTL_SHIFT);
				break;
			case 5:
				dctl|=(5<<DCTL_TCTL_SHIFT);
				break;
		}

		test_mode_data=dctl;
		pdev->dev.test_mode=1;
		usbd_ctl_send_status(pdev);
	}
}

static void usbd_clr_feature(struct usb_dev_handle *pdev,
				struct usb_setup_req *req) {

	switch(pdev->dev.dev_status) {
		case USB_DEV_STAT_ADDRESSED:
		case USB_DEV_STAT_CONFIGURED:
			if (req->wValue==USB_FEATURE_REMOTE_WAKEUP) {
				pdev->dev.remote_wakeup=0;
				pdev->dev.class_cb->setup(pdev,req);
				usbd_ctl_send_status(pdev);
			}
			break;
		default:
			usbd_ctl_error(pdev,req);
			break;
	}
}

int usbd_std_dev_req(struct usb_dev_handle *pdev,
			struct usb_setup_req *req) {

	switch (req->bRequest) {
		case USB_REQ_GET_DESCRIPTOR:
			usbd_get_descriptor(pdev,req);
			break;
		case USB_REQ_SET_ADDRESS:
			usbd_set_address(pdev,req);
			break;
		case USB_REQ_SET_CONFIGURATION:
			usbd_set_config(pdev,req);
			break;
		case USB_REQ_GET_CONFIGURATION:
			usbd_get_config(pdev,req);
			break;
		case USB_REQ_GET_STATUS:
			usbd_get_status(pdev,req);
			break;
		case USB_REQ_SET_FEATURE:
			usbd_set_feature(pdev,req);
			break;
		case USB_REQ_CLR_FEATURE:
			usbd_clr_feature(pdev,req);
			break;
		default:
			usbd_ctl_error(pdev,req);
			break;
	}
	return 0;
}

int usbd_std_itf_req(struct usb_dev_handle *pdev,
			struct usb_setup_req *req) {
	int ret=0;
	switch(pdev->dev.dev_status) {
		case USB_DEV_STAT_CONFIGURED:
			if ((req->wIndex&0xff)<= USBD_ITF_MAX_NUM) {
				ret=pdev->dev.class_cb->setup(pdev->uref,req);
				if ((req->wLength==0)&&(!ret)) {
					usbd_ctl_send_status(pdev);
				}
			} else {
				usbd_ctl_error(pdev,req);
			}
			break;
		default:
			usbd_ctl_error(pdev,req);
			break;
	}
	return ret;
}



int usbd_std_ep_req(struct usb_dev_handle *pdev,
			struct usb_setup_req *req) {
	unsigned char ep_addr;
	unsigned char usbd_ep_status=0;

	ep_addr=req->wIndex&0xff;

	switch (req->bRequest) {
		case USB_REQ_SET_FEATURE:
			switch(pdev->dev.dev_status) {
				case USB_DEV_STAT_ADDRESSED:
					if (ep_addr&0x7f) {
						usb_dev_ep_stall(pdev,ep_addr);
					}
					break;
				case USB_DEV_STAT_CONFIGURED:
					if (req->wValue==USB_FEATURE_EP_HALT) {
						if (ep_addr&0x7f) {
							usb_dev_ep_stall(pdev,ep_addr);
						}
					}
					pdev->dev.class_cb->setup(pdev->uref,req);
					usbd_ctl_send_status(pdev);
					break;
				default:
					usbd_ctl_error(pdev,req);
					break;
			}
			break;
		case USB_REQ_CLR_FEATURE:
			switch(pdev->dev.dev_status) {
				case USB_DEV_STAT_ADDRESSED:
					if (ep_addr&0x7f) {
						usb_dev_ep_stall(pdev,ep_addr);
					}
					break;
				case USB_DEV_STAT_CONFIGURED:
					if (req->wValue==USB_FEATURE_EP_HALT) {
						if (ep_addr&0x7f) {
							usb_dev_ep_clr_stall(pdev,ep_addr);
							pdev->dev.class_cb->setup(pdev->uref,req);
						}
						usbd_ctl_send_status(pdev);
					}
					break;
				default:
					usbd_ctl_error(pdev,req);
					break;
			}
			break;
		case USB_REQ_GET_STATUS:
			switch(pdev->dev.dev_status) {
				case USB_DEV_STAT_ADDRESSED:
					if (ep_addr&0x7f) {
						usb_dev_ep_stall(pdev,ep_addr);
					}
					break;
				case USB_DEV_STAT_CONFIGURED:
					if (ep_addr&0x80) {
						if (pdev->dev.in_ep[ep_addr&0x7f].is_stall) {
							usbd_ep_status=1;
						} else {
							usbd_ep_status=0;
						}
					} else {
						if (pdev->dev.out_ep[ep_addr].is_stall) {
							usbd_ep_status=1;
						} else {
							usbd_ep_status=0;
						}
					}
					usbd_ctl_send_data(pdev,&usbd_ep_status,2);
					break;
				default:
					usbd_ctl_error(pdev,req);
					break;
			}
			break;
		default:
			break;
	}
	return 0;
}

void usbd_parse_setup_req(struct usb_dev_handle *pdev,
				struct usb_setup_req *req) {

	unsigned short int tmp;
	req->bmRequest	=pdev->dev.setup_packet[0];
	req->bRequest	=pdev->dev.setup_packet[1];
	memcpy(&tmp,&pdev->dev.setup_packet[2],sizeof(tmp));
//	req->wValue	= ntohs(tmp);
	req->wValue	= tmp;
	memcpy(&tmp,&pdev->dev.setup_packet[4],sizeof(tmp));
//	req->wIndex	= ntohs(tmp);
	req->wIndex	= tmp;
	memcpy(&tmp,&pdev->dev.setup_packet[6],sizeof(tmp));
//	req->wLength	= ntohs(tmp);
	req->wLength	= tmp;

//	sys_printf("parse_setup_req: bmReq(%d),bReq(%d),wVal(%d),wIx(%d),wLen(%d)\n",
//		req->bmRequest,req->bRequest,req->wValue,req->wIndex,req->wLength);

	pdev->dev.in_ep[0].ctl_data_len=req->wLength;
	pdev->dev.dev_state=USB_DEV_EP0_SETUP;
}

int usbd_setup(struct usb_dev_handle *pdev) {
	struct usb_setup_req req;

	usbd_parse_setup_req(pdev,&req);

	switch (req.bmRequest&0x1f) {
		case USB_REQ_RECIPIENT_DEVICE:
			usbd_std_dev_req(pdev,&req);
			break;
		case USB_REQ_RECIPIENT_INTERFACE:
			usbd_std_itf_req(pdev,&req);
			break;
		case USB_REQ_RECIPIENT_ENDPOINT:
			usbd_std_ep_req(pdev, &req);
			break;
		default:
			sys_printf("calling epstall for req %d\n", req.bmRequest);
			usb_dev_ep_stall(pdev,req.bmRequest&0x80);
			break;
	}
	return 0;
}

int usbd_data_out(struct usb_dev_handle *pdev, unsigned char epnum) {
	struct usb_dev_ep *ep;

	if (epnum==0) {
		ep=&pdev->dev.out_ep[0];
		if (pdev->dev.dev_state==USB_DEV_EP0_DATA_OUT) {
			if (ep->rem_data_len>ep->maxpacket) {
				ep->rem_data_len-=ep->maxpacket;
				if (pdev->cfg.dma_enable==1) {
					ep->xfer_buf+=ep->maxpacket;
				}
				usbd_ctl_continue_rx(pdev,
							ep->xfer_buf,
						MIN(ep->rem_data_len,ep->maxpacket));
			} else {
				if (pdev->dev.class_cb->ep0_rx_ready&&
					(pdev->dev.dev_status==USB_DEV_STAT_CONFIGURED)) {
					pdev->dev.class_cb->ep0_rx_ready(pdev->uref);
				}
				usbd_ctl_send_status(pdev);
			}
		}
	} else if (pdev->dev.class_cb->data_out&&
			(pdev->dev.dev_status==USB_DEV_STAT_CONFIGURED)) {
		pdev->dev.class_cb->data_out(pdev->uref,epnum);
	}
	return 0;
}

int usbd_data_in(struct usb_dev_handle *pdev, unsigned char epnum) {
	struct usb_dev_ep *ep;

	if (epnum==0) {
		ep=&pdev->dev.in_ep[0];
		if (pdev->dev.dev_state==USB_DEV_EP0_DATA_IN) {
			if (ep->rem_data_len>ep->maxpacket) {
				ep->rem_data_len-=ep->maxpacket;
				if(pdev->cfg.dma_enable==1) {
					ep->xfer_buf+=ep->maxpacket;
				}
				usbd_ctl_continue_tx(pdev, ep->xfer_buf,
							ep->rem_data_len);
			} else {
				if ((!(ep->total_data_len%ep->maxpacket))&&
					(ep->total_data_len>=ep->maxpacket)&&
					(ep->total_data_len<ep->ctl_data_len)){
					usbd_ctl_continue_tx(pdev,0,0);
					ep->ctl_data_len=0;
				} else {
					if (pdev->dev.class_cb->ep0_tx_sent&&
					(pdev->dev.dev_status==USB_DEV_STAT_CONFIGURED)) {
						pdev->dev.class_cb->ep0_tx_sent(pdev->uref);
					}
					usbd_ctl_receive_status(pdev);
				}
			}
		}
//		if (pdev->dev.test_mode) {
//			usbd_run_test_mode(pdev);
//			pdev->dev.test_mode=0;
//		}
	} else if (pdev->dev.class_cb->data_in&&
			(pdev->dev.dev_status==USB_DEV_STAT_CONFIGURED)) {
		pdev->dev.class_cb->data_in(pdev->uref,epnum);
	}
	return 0;
}

int usbd_run_test_mode(struct usb_dev_handle *pdev) {
	sys_printf("usbd_run_test_mode\n");
	pdev->regs->dev.d_ctl=test_mode_data;
	return 0;
}

int usbd_reset(struct usb_dev_handle *pdev) {

//	sys_printf("usbd_reset\n");
	usb_dev_ep_open(pdev,0x00,
			USB_DEV_MAX_EP0_SIZE,
			EP_TYPE_CTRL);

	usb_dev_ep_open(pdev,0x80,
			USB_DEV_MAX_EP0_SIZE,
			EP_TYPE_CTRL);

	pdev->dev.dev_status=USB_DEV_STAT_DEFAULT;
	pdev->dev.usr_cb->dev_reset(pdev->cfg.speed);

	return 0;
}

int usbd_resume(struct usb_dev_handle *pdev) {
//	sys_printf("usbd_resume\n");
	pdev->dev.usr_cb->dev_resumed();
	pdev->dev.dev_status=pdev->dev.dev_old_status;
	return 0;
}

int usbd_suspend(struct usb_dev_handle *pdev) {
//	sys_printf("usbd_suspend\n");
	pdev->dev.dev_old_status=pdev->dev.dev_status;
	pdev->dev.dev_status=USB_DEV_STAT_SUSPENDED;
	pdev->dev.usr_cb->dev_suspended();
	return 0;
}

int usbd_sof(struct usb_dev_handle *pdev) {
//	sys_printf("usbd_sof\n");
	if (pdev->dev.class_cb->sof) {
		pdev->dev.class_cb->sof(pdev->uref);
	}
	return 0;
}

int usbd_set_cfg(struct usb_dev_handle *pdev, unsigned char cfgidx) {
	pdev->dev.class_cb->init(pdev->uref,cfgidx);
	pdev->dev.usr_cb->dev_configured();
	return 0;
}

int usbd_clr_cfg(struct usb_dev_handle *pdev, unsigned char cfgidx) {
	sys_printf("usbd_clr_cfg\n");
	pdev->dev.class_cb->deinit(pdev->uref,cfgidx);
	return 0;
}

int usbd_iso_in_incomplete(struct usb_dev_handle *pdev) {
	sys_printf("usbd_iso_in_incomplete\n");
	pdev->dev.class_cb->iso_in_incomplete(pdev->uref);
	return 0;
}

int usbd_iso_out_incomplete(struct usb_dev_handle *pdev) {
	sys_printf("usbd_iso_out_incomplete\n");
	pdev->dev.class_cb->iso_out_incomplete(pdev->uref);
	return 0;
}

int usbd_dev_connected(struct usb_dev_handle *pdev) {
	sys_printf("usbd_dev_connected\n");
	pdev->dev.usr_cb->dev_connected();
	pdev->dev.conn_status=1;
	/********                        ******/
	usb_dev_init(pdev);
	pdev->dev.usr_cb->init();
	return 0;
}

int usbd_dev_disconnected(struct usb_dev_handle *pdev) {
	sys_printf("usbd_dev_disconnected\n");
	pdev->dev.usr_cb->dev_disconnected();
	pdev->dev.class_cb->deinit(pdev->uref,0);
	pdev->dev.conn_status=0;
	return 0;
}
