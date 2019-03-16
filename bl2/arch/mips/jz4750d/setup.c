#include "sys.h"
#include "irq.h"

#include "clocks.h"
#include "jz4750_clocks.h"

#include <devices.h>
#include <fb.h>

#include <string.h>
#include <malloc.h>

jz_clocks_t jz_clocks;

static void sysclocks_setup(void) {
#ifndef CONFIG_MIPS_JZ_EMURUS /* FPGA */
	jz_clocks.cclk = __cpm_get_cclk();
	jz_clocks.hclk = __cpm_get_hclk();
	jz_clocks.pclk = __cpm_get_pclk();
	jz_clocks.mclk = __cpm_get_mclk();
	jz_clocks.h1clk = __cpm_get_h1clk();
	jz_clocks.pixclk = __cpm_get_pixclk();
	jz_clocks.i2sclk = __cpm_get_i2sclk();
	jz_clocks.usbclk = __cpm_get_usbclk();
	jz_clocks.mscclk = __cpm_get_mscclk(0);
	jz_clocks.extalclk = __cpm_get_extalclk();
	jz_clocks.rtcclk = __cpm_get_rtcclk();
#else
#define FPGACLK 8000000

	jz_clocks.cclk = FPGACLK;
	jz_clocks.hclk = FPGACLK;
	jz_clocks.pclk = FPGACLK;
	jz_clocks.mclk = FPGACLK;
	jz_clocks.h1clk = FPGACLK;
	jz_clocks.pixclk = FPGACLK;
	jz_clocks.i2sclk = FPGACLK;
	jz_clocks.usbclk = FPGACLK;
	jz_clocks.mscclk = FPGACLK;
	jz_clocks.extalclk = FPGACLK;
	jz_clocks.rtcclk = FPGACLK;
#endif

	sys_printf("CPU clock: %dMHz, System clock: %dMHz, Peripheral clock: %dMHz, Memory clock: %dMHz\n",
		(jz_clocks.cclk + 500000) / 1000000,
		(jz_clocks.hclk + 500000) / 1000000,
		(jz_clocks.pclk + 500000) / 1000000,
		(jz_clocks.mclk + 500000) / 1000000);
}

static void soc_cpm_setup(void) {
        /* Start all module clocks
         */
	__cpm_start_all();

	/* Enable CKO to external memory */
	__cpm_enable_cko();

	/* CPU enters IDLE mode when executing 'wait' instruction */
	__cpm_idle_mode();

	/* Setup system clocks */
	sysclocks_setup();
}


static void soc_harb_setup(void) {
//      __harb_set_priority(0x00);  /* CIM>LCD>DMA>ETH>PCI>USB>CBB */
//      __harb_set_priority(0x03);  /* LCD>CIM>DMA>ETH>PCI>USB>CBB */
//      __harb_set_priority(0x0a);  /* ETH>LCD>CIM>DMA>PCI>USB>CBB */
}

static void soc_emc_setup(void) {
}


static void soc_dmac_setup(void) {
	__dmac_enable_module(0);
	__dmac_enable_module(1);
}

static void jz_soc_setup(void) {
	soc_cpm_setup();
	soc_harb_setup();
	soc_emc_setup();
	soc_dmac_setup();
}

static void jz_serial_setup(void) {
#ifdef CONFIG_SERIAL_8250
	struct uart_port s;
	REG8(UART0_FCR) |= UARTFCR_UUE; /* enable UART module */
	memset(&s, 0, sizeof(s));
	s.flags = UPF_BOOT_AUTOCONF | UPF_SKIP_TEST;
	s.iotype = SERIAL_IO_MEM;
	s.regshift = 2;
	s.uartclk = jz_clocks.extalclk ;

	s.line = 0;
	s.membase = (u8 *)UART0_BASE;
	s.irq = IRQ_UART0;
	if (early_serial_setup(&s) != 0) {
		sys_printf("Serial ttyS0 setup failed!\n");
	}

	s.line = 1;
	s.membase = (u8 *)UART1_BASE;
	s.irq = IRQ_UART1;
	if (early_serial_setup(&s) != 0) {
		sys_printf("Serial ttyS1 setup failed!\n");
	}

	s.line = 2;
	s.membase = (u8 *)UART2_BASE;
	s.irq = IRQ_UART2;

	if (early_serial_setup(&s) != 0) {
		sys_printf("Serial ttyS2 setup failed!\n");
	}
/*
        s.line = 3;
        s.membase = (u8 *)UART3_BASE;
        s.irq = IRQ_UART3;
        if (early_serial_setup(&s) != 0) {
                printk(KERN_ERR "Serial ttyS3 setup failed!\n");
        }
*/
#endif
}

void plat_mem_setup(void) {
	char *argptr;

//	argptr = prom_getcmdline();

	/* IO/MEM resources. Which will be the addtion value in `inX' and
	 * `outX' macros defined in asm/io.h */
//	set_io_port_base(0);
//	ioport_resource.start   = 0x00000000;
//	ioport_resource.end     = 0xffffffff;
//	iomem_resource.start    = 0x00000000;
//	iomem_resource.end      = 0xffffffff;

//	_machine_restart = jz_restart;
//	_machine_halt = jz_halt;
//	pm_power_off = jz_power_off;

	jz_soc_setup();
	jz_serial_setup();
//	jz_board_setup();
}

