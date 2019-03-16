

#define CONFIG_JZSOC

#define CONFIG_JZ4750D_KMP510
#define CONFIG_JZ4750D_KMP510_LCD

#define LCD_DEFAULT_BACKLIGHT		100

/*-----------------------------------------------------------------------
 * NAND FLASH configuration
 */
#define CFG_NAND_BW8            1              /* Data bus width: 0-16bit, 1-8bit */
#define CFG_NAND_PAGE_SIZE      2048
#define CFG_NAND_ROW_CYCLE      3     
#define CFG_NAND_BLOCK_SIZE     (256 << 10)     /* NAND chip block size         */
#define CFG_NAND_BADBLOCK_PAGE  63              /* NAND bad block was marked at this page in a block, starting from 0 */
#define CFG_NAND_BCH_BIT        4               /* Specify the hardware BCH algorithm for 4750 (4|8) */
#define CFG_NAND_ECC_POS        24              /* Ecc offset position in oob area, its default value is 3 if it isn't defined. */
#define CFG_NAND_IS_SHARE       1               /* Just for other boot mode(e.g. SD boot), it can be auto-detected by NAND boot */

#define CFG_MAX_NAND_DEVICE     1
#define NAND_MAX_CHIPS          1
#define CFG_NAND_BASE           0xB8000000
#define CFG_NAND_SELECT_DEVICE  1       /* nand driver supports mutipl. chips   */
#define CFG_NAND_BCH_WITH_OOB   1       /* linux bch */


#define CFG_CPU_SPEED           378000000       /* CPU clock: 378 MHz */
#define CFG_EXTAL               24000000        /* EXTAL freq: 24 MHz */
#define CFG_HZ                  (CFG_EXTAL/256) /* incrementer freq */

#define CFG_UART_BASE           UART1_BASE      /* Base of the UART channel */
#define CFG_CONSOLE_DEV		"usart1"

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
#define SDRAM_TREF              7812    /* Refresh period: 4096 refresh cycles/64ms */

#define SDRAM_SIZE		(64*1024*1024)
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

#if 0
/*-----------------------------------------------------------------------
 * GPIO
 */
#define GPIO_SD0_VCC_EN_N       (32*4+0) /* CIM_D0 */
#define GPIO_SD0_CD_N           (32*4+1) /* CIM_D1 */
#define GPIO_SD0_WP             (32*4+2) /* CIM_D2 */
#define GPIO_SD1_VCC_EN_N       (32*4+3) /* CIM_D3 */
#define GPIO_SD1_CD_N           (32*4+4) /* CIM_D4 */
#define GPIO_USB_DETE           (32*4+6) /* CIM_D6 */
#define GPIO_DC_DETE_N          (32*4+8) /* CIM_MCLK */
#define GPIO_CHARG_STAT_N       (32*4+10) /* CIM_VSYNC */
#define GPIO_DISP_OFF_N         (32*4+18) /* SDATO */
#define GPIO_LCD_VCC_EN         (32*4+19) /* SDATI */
#define GPIO_LCD_PWM            (32*4+22) /* GPE22 PWM2 */ 
#define LCD_PWM_CHN             2         /* pwm channel */
#endif

#include <jz4750d.h>
#include <board_kmp510.h>

