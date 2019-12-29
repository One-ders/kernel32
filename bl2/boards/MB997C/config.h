

#define SYS_CLOCK	168000000

/* define board */
#define MB997C

#define SDRAM_SIZE	192*1024
#define BYTES_PER_LONG	4


/* Usart */
#define USE_USART	3
#define USART_TX_PIN	GPIO_PIN(PC,10)
#define USART_RX_PIN	GPIO_PIN(PC,11)

/* Usb */
#define USB_VBUS	GPIO_PIN(PA,9)
#define USB_ID		GPIO_PIN(PA,10)
#define USB_DM		GPIO_PIN(PA,11)
#define USB_DP		GPIO_PIN(PA,12)
#if 0
#define USB_PO		GPIO_PIN(PC,0)
#define USB_OC		GPIO_PIN(PD,5)
#endif


/* User leds */
#define USER_LEDS	4
#define LED_PINS 	(GPIO_PIN(PD,13) | (GPIO_PIN(PD,12)<<8) | (GPIO_PIN(PD,14)<<16) | (GPIO_PIN(PD,15)<<24))
#define LED_AMBER 	1
#define LED_GREEN	2
#define LED_RED		4
#define LED_BLUE	8

/* Cec */
//#define CEC_PIN         GPIO_PIN(PC,4);
#define CEC_PIN         GPIO_PIN(PC,5);

//#define WITH_SONY_A1
/* Sony A1 */
// previous use of PB0 changed, because of burned gpio pin
//#define A1_PIN          GPIO_PIN(PC,5);

