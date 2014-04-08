/* $CecA1GW: , v1.1 2014/04/07 21:44:00 anders Exp $ */

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
 * @(#)pulse_eight.c
 */

#include "io.h"
#include "sys.h"

#include "asynchio.h"
#include "cec.h"
#include "usb_serial_drv.h"


static int fd;
static unsigned char rbuf[24];
static int bix;
static unsigned char tbuf[24];
static int tbix;
extern int cec_debug;

#define CMD_PING 		1
#define CMD_ACCEPTED		8
#define CMD_REJECTED		9
#define CMD_SET_ACK_MASK	10
#define CMD_TX			11
#define CMD_TX_EOM		12
#define CMD_SET_TXIDLE_TIME	13
#define CMD_SET_ACK_POLARITY	14
#define CMD_TX_SUCCESS		16
#define CMD_TX_NO_ACK		18
#define CMD_GET_FW_VER		21
#define CMD_GET_BUILDDATE	23
#define CMD_SET_CONTROLLED	24
#define CMD_GET_AUTO_ENABLED	25
#define CMD_GET_DFLT_LA		27
#define CMD_SET_DFLT_LA		28
#define CMD_GET_LA_MASK		29
#define CMD_SET_LA_MASK		30
#define CMD_GET_PA		31
#define CMD_SET_PA		32
#define CMD_GET_DEVICE_TYPE	33
#define CMD_GET_HDMI_VERSION	35
#define CMD_GET_OSD_NAME	37
#define CMD_SET_OSD_NAME	38
#define CMD_WRITE_EEPROM	39
#define CMD_GET_ADAPTER_TYPE	40


static int send_accepted(int fd) {
	unsigned char buf[3] = { 0xff,CMD_ACCEPTED,0xfe };
	int rc=io_write(fd,(char *)buf,3);
	if (rc!=3) {
		DPRINTF("send_accepted: got rc %d insted of 3\n", rc);
	}
	return rc;
}

static int send_reject(int fd) {
	unsigned char buf[3] = { 0xff,CMD_REJECTED,0xfe };
	int rc=io_write(fd,(char *)buf,3);
	if (rc!=3) {
		DPRINTF("send_accepted: got rc %d insted of 3\n", rc);
	}
	return rc;
}


static int handle_ping(int fd) {
	DPRINTF("received ping\n");
	return send_accepted(fd);
}

static int handle_get_fw_version(int fd) {
	unsigned char buf[]= { 0xff, CMD_GET_FW_VER, 0x0, 0x2, 0xfe };
	DPRINTF("handle_get_fw_version\n");
	int rc=io_write(fd,(char *)buf,sizeof(buf));
	if (rc!=sizeof(buf)) {
		DPRINTF("send_fw_version: got rc %d insted of %d\n", rc,sizeof(buf));
	}
	return rc;
}

static int handle_set_controlled(int fd, unsigned char *buf, int len) {
	DPRINTF("handle_set_controlled: %d, len %d\n",
				buf[2], len);
	if (buf[2]) {
		cec_set_rec(1);
	} else {
		cec_set_rec(0);
	}
	return send_accepted(fd);
}

static int handle_get_builddate(int fd, unsigned char *buf, int len) {
	unsigned char sbuf[] = { 0xff, CMD_GET_BUILDDATE, 0x50, 0x09, 0xf0, 0xa3, 0xfe };
	DPRINTF("handle_get_builddate: %d, len %d\n",
				buf[2], len);
	int rc=io_write(fd,(char *)sbuf,sizeof(sbuf));
	if (rc!=sizeof(sbuf)) {
		DPRINTF("send_get_builddate: got rc %d insted of %d\n", rc,sizeof(sbuf));
	}
	return rc;
}

static int handle_get_adapter_type(int fd) {
	unsigned char sbuf[] = { 0xff, CMD_GET_ADAPTER_TYPE, 0x1, 0xfe };
	DPRINTF("handle_get_adpter_type\n");
	int rc=io_write(fd,(char *)sbuf,sizeof(sbuf));
	if (rc!=sizeof(sbuf)) {
		DPRINTF("send_adapter_type: got rc %d insted of %d\n", rc,sizeof(sbuf));
	}
	return rc;
}

static int handle_set_txidle_time(int fd, unsigned char *buf, int len) {
	DPRINTF("handle_set_txidle_time: %d, len %d\n",
				buf[2], len);
	return send_accepted(fd);
}

static int handle_set_ack_polarity(int fd, unsigned char *buf, int len) {
	DPRINTF("handle_set_ack_polarity: %d, len %d\n",
				buf[2], len);
	tbix=0;
	return send_accepted(fd);
}


static int handle_tx(int fd, unsigned char *buf, int len) {
	if (tbix) {
		DPRINTF(", %x",buf[2]);
	} else {
		DPRINTF("Sending to Cec dev: %x", buf[2]);
	}
	
	tbuf[tbix++]=buf[2];
	return send_accepted(fd);
}

static int send_tx_success(int fd) {
	unsigned char sbuf[] = { 0xff, CMD_TX_SUCCESS, 0xfe };
	int rc;
	DPRINTF("reply: tx_success\n");
	rc=io_write(fd,(char *)sbuf,sizeof(sbuf));
	if (rc!=sizeof(sbuf)) {
		DPRINTF("tx_success: got rc %d instead of %d\n", rc,sizeof(sbuf));
	}
	return rc;
}

static int send_tx_ack_failed(int fd) {
	unsigned char sbuf[] = { 0xff, CMD_TX_NO_ACK, 0xfe };
	DPRINTF("reply: tx_failed\n");
	int rc=io_write(fd,(char *)sbuf,sizeof(sbuf));
	if (rc!=sizeof(sbuf)) {
		DPRINTF("tx_failed: got rc %d instead of %d\n", rc,sizeof(sbuf));
	}
	return rc;
}



static int handle_tx_eom(int fd, unsigned char *buf, int len) {
	int rc;
	if (tbix) {
		DPRINTF(", %x\n",buf[2]);
	} else {
		DPRINTF("Sending to Cec dev: %x\n", buf[2]);
	}
	
	tbuf[tbix++]=buf[2];
	send_accepted(fd);
	rc=cec_send(USB_BUS,tbuf,tbix);
	tbix=0;
	if (rc<0) {
		send_tx_ack_failed(fd);
	} else  {
		send_tx_success(fd);
	}
	return 0;
}

static int pe_data_in(unsigned char *buf, int len);
static int prev_ack_mask;

static int handle_set_ack_mask(int fd, unsigned char *buf, int len) {
	int ack_mask=(buf[2]<<8)|buf[3];
	DPRINTF("set_ack_mask: %x\n",ack_mask);
	if (ack_mask!=prev_ack_mask) {
		int i;
		for(i=0;i<15;i++) {
			if ((ack_mask&(1<<i)) ^
				(prev_ack_mask&(1<<i))) {
				if (ack_mask&(1<<i)) {
					cec_attach(USB_BUS,i,pe_data_in);
				}
				if (prev_ack_mask&(1<<i)) {
					cec_detach(i);
				}
			}
		}
		prev_ack_mask=ack_mask;
	}
	return send_accepted(fd);
}

static int handle_get_auto_enabled(int fd,unsigned char *buf, int len) {
	unsigned char sbuf[] = { 0xff, CMD_GET_AUTO_ENABLED, 1, 0xfe };
	DPRINTF("reply: get auto enabled\n");
	int rc=io_write(fd,(char *)sbuf,sizeof(sbuf));
	if (rc!=sizeof(sbuf)) {
		DPRINTF("tx_failed: got rc %d instead of %d\n", rc,sizeof(sbuf));
	}
	return rc;
}

static int handle_get_hdmi_version(int fd,unsigned char *buf, int len) {
	unsigned char sbuf[] = { 0xff, CMD_GET_HDMI_VERSION, 5, 0xfe };
	DPRINTF("reply: get hdmi version 5\n");
	int rc=io_write(fd,(char *)sbuf,sizeof(sbuf));
	if (rc!=sizeof(sbuf)) {
		DPRINTF("tx_failed: got rc %d instead of %d\n", rc,sizeof(sbuf));
	}
	return rc;
}

static int handle_get_dflt_la(int fd,unsigned char *buf, int len) {
	unsigned char sbuf[] = { 0xff, CMD_GET_DFLT_LA, 4, 0xfe };
	DPRINTF("reply: get default logical address 4\n");
	int rc=io_write(fd,(char *)sbuf,sizeof(sbuf));
	if (rc!=sizeof(sbuf)) {
		DPRINTF("tx_failed: got rc %d instead of %d\n", rc,sizeof(sbuf));
	}
	return rc;
}


static int handle_get_device_type(int fd,unsigned char *buf, int len) {
	unsigned char sbuf[] = { 0xff, CMD_GET_DEVICE_TYPE, 4, 0xfe };
	DPRINTF("reply: get device type 4\n");
	int rc=io_write(fd,(char *)sbuf,sizeof(sbuf));
	if (rc!=sizeof(sbuf)) {
		DPRINTF("tx_failed: got rc %d instead of %d\n", rc,sizeof(sbuf));
	}
	return rc;
}

static int handle_get_la_mask(int fd,unsigned char *buf, int len) {
	unsigned char sbuf[] = { 0xff, CMD_GET_LA_MASK, 0, 0x10 , 0xfe };
	DPRINTF("reply: get la_mask 2\n");
	int rc=io_write(fd,(char *)sbuf,sizeof(sbuf));
	if (rc!=sizeof(sbuf)) {
		DPRINTF("tx_failed: got rc %d instead of %d\n", rc,sizeof(sbuf));
	}
	return rc;
}


static int handle_get_osd_name(int fd,unsigned char *buf, int len) {
	unsigned char sbuf[] = { 0xff, CMD_GET_OSD_NAME, 0x41, 0x42, 0x43, 0x44 , 0xfe };
	DPRINTF("reply: OSD name abcd\n");
	int rc=io_write(fd,(char *)sbuf,sizeof(sbuf));
	if (rc!=sizeof(sbuf)) {
		DPRINTF("tx_failed: got rc %d instead of %d\n", rc,sizeof(sbuf));
	}
	return rc;
}

static int handle_get_pa(int fd,unsigned char *buf, int len) {
	unsigned char sbuf[] = { 0xff, CMD_GET_PA, 0x20, 0x00, 0xfe };
	DPRINTF("reply: get_pa 0x2000\n");
	int rc=io_write(fd,(char *)sbuf,sizeof(sbuf));
	if (rc!=sizeof(sbuf)) {
		DPRINTF("tx_failed: got rc %d instead of %d\n", rc,sizeof(sbuf));
	}
	return rc;
}



/* 
 * The start up sequence from libcec:
 * 	1. CMD_PING
 * 	2. CMD_GET_FW_VER
 * 	3, CMD_SET_CONTROLLED
 * 	4. CMD_GET_BUILD_DATE
 * 	5. CMD_GET_ADAPTER_TYPE
 * 	6. CMD_SET_TXIDLE_TIME
 * 	Repeat
 * 	7. CMD_SET_ACK_POLARITY
 * 	8. CMD_TX_EOM
 */

static int handle_pe_cmd(int fd,unsigned char *buf, int len) {
	switch (buf[1]) {
		case CMD_PING:
			handle_ping(fd);
			break;
		case CMD_GET_FW_VER:
			handle_get_fw_version(fd);
			break;
		case CMD_SET_CONTROLLED:
			handle_set_controlled(fd,buf,len);
			break;
		case CMD_GET_BUILDDATE:
			handle_get_builddate(fd,buf,len);
			break;
		case CMD_GET_ADAPTER_TYPE:
			handle_get_adapter_type(fd);
			break;
		case CMD_SET_TXIDLE_TIME:
			handle_set_txidle_time(fd,buf,len);
			break;
		case CMD_SET_ACK_POLARITY:
			handle_set_ack_polarity(fd,buf,len);
			break;
		case CMD_TX_EOM:
			handle_tx_eom(fd,buf,len);
			break;
		case CMD_TX:
			handle_tx(fd,buf,len);
			break;
		case CMD_SET_ACK_MASK:
			handle_set_ack_mask(fd,buf,len);
			break;
		case CMD_GET_AUTO_ENABLED:
			handle_get_auto_enabled(fd,buf,len);
			break;
		case CMD_GET_HDMI_VERSION:
			handle_get_hdmi_version(fd,buf,len);
			break;
		case CMD_GET_DFLT_LA:
			handle_get_dflt_la(fd,buf,len);
			break;
		case CMD_GET_DEVICE_TYPE:
			handle_get_device_type(fd,buf,len);
			break;
		case CMD_GET_LA_MASK:
			handle_get_la_mask(fd,buf,len);
			break;
		case CMD_GET_OSD_NAME:
			handle_get_osd_name(fd,buf,len);
			break;
		case CMD_GET_PA:
			handle_get_pa(fd,buf,len);
			break;
		default:
			DPRINTF("got unhandled pe command: %d\n", buf[1]);
			send_reject(fd);
			break;
	}
	return 0;
}






static int  handle_read_event(int fd, int ev, void *dum) {
	unsigned char ch;
	int rc;
	while(((rc=io_read(fd,(char *)&ch,1))==1)) {
		if (!bix) {
			if (ch==0xff) {
				/* start byte */
				rbuf[bix++]=ch;
			}
		} else {
			rbuf[bix++]=ch;
			if (ch==0xfe) {
				handle_pe_cmd(fd,rbuf,bix);
				bix=0;
			}
		}	
	}
	return 0;
}

static int pe_data_in(unsigned char *buf, int len) {
	int i;
	int rc;
	unsigned char sbuf[] = { 0xff, 0x05, 0, 0xfe };
	if ((len<1)||(len>16)) {
		DPRINTF("pe_data_in: got weird message len %d\n", len);
		return -1;
	}
#if DEBUG
	if (cec_debug) {
		printf("got data on cec: %x",buf[0]);
		for(i=1;i<len;i++) {
			printf(", %02x",buf[i]);
		}
		printf(" ---> USB\n");
	}
#endif

	if (len==1) sbuf[1]|=0x80;
	sbuf[2]=buf[0];
	rc=io_write(fd,(char *)sbuf,sizeof(sbuf));
	if (rc!=sizeof(sbuf)) {
		DPRINTF("pe_data_in: failed to write data on usb bus, rc=%d\n", rc);
		return 0;
	}
	sbuf[1]=0x06;
	for (i=1;i<len;i++) {
		if ((i+1)==len) {
			sbuf[1]|=0x80;
		}
		sbuf[2]=buf[i];
		rc=io_write(fd,(char *)sbuf,sizeof(sbuf));
		if (rc!=sizeof(sbuf)) {
			DPRINTF("pe_data_in: failed to write data on usb bus, rc=%d\n", rc);
			return 0;
		}
	}
	return 0;
}

int wakeup_usb_dev() {
	int rc=io_control(fd,USB_REMOTE_WAKEUP,0,0);
	DPRINTF("remote wakeup returned %d\n",rc);
	return 0;
}

int init_pulse_eight(void) {
	fd=io_open("usb_serial0");
	if (fd<0) return -1;

	printf("init_pulse_eight: have usb descriptor %d\n", fd);
	io_control(fd,F_SETFL,(void *)O_NONBLOCK,0);

	register_event(fd,EV_READ,handle_read_event,0);
	
	return 0;
}
