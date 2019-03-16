
#ifndef __DEVICES_H
#define __DEVICES_H

#include <busses.h>

#include <cgu.h>
#include <pmc.h>
#include <lcdc.h>
#include <wdt.h>
#include <intc.h>
#include <uart.h>

#define CGU	((struct CGU *)(APB+0))
#define PMC	((struct PMC *)(APB+0))   //overlap with prev
#define INTC	((struct INTC *)(APB+0x1000))
#define TCU	((struct TCU *)(APB+0x2000))
#define OST	((struct TCU *)(APB+0x2000))
#define WDT     ((struct WDT *)(APB+0x2000))
#define RTC	((struct RTC *)(APB+0x3000))
#define GPIO	((struct GPIO *)(APB+0x10000))
#define AIC	((struct AIC *)(APB+0x20000))
// also Codec
#define MSC0  	((struct MSC *)(APB+0x21000))
#define MSC1  	((struct MSC *)(APB+0x22000))
#define UART0	((struct UART *)(APB+0x30000))
#define UART1	((struct UART *)(APB+0x31000))
#define UART2	((struct UART *)(APB+0x32000))
#define I2C	((struct I2C *)(APB+0x42000))
#define SSI0	((struct SSI *)(APB+0x43000))
#define SSI1	((struct SSI *)(APB+0x45000))
#define SADC	((struct SADC *)(APB+0x70000))
#define OWI 	((struct OWI  *)(APB+0x72000))
#define TSSI	((struct TSSI *)(APB+0x73000))

#define EMC	((struct EMC *)(AHB0+0x10000))
#define DMAC	((struct DMAC *)(AHB0+0x20000))
#define UDC	((struct UDC *)(AHB0+0x40000))
#define LCDC	((struct LCDC *)(AHB0+0x50000))
#define CIM	((struct CIM *)(AHB0+0x60000))
#define IPU	((struct IPU *)(AHB0+0x80000))

#define MC	((struct MC *)(AHB1+0x90000))
#define ME	((struct ME *)(AHB1+0xa0000))
#define DEBLK	((struct DEBLK *)(AHB1+0xb0000))
#define IDCT	((struct IDCT *)(AHB1+0xc0000))
#define BCH	((struct BCH *)(AHB1+0xd0000))
#endif
