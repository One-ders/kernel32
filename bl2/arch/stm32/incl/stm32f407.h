
typedef int IRQn_Type;
#define __NVIC_PRIO_BITS 4

#define SysTick_IRQn		-1
#define PendSV_IRQn		-2
#define SVCall_IRQn		-5

#define EXTI0_IRQn		6
#define EXTI1_IRQn		7
#define EXTI2_IRQn		8
#define EXTI3_IRQn		9
#define EXTI4_IRQn		10

#define EXTI9_5_IRQn		23


#define TIM1_UP_TIM10_IRQn	25

#define USART1_IRQn		37
#define USART2_IRQn		38
#define USART3_IRQn		39

#define EXTI15_10_IRQn		40

#define OTG_FS_IRQn		67
#define OTG_HS_IRQn		77

extern unsigned int SystemCoreClock;

#include <core_cm4.h>

#define set_reg(a,b) (*((unsigned int *)a)=(b))

#define AIRC		0xE000ED0C   /* Application Interrupt and Reset Control Register */
#define VECTKEY		0x05fa0000
#define SYSRESETREQ	0x00000004
		


#if 0
#define USART1_BASE	0x40011000   /* towards 0x400113ff  BUS APB2 */
#define USART2_BASE	0x40004400   /* towards 0x400047ff  BUS APB1 */
#define USART3_BASE	0x40004700   /* towards 0x40004bff  BUS APB1 */
#define UART4_BASE	0x40004c00   /* towards 0x40004fff  BUS APB1 */
#define UART5_BASE	0x40005000   /* towards 0x400053ff  BUS APB1 */
#define USART6_BASE	0x40011400   /* towards 0x400117ff  BUS APB2 */
#define UART7_BASE	0x40007800   /* towards 0x40007bff  BUS APB1 */
#define UART8_BASE	0x40007c00   /* towards 0x40007fff  BUS APB1 */
#endif



/* U(S)ARTS */


