//#include <regdef.h>
//#include <frame.h>
#include <irq.h>
#include <devices.h>
#include <io.h>
#include <sys.h>

int ffs(int i);

struct IH_DATA {
	IRQ_HANDLER handler;
	void		*h_data;
};

static struct IH_DATA handlers[64];

static unsigned int installed_mask;

static const unsigned int p1vmask=0x2000020a;
static const unsigned int p2vmask=0x02ff6800;

void init_irq(void) {
}

int install_irq_handler(int irq_num,
			IRQ_HANDLER irq_handler,
			void *handler_data) {
	struct IRQ_CTRL *irq_ctrl=IRQ_CTRL_BASE;

	if (irq_num<32) {
		if (!((1<<irq_num)&p1vmask)) {
			sys_printf("someone installing invalid irq handler\n");
			return -1;
		}
	} else if (irq_num<64) {
		if (!((1<<(irq_num-32))&p2vmask)) {
			sys_printf("someone installing invalid irq handler\n");
			return -1;
		}
	} else {
		sys_printf("someone installing invalid irq handler\n");
		return -1;
	}

	if (irq_handler) {
		handlers[irq_num].handler=irq_handler;
		handlers[irq_num].h_data=handler_data;
		if (irq_num<32) {
			irq_ctrl->enable_1|=(1<<irq_num);
		} else {
			irq_ctrl->enable_2|=(1<<(irq_num-32));
		}
	} else {
		if (irq_num<32) {
			irq_ctrl->disable_1|=(1<<irq_num);
		} else {
			irq_ctrl->disable_2|=(1<<(irq_num-32));
		}
		handlers[irq_num].handler=0;
	}
	return 0;
}


void irq_dispatch(void *sp) {
	struct IRQ_CTRL *irq_ctrl=IRQ_CTRL_BASE;
	struct fullframe *ff=(struct fullframe *)sp;
	unsigned int p1=irq_ctrl->pending_1&p1vmask;
	unsigned int p2=irq_ctrl->pending_2&p2vmask;
	unsigned int pos;

#if 0
	if (irq_ctrl->pending_basic) {
		sys_printf("pending basic=%x\n",irq_ctrl->pending_basic);
	}
#endif
	while(pos=ffs(p1)) {
		unsigned int irq=pos-1;
		p1&=~(1<<(pos-1));
		if (handlers[irq].handler) {
			handlers[irq].handler(irq,handlers[irq].h_data);
		} else {
			sys_printf("unhandled irq: %d\n",irq);
		}
	}
	while(pos=ffs(p2)) {
		unsigned int irq=pos-1+32;
		p2&=~(1<<(pos-1));
		if (handlers[irq].handler) {
			handlers[irq].handler(irq,handlers[irq].h_data);
		} else {
			sys_printf("unhandled irq: %d\n",irq);
		}
	}
	disable_interrupts();
}

void wait_irq(void) {
	asm volatile ("wfe\t\n\t"
			: : );
}

