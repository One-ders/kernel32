
#include <sys.h>
#include <string.h>
#include <stm32f407.h>
#include <devices.h>

void _exit(int status) {
}

struct task main_task = {
.name	=	"init_main",
.sp	=	(void *)(0x20020000),
.id	=	256,
.next	=	0,
.next2	=	0,
.state	=	1,
.prio_flags=	3,
.estack	=	(void *)(0x20020000-0x800)
};

extern int __usr_main(int argc, char **argv);
void *usr_init=__usr_main;

void build_free_page_list(void);
static unsigned long free_page_map[SDRAM_SIZE/(PAGE_SIZE*sizeof(unsigned long))];

extern unsigned long __bss_start__;
extern unsigned long __bss_end__;

#define SDRAM_START 0x20000000
#define PHYS2PAGE(a)    ((((unsigned long int)(a))&(SDRAM_START-1))>>12)
#define set_in_use(a,b) ((a)[((unsigned long int)(b))/32]&=~(1<<(((unsigned long int)(b))%32)))
#define set_free(a,b) ((a)[((unsigned long int)(b))/32]|=(1<<(((unsigned long int)(b))%32)))


void init_sys_arch(void) {
//	unsigned long int psize;
	build_free_page_list();
//	psize=(SDRAM_SIZE+SDRAM_START)-__bss_end__;
//	sys_printf("psize=%x\n",psize);
}

void build_free_page_list() {
	unsigned long int mptr=SDRAM_START;
	int i;

	for(i=0;i<(sizeof(free_page_map)/sizeof(unsigned long int));i++) {
		free_page_map[i]=~0;
	}

	while(mptr<((unsigned long int)&__bss_end__)) {
		set_in_use(free_page_map,PHYS2PAGE(mptr));
#if 0
		sys_printf("build_free_page_list: set %x non-free, map %x\n",mptr, free_page_map[0]);
		sys_printf("map index %x, mapmask %x\n",PHYS2PAGE(mptr)/32,
				~(1<<((PHYS2PAGE(mptr))%32)));
#endif
		mptr+=4096;
	}
}

void *get_page(void) {
	int i;
	for(i=0;i<(sizeof(free_page_map)/sizeof(unsigned long int));i++) {
		if (free_page_map[i]) {
			int j=ffsl(free_page_map[i]);
			if (!j) return 0;
			sys_printf("found free page att index %x, bit %x\n",
					i,j);
			j--;
			free_page_map[i]&=~(1<<j);
			return (void *)(SD_RAM_START + (i*32*4096) + (j*4096));
		}
	}
	return 0;
}

void put_page(void *p) {
	set_free(free_page_map,PHYS2PAGE(p));
}

#define PLL_M	8

#ifdef MB1075B
#define PLL_N	360
#else
#define PLL_N	336
#endif
#define PLL_P	2
#define PLL_q	7

#define HSE_STARTUP_TIMEOUT	0x0500

unsigned int  SystemCoreClock = SYS_CLOCK;

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

#ifdef MB1075B
	RCC->PLLCFGR= 0x20000000 |
			(RCC_PLLCFGR_PLLQ2 |
			 (0xc0 << RCC_PLLCFGR_PLLN_SHIFT) |
			RCC_PLLCFGR_PLLM4);
#else
	RCC->PLLCFGR= 0x20000000 |
			(RCC_PLLCFGR_PLLQ2 |
			 (0xc0 << RCC_PLLCFGR_PLLN_SHIFT) |
			RCC_PLLCFGR_PLLM4);
#endif

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

	/* Enable high perf mode, system clock to 168/180 MHz */
	RCC->APB1ENR |= RCC_APB1ENR_PWREN;
#ifdef MB1075B
	PWR->CR |= (3<<PWR_CR_VOS_SHIFT);
#else
	PWR->CR |= (1<<PWR_CR_VOS_SHIFT);
#endif

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

#ifdef MB1075B
	PWR->CR |= PWR_CR_ODEN;
	while(!(PWR->CSR&PWR_CSR_ODRDY));
	PWR->CR |= PWR_CR_ODSW;
	while(!(PWR->CSR&PWR_CSR_ODSWRDY));
#endif

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
	NVIC_SetPriority(SysTick_IRQn,0x1);  /* preemptive tics... */
	NVIC_SetPriority(TIM1_UP_TIM10_IRQn,0x0);
	NVIC_SetPriority(EXTI0_IRQn,0x2);
	NVIC_SetPriority(EXTI1_IRQn,0x2);
	NVIC_SetPriority(EXTI2_IRQn,0x2);
	NVIC_SetPriority(EXTI3_IRQn,0x2);
	NVIC_SetPriority(EXTI4_IRQn,0x2);
	NVIC_SetPriority(EXTI9_5_IRQn,0x2);
	NVIC_SetPriority(EXTI15_10_IRQn,0x2);
	NVIC_SetPriority(USART3_IRQn,0xe);
}

void config_sys_tic(unsigned int ms) {
	unsigned int perSec=1000/ms;
	SysTick_Config(SystemCoreClock/perSec);
}

void board_reboot(void) {
	NVIC_SystemReset();
}
