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
 * @(#)a1_drv.c
 */
#include <sys.h>
#include <io.h>
#include <led_drv.h>
#include <hr_timer.h>
#include <gpio_drv.h>

#include "a1_drv.h"

#if 1
#define TIM_START	2400
#define TIM_BASIC	600
#define TIM_HIGH	600
#define TIM_LOW_0	600
#define TIM_LOW_1	1200
#endif

#define RXB_SIZE 64
#define RXB_MASK (RXB_SIZE-1)
#define TXB_SIZE 64
#define TXB_MASK (TXB_SIZE-1)

#define MAX_USERS 4

static struct driver *leddrv;
static struct driver *pindrv;
static struct driver *timerdrv;

unsigned int amber=LED_AMBER;
unsigned int red=LED_RED;

#define A1_STATE_IDLE		0
#define A1_STATE_RX_INIT0	1
#define A1_STATE_RX_INIT1	2
#define A1_STATE_RX_DATA0	3
#define A1_STATE_RX_DATA1	4
#define A1_STATE_TX_INIT0	5
#define A1_STATE_TX_INIT1	6
#define A1_STATE_TX_DATA0	7
#define A1_STATE_TX_DATA1	8

struct a1_data {
	struct device_handle *led_dh;
	struct device_handle *pin_dh;
	struct device_handle *timer_dh;
	volatile int	prev_pin_stat;
	unsigned int	state;
	volatile int	rx_tout_cnt;
	volatile int	rbi;
	volatile int 	rxbyte;
	volatile int	rxlen;
	volatile int	rxIn;
	volatile int	rxOut;
	volatile int	txbyte;
	volatile int	txbi;
	volatile int	txlen;
	volatile int	tx_oix;
	volatile unsigned char rxbuf[RXB_SIZE];
	volatile int	txIn;
	volatile int	txOut;
	volatile unsigned char txbuf[TXB_SIZE];
};

struct user_data {
	struct device_handle dh;
	DRV_CBH callback;
	void	*userdata;
	int	events;
	int	in_use;
	struct a1_data *a1_data;
};

static struct a1_data a1_data_0 = {
	prev_pin_stat: -1,
};

static struct user_data user_data[MAX_USERS];

static struct user_data *get_user_data(void) {
	int i;
	for(i=0;i<MAX_USERS;i++) {
		if (!user_data[i].in_use) {
			user_data[i].in_use=1;
			return &user_data[i];
		}
	}
	return 0;
}

static int wakeup_users(struct a1_data *a1,int ev) {
	int i;
	for(i=0;i<MAX_USERS;i++) {
		if ((user_data[i].in_use) &&
			(user_data[i].a1_data==a1) &&
			(user_data[i].events&ev) &&
			(user_data[i].callback)) {
			user_data[i].callback(&user_data[i].dh,ev&user_data[i].events,user_data[i].userdata);
		}
	}
	return 0;
}

/************** The Irq'ers ************/
static int handle_rx_start(struct a1_data *a1,int pin_lev);
static int handle_rx_init0(struct a1_data *a1,int pin_lev);
static int handle_rx_init1(struct a1_data *a1,int pin_lev);
static int handle_rx_data0(struct a1_data *a1,int pin_lev);
static int handle_rx_data1(struct a1_data *a1,int pin_lev);
static int a1_send_bit(struct a1_data *a1);
static int a1_start_tx(struct a1_data *a1);

static int pin_irq(struct device_handle *dh, int ev, void *dum) {
	int pin_stat;
	struct a1_data *a1=(struct a1_data *)dum;

	pindrv->ops->control(a1->pin_dh,GPIO_SENSE_PIN,&pin_stat,sizeof(pin_stat));
	if (pin_stat==a1->prev_pin_stat) return 0;
	a1->prev_pin_stat=pin_stat;

	switch(a1->state) {
		case A1_STATE_IDLE:
			handle_rx_start(a1,pin_stat);
			break;
		case A1_STATE_RX_INIT0:
			handle_rx_init0(a1,pin_stat);
			break;
		case A1_STATE_RX_INIT1:
			handle_rx_init1(a1,pin_stat);
			break;
		case A1_STATE_RX_DATA0:
			handle_rx_data0(a1,pin_stat);
			break;
		case A1_STATE_RX_DATA1:
			handle_rx_data1(a1,pin_stat);
			break;
		default:
			sys_printf("Sony A1 protocol: bad state in pin irq\n");
	};
	return 0;
}

static int a1_timeout(struct device_handle *dh, int ev, void *dum) {
	struct a1_data *a1=(struct a1_data *)dum;

	switch (a1->state) {
		case A1_STATE_RX_INIT0:
		case A1_STATE_RX_DATA0: {  /* counting periods */
			int uSec=TIM_BASIC;
			a1->rx_tout_cnt++;
			timerdrv->ops->control(a1->timer_dh,HR_TIMER_SET,&uSec,sizeof(uSec));
			break;
		}
		case A1_STATE_RX_INIT1: { /* means we have a startbit but timedout on data */
			a1->state=A1_STATE_IDLE;
			leddrv->ops->control(a1->led_dh,LED_CTRL_DEACTIVATE,&red,sizeof(red));
			sys_printf("sender never started to send data\n");
			break;
		}
		case A1_STATE_RX_DATA1: {
			a1->state=A1_STATE_IDLE;
			leddrv->ops->control(a1->led_dh,LED_CTRL_DEACTIVATE,&red,sizeof(red));
			if (a1->rbi==8) {
//				sys_printf("frame received: rxIn %d len %d\n",a1->rxIn,a1->rxlen);
				a1->rxbuf[a1->rxIn&RXB_MASK]=a1->rxlen;
				a1->rxIn+=(a1->rxlen+1);
//				sys_printf("new rxIn %d, rxOut %d\n", a1->rxIn,a1->rxOut);
				wakeup_users(a1,EV_READ|EV_WRITE);
			} else {
				sys_printf("timeout during data reception\n");
			}
			if (a1->txIn-a1->txOut)  {
				a1_start_tx(a1);
			}
			break;
		}
		case A1_STATE_TX_INIT0: {
			unsigned int uSec=TIM_HIGH;
			a1->state=A1_STATE_TX_DATA0;
			pindrv->ops->control(a1->pin_dh,GPIO_RELEASE_PIN,0,0);
			timerdrv->ops->control(a1->timer_dh,HR_TIMER_SET,&uSec,sizeof(uSec));
			break;
		}
		case A1_STATE_TX_DATA0: {
			int pin_stat;
			pindrv->ops->control(a1->pin_dh,GPIO_SENSE_PIN,&pin_stat,sizeof(pin_stat));
			if (!pin_stat) {
				int flags=GPIO_IRQ_ENABLE(0);
				a1->prev_pin_stat=1;
				pindrv->ops->control(a1->pin_dh,GPIO_SET_FLAGS,&flags,sizeof(flags));
				leddrv->ops->control(a1->led_dh,LED_CTRL_DEACTIVATE,&amber,sizeof(amber));
				sys_printf("a1: bus collision at start of send bit\n");
				a1->state=A1_STATE_IDLE;
				if (a1->txIn-a1->txOut)  {
					a1_start_tx(a1);
				} else {
					wakeup_users(a1,EV_WRITE);
				}
				return 0;
			}
			a1_send_bit(a1);
			break;
		}
		case A1_STATE_TX_DATA1: {
			unsigned int uSec=TIM_HIGH;
			a1->state=A1_STATE_TX_DATA0;
			pindrv->ops->control(a1->pin_dh,GPIO_RELEASE_PIN,0,0);
			timerdrv->ops->control(a1->timer_dh,HR_TIMER_SET,&uSec,sizeof(uSec));
			break;
		}
		default:
			sys_printf("got timeout in state %d\n", a1->state);
			break;
	}

	return 0;
}

/******** handlers for different state *************/

/************** Receive functions ******************/

/* A1 IDLE */

static int handle_rx_start(struct a1_data *a1, int pin_lev) {
	int uSec=TIM_BASIC/2;
	if (pin_lev) {
		/* stay in idle, must have been a disturbance in the force */
		sys_printf("got a 'pin to high' transition in idle??\n");
		return 0;
	}
	a1->rx_tout_cnt=0;
	a1->state=A1_STATE_RX_INIT0;
	a1->rxlen=0;
	timerdrv->ops->control(a1->timer_dh,HR_TIMER_SET,&uSec,sizeof(uSec));
	leddrv->ops->control(a1->led_dh,LED_CTRL_ACTIVATE,&red,sizeof(red));
	return 0;
}

/* A1 RX INIT 0:  the remote device is keeping the line low, we count how long on a high transition*/

static int handle_rx_init0(struct a1_data *a1,int pin_lev) {
	int uSec=TIM_START;
	timerdrv->ops->control(a1->timer_dh,HR_TIMER_CANCEL,0,0);
	if (!pin_lev) {
		a1->state=A1_STATE_IDLE;
		leddrv->ops->control(a1->led_dh,LED_CTRL_DEACTIVATE,&red,sizeof(red));
		sys_printf("a1, got a 'pin to low' transition in init0\n");
		return 0;
	}

	if (a1->rx_tout_cnt!=4) {
		a1->state=A1_STATE_IDLE;
		leddrv->ops->control(a1->led_dh,LED_CTRL_DEACTIVATE,&red,sizeof(red));
		sys_printf("a1, in INIT0,  got a 'pin low' wrong timer for startbit %d\n",a1->rx_tout_cnt);
		return 0;
	}

	a1->state=A1_STATE_RX_INIT1;
	timerdrv->ops->control(a1->timer_dh,HR_TIMER_SET,&uSec,sizeof(uSec));
	a1->rbi=8;
	a1->rxbyte=0;
	return 0;
}

/* A1 RX INIT 1: We are waiting for the line to go lo, the start of the first data bits */

static int handle_rx_init1(struct a1_data *a1, int pin_lev) {
	int uSec=TIM_BASIC/2;

	timerdrv->ops->control(a1->timer_dh,HR_TIMER_CANCEL,0,0);
	if (pin_lev) {
		a1->state=A1_STATE_IDLE;
		leddrv->ops->control(a1->led_dh,LED_CTRL_DEACTIVATE,&red,sizeof(red));
		sys_printf("a1, got a 'pin to high' transition in init0\n");
		return 0;
	}

	a1->state=A1_STATE_RX_DATA0;
	a1->rx_tout_cnt=0;
	timerdrv->ops->control(a1->timer_dh,HR_TIMER_SET,&uSec,sizeof(uSec));

//	sys_printf("a1: successfully synched, startbit received\n");
	return 0;
}

/* A1 RX DATA 0: We are waiting for the line to go hi,  */
static int handle_rx_data0(struct a1_data *a1,int pin_lev) {
	int uSec=TIM_START;
	timerdrv->ops->control(a1->timer_dh,HR_TIMER_CANCEL,0,0);
	if (!pin_lev) {
		a1->state=A1_STATE_IDLE;
		leddrv->ops->control(a1->led_dh,LED_CTRL_DEACTIVATE,&red,sizeof(red));
		sys_printf("a1, got a 'pin to low' transition in data0\n");
		return 0;
	}

	if (a1->rx_tout_cnt==2) {
		a1->rxbyte|=1;
	} else if (a1->rx_tout_cnt!=1) {
		a1->state=A1_STATE_IDLE;
		leddrv->ops->control(a1->led_dh,LED_CTRL_DEACTIVATE,&red,sizeof(red));
		sys_printf("a1, bit indicator bad %d\n",a1->rx_tout_cnt);
		return 0;
	}
	a1->rbi--;
	if (!a1->rbi) {
		int len=a1->rxlen;
		int size=RXB_SIZE-(a1->rxIn-a1->rxOut);
//		sys_printf("a1: got a byte %02x\n", a1->rxbyte);
		len++;
		if (len<size) {
			a1->rxbuf[(a1->rxIn+len)&RXB_MASK]=a1->rxbyte;
			a1->rxlen=len;
		} else {
			sys_printf("a1: skipping byte, len %d, size %d\n",len<size);
		}
		a1->rbi=8;
		a1->rxbyte=0;
	} else {
		a1->rxbyte<<=1;
	}
	a1->state=A1_STATE_RX_DATA1;
	timerdrv->ops->control(a1->timer_dh,HR_TIMER_SET,&uSec,sizeof(uSec));
	return 0;
}

/* A1 RX DATA 1:  */
static int handle_rx_data1(struct a1_data *a1, int pin_lev) {
	int uSec=TIM_BASIC/2;

	timerdrv->ops->control(a1->timer_dh,HR_TIMER_CANCEL,0,0);
	if (pin_lev) {
		a1->state=A1_STATE_IDLE;
		leddrv->ops->control(a1->led_dh,LED_CTRL_DEACTIVATE,&red,sizeof(red));
		sys_printf("a1, got a 'pin to high' transition in data1\n");
		return 0;
	}

	a1->state=A1_STATE_RX_DATA0;
	a1->rx_tout_cnt=0;
	timerdrv->ops->control(a1->timer_dh,HR_TIMER_SET,&uSec,sizeof(uSec));
	return 0;
}

/*************** Transmit functions **************/

int a1_send_bit(struct a1_data *a1) {
	unsigned int bit_1=a1->txbyte&0x80;
	unsigned int uSec;

	if (!a1->txbi) {
		int flags=GPIO_IRQ_ENABLE(0);
		leddrv->ops->control(a1->led_dh,LED_CTRL_DEACTIVATE,&amber,sizeof(amber));
		a1->state=A1_STATE_IDLE;
		a1->prev_pin_stat=1;
		pindrv->ops->control(a1->pin_dh,GPIO_SET_FLAGS,&flags,sizeof(flags));
		if (a1->txIn-a1->txOut)  {
			a1_start_tx(a1);
		} else {
			wakeup_users(a1,EV_WRITE);
		}
		return 0;
	}
	a1->state=A1_STATE_TX_DATA1;
	if (bit_1) {
		uSec=TIM_LOW_1;
	} else {
		uSec=TIM_LOW_0;
	}
	timerdrv->ops->control(a1->timer_dh,HR_TIMER_SET,&uSec,sizeof(uSec));
	pindrv->ops->control(a1->pin_dh,GPIO_SINK_PIN,0,0);

	a1->txbi--;
	if (a1->txbi) {
		a1->txbyte<<=1;
	} else {
		a1->tx_oix++;
		if (a1->tx_oix<a1->txlen) {
			a1->txbyte=a1->txbuf[(a1->tx_oix+a1->txOut+1)&TXB_MASK];
			a1->txbi=8;
		} else {
			a1->txOut+=(a1->txlen+1);
		}
	}
	return 0;
}

static int a1_start_tx(struct a1_data *a1) {
	unsigned int uSec=TIM_START;
	unsigned int flags=GPIO_IRQ_ENABLE(0);

	pindrv->ops->control(a1->pin_dh,GPIO_CLR_FLAGS,&flags,sizeof(flags));
	a1->state=A1_STATE_TX_INIT0;
	a1->txlen=a1->txbuf[a1->txOut&TXB_MASK];
	a1->tx_oix=0;
	a1->txbyte=a1->txbuf[(a1->tx_oix+a1->txOut+1)&TXB_MASK];
	a1->txbi=8;

	timerdrv->ops->control(a1->timer_dh,HR_TIMER_SET,&uSec,sizeof(uSec));
	pindrv->ops->control(a1->pin_dh,GPIO_SINK_PIN,0,0);

	leddrv->ops->control(a1->led_dh,LED_CTRL_ACTIVATE,&amber,sizeof(amber));
	return 0;
}

/*****  Driver API *****/

static struct device_handle *a1_drv_open(void *inst, DRV_CBH cb, void *udata) {
	struct user_data *u=get_user_data();

	if (!u) return 0;
	u->a1_data=(struct a1_data *)inst;
	u->callback=cb;
	u->userdata=udata;
	u->events=0;
	return &u->dh;
}

static int a1_drv_close(struct device_handle *dh) {
	struct user_data *u=(struct user_data *)dh;
	if (!u) {
		return 0;
	}
	if (u) {
		u->in_use=0;
		u->a1_data=0;
	}
	return 0;
}

static int a1_drv_control(struct device_handle *dh, int cmd, void *arg, int size) {
	struct user_data *u=(struct user_data *)dh;
	struct a1_data *a1;

	if (!u) {
		return -1;
	}
	a1=u->a1_data;

	switch(cmd) {
		case RD_CHAR: {
			int len,i;
			unsigned char *buf=(unsigned char *)arg;
			if (!(a1->rxIn-a1->rxOut)) {
				u->events|=EV_READ;
				return -DRV_AGAIN;
			}
			u->events&=~EV_READ;
			len=a1->rxbuf[a1->rxOut&RXB_MASK];
			if (size<len) {
				return -1;
			}
			a1->rxbuf[a1->rxOut&RXB_MASK]=0;
			a1->rxOut++;
			for(i=0;i<len;i++) {
				buf[i]=a1->rxbuf[(a1->rxOut+i)&RXB_MASK];
			}
			a1->rxOut+=len;
			return len;
			break;
		}
		case WR_CHAR: {
			int i;
			unsigned char *buf=(unsigned char *)arg;
			if ((a1->txIn-a1->txOut)>=(TXB_SIZE-size)) {
				u->events|=EV_WRITE;
				return -DRV_AGAIN;
			}
			u->events&=~EV_WRITE;
			a1->txbuf[a1->txIn&TXB_MASK]=size;
			a1->txIn++;
			for(i=0;i<size;i++) {
				a1->txbuf[a1->txIn&TXB_MASK]=buf[i];
				a1->txIn++;
			}
			if (a1->state==A1_STATE_IDLE) {
				a1_start_tx(a1);
			}
			return size;
			break;
		}
		case IO_POLL: {
			unsigned int events=(unsigned int)arg;
			unsigned int revents=0;
			if (EV_WRITE&events) {
				if ((a1->txIn-a1->txOut)>=TXB_SIZE) {
					u->events|=EV_WRITE;
				} else {
					revents|=EV_WRITE;
				}
			}
			if (EV_READ&events) {
				if (a1->rxIn-a1->rxOut) {
					revents|=EV_READ;
				} else {
					u->events|=EV_READ;
				}
			}
			return revents;
			break;
		}
	}
	return -1;
}

static int a1_drv_init(void *inst) {
	return 0;
}

static int a1_drv_start(void *inst) {
	struct a1_data *a1=(struct a1_data *)inst;
	int flags;
	int rc;
	int pin;

	/* Open Led driver so we can flash the leds a bit */
	if (!leddrv) leddrv=driver_lookup(LED_DRV);
	if (!leddrv) return 0;
	a1->led_dh=leddrv->ops->open(leddrv->instance,0,0);
	if (!a1->led_dh) return 0;

	/* Open gpio driver, this will be our cable to the amp */
	if (!pindrv) pindrv=driver_lookup(GPIO_DRV);
	if (!pindrv) goto out1;
	a1->pin_dh=pindrv->ops->open(pindrv->instance,pin_irq,(void *)a1);
	if (!a1->pin_dh) goto out1;

	/* Open High Resolution timer for pulse meassurements */
	if (!timerdrv) timerdrv=driver_lookup(HR_TIMER);
	if (!timerdrv) goto out2;
	a1->timer_dh=timerdrv->ops->open(timerdrv->instance,a1_timeout,(void *)a1);
	if (!a1->timer_dh) goto out2;

	if (a1==&a1_data_0) {
		pin=GPIO_PIN(PB,0);
	} else {
		sys_printf("A1 protocol driver: error no pin assigned for driver\n");
		goto out3;
	}

	/* Program pin to be open drain, pull up, irq */
	rc=pindrv->ops->control(a1->pin_dh,GPIO_BIND_PIN,&pin,sizeof(pin));
	if (rc<0) goto out3;

	flags=GPIO_DIR(0,GPIO_BUSPIN);
	flags=GPIO_DRIVE(flags,GPIO_PULLUP);
	flags=GPIO_SPEED(flags,GPIO_SPEED_MEDIUM);
	flags=GPIO_IRQ_ENABLE(flags);
	rc=pindrv->ops->control(a1->pin_dh,GPIO_SET_FLAGS,&flags,sizeof(flags));
	if (rc<0) {
		sys_printf("A1 protocol driver: pin_flags update failed\n");
		goto out3;
	}
	sys_printf("A1 protocol driver: Started\n");

	return 0;

out3:
	sys_printf("a1: failed to bind pin to GPIO\n");
	timerdrv->ops->close(a1->timer_dh);

out2:
	sys_printf("a1: failed to open HR_TIMER\n");
	pindrv->ops->close(a1->pin_dh);

out1:
	sys_printf("a1: failed to open GPIO_DRV\n");
	leddrv->ops->close(a1->led_dh);
	return 0;
}


static struct driver_ops a1_drv_ops = {
	a1_drv_open,
	a1_drv_close,
	a1_drv_control,
	a1_drv_init,
	a1_drv_start,
};

static struct driver a1_drv = {
	A1_DRV0,
	&a1_data_0,
	&a1_drv_ops,
};

void init_a1(void) {
	driver_publish(&a1_drv);
}

INIT_FUNC(init_a1);
