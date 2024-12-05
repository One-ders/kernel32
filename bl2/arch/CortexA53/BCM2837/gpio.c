
#include "sys.h"
#include "devices.h"

int wait_cycles(unsigned long int t) {
	asm volatile(
		"1: subs %0, %0, #1\n\t"
		"bne 1b"
		: "=r" (t)
		: "0" (t)
	);
	return 0;
}

int gpio_set_function(unsigned int pin,unsigned int fnc) {
	struct GPIO_DEV *gpio_dev=GPIO_DEV_BASE;
	unsigned int pin_bit=pin%10;
	unsigned int pin_reg=pin/10;
	unsigned int fsel=gpio_dev->gpfsel[pin_reg];

	// clear out bit field
	fsel&=~PIN_X(pin_bit,GPIO_PIN_MASK);
	if (fnc!=0) { // input is default when cleared
		fsel|=PIN_X(pin_bit,fnc);
	}
	gpio_dev->gpfsel[pin_reg]=fsel;
	return 0;
}

int gpio_set_pull(unsigned int pin, unsigned int pull_type) {
	struct GPIO_DEV *gpio_dev=GPIO_DEV_BASE;
	unsigned int pin_bit=pin%32;
	unsigned int pin_reg=pin/32;

	gpio_dev->gppud=pull_type;
	wait_cycles(150);
	gpio_dev->gppudclk[pin_reg]=(1<<pin_bit);
	wait_cycles(150);
	gpio_dev->gppudclk[pin_reg]=0;
	gpio_dev->gppud=0;
	if (pin!=14&&pin!=15) {
		sys_printf("set_pul real reg fsel4 after: %08x\n", gpio_dev->gpfsel[4]);
	}
	return 0;
}

int gpio_set_hdetect(unsigned int pin) {
	struct GPIO_DEV *gpio_dev=GPIO_DEV_BASE;
	unsigned int pin_bit=pin%32;
	unsigned int pin_reg=pin/32;
	gpio_dev->gphen[pin_reg]|=(1<<pin_bit);
	if (pin!=14&&pin!=15) {
		sys_printf("set_hdet real reg fsel4 after: %08x\n", gpio_dev->gpfsel[4]);
	}
	return 0;
}

int  gpio_read_pin(unsigned int pin) {
	struct GPIO_DEV *gpio_dev=GPIO_DEV_BASE;
	unsigned int pin_bit=pin%32;
	unsigned int pin_reg=pin/32;
	return gpio_dev->gplev[pin_reg]&(1<<pin_bit);
}

int gpio_read_event(unsigned int pin) {
	struct GPIO_DEV *gpio_dev=GPIO_DEV_BASE;
	unsigned int pin_bit=pin%32;
	unsigned int pin_reg=pin/32;
	return gpio_dev->gpeds[pin_reg]&(1<<pin_bit);
}
