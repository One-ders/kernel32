
#include "sys.h"
#include <string.h>

#include "jz_nand.h"

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
	{0, 262144},        /* 0-0x3ffff       */
	{262144, 393216},   /* 0x40000-0x9ffff */
};

#define NAND_DATAPORT   0xb8000000
#define NAND_ADDRPORT   0xb8010000
#define NAND_COMMPORT   0xb8008000

#define ECC_BLOCK       512
#define ECC_POS         6
#define PAR_SIZE        9

#define __nand_enable()         (REG_NFCSR |= NFCSR_NFE1 | NFCSR_NFCE1)
#define __nand_disable()        (REG_NFCSR &= ~(NFCSR_NFCE1))
#define __nand_ecc_rs_encoding() \
        (REG_NFECCR = NFECCR_ECCE | NFECCR_ERST | NFECCR_RSE | NFECCR_ENCE)
#define __nand_ecc_rs_decoding() \
        (REG_NFECCR = NFECCR_ECCE | NFECCR_ERST | NFECCR_RSE)
#define __nand_ecc_disable()    (REG_NFECCR &= ~NFECCR_ECCE)
#define __nand_ecc_encode_sync() while (!(REG_NFINTS & NFINTS_ENCF))
#define __nand_ecc_decode_sync() while (!(REG_NFINTS & NFINTS_DECF))

static inline void __nand_dev_ready(void)
{
        unsigned int timeout = 10000;
        while ((REG_GPIO_PxPIN(PORT_C) & 0x40000000) && timeout--);
        while (!(REG_GPIO_PxPIN(PORT_C) & 0x40000000));
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
static int page_per_block = 64;
static int bad_block_pos = 0;
static int block_size	= 131072;

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


static void rs_correct(unsigned char *dat, int idx, int mask) {
        int i;

        idx--;

        i = idx + (idx >> 3);
        if (i >= 512) return;

        mask <<= (idx & 0x7);

        dat[i] ^= mask & 0xff;
        if (i < 511) dat[i+1] ^= (mask >> 8) & 0xff;
}

static int nand_read_oob(int page_addr, unsigned char *buf, int size) {
        int col_addr;

        if (page_size == 2048) col_addr = 2048;
        else col_addr = 0;

        if (page_size == 2048) {
                /* Send READ0 command */
                __nand_cmd(NAND_CMD_READ0);
        } else {
                /* Send READOOB command */
                __nand_cmd(NAND_CMD_READOOB);
        }

        /* Send column address */
        __nand_addr(col_addr & 0xff);
        if (page_size == 2048) {
                __nand_addr((col_addr >> 8) & 0xff);
        }
        /* Send page address */
        __nand_addr(page_addr & 0xff);
        __nand_addr((page_addr >> 8) & 0xff);
        if (row_cycle == 3) {
                __nand_addr((page_addr >> 16) & 0xff);
        }

        /* Send READSTART command for 2048 ps NAND */
        if (page_size == 2048) {
                __nand_cmd(NAND_CMD_READSTART);
        }

        /* Wait for device ready */
        __nand_dev_ready();

        /* Read oob data */
        nand_read_buf(buf, size, bus_width);

        return 0;
}


static int nand_read_page(int block, int page, unsigned char *dst, unsigned char *oobbuf) {
	int page_addr=page+block*page_per_block;
	unsigned char *databuf=dst;
	unsigned char *tmpbuf;
	int i,j;

	nand_read_oob(page_addr, oobbuf, oob_size);

	__nand_cmd(NAND_CMD_READ0);
	__nand_addr(0);
	if (page_size==2048) __nand_addr(0);

	__nand_addr(page_addr&0xff);
	__nand_addr((page_addr>>8)&0xff);
	if (row_cycle==3) __nand_addr((page_addr>>16)&0xff);

	if (page_size==2048) __nand_cmd(NAND_CMD_READSTART);

	__nand_dev_ready();

	tmpbuf=databuf;

	for(i=0;i<ecc_count;i++) {
		volatile unsigned char *paraddr=
			(volatile unsigned char *)&(REG_NFPAR0);
		unsigned int stat;

		REG_NFINTS=0;
		__nand_ecc_rs_decoding();
		nand_read_buf((void *)tmpbuf,ECC_BLOCK,bus_width);

		for (j=0;j<PAR_SIZE;j++) {
			*paraddr++ =oobbuf[ECC_POS+i*PAR_SIZE+j];
		}

		REG_NFECCR |= NFECCR_PRDY;

		__nand_ecc_decode_sync();
		__nand_ecc_disable();

		stat=REG_NFINTS;
		if (stat&NFINTS_ERR) {
			if (stat&NFINTS_UNCOR) {
			} else {
				unsigned int errcnt, index, mask;

				errcnt=(stat&NFINTS_ERRC_MASK) >> NFINTS_ERRC_SHIFT;
				switch(errcnt) {
					case 4:
						index=(REG_NFERR3&NFERR_INDEX_MASK) >> NFERR_INDEX_SHIFT;
						mask=(REG_NFERR3&NFERR_MASK_MASK)>>NFERR_MASK_SHIFT;
						rs_correct(tmpbuf,index,mask);
					case 3:
						index=(REG_NFERR2&NFERR_INDEX_MASK)>>NFERR_INDEX_SHIFT;
						mask=(REG_NFERR2&NFERR_MASK_MASK)>>NFERR_MASK_SHIFT;
						rs_correct(tmpbuf,index,mask);
					case 2:
						index=(REG_NFERR1&NFERR_INDEX_MASK)>>NFERR_INDEX_SHIFT;
						mask=(REG_NFERR1&NFERR_MASK_MASK)>>NFERR_MASK_SHIFT;
						rs_correct(tmpbuf,index,mask);
					case 1:
						index=(REG_NFERR0&NFERR_INDEX_MASK)>>NFERR_INDEX_SHIFT;
						mask=(REG_NFERR0&NFERR_MASK_MASK)>>NFERR_MASK_SHIFT;
						rs_correct(tmpbuf,index,mask);
					default:
						break;
				}
			}
		}
		tmpbuf += ECC_BLOCK;
	}
	return 0;
}

static void nand_read(int offs, int size, unsigned char *dst) {
	int block;
	int pagecopy_count;
	int page_inblock;
	int pages;

	__nand_enable();

	block=offs/block_size;
	pagecopy_count=0;
	pages=((size-1)/page_size)+1;
        page_inblock=(offs/page_size)%page_per_block;


	while (pagecopy_count < pages) {
		nand_read_oob(block*page_per_block+CFG_NAND_BADBLOCK_PAGE,
				oob_buf, oob_size);

		if (oob_buf[bad_block_pos]!=0xff) {
			block++;
			continue;
		}

		for (;page_inblock<page_per_block;page_inblock++) {
			nand_read_page(block,page_inblock,dst,oob_buf);
			dst+=page_size;
			pagecopy_count++;
			if (pagecopy_count>pages) break;
		}

		page_inblock=0;
		block++;
	}
	__nand_disable();
}

static struct device_handle *nand_open(void *instance, DRV_CBH callback, void *userdata) {
	struct userdata *ud=get_userdata();
	if (!ud) return 0;
	ud->fpos=0;
	ud->partition=(unsigned int)instance;

	return (struct device_handle *)ud;
}

static int nand_close(struct device_handle *udh) {
	put_userdata((struct userdata *)udh);
	return 0;
}

static int nand_control(struct device_handle *dh, int cmd, void *arg1, int arg2) {
	struct userdata *udh=(struct userdata *)dh;

	switch(cmd) {
		case NAND_READ_PAGE: {
			nand_read(ud->fpos+partitions[udh->partition].start,
				  arg2,
				  arg1);
			ud->fpos=arg2;
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
	}
	return -1;
}

static int nand_start(void *inst) {
	sys_printf("nand_start: inst=%x\n",inst);
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

	sys_printf("nand_init: inst=%x\n",inst);
	gpio_init();
	boot_sel = REG_BCR >> 30;

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

}

static struct driver_ops nand_drv_ops = {
	nand_open,
	nand_close,
	nand_control,
	nand_init,
	nand_start,
};

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

void init_jz_nand() {
	driver_publish(&nand_drv0);
	driver_publish(&nand_drv1);
}

INIT_FUNC(init_jz_nand);
