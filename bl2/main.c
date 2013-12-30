
#include "stm32f4xx_conf.h"
#include "sys.h"
#include "io.h"

#include <string.h>


void tic_wait(unsigned int tic) {
	while(1) {
		if ((((int)tic)-((int)tq_tic))<=0)  {
			return;
		}	
	}
}


struct blink_data {
	unsigned int pin;
	unsigned int delay;
};


void blink(struct blink_data *bd) {
	while(1) {
		GPIOD->ODR ^= (1 << bd->pin);
		sleep(bd->delay);
	}
}

void blink_loop(struct blink_data *bd) {
	while(1) {
		GPIOD->ODR ^= (1 << bd->pin);
		tic_wait((bd->delay/10)+tq_tic);
	}
}


int main(void) {
	struct blink_data green={12,1000};
	struct blink_data amber={13,500};
	struct blink_data red={14,750};
	struct blink_data blue={15,100};

	SysTick_Config(SystemCoreClock/ 100); // 10 mS tic is 100/Sec
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN; // enable the clock to GPIOD
	GPIOD->MODER |= (1 << 24); // set pin 12 to be general purpose output
	GPIOD->MODER |= (1 << 26); // set pin 13 to be general purpose output
	GPIOD->MODER |= (1 << 28); // set pin 14 to be general purpose output
	GPIOD->MODER |= (1 << 30); // set pin 15 to be general purpose output

	init_usart();
	init_sys();
	init_io();
	io_puts("In main, starting blink tasks\n");

	thread_create(blink,&green,256,"green");
	thread_create(blink,&amber,256,"amber");
	thread_create(blink_loop,&red,256,"red_looping");
	thread_create(blink,&blue,256,"blue");
	while (1);
}
