
extern unsigned int irq_lev;

#define I2C_IRQ		1
#define EMC_IRQ		2
#define UHC_IRQ		3
#define UART0_IRQ	9
#define SADC_IRQ	12
#define MSC_IRQ		14
#define RTC_IRQ		15
#define SSI_IRQ		16
#define CIM_IRQ		17
#define AIC_IRQ		18
#define DMA_IRQ		20
#define TCU2_IRQ	21
#define TCU1_IRQ	22
#define TCU0_IRQ	23
#define UDC_IRQ		24
#define GPIO3_IRQ	25
#define GPIO2_IRQ	26
#define	GPIO1_IRQ	27
#define GPIO0_IRQ	28
#define IPU_IRQ		29
#define LCD_IRQ		30

#define VALID_IRQS	0x7ff7d20e

typedef int (*IRQ_HANDLER)(int irq_num, void *hdata);

int install_irq_handler(int irq_num, IRQ_HANDLER irq_handler, void *handler_data);
