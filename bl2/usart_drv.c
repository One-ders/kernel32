
#include "sys.h"
#include "io.h"
#include "stm32f4xx.h"

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

#define TXB_SIZE 256
#define TXB_MASK (TXB_SIZE-1)
#define IX(a) (a&TXB_MASK)

#define RX_BSIZE 16

struct usart_data {
	char tx_buf[TXB_SIZE];
	int tx_in;
	int tx_out;
	int txr; /*transmitter running*/

	char rx_buf[RX_BSIZE];
	int rx_i;
	int rx_o;

	struct sleep_obj rxblocker;
	struct sleep_obj txblocker; 
};


static struct usart_data usart_data0 = {
.rxblocker = { "rxobj",},
.txblocker = { "txobj",}
};

static struct usart_data *udata[4];

void USART3_IRQHandler(void) {
	unsigned int st=USART3->SR;
	
//	io_printf("got usart irq, status %x\n", USART3->SR);
	if ((st&USART_SR_TXE)&&(usart_data0.tx_in-usart_data0.tx_out)) {
		USART3->DR=usart_data0.tx_buf[IX(usart_data0.tx_out)];
		usart_data0.tx_out++;
		if ((usart_data0.tx_in-usart_data0.tx_out)==0) {
			sys_wakeup(&usart_data0.txblocker,0,0);
		}
	} else if((st&USART_SR_TC)) {
		USART3->CR1&=~(USART_CR1_TE|USART_CR1_TXEIE|USART_CR1_TCIE);
		usart_data0.txr=0;
	}

	if (st&USART_SR_RXNE) {
		int c=USART3->DR;
		usart_data0.rx_buf[usart_data0.rx_i%(RX_BSIZE-1)]=c;
		usart_data0.rx_i++;
		sys_wakeup(&usart_data0.rxblocker,0,0);
	}
}


static int (*usart_putc_fnc)(struct usart_data *, int c);

static int usart_putc(struct usart_data *ud, int c) {

	if ((ud->tx_in-ud->tx_out)>=TXB_SIZE)  {
		sys_sleepon(&ud->txblocker,0,0);
	}
	ud->tx_buf[IX(ud->tx_in)]=c;
	ud->tx_in++;	
	if (!ud->txr) {
		ud->txr=1;
		USART3->CR1|=(USART_CR1_TXEIE|USART_CR1_TCIE|USART_CR1_TE);
	}
	return 1;
}

static int usart_polled_putc(struct usart_data *ud, int c) {

	if (ud->txr) {
		while(!(((volatile short int)USART3->SR)&USART_SR_TXE));
	} else {
		ud->txr=1;
//		USART3->CR1|=(USART_CR1_TXEIE|USART_CR1_TCIE|USART_CR1_TE);
		USART3->CR1|=(USART_CR1_TE|USART_CR1_TCIE);
	}
	USART3->DR=c;
	return 1;
}



static int usart_read(struct usart_data *ud, char *buf, int len) {
	int i=0;
	while(i<len) {
		if  ((ud->rx_i-ud->rx_o)>0) {
			int ix=ud->rx_o%(RX_BSIZE-1);
			int ch;
			ud->rx_o++;
			ch=buf[i++]=ud->rx_buf[ix];
			if (1) {
				usart_putc_fnc(ud,ch);
			}
//			io_printf("usart read got a char %c(%d), store at index %d\n",ch,ch,i-1);
			if (ch==0x0d) {
				return i-1;
			}
		} else {
			sys_sleepon(&ud->rxblocker,0,0);
		}
	}
	return i;
}

static int usart_write(struct usart_data *ud, char *buf, int len) {
	int i;
	for (i=0;i<len;i++) {
		usart_putc_fnc(ud,buf[i]);
		if (buf[i]=='\n') {
			usart_putc_fnc(ud,'\r');
		}
	}
	return len;
}


static int usart_open(void * driver_instance, DRV_CBH cb_handler) {
	int i=0;
	for(i=0;i<(sizeof(udata)/sizeof(udata[0]));i++) {
		if (udata[i]==0) break;
	}
	if (i==sizeof(udata)) return -1;
	udata[i]=driver_instance;
	return i;
}

static int usart_close(int driver_fd) {
	udata[driver_fd]=0;
	return 0;
}

static int usart_control(int driver_fd, int cmd, void *arg1, int arg2) {
	struct usart_data *ud=udata[driver_fd];
	switch(cmd) {
		case RD_CHAR:
			return usart_read(ud,arg1,arg2);
		case WR_CHAR:
			return usart_write(ud,arg1,arg2);
		case WR_POLLED_MODE:
			if (arg1) {
				usart_putc_fnc=usart_polled_putc;
			} else {
				usart_putc_fnc=usart_putc;
			}
			break;
		default:
			return -1;
	}
	return 0;
}

static int usart_init(void *instance) {

	usart_putc_fnc=usart_putc;

	RCC->APB1ENR|=RCC_APB1ENR_USART3EN;
	RCC->AHB1ENR|=RCC_AHB1ENR_GPIOCEN;
	/* USART3 uses PC10 for tx, PC11 for rx according to table 5 in  discovery documentation */
	GPIOC->AFR[1] = 0;
	GPIOC->AFR[1] |= 0x00007000;  /* configure pin 11 to AF7 (USART3) */
	GPIOC->AFR[1] |= 0x00000700;  /* confiure pin 10 to AF7 */
	GPIOC->MODER |= (0x2 << 22);  /* set pin 11 to AF */
	GPIOC->MODER |= (0x2 << 20) ;  /* set pin 10 to AF */
	GPIOC->OSPEEDR |= (0x3 << 20); /* set pin 10 output high speed */
	USART3->BRR=0x167;   /* 38400 baud at 8Mhz fpcl */
	USART3->CR1=USART_CR1_UE;
	USART3->CR1|=(USART_CR1_TXEIE|USART_CR1_TCIE|USART_CR1_TE|USART_CR1_RE|USART_CR1_RXNEIE);
	NVIC_EnableIRQ(USART3_IRQn);

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
