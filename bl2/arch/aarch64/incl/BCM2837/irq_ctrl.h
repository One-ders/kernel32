

#define SYS_TIMER1_MATCH	0x01
#define SYS_TIMER3_MATCH	0x03
#define USB_CTRL		0x09
#define AUX			0x1D
#define I2CSPI			0x2B
#define PWA0			0x2D
#define PWA1			0x2E
#define SMI			0x30
#define GPIO_INT0		0x31
#define GPIO_INT1		0x32
#define GPIO_INT2		0x33
#define GPIO_INT3		0x34
#define I2C			0x35
#define SPI			0x36
#define PCM			0x37
#define UART			0x39

struct IRQ_CTRL {
	volatile unsigned int pending_basic;
	volatile unsigned int pending_1;
	volatile unsigned int pending_2;
	volatile unsigned int FIQ_ctrl;
	volatile unsigned int enable_1;
	volatile unsigned int enable_2;
	volatile unsigned int enable_basic;
	volatile unsigned int disable_1;
	volatile unsigned int disable_2;
	volatile unsigned int disable_basic;
};
