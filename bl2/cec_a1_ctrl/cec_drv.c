
#include <sys.h>
#include <io.h>
#include <led_drv.h>

#include "cec_drv.h"
#include "hr_timer.h"


static int a1s_fd;
static int a1r_fd;
static int cecs_fd;
static int cecr_fd;
static int led_fd;
struct driver *drv;
struct driver *leddrv;


static unsigned int sc=0;
static unsigned int green=LED_GREEN;

unsigned int rc=0;
unsigned int amber=LED_AMBER;

unsigned int csc=0;
unsigned int red=LED_RED;

unsigned int crc=0;
unsigned int blue=LED_BLUE;

int a1s_timeout(void *dum) {
	int nSec=2500;
	sc++;
	if (sc==1000) {
		leddrv->ops->control(led_fd,LED_CTRL_ACTIVATE,&green,sizeof(green));
		io_printf("lighting up\n");
	} else if (sc==2000) {
		sc=0;
		leddrv->ops->control(led_fd,LED_CTRL_DEACTIVATE,&green,sizeof(green));
		io_printf("light out\n");
	}
	drv->ops->control(a1s_fd, HR_TIMER_SET, &nSec, sizeof(nSec));
	return 0;
}


int a1r_timeout(void *dum) {
	int nSec=1250;
	rc++;
	if (rc==1000) {
		leddrv->ops->control(led_fd,LED_CTRL_ACTIVATE,&amber,sizeof(amber));
	} else if (rc==2000) {
		rc=0;
		leddrv->ops->control(led_fd,LED_CTRL_DEACTIVATE,&amber,sizeof(amber));
	}
	drv->ops->control(a1r_fd, HR_TIMER_SET, &nSec, sizeof(nSec));
	return 0;
}


int  cecs_timeout(void *dum) {
	int nSec=610;
	csc++;
	if (csc==1000) {
		leddrv->ops->control(led_fd,LED_CTRL_ACTIVATE,&red,sizeof(red));
	} else if (csc==2000) {
		csc=0;
		leddrv->ops->control(led_fd,LED_CTRL_DEACTIVATE,&red,sizeof(red));
	}
	drv->ops->control(cecs_fd, HR_TIMER_SET, &nSec, sizeof(nSec));
	return 0;
}


int cecr_timeout(void *dum) {
	int nSec=620;
	crc++;
	if (crc==1000) {
		leddrv->ops->control(led_fd,LED_CTRL_ACTIVATE,&blue,sizeof(blue));
	} else if (crc==2000) {
		crc=0;
		leddrv->ops->control(led_fd,LED_CTRL_DEACTIVATE,&blue,sizeof(blue));
	}
	drv->ops->control(cecr_fd, HR_TIMER_SET, &nSec, sizeof(nSec));
	return 0;
}



static int cec_drv_open(void *inst, DRV_CBH cb, void *dum) {
	int nSec;

	leddrv=driver_lookup(LED_DRV);
	if (!leddrv) return 0;
	led_fd=leddrv->ops->open(leddrv->instance,0,0);
	if (led_fd<0) return -1;

	drv=driver_lookup(HR_TIMER);
	if (!drv) return 0;

	nSec=1000;
	a1s_fd=drv->ops->open(drv->instance,a1s_timeout, 0);
	if (a1s_fd<0) {
		drv=0;
		return -1;
	}
	drv->ops->control(a1s_fd, HR_TIMER_SET, &nSec, sizeof(nSec));

//	nSec=2500000;
	nSec=500;
	a1r_fd=drv->ops->open(drv->instance,a1r_timeout, 0);
	if (a1r_fd<0) {
		drv=0;
		return -1;
	}
	drv->ops->control(a1r_fd, HR_TIMER_SET, &nSec, sizeof(nSec));

	nSec=300;
	cecs_fd=drv->ops->open(drv->instance,cecs_timeout, 0);
	if (cecs_fd<0) {
		drv=0;
		return -1;
	}
	drv->ops->control(cecs_fd, HR_TIMER_SET, &nSec, sizeof(nSec));

	nSec=610;
	cecr_fd=drv->ops->open(drv->instance,cecr_timeout, 0);
	if (cecr_fd<0) {
		drv=0;
		return -1;
	}
	drv->ops->control(cecr_fd, HR_TIMER_SET, &nSec, sizeof(nSec));
	
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
