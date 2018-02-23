
#include <busses.h>
#include <usart_reg.h>
#include <rcc_reg.h>
#include <gpio_reg.h>
#include <timer_reg.h>
#include <power_reg.h>
#include <flash_reg.h>
#include <iwdg_reg.h>
#include <usb_otg_reg.h>
#include <fmc_reg.h>


#define RCC	((struct RCC *)(AHB1+0x3800))

#define GPIOA	((struct GPIO *)(AHB1+0x0000))
#define GPIOB	((struct GPIO *)(AHB1+0x0400))
#define GPIOC	((struct GPIO *)(AHB1+0x0800))
#define GPIOD	((struct GPIO *)(AHB1+0x0C00))
#define GPIOE	((struct GPIO *)(AHB1+0x1000))
#define GPIOF	((struct GPIO *)(AHB1+0x1400))
#define GPIOG	((struct GPIO *)(AHB1+0x1800))
#define GPIOH	((struct GPIO *)(AHB1+0x1C00))
#define GPIOI	((struct GPIO *)(AHB1+0x2000))
#define GPIOJ	((struct GPIO *)(AHB1+0x2400))
#define GPIOK	((struct GPIO *)(AHB1+0x2800))

#define USB_OTG_HS_ADDR	((struct usb_otg_regs *)(AHB1+0x20000))
#define USB_OTG_FS_ADDR	((struct usb_otg_regs *)(AHB2))

#define USART1	((struct USART *)(APB2+0x1000))
#define USART2	((struct USART *)(APB1+0x4400))
#define USART3	((struct USART *)(APB1+0x4800))

#define UART4	((struct USART *)(APB1+0x4c00))
#define UART5	((struct USART *)(APB1+0x5000))

#define USART6	((struct USART *)(APB2+0x1400))

#define UART7	((struct USART *)(APB1+0x7800))
#define UART8	((struct USART *)(APB1+0x7c00))


#define TIM1	((struct TIMER *)(APB2+0x0000))
#define TIM2	((struct TIMER *)(APB1+0x0000))
#define TIM3	((struct TIMER *)(APB1+0x0400))
#define TIM4	((struct TIMER *)(APB1+0x0800))
#define TIM5	((struct TIMER *)(APB1+0x0c00))
#define TIM6	((struct TIMER *)(APB1+0x1000))
#define TIM7	((struct TIMER *)(APB1+0x1400))
#define TIM8	((struct TIMER *)(APB2+0x0400))
#define TIM9	((struct TIMER *)(APB2+0x4000))
#define TIM10	((struct TIMER *)(APB2+0x4400))
#define TIM11	((struct TIMER *)(APB2+0x4800))
#define TIM12	((struct TIMER *)(APB1+0x1800))
#define TIM13	((struct TIMER *)(APB1+0x1c00))
#define TIM14	((struct TIMER *)(APB1+0x2000))

#define IWDG	((struct IWDG *)(APB1+0x3000))
#define PWR	((struct POWER *)(APB1+0x7000))

#define FLASH	((struct FLASH *)(AHB1+0x3c00))

#define FMC	((struct FMC *)(AHB3+0x0000))
