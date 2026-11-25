
#include "sys.h"
#include "sys_const.h"
#include <string.h>

#include <dtb.h>
#include <mtd_nand.h>

#include <nand.h>


struct userdata {
        struct device_handle dh;
        unsigned long int fpos;
	unsigned int partition;
        int busy;
};

static struct userdata ud[16];

static struct userdata *get_userdata(void) {
        int i;
        for(i=0;i<16;i++) {
                if (!ud[i].busy) {
                        ud[i].busy=1;
                        return &ud[i];
                }
        }
        return 0;
}


static void put_userdata(struct userdata *uud) {
        int i;
        for(i=0;i<16;i++) {
                if (&ud[i]==uud) {
                        uud->busy=0;
                        return;
                }
        }
}

struct partition {
	unsigned int start;
	unsigned int size;
};

static struct partition partitions[] = {
	{0x00000000, 0x00400000}, // ROOT_FS on yaffs2
	{0,0}
};

#if 0
static struct partition partitions[] = {
	{0x00000000, 0x00400000}, // NAND BOOT
	{0x00400000, 0x00400000}, // NAND KERNEL
	{0x00800000, 0x00400000}, // Failsafe Kernel
	{0x00c00000, 0x00400000}, // System Data partition
	{0x01000000, 0x08000000}, // Alt FS
	{0x09000000, 0x10000000}, // Root FS
	{0x19000000, 0x00c00000}, // data 1
	{0x19c00000, 0x02000000}, // data 2
	{0x1bc00000, 0x02000000}, // data 3
	{0x1dc00000, 0x02400000}, // VFAT
	{0,0}
};
#endif

#define RAM_NAND_BASE (VA_START+initrd_phys_start)

static int par_size;

static inline void nand_wait_ready(void) {
}

static int bus_width	= 8;
static int page_size	= 2048;
static int oob_size	= 64;
static int ecc_count	= 0;
static int row_cycle	= 3;
static int page_per_block = 128;
static int bad_block_pos = 0;
static int block_size	= 262144;

static unsigned char oob_buf[128] = {0};

static int ramnand_read_oob(int page_addr, unsigned char *buf, int size) {
	unsigned char *ramnand_addr=(unsigned char *)RAM_NAND_BASE;

	unsigned long int addr=(page_addr*(2048+64))+2048;
	memcpy(buf,ramnand_addr+addr,size);

	return 0;
}

static int ramnand_read_page(int page_addr,
			unsigned char *dst,
			unsigned char *oobbuf) {
	unsigned char *data_buf = dst;
	unsigned int flag=0;
	unsigned char *ramnand_addr=(unsigned char *)RAM_NAND_BASE;

	unsigned long int addr=(page_addr*(2048+64));
	memcpy(data_buf,&ramnand_addr[addr],page_size);
	memcpy(oobbuf,&ramnand_addr[addr+2048],oob_size);

	return 0;
}

#ifndef CFG_NAND_BADBLOCK_PAGE
#define CFG_NAND_BADBLOCK_PAGE 127 /* NAND bad block was marked at this page in a block, starting from 0 */
#endif


static void ramnand_read(int offs, int size, unsigned char *dst) {
	int pagecopy_count;
	int page;

	page=offs/page_size;
	pagecopy_count=0;

	while (pagecopy_count < (size/page_size)) {
		if (page % page_per_block == 0) {
			ramnand_read_oob(page + CFG_NAND_BADBLOCK_PAGE, oob_buf, oob_size);
			if (oob_buf[bad_block_pos] != 0xff) {
				page += page_per_block;
				/* Skip bad block */
				continue;
			}
		}

		/* Load this page to dst, do the ECC */
		ramnand_read_page(page, dst, oob_buf);

		dst += page_size;
		page++;
		pagecopy_count++;
	}
}

static unsigned char dumbuf[4096];

static void ramnand_read_spec(unsigned int p_page,
				unsigned char *p_data_buf,
				int p_data_buf_size,
				unsigned char *p_oob,
				int p_oob_size,
				unsigned int *ecc_info) {
	int pagecopy_count;
	int page;


	page=p_page;
	pagecopy_count=0;

	while (pagecopy_count < 1) {
		if (!p_data_buf) {
			p_data_buf=dumbuf;
		}

		ramnand_read_page(page,p_data_buf,oob_buf);

		if (p_oob) {
			memcpy(p_oob,oob_buf,p_oob_size);
		}

		p_data_buf+=page_size;
		page++;
		pagecopy_count++;
	}

	if (ecc_info) *ecc_info=0;
}

static int ramnand_check_block(unsigned int block) {
unsigned int calculated_page = block * page_per_block + CFG_NAND_BADBLOCK_PAGE;

	ramnand_read_oob(block*page_per_block+CFG_NAND_BADBLOCK_PAGE,
				oob_buf, oob_size);

	if (oob_buf[bad_block_pos]!=0xff) {
		sys_printf("ramnand_check_block: block %d is bad\n", block);
		return 1;
	}

	return 0;
}



static struct device_handle *ramnand_open(void *instance, DRV_CBH callback, void *userdata) {
	struct userdata *ud=get_userdata();
	if (!ud) return 0;
	ud->fpos=0;
	ud->partition=(unsigned int)instance;

	return &ud->dh;
}

static int ramnand_close(struct device_handle *udh) {
	put_userdata((struct userdata *)udh);
	return 0;
}

static int ramnand_control(struct device_handle *dh, int cmd, void *arg1, int arg2) {
	struct userdata *udh=(struct userdata *)dh;

	switch(cmd) {
		case NAND_READ_RAW: {
			int page=(ud->fpos+partitions[udh->partition].start)/2048;
			if (arg2<(2048+64)) {
				sys_printf("nand_read_raw, buffer to small\n");
				return -1;
			}
//			sys_printf("nand_read_raw: page=%d\n", page);
			ramnand_read_page(page, arg1, &((char *)arg1)[2048]);

			ud->fpos+=2048;
			return arg2;
		}
		case NAND_READ: {
			struct nand_read_data *nrd=(struct nand_read_data *)arg1;
			if (nrd->dbuf) {
				if (nrd->dbuf_size!=2048) {
					sys_printf("nand_read: size is %d expect 2048\n",
								nrd->dbuf_size);
				}
			}

//			sys_printf("nand_read_spec: page %d, buf %x, size %d\n",
//					nrd->page, nrd->dbuf, nrd->dbuf_size);
			ramnand_read_spec(nrd->page+(partitions[udh->partition].start/page_size),
						nrd->dbuf,nrd->dbuf_size,
						nrd->oob_buf, nrd->oob_buf_size,
						nrd->ecc_info);
			return arg2;
		}
		case IO_LSEEK: {
                        switch(arg2) {
                                case 0:
                                        udh->fpos=(unsigned long int)arg1;
                                        break;
                                case 1:
                                        udh->fpos+=(unsigned long int)arg1;
                                        break;
                                default:
                                        return -1;
                        }
                        return udh->fpos;
                }
		case NAND_GET_CONFIG: {
			struct nand_config *nand_config=
				(struct nand_config *)arg1;
			nand_config->page_size=page_size;
			nand_config->pages_per_block=page_per_block;
			nand_config->n_blocks=
				partitions[udh->partition].size/
				 (page_per_block*page_size);
				;
			nand_config->bad_block_mark=bad_block_pos;
			nand_config->oob_size=oob_size;
			nand_config->ecc_count=ecc_count;
			return 0;
		}
		case NAND_CHECK_BLOCK: {
			int rblock=*((unsigned int *)arg1);
			unsigned int boffs=partitions[udh->partition].start/
					(page_per_block*page_size);

			return ramnand_check_block(boffs+rblock);
		}
	}
	return -1;
}

static int ramnand_start(void *inst) {
	return 0;
}

static int ramnand_init(void *inst) {
	page_size = 2048;

	bus_width = 8;
	row_cycle = 1;

        page_per_block = CFG_NAND_BLOCK_SIZE / page_size;
        bad_block_pos =  0;
        oob_size = page_size / 32;
        block_size = page_size * page_per_block;
        ecc_count = 0;
	return 1;
}

static struct driver_ops nand_drv_ops = {
	ramnand_open,
	ramnand_close,
	ramnand_control,
	ramnand_init,
	ramnand_start,
};

//static char driver_names[1][16];
static struct driver driver_instances[1];

static void setup() {
	int i=0;

//	sys_sprintf(driver_names[i],"ramnand%d", i);
//	driver_instances[i].name=driver_names[i];
	driver_instances[i].name="ramnand0";
	driver_instances[i].instance=(void *)i;
	driver_instances[i].ops=&nand_drv_ops;
	driver_publish(&driver_instances[i]);
//	sys_printf("publishing driver %s\n", driver_instances[i].name);
}

void init_ramnand(void) {
	setup();
}

INIT_FUNC(init_ramnand);
