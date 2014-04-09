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
 * @(#)a1.c
 */
#include "sys.h"
#include "io.h"
#include "a1_drv.h"
#include "a1.h"
#include "cec.h"

#include "asynchio.h"

static int fd_a1;
int a1_send(unsigned char *buf, int len);
static int a1_volume_up(int a1, int a2, void *a3);
static int a1_volume_down(int a1, int a2, void *a3);
static int a1_volume_up_pressed(void);
static int a1_volume_down_pressed(void);
static int a1_volume_released(void);
static int a1_attach_amp(void);

#define VOL_UP		1
#define VOL_DOWN	2
static int vol_state;

#define SOURCE_TUNER 0x00
#define SOURCE_PHONO 0x01
#define SOURCE_CD    0x02
/* Undefined		0x03 */
#define SOURCE_MDTAPE 0x04
/* Undefined 		0x05 */
/* Undefined 		0x06 */
/* Undefined 		0x07 */
/* Undefined 		0x08 */
/* Undefined 		0x09 */
/* Undefined 		0x0a */
/* Undefined 		0x0b */
/* Undefined 		0x0c */
/* Undefined 		0x0d */
/* Undefined 		0x0e */
/* Undefined 		0x0f */
#define SOURCE_VIDEO1	0x10
#define SOURCE_VIDEO2	0x11
#define SOURCE_VIDEO3	0x12
/* Undefined 		0x13 */
/* Undefined 		0x14 */
/* Set TVLD		0x15 */
#define SOURCE_TVLD	0x16
/* Undefined 		0x17 */
/* Undefined 		0x18 */
#define SOURCE_DVD	0x19


#define POWER_STAT_BIT	1
#define MUTE_STAT_BIT	2



static int amp_avail;
static int power_state;
static int audio_source;
unsigned char idbuf[16];

static int q_count;
static int cec_handle;


static int ping_cec_AUDIO(void) {
	unsigned char addr=0xf5;
	return cec_send(A1_LINK, &addr,sizeof(addr));
}

static int handle_cec_data(unsigned char *buf, int size) {
	int fromAddr=buf[0]>>4;
	int cmd=buf[1];
	int param=buf[2];

	if (size==1) {
		DPRINTF("a1: got CEC ping\n");
		return 0;
	}
	switch(cmd) {
		case CEC_OPCODE_GIVE_PHYSICAL_ADDRESS:
			cec_send_physical_address(A1_LINK,(5<<4)|0xf, 0x1000, 5);
			a1_attach_amp();
			break;
		case CEC_OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS:
			cec_send_system_audio_mode_status(A1_LINK,(5<<4)|fromAddr, 1);
			break;
		case CEC_OPCODE_ROUTING_CHANGE:
			break;
		case CEC_OPCODE_GIVE_OSD_NAME:
			cec_send_osd_name(A1_LINK,(5<<4)|fromAddr,idbuf);
			break;
		case CEC_OPCODE_GIVE_DEVICE_VENDOR_ID:
			cec_send_vendor_id(A1_LINK,(5<<4)|0xf,0x080046);
			break;
		case CEC_OPCODE_GIVE_DEVICE_POWER_STATUS:
			cec_send_power_status(A1_LINK,(5<<4)|fromAddr, power_state);
			break;
		case CEC_OPCODE_USER_CONTROL_PRESSED:
			switch (param) {
				case 0x41:
					a1_volume_up_pressed();
					break;
				case 0x42:
					a1_volume_down_pressed();
					break;
				default:
					printf("cec: uknown key pressed 0x%x\n", param);
			}
			break;
		case CEC_OPCODE_USER_CONTROL_RELEASE:
			a1_volume_released();
			break;
		case CEC_OPCODE_STANDBY:
			a1_power_off();
			break;
		default:
			printf("a1: unknown cec_cmd %x\n", cmd);
			break;
	}
	return 0;
}

static int name_requested;

int a1_get_amp_name(int a1, int a2, void *a3) {
	unsigned char buf[]={0xc0,0x6a};
	printf("sending a1_get_amp_name\n");
	name_requested=1;
	return a1_send(buf,sizeof(buf));
}

int set_amp_avail(int state) {
	if (amp_avail!=state) {
		amp_avail=state;
		printf("change amp avail to %d\n", state);
		if (!state) {
			cec_detach(cec_handle);
		}
	}
	if (state) {
		q_count=0;
	}
	return 0;
}

int a1_attach_amp(void) {
	register_timer(0,0,0);
//	if (ping_cec_AUDIO()<0) {
		if (!cec_handle) {
			cec_handle=cec_attach(A1_LINK, CECDEVICE_AUDIOSYSTEM, handle_cec_data);
		}
		cec_send_physical_address(A1_LINK,(cec_handle<<4)|0xf,0x1000, cec_handle);
		cec_send_system_audio_mode_set(A1_LINK,cec_handle<<4|0xf,1);
//	} else {
//		printf("already have cec audio system\n");
//	}
	return 0;
}


int set_power_state(int state) {
	if (power_state!=state) {
		printf("change power state to %d\n", state);
		power_state=state;
		if (state) {
			a1_get_amp_name(0,0,0);
			register_timer(500,a1_get_amp_name,0);
			wakeup_usb_dev();
			cec_send_image_view_on(A1_LINK,0x50);
		} else {
			cec_send_standby(A1_LINK,0x5f);
		}
	}
	return 0;
}

int set_audio_source(int source) {
	if (audio_source!=source) {
		audio_source=source;
		printf("change audio source to %x\n", source);
	}
	return 0;
}

int a1_power_off(void) {
	unsigned char buf[]={0xc0, 0x2f};
	return a1_send(buf,sizeof(buf));
}

int a1_power_on(void) {
	unsigned char buf[]={0xc0, 0x2e};
	return a1_send(buf,sizeof(buf));
}


static int a1_volume_up(int a1, int a2, void *a3) {
	unsigned char buf[]={0xc0, 0x14};
	return a1_send(buf,sizeof(buf));
}

static int a1_volume_down(int a1, int a2, void *a3) {
	unsigned char buf[]={0xc0, 0x15};
	return a1_send(buf,sizeof(buf));
}

static int a1_volume_up_pressed(void) {
	a1_volume_up(0,0,0);
	vol_state=VOL_UP;
	register_timer(30,a1_volume_up,0);
	return 0;
}

static int a1_volume_down_pressed(void) {
	a1_volume_down(0,0,0);
	vol_state=VOL_DOWN;
	register_timer(30,a1_volume_down,0);
	return 0;
}

static int a1_volume_released(void) {
	register_timer(0,0,0);
	return 0;
}


static int handle_timeout(int fd, int ev, void *dum) {
/* send a request for status, answer will be received below,
 */
	unsigned char send_data[] = {0xC0,0x0F};
	io_write(fd_a1,(char *)send_data,sizeof(send_data));
	q_count++;
	if (q_count==3) {
		set_amp_avail(0);
	}
	return 0;
}

static int handle_a1_data(int fd, int ev, void *dum) {
	int rc;
	unsigned char a1_rbuf[24];
	rc=io_read(fd,(char *)a1_rbuf,sizeof(a1_rbuf));
	if (rc<0) {
		printf("got error a1 read %d\n", rc);
		return 0;
	}

	if (!(a1_rbuf[0]&0x8)) {
		DPRINTF("received a1 data, not for me!\n");
		DUMP_DATA(A1_LINK,"got a1 data", a1_rbuf,rc);
	}
	a1_rbuf[0]&=~0x8;
	if (a1_rbuf[0]==0xC0) {
		DPRINTF("a1: msg from amp: ");
		set_amp_avail(1);
		switch (a1_rbuf[1]) {
			case 0x06:
				DPRINTF("Mute\n");
				break;
			case 0x07:
				DPRINTF("Unmute\n");
				break;
			case 0x0c:
				DPRINTF("5.1 on\n");
				break;
			case 0x0d:
				DPRINTF("5.1 off\n");
				break;
			case 0x0e:
				DPRINTF("Unavailable\n");
				break;
			case 0x0f:
				DPRINTF("Error\n");
				break;
			case 0x2E:
				DPRINTF("Power on\n");
				set_power_state(1);
				break;
			case 0x2F:
				DPRINTF("Power off\n");
				set_power_state(0);
				break;
			case 0x43:
				DPRINTF("Input stat\n");
				break;
			case 0x70:
				DPRINTF("Status source AA(%02x), AV(%02x) CC(%02x)\n",
					a1_rbuf[2], a1_rbuf[3], a1_rbuf[4]);
				set_power_state((a1_rbuf[4]&POWER_STAT_BIT)?1:0);
				set_audio_source(a1_rbuf[2]);
				break;
			case 0x6A:
				DPRINTF("Device Name %s\n", &a1_rbuf[2]);
				if (name_requested) {
					name_requested=0;
					__builtin_strcpy((char *)idbuf,(char *)&a1_rbuf[2]);
					a1_attach_amp();
				}
				break;
			case 0x71:
				DPRINTF("Status 2nd aud\n");
				break;
			case 0xE1:
				DPRINTF("Status\n");
				break;
			default:
				DPRINTF("Uknown (%x)\n", a1_rbuf[1]);
				if (cec_debug) {
					DUMP_DATA(A1_LINK,"got a1 data", a1_rbuf,rc);
				}
				break;
		}
	} else if (a1_rbuf[0]==0xC1) {
		DPRINTF("a1: msg from tuner: ");
		switch (a1_rbuf[1]) {
			case 0x40:
				DPRINTF("Station Status\n");
				break;
			case 0x70:
				DPRINTF("Tuner Status\n");
				break;
			default:
				DPRINTF("Unknown (%x)\n", a1_rbuf[1]);
				if (cec_debug) {
					DUMP_DATA(A1_LINK,"got a1 data", a1_rbuf,rc);
				}
				break;
		}
	} else if (a1_rbuf[0]==0xC2) {
		DPRINTF("a1: msg from uknown dev(%x)\n",0xc2);
	} else if (a1_rbuf[0]==0xC3) {
		DPRINTF("a1: msg from surround: ");
		switch(a1_rbuf[1]) {
			case 0x40:
				DPRINTF("Status R vol\n");
				break;
			case 0x50:
				DPRINTF("Status F vol\n");
				break;
			case 0x70:
				DPRINTF("Status Surround\n");
				break;
			default:
				DPRINTF("Unknown (%x)\n",a1_rbuf[1]);
				if (cec_debug) {
					DUMP_DATA(A1_LINK, "got a1 data", a1_rbuf,rc);
				}
				break;
		}
	} else {
		DPRINTF("a1: got msg from unknown originator (%x)\n", a1_rbuf[0]);
	}
	return 0;
}

int a1_send(unsigned char *buf, int len) {
	int rc;
	io_control(fd_a1,F_SETFL,(void *)0,0);
	rc=io_write(fd_a1,(char *)buf,len);
	io_control(fd_a1,F_SETFL,(void *)O_NONBLOCK,0);
	return rc;
}


int a1_init_a1(void) {
	fd_a1=io_open(A1_DRV0);
	io_control(fd_a1,F_SETFL,(void *)O_NONBLOCK,0);
	register_event(fd_a1,EV_READ,handle_a1_data,0);
	register_timer(3000, handle_timeout, 0);
	handle_timeout(0,0,0);
	return fd_a1;
}
