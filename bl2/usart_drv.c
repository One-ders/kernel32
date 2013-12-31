
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

static char tx_buf[TXB_SIZE];
static int tx_in;
static int tx_out;
static int txr; /*transmitter running*/

#define RX_BSIZE 16
static char rx_buf[RX_BSIZE];
static int rx_i;
static int rx_o;

struct sleep_obj usart_rxobj = { "rxobj",};
struct sleep_obj usart_txobj = { "txobj",};

void USART3_IRQHandler(void) {
	unsigned int st=USART3->SR;
//	io_printf("got usart irq, status %x\n", USART3->SR);
	if ((st&USART_SR_TXE)&&(tx_in-tx_out)) {
		USART3->DR=tx_buf[IX(tx_out)];
		tx_out++;
		if ((tx_in-tx_out)==0) {
			sys_wakeup(&usart_txobj,0,0);
		}
	} else if(st&USART_SR_TC) {
		USART3->CR1&=~(USART_CR1_TE|USART_CR1_TXEIE|USART_CR1_TCIE);
		txr=0;
	}

	if (st&USART_SR_RXNE) {
		int c=USART3->DR;
		rx_buf[rx_i%(RX_BSIZE-1)]=c;
		rx_i++;
		sys_wakeup(&usart_rxobj,0,0);
	}
}


int usart_putc(int c) {

	if ((tx_in-tx_out)>=TXB_SIZE)  {
		sys_sleepon(&usart_txobj,0,0);
	}
	tx_buf[IX(tx_in)]=c;
	tx_in++;	
	if (!txr) {
		txr=1;
		USART3->CR1|=(USART_CR1_TXEIE|USART_CR1_TCIE|USART_CR1_TE);
	}
	return 1;
}


#if 0
int usart_getc(void) {
	sleep_on(&usart_rxobj,rx_buf,1);
	io_printf("got char from serial: %c\n", rx_buf);
	return rx_buf[0];
}
#endif

int usart_read(char *buf,int len) {
	int i=0;
	while(i<len) {
		if  ((rx_i-rx_o)>0) {
			int ix=rx_o%(RX_BSIZE-1);
			int ch;
			rx_o++;
			ch=buf[i++]=rx_buf[ix];
			if (1) {
				usart_putc(ch);
			}
//			io_printf("usart read got a char %c(%d), store at index %d\n",ch,ch,i-1);
			if (ch==0x0d) {
				return i-1;
			}
		} else {
			sys_sleepon(&usart_rxobj,0,0);
		}
	}
	return i;
}

int usart_write(char *buf, int len) {
	int i;
	for (i=0;i<len;i++) {
		usart_putc(buf[i]);
		if (buf[i]=='\n') {
			usart_putc('\r');
		}
	}
	return len;
}


static int usart_open(struct driver *drv, DRV_CBH cb_handler) {
	return 0;
}

static int usart_close(int driver_instance) {
	return 0;
}

static int usart_control(int driver_instance, int cmd, void *arg1, int arg2) {
	switch(cmd) {
		case RD_CHAR:
			return usart_read(arg1,arg2);
		case WR_CHAR:
			return usart_write(arg1,arg2);
		default:
			return -1;
	}
	return 0;
}

static int usart_init(void) {
	return 0;
}

static int usart_start(void) {
	return 0;
}

struct driver usart_drv = { 
	USART_DRV, 
	usart_open,
	usart_close,
	usart_control,
	usart_init,
	usart_start
};
			

void init_usart(void) {
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

	driver_publish(&usart_drv);
#if 0
	thread_create(usart_rx,0,256,"usart_rx");

	usart_putc('h');
	usart_putc('e');
	usart_putc('l');
	usart_putc('l');
	usart_putc('o');
	usart_putc('o');
	usart_putc('o');
#endif
}
