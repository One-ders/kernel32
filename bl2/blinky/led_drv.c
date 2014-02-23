
#include "stm32f4xx_conf.h"
#include "sys.h"
#include "io.h"
#include "led_drv.h"

/*************************  Led driver ***************************/

static unsigned int led_bits;

static int led_control(int kfd, int cmd, void *arg1, int arg2) {
        switch(cmd) {
                case LED_CTRL_STAT:
			if (arg2<1) return -1;
			if (arg2==1) *((unsigned char *)arg1)=led_bits;
			if (arg2==2) *((unsigned short int *)arg1)=led_bits;
			if (arg2==4) *((unsigned int *)arg1)=led_bits;
                        return 0;
                case LED_CTRL_ACTIVATE: {
			if (arg2<1) return -1;
			if (*((unsigned char *)arg1)&LED_GREEN) {
				GPIOD->ODR |= (1<<12);
			}
			if (*((unsigned char *)arg1)&LED_AMBER) {
				GPIOD->ODR |= (1<<13);
			}
			if (*((unsigned char *)arg1)&LED_RED) {
				GPIOD->ODR |= (1<<14);
			}
			if (*((unsigned char *)arg1)&LED_BLUE) {
				GPIOD->ODR |= (1<<15);
			}
			led_bits = (GPIOD->ODR>>12);
			break;
		}
                case LED_CTRL_DEACTIVATE: {
			if (arg2<1) return -1;
			if (*((unsigned char *)arg1)&LED_GREEN) {
				GPIOD->ODR &= ~(1<<12);
			}
			if (*((unsigned char *)arg1)&LED_AMBER) {
				GPIOD->ODR &= ~(1<<13);
			}
			if (*((unsigned char *)arg1)&LED_RED) {
				GPIOD->ODR &= ~(1<<14);
			}
			if (*((unsigned char *)arg1)&LED_BLUE) {
				GPIOD->ODR &= ~(1<<15);
			}
			led_bits = (GPIOD->ODR>>12);
			break;
		}
                default:
                        return -1;
        }
        return 0;
}

static int led_close(int kfd) {
        return 0;
}

static int led_open(void *instance, DRV_CBH cb_handler, void *dum) {
        return 0;
}


static int led_init(void *inst) {
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN; // enable the clock to GPIOD
	GPIOD->MODER |= (1 << 24); // set pin 12 to be general purpose output
	GPIOD->MODER |= (1 << 26); // set pin 13 to be general purpose output
	GPIOD->MODER |= (1 << 28); // set pin 14 to be general purpose output
	GPIOD->MODER |= (1 << 30); // set pin 15 to be general purpose output
	return 0;
};

static int led_start(void *inst) {
	return 0;
}

static struct driver_ops led_drv_ops = { 
        led_open,
        led_close,
        led_control,
	led_init,
	led_start,
};

static struct driver led_drv = {
	LED_DRV,
	0,
	&led_drv_ops,
};
 
void init_led_drv(void) {
	driver_publish(&led_drv);
}

INIT_FUNC(init_led_drv);
