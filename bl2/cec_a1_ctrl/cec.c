
#include "sys.h"
#include "io.h"
#include "cec_drv.h"
#include "cec.h"

#include "asynchio.h"


static int fd_cec;
static unsigned char cec_rbuf[24];
extern int cec_debug;
static int active_receiver;

static int handle_cec_data(int fd,int ev, void *dum);

int cec_init_cec(void) {
	fd_cec=io_open(CEC_DRV);
	io_control(fd_cec,F_SETFL,(void *)O_NONBLOCK,0);
	register_event(fd_cec,EV_READ,handle_cec_data,0);
	return fd_cec;
}

extern int pe_data_in(unsigned char *,int);

static int handle_cec_data(int fd,int ev, void *dum) {
	int rc;
	
	rc=io_read(fd,(char *)cec_rbuf,sizeof(cec_rbuf));
	if (rc<0) {
		return rc;
	}

	if (rc==9) {
                unsigned char sonyOn[]={0x0f,0xa0,0x08,0x00,0x46,0x00,0x04};
                if (__builtin_memcmp(cec_rbuf,sonyOn,sizeof(sonyOn))==0) {
                        /* wakeup  set top box */
			wakeup_usb_dev();
                }
        }

	
	if (active_receiver) {
		pe_data_in(cec_rbuf,rc);
	}
	return 0;
}

int cec_set_rec(int enable) {
	if (cec_debug) {
		printf("cec_set_rec: %d\n", enable);
	}

	io_control(fd_cec,F_SETFL,(void *)O_NONBLOCK,0);
//	io_control(fd_cec,CEC_SET_PROMISC,&enable,sizeof(enable));
	active_receiver=enable;
	return 0;
}

int cec_send(unsigned char *buf, int len) {
	int rc;
	io_control(fd_cec,F_SETFL,(void *)0,0);
	rc=io_write(fd_cec,(char *)buf,len);
	io_control(fd_cec,F_SETFL,(void *)O_NONBLOCK,0);
	return rc;
}

int cec_set_ackmask(int ackmask) {
	return io_control(fd_cec,CEC_SET_ACK_MSK,&ackmask,sizeof(ackmask));
}
