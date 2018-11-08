
/*
 * Standard NAND flash commands
 */
#define NAND_CMD_READ0          0
#define NAND_CMD_READ1          1
#define NAND_CMD_PAGEPROG       0x10
#define NAND_CMD_READOOB        0x50
#define NAND_CMD_ERASE1         0x60
#define NAND_CMD_STATUS         0x70
#define NAND_CMD_STATUS_MULTI   0x71
#define NAND_CMD_SEQIN          0x80
#define NAND_CMD_READID         0x90
#define NAND_CMD_ERASE2         0xd0
#define NAND_CMD_RESET          0xff

/* Extended commands for large page devices */
#define NAND_CMD_READSTART      0x30
#define NAND_CMD_CACHEDPROG     0x15

/* Status bits */
#define NAND_STATUS_FAIL        0x01
#define NAND_STATUS_FAIL_N1     0x02
#define NAND_STATUS_TRUE_READY  0x20
#define NAND_STATUS_READY       0x40
#define NAND_STATUS_WP          0x80

