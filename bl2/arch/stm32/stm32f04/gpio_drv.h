/* $FrameWorks: , v1.1 2014/04/07 21:44:00 anders Exp $ */

/*
 * Copyright (c) 2014, Anders Franzen.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @(#)gpio_drv.h
 */
#define GPIO_DRV "gpio_drv"

#define GPIO_BIND_PIN			1
#define GPIO_GET_BOUND_PIN		2
#define PA				0x00
#define PB				0x10
#define PC				0x20
#define PD				0x30
#define PE				0x40
#define PF				0x50
#define PG				0x60
#define PH				0x70
#define PI				0x80

#define GPIO_PIN(a,b)			(a|b)

#define GPIO_SET_FLAGS			3
#define GPIO_CLR_FLAGS			4
#define GPIO_GET_FLAGS			5

#define GPIO_DIR(a,b)			((a&GPIO_DIR_MASK)|b)
#define GPIO_DIR_MASK			0x3
#define GPIO_INPUT 			0x00000001
#define GPIO_OUTPUT			0x00000002
#define GPIO_BUSPIN			0x00000003

#define GPIO_DRIVE(a,b)			((a&~GPIO_DRIVE_MASK)|(b<<GPIO_DRIVE_SHIFT))
#define GPIO_DRIVE_MASK			0xc
#define GPIO_DRIVE_SHIFT		2
#define GPIO_FLOAT			0
#define GPIO_PUSHPULL			0
#define GPIO_PULLUP			1
#define GPIO_PULLDOWN			2
#define GPIO_OPENDRAIN			3

#define GPIO_SPEED(a,b)			((a&~GPIO_SPEED_MASK)|(b<<GPIO_SPEED_SHIFT))
#define GPIO_SPEED_MASK			0x30
#define GPIO_SPEED_SHIFT		4
#define GPIO_SPEED_SLOW			0
#define GPIO_SPEED_MEDIUM		1
#define GPIO_SPEED_FAST			2
#define GPIO_SPPED_HIGH			3
#define GPIO_SPEED_2MHZ			GPIO_SPEED_SLOW
#define GPIO_SPEED_25MHZ		GPIO_SPEED_MEDIUM
#define GPIO_SPEED_50MHZ		GPIO_SPEED_FAST
#define GPIO_SPEED_100MHZ		GPIO_SPEED_HIGH

#define GPIO_IRQ			0x40
#define GPIO_IRQ_ENABLE(a)		(a|GPIO_IRQ)
#define GPIO_IRQ_DISABLE(a)		(a&~GPIO_IRQ)

#define	GPIO_SENSE_PIN			6
#define GPIO_SET_PIN			7
#define GPIO_SINK_PIN			8
#define GPIO_RELEASE_PIN		9

/* free GPIO IO pins
 *	PA8,PA15.PB0,PB1,PB2,PB4,PB5,PB7,
 *	PB8,PB11,PB12,PB13,PB14.PB15,PC1,
 *	PC2,PC4,PC5,PC6,PC8,PC9,PC11,PC13,
 *	PC14,PC15,PD0,PD1,PD2,PD3,PD6,PD7,
 *	PD8,PD9,PD10,PD11,PE2,PE4,PE5,PE6
 *	PE7,PE8,PE9,PE10,PE11,PE12,PE13,PE14,
 *	PE15
 */

/* Pins connected to external functions
 *
 * CS43L22, audio dac, Speaker driver
 * 
 * 	PA4	LRCK/AIN 1.x
 * 	PB6	SCL
 * 	PB9	SDA
 * 	PC7	MCLK
 * 	PC10	SCLK
 * 	PC12	SDIN
 * 	PD4	RESET
 *
 * MP45DT02, ST_MEMS audio sensor microphone
 *
 * 	PB10	CLK
 * 	PC3	DOUT/AIN 4x
 *
 * LIS302DL Motion sensor
 *
 * 	PA5	SCL/SPC
 * 	PA6	SDDO
 * 	PA7	SDA/SDI/SDO
 * 	PE0	INT1
 *	PE1	INT2
 * 	PE3	CS_I2C/SPI
 *
 * PushButton (Blue)
 * 	PA0
 *
 *  LEDs
 *  	PD5	RED
 *  	PD12	GREEN
 *  	PD13	ORANGE
 *  	PD14	RED
 *  	PD15	BLUE
 *
 *  SWD
 *  	PA13	SWDIO
 *	PA14	SWCLK
 *	PB3	SWO
 *
 * USB
 * 	PA9	VBUA
 * 	PA10	ID
 * 	PA11	DM
 * 	PA12	DP
 * 	PC0	PowerOn
 * 	PD5	OverCurrent
 *
 * OSC
 * 	PC14	OSC32_IN
 * 	PC15	OSC32_OUT
 * 	PH0	OSC_IN
 * 	PH1	OSC_OUT
 *
 * CN5 (USB)
 * 	PA9	1
 * 	PA10	4
 * 	PA11	2
 * 	PA12	3
 */
 


/* Alternate PIN map/assignements, on Discovery 
 * ETH_MII
 * 	PA0,PA1,PA2,PA3,PA7,PB0,PB5,PB8,PB10,PB11,PB12,PB13,PC1,PC2,PC3,
 * 	PC4,PC5,PE2.
 *
 * USART 1
 *	PA8,PA9,PA10,PA11,PA12,PB6,PB7
 *
 * USART_2
 * 	PA0,PA1,PA2,PA3,PA4,PD3,PD4,PD5,PD6,PD7.
 *
 * USART 4
 * 	PA0,PA1
 *
 * TIM1
 * 	PA6,PA7,PA8,PA9,PA10,PA11,PA12,PB0,PB1,PB12,PB13,PB14,PB15,PE7,PE8,
 * 	PE9,PE10,PE11,PE12,PE13,PE14,PE15
 *
 * TIM 2
 * 	PA0,PA1,PA2,PA3,PA5,PA15,PB3,PB10,PB11
 *
 * TIM 3
 * 	PA6,PA7,PB0,PB1,PB4,PB5,PC6,PC7,PC8,PC9,PD2
 *
 * TIM 4
 *	PB6,PB7,PB8,PB9,PD12,PD13,PD14,PD15,PE0
 *
 * TIM 5
 * 	PA0,PA1,PA2,PA3
 *
 * TIM 8
 * 	PA0,PA5,PA6,PA7,PB0,PB1,PB14,PB15,PC6,PC7,PC8,PC9
 *
 * TIM 9
 * 	PA2,PA3,PE5,PE6
 *
 * TIM10
 *	PB8
 *
 * TIM11
 *	PB9
 *
 * TIM 13
 * 	PA6
 *
 * TIM 14
 *	PA7
 *
 * ADC 123
 * 	PA0,PA1,PA2,PA3,PA4,PA5,PA6,PA7,PB0,PB1,PC0,PC1,PC2,
 * 	PC3,PC4,PC5
 *
 * WAKEUP
 * 	PA0
 * 	
 * OTG_HS
 * 	PA3,PA4,PA5,PB0,PB1,PB5,PB10,PB11,PB12,PB13,PB14,PB15,
 * 	PC0,PC2,PC3
 *
 * SPI 1
 * 	(PA4,PA5,PA6,PA7),(PA15,PB3,PB4,PB5)
 *
 * SPI 2
 * 	PB9,PB10,PB12,PB13.PB14,PB15,PC2,PC3
 *
 * SPI 3
 * 	PA4,PA15,PB3,PB4,PB5,PC10,PC11,PC12
 *
 * DCMI
 * 	PA4,PA6,PA9,PA10,PB5,PB6,PB7,PB8,PB9,PC6,PC7,PC8,
 * 	PC9,PC10,PC11,PC12,PD2,PE0,PE1,PE4,PE5,PE6
 *
 * I2S 3
 * 	PA4,PA15,PB3,PB4,PB5,PC7,PC10,PC11,PC12
 *
 * DAC 1
 * 	PA4
 *
 * DAC 2
 * 	PA5
 *
 * I2C 3
 *	PA8,PA9,PC9
 *
 * OTG_FS
 *	PA8,PA9,PA10,PA11,PA12,PB6,PB8,PB9
 *
 * MCO1
 *	PA8
 *
 * MCO2
 *	PC9
 *
 * CAN 1
 *	PA11,PA12,PB8,PB9,PD0,PD1
 *
 * JT
 *	PA13,PA14,PA15,PB3
 *
 * I2C 1
 *	PB5,PB6,PB7,PB8
 *
 * CAN 2
 *	PB5,PB6,PB12,PB13
 *
 * FSMC
 *	PB7,PD0,PD1,PD3,PD4,PD5,PD6,PD7,PD8,PD9,PD10,PD11,PD12,
 *	PD13,PD14,PD15,PE0,PE1,PE2,PE3,PE4,PE5,PE6,PE7,PE8,
 *	PE9,PE10,PE11,PE12,PE13,PE14,PE15
 *
 * SDIO
 *	PB8,PB9,PC6,PC7,PC8,PC9,PC10,PC11,PC12,PD2
 *
 * I2S 2
 *	PB9,PB10,PB12,PB13,PB14,PB15,PC2,PC3,PC6
 *
 * I2C 2
 *	PB10,PB11,PB12
 *	
 */
