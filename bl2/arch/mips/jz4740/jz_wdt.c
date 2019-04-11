#include <sys.h>
#include <io.h>
#include <devices.h>


void board_reboot(void) {
        struct driver *drv_wdg=driver_lookup("wdg");
        if (!drv_wdg) return;
        drv_wdg->ops->open(drv_wdg->instance,0,0);
}


/*****************************  Watch dog driver ***************************/

static struct device_handle my_dh;


static int wdt_control(struct device_handle *dh, int cmd, void *arg1, int arg2) {
        WDT->tcnt=0;
        return 0;
}

static int wdt_close(struct device_handle *dh) {
        return 0;
}

static struct device_handle *wdt_open(void *instance, DRV_CBH cb_handler, void *dum) {

	WDT->tdr=0xf000;
	WDT->tcnt=0;
	WDT->tcer=WDT_TCER_TCEN;
        return &my_dh;
}


static int wdt_init(void *inst) {
        return 0;
};

static int wdt_start(void *inst) {
//	WDT->tcsr=(WDT_TCSR_PRESCALE_256CLK|WDT_TCSR_PCK_EN);	
        return 0;
}


static struct driver_ops wdt_drv_ops = {
	wdt_open,
	wdt_close,
	wdt_control,
	wdt_init,
	wdt_start
};

static struct driver wdt_drv = {
	"wdg",
	0,
	&wdt_drv_ops,
};

void init_wdt(void) {
	driver_publish(&wdt_drv);
}

INIT_FUNC(init_wdt);
