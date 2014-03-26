
#include <stm32f4xx.h>
#include <sys.h>
#include <io.h>

#include "gpio_drv.h"

struct GPIO_REG {
	volatile unsigned int moder;
	volatile unsigned int otyper;
	volatile unsigned int ospeedr;
	volatile unsigned int pupdr;
	volatile unsigned int idr;
	volatile unsigned int odr;
	volatile unsigned int bsrr;
	volatile unsigned int lckr;
	volatile unsigned int afrl;
	volatile unsigned int afrh;
};

struct GPIO_REG *GPIO[9];

#define GPIO_X 9
static unsigned short int pinmap[GPIO_X];

#define PIN_FLAGS_ASSIGNED	0x40000000
#define PIN_FLAGS_IN_USE	0x80000000
struct pin_data {
	unsigned int pin_flags;
	unsigned int bus;
	unsigned int bpin;
	/*=======================*/
	void		*userdata;
	int 		fd;
	DRV_CBH 	callback;
};

struct exti_regs {
	volatile unsigned int imr;
	volatile unsigned int emr;
	volatile unsigned int rtsr;
	volatile unsigned int ftsr;
	volatile unsigned int swier;
	volatile unsigned int pr;
};

struct exti_regs * const exti_regs=(struct exti_regs *)(APB2PERIPH_BASE+0x3c00);

#define MAX_USERS 16
static struct pin_data pd[MAX_USERS];

static struct pin_data *exti2pd[16];

unsigned int * const exti_cr=(unsigned int *)(APB2PERIPH_BASE+0x3808);

static unsigned int get_user(struct pin_data **pdpptr) {
	int i;
	for(i=0;i<MAX_USERS;i++) {
		if (!pd[i].pin_flags) {
			pd[i].pin_flags=PIN_FLAGS_IN_USE;
			(*pdpptr)=&pd[i];
			return i;
		}
	}
	return -1;
}


/*************** The Irqers *************************/

void EXTI0_IRQHandler(void) {
	if (exti2pd[0]) {
		if (exti2pd[0]->callback) {
			exti2pd[0]->callback(exti2pd[0]->fd,EV_STATE,exti2pd[0]->userdata);
		}
	}
	exti_regs->pr=(1<<0);
}

void EXTI1_IRQHandler(void) {
	if (exti2pd[1]) {
		if (exti2pd[1]->callback) {
			exti2pd[1]->callback(exti2pd[1]->fd,EV_STATE,exti2pd[1]->userdata);
		}
	}
	exti_regs->pr=(1<<1);
}

void EXTI2_IRQHandler(void) {
	if (exti2pd[2]) {
		if (exti2pd[2]->callback) {
			exti2pd[2]->callback(exti2pd[2]->fd,EV_STATE,exti2pd[2]->userdata);
		}
	}
	exti_regs->pr=(1<<2);
}

void EXTI3_IRQHandler(void) {
	if (exti2pd[3]) {
		if (exti2pd[3]->callback) {
			exti2pd[3]->callback(exti2pd[3]->fd,EV_STATE,exti2pd[3]->userdata);
		}
	}
	exti_regs->pr=(1<<3);
}

void EXTI4_IRQHandler(void) {
	if (exti2pd[4]) {
		if (exti2pd[4]->callback) {
			exti2pd[4]->callback(exti2pd[4]->fd,EV_STATE,exti2pd[4]->userdata);
		}
	}
	exti_regs->pr=(1<<4);
}

void EXTI9_5_IRQHandler(void) {
	sys_printf("exti9_5_irq\n");
	exti_regs->pr=0x03e0;
}

void EXTI15_10_IRQHandler(void) {
	sys_printf("exti15_10_irq\n");
	exti_regs->pr=0xfc00;
}


/**************** Support functions **********************/


static int assign_pin(struct pin_data *pdp, int pin) {
	int bus=pin>>4;
	int bpin=pin&0xf;

	if (pdp->pin_flags&PIN_FLAGS_ASSIGNED) {
		return -1;
	}
	sys_printf("lookup pin %d at bus %d\n",bpin,bus);

	if (pinmap[bus]&(1<<bpin)) {
		sys_printf("pin in use %x\n", pin);
		return -1;
	}

	if (!pinmap[bus]) {
		RCC->AHB1ENR|=(1<<bus);
	}
	pinmap[bus]|=(1<<bpin);
	pdp->bus=bus;
	pdp->bpin=bpin;
	pdp->pin_flags|=PIN_FLAGS_ASSIGNED;
	return 0;
}

static int read_pin(struct pin_data *pdp) {
	if (!(pdp->pin_flags&PIN_FLAGS_ASSIGNED)) {
		return -1;
	}
	return (pdp->bus<<4)|pdp->bpin;
}

static int sense_pin(struct pin_data *pdp) {
	if (GPIO[pdp->bus]->idr&(1<<pdp->bpin)) {
		return 1;
	}
	return 0;
}

static int out_pin(struct pin_data *pdp, unsigned int val) {
	if (val) {
		GPIO[pdp->bus]->bsrr=(1<<pdp->bpin);
	} else {
		GPIO[pdp->bus]->bsrr=(1<<(pdp->bpin+16));
	}
	return 0;
}

static int sink_pin(struct pin_data *pdp) {
	GPIO[pdp->bus]->moder|=(1<<(pdp->bpin<<1));
	return 0;
}

static int release_pin(struct pin_data *pdp) {
	exti_regs->pr=(1<<pdp->bpin);
	GPIO[pdp->bus]->moder&=~(3<<(pdp->bpin<<1));
	return 0;
}

static int set_flags(struct pin_data *pdp, unsigned int flags) {
	int drive=(flags&GPIO_DRIVE_MASK)>>GPIO_DRIVE_SHIFT;
	int speed=(flags&GPIO_SPEED_MASK)>>GPIO_SPEED_SHIFT;
	if ((flags&GPIO_OUTPUT)||(flags&GPIO_BUSPIN)) {
		sys_printf("pin is output or bus\n");
		GPIO[pdp->bus]->moder&=~(3<<(pdp->bpin<<1));
		if (flags&GPIO_OUTPUT) {
			GPIO[pdp->bus]->moder|=(1<<(pdp->bpin<<1));
		}

		if ((flags&GPIO_OPENDRAIN)||(flags&GPIO_BUSPIN)) {
			GPIO[pdp->bus]->otyper&=~(1<<pdp->bpin);
		} else if (flags&GPIO_PUSHPULL) {
			GPIO[pdp->bus]->otyper|=(1<<pdp->bpin);
		}

		GPIO[pdp->bus]->ospeedr&=~(3<<(pdp->bpin<<1));
		GPIO[pdp->bus]->ospeedr|=(speed<<(pdp->bpin<<1));
	}

	if (flags&GPIO_BUSPIN) {    /* make sure output is low */
		GPIO[pdp->bus]->bsrr=(1<<(pdp->bpin+16));
	}
		
	if ((flags&GPIO_INPUT)||(flags&GPIO_BUSPIN)) {
		sys_printf("pin is input or bus\n");
		GPIO[pdp->bus]->moder&=~(3<<(pdp->bpin<<1));

		switch(drive) {
			case GPIO_FLOAT: {
				sys_printf("gpio float\n");
				GPIO[pdp->bus]->pupdr&=~(3<<(pdp->bpin<<1));
				break;
			}
			case GPIO_PULLUP: {
				sys_printf("gpio pullup\n");
				GPIO[pdp->bus]->pupdr&=~(3<<(pdp->bpin<<1));
				GPIO[pdp->bus]->pupdr|=(1<<(pdp->bpin<<1));
				break;
			}
			case GPIO_PULLDOWN: {
				sys_printf("gpio_pulldown\n");
				GPIO[pdp->bus]->pupdr&=~(3<<(pdp->bpin<<1));
				GPIO[pdp->bus]->pupdr|=(2<<(pdp->bpin<<1));
				break;
			}
			default: {
				sys_printf(" gpio unknown drive mode %d\n",drive);
			}
		}
	}

	if (flags&GPIO_IRQ) {
		int exticr=pdp->bpin>>2;
		int exticrshift=pdp->bpin&0x3;

		if ((exti2pd[pdp->bpin]) && (exti2pd[pdp->bpin]!=pdp)) {
			sys_printf("interrupt line cannot not be assigned, in use\n");
			return -1;
		}	
		exti2pd[pdp->bpin]=pdp;

		if ((pdp->bpin>=0)&&(pdp->bpin<5)) {
			NVIC_SetPriority(pdp->bpin+6,0x1);
			NVIC_EnableIRQ(pdp->bpin+6);
		} else if ((pdp->bpin>=5)&&(pdp->bpin<10)) {
			NVIC_SetPriority(23,0x1);
			NVIC_EnableIRQ(23);
		} else if ((pdp->bpin>=10)&&(pdp->bpin<16)) {
			NVIC_SetPriority(40,0x1);
			NVIC_EnableIRQ(40);
		}	
	

		/* Syscfg exticr */
		exti_cr[exticr]&=~(0xf<<exticrshift);
		exti_cr[exticr]|=(pdp->bus<<exticrshift);

		exti_regs->rtsr|=(1<<pdp->bpin);
		exti_regs->ftsr|=(1<<pdp->bpin);
		exti_regs->imr|=(1<<pdp->bpin);
	}
	return 0;
}

static int clr_flags(struct pin_data *pdp, unsigned int flags) {

	if (flags&GPIO_IRQ) {
		exti_regs->imr&=~(1<<pdp->bpin);
	}
	return 0;
}



/**********************************************************/

static int gpio_open(void *instance, DRV_CBH callback, void *userdata,int fd) {
	struct pin_data *pd=0;
	int ix=get_user(&pd);
	sys_printf("gpio_open\n");
	if (ix<0) return -1;
	pd->userdata=userdata;
	pd->fd=fd;
	pd->callback=callback;
	return ix;
}

static int gpio_close(int fd) {
	sys_printf("gpio_close\n");
	if (pd[fd].pin_flags) {
		pd[fd].pin_flags=0;
	}
	return 0;
}

static int gpio_control(int fd, int cmd, void *arg1, int arg2) {
	struct pin_data *pdp;
	if (fd>MAX_USERS||fd<0) {
		return -1;
	}
	pdp=&pd[fd];
	if (!(pdp->pin_flags&PIN_FLAGS_IN_USE)) return -1;
	switch(cmd) {
		case GPIO_BIND_PIN: {
			unsigned int pin;
			if (arg2!=4) return -1;
			pin=*((unsigned int *)arg1);
			return assign_pin(pdp,pin);
			break;
		}
		case GPIO_GET_BOUND_PIN: {
			if (arg2!=4) return -1;
			*((unsigned int *)arg1)=read_pin(pdp);
			return 0;
			break;	
		}
		case GPIO_SET_FLAGS: {
			unsigned int flags;
			if (arg2!=4) return -1;
			flags=*((unsigned int *)arg1);
			set_flags(pdp,flags);
			return 0;
		}
		case GPIO_CLR_FLAGS: {
			unsigned int flags;
			if (arg2!=4) return -1;
			flags=*((unsigned int *)arg1);
			clr_flags(pdp,flags);
			return 0;
		}
		case GPIO_SENSE_PIN: {
			if (arg2!=4) return -1;
			*((unsigned int *)arg1)=sense_pin(pdp);
			return 0;
			break;	
		}
		case GPIO_SET_PIN: {
			if (arg2!=4) return -1;
			out_pin(pdp,*((unsigned int *)arg1));
			return 0;
			break;
		}
		case GPIO_SINK_PIN: {
			return sink_pin(pdp);
			break;
		}
		case GPIO_RELEASE_PIN: {
			return release_pin(pdp);
			break;
		}
	
	}
	return 0;
}

static int gpio_init(void *inst) {
	int i;
	for(i=0;i<9;i++) {
		GPIO[i]=(struct GPIO_REG *)(AHB1PERIPH_BASE+(i*0x400));
	}
	RCC->APB2ENR|=RCC_APB2ENR_SYSCFGEN;
	return 0;
}

static int gpio_start(void *inst) {
	return 0;
}

static struct driver_ops gpio_drv_ops = {
	gpio_open,
	gpio_close,
	gpio_control,
	gpio_init,
	gpio_start,
};

static struct driver gpio_drv = {
	GPIO_DRV,
	0,
	&gpio_drv_ops,
};

void init_gpio_drv(void) {
	driver_publish(&gpio_drv);
}

INIT_FUNC(init_gpio_drv);
