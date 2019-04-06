
#include "sys.h"
#include <string.h>

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


#define NAND_DATAPORT   0xb8000000
unsigned int NAND_ADDRPORT;
unsigned int NAND_COMMPORT;

//#define NAND_ADDRPORT   0xb8010000
//#define NAND_COMMPORT   0xb8008000

#define ECC_BLOCK       512
#ifdef CFG_NAND_ECC_POS
#define ECC_POS CFG_NAND_ECC_POS
#else
#define ECC_POS         3
#endif
static int par_size;

#define __nand_enable()         (REG_EMC_NFCSR |= EMC_NFCSR_NFE1 | EMC_NFCSR_NFCE1)
#define __nand_disable()        (REG_EMC_NFCSR &= ~(EMC_NFCSR_NFCE1))

static inline void nand_wait_ready(void) {
        unsigned int timeout = 10000;
        while ((REG_GPIO_PXPIN(2) & 0x08000000) && timeout--);
        while (!(REG_GPIO_PXPIN(2) & 0x08000000));
}


#define __nand_cmd(n)           (REG8(NAND_COMMPORT) = (n))
#define __nand_addr(n)          (REG8(NAND_ADDRPORT) = (n))
#define __nand_data8()          REG8(NAND_DATAPORT)
#define __nand_data16()         REG16(NAND_DATAPORT)


static int bus_width	= 8;
static int page_size	= 2048;
static int oob_size	= 64;
static int ecc_count	= 4;
static int row_cycle	= 3;
static int page_per_block = 128;
static int bad_block_pos = 0;
static int block_size	= 262144;

static unsigned char oob_buf[128] = {0};

static inline void nand_read_buf16(void *buf, int count) {
        int i;
        unsigned short int *p = (unsigned short int *)buf;

        for (i = 0; i < count; i += 2) {
                *p++ = __nand_data16();
        }
}

static inline void nand_read_buf8(void *buf, int count) {
        int i;
        unsigned char *p = (unsigned char *)buf;

        for (i = 0; i < count; i++) {
                *p++ = __nand_data8();
        }
}


static inline void nand_read_buf(void *buf, int count, int bw) {
        if (bw == 8) {
                nand_read_buf8(buf, count);
        } else {
                nand_read_buf16(buf, count);
        }
}


static void bch_correct(unsigned char *dat, int idx) {
	int i, bit;  // the 'bit' of i byte is error 

	i = (idx - 1) >> 3;
	bit = (idx - 1) & 0x7;
	if (i < 512) {
		dat[i] ^= (1 << bit);
	}
}

static int nand_read_oob(int page_addr, unsigned char *buf, int size) {
	int col_addr;

	if (page_size != 512) {
		if (bus_width == 8) {
			col_addr = page_size;
		} else {
			col_addr = page_size / 2;
		}
	} else {
		col_addr = 0;
	}

	if (page_size != 512) {
		/* Send READ0 command */
		__nand_cmd(NAND_CMD_READ0);
	} else {
		/* Send READOOB command */
		__nand_cmd(NAND_CMD_READOOB);
	}

	/* Send column address */
	__nand_addr(col_addr & 0xff);
	if (page_size != 512) {
		__nand_addr((col_addr >> 8) & 0xff);
	}
        /* Send page address */
	__nand_addr(page_addr & 0xff);
	__nand_addr((page_addr >> 8) & 0xff);
	if (row_cycle == 3) {
		__nand_addr((page_addr >> 16) & 0xff);
	}

	/* Send READSTART command for 2048 or 4096 ps NAND */
	if (page_size != 512) {
		__nand_cmd(NAND_CMD_READSTART);
	}

	/* Wait for device ready */
	nand_wait_ready();

	/* Read oob data */
	nand_read_buf(buf, size, bus_width);

	if (page_size == 512) {
		nand_wait_ready();
	}

	return 0;
}


static int nand_read_page(int page_addr, 
			unsigned char *dst, 
			unsigned char *oobbuf) {
	unsigned char *data_buf = dst;
	int i, j;
	unsigned int flag=0;

	/* Send READ0 command */
	__nand_cmd(NAND_CMD_READ0);

	/* Send column address */
	__nand_addr(0);
	if (page_size != 512) {
		__nand_addr(0);
	}

	/* Send page address */
	__nand_addr(page_addr & 0xff);
	__nand_addr((page_addr >> 8) & 0xff);
	if (row_cycle == 3) {
		__nand_addr((page_addr >> 16) & 0xff);
	}

	/* Send READSTART command for 2048 or 4096 ps NAND */
	if (page_size != 512) {
		__nand_cmd(NAND_CMD_READSTART);
	}

	/* Wait for device ready */
	nand_wait_ready();

	/* Read page data */
	data_buf = dst;

	/* Read data */
	nand_read_buf((void *)data_buf, page_size, bus_width);
	nand_read_buf((void *)oobbuf, oob_size, bus_width);

	ecc_count = page_size / ECC_BLOCK;

	for (i = 0; i < ecc_count; i++) {
		unsigned int stat;

//		sys_printf("in forloop: i=%d, ecc_count=%d\n",
//			i,ecc_count);
#ifndef CFG_NAND_BCH_WITH_OOB
		__ecc_cnt_dec(ECC_BLOCK + par_size);
#else
//		sys_printf("ecc_cnt_dec: %d\n",
//			ECC_BLOCK + 
//                               (CFG_NAND_ECC_POS/ecc_count) +
//                               par_size);
		__ecc_cnt_dec(ECC_BLOCK + 
				(CFG_NAND_ECC_POS/ecc_count) +
				par_size);
#endif

		/* Enable BCH decoding */
		REG_BCH_INTS = 0xffffffff;
		if (CFG_NAND_BCH_BIT == 8) {
			__ecc_decoding_8bit();
		} else {
			__ecc_decoding_4bit();
		}

		/* Write 512 bytes and par_size parities to REG_BCH_DR */
		for (j = 0; j < ECC_BLOCK; j++) {
			REG_BCH_DR = data_buf[j];
		}

#ifdef CFG_NAND_BCH_WITH_OOB
		for(j=0; j<(CFG_NAND_ECC_POS/ecc_count);j++) {
//			sys_printf("write bch byte from oob[%d]\n", j+(i*(CFG_NAND_ECC_POS/ecc_count)));
			REG_BCH_DR=oob_buf[j+(i*(CFG_NAND_ECC_POS/ecc_count))];
		}
#endif

		for (j = 0; j < par_size; j++) {
//#if defined(CFG_NAND_ECC_POS)
//                      REG_BCH_DR = oob_buf[CFG_NAND_ECC_POS + i*par_size + j];
//                      if (oob_buf[CFG_NAND_ECC_POS + i*par_size + j]!=0xff)
//                              flag=1;
//#else
			REG_BCH_DR = oob_buf[ECC_POS + i*par_size + j];
			if (oob_buf[ECC_POS + i*par_size + j]!=0xff) {
				flag=1;
			}
//#endif
		}

		/* Wait for completion */
		__ecc_decode_sync();
		__ecc_disable();

		/* Check decoding */
		stat = REG_BCH_INTS;
		if (stat & BCH_INTS_ERR) {
			if (stat & BCH_INTS_UNCOR) {
				if (flag) {
					/* Uncorrectable error occurred */
					sys_printf("Uncorrectable page - 0x%x\n",page_addr);
					return -1;
				}
			} else {
				unsigned int errcnt = (stat & BCH_INTS_ERRC_MASK) >> BCH_INTS_ERRC_BIT;
				switch (errcnt) {
					case 8:
						bch_correct(data_buf, (REG_BCH_ERR3 & BCH_ERR_INDEX_ODD_MASK) >> BCH_ERR_INDEX_ODD_BIT);
					case 7:
						bch_correct(data_buf, (REG_BCH_ERR3 & BCH_ERR_INDEX_EVEN_MASK) >> BCH_ERR_INDEX_EVEN_BIT);
					case 6:
						bch_correct(data_buf, (REG_BCH_ERR2 & BCH_ERR_INDEX_ODD_MASK) >> BCH_ERR_INDEX_ODD_BIT);
					case 5:
						bch_correct(data_buf, (REG_BCH_ERR2 & BCH_ERR_INDEX_EVEN_MASK) >> BCH_ERR_INDEX_EVEN_BIT);
					case 4:
						bch_correct(data_buf, (REG_BCH_ERR1 & BCH_ERR_INDEX_ODD_MASK) >> BCH_ERR_INDEX_ODD_BIT);
					case 3:
						bch_correct(data_buf, (REG_BCH_ERR1 & BCH_ERR_INDEX_EVEN_MASK) >> BCH_ERR_INDEX_EVEN_BIT);
					case 2:
						bch_correct(data_buf, (REG_BCH_ERR0 & BCH_ERR_INDEX_ODD_MASK) >> BCH_ERR_INDEX_ODD_BIT);
					case 1:
						bch_correct(data_buf, (REG_BCH_ERR0 & BCH_ERR_INDEX_EVEN_MASK) >> BCH_ERR_INDEX_EVEN_BIT);
						break;
					default:
						break;
				}
			}
		}
		/* increment pointer */
		data_buf += ECC_BLOCK;
	}

	return 0;
}

#ifndef CFG_NAND_BADBLOCK_PAGE
#define CFG_NAND_BADBLOCK_PAGE 127 /* NAND bad block was marked at this page in a block, starting from 0 */
#endif


static void nand_read(int offs, int size, unsigned char *dst) {
	int pagecopy_count;
	int page;

	__nand_enable();

	page=offs/page_size;
	pagecopy_count=0;

	while (pagecopy_count < (size/page_size)) {
		if (page % page_per_block == 0) {
			nand_read_oob(page + CFG_NAND_BADBLOCK_PAGE, oob_buf, oob_size);
			if (oob_buf[bad_block_pos] != 0xff) {
				page += page_per_block;
				/* Skip bad block */
				continue;
			}
		}

		/* Load this page to dst, do the ECC */
		nand_read_page(page, dst, oob_buf);

		dst += page_size;
		page++;
		pagecopy_count++;
	}

	__nand_disable();
}

static unsigned char dumbuf[4096];

static void nand_read_spec(unsigned int p_page, 
				unsigned char *p_data_buf,
				int p_data_buf_size,
				unsigned char *p_oob,
				int p_oob_size,
				unsigned int *ecc_info) {
	int pagecopy_count;
	int page;

	__nand_enable();

	page=p_page;
	pagecopy_count=0;

	while (pagecopy_count < 1) {
		if (!p_data_buf) {
			p_data_buf=dumbuf;
		}

		nand_read_page(page,p_data_buf,oob_buf);

		if (p_oob) {
			memcpy(p_oob,oob_buf,p_oob_size);
		}

		p_data_buf+=page_size;
		page++;
		pagecopy_count++;
	}
	__nand_disable();

	if (ecc_info) *ecc_info=0;
}

static int nand_check_block(unsigned int block) {

	__nand_enable();


	nand_read_oob(block*page_per_block+CFG_NAND_BADBLOCK_PAGE,
				oob_buf, oob_size);

	if (oob_buf[bad_block_pos]!=0xff) {
		__nand_disable();
		sys_printf("nand_check_block: block %d is bad\n", block);
		return 1;
	}

	__nand_disable();
//	sys_printf("nand_check_block: block %d is ok\n", block);
	return 0;
}



static struct device_handle *nand_open(void *instance, DRV_CBH callback, void *userdata) {
	struct userdata *ud=get_userdata();
	if (!ud) return 0;
	ud->fpos=0;
	ud->partition=(unsigned int)instance;

	return &ud->dh;
}

static int nand_close(struct device_handle *udh) {
	put_userdata((struct userdata *)udh);
	return 0;
}

static int nand_control(struct device_handle *dh, int cmd, void *arg1, int arg2) {
	struct userdata *udh=(struct userdata *)dh;

	switch(cmd) {
		case NAND_READ_RAW: {
			int page=(ud->fpos+partitions[udh->partition].start)/2048;
			if (arg2<(2048+64)) {
				sys_printf("nand_read_raw, buffer to small\n");
				return -1;
			}
			sys_printf("nand_read_raw\n");
			nand_read_page(page, arg1, &((char *)arg1)[2048]);
	
#if 0
			nand_read(ud->fpos+partitions[udh->partition].start,
				  arg2,
				  arg1);
#endif
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
			nand_read_spec(nrd->page+(partitions[udh->partition].start/page_size),
						nrd->dbuf,nrd->dbuf_size,
						nrd->oob_buf, nrd->oob_buf_size,
						nrd->ecc_info);
			return arg2;
		}
		case IO_LSEEK: {
			sys_printf("nand io_lseek\n");
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
			sys_printf("nand_get_config: return\n");
			return 0;
		}
		case NAND_CHECK_BLOCK: {
//			sys_printf("nand_check_block: nr %d\n",
//					*((unsigned int *)arg1));
			int rblock=*((unsigned int *)arg1);
			unsigned int boffs=partitions[udh->partition].start/
					(page_per_block*page_size);
			
			return nand_check_block(boffs+rblock);
		}
	}
	return -1;
}

static int nand_start(void *inst) {
	return 0;
}

static void gpio_init(void) {
        /*
         * Initialize SDRAM pins
         */
        __gpio_as_sdram_32bit();

        /*
         * Initialize UART0 pins
         */
        __gpio_as_uart0();
}


static int nand_init(void *inst) {
	int boot_sel;

	gpio_init();
	boot_sel = REG_EMC_BCR >> EMC_BCR_BT_SEL_BIT;

        if (boot_sel == 0x2)
                page_size = 512;
        else
                page_size = 2048;

        bus_width = 8;
        row_cycle = 3;

        page_per_block = (page_size == 2048) ? CFG_NAND_BLOCK_SIZE / page_size : 32;
        bad_block_pos = (page_size == 2048) ? 0 : 5;
        oob_size = page_size / 32;
        block_size = page_size * page_per_block;
        ecc_count = page_size / ECC_BLOCK;
	return 1;
}

static struct driver_ops nand_drv_ops = {
	nand_open,
	nand_close,
	nand_control,
	nand_init,
	nand_start,
};

static char driver_names[10][6];
static struct driver driver_instances[10];

#if 0
static struct driver nand_drv0 = {
	"nand0",
	(void *)0,
	&nand_drv_ops,
};

static struct driver nand_drv1 = {
	"nand1",
	(void *)1,
	&nand_drv_ops,
};

static struct driver nand_drv2 = {
	"nand2",
	(void *)2,
	&nand_drv_ops,
};

#endif


static void setup() {
	int i=0;

	if (CFG_NAND_BCH_BIT==8) {
		par_size=13;
	} else {
		par_size=7;
	}

       if (is_share_mode()) {
                NAND_ADDRPORT = 0xb8010000;
                NAND_COMMPORT = 0xb8008000;
        } else {
                NAND_ADDRPORT = 0xb8000010;
                NAND_COMMPORT = 0xb8000008;
        }

	while(partitions[i].size) {
		sys_sprintf(driver_names[i],"nand%d", i);
		driver_instances[i].name=driver_names[i];
		driver_instances[i].instance=(void *)i;
		driver_instances[i].ops=&nand_drv_ops;
		driver_publish(&driver_instances[i]);
		sys_printf("publishing driver %s\n", driver_names[i]);
		i++;
	}
		
}

void init_jz_nand(void) {
	setup();
}

INIT_FUNC(init_jz_nand);
