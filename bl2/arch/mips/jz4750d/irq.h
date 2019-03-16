
extern unsigned int irq_lev;

#define ETH_IRQ		0
#define SFT_IRQ		4
#define I2C_IRQ		5
#define RTC_IRQ		6
#define UART2_IRQ	7
#define UART1_IRQ	8
#define UART0_IRQ	9
#define AIC_IRQ		10
#define GPIO5_IRQ	11
#define GPIO4_IRQ	12
#define GPIO3_IRQ	13
#define GPIO2_IRQ	14
#define GPIO1_IRQ	15
#define GPIO0_IRQ	16
#define BCH_IRQ		17
#define SADC_IRQ	18
#define CIM_IRQ		19
#define TSSI_IRQ	20
#define TCU2_IRQ	21
#define TCU1_IRQ	22
#define TCU0_IRQ	23
#define MSC1_IRQ	24
#define MSC0_IRQ	25
#define SSI_IRQ		26
#define UDC_IRQ		27
#define DMA1_IRQ	28
#define DMA0_IRQ	29
#define IPU_IRQ		30
#define LCD_IRQ		31

#define VALID_IRQS      0xfffffff1

typedef int (*IRQ_HANDLER)(int irq_num, void *hdata);

int install_irq_handler(int irq_num, IRQ_HANDLER irq_handler, void *handler_data);
