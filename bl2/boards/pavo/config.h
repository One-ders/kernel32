
#include <jz4740.h>

#define JZ4740_BOOT_NAND_CFG	JZ4740_BOOT_NAND_CFG_3PC8B

#if 0
#define CFG_NAND_U_BOOT_OFFS	(256 << 10) /* Offset to RAM U-Boot image   */
#define CFG_NAND_U_BOOT_SIZE	(512 << 10) /* Size of RAM U-Boot image     */
#endif

#define CFG_NAND_BLOCK_SIZE	(256 << 10) /* NAND chip block size         */
#define CFG_NAND_BADBLOCK_PAGE	127 /* NAND bad block was marked at this page in a block, starting from 0 */

#define CFG_CPU_SPEED           336000000       /* CPU clock: 336 MHz */
#define CFG_EXTAL               12000000        /* EXTAL freq: 12 MHz */
#define CFG_HZ                  (CFG_EXTAL/256) /* incrementer freq */

#define CFG_UART_BASE           UART0_BASE      /* Base of the UART channel */

#define CONFIG_BAUDRATE         57600

/*-----------------------------------------------------------------------
 * SDRAM Info.
 */
#define CONFIG_NR_DRAM_BANKS    1

// SDRAM paramters
#define SDRAM_BW16              0       /* Data bus width: 0-32bit, 1-16bit */
#define SDRAM_BANK4             1       /* Banks each chip: 0-2bank, 1-4bank */
#define SDRAM_ROW               13      /* Row address: 11 to 13 */
#define SDRAM_COL               9       /* Column address: 8 to 12 */
#define SDRAM_CASL              2       /* CAS latency: 2 or 3 */

// SDRAM Timings, unit: ns
#define SDRAM_TRAS              45      /* RAS# Active Time */
#define SDRAM_RCD               20      /* RAS# to CAS# Delay */
#define SDRAM_TPC               20      /* RAS# Precharge Time */
#define SDRAM_TRWL              7       /* Write Latency Time */
#define SDRAM_TREF              15625   /* Refresh period: 4096 refresh cycles/64ms */

#define SDRAM_SIZE		(64*1024*1024)
#define PAGE_SIZE		4096
#define	BYTES_PER_LONG		4

/*-----------------------------------------------------------------------
 * Cache Configuration
 */
#define CFG_DCACHE_SIZE         16384
#define CFG_ICACHE_SIZE         16384
#define CFG_CACHELINE_SIZE      32

#define MMU
#define PT_MASK 0xffc00000
#define PT_SHIFT 22

#define P_MASK  0x003ff000
#define P_SHIFT 12

#define P_SIZE	4096
