
#include "sys.h"
#include "io.h"

#include "led_drv.h"

#include <string.h>


void tic_wait(unsigned int tic) {
	while(1) {
		if ((((int)tic)-((int)tq_tic))<=0)  {
			return;
		}
	}
}


struct blink_data {
	unsigned int led;
	unsigned int delay;
};


void blink(struct blink_data *bd) {
	int fd=io_open(LED_DRV);
	if (fd<0) return;
	while(1) {
		int led_stat;
		int rc=io_control(fd,LED_CTRL_STAT,&led_stat,sizeof(led_stat));
		if (rc<0) return;
		if (led_stat&bd->led) {
			rc=io_control(fd,LED_CTRL_DEACTIVATE,&bd->led,sizeof(bd->led));
		} else {
			rc=io_control(fd,LED_CTRL_ACTIVATE,&bd->led,sizeof(bd->led));
		}

		sleep(bd->delay);
	}
}

void blink_loop(struct blink_data *bd) {
	int fd=io_open(LED_DRV);
	if (fd<0) return;
	while(1) {
		int led_stat;
		int rc=io_control(fd,LED_CTRL_STAT,&led_stat,sizeof(led_stat));
		if (rc<0) return;
		if (led_stat&bd->led) {
			rc=io_control(fd,LED_CTRL_DEACTIVATE,&bd->led,sizeof(bd->led));
		} else {
			rc=io_control(fd,LED_CTRL_ACTIVATE,&bd->led,sizeof(bd->led));
		}
		tic_wait((bd->delay/10)+tq_tic);
	}
}


int main(void) {
	struct blink_data green={LED_GREEN,1000};
	struct blink_data amber={LED_AMBER,500};
	struct blink_data red={LED_RED,750};
	struct blink_data blue={LED_BLUE,100};

	init_sys();
	init_io();

	start_sys();
	printf("In main, starting blink tasks\n");

	io_open("test_driver");

	thread_create(sys_mon,"usb_serial0",0,2,"sys_mon_u");
	thread_create(blink,&green,sizeof(green),1,"green");
	thread_create(blink,&amber,sizeof(amber),1,"amber");
//	thread_create(blink_loop,&red,sizeof(red),256,"red_looping");
	thread_create(blink, &red,sizeof(red),1, "red");
	thread_create(blink, &blue,sizeof(blue),1,"blue");
	while (1);
}
