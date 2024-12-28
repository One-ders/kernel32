
#if 0
#include "sys.h"
#include "devices.h"
#include "gpio.h"

// GPIO pins used for EMMC.
#define GPIO_DAT3  53
#define GPIO_DAT2  52
#define GPIO_DAT1  51
#define GPIO_DAT0  50
#define GPIO_CMD   49
#define GPIO_CLK   48
#define GPIO_CD    47

extern int wait_cycles(unsigned long int t);

// Standard driver user management
struct userdata {
	struct device_handle dh;
	unsigned long int fpos;
	unsigned int partition;
	int busy;
};

static struct userdata ud[16];

static struct userdata *get_userdata(void) {
	int i;
	for(i=0;i<16;i++) {
		if (!ud[i].busy) {
			ud[i].busy=1;
			return &ud[i];
		}
	}
	return 0;
}

static void put_userdata(struct userdata *uud) {
	int i;
	for(i=0;i<16;i++) {
		if (&ud[i]==uud) {
			uud->busy=0;
			return;
		}
	}
}


// support funcs
static void init_gpio() {
#if 0
	// gpio for card detect pin 47
	gpio_set_function(GPIO_CD, GPIO_PIN_INPUT);
	gpio_set_pull(GPIO_CD, GPPUD_PULL_UP);
	gpio_set_hdetect(GPIO_CD);
#endif

	// gpio data0 to data3 --> pin 50-53
	gpio_set_function(GPIO_DAT0, GPIO_PIN_ALT3);
	gpio_set_function(GPIO_DAT1, GPIO_PIN_ALT3);
	gpio_set_function(GPIO_DAT2, GPIO_PIN_ALT3);
	gpio_set_function(GPIO_DAT3, GPIO_PIN_ALT3);
	gpio_set_pull(GPIO_DAT0, GPPUD_PULL_UP);
	gpio_set_pull(GPIO_DAT1, GPPUD_PULL_UP);
	gpio_set_pull(GPIO_DAT2, GPPUD_PULL_UP);
	gpio_set_pull(GPIO_DAT3, GPPUD_PULL_UP);

	// clk & cmd pin 48 and 49
	gpio_set_function(GPIO_CLK, GPIO_PIN_ALT3);
	gpio_set_function(GPIO_CMD, GPIO_PIN_ALT3);

	gpio_set_pull(GPIO_CLK, GPPUD_PULL_UP);
	gpio_set_pull(GPIO_CMD, GPPUD_PULL_UP);
}

// emmc driver <-> os integration

static struct device_handle *emmc_open(void *driver_instance, DRV_CBH cb_handler, void *dum) {
	return 0;
}

static int emmc_close(struct device_handle *dh) {
	put_userdata((struct userdata *)dh);
	return 0;
}

static int emmc_control(struct device_handle *dh,
			int cmd,
			void *arg1,
			int arg2) {
	return -1;
}

static int emmc_start(void *inst) {
	return 0;
}

static int init_done=0;
static int emmc_init(void *inst) {
	struct GPIO_DEV *gpio_dev=GPIO_DEV_BASE;
	if (!init_done) {
		init_done=1;
		init_gpio();

		while(1) {
			sys_printf("gpfel0=%08x\n", gpio_dev->gpfsel[0]);
			sys_printf("gpfel1=%08x\n", gpio_dev->gpfsel[1]);
			sys_printf("gpfel2=%08x\n", gpio_dev->gpfsel[2]);
			sys_printf("gpfel3=%08x\n", gpio_dev->gpfsel[3]);
			sys_printf("gpfel4=%08x\n", gpio_dev->gpfsel[4]);
			sys_printf("gpfel5=%08x\n", gpio_dev->gpfsel[5]);
			sys_printf("reg @%08x:%08x,%08x\n",
				&gpio_dev->gplev[0],
				gpio_dev->gplev[0],
				gpio_dev->gplev[1]);
			wait_cycles(250000000);
		}

		cardAbsent=gpio_read_pin(GPIO_CD);
		cardEjected=gpio_read_event(GPIO_CD);
		sys_printf("emmc_init: cardAbsend=%d, cardEjected=%d\n",cardAbsent,cardEjected);
	}

	return 0;
}

static struct driver_ops emmc_drv_ops = {
	emmc_open,
	emmc_close,
	emmc_control,
	emmc_init,
	emmc_start,
};

static struct driver emmc_drv0 = {
	"emmc0",
	(void *)0,
	&emmc_drv_ops,
};

static struct driver emmc_drv1 = {
	"emmc1",
	(void *)1,
	&emmc_drv_ops,
};

void init_emmc() {
	driver_publish(&emmc_drv0);
	driver_publish(&emmc_drv1);
}

INIT_FUNC(init_emmc);
#endif
