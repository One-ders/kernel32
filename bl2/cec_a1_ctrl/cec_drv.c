
#include <sys.h>
#include <io.h>
#include <led_drv.h>

#include "cec_drv.h"
#include "hr_timer.h"
#include "gpio_drv.h"


#if 0
static int cecs_timer_fd;
#endif
static int cecr_timer_fd;
static struct driver *timerdrv;

static int led_fd;
static struct driver *leddrv;

static int pin_fd;
static struct driver *pindrv;

unsigned int blue=LED_BLUE;

#define CEC_IDLE	0
#define CEC_R_INIT	1
#define CEC_REC		2
#define CEC_REC_A	3
#define CEC_REC_P	4
#define CEC_TX		5
static unsigned char cec_state;

/* Substate for State CEC_R_INIT */
#define CEC_RSYNC_IDLE	0
#define CEC_RSYNC_LOW	1
#define CEC_RSYNC_TOH	2
#define CEC_RSYNC_HI1	3
#define CEC_RSYNC_HI2	4
#define CEC_RSYNC_TOL	5

/* Substate for State CEC_REC, CEC_REC_A and CEC_REC_P */
#define CEC_REC_IDLE	0
#define CEC_REC_PLB_I	1
#define CEC_REC_PLB_E	2
#define CEC_REC_PLB_N	3
#define CEC_REC_EOM_I	4
#define CEC_REC_EOM_E	5
#define CEC_REC_EOM_N	6
#define CEC_REC_ACK_I	7
#define CEC_REC_ACK_E	8
static unsigned char cec_sub_state;

/* CEC_FLAGS bits */
#define CECR_EOM	0x01
#define CECR_BYTE_READY	0x02
#define CECR_TO_ME	0x04
#define CECR_MC		0x08
#define CECR_ACK	0x10
#define CECR_START	0x20
static unsigned int cec_flags;

static unsigned int cec_rx;
static unsigned int cec_rx_tmp;
static unsigned int cec_rbi;
static unsigned int cec_ack_mask;

/* A Cec device claims the bus by sinking the wire for 3400 mS */
static int start_r_sync(int pstate) {
	int uSec=3500;
	if (pstate) {
		io_printf("false start_bit signal\n");
		return 0;
	}
	cec_flags=0;
	cec_state=CEC_R_INIT;
	cec_sub_state=CEC_RSYNC_LOW;
	timerdrv->ops->control(cecr_timer_fd, HR_TIMER_SET, &uSec, sizeof(uSec));
	leddrv->ops->control(led_fd,LED_CTRL_ACTIVATE,&blue,sizeof(blue));
	return 0;
}

static int handle_r_init(int pstate) {
	switch(cec_sub_state) {
		case	CEC_RSYNC_IDLE:
		case	CEC_RSYNC_LOW:
		case	CEC_RSYNC_HI1:
		case	CEC_RSYNC_HI2: {
			/* Start Bit Failed */
			io_printf("Start Bit Failed in substate %d pin irq\n", cec_sub_state);
			cec_state=CEC_IDLE;
			cec_sub_state=CEC_RSYNC_IDLE;
			timerdrv->ops->control(cecr_timer_fd, HR_TIMER_CANCEL, 0, 0);
			leddrv->ops->control(led_fd,LED_CTRL_DEACTIVATE,&blue,sizeof(blue));
			break;
		}
		case	CEC_RSYNC_TOH:
			if (pstate) {
				cec_sub_state=CEC_RSYNC_HI1;
			} else {
				io_printf("Start Bit Failed in substate RSYNC_TOH\n");
				cec_state=CEC_IDLE;
				cec_sub_state=CEC_RSYNC_IDLE;
				timerdrv->ops->control(cecr_timer_fd, HR_TIMER_CANCEL, 0, 0);
				leddrv->ops->control(led_fd,LED_CTRL_DEACTIVATE,&blue,sizeof(blue));
			}
			break;
		case	CEC_RSYNC_TOL:
			if (!pstate)  {  /* verify that line is low */
				int uSec=1050;
				timerdrv->ops->control(cecr_timer_fd, HR_TIMER_CANCEL, 0, 0);
				timerdrv->ops->control(cecr_timer_fd, HR_TIMER_SET, &uSec, sizeof(uSec));
				cec_state=CEC_REC;
				cec_sub_state=CEC_REC_PLB_I;
				cec_rbi=8;
				cec_rx_tmp=0;
				/*  Start bit done!!!!! */
			} else {
				/* Start Bit Failed */
				io_printf("Start Bit Failed in substate RSYNC_TOL\n");
				cec_state=CEC_IDLE;
				cec_sub_state=CEC_RSYNC_IDLE;
				timerdrv->ops->control(cecr_timer_fd, HR_TIMER_CANCEL, 0, 0);
				leddrv->ops->control(led_fd,LED_CTRL_DEACTIVATE,&blue,sizeof(blue));
			}
			break;
	}
	return 0;
}

static void handle_r_sync_tout() {
	switch(cec_sub_state) {
		case CEC_RSYNC_IDLE:
		case CEC_RSYNC_TOH:
		case CEC_RSYNC_TOL:
			/* Start bit failed */
			io_printf("Start Bit Failed in substate %d tout\n", cec_sub_state);
			cec_state=CEC_IDLE;
			cec_sub_state=CEC_RSYNC_IDLE;
			leddrv->ops->control(led_fd,LED_CTRL_DEACTIVATE,&blue,sizeof(blue));
			break;
		case CEC_RSYNC_LOW: {
			int uSec=400;
			cec_sub_state=CEC_RSYNC_TOH;
			timerdrv->ops->control(cecr_timer_fd, HR_TIMER_SET, &uSec, sizeof(uSec));
			break;
		}
		case CEC_RSYNC_HI1: {
			int uSec=400;
			cec_sub_state=CEC_RSYNC_HI2;
			timerdrv->ops->control(cecr_timer_fd, HR_TIMER_SET, &uSec, sizeof(uSec));
			break;
		}
		case CEC_RSYNC_HI2: {
			int uSec=600;
			cec_sub_state=CEC_RSYNC_TOL;
			timerdrv->ops->control(cecr_timer_fd, HR_TIMER_SET, &uSec, sizeof(uSec));
			break;
		}
	}
}

/*********************** End of Start bit, Start of Data bits ***********************/

/* Set up timeout for next bit probe, either data or eom bit */
static void cec_set_probe(void) {
	int uSec=1050;

	timerdrv->ops->control(cecr_timer_fd, HR_TIMER_CANCEL, 0, 0);
	timerdrv->ops->control(cecr_timer_fd, HR_TIMER_SET, &uSec, sizeof(uSec));
	if (!cec_rbi) {
		cec_sub_state=CEC_REC_EOM_I;
	} else {
		cec_sub_state=CEC_REC_PLB_I;
	}
}

/* Setup timeout for next bit probe if passive ack, if active ack, sink bus */
static void cec_set_ack(void) {
	timerdrv->ops->control(cecr_timer_fd, HR_TIMER_CANCEL, 0, 0);
	if (cec_state==CEC_REC_A) {  /* Active ack */
		int uSec=1500;
		/* sink bus line */
		timerdrv->ops->control(cecr_timer_fd, HR_TIMER_SET, &uSec, sizeof(uSec));
		cec_sub_state=CEC_REC_ACK_E;
		cec_flags|=CECR_BYTE_READY;
		
	} else { 
		int uSec=1050;
		timerdrv->ops->control(cecr_timer_fd, HR_TIMER_SET, &uSec, sizeof(uSec));
		cec_sub_state=CEC_REC_ACK_I;
	}
}

static void handle_cec_rec(int pstat) {

	if (pstat) return;

	switch(cec_sub_state) {
		case CEC_REC_IDLE:
		case CEC_REC_PLB_I:
		case CEC_REC_PLB_E:
		case CEC_REC_EOM_I:
		case CEC_REC_EOM_E:
		case CEC_REC_ACK_I:
		case CEC_REC_ACK_E:
			/* Rec fail */
			io_printf("Rec Failed in substate %d, on pinirq\n", cec_sub_state);
			cec_state=0;
			cec_sub_state=0;
			leddrv->ops->control(led_fd,LED_CTRL_DEACTIVATE,&blue,sizeof(blue));
			break;
		case CEC_REC_PLB_N:
			cec_set_probe();
			break;
		case CEC_REC_EOM_N:
			cec_set_ack();
			break;
	}
}


static void cec_get_bit(void) {
	int uSec;
	int pin_stat;

	cec_rx_tmp<<=1;
	pindrv->ops->control(pin_fd, GPIO_SENSE_PIN,&pin_stat,sizeof(pin_stat));

	if (pin_stat) { /* a one bit */
		cec_rx_tmp|=1;
	} /* bit is zero, tmp is already zero */

	cec_rbi--;
	if (!cec_rbi) { /* we got a byte */
		if (cec_state==CEC_REC) {
			/* header byte */
			cec_flags|=CECR_START;
			cec_rx=cec_rx_tmp;
			if ((cec_rx&0xf)==0xf) { /* multicast */
				cec_flags|=CECR_MC;
				cec_state=CEC_REC_P;
			} else if ((1<<(cec_rx&0xf))&cec_ack_mask) {
				cec_state=CEC_REC_A;
				cec_flags|=CECR_TO_ME;
			} else {
				cec_state=CEC_REC_P;
			}
		} else {
			cec_flags&=~CECR_START;
			cec_rx=cec_rx_tmp;
			cec_rx_tmp=0;
		}
	}
	uSec=750;
	timerdrv->ops->control(cecr_timer_fd, HR_TIMER_SET, &uSec, sizeof(uSec));
	cec_sub_state=CEC_REC_PLB_E;
}

/* handle the end of message bit */
static void cec_get_eom(void) {
	int uSec;
	int pin_stat;
	
	pindrv->ops->control(pin_fd, GPIO_SENSE_PIN,&pin_stat,sizeof(pin_stat));
	if (pin_stat) {
		cec_flags|=CECR_EOM;
	} else {
		cec_rbi=8;
	}

	uSec=700;
	timerdrv->ops->control(cecr_timer_fd, HR_TIMER_SET, &uSec, sizeof(uSec));
	cec_sub_state=CEC_REC_EOM_E;
}

static void cec_get_ack(void) {
	int pin_stat;
	int uSec;

	pindrv->ops->control(pin_fd, GPIO_SENSE_PIN,&pin_stat,sizeof(pin_stat));
	if (!pin_stat) { /*Ack is active low, some body else acked */
		cec_flags|=CECR_ACK;
		uSec=700;
		timerdrv->ops->control(cecr_timer_fd, HR_TIMER_SET, &uSec, sizeof(uSec));
		if (cec_state==CEC_REC) {
			cec_state=CEC_REC_P;
		}
		cec_sub_state=CEC_REC_ACK_E;
		cec_flags|=CECR_BYTE_READY;
	} else {
		cec_flags&=~CECR_ACK;
		uSec=700;
		timerdrv->ops->control(cecr_timer_fd, HR_TIMER_SET, &uSec, sizeof(uSec));
		cec_sub_state=CEC_REC_ACK_E;
		cec_flags|=CECR_BYTE_READY;
	}
	io_printf("got byte %02x, flags %x\n",cec_rx, cec_flags);
}

static void handle_rec_tout(void) {
	
	switch(cec_sub_state) {
		case CEC_REC_IDLE:
		case CEC_REC_PLB_N:
		case CEC_REC_EOM_N:
			/* Rec fail */
			io_printf("Rec Failed in substate %d on tout\n", cec_sub_state);
			cec_state=0;
			cec_sub_state=0;
			leddrv->ops->control(led_fd,LED_CTRL_DEACTIVATE,&blue,sizeof(blue));
			break;
		case CEC_REC_PLB_I:
			cec_get_bit();
			break;
		case CEC_REC_PLB_E: {  /* verify line is high at end of bit */
			int pin_stat;
			int uSec=1000;
			pindrv->ops->control(pin_fd, GPIO_SENSE_PIN,&pin_stat,sizeof(pin_stat));
			if (!pin_stat) {
				io_printf("Rec Failed in substate REC_PLB_E on tout\n");
				cec_state=0;
				cec_sub_state=0;
				break;
			}
			timerdrv->ops->control(cecr_timer_fd, HR_TIMER_SET, &uSec, sizeof(uSec));
			cec_sub_state=CEC_REC_PLB_N;
			break;
		}
		case CEC_REC_EOM_I:
			cec_get_eom();
			break;
		case CEC_REC_EOM_E: { /* verify line is high at end of bit */
			int pin_stat;
			int uSec=1000;
			pindrv->ops->control(pin_fd, GPIO_SENSE_PIN,&pin_stat,sizeof(pin_stat));
			if (!pin_stat) {
				io_printf("Rec Failed in substate REC_EOM_E on timeout\n");
				cec_state=0;
				cec_sub_state=0;
				leddrv->ops->control(led_fd,LED_CTRL_DEACTIVATE,&blue,sizeof(blue));
				break;
			}
			timerdrv->ops->control(cecr_timer_fd, HR_TIMER_SET, &uSec, sizeof(uSec));
			cec_sub_state=CEC_REC_EOM_N;      /* wait for line to get low for ack */
			break;
		}
		case CEC_REC_ACK_I:
			cec_get_ack();
			break;
		case CEC_REC_ACK_E: {
			int uSec;
			/* release pin if held for active ack */
			if (cec_flags&CECR_EOM) {
				cec_state=0;
				cec_sub_state=0;
				leddrv->ops->control(led_fd,LED_CTRL_DEACTIVATE,&blue,sizeof(blue));
				break;
			}
			uSec=1200;
			timerdrv->ops->control(cecr_timer_fd, HR_TIMER_SET, &uSec, sizeof(uSec));
			cec_sub_state=CEC_REC_PLB_N;
			break;
		}
	}
		
}

static int pin_irq(void *dum) {
	static int prev_pin_stat=-1;
	int pin_stat;
	pindrv->ops->control(pin_fd, GPIO_SENSE_PIN,&pin_stat,sizeof(pin_stat));
	if (pin_stat==prev_pin_stat) return 0;
	prev_pin_stat=pin_stat;

	switch(cec_state) {
		case CEC_IDLE:
			start_r_sync(pin_stat);
			break;
		case CEC_R_INIT:
			handle_r_init(pin_stat);
			break;
		case CEC_REC:
		case CEC_REC_A:
		case CEC_REC_P:
			handle_cec_rec(pin_stat);
			break;
		default:
			io_printf("bad cec state in pin irq\n");
			break;
	}
			
	return 0;
}

int cecr_timeout(void *dum) {
	switch(cec_state) {
		case CEC_IDLE:
		case CEC_TX:
			break;
		case CEC_R_INIT:
			handle_r_sync_tout();
			break;
		case CEC_REC:
		case CEC_REC_A:
		case CEC_REC_P:
			handle_rec_tout();
			break;
	}
	return 0;
}

#if 0
int  cecs_timeout(void *dum) {
	int uSec=610;
	csc++;
	if (csc==1000) {
		leddrv->ops->control(led_fd,LED_CTRL_ACTIVATE,&red,sizeof(red));
	} else if (csc==2000) {
		csc=0;
		leddrv->ops->control(led_fd,LED_CTRL_DEACTIVATE,&red,sizeof(red));
	}
	drv->ops->control(cecs_fd, HR_TIMER_SET, &uSec, sizeof(uSec));
	return 0;
}


int cecr_timeout(void *dum) {
	int uSec=620;
	crc++;
	if (crc==1000) {
		leddrv->ops->control(led_fd,LED_CTRL_ACTIVATE,&blue,sizeof(blue));
	} else if (crc==2000) {
		crc=0;
		leddrv->ops->control(led_fd,LED_CTRL_DEACTIVATE,&blue,sizeof(blue));
	}
	drv->ops->control(cecr_timer_fd, HR_TIMER_SET, &uSec, sizeof(uSec));
	return 0;
}

#endif


static int cec_drv_open(void *inst, DRV_CBH cb, void *dum) {
	int flags;
	int rc;
	int pin;

	leddrv=driver_lookup(LED_DRV);
	if (!leddrv) return 0;
	led_fd=leddrv->ops->open(leddrv->instance,0,0);
	if (led_fd<0) return -1;

	pindrv=driver_lookup(GPIO_DRV);
	if (!pindrv) return 0;
	pin_fd=pindrv->ops->open(pindrv->instance,pin_irq,0);
	if (pin_fd<0) return -1;

	timerdrv=driver_lookup(HR_TIMER);
	if (!timerdrv) return 0;

	cecr_timer_fd=timerdrv->ops->open(timerdrv->instance,cecr_timeout, 0);
	if (cecr_timer_fd<0) {
		timerdrv=0;
		return -1;
	}
	/* */
			/*PC4 pin for CEC */
	pin=GPIO_PIN(PC,4);
	rc=pindrv->ops->control(pin_fd,GPIO_BIND_PIN,&pin,sizeof(pin));
	if (rc<0) {
		io_printf("pin_assignment failed\n");
		return -1;
	}
	
	flags=GPIO_DIR(0,GPIO_INPUT);
	flags=GPIO_DRIVE(flags,GPIO_PULLUP); 
	flags=GPIO_IRQ_ENABLE(flags);
	rc=pindrv->ops->control(pin_fd,GPIO_SET_FLAGS,&flags,sizeof(flags));
	if (rc<0) {
		io_printf("pin_flags update failed\n");
		return -1;
	}

	return 0;
}

static int cec_drv_close(int fd) {
	return 0;
}

static int cec_drv_control(int fd, int cmd, void *arg1, int size) {
	return -1;
}

static int cec_drv_init(void *inst) {
	return 0;
}

static int cec_drv_start(void *inst) {
	return 0;
}

static struct driver_ops cec_drv_ops = {
	cec_drv_open,
	cec_drv_close,
	cec_drv_control,
	cec_drv_init,
	cec_drv_start,
};

static struct driver cec_drv = {
	CEC_DRV,
	0,
	&cec_drv_ops,
};

void init_cec(void) {
	driver_publish(&cec_drv);
}

INIT_FUNC(init_cec);
