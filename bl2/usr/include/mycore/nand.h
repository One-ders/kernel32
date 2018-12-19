#define NAND_READ_RAW   0x1000
#define NAND_READ       0x1001
#define NAND_GET_CONFIG 0x1002
#define NAND_CHECK_BLOCK 0x1003

struct nand_read_data {
	unsigned int	page;
	unsigned char	*dbuf;
	int		dbuf_size;
	unsigned char	*oob_buf;
	int		oob_buf_size;
	unsigned int	*ecc_info;
};

struct nand_config {
	unsigned int page_size;
	unsigned int pages_per_block;
	unsigned int n_blocks;
	unsigned int bad_block_mark;
	unsigned int oob_size;
	unsigned int ecc_count;
};
