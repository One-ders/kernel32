#define NAND_READ_PAGE  0x1000
#define NAND_GET_CONFIG 0x1001

struct nand_config {
	unsigned int page_size;
	unsigned int pages_per_block;
	unsigned int n_blocks;
	unsigned int bad_block_mark;
	unsigned int oob_size;
	unsigned int ecc_count;
};
