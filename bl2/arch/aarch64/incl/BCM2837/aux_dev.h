

struct AUX_DEV {
#define AUX_IRQ_UART	1
#define AUX_IRQ_SPI1	2
#define AUX_IRQ_SPI2	4
	volatile unsigned int irq;
#define AUX_ENABLES_UART	1
#define AUX_ENABLES_SPI1	2
#define AUX_ENABLES_SPI2	4
	volatile unsigned int enables;
	unsigned int pad1[14];
	// Mini uart
	// B-rate reg if  DLAB=1, else Tx or Rx
	volatile unsigned int mu_io_reg;
	// Hi B-rate reg if DLAB=1 else
#define AUX_MU_IER_REG_RX_IRQ_ENABLE 1
#define AUX_MU_IER_REG_TX_IRQ_ENABLE 2
	volatile unsigned int mu_ier_reg;
#define AUX_MU_IIR_REG_IRQ_PENDING	 0x01
#define AUX_MU_IIR_REG_TX_HOLD_REG_EMPTY 0x02
#define AUX_MU_IIR_REG_RX_HOLD_BYTE	 0x04
#define AUX_MU_IIR_REG_CLEAR_RX_FIFO	 0x02
#define AUX_MU_IIR_REG_CLEAR_TX_FIFO	 0x04
	volatile unsigned int mu_iir_reg;
#define AUX_MU_LCR_REG_DATA_SIZE_7	0x00
#define AUX_MU_LCR_REG_DATA_SIZE_8	0x03
#define AUX_MU_LCR_REG_BREAK		0x40
#define AUX_MU_LCR_REG_DLAB		0x80
	volatile unsigned int mu_lcr_reg;
#define AUX_MU_MCR_REG_RTS		0x02
	volatile unsigned int mu_mcr_reg;
#define AUX_MU_LSR_REG_DATA_READY	0x01
#define AUX_MU_LSR_REG_REC_OVER		0x02
#define AUX_MU_LSR_REG_TX_EMPTY		0x20
#define AUX_MU_LSR_REG_TX_IDLE		0x40
	volatile unsigned int mu_lsr_reg;
#define AUX_MU_MSR_REG_CTS		0x20
	volatile unsigned int mu_msr_reg;
	volatile unsigned int mu_scratch;
#define AUX_MU_CNTL_REG_RX_ENA		0x01
#define AUX_MU_CNTL_REG_TX_ENA		0x02
#define AUX_MU_CNTL_REG_RX_AUTOFLOW_ENA	0x04
#define AUX_MU_CNTL_REG_TX_AUTOFLOW_ENA 0x08
#define AUX_MU_CNTL_REG_RTS_AUTOFLOW_3  0x00
#define AUX_MU_CNTL_REG_RTS_AUTOFLOW_2  0x10
#define AUX_MU_CNTL_REG_RTS_AUTOFLOW_1	0x20
#define AUX_MU_CNTL_REG_RTS_AUTOFLOW_4  0x30
#define AUX_MU_CNTL_REG_RTS_ASSERT_LEV	0x40
#define AUX_MU_CNTL_REG_CTS_ASSERT_LEV  0x80
	volatile unsigned int mu_cntl_reg;
#define AUX_MU_STAT_REG_SYMBOL_AVAIL	0x01
#define AUX_MU_STAT_REG_TXSPACE_AVAIL	0x02
#define AUX_MU_STAT_REG_RX_IDLE		0x04
#define AUX_MU_STAT_REG_TX_IDLE		0x08
#define AUX_MU_STAT_REG_RX_OVER		0x10
#define AUX_MU_STAT_REG_TX_FULL		0x20
#define AUX_MU_STAT_REG_RTS_STAT	0x40
#define AUX_MU_STAT_REG_CTS_LINE	0x80
#define AUX_MU_STAT_REG_TX_FIFO_EMPTY  0x100
#define AUX_MU_STAT_REG_TX_DONE	       0x200
#define AUX_MU_STAT_REG_RX_LEV_MASK	0x000f0000
#define AUX_MU_STAT_REG_TX_LEV_MASK	0x0f000000
	volatile unsigned int mu_stat_reg;
#define AUX_MU_STAT_REG_BDRATE_MASK	0x0000ffff
	volatile unsigned int mu_baud_reg;
	unsigned int pad2[6];
	// SPI 1
	volatile unsigned int SPI1_CNTL0_REG;
	volatile unsigned int SPI1_CNTL1_REG;
	volatile unsigned int SPI1_STAT_REG;
	volatile unsigned int pad3;
	volatile unsigned int SPI1_IO_REG;
	volatile unsigned int SPI1_PEEK_REG;
	unsigned int pad4[11];
	// SPI 2
	volatile unsigned int SPI2_CNTL0_REG;
	volatile unsigned int SPI2_CNTL1_REG;
	volatile unsigned int SPI2_STAT_REG;
	unsigned int pad5;
	volatile unsigned int SPI2_IO_REG;
	volatile unsigned int SPI2_PEEK_REG;
};


