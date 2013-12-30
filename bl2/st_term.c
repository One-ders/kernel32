
#include "sys.h"
#include "st_term.h"

/*************************  St-term driver ***************************/
#define ST_BSIZE 252
struct st_term {
	volatile unsigned int  magic;
	volatile unsigned char bsize;
	volatile unsigned char tx_len;
	volatile unsigned char rx_ready;
	volatile unsigned char st_term_xx;
	unsigned char tx_buf[ST_BSIZE];
	unsigned char rx_buf[ST_BSIZE];
} st_term = { 0xdeadf00d, ST_BSIZE, 0x00, 0x00, 0x00, };


void st_io_r(void *dum) {
        char *buf=getSlab_256();
        int ix=0;
        int rerun=0;
        while(1) {
                if (st_term.rx_ready) {
                        int i;
                        for(i=0;i<st_term.rx_ready;i++) {
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
                        }
                        st_term.rx_ready=0;
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

#define TX_BUF_SIZE 1024
#define TXI_MASK (TX_BUF_SIZE-1)

char tx_buf[TX_BUF_SIZE];
unsigned int tx_out;
unsigned int tx_in;


void st_io_t(void *dum) {
        int len;
        while(1) {
                if ((len=tx_in-tx_out)) {
                        while(st_term.tx_len) {
                                sleep(50);
                        }
                        if (len>1023) {
                                len=1023;
                                tx_out=tx_in-1023;
                        }

                        len=MIN(len,ST_BSIZE);
                        if (((tx_out%TXI_MASK)+len)>1023) {
                                int len1=1024-(tx_out%TXI_MASK);
                                __builtin_memcpy(st_term.tx_buf,&tx_buf[tx_out%TXI_MASK],len1);
                                __builtin_memcpy(&st_term.tx_buf[len1],tx_buf,len-len1);
                        } else {
                                __builtin_memcpy(st_term.tx_buf,&tx_buf[tx_out%TXI_MASK],len);
                        }
                        tx_out+=len;
                        st_term.tx_len=len;
                } else {
                        sleep(100);
                }
        }

}

static int st_term_open(struct driver *drv) {
        return 0;
}

static int st_term_close(int driver_instance) {
        return 0;
}

static int st_term_control(int driver_instance, int cmd, void *arg1, int arg2) {
        switch(cmd) {
                case RD_CHAR:
                        return st_term_read(arg1,arg2);
                case WR_CHAR:
                        return st_term_write(arg1,arg2);
                default:
                        return -1;
        }
        return 0;
}

struct driver st_term_drv = { 
        ST_TERM_DRV_NAME, 
        st_term_open,
        st_term_close,
        st_term_control,
};
 
void init_st_term(void) {
	driver_publish(&st_term_drv);
}
