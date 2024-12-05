
extern unsigned int irq_lev;

#define VALID_IRQS      0xffffffff

typedef int (*IRQ_HANDLER)(int irq_num, void *hdata);

int install_irq_handler(int irq_num, IRQ_HANDLER irq_handler, void *handler_data);
