
/* Defines for Usart Status Register */
#define USART_SR_PE	0x00000001
#define USART_SR_FE	0x00000002
#define USART_SR_NF	0x00000004
#define USART_SR_ORE	0x00000008
#define USART_SR_IDLE	0x00000010
#define USART_SR_RXNE	0x00000020
#define USART_SR_TC	0x00000040
#define USART_SR_TXE	0x00000080
#define USART_SR_LBD	0x00000100
#define USART_SR_CTS	0x00000200

/* Defines for Usart Control Register 1 */
#define USART_CR1_SBK		0x00000001
#define USART_CR1_RWU		0x00000002
#define USART_CR1_RE		0x00000004
#define USART_CR1_TE		0x00000008
#define USART_CR1_IDLEIE	0x00000010
#define USART_CR1_RXNEIE	0x00000020
#define USART_CR1_TCIE		0x00000040
#define USART_CR1_TXEIE		0x00000080
#define USART_CR1_PEIE		0x00000100
#define USART_CR1_PS		0x00000200
#define USART_CR1_PCE		0x00000400
#define USART_CR1_WAKE		0x00000800
#define USART_CR1_M		0x00001000
#define USART_CR1_UE		0x00002000
#define USART_CR1_OVER8		0x00008000

/* Defines for Usart Control Register 2 */
#define USART_CR2_ADD_MASK	0x0000000f
#define USART_CR2_LBDL		0x00000020
#define USART_CR2_LBDIE		0x00000040
#define USART_CR2_LBCL		0x00000100
#define USART_CR2_CPHA		0x00000200
#define USART_CR2_CPOL		0x00000400
#define USART_CR2_CLKEN		0x00000800
#define USART_CR2_STOP_MASK	0x00003000
#define USART_CR2_STOP_SHIFT	12
#define USART_CR2_LINEN		0x00004000

/* Defines for Usart Control Register 3 */
#define USART_CR3_EIE		0x00000001
#define USART_CR3_IREN		0x00000002
#define USART_CR3_IRLP		0x00000004
#define USART_CR3_HDSEL		0x00000008
#define USART_CR3_NACK		0x00000010
#define USART_CR3_SCEN		0x00000020
#define USART_CR3_DMAR		0x00000040
#define USART_CR3_DMAT		0x00000080
#define USART_CR3_RTSE		0x00000100
#define USART_CR3_CTSE		0x00000200
#define USART_CR3_CTSIE		0x00000400
#define USART_CR3_ONEBIT	0x00000800

/* Usart Guard time and prescaler register */
#define USART_GTPR_PSC		0x000000ff
#define USART_GTPR_GT		0x0000ff00

struct USART {
	volatile unsigned int SR;  /* Status reg */
	volatile unsigned int DR;	/* Data reg */
	volatile unsigned int BRR; /* Baud rate reg */
	volatile unsigned int CR1; /* Control register */
	volatile unsigned int CR2; /* Control register */
	volatile unsigned int CR3; /* Control register */
	volatile unsigned int GTPR;/* Guard time and prescaler reg */
};
