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
 * @(#)cec.c
 */

#include "sys.h"
#include "io.h"
#include "cec_drv.h"
#include "cec.h"
#include "a1.h"

#include "asynchio.h"


int ser_fd;

static int fd_cec;
static unsigned char cec_rbuf[24];

static int distribute_msg(int itf, unsigned char *buf, int len);
static int handle_cec_data(int fd,int ev, void *dum);

struct devt {
	int bus;
	CEC_CALLBACK callback;
};

struct devt devt[16];
static int ack_mask;

const char *devstr[] = {"TV", "Rec1", "Rec2", "Tuner1",
			"Playback1", "AudioSys", "Tuner2",
			"Tuner2", "Tuner3", "Playback2",
			"Rec3", "Tuner4", "Playback3",
			"??", "??", "Any"};

const char *cmdstr[] = {"FABORT", "??", "??", "??", "Image_view_on",
/* 0x05 */		"TUNER_STEP_INC", "TUNER_STEP_DEC", "TUNER_DEV_STAT", "Give_Tuner_Stat", "Rec_ON",
/* 0x0a */		"Rec_Stat", "Rec_off", "??", "TEXT_VIEW_ON", "??",
/* 0x0f */		"Rec_TV_SCREEN", "??", "??", "??", "??",
/* 0x14 */		"??", "??", "??", "??", "??",
/* 0x19 */		"??", "Give_Deck_Stat", "Deck_Stat", "??", "??",
/* 0x1e */		"??", "??", "??", "??", "??", 
/* 0x23 */		"??", "??", "??", "??", "??",
/* 0x28 */		"??", "??", "??", "??", "??",
/* 0x2d */		"??", "??", "??", "??", "??",
/* 0x32 */		"Set_Menu_Lang", "CLR_ANALOGUE_TIMER", "Set_Analogue_Timer", "Timer_Status", "Standby",
/* 0x37 */		"??", "??", "??", "??", "??",
/* 0x3c */		"??", "??", "??", "??", "??",
/* 0x41 */		"Play", "Deck_Ctl", "Timer_cleared_stat", "User_ctrl_pressed", "User_ctrl_released",
/* 0x46 */		"Give_osd_name", "Set_osd_name", "??", "??", "??",
/* 0x4b */		"??", "??", "??", "??", "??",
/* 0x50	*/		"??", "??", "??", "??", "??",
};



int cec_dump_data(int itf, char *pretext, unsigned char *buf, int len) {
        int i;
	int cmd;
	char *itf_str=(itf==CEC_BUS)?"CEC_BUS":(itf==A1_LINK)?"A1_LINK":"USB_BUS";
        DPRINTF("%t: on %s, %s: %s(%x)-->%s(%x) :",itf_str,pretext, devstr[buf[0]>>4],buf[0]>>4,devstr[buf[0]&0xf], buf[0]&0xf);
	if (len>1) {
		cmd=buf[1];
		if (cmd<sizeof(cmdstr)) {
			DPRINTF(", %s(%02x)", cmdstr[cmd], cmd);
		} else {
			DPRINTF(", %s(%02x)", "??", cmd);
		}
	}
        for(i=2;i<len;i++) {
                DPRINTF(", %02x",buf[i]);
        }
        DPRINTF("\n");
        return 0;
}


int cec_init_cec(void) {
	unsigned int val_i=1;
//	ser_fd=io_open("usb_serial0");
	ser_fd=io_open("usart0");
	fd_cec=io_open(CEC_DRV);
	io_control(fd_cec,F_SETFL,(void *)O_NONBLOCK,0);
	io_control(fd_cec,CEC_SET_PROMISC,&val_i,sizeof(val_i));
	register_event(fd_cec,EV_READ,handle_cec_data,0);
	return fd_cec;
}

int cec_attach(int itf, unsigned int la, CEC_CALLBACK callback) {
#if 0
	if (!devt[la].callback) {
		devt[la].bus=itf;
		devt[la].callback=callback;
		ack_mask|=(1<<la);
		DPRINTF("installing dev %x\n", la);
		io_control(fd_cec,CEC_SET_ACK_MSK,&ack_mask,sizeof(ack_mask));
		return la;
	} else {
		DPRINTF("cant install already installed dev %x\n", la);
	}
#endif
	return -1;
}

int cec_detach(unsigned int la) {
#if 0
	if (devt[la].callback) {
		devt[la].callback=0;
		ack_mask&=~(1<<la);
		DPRINTF("uninstalling dev %x\n", la);
		return io_control(fd_cec,CEC_SET_ACK_MSK,&ack_mask,sizeof(ack_mask));
	}
#endif
	return 0;
}

int cec_send_physical_address(int itf, unsigned int addr, unsigned short paddr, int function) {
#if 0
	unsigned char buf[] = {addr, CEC_OPCODE_REPORT_PHYSICAL_ADDRESS, 0, 0, function};
	buf[2]=paddr>>8;
	buf[3]=paddr&0xff;
	return distribute_msg(itf,buf,sizeof(buf));
}

int cec_send_osd_name(int itf, unsigned int addr, unsigned char *idbuf) {
	unsigned char buf[16]={addr,CEC_OPCODE_SET_OSD_NAME,};
	int len=__builtin_strlen((char *)idbuf);
	__builtin_memcpy(&buf[2],idbuf,len);

	len+=2;
	return distribute_msg(itf,buf,len);
#endif
	return 0;
}

int cec_send_system_audio_mode_status(int itf, unsigned int addr, int on) {
#if 0
	unsigned char buf[]={addr,CEC_OPCODE_SYSTEM_AUDIO_MODE_STATUS,(on)?1:0};
	return distribute_msg(itf,buf,sizeof(buf));
#endif
	return 0;
}

int cec_send_system_audio_mode_set(int itf, unsigned int addr, int on) {
#if 0
	unsigned char buf[]={addr,CEC_OPCODE_SET_SYSTEM_AUDIO_MODE,(on)?1:0};
	return distribute_msg(itf,buf,sizeof(buf));
#endif
	return 0;
}


int cec_send_vendor_id(int itf, unsigned int addr,unsigned int vendor_id) {
#if 0
	unsigned char *vid=(unsigned char *)&vendor_id;
	unsigned char buf[]={addr,CEC_OPCODE_DEVICE_VENDOR_ID,vid[2],vid[1],vid[0]};
	return distribute_msg(itf,buf,sizeof(buf));
#endif
	return 0;
}

int cec_send_power_status(int itf, unsigned int addr, int on) {
#if 0
	unsigned char buf[]={addr,CEC_OPCODE_REPORT_POWER_STATUS,(on)?1:0};
	return distribute_msg(itf,buf,sizeof(buf));
#endif
	return 0;
}

int cec_send_active_source(int itf, unsigned int cec_addr, unsigned short paddr) {
#if 0
	unsigned char buf[] = {cec_addr, CEC_OPCODE_ACTIVE_SOURCE, 0, 0};
	buf[2]=paddr>>8;
	buf[3]=paddr&0xff;

	return distribute_msg(itf,buf,sizeof(buf));
#endif
	return 0;
}

int cec_send_standby(int itf, unsigned int cec_addr) {
#if 0
	unsigned char buf[]={cec_addr,CEC_OPCODE_STANDBY};
	return distribute_msg(itf,buf,sizeof(buf));
#endif
	return 0;
}

int cec_send_image_view_on(int itf, unsigned int cec_addr) {
#if 0
	unsigned char buf[]={cec_addr,CEC_OPCODE_IMAGE_VIEW_ON};
	return distribute_msg(itf,buf,sizeof(buf));
#endif
	return 0;
}

int cec_send_cec_version(int itf, unsigned int cec_addr, unsigned char version) {
#if 0
	unsigned char buf[]={cec_addr,CEC_OPCODE_CEC_VERSION,version};
	return distribute_msg(itf,buf,sizeof(buf));
#endif
	return 0;
}

int cec_send_abort(int itf, unsigned int cec_addr,
		unsigned char opcode, unsigned char reason) {
#if 0
	unsigned char buf[]={cec_addr,CEC_OPCODE_FEATURE_ABORT,opcode,reason};
	return distribute_msg(itf,buf,sizeof(buf));
#endif
	return 0;
}



int cec_send(int itf, unsigned char *buf, int len) {
	return distribute_msg(itf,buf,len);
}

int cec_bus_send(unsigned char *buf, int len) {
	int rc;
	io_control(fd_cec,F_SETFL,(void *)0,0);
	rc=io_write(fd_cec,(char *)buf,len);
	io_control(fd_cec,F_SETFL,(void *)O_NONBLOCK,0);
	return rc;
}


static int handle_cec_data(int fd,int ev, void *dum) {
	int rc;

	rc=io_read(fd,(char *)cec_rbuf,sizeof(cec_rbuf));
	if (rc<0) {
		return rc;
	}

#if 0
	if (rc==9) {
                unsigned char sonyOn[]={0x0f,0xa0,0x08,0x00,0x46,0x00,0x04};
                if (__builtin_memcmp(cec_rbuf,sonyOn,sizeof(sonyOn))==0) {
                        /* wakeup  set top box */
			wakeup_usb_dev();

			/* Start amp */
			a1_power_on();
                }
        }
#endif

	distribute_msg(CEC_BUS, cec_rbuf,rc);
	return 0;
}

int distribute_msg(int itf, unsigned char *buf, int len) {
	int to=buf[0]&0xf;
	int rc=0;
	int rc1=0;
	int found=0;

	DUMP_DATA(itf, "cec: ", buf,len);
#if 0
	if (to==0xf) { // Broadcast
		int i;
		for (i=0;i<15;i++) {
			if (devt[i].callback&&(devt[i].bus!=itf)) {
				rc=devt[i].callback(buf,len);
			}
		}
	} else {
		if (devt[to].callback&&(devt[to].bus!=itf)) {
			rc=devt[to].callback(buf,len);
			found=1;
		}
	}
	if (itf!=CEC_BUS) {
		rc1=cec_bus_send(buf,len);
	}
	if (found) {
		return rc;
	}
#endif
	return rc1;
}

int cec_set_rec(int enable) {
	DPRINTF("cec_set_rec: %d\n", enable);
	io_control(fd_cec,F_SETFL,(void *)O_NONBLOCK,0);
	return 0;
}
