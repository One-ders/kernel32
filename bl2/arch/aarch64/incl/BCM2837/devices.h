
#ifndef __DEVICES_H
#define __DEVICES_H

#include <sys_const.h>

//#define VA_START 0xffff000000000000
//#define PBASE	0x3F000000

#include <sys_timer.h>
#include <irq_ctrl.h>
#include <gpio_dev.h>
#include <uart_dev.h>
#include <sdhost_reg.h>
#include <aux_dev.h>
#include <emmc_dev.h>

#define SYS_TIMER_BASE	((struct SYS_TIMER *)(VA_START+PBASE+0x3000))
#define IRQ_CTRL_BASE	((struct IRQ_CTRL  *)(VA_START+PBASE+0xB200))
#define GPIO_DEV_BASE	((struct GPIO_DEV  *)(VA_START+PBASE+0x200000))
#define UART_DEV_BASE	((struct UART_DEV  *)(VA_START+PBASE+0x201000))
#define SDHOST_BASE	((struct SDHOST    *)(VA_START+PBASE+0x202000))
#define AUX_DEV_BASE	((struct AUX_DEV   *)(VA_START+PBASE+0x215000))
#define EMMC_DEV_BASE	((struct EMMC_DEV  *)(VA_START+PBASE+0x300000))
#endif
