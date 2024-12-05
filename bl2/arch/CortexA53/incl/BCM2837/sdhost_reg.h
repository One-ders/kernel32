
struct SDHOST {
#define SDCMD_NEW_FLAG                  0x8000
#define SDCMD_FAIL_FLAG                 0x4000
#define SDCMD_BUSYWAIT                  0x800
#define SDCMD_NO_RESPONSE               0x400
#define SDCMD_LONG_RESPONSE             0x200
#define SDCMD_WRITE_CMD                 0x80
#define SDCMD_READ_CMD                  0x40
#define SDCMD_CMD_MASK                  0x3f
	unsigned int cmd;	/* 0x00 Command to SD card	- 16 R/W */
	unsigned int arg;	/* 0x04 Argument to SD card	- 32 R/W */
	unsigned int tout;	/* 0x08 Start val for tout cnt	- 32 R/W */

#define SDCDIV_MAX_CDIV                 0x7ff
	unsigned int cdiv;	/* 0x0c Start val for clk div	- 11 R/W */
	unsigned int resp[4];	/* 0x10 SD card resp. (31:0)	- 32 R   */
//	unsigned int resp1;	/* 0x14 SD card resp. (63:32)	- 32 R   */
//	unsigned int resp2;	/* 0x18 SD card resp. (95:64)	- 32 R   */
//
//	unsigned int resp3;	/* 0x1c SD card resp. (127:96)	- 32 R   */

#define SDHSTS_BUSY_IRPT                0x400
#define SDHSTS_BLOCK_IRPT               0x200
#define SDHSTS_SDIO_IRPT                0x100
#define SDHSTS_REW_TIME_OUT             0x80
#define SDHSTS_CMD_TIME_OUT             0x40
#define SDHSTS_CRC16_ERROR              0x20
#define SDHSTS_CRC7_ERROR               0x10
#define SDHSTS_FIFO_ERROR               0x08
/* Reserved */
/* Reserved */
#define SDHSTS_DATA_FLAG                0x01

#define SDHSTS_TRANSFER_ERROR_MASK      (SDHSTS_CRC7_ERROR | \
                                         SDHSTS_CRC16_ERROR | \
                                         SDHSTS_REW_TIME_OUT | \
                                         SDHSTS_FIFO_ERROR)

#define SDHSTS_ERROR_MASK               (SDHSTS_CMD_TIME_OUT | \
                                         SDHSTS_TRANSFER_ERROR_MASK)
	unsigned int hstat;	/* 0x20 SD host status		- 11 R/W */
	unsigned int pad0[3];

#define SDVDD_POWER_OFF         0
#define SDVDD_POWER_ON          1
	unsigned int vdd;	/* 0x30 SD card power ctrl	-  1 R/W */

#define SDEDM_FORCE_DATA_MODE   BIT(19)
#define SDEDM_CLOCK_PULSE       BIT(20)
#define SDEDM_BYPASS            BIT(21)

#define SDEDM_WRITE_THRESHOLD_SHIFT     9
#define SDEDM_READ_THRESHOLD_SHIFT      14
#define SDEDM_THRESHOLD_MASK            0x1f

#define SDEDM_FSM_MASK          0xf
#define SDEDM_FSM_IDENTMODE     0x0
#define SDEDM_FSM_DATAMODE      0x1
#define SDEDM_FSM_READDATA      0x2
#define SDEDM_FSM_WRITEDATA     0x3
#define SDEDM_FSM_READWAIT      0x4
#define SDEDM_FSM_READCRC       0x5
#define SDEDM_FSM_WRITECRC      0x6
#define SDEDM_FSM_WRITEWAIT1    0x7
#define SDEDM_FSM_POWERDOWN     0x8
#define SDEDM_FSM_POWERUP       0x9
#define SDEDM_FSM_WRITESTART1   0xa
#define SDEDM_FSM_WRITESTART2   0xb
#define SDEDM_FSM_GENPULSES     0xc
#define SDEDM_FSM_WRITEWAIT2    0xd
#define SDEDM_FSM_STARTPOWDOWN  0xf
	unsigned int edm;	/* 0x34 Emerg. debug mode	- 13 R/W */

#define SDHCFG_BUSY_IRPT_EN     BIT(10)
#define SDHCFG_BLOCK_IRPT_EN    BIT(8)
#define SDHCFG_SDIO_IRPT_EN     BIT(5)
#define SDHCFG_DATA_IRPT_EN     BIT(4)
#define SDHCFG_SLOW_CARD        BIT(3)
#define SDHCFG_WIDE_EXT_BUS     BIT(2)
#define SDHCFG_WIDE_INT_BUS     BIT(1)
#define SDHCFG_REL_CMD_LINE     BIT(0)
	unsigned int hcfg;	/* 0x38 Host config		-  2 R/W */
	unsigned int hbct;	/* 0x3c Host byte count (dbg)	- 32 R/W */
	unsigned int data;	/* 0x40 Data to/from SD card	- 32 R/W */
	unsigned int pad1[3];
	unsigned int hblc;	/* 0x50 Host block count 	-  9 R/W */
};

#define SDDATA_FIFO_WORDS       16

#define FIFO_READ_THRESHOLD     4
#define FIFO_WRITE_THRESHOLD    4
#define SDDATA_FIFO_PIO_BURST   8

