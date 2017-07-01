#include <sys.h>
#include <io.h>
#include <devices.h>
#include <stm32f407.h>
#include <gpio_drv.h>

#include "usb_core.h"

static struct usb_dev_handle *usb_dev;

void OTG_FS_IRQHandler(void) {
	sys_irqs++;
	handle_device_irq(usb_dev);
}

void OTG_HS_IRQHandler(void) {
	sys_irqs++;
	handle_device_irq(usb_dev);
}

int bsp_usb_enable_interrupt(struct usb_dev_handle *pdev) {
	if (pdev->regs==USB_OTG_HS_ADDR) {
		NVIC_SetPriority(OTG_HS_IRQn,0xc);
		NVIC_EnableIRQ(OTG_HS_IRQn);
	} else {
		NVIC_SetPriority(OTG_FS_IRQn,0xc);
		NVIC_EnableIRQ(OTG_FS_IRQn);
	}
	return 0;
}


int bsp_usb_init(struct usb_dev_handle *pdev) {
	struct driver *pindrv=driver_lookup(GPIO_DRV);
	struct device_handle *pin_dh;
	struct pin_spec ps[4];
	unsigned int flags;
	int altfn;

	usb_dev=pdev;

	if (!pindrv) return 0;
	pin_dh=pindrv->ops->open(pindrv->instance,0,0);
	if(!pin_dh) return 0;

	if (pdev->regs==USB_OTG_HS_ADDR) {
		altfn=12;
	} else {
		altfn=10;
	}


//      ps[0].pin=USB_VBUS;
	ps[0].pin=USB_ID;
	flags=GPIO_DIR(0,GPIO_ALTFN_PIN);
//      flags=GPIO_DIR(0,GPIO_INPUT);
	flags=GPIO_SPEED(flags,GPIO_SPEED_HIGH);
	flags=GPIO_DRIVE(flags,GPIO_PUSHPULL);
	flags=GPIO_ALTFN(flags,altfn);
	ps[0].flags=flags;
//      ps[1].pin=USB_ID;
	ps[1].pin=USB_VBUS;
	flags=GPIO_DIR(0,GPIO_INPUT);
//      flags=GPIO_DRIVE(flags,GPIO_PULLUP);
//      flags=GPIO_SPEED(flags,GPIO_SPEED_HIGH);
//      flags=GPIO_ALTFN(flags,0xa);
	ps[1].flags=flags;
	ps[2].pin=USB_DM;
	flags=GPIO_DIR(0,GPIO_ALTFN_PIN);
	flags=GPIO_SPEED(flags,GPIO_SPEED_HIGH);
	flags=GPIO_DRIVE(flags,GPIO_PUSHPULL);
	flags=GPIO_ALTFN(flags,altfn);
	ps[2].flags=flags;
	ps[3].pin=USB_DP;
	flags=GPIO_DIR(0,GPIO_ALTFN_PIN);
	flags=GPIO_SPEED(flags,GPIO_SPEED_HIGH);
	flags=GPIO_DRIVE(flags,GPIO_PUSHPULL);
	flags=GPIO_ALTFN(flags,altfn);
	ps[3].flags=flags;

	pindrv->ops->control(pin_dh,GPIO_BUS_ASSIGN_PINS,ps,sizeof(ps));

	if (pdev->regs==USB_OTG_HS_ADDR) {
		RCC->AHB1ENR|=RCC_AHB1ENR_OTGHSEN;
	} else {
		RCC->AHB2ENR|=RCC_AHB2ENR_OTGFSEN;
	}

	return 0;
}

void init_usb_core_drv(void) {

}

INIT_FUNC(init_usb_core_drv);
