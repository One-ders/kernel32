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
 * @(#)cec_drv.c
 */
#include <sys.h>
#include <io.h>
#include <led_drv.h>

#include <hr_timer.h>
#include <gpio_drv.h>

#include "cec_drv.h"

#define MIN(a,b)	(a<b?a:b)

struct u_fd {
	struct device_handle dh;
	DRV_CBH callback;
	void *userdata;
	int events;
	int in_use;
};

#define MAX_USERS 4
static struct u_fd user_fd[MAX_USERS];

static struct u_fd *get_user_fd() {
	int i;
	for(i=0;i<MAX_USERS;i++) {
		if (!user_fd[i].in_use) {
			user_fd[i].in_use=1;
			return &user_fd[i];
		}
	}
	return 0;
}

static int wakeup_users(int ev) {
	int i;
	for(i=0;i<MAX_USERS;i++) {
		int rev;
		if ((user_fd[i].in_use)&&
			(rev=user_fd[i].events&ev)) {
			if (user_fd[i].callback) {
				user_fd[i].callback(&user_fd[i].dh, rev,user_fd[i].userdata);
			}
			user_fd[i].events&=~ev;
		}
	}
	return 0;
}

static struct device_handle *cec_timer_dh;
static struct driver *timerdrv;

static struct device_handle *led_dh;
static struct driver *leddrv;

static struct device_handle *pin_dh;
static struct driver *pindrv;

unsigned int blue=LED_BLUE;
unsigned int green=LED_GREEN;

#define CEC_IDLE	0
#define CEC_R_INIT	1
#define CEC_REC		2
#define CEC_REC_A	3
#define CEC_REC_P	4
#define CEC_TX		5
#define CEC_TX_GUARD	6
static unsigned char cec_state;

/* Substate for State CEC_R_INIT */
#define CEC_RSYNC_IDLE	0
#define CEC_RSYNC_LOW	1
#define CEC_RSYNC_TOH	2
#define CEC_RSYNC_HI1	3
#define CEC_RSYNC_HI2	4
#define CEC_RSYNC_TOL	5

/* Substate for State CEC_REC, CEC_REC_A and CEC_REC_P */
#define CEC_REC_IDLE	0
#define CEC_REC_PLB_I	1
#define CEC_REC_PLB_E	2
#define CEC_REC_PLB_N	3
#define CEC_REC_EOM_I	4
#define CEC_REC_EOM_E	5
#define CEC_REC_EOM_N	6
#define CEC_REC_ACK_I	7
#define CEC_REC_ACK_E	8

/* Substate for State CEC_TX */
#define CEC_TX_INIT_LO	0
#define CEC_TX_INIT_HI	1
#define CEC_TX_OUT_NEXT	2
#define CEC_TX_OUT_LO_0	3
#define CEC_TX_OUT_LO_1	4
#define CEC_TX_EOM_LO_0	5
#define CEC_TX_EOM_LO_1	6
#define CEC_TX_ACK_NEXT	7
#define CEC_TX_ACK_1	8
#define CEC_TX_ACK_2	9
#define CEC_TX_DONE	10
static unsigned char cec_sub_state;

/* CEC_FLAGS bits */
#define CECR_EOM	0x01
#define CECR_BYTE_READY	0x02
#define CECR_TO_ME	0x04
#define CECR_MC		0x08
#define CECR_ACK	0x10
#define CECR_START	0x20
static unsigned int cecr_flags;

static unsigned int cec_rx_tmp;
static unsigned int cec_rx;
static unsigned int cec_rbi;
static unsigned int cec_ack_mask;

#define RXB_LINES 4
static int rxIn;
static int rxOut;

static unsigned char rxbufs[4][17];
static unsigned int cec_rx_ix;

static unsigned int cec_promisc;
static int prev_pin_stat=-1;

/* A Cec device claims the bus by sinking the wire for 3700 mS */
static int start_r_sync(int pstate) {
	int uSec=3500;
	if (pstate) {
		sys_printf("false start_bit signal\n");
		return 0;
	}
	cecr_flags=0;
	cec_state=CEC_R_INIT;
	cec_sub_state=CEC_RSYNC_LOW;
	timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
	leddrv->ops->control(led_dh,LED_CTRL_ACTIVATE,&blue,sizeof(blue));
	return 0;
}

static int handle_r_init(int pstate) {
	switch(cec_sub_state) {
		case	CEC_RSYNC_IDLE:
		case	CEC_RSYNC_LOW:
		case	CEC_RSYNC_HI1:
		case	CEC_RSYNC_HI2: {
			/* Start Bit Failed */
			sys_printf("Start Bit Failed in substate %d pin irq\n", cec_sub_state);
			cec_state=CEC_IDLE;
			cec_sub_state=CEC_RSYNC_IDLE;
			timerdrv->ops->control(cec_timer_dh, HR_TIMER_CANCEL, 0, 0);
			leddrv->ops->control(led_dh,LED_CTRL_DEACTIVATE,&blue,sizeof(blue));
			wakeup_users(EV_WRITE);
			break;
		}
		case	CEC_RSYNC_TOH:
			if (pstate) {
				cec_sub_state=CEC_RSYNC_HI1;
			} else {
				sys_printf("Start Bit Failed in substate RSYNC_TOH\n");
				cec_state=CEC_IDLE;
				cec_sub_state=CEC_RSYNC_IDLE;
				timerdrv->ops->control(cec_timer_dh, HR_TIMER_CANCEL, 0, 0);
				leddrv->ops->control(led_dh,LED_CTRL_DEACTIVATE,&blue,sizeof(blue));
				wakeup_users(EV_WRITE);
			}
			break;
		case	CEC_RSYNC_TOL:
			if (!pstate)  {  /* verify that line is low */
				int uSec=1050;
				timerdrv->ops->control(cec_timer_dh, HR_TIMER_CANCEL, 0, 0);
				timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
				cec_state=CEC_REC;
				cec_sub_state=CEC_REC_PLB_I;
				cec_rbi=8;
				cec_rx_tmp=0;
				/*  Start bit done!!!!! */
			} else {
				/* Start Bit Failed */
				sys_printf("Start Bit Failed in substate RSYNC_TOL\n");
				cec_state=CEC_IDLE;
				cec_sub_state=CEC_RSYNC_IDLE;
				timerdrv->ops->control(cec_timer_dh, HR_TIMER_CANCEL, 0, 0);
				leddrv->ops->control(led_dh,LED_CTRL_DEACTIVATE,&blue,sizeof(blue));
			wakeup_users(EV_WRITE);
			}
			break;
	}
	return 0;
}

static void handle_r_sync_tout() {
	switch(cec_sub_state) {
		case CEC_RSYNC_IDLE:
		case CEC_RSYNC_TOH:
		case CEC_RSYNC_TOL: {
			/* Start bit failed */
			int pin_stat;
			pindrv->ops->control(pin_dh, GPIO_SENSE_PIN,&pin_stat,sizeof(pin_stat));
			sys_printf("Start Bit Failed in substate %d tout, pin is %d\n", cec_sub_state,pin_stat);
			cec_state=CEC_IDLE;
			cec_sub_state=CEC_RSYNC_IDLE;
			leddrv->ops->control(led_dh,LED_CTRL_DEACTIVATE,&blue,sizeof(blue));
			wakeup_users(EV_WRITE);
			break;
		}
		case CEC_RSYNC_LOW: {
			int uSec=500;
			cec_sub_state=CEC_RSYNC_TOH;
			timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
			break;
		}
		case CEC_RSYNC_HI1: {
			int uSec=400;
			cec_sub_state=CEC_RSYNC_HI2;
			timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
			break;
		}
		case CEC_RSYNC_HI2: {
			int uSec=500;
			cec_sub_state=CEC_RSYNC_TOL;
			timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
			break;
		}
	}
}

/*********************** End of Start bit, Start of Data bits ***********************/

/* Set up timeout for next bit probe, either data or eom bit */
static void cec_set_probe(void) {
	int uSec=1050;

	timerdrv->ops->control(cec_timer_dh, HR_TIMER_CANCEL, 0, 0);
	timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
	if (!cec_rbi) {
		cec_sub_state=CEC_REC_EOM_I;
	} else {
		cec_sub_state=CEC_REC_PLB_I;
	}
}

/* Setup timeout for next bit probe if passive ack, if active ack, sink bus */
static void cec_set_ack(void) {
	timerdrv->ops->control(cec_timer_dh, HR_TIMER_CANCEL, 0, 0);
	if (cec_state==CEC_REC_A) {  /* Active ack */
		int uSec=1500;
		/* sink bus line */
		timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
		pindrv->ops->control(pin_dh,GPIO_SINK_PIN,0,0);
		cec_sub_state=CEC_REC_ACK_E;
		cecr_flags|=CECR_BYTE_READY;
		if (cecr_flags&CECR_EOM) {
			if (cec_promisc||(cecr_flags&CECR_TO_ME)||(cecr_flags&CECR_MC)) {
				rxbufs[rxIn&(RXB_LINES-1)][0]=cec_rx_ix;
				rxIn++;
				wakeup_users(EV_WRITE|EV_READ);
			}
		}
	} else {
		int uSec=1050;
		timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
		cec_sub_state=CEC_REC_ACK_I;
	}
}

static void handle_cec_rec(int pstat) {

	if (pstat) return;

	switch(cec_sub_state) {
		case CEC_REC_IDLE:
		case CEC_REC_PLB_I:
		case CEC_REC_PLB_E:
		case CEC_REC_EOM_I:
		case CEC_REC_EOM_E:
		case CEC_REC_ACK_I:
		case CEC_REC_ACK_E:
			/* Rec fail */
			sys_printf("Rec Failed in substate %d, on pinirq\n", cec_sub_state);
			cec_state=CEC_IDLE;
			cec_sub_state=0;
			leddrv->ops->control(led_dh,LED_CTRL_DEACTIVATE,&blue,sizeof(blue));
			wakeup_users(EV_WRITE);
			break;
		case CEC_REC_PLB_N:
			cec_set_probe();
			break;
		case CEC_REC_EOM_N:
			cec_set_ack();
			break;
	}
}


static void cec_get_bit(void) {
	int uSec;
	int pin_stat;

	cec_rx_tmp<<=1;
	pindrv->ops->control(pin_dh, GPIO_SENSE_PIN,&pin_stat,sizeof(pin_stat));

	if (pin_stat) { /* a one bit */
		cec_rx_tmp|=1;
	} /* bit is zero, tmp is already zero */

	cec_rbi--;
	if (!cec_rbi) { /* we got a byte */
		if (cec_state==CEC_REC) {
			/* header byte */
			cecr_flags|=CECR_START;
			cec_rx=cec_rx_tmp;
			cec_rx_ix=0;
			if ((cec_rx&0xf)==0xf) { /* multicast */
				cecr_flags|=CECR_MC;
				cec_state=CEC_REC_P;
			} else if ((1<<(cec_rx&0xf))&cec_ack_mask) {
				cec_state=CEC_REC_A;
				cecr_flags|=CECR_TO_ME;
			} else {
				cec_state=CEC_REC_P;
			}
		} else {
			cecr_flags&=~CECR_START;
			cec_rx=cec_rx_tmp;
			cec_rx_tmp=0;
		}

		if (cec_promisc||(cecr_flags&CECR_TO_ME)||(cecr_flags&CECR_MC)) {
			rxbufs[rxIn&(RXB_LINES-1)][cec_rx_ix+1]=cec_rx;
			cec_rx_ix++;
		}
	}
	uSec=750;
	timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
	cec_sub_state=CEC_REC_PLB_E;
}

/* handle the end of message bit */
static void cec_get_eom(void) {
	int uSec;
	int pin_stat;

	pindrv->ops->control(pin_dh, GPIO_SENSE_PIN,&pin_stat,sizeof(pin_stat));
	if (pin_stat) {
		cecr_flags|=CECR_EOM;
	} else {
		cec_rbi=8;
	}

	uSec=700;
	timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
	cec_sub_state=CEC_REC_EOM_E;
}

static void cec_get_ack(void) {
	int pin_stat;
	int uSec;

	pindrv->ops->control(pin_dh, GPIO_SENSE_PIN,&pin_stat,sizeof(pin_stat));
	if (!pin_stat) { /*Ack is active low, some body else acked */
		cecr_flags|=CECR_ACK;
		uSec=700;
		timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
		if (cec_state==CEC_REC) {
			cec_state=CEC_REC_P;
		}
		cec_sub_state=CEC_REC_ACK_E;
		cecr_flags|=CECR_BYTE_READY;
	} else {
		cecr_flags&=~CECR_ACK;
		uSec=700;
		timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
		cec_sub_state=CEC_REC_ACK_E;
		cecr_flags|=CECR_BYTE_READY;
	}
	if (cecr_flags&CECR_EOM) {
		if (cec_promisc||(cecr_flags&CECR_TO_ME)||(cecr_flags&CECR_MC)) {
			rxbufs[rxIn&(RXB_LINES-1)][0]=cec_rx_ix;
			rxIn++;
			wakeup_users(EV_READ);
		}
	}
//	sys_printf("got byte %02x, flags %x\n",cec_rx, cecr_flags);
}

static void handle_rec_tout(void) {

	switch(cec_sub_state) {
		case CEC_REC_IDLE:
		case CEC_REC_PLB_N:
		case CEC_REC_EOM_N:
			/* Rec fail */
			sys_printf("Rec Failed in substate %d on tout\n", cec_sub_state);
			cec_state=CEC_IDLE;
			cec_sub_state=0;
			leddrv->ops->control(led_dh,LED_CTRL_DEACTIVATE,&blue,sizeof(blue));
			wakeup_users(EV_WRITE);
			break;
		case CEC_REC_PLB_I:
			cec_get_bit();
			break;
		case CEC_REC_PLB_E: {  /* verify line is high at end of bit */
			int pin_stat;
			int uSec=1000;
			pindrv->ops->control(pin_dh, GPIO_SENSE_PIN,&pin_stat,sizeof(pin_stat));
			if (!pin_stat) {
				sys_printf("Rec Failed in substate REC_PLB_E on tout\n");
				cec_state=CEC_IDLE;
				cec_sub_state=0;
				wakeup_users(EV_WRITE);
				break;
			}
			timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
			cec_sub_state=CEC_REC_PLB_N;
			break;
		}
		case CEC_REC_EOM_I:
			cec_get_eom();
			break;
		case CEC_REC_EOM_E: { /* verify line is high at end of bit */
			int pin_stat;
			int uSec=1000;
			pindrv->ops->control(pin_dh, GPIO_SENSE_PIN,&pin_stat,sizeof(pin_stat));
			if (!pin_stat) {
				sys_printf("Rec Failed in substate REC_EOM_E on timeout\n");
				cec_state=CEC_IDLE;
				cec_sub_state=0;
				leddrv->ops->control(led_dh,LED_CTRL_DEACTIVATE,&blue,sizeof(blue));
				wakeup_users(EV_WRITE);
				break;
			}
			timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
			cec_sub_state=CEC_REC_EOM_N;      /* wait for line to get low for ack */
			break;
		}
		case CEC_REC_ACK_I:
			cec_get_ack();
			break;
		case CEC_REC_ACK_E: {
			int uSec;
			/* release pin if held for active ack */
			if (cec_state==CEC_REC_A) {
				prev_pin_stat=1;   /* to ignore the irq, from pin going high */
				pindrv->ops->control(pin_dh,GPIO_RELEASE_PIN,0,0);
			}
			if (cecr_flags&CECR_EOM) {
				cec_state=CEC_TX_GUARD;
				cec_sub_state=0;
				uSec=2400*5;
				timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
				leddrv->ops->control(led_dh,LED_CTRL_DEACTIVATE,&blue,sizeof(blue));
				wakeup_users(EV_READ);
				break;
			}
			uSec=1200;
			timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
			cec_sub_state=CEC_REC_PLB_N;
			break;
		}
	}
}

/********************* Pin irq and Timer *****************************/

static void handle_tx_tout(void);
static int pin_irq(struct device_handle *dh, int ev, void *dum) {
	int pin_stat;
	pindrv->ops->control(pin_dh, GPIO_SENSE_PIN,&pin_stat,sizeof(pin_stat));
	if (pin_stat==prev_pin_stat) {
//		sys_printf("cec: spurious pin irq, pin at %d\n", pin_stat);
		goto pin_out;
	}
	prev_pin_stat=pin_stat;

	switch(cec_state) {
		case CEC_TX_GUARD:
			timerdrv->ops->control(cec_timer_dh, HR_TIMER_CANCEL, 0, 0);
			// fall through
		case CEC_IDLE:
			start_r_sync(pin_stat);
			break;
		case CEC_R_INIT:
			handle_r_init(pin_stat);
			break;
		case CEC_REC:
		case CEC_REC_A:
		case CEC_REC_P:
			handle_cec_rec(pin_stat);
			break;
		default:
			sys_printf("bad cec state in pin irq\n");
			break;
	}
pin_out:
	return 0;
}

int cec_timeout(struct device_handle *dh, int ev, void *dum) {
	switch(cec_state) {
		case CEC_IDLE:
			break;
		case CEC_TX:
			handle_tx_tout();
			break;
		case CEC_R_INIT:
			handle_r_sync_tout();
			break;
		case CEC_REC:
		case CEC_REC_A:
		case CEC_REC_P:
			handle_rec_tout();
			break;
		case CEC_TX_GUARD:
			cec_state=CEC_IDLE;
			wakeup_users(EV_WRITE);
			break;
	}
	return 0;
}

/*****   Transmit code ***********/

static int cec_bi;
static int olen;
static int cec_oix;
static int cec_byte_out;
static unsigned char cec_tx_buf[16];

#define CEC_TX_CHECK_ACK	1
#define CEC_TX_CHECK_NACK	2
#define CEC_TX_NO_ACK		4
static int cect_flags;

static int cec_tx_init_hi(void);
static int cec_send_bit(void);
static int cec_tx_out_0_toh(void);
static int cec_tx_out_1_toh(void);
static int cec_tx_eom_0_toh(void);
static int cec_tx_eom_1_toh(void);
static int cec_tx_get_ack_1(void);
static int cec_tx_get_ack_2(void);
static int cec_tx_get_ack_3(void);

static void handle_tx_tout(void) {

	switch(cec_sub_state) {
		case CEC_TX_INIT_LO:
			cec_tx_init_hi();
			break;
		case CEC_TX_INIT_HI:
		case CEC_TX_OUT_NEXT:
			cec_send_bit();
			break;
		case CEC_TX_OUT_LO_0:  /* we have kept the line low for a long time time to go hi */
			cec_tx_out_0_toh();
			break;
		case CEC_TX_OUT_LO_1:  /* time to lift the line for the reciver to probe a 1 */
			cec_tx_out_1_toh();
			break;
		case CEC_TX_EOM_LO_0: /* sent eom false, time to end bit */
			cec_tx_eom_0_toh();
			break;
		case CEC_TX_EOM_LO_1: /* we sent EOM, time to end bit */
			cec_tx_eom_1_toh();
			break;
		case CEC_TX_ACK_NEXT:   /* now set it up for receiption of the ack bit */
			cec_tx_get_ack_1();  /* start by sinking line */
			break;
		case CEC_TX_ACK_1:
			cec_tx_get_ack_2();  /* release line */
			break;
		case CEC_TX_ACK_2:
			cec_tx_get_ack_3();  /* probe line */
			break;
		case CEC_TX_DONE: {
			unsigned int flags;
			int uSec=2400*7;
			cec_sub_state=0;
			cec_state=CEC_TX_GUARD;
			leddrv->ops->control(led_dh,LED_CTRL_DEACTIVATE,&green,sizeof(green));
			wakeup_users(EV_WRITE);
			flags=GPIO_IRQ_ENABLE(0);
			pindrv->ops->control(pin_dh,GPIO_SET_FLAGS,&flags,sizeof(flags));
			timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
			break;
		}
	}
}


static int cec_tx_init_hi(void) {
	unsigned int uSec=800;
	timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
	pindrv->ops->control(pin_dh,GPIO_RELEASE_PIN,0,0);
	cec_sub_state=CEC_TX_INIT_HI;
	return 0;
}

static int cec_send_bit(void) {
	/* Init done, sink the bus for a while */
	pindrv->ops->control(pin_dh,GPIO_SINK_PIN,0,0);
	if (!cec_bi) { /* all bits sent, send eom */
		if (olen==(cec_oix+1))  { /* we have just sent the last byte */
			unsigned int uSec=600;
			timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
			cec_sub_state=CEC_TX_EOM_LO_1;
		} else {
			unsigned int uSec=1500;
			timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
			cec_sub_state=CEC_TX_EOM_LO_0;
		}
		return 0;
	}

	if (cec_byte_out&0x80) { /* keep line low for 600 uS */
		unsigned int uSec=600;
		timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
		cec_sub_state=CEC_TX_OUT_LO_1;
	} else {                 /* keep line low for 1500 uS */
		unsigned int uSec=1500;
		timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
		cec_sub_state=CEC_TX_OUT_LO_0;
	}
	cec_bi--;
	cec_byte_out<<=1;
	return 0;
}

static int cec_tx_out_0_toh(void) {
	unsigned int uSec=900;
	pindrv->ops->control(pin_dh,GPIO_RELEASE_PIN,0,0);
	timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
	cec_sub_state=CEC_TX_OUT_NEXT;
	return 0;
}

static int cec_tx_out_1_toh(void) {
	unsigned int uSec=1800;
	pindrv->ops->control(pin_dh,GPIO_RELEASE_PIN,0,0);
	timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
	cec_sub_state=CEC_TX_OUT_NEXT;
	return 0;
}

static int cec_tx_eom_0_toh(void) {
	unsigned int uSec=900;
	pindrv->ops->control(pin_dh,GPIO_RELEASE_PIN,0,0);
	timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
	cec_sub_state=CEC_TX_ACK_NEXT;
	return 0;
}

static int cec_tx_eom_1_toh(void) {
	unsigned int uSec=1800;
	pindrv->ops->control(pin_dh,GPIO_RELEASE_PIN,0,0);
	timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
	cec_sub_state=CEC_TX_ACK_NEXT;
	return 0;
}

static int cec_tx_get_ack_1(void) {
	unsigned int uSec=600;
	pindrv->ops->control(pin_dh,GPIO_SINK_PIN,0,0);
	timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
	cec_sub_state=CEC_TX_ACK_1;
	return 0;
}

static int cec_tx_get_ack_2(void) {
	unsigned int uSec=450;
	pindrv->ops->control(pin_dh,GPIO_RELEASE_PIN,0,0);
	timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
	cec_sub_state=CEC_TX_ACK_2;
	return 0;
}

static int cec_tx_get_ack_3(void) {
	unsigned int uSec=1350;  /* should be 1400, but compensate for previous state */
	int pin_stat;
	pindrv->ops->control(pin_dh, GPIO_SENSE_PIN,&pin_stat,sizeof(pin_stat));

	if (cect_flags&CEC_TX_CHECK_ACK) {
		if (pin_stat) goto no_ack;
	} else if (cect_flags&CEC_TX_CHECK_NACK) {
		if (!pin_stat) goto got_nack;
	}

	cec_oix++;
	if (cec_oix==olen) {               /* already send all bytes, change state to done */
		cec_sub_state=CEC_TX_DONE;
		timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
		return 0;
	}
	cec_sub_state=CEC_TX_OUT_NEXT;     /* state for starting receiving next byte */
	timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));

	cec_byte_out=cec_tx_buf[cec_oix];
	cec_bi=8;

	return 0;

no_ack:
got_nack:
	cect_flags|=CEC_TX_NO_ACK;
	cec_sub_state=CEC_TX_DONE;
	timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
	return 0;
}

static int send_cec(struct u_fd *u, unsigned char *data, int len) {
	unsigned int flags;
	unsigned char addr=data[0];
	unsigned int uSec;

	if (len>sizeof(cec_tx_buf)) {
		return -DRV_ERR;
	}
	if ((cec_state!=CEC_IDLE)||(cect_flags)) {
		u->events|=EV_WRITE;
		return -DRV_AGAIN;
	}

	leddrv->ops->control(led_dh,LED_CTRL_ACTIVATE,&green,sizeof(green));
	flags=GPIO_IRQ_ENABLE(0);
	pindrv->ops->control(pin_dh,GPIO_CLR_FLAGS,&flags,sizeof(flags));
	cect_flags=0;

	if ((addr&0xf)==0xf) { /* broadcast */
		cect_flags|=CEC_TX_CHECK_NACK;
	} else {
		cect_flags|=CEC_TX_CHECK_ACK;
	}
	cec_oix=0;
	cec_bi=8;

	__builtin_memcpy(cec_tx_buf,data,len);
	olen=len;
	cec_byte_out=cec_tx_buf[0];

	cec_state=CEC_TX;
	cec_sub_state=CEC_TX_INIT_LO;

	uSec=3700;
	timerdrv->ops->control(cec_timer_dh, HR_TIMER_SET, &uSec, sizeof(uSec));
	pindrv->ops->control(pin_dh,GPIO_SINK_PIN,0,0);
	u->events|=EV_WRITE;
	return -DRV_INPROGRESS;
}

/**** Driver API ********************/

static struct device_handle *cec_drv_open(void *inst, DRV_CBH cb, void *udata) {
	struct u_fd *user_fd=get_user_fd();

	if (!user_fd) return 0;
	user_fd->callback=cb;
	user_fd->userdata=udata;
	user_fd->events=0;
	return &user_fd->dh;
}

static int cec_drv_close(struct device_handle *dh) {
	struct u_fd *u=(struct u_fd *)dh;

	if (u)  {
		u->in_use=0;
	}
	return 0;
}

static int cec_drv_control(struct device_handle *dh, int cmd, void *arg1, int size) {
	struct u_fd *u=(struct u_fd *)dh;
	if (!u) return -1;
	switch(cmd) {
		case RD_CHAR: {
			int len;
			unsigned char *buf=(unsigned char *)arg1;
			if (!(rxIn-rxOut)) {
				user_fd->events|=EV_READ;
				return -DRV_AGAIN;
			}
			len=MIN(size,rxbufs[rxOut&(RXB_LINES-1)][0]);
			__builtin_memcpy(buf,&rxbufs[rxOut&(RXB_LINES-1)][1],len);
			rxOut++;
			return len;
			break;
		}
		case WR_CHAR: {
			return send_cec(u,arg1,size);
			break;
		}
		case IO_POLL: {
			unsigned int events=(unsigned int)arg1;
			unsigned int revents=0;
			if (EV_WRITE&events) {
				if (!((cec_state!=CEC_IDLE)||(cect_flags))) {
					revents|=EV_WRITE;
				} else {
					user_fd->events|=EV_WRITE;
				}
			}
			if (EV_READ&events) {
				if (rxIn-rxOut) {
					revents|=EV_READ;
				} else {
					user_fd->events|=EV_READ;
				}
			}
			return revents;
			break;
		}
		case WR_GET_RESULT: {
			if (cect_flags) {
				unsigned int *val=(unsigned int *)arg1;
				*val=(cect_flags&CEC_TX_NO_ACK)?-2:olen;
				cect_flags=0;
				return 0;
			} else {
				sys_printf("cec_drv: WR_GET_RESULT  bad state\n");
				return -1;
			}
			break;
		}
		case CEC_SET_ACK_MSK: {
			unsigned int *mask=(unsigned int *)arg1;
			cec_ack_mask=*mask;
			return 0;
			break;
		}
		case CEC_GET_ACK_MSK: {
			unsigned int *mask=(unsigned int *)arg1;
			*mask=cec_ack_mask;
			return 0;
			break;
		}
		case CEC_SET_PROMISC: {
			unsigned int *val=(unsigned int *)arg1;
			cec_promisc=*val;
			return 0;
			break;
		}
		case CEC_GET_PROMISC: {
			unsigned int *val=(unsigned int *)arg1;
			*val=cec_promisc;
			return 0;
			break;
		}
	}
	return -1;
}

static int cec_drv_init(void *inst) {
	return 0;
}

static int cec_drv_start(void *inst) {
	int flags;
	int rc;
	int pin;

	leddrv=driver_lookup(LED_DRV);
	if (!leddrv) return 0;
	led_dh=leddrv->ops->open(leddrv->instance,0,0);
	if (!led_dh) return -1;

	pindrv=driver_lookup(GPIO_DRV);
	if (!pindrv) return 0;
	pin_dh=pindrv->ops->open(pindrv->instance,pin_irq,0);
	if (!pin_dh) return -1;

	timerdrv=driver_lookup(HR_TIMER);
	if (!timerdrv) return 0;

	cec_timer_dh=timerdrv->ops->open(timerdrv->instance,cec_timeout,0);
	if (!cec_timer_dh) {
		timerdrv=0;
		return -1;
	}
	/* */
			/*PC4 pin for CEC */
	pin=GPIO_PIN(PC,4);
	rc=pindrv->ops->control(pin_dh,GPIO_BIND_PIN,&pin,sizeof(pin));
	if (rc<0) {
		sys_printf("pin_assignment failed\n");
		return -1;
	}

	flags=GPIO_DIR(0,GPIO_BUSPIN);
	flags=GPIO_DRIVE(flags,GPIO_PULLUP);
	flags=GPIO_SPEED(flags,GPIO_SPEED_MEDIUM);
	flags=GPIO_IRQ_ENABLE(flags);
	rc=pindrv->ops->control(pin_dh,GPIO_SET_FLAGS,&flags,sizeof(flags));
	if (rc<0) {
		sys_printf("pin_flags update failed\n");
		return -1;
	}

	return 0;
}

static struct driver_ops cec_drv_ops = {
	cec_drv_open,
	cec_drv_close,
	cec_drv_control,
	cec_drv_init,
	cec_drv_start,
};

static struct driver cec_drv = {
	CEC_DRV,
	0,
	&cec_drv_ops,
};

void init_cec(void) {
	driver_publish(&cec_drv);
}

INIT_FUNC(init_cec);
