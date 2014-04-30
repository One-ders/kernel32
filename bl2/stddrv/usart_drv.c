/* $FrameWorks: , v1.1 2014/04/07 21:44:00 anders Exp $ */

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
 * @(#)usart_drv.c
 */
#include "sys.h"
#include "io.h"
#include <stm32/devices.h>
#include <stm32/stm32f407.h>

#include "usart_drv.h"



#if 0

typedef struct
{
  __IO uint16_t SR;         /*!< USART Status register,                   Address offset: 0x00 */
  uint16_t      RESERVED0;  /*!< Reserved, 0x02                                                */
  __IO uint16_t DR;         /*!< USART Data register,                     Address offset: 0x04 */
  uint16_t      RESERVED1;  /*!< Reserved, 0x06                                                */
  __IO uint16_t BRR;        /*!< USART Baud rate register,                Address offset: 0x08 */
  uint16_t      RESERVED2;  /*!< Reserved, 0x0A                                                */
  __IO uint16_t CR1;        /*!< USART Control register 1,                Address offset: 0x0C */
  uint16_t      RESERVED3;  /*!< Reserved, 0x0E                                                */
  __IO uint16_t CR2;        /*!< USART Control register 2,                Address offset: 0x10 */
  uint16_t      RESERVED4;  /*!< Reserved, 0x12                                                */
  __IO uint16_t CR3;        /*!< USART Control register 3,                Address offset: 0x14 */
  uint16_t      RESERVED5;  /*!< Reserved, 0x16                                                */
  __IO uint16_t GTPR;       /*!< USART Guard time and prescaler register, Address offset: 0x18 */
  uint16_t      RESERVED6;  /*!< Reserved, 0x1A                                                */
} USART_TypeDef;



#endif

#define TXB_SIZE 1024
#define TXB_MASK (TXB_SIZE-1)
#define IX(a) (a&TXB_MASK)

#define RX_BSIZE 16

struct usart_data {
	int chip_dead;
	char tx_buf[TXB_SIZE];
	volatile int tx_in;
	volatile int tx_out;
	volatile int txr; /*transmitter running*/

	char rx_buf[RX_BSIZE];
	volatile int rx_i;
	volatile int rx_o;
	struct blocker_list rblocker_list;
	struct blocker_list wblocker_list;
};


struct user_data {
	struct device_handle dh;
	struct usart_data *drv_data;
	DRV_CBH	callback;
	void  	*userdata;
	int	events;
};

static struct usart_data usart_data0;

#define MAX_USERS 4
static struct user_data udata[MAX_USERS];
static int (*usart_putc_fnc)(struct user_data *, int c);
static int usart_putc(struct user_data *u, int c);

void USART3_IRQHandler(void) {
	unsigned int st=USART3->SR;

	if (st&USART_SR_LBD) {
		usart_data0.chip_dead=1;
		usart_data0.tx_out=usart_data0.tx_in;
		USART3->SR&=~USART_SR_LBD;	
		if (usart_data0.wblocker_list.first){
			 sys_wakeup_from_list(&usart_data0.wblocker_list);
		}
		sys_printf("SR_LBD\n");
		return;
	} else {
		usart_data0.chip_dead=0;
	}
	
//	sys_printf("got usart irq, status %x\n", USART3->SR);
	if ((st&(USART_SR_TXE|USART_SR_TC))&&(usart_data0.tx_in-usart_data0.tx_out)) {
		if (usart_putc_fnc==usart_putc) {
			USART3->DR=usart_data0.tx_buf[IX(usart_data0.tx_out)];
			usart_data0.tx_out++;
			st&=~USART_SR_TC;	// Dont shut off transmitter if we send some data
			if ((usart_data0.tx_in-usart_data0.tx_out)==0) {
				if (usart_data0.wblocker_list.first) {
					sys_wakeup_from_list(&usart_data0.wblocker_list);
				}
			}
		}
	}  else if((st&USART_SR_TC)) {
		if (usart_putc_fnc==usart_putc) {
			USART3->CR1&=~(USART_CR1_TE|USART_CR1_TXEIE|USART_CR1_TCIE);
			usart_data0.txr=0;
		} else {
			USART3->CR1&=~(USART_CR1_TXEIE|USART_CR1_TCIE);
		}
	}

	if (st&USART_SR_RXNE) {
		int c=USART3->DR;
		usart_data0.rx_buf[usart_data0.rx_i%(RX_BSIZE)]=c;
		usart_data0.rx_i++;
		if (usart_data0.rblocker_list.first) {
			sys_wakeup_from_list(&usart_data0.rblocker_list);
		}
	}
	if (st&USART_SR_ORE) {
		USART3->DR=USART3->DR;
	}
}


static int (*usart_putc_fnc)(struct user_data *, int c);

static int usart_putc(struct user_data *u, int c) {
	struct usart_data *ud=u->drv_data;

again:
	if (ud->chip_dead) return -1;
	disable_interrupt();
	if ((ud->tx_in-ud->tx_out)>=TXB_SIZE)  {
		if (!task_sleepable()) {
			enable_interrupt();
			return -1;
		}
		if (!sys_sleepon_update_list(&current->blocker,&ud->wblocker_list)) {
			enable_interrupt();
			return -1;
		}
		enable_interrupt();
		goto again;
	}
	enable_interrupt();
	ud->tx_buf[IX(ud->tx_in)]=c;
	ud->tx_in++;	
	if (!ud->txr) {
		ud->txr=1;
		USART3->CR1|=(USART_CR1_TXEIE|USART_CR1_TCIE|USART_CR1_TE);
	}
	return 1;
}

static int usart_polled_putc(struct user_data *u, int c) {
	struct usart_data *ud=u->drv_data;

	if (ud->txr) {
		while((!ud->chip_dead)&&!(((volatile short int)USART3->SR)&USART_SR_TXE));
	} else {
		ud->txr=1;
		USART3->CR1|=(USART_CR1_TE|USART_CR1_TCIE);
	}
	USART3->DR=c;
	return 1;
}



static int usart_read(struct user_data *u, char *buf, int len) {
	struct usart_data *ud=u->drv_data;
	int i=0;
	while(i<len) {
		int ix=ud->rx_o%(RX_BSIZE);

again:
		disable_interrupt();
		if (!(ud->rx_i-ud->rx_o)) {
			sys_sleepon_update_list(&current->blocker,&ud->rblocker_list);
			enable_interrupt();
			goto again;
		}
		enable_interrupt();
		ud->rx_o++;
		buf[i++]=ud->rx_buf[ix];
	}
	return i;
}

static int usart_write(struct user_data *u, char *buf, int len) {
	int i;
	int rc=0;
	for (i=0;i<len;i++) {
		rc=usart_putc_fnc(u,buf[i]);
		if (rc<0) return rc;
	}
	return (len);
}

static int tx_buf_empty(void) {
	struct usart_data *ud=&usart_data0;
	if (!(ud->tx_in-ud->tx_out))  {
		return 1;
	}
	return 0;
}

static int rx_data_avail(void) {
	struct usart_data *ud=&usart_data0;
	if (ud->rx_i-ud->rx_o) {
		return 1;
	}
	return 0;
}


static struct device_handle *usart_open(void *driver_instance, DRV_CBH cb_handler, void *dum) {
	int i=0;
	for(i=0;i<MAX_USERS;i++) {
		if (udata[i].drv_data==0) break;
	}
	if (i>=MAX_USERS) return 0;
	udata[i].drv_data=driver_instance;
	udata[i].callback=cb_handler;
	udata[i].userdata=dum;
	return &udata[i].dh;
}

static int usart_close(struct device_handle *dh) {
	struct user_data *udata=(struct user_data *)dh;
	udata->drv_data=0;
	return 0;
}

static int usart_control(struct device_handle *dh,
			int cmd,
			void *arg1,
			int arg2) {
	struct user_data *u=(struct user_data *)dh;
	struct usart_data *ud=u->drv_data;

	switch(cmd) {
		case RD_CHAR: {
			int rc=usart_read(u,arg1,arg2);
			return rc;
		}
		case WR_CHAR: {
			int rc=usart_write(u,arg1,arg2);
			return rc;
		}
		case IO_POLL: {
			unsigned int events=(unsigned int)arg1;
			unsigned int revents=0;
			if (EV_WRITE&events) {
				if ((ud->tx_in-ud->tx_out)>=TXB_SIZE)  {
					u->events|=EV_WRITE;
				} else {
					u->events&=~EV_WRITE;
					revents|=EV_WRITE;
				}
			}
			if (EV_READ&events) {
				if (!(ud->rx_i-ud->rx_o)) {
					u->events|=EV_READ;
				} else {
					u->events&=~EV_READ;
					revents|=EV_READ;
				}
			}
			return revents;
		}
		case WR_POLLED_MODE: {
			int enabled=*((unsigned int *)arg1);
			if (enabled) {
				usart_putc_fnc=usart_polled_putc;
			} else {
				usart_putc_fnc=usart_putc;
				USART3->CR1|=(USART_CR1_TXEIE|USART_CR1_TCIE|USART_CR1_TE);
			}
			break;
		}
		default: {
			return -1;
		}
	}
	return 0;
}

static int usart_init(void *instance) {
	struct usart_data *ud=(struct usart_data *)instance;
	usart_putc_fnc=usart_putc;

	RCC->APB1ENR|=RCC_APB1ENR_USART3EN;
	RCC->AHB1ENR|=RCC_AHB1ENR_GPIOCEN;
	/* USART3 uses PC10 for tx, PC11 for rx according to table 5 in  discovery documentation */
//	GPIOC->AFRH = 0;
	GPIOC->AFRH |= 0x00007000;  /* configure pin 11 to AF7 (USART3) */
	GPIOC->AFRH |= 0x00000700;  /* confiure pin 10 to AF7 */
	GPIOC->MODER |= (0x2 << 22);  /* set pin 11 to AF */
	GPIOC->MODER |= (0x2 << 20);  /* set pin 10 to AF */
	GPIOC->OSPEEDR |= (0x3 << 20); /* set pin 10 output high speed */
	USART3->BRR=0x167;   /* 38400 baud at 8Mhz fpcl */
	USART3->CR1=USART_CR1_UE;
	ud->txr=1;
	USART3->CR1|=(USART_CR1_TXEIE|USART_CR1_TCIE|USART_CR1_TE|USART_CR1_RE|USART_CR1_RXNEIE);
	NVIC_SetPriority(USART3_IRQn,0xd);
	NVIC_EnableIRQ(USART3_IRQn);

	ud->wblocker_list.is_ready=tx_buf_empty;
	ud->rblocker_list.is_ready=rx_data_avail;
	return 0;
}

static int usart_start(void *instance) {
	return 0;
}

static struct driver_ops usart_ops = { 
	usart_open,
	usart_close,
	usart_control,
	usart_init,
	usart_start
};

static struct driver usart0 = {
	"usart0",
	&usart_data0,
	&usart_ops,
};

void init_usart_drv(void) {
	driver_publish(&usart0);
}

INIT_FUNC(init_usart_drv);
