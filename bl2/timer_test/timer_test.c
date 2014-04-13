
#include <sys.h>
#include <io.h>
#include <led_drv.h>

#include "hr_timer.h"
#include "gpio_drv.h"


static struct device_handle *cecs_dh;
static struct device_handle *cecr_dh;
static struct device_handle *led_dh;
struct driver *drv;
struct driver *leddrv;

static struct device_handle *pin_dh;
static struct device_handle *pin_out_dh;
struct driver *pindrv;

unsigned int csc=0;
unsigned int red=LED_RED;

unsigned int crc=0;
unsigned int blue=LED_BLUE;


static int pin_irq(struct device_handle *dh, int ev, void *dum) {
	int pin_stat;
	sys_printf("pin_irq\n");

	pindrv->ops->control(pin_dh, GPIO_SENSE_PIN,&pin_stat,sizeof(pin_stat));
	if (pin_stat) {
		pin_stat=0;
	} else {
		pin_stat=1;
	}
	pindrv->ops->control(pin_out_dh, GPIO_SET_PIN,&pin_stat,sizeof(pin_stat));

	return 0;
}

int  cecs_timeout(struct device_handle *dh, int ev, void *dum) {
	int nSec=610;
	csc++;
	if (csc==1000) {
		leddrv->ops->control(led_dh,LED_CTRL_ACTIVATE,&red,sizeof(red));
	} else if (csc==2000) {
		csc=0;
		leddrv->ops->control(led_dh,LED_CTRL_DEACTIVATE,&red,sizeof(red));
	}
	drv->ops->control(cecs_dh, HR_TIMER_SET, &nSec, sizeof(nSec));
	return 0;
}


int cecr_timeout(struct device_handle *dh, int ev, void *dum) {
	int nSec=620;
	crc++;
	if (crc==1000) {
		leddrv->ops->control(led_dh,LED_CTRL_ACTIVATE,&blue,sizeof(blue));
	} else if (crc==2000) {
		crc=0;
		leddrv->ops->control(led_dh,LED_CTRL_DEACTIVATE,&blue,sizeof(blue));
	}
	drv->ops->control(cecr_dh, HR_TIMER_SET, &nSec, sizeof(nSec));
	return 0;
}



static struct device_handle *cec_drv_open(void *inst, DRV_CBH cb, void *dum) {
	int nSec;
	int flags;
	int rc;
	int pin;
	int one=1;

	leddrv=driver_lookup(LED_DRV);
	if (!leddrv) return 0;
	led_dh=leddrv->ops->open(leddrv->instance,0,0);
	if (!led_dh) return 0;

	pindrv=driver_lookup(GPIO_DRV);
	if (!pindrv) return 0;
	pin_dh=pindrv->ops->open(pindrv->instance,pin_irq,0);
	if (!pin_dh) return 0;
	pin_out_dh=pindrv->ops->open(pindrv->instance,0,0);
	if (!pin_out_dh) return 0;

	drv=driver_lookup(HR_TIMER);
	if (!drv) return 0;

	nSec=300;
	cecs_dh=drv->ops->open(drv->instance,cecs_timeout, 0);
	if (!cecs_dh) {
		drv=0;
		return 0;
	}
	drv->ops->control(cecs_dh, HR_TIMER_SET, &nSec, sizeof(nSec));

	nSec=610;
	cecr_dh=drv->ops->open(drv->instance,cecr_timeout, 0);
	if (!cecr_dh) {
		drv=0;
		return 0;
	}
	drv->ops->control(cecr_dh, HR_TIMER_SET, &nSec, sizeof(nSec));

	/* */
			/*PA0 is the blue button */
	pin=GPIO_PIN(PA,0);
	rc=pindrv->ops->control(pin_dh,GPIO_BIND_PIN,&pin,sizeof(pin));
	if (rc<0) {
		sys_printf("pin_assignment failed\n");
		return 0;
	}

	flags=GPIO_DIR(0,GPIO_INPUT);
	flags=GPIO_DRIVE(flags,GPIO_FLOAT);
	flags=GPIO_IRQ_ENABLE(flags);
	rc=pindrv->ops->control(pin_dh,GPIO_SET_FLAGS,&flags,sizeof(flags));
	if (rc<0) {
		sys_printf("pin_flags update failed\n");
		return 0;
	}

	pin=GPIO_PIN(PD,5);
	rc=pindrv->ops->control(pin_out_dh,GPIO_BIND_PIN,&pin,sizeof(pin));
	if (rc<0) {
		sys_printf("pin_assignment failed\n");
		return 0;
	}


	flags=GPIO_DIR(0,GPIO_OUTPUT);
	flags=GPIO_DRIVE(flags,GPIO_OPENDRAIN);
	rc=pindrv->ops->control(pin_out_dh,GPIO_SET_FLAGS,&flags,sizeof(flags));
	rc=pindrv->ops->control(pin_out_dh,GPIO_SET_PIN,&one,sizeof(one));

	return 0;
}

static int cec_drv_close(struct device_handle *dh) {
	return 0;
}

static int cec_drv_control(struct device_handle *dh, int cmd, void *arg1, int size) {
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
	"test_driver",
	0,
	&cec_drv_ops,
};

void init_cec(void) {
	driver_publish(&cec_drv);
}

INIT_FUNC(init_cec);
