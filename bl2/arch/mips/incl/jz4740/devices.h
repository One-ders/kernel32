
#ifndef __DEVICES_H
#define __DEVICES_H

#include <busses.h>

#include <cgu.h>
#include <pmc.h>
#include <lcdc.h>
#include <wdt.h>

#define CGU	((struct CGU *)(APB+0))
#define PMC	((struct PMC *)(APB+0))   //overlap with prev
//#define INTC	((struct INTC *)(APB+0x1000))
#define TCU	((struct TCU *)(APB+0x2000))
#define WDT     ((struct WDT *)(APB+0x2000))
#define RTC	((struct RTC *)(APB+0x3000))
#define GPIO	((struct GPIO *)(APB+0x10000))
#define AIC	((struct AIC *)(APB+0x20000))
// also Codec
#define MSC	((struct MSC *)(APB+0x21000))
#define UART0	((struct UART *)(APB+0x30000))
#define UART1	((struct UART *)(APB+0x31000))
#define UART2	((struct UART *)(APB+0x32000))
#define UART3	((struct UART *)(APB+0x33000))
#define I2C	((struct I2C *)(APB+0x42000))
#define SSI	((struct SSI *)(APB+0x43000))
#define SADC	((struct SADC *)(APB+0x70000))

#define EMC	((struct EMC *)(AHB+0x10000))
#define DMAC	((struct DMAC *)(AHB+0x20000))
#define UCH	((struct UHC *)(AHB+0x30000))
#define UDC	((struct UDC *)(AHB+0x40000))
#define LCDC	((struct LCDC *)(AHB+0x50000))
#define CIM	((struct CIM *)(AHB+0x60000))
#define IPU	((struct IPU *)(AHB+0x80000))

#endif
