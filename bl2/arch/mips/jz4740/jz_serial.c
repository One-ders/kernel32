
#ifndef IPL

#include "sys.h"
#include "io.h"
#include "irq.h"
#include "frame.h"

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
	struct UART_regs *regs;
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
//static struct user_data udata[MAX_USERS];
struct user_data udata[MAX_USERS];

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
	*uart_fcr = ~UART_FCR_UME;

	/* Set both receiver and transmitter in UART mode (not SIR) */
	*uart_sircr = ~(SIRCR_RSIRE | SIRCR_TSIRE);

	/* Set databits, stopbits and parity. 
                (8-bit data, 1 stopbit, no parity) */
	*uart_lcr = UART_LCR_WLEN_8 | UART_LCR_STOP_1;

	/* Set baud rate */
	serial_setbrg();

	/* Enable UART unit, enable and clear FIFO */
	*uart_fcr = UART_FCR_UME | UART_FCR_FME | UART_FCR_TFRT | UART_FCR_RFRT;
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
	while ( !((*uart_lsr & (UART_LSR_TDRQ | UART_LSR_TEMP)) == 0x60) );

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

	if (*uart_lsr & UART_LSR_DRY) {
		/* Data in rfifo */
		return (1);
	}
	return 0;
}

#ifndef IPL

static int (*usart_putc_fnc)(struct user_data *, int c);
static int usart_putc(struct user_data *u, int c);

int uart_irq_handler(int irq_num, void *dum) {
	struct usart_data *ud=(struct usart_data *)dum;
	unsigned int uirq=ud->regs->uiir&0xf;

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
	while (ud->regs->ulsr&UART_LSR_TDRQ) {
		if (ud->tx_in-ud->tx_out) {
                	if (usart_putc_fnc==usart_putc) {
				ud->regs->uthr=ud->tx_buf[IX(ud->tx_out)];
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

	if ((ud->regs->ulsr&UART_LSR_TEMP)&& !(ud->tx_in-ud->tx_out)) {
//               if (usart_putc_fnc==usart_putc) {
			ud->regs->uier&=~UART_IER_TDRIE;
                        ud->txr=0;
//                } else {
//			ud->regs->uier&=~UART_IER_TDRIE;
//               }
        }
	
	if (uirq&4) {
		while (ud->regs->ulsr&UART_LSR_DRY) {
			unsigned char c=ud->regs->urbr;
			ud->rx_buf[ud->rx_i%(RX_BSIZE)]=c;
			ud->rx_i++;
		}
                if (ud->rblocker_list.first) {
                        sys_wakeup_from_list(&ud->rblocker_list);
                }
        }

	return 0;
}

static int usart_putc(struct user_data *u, int c) {
	struct usart_data *ud=u->drv_data;
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
	restore_cpu_flags(cpu_flags);
	ud->tx_buf[IX(ud->tx_in)]=c;
	ud->tx_in++;
	if (!ud->txr) {
		ud->txr=1;
		ud->regs->uier|=UART_IER_TDRIE;
	}
	return 1;
}


static int usart_polled_putc(struct user_data *u, int c) {
	struct usart_data *ud=u->drv_data;

//	if (ud->txr) {
		while((!ud->chip_dead)&&
			!((ud->regs->ulsr & (UART_LSR_TDRQ | UART_LSR_TEMP)) == 0x60) );
//	} else {
//		ud->txr=1;
//		ud->regs->uier|=UART_IER_TDRIE;
//        }
	ud->regs->uthr = (unsigned char)c;
        return 1;
}


static int usart_read(struct user_data *u, char *buf, int len) {
	struct usart_data *ud=u->drv_data;
	unsigned long int cpu_flags;
	int i=0;
	volatile unsigned char *uart_tdr = (volatile unsigned char *)(UART_BASE + OFF_TDR);

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

static int usart_write(struct user_data *u,
			char *buf, int len) {
	int i;
	int rc=0;
	for(i=0;i<len;i++) {
		rc=usart_putc_fnc(u,buf[i]);
		if (rc<0) return rc;
	}
	return len;
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

static int usart_close(struct device_handle *dh) {
	struct user_data *udata=(struct user_data *)dh;
	udata->drv_data=0;
	return 0;
}

static int usart_control(struct device_handle *dh,
			int cmd,
			void *arg1,
			int arg2) {
	struct user_data	*u=(struct user_data *)dh;
	struct usart_data	*ud;

	if (!u) return -1;

	ud=u->drv_data;

	switch(cmd) {
		case RD_CHAR: {
			int rc=usart_read(u,arg1,arg2);
//sys_printf("read got %x\n",((unsigned char *)arg1)[0]);
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
				ud->regs->uier&=~UART_IER_TDRIE;
				usart_putc_fnc=usart_polled_putc;
			} else {
				usart_putc_fnc=usart_putc;
				if (!tx_buf_empty()) {
					ud->regs->uier|=UART_IER_TDRIE;
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

static int usart_init(void *instance) {
	struct usart_data *ud=(struct usart_data *)instance;
	unsigned short bauddivisor=2; /* -> 115200 (3686400/'2'*16) */

	usart_putc_fnc=usart_putc;
//	usart_putc_fnc=usart_polled_putc;

	ud->wblocker_list.is_ready=tx_buf_empty;
	ud->rblocker_list.is_ready=rx_data_avail;

	ud->txr=1;
	ud->regs->ufcr=0;
	ud->regs->ulcr=0x80;
	ud->regs->udllr=bauddivisor&0xff;
	ud->regs->udlhr=bauddivisor>>8;
	
	ud->regs->umcr=0;
	ud->regs->ulcr=0x03;
	ud->regs->uier|=(UART_IER_TDRIE | UART_IER_RDRIE);
	ud->regs->ufcr=UART_FCR_FME|UART_FCR_RFRT|UART_FCR_TFRT;
//	ud->regs->ufcr=UART_FCR_RFRT|UART_FCR_TFRT|UART_FCR_UME;
	install_irq_handler(UART0_IRQ, uart_irq_handler, ud);
	ud->regs->ufcr=UART_FCR_FME|UART_FCR_RFRT|UART_FCR_TFRT|UART_FCR_UME;
	

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
	usart_data0.regs=(struct UART_regs *)UART0_BASE;
	driver_publish(&usart0);
}

INIT_FUNC(init_usart_drv);
#endif
