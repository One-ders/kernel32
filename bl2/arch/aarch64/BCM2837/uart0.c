
#include "io.h"
#include "sys.h"
#include "devices.h"
#include "uart.h"
#include "irq.h"
#include "gpio.h"
#if 0
#include "frame.h"
#endif

#define TXB_SIZE 1024
#define TXB_MASK (TXB_SIZE-1)
#define IX(a) (a&TXB_MASK)

#define RX_BSIZE 16
#define UART0_TX 14
#define UART0_RX 15

struct uart_data {
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
	struct UART_DEV *regs;
};

struct user_data {
        struct device_handle dh;
        struct uart_data *drv_data;
        DRV_CBH callback;
        void    *userdata;
        int     events;
};

static struct uart_data uart_data0;

#define MAX_USERS 4
//static struct user_data udata[MAX_USERS];
struct user_data udata[MAX_USERS];

void serial_setbrg (void);
int serial_tstc (void);

unsigned long int in_print;
int serial_init (void) {
	struct UART_DEV *uart_dev=UART_DEV_BASE;

	in_print=0;

	gpio_set_function(UART0_RX,GPIO_PIN_ALT0);
	gpio_set_function(UART0_TX,GPIO_PIN_ALT0);

	gpio_set_pull(UART0_RX,GPPUD_PULL_DISABLE);
	gpio_set_pull(UART0_TX,GPPUD_PULL_DISABLE);

	uart_dev->lcrh=LCRH_WLEN(3);
	uart_dev->ibrd=1;  // (3000000(clk)/(115200(baud)*16))
	uart_dev->fbrd=41;   // fraction from above *64 + 0.5
	uart_dev->cr=CR_RXE|CR_TXE|CR_UARTEN;
	return 0;
}

void serial_setbrg (void) {
	struct UART_DEV *uart_dev=UART_DEV_BASE;

//	uart_dev->ibrd=135;
//	uart_dev->fbrd=41;
	uart_dev->ibrd=1;  // (3000000(clk)/(115200(baud)*16))
	uart_dev->fbrd=41;   // fraction from above *64 + 0.5
}

void serial_putc (const char c) {
	struct UART_DEV *uart_dev=UART_DEV_BASE;

	if (c == '\n') serial_putc ('\r');

	/* Wait for fifo to shift out some bytes */
	while(1) {
		if (!(uart_dev->fr&FR_TXFF)) {
			break;
		}
	}
	uart_dev->dr=c;
}

void serial_puts (const char *s) {
	while (*s) {
		serial_putc (*s++);
	}
}

static char ttab[]={'0','1','2','3','4','5','6','7',
			'8','9','a','b','c','d','e','f'};
void serial_put_hexint(unsigned int val) {
	char val_str[10];
	int i;

	for(i=0;i<8;i++) {
		unsigned char nib=(val>>(i*4))&0xf;
		val_str[7-i]=ttab[nib];
	}
	val_str[8]=0;
	serial_puts(val_str);
}

int serial_getc (void) {
	struct UART_DEV *uart_dev=UART_DEV_BASE;

	while (!serial_tstc());

	return uart_dev->dr;
}

int serial_tstc (void) {
	struct UART_DEV *uart_dev=UART_DEV_BASE;

	if (!(uart_dev->fr & FR_RXFE)) {
		/* Data in rfifo */
		return (1);
	}
	return 0;
}

static int (*uart_putc_fnc)(struct user_data *, int c);
static int uart_putc(struct user_data *u, int c);

int uart_irq_handler(int irq_num, void *dum) {
	struct uart_data *ud=(struct uart_data *)dum;
	unsigned int uirq=ud->regs->mis;

#if 0
	if (uirq!=1) {
		struct fullframe *ff=(struct fullframe *)((unsigned int *)current->sp-37);
		sys_printf("uart_irq_handler, irq reg %x\n", uirq);
		sys_printf("current %x is irqed stat %x\n", current, ff->cp0_status);
		sys_printf("cause %x, epc %x\n", ff->cp0_cause, ff->cp0_epc);
		sys_printf("ra %x, sp %x\n", ff->ra, ff->sp);
	}
#endif
//ud->regs->ulsr & (UART_LSR_TDRQ | UART_LSR_TEMP
	ud->regs->icr=ICR_TXIC;
	while (ud->txr && !(ud->regs->fr&FR_TXFF)) {
		if (ud->tx_in-ud->tx_out) {
                	if (uart_putc_fnc==uart_putc) {
				ud->regs->dr=ud->tx_buf[IX(ud->tx_out)];
				ud->tx_out++;
				if ((ud->tx_in-ud->tx_out)==0) {
					if (ud->wblocker_list.first) {
						sys_wakeup_from_list(&ud->wblocker_list);
					}
				}
			} else {
				break;
			}
		} else {
			break;
		}
	}

	if (ud->txr && (ud->regs->fr&FR_TXFE)&&
		!(ud->tx_in-ud->tx_out)) {
		ud->regs->imsc&=~IMSC_TXIM;
		ud->txr=0;
        }

	if (uirq&RIS_RXRIS) {
		ud->regs->icr=ICR_RXIC;
		while (!ud->regs->fr&FR_RXFE) {
			unsigned char c=ud->regs->dr;
			ud->rx_buf[ud->rx_i%(RX_BSIZE)]=c;
			ud->rx_i++;
		}
                if (ud->rblocker_list.first) {
                        sys_wakeup_from_list(&ud->rblocker_list);
                }
        }
	return 0;
}

static int uart_putc(struct user_data *u, int c) {
	struct uart_data *ud=u->drv_data;
	unsigned long int cpu_flags;

again:
	if (ud->chip_dead) return -1;
	cpu_flags=disable_interrupts();
	if ((ud->tx_in-ud->tx_out)>=TXB_SIZE)  {
		if (!task_sleepable()) {
			restore_cpu_flags(cpu_flags);
			return -1;
		}
		if (!sys_sleepon_update_list(&current->blocker,&ud->wblocker_list)) {
			restore_cpu_flags(cpu_flags);
			return -1;
		}
		restore_cpu_flags(cpu_flags);
		goto again;
	}
	ud->tx_buf[IX(ud->tx_in)]=c;
	ud->tx_in++;
	if (!ud->txr) {
		ud->txr=1;
		ud->regs->imsc|=IMSC_TXIM;
		ud->regs->dr=ud->tx_buf[IX(ud->tx_out)];
		ud->tx_out++;
	}
	restore_cpu_flags(cpu_flags);
	return 1;
}


static int uart_polled_putc(struct user_data *u, int c) {
	struct uart_data *ud=u->drv_data;

	/* Wait for fifo to shift out some bytes */
	while((!ud->chip_dead)&&
		(ud->regs->fr&FR_TXFF));
	ud->regs->dr=c;

        return 1;
}


static int uart_read(struct user_data *u, char *buf, int len) {
	struct uart_data *ud=u->drv_data;
	unsigned long int cpu_flags;
	int i=0;

	while(i<len) {
		int ix=ud->rx_o%(RX_BSIZE);
again:
		cpu_flags=disable_interrupts();
		if (!(ud->rx_i-ud->rx_o)) {
			sys_sleepon_update_list(&current->blocker,&ud->rblocker_list);
			restore_cpu_flags(cpu_flags);
			goto again;
		}
		restore_cpu_flags(cpu_flags);
		ud->rx_o++;
		buf[i++]=ud->rx_buf[ix];
	}
	return i;
}

static int uart_write(struct user_data *u,
			char *buf, int len) {
	int i;
	int rc=0;
	for(i=0;i<len;i++) {
		rc=uart_putc_fnc(u,buf[i]);
		if (rc<0) return rc;
		if (buf[i]=='\n') uart_putc_fnc(u,'\r');
	}
	return len;
}

static int tx_buf_empty(void) {
	struct uart_data *ud=&uart_data0;
	if (!(ud->tx_in-ud->tx_out))  {
		return 1;
	}
	return 0;
}

static int rx_data_avail(void) {
	struct uart_data *ud=&uart_data0;
	if (ud->rx_i-ud->rx_o) {
		return 1;
	}
	return 0;
}

static struct device_handle *uart_open(void *driver_instance, DRV_CBH cb_handler, void *dum) {
	int i=0;

	for(i=0;i<MAX_USERS;i++) {
		if (udata[i].drv_data==0) {
			break;
		}
	}
	if (i>=MAX_USERS) return 0;
	udata[i].drv_data=driver_instance;
	udata[i].callback=cb_handler;
	udata[i].userdata=dum;
	return &udata[i].dh;
}

static int uart_close(struct device_handle *dh) {
	struct user_data *udata=(struct user_data *)dh;
	udata->drv_data=0;
	return 0;
}

static int uart_control(struct device_handle *dh,
			int cmd,
			void *arg1,
			int arg2) {
	struct user_data	*u=(struct user_data *)dh;
	struct uart_data	*ud;

	if (!u) return -1;

	ud=u->drv_data;

	switch(cmd) {
		case RD_CHAR: {
			int rc=uart_read(u,arg1,arg2);
//sys_printf("read got %x\n",((unsigned char *)arg1)[0]);
			return rc;
		}
		case WR_CHAR: {
			int rc=uart_write(u,arg1,arg2);
			return rc;
		}
		case IO_POLL: {
			unsigned int events=(unsigned int)(((unsigned long int)arg1)&0xffffffff);
			unsigned int revents=0;
			if (EV_WRITE&events) {
				if ((ud->tx_in-ud->tx_out)>=TXB_SIZE) {
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
//				ud->regs->mu_ier_reg&=~AUX_MU_IER_REG_TX_IRQ_ENABLE;
				uart_putc_fnc=uart_polled_putc;
			} else {
				uart_putc_fnc=uart_putc;
				if (!tx_buf_empty()) {
//					ud->regs->mu_ier_reg|=AUX_MU_IER_REG_TX_IRQ_ENABLE;
				}
			}
			break;
		}
		default: {
			return -1;
		}
	}
	return 0;
}

static int uart_init(void *instance) {
	struct uart_data *ud=(struct uart_data *)instance;

	uart_putc_fnc=uart_putc;
//	uart_putc_fnc=uart_polled_putc;

	ud->wblocker_list.is_ready=tx_buf_empty;
	ud->rblocker_list.is_ready=rx_data_avail;

	gpio_set_function(UART0_RX,GPIO_PIN_ALT0);
	gpio_set_function(UART0_TX,GPIO_PIN_ALT0);

	gpio_set_pull(UART0_RX,GPPUD_PULL_DISABLE);
	gpio_set_pull(UART0_TX,GPPUD_PULL_DISABLE);

	ud->txr=0;
	ud->regs->cr=0;
//	ud->regs->ibrd=135;  // (250000000(clk)/(115200(baud)*16))
//	ud->regs->fbrd=41;   // fraction from above *64 + 0.5
	ud->regs->ibrd=1;  // (3000000(clk)/(115200(baud)*16))
	ud->regs->fbrd=41;   // fraction from above *64 + 0.5

//	ud->regs->lcrh=LCRH_WLEN(3)|LCRH_FEN;
	ud->regs->lcrh=LCRH_WLEN(3);
	ud->regs->imsc=IMSC_RXIM|IMSC_TXIM;
	ud->regs->cr=CR_RXE|CR_TXE|CR_UARTEN;
	install_irq_handler(UART,uart_irq_handler,ud);
	ud->regs->icr=ICR_OEIC|ICR_BEIC|ICR_PEIC|ICR_FEIC|ICR_RTIC|ICR_TXIC|
			ICR_RXIC|ICR_CTSMIC;

#if 0
	ud->regs->ufcr=0;

	ud->regs->ufcr=UART_FCR_FME|UART_FCR_RFRT|UART_FCR_TFRT;
	install_irq_handler(UART0_IRQ, uart_irq_handler, ud);
	ud->regs->ufcr=UART_FCR_FME|UART_FCR_RFRT|UART_FCR_TFRT|UART_FCR_UME;

#endif
	return 0;
}

static int uart_start(void *instance) {
	return 0;
}

static struct driver_ops uart_ops = {
	uart_open,
	uart_close,
	uart_control,
	uart_init,
	uart_start
};

static struct driver uart0 = {
	"uart0",
	&uart_data0,
	&uart_ops,
};

void init_uart_drv(void) {
	uart_data0.regs=(struct UART_DEV *)UART_DEV_BASE;
	driver_publish(&uart0);
}

INIT_FUNC(init_uart_drv);
