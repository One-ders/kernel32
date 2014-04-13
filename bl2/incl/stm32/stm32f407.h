
typedef int IRQn_Type;
#define __NVIC_PRIO_BITS 4

#define SysTick_IRQn		-1
#define PendSV_IRQn		-2
#define SVCall_IRQn		-5

#define TIM1_UP_TIM10_IRQn	25

#define USART1_IRQn		37
#define USART2_IRQn		38
#define USART3_IRQn		39



#define SystemCoreClock 168000000

#include <stm32/core_cm4.h>

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


