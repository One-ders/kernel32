


struct GPIO_DEV {
#define	GPIO_PIN_INPUT	0x0
#define GPIO_PIN_OUTPUT	0x1
#define GPIO_PIN_ALT0	0x4
#define GPIO_PIN_ALT1	0x5
#define GPIO_PIN_ALT2	0x6
#define GPIO_PIN_ALT3	0x7
#define GPIO_PIN_ALT4	0x3
#define GPIO_PIN_ALT5	0x2
#define GPIO_PIN_MASK	0x7

#define PIN_X(X,FNC)	(FNC<<(X*3))
	// pin  0-9, 10-19, 20-29, 30-39, 40-49, 50-53
	volatile unsigned int gpfsel[6];
	unsigned int pad0;
	volatile unsigned int gpset[2];  // pin 0-31, 32-53
	unsigned int pad1;
	volatile unsigned int gpclr[2];  // pin 0-31, 32-53
	unsigned int pad2;
	volatile unsigned int gplev[2];  // pin 0-31, 32-53
	unsigned int pad3;
	volatile unsigned int gpeds[2];  // pin 0-31, 32-53
	unsigned int pad4;
	volatile unsigned int gpren[2];  // pin 0-31, 32-53
	unsigned int pad5;
	volatile unsigned int gpfen[2];  // pin 0-31, 32-53
	unsigned int pad6;
	volatile unsigned int gphen[2];  // pin 0-31, 32-53
	unsigned int pad7;
	volatile unsigned int gplen[2];  // pin 0-31, 32-53
	unsigned int pad8;
	volatile unsigned int gparen[2]; // pin 0-31, 32-53
	unsigned int pad9;
	volatile unsigned int gpafen[2]; // pin 0-31, 32-53
	unsigned int pad10;
#define GPPUD_PULL_DISABLE	0
#define GPPUD_PULL_DOWN		1
#define GPPUD_PULL_UP		2
	volatile unsigned int gppud;
	volatile unsigned int gppudclk[2]; // pin 0-31, 32-53
	unsigned int pad11;
	volatile unsigned int test;
};
