#include "sys.h"
#include "io.h"

/*************************  St-term driver ***************************/
#define ST_BSIZE 252
static struct st_term {
	volatile unsigned int  magic;
	volatile unsigned char bsize;
	volatile unsigned char tx_len;
	volatile unsigned char rx_ready;
	volatile unsigned char st_term_xx;
	unsigned char tx_buf[ST_BSIZE];
	unsigned char rx_buf[ST_BSIZE];
} st_term = { 0xdeadf00d, ST_BSIZE, 0x00, 0x00, 0x00, };


#define TXB_SIZE 256
#define TXB_MASK (TXB_SIZE-1)
#define IX(a) (a&TXB_MASK)

#define RX_BSIZE 16

static char tx_buf[TXB_SIZE];
static int tx_in;
static int tx_out;

static char rx_buf[RX_BSIZE];
static int rx_i;
static int rx_o;

static struct sleep_obj rxblocker;
static struct sleep_obj txblocker;

static void stterm_io_r(void *dum) {
        int rerun=0;
        while(1) {
                if (st_term.rx_ready) {
                        int i;
                        for(i=0;i<st_term.rx_ready;i++) {
				rx_buf[rx_i%(RX_BSIZE-1)]=st_term.rx_buf[i];
				rx_i++;
#if 0
                                if (st_term.rx_buf[i]=='\n') {
                                        st_add_c('\n');
                                        st_term.rx_buf[i]=0;
                                        wakeup(&io_term,buf,ix);
                                        ix=0;
                                } else if (st_term.rx_buf[i]==0x7f) {
                                        st_add_c('\b');
                                        st_add_c(' ');
                                        st_add_c('\b');
                                        if (ix) ix--;
                                } else {
                                        st_add_c(st_term.rx_buf[i]);
                                        buf[ix]=st_term.rx_buf[i];
                                        if (ix<254) ix++;
                                }
#endif
                        }
                        st_term.rx_ready=0;
			wakeup(&rxblocker,0,0);
                        rerun=1;
                } else {
                        rerun=0;
                }

                if (rerun) {
                        sleep(20);
                } else {
                        sleep(100);
                }
        }
}

#define MIN(A,B) (A<B?A:B)


static void stterm_io_t(void *dum) {
        int len;
	int wk=0;
        while(1) {
                if ((len=tx_in-tx_out)) {
			wk=1;
                        while(st_term.tx_len) {
                                sleep(50);
                        }
                        if (len>TXB_MASK) {
                                len=TXB_MASK;
                                tx_out=tx_in-TXB_MASK;
                        }

                        len=MIN(len,ST_BSIZE);
                        if (((tx_out%TXB_MASK)+len)>TXB_MASK) {
                                int len1=TXB_SIZE-(tx_out%TXB_MASK);
                                __builtin_memcpy(st_term.tx_buf,&tx_buf[tx_out%TXB_MASK],len1);
                                __builtin_memcpy(&st_term.tx_buf[len1],tx_buf,len-len1);
                        } else {
                                __builtin_memcpy(st_term.tx_buf,&tx_buf[tx_out%TXB_MASK],len);
                        }
                        tx_out+=len;
                        st_term.tx_len=len;
                } else {
			if (wk) {
				wk=0;
				wakeup(&txblocker,0,0);
			}
                        sleep(100);
                }
        }

}

static int stterm_putc(int c) {
	if ((tx_in-tx_out)>=TXB_SIZE) {
		sys_sleepon(&txblocker,0,0);
	}
	tx_buf[tx_in]=c;
	tx_in++;
	return 1;
}

static int stterm_read(char *buf, int len) {
	int i=0;
	while(i<len) {
		if ((rx_i-rx_o)>0) {
			int ix=rx_o%(RX_BSIZE-1);
			int ch;
			rx_o++;
			ch=buf[i++]=rx_buf[ix];
			if (1) stterm_putc(ch);
			if (ch==0x0d) return i-1;
		} else {
			sys_sleepon(&rxblocker,0,0);
		}
	}
	return i;
}

static int stterm_write(char *buf, int len) {
	int i;
	for(i=0;i<len;i++) {
		stterm_putc(buf[i]);
	}
	return len;
}

static int stterm_open(void *instance, DRV_CBH cb_handler) {
        return 0;
}

static int stterm_close(int kfd) {
        return 0;
}

static int stterm_control(int kfd, int cmd, void *arg1, int arg2) {
        switch(cmd) {
                case RD_CHAR:
                        return stterm_read(arg1,arg2);
                case WR_CHAR:
                        return stterm_write(arg1,arg2);
                default:
                        return -1;
        }
        return 0;
}

static int stterm_start(void *inst) {
	thread_create(stterm_io_r,0,256,"stterm_io_r");
	thread_create(stterm_io_t,0,256,"stterm_io_t");
	return 0;
}

static struct driver_ops stterm_ops = { 
        stterm_open,
        stterm_close,
        stterm_control,
	0,
	stterm_start,
};

static struct driver stterm0 = {
	"stterm0",
	0,
	&stterm_ops,
};
 
void init_st_term(void) {
	driver_publish(&stterm0);
}

INIT_FUNC(init_st_term);
