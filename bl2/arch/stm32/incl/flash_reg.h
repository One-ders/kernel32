
/* Flash access control register */
#define FLASH_ACR_LATENCY_MASK		0x00000007
#define FLASH_ACR_LATENCY_SHIFT		0
#define FLASH_ACR_PRFTEN		0x00000100
#define FLASH_ACR_ICEN			0x00000200
#define FLASH_ACR_DCEN			0x00000400
#define FLASH_ACR_ICRST			0x00000800
#define FLAGS_ACR_DCRST			0x00001000

/* Flash status register */
#define FLASH_SR_EOP			0x00000001
#define FLASH_SR_OPERR			0x00000002
#define FLASH_SR_WRPERR			0x00000010
#define FLASH_SR_PGAERR			0x00000020
#define FLASH_SR_PGPERR			0x00000040
#define FLASH_SR_PGSERR			0x00000080
#define FLASH_SR_BSY			0x00010000

/* Flash control register */
#define FLASH_CR_PG			0x00000001
#define FLASH_CR_SER			0x00000002
#define FLASH_CR_MER			0x00000004
#define FLASH_CR_SNB_MASK		0x00000078
#define FLASH_CR_SNB_SHIFT		3
#define FLASH_CR_PSIZE_MASK		0x00000300
#define FLASH_CR_PSIZE_SHIFT		8
#define FLASH_CR_STRT			0x00010000
#define FLASH_CR_EOPIE			0x01000000
#define FLASH_CR_ERRIE			0x02000000
#define FLASH_CR_LOCK			0x80000000

/* Flash option control register */
#define FLASH_OPTCR_OPTLOCK		0x00000001
#define FLASH_OPTCR_OPTSTRT		0x00000002
#define FLASH_OPTCR_BOR_LEV_MASK	0x0000000c
#define FLASH_OPTCR_BOR_LEV_SHIFT	2
#define FLASH_OPTCR_WDG_SW		0x00000020
#define FLASH_OPTCR_NRST_STOP		0x00000040
#define FLASH_OPTCR_NRST_STDBY		0x00000080
#define FLASH_OPTCR_RDP_MASK		0x0000ff00
#define FLASH_OPTCR_RDP_SHIFT		8
#define FLASH_OPTCR_NWRP_MASK		0x0fff0000
#define FLASH_OPTCR_NWRP_SHIFT		16


struct FLASH {
	volatile unsigned int ACR;
	volatile unsigned int KEYR;
	volatile unsigned int OPTKEYR;
	volatile unsigned int SR;
	volatile unsigned int CR;
	volatile unsigned int OPTCR;
};
