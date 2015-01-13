

#define SYS_CLOCK	168000000

#define SDRAM_SIZE	192*1024
#define BYTES_PER_LONG	4

#define PAGE_SIZE	4096


/* Usart */
#define USE_USART	3
#define USART_TX_PIN	GPIO_PIN(PC,10)
#define USART_RX_PIN	GPIO_PIN(PC,11)


/* User leds */
#define USER_LEDS	4
#define LED_PINS 	(GPIO_PIN(PD,13) | (GPIO_PIN(PD,12)<<8 | (GPIO_PIN(PD,14)<<16) | (GPIO_PIN(PD,15)<<24))
#define LED_AMBER 	1
#define LED_GREEN	2
#define LED_RED		4
#define LED_BLUE	8

/* Cec */
#define CEC_PIN         GPIO_PIN(PC,4);

/* Sony A1 */
#define A1_PIN          GPIO_PIN(PB,0);

