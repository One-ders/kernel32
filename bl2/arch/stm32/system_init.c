
#include <sys.h>
#include <stm32f407.h>
#include <devices.h>

void _exit(int status) {
}

#define PLL_M	8
#define PLL_N	336
#define PLL_P	2
#define PLL_q	7

#define HSE_STARTUP_TIMEOUT	0x0500

unsigned int  SystemCoreClock = 168000000;

void system_init(void) {
	volatile unsigned int startup_cnt=0;
	volatile unsigned int HSE_status=0;

	/* Reset RCC clocks */
	RCC->CR |= RCC_CR_HSION;

	RCC->CFGR = 0;

	RCC->CR &= 
		~(RCC_CR_HSEON |
		 RCC_CR_CSSON |
		 RCC_CR_PLLON);

	RCC->PLLCFGR= 0x20000000 |
			(RCC_PLLCFGR_PLLQ2 |
			 (0xc0 << RCC_PLLCFGR_PLLN_SHIFT) |
			RCC_PLLCFGR_PLLM4);

	RCC->CR &= ~RCC_CR_HSEBYP;

	RCC->CIR = 0;

	/* Config clocks */
	RCC->CR |= RCC_CR_HSEON;
	do {
		HSE_status=RCC->CR&RCC_CR_HSERDY;
		startup_cnt++;
	} while ((!HSE_status) && (startup_cnt!=HSE_STARTUP_TIMEOUT));
	
	if (RCC->CR&RCC_CR_HSERDY) {
		HSE_status=1;
	} else {
		HSE_status=0;
	}

	if (!HSE_status) {
		/* Failed to start external clock */
		return;
		ASSERT(0);
	}

	/* Enable high perf mode, system clock to 168 MHz */
	RCC->APB1ENR |= RCC_APB1ENR_PWREN;
	PWR->CR |= (1<<PWR_CR_VOS_SHIFT);

	/* HCLK = SYSCLK/1 */
//	RCC->CFGR |= (1<<RCC_CFGR_HPRE_SHIFT);

	/* PCLK2 = HCLK/2 */
	RCC->CFGR |= (4<<RCC_CFGR_PPRE2_SHIFT);

	/* PCLK1 = HCLK/4 */
	RCC->CFGR |= (5<<RCC_CFGR_PPRE1_SHIFT);

	/* Configure the main PLL */
	RCC->PLLCFGR = RCC_PLLCFGR_PLLM3 |
			(PLL_N<<RCC_PLLCFGR_PLLN_SHIFT) |
			RCC_PLLCFGR_PLLSRC |
			RCC_PLLCFGR_PLLQ0 |
			RCC_PLLCFGR_PLLQ1 |
			RCC_PLLCFGR_PLLQ2;

	/* Enable the main PLL */
	RCC->CR |= RCC_CR_PLLON;

	/* Wait for main PLL ready */
	while(!(RCC->CR&RCC_CR_PLLRDY));

	/* Configure flash prefetch, Icache, dcache and wait state */
	FLASH->ACR = FLASH_ACR_ICEN| FLASH_ACR_DCEN | (FLASH_ACR_LATENCY_MASK&0x5);

	/* Select main PLL as system clock source */
	RCC->CFGR &= ~RCC_CFGR_SW0;
	RCC->CFGR |= RCC_CFGR_SW1;

	while((RCC->CFGR&RCC_CFGR_SWS_MASK)!=(0x2<<RCC_CFGR_SWS_SHIFT));
}

void init_irq(void) {
//      NVIC_SetPriority(SVCall_IRQn,0xe);
	NVIC_SetPriority(SVCall_IRQn,0xf);  /* try to share level with pendsv */
	NVIC_SetPriority(SysTick_IRQn,0xd);  /* preemptive tics... */
}

void config_sys_tic(unsigned int ms) {
	unsigned int perSec=1000/ms;
	SysTick_Config(SystemCoreClock/perSec);
}

void board_reboot(void) {
	NVIC_SystemReset();
}
