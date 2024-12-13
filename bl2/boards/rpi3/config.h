

#define SYS_CLOCK	250000000

#define MAX_TASKS	256
#define TQ_SIZE         1024

#define USB_TX_BSIZE    128   // Lower Tx crashes system during boot if all usb
#define USB_RX_BSIZE	512

#define USART_TX_BSIZE	1024
#define USART_RX_BSIZE	16

/* define board */
#define RPI3
#define _M_X64

#define CONFIG_BAUDRATE	115200

#define SDRAM_SIZE	1024*1024*1024
#define BYTES_PER_LONG	8


/* Uart */
#define USE_UART	3
#define UART_TX_PIN	GPIO_PIN(GPIO_PC,10)
#define UART_RX_PIN	GPIO_PIN(GPIO_PC,11)

/* Usb */
#define USB_VBUS	GPIO_PIN(GPIO_PA,9)
#define USB_ID		GPIO_PIN(GPIO_PA,10)
#define USB_DM		GPIO_PIN(GPIO_PA,11)
#define USB_DP		GPIO_PIN(GPIO_PA,12)
#if 0
#define USB_PO		GPIO_PIN(GPIO_PC,0)
#define USB_OC		GPIO_PIN(GPIO_PD,5)
#endif

// define Vendor and product to pulse eight for backward comp.
// the linux ACM driver will add an echo to what it thinks is
// an extern monitor :(
//#define USB_VENDOR	0x48,0x25
//#define USB_PRODUCT	0x01,0x10

#ifndef CFG_CONSOLE_DEV
#define CFG_CONSOLE_DEV		"uart0"
//#define CFG_CONSOLE_DEV	"usb_serial0"
#endif
#ifndef USER_CONSOLE_DEV
#define USER_CONSOLE_DEV	"uart0"
//#define USER_CONSOLE_DEV	"usb_serial0"
#endif

#define CFG_ROOT_FS_DEV		"nand2"

/* Macros defined by LED drv.
 * User leds */
#define USER_LEDS	4
#define LED_PORT	GPIO_PD
#define LED_PORT_PINS 	(13 | (12<<4) | (14<<8) | (15<<12))
#define LED_AMBER 	1
#define LED_GREEN	2
#define LED_RED		4
#define LED_BLUE	8


#if 0
/* Cec */
#define CEC_PIN         GPIO_PIN(GPIO_PC,4)

/* Sony A1 */
// previous use of PB0 changed, because of burned gpio pin
#define A1_PIN          GPIO_PIN(GPIO_PC,5)

// OBD1 GM ALDL
#define OBD160_PIN          GPIO_PIN(GPIO_PC,5)

/* Timer 1 Ch1 & Ch 2, AF1 */
#define TIM1_CH1_PIN	GPIO_PIN(GPIO_PE,9)
#define TIM1_CH2_PIN	GPIO_PIN(GPIO_PE,11)

/* Timer 8 ch1, AF3 */
#define TIM8_CH1_PIN	GPIO_PIN(GPIO_PC,6)
#endif
