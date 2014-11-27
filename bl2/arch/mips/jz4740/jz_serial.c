
#include <config.h>
#include <jz4740.h>

#ifndef IPL

#include "sys.h"
#include "io.h"

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
        DRV_CBH callback;
        void    *userdata;
        int     events;
};

static struct usart_data usart_data0;

#define MAX_USERS 4
static struct user_data udata[MAX_USERS];

#endif

#define UART_BASE	UART0_BASE

void serial_setbrg (void);
int serial_tstc (void);

int serial_init (void) {
	volatile unsigned char *uart_fcr = (volatile unsigned char *)(UART_BASE + OFF_FCR);
	volatile unsigned char *uart_lcr = (volatile unsigned char *)(UART_BASE + OFF_LCR);
	volatile unsigned char *uart_ier = (volatile unsigned char *)(UART_BASE + OFF_IER);
	volatile unsigned char *uart_sircr = (volatile unsigned char *)(UART_BASE + OFF_SIRCR);

	/* Disable port interrupts while changing hardware */
	*uart_ier = 0;

	/* Disable UART unit function */
	*uart_fcr = ~UART_FCR_UUE;

	/* Set both receiver and transmitter in UART mode (not SIR) */
	*uart_sircr = ~(SIRCR_RSIRE | SIRCR_TSIRE);

	/* Set databits, stopbits and parity. (8-bit data, 1 stopbit, no parity) */
	*uart_lcr = UART_LCR_WLEN_8 | UART_LCR_STOP_1;

	/* Set baud rate */
	serial_setbrg();

	/* Enable UART unit, enable and clear FIFO */
	*uart_fcr = UART_FCR_UUE | UART_FCR_FE | UART_FCR_TFLS | UART_FCR_RFLS;
	return 0;
}

void serial_setbrg (void) {
	volatile unsigned char *uart_lcr = (volatile unsigned char *)(UART_BASE + OFF_LCR);
	volatile unsigned char *uart_dlhr = (volatile unsigned char *)(UART_BASE + OFF_DLHR);
	volatile unsigned char *uart_dllr = (volatile unsigned char *)(UART_BASE + OFF_DLLR);
	unsigned int baud_div, tmp;

	baud_div = CFG_EXTAL / 16 / CONFIG_BAUDRATE;
	tmp = *uart_lcr;
	tmp |= UART_LCR_DLAB;
	*uart_lcr = tmp;

	*uart_dlhr = (baud_div >> 8) & 0xff;
	*uart_dllr = baud_div & 0xff;

	tmp &= ~UART_LCR_DLAB;
	*uart_lcr = tmp;
}

void serial_putc (const char c) {
	volatile unsigned char *uart_lsr = (volatile unsigned char *)(UART_BASE + OFF_LSR);
	volatile unsigned char *uart_tdr = (volatile unsigned char *)(UART_BASE + OFF_TDR);

	if (c == '\n') serial_putc ('\r');

	/* Wait for fifo to shift out some bytes */
	while ( !((*uart_lsr & (UART_LSR_TDRQ | UART_LSR_TEMT)) == 0x60) );

	*uart_tdr = (unsigned char)c;
}

void serial_puts (const char *s) {
	while (*s) {
		serial_putc (*s++);
	}
}

int serial_getc (void) {
	volatile unsigned char *uart_rdr = (volatile unsigned char *)(UART_BASE + OFF_RDR);

	while (!serial_tstc());

	return *uart_rdr;
}

int serial_tstc (void) {
	volatile unsigned char *uart_lsr = (volatile unsigned char *)(UART_BASE + OFF_LSR);

	if (*uart_lsr & UART_LSR_DR) {
		/* Data in rfifo */
		return (1);
	}
	return 0;
}

#ifndef IPL

static int usart_write(struct user_data *u,
			char *buf, int len) {
	int i;
	int rc=0;
	for(i=0;i<len;i++) {
		serial_putc(buf[i]);
	}
	return len;
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
		case WR_CHAR: {
			int rc=usart_write(u,arg1,arg2);
			return rc;
		}
		default: {
			return -1;
		}
	}
}

static int usart_init(void *instance) {
	struct usart_data *ud=(struct usart_data *)instance;
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
#endif
