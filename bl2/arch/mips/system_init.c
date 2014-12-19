
#include <sys.h>

void _exit(int status) {
}

void build_free_page_list(void);

void init_sys_arch(void) {
	build_free_page_list();
}

void init_irq(void) {
}

void board_reboot(void) {
}


static unsigned long free_page_map[SDRAM_SIZE/(PAGE_SIZE*sizeof(unsigned long))];
extern unsigned long __bss_start__;
extern unsigned long __bss_end__;

#define PHYS2PAGE(a)	((((unsigned long int)(a))&0x7fffffff)>>12)
#define set_in_use(a,b) ((a)[((unsigned long int)(b))/32]&=~(1<<(((unsigned long int)(b))%32)))
#define set_free(a,b) ((a)[((unsigned long int)(b))/32]|=(1<<(((unsigned long int)(b))%32)))

void build_free_page_list() {
	unsigned long int mptr=0x80000000;
	int i;
//	sys_printf("build_free_page_list\n");

	for(i=0;i<(sizeof(free_page_map)/sizeof(unsigned long int));i++) {
		free_page_map[i]=~0;
	}

	while(mptr<((unsigned long int)&__bss_end__)) {
		set_in_use(free_page_map,PHYS2PAGE(mptr));
#if 0
		sys_printf("build_free_page_list: set %x non-free, map %x\n",mptr, free_page_map[0]);
		sys_printf("map index %x, mapmask %x\n",PHYS2PAGE(mptr)/32,
						~(1<<((PHYS2PAGE(mptr))%32)));
#endif
		mptr+=4096;
	}
}

void *get_page(void) {
	int i;
	for(i=0;i<(sizeof(free_page_map)/sizeof(unsigned long int));i++) {
		if (free_page_map[i]) {
			int j=ffsl(free_page_map[i]);
			if (!j) return 0;
//			sys_printf("found free page att index %x, bit %x\n",
//					i,j);
			j--;
			free_page_map[i]&=~(1<<j);
			return (void *)(0x80000000 + (i*32*4096) + (j*4096));
		}
	}
	return 0;
}

void put_page(void *p) {
	set_free(free_page_map,PHYS2PAGE(p));
}

unsigned char to_val(char n) {
	switch (n) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			return n-'0';
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			return 0xa+(n-'a');
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			return 0xa+(+n-'A');
	}
	return -1;
}


int handle_srec_header(unsigned char *r, int len) {
	char bbuf[len];
	int i;
//	sys_printf("handle_srec_header: r(%x) len(%x)\n", *r, len);
	for (i=0;i<(len-1);i++) {
		bbuf[i]=(to_val(r[i*2])<<4)|to_val(r[(i*2)+1]);
	}
	bbuf[len-1]=0;
//	sys_printf("srec header: %s\n", &bbuf[2]);
	return 0;
}

int handle_srec_memline(unsigned char *r, int len) {
	char bbuf[len];
	unsigned long addr;
	unsigned char *p;
	int i;
//	sys_printf("srec_memline: at %x, len %x\n", r, len);
	addr=(to_val(r[0])<<28)|(to_val(r[1])<<24)|
		(to_val(r[2])<<20)|(to_val(r[3])<<16)|
		(to_val(r[4])<<12)|(to_val(r[5])<<8)|
		(to_val(r[6])<<4)|(to_val(r[7])<<0);

//	sys_printf("addr is %x\n",addr);
	p=(unsigned char *)addr;
	for (i=4;i<(len-1);i++) {
		(*p)=(to_val(r[i*2])<<4)|to_val(r[(i*2)+1]);
		p++;
	}
}

int handle_srec_start_addr(unsigned char *r, int len) {
	unsigned long addr;
	int i;
	addr=(to_val(r[0])<<28)|(to_val(r[1])<<24)|
		(to_val(r[2])<<20)|(to_val(r[3])<<16)|
		(to_val(r[4])<<12)|(to_val(r[5])<<8)|
		(to_val(r[6])<<4)|(to_val(r[7])<<0);

	sys_printf("start_addr: %x\n", addr);
}


static unsigned char sbuf[256];

int parse_srec(unsigned char *srec_buf, int *ofs) {
	int type;
	unsigned int len;
	unsigned char *r;
	unsigned char *b_start=0;
	unsigned char *b_end=srec_buf+4096;
	
	if (*ofs) {
//		sys_printf("parse_srec: got ofs %d\n", *ofs);
		b_start=srec_buf;
		memcpy(&sbuf[*ofs],srec_buf,255-*ofs);
		srec_buf=sbuf;
	}

again:
//	sys_printf("parse_srec: srec_buf@%x, end@%x\n", srec_buf,b_end);
	if (srec_buf[0]!='S') {
		sys_printf("parse_srec: weird srec leaving\n");
		return 0;
	}
	
	type=srec_buf[1];
	len=(to_val(srec_buf[2])<<4)|to_val(srec_buf[3]);
	if (((srec_buf+(len*2)+4))>=b_end) {
		*ofs=b_end-srec_buf;
//		sys_printf("parse_srec: premature end of payload buf, ofs=%d\n",*ofs);
		memcpy(sbuf,srec_buf,b_end-srec_buf);
		return 1;
	}
	r=&srec_buf[4];
	switch(type) {
		case '0':
			handle_srec_header(r,len);
			break;
		case '3':
			handle_srec_memline(r,len);
			break;
		case '7':
			handle_srec_start_addr(r,len);
			return 0;
		default:;
//			sys_printf("srec type yeahh %d\n", type);
	}
	if (b_start&&srec_buf!=b_start) {
		srec_buf=b_start-*ofs;
		*ofs=0;
		b_start=0;
	}
	srec_buf+=((len*2)+4+2); /* payload len + 'S' + type + len + crlf */
	if (srec_buf>=(b_end-4)) {
		*ofs=b_end-srec_buf;
//		sys_printf("parse_srec: premature end of header buf, ofs %d\n",*ofs);
		memcpy(sbuf,srec_buf,b_end-srec_buf);
		return 1;
	}
	goto again;
	return 0;
}

extern unsigned long int *curr_pgd;

int load_init(struct task *t) {
	unsigned char buf[5096];
	unsigned int nand_pos=(unsigned int)&__bss_start__;
	int ofs=0;
	
	nand_pos=nand_pos&~0x80000000;	/* mask away ram offset */
	nand_pos=(nand_pos+4095)&~4095; /* align up to nearest 4k pos */
	nand_pos+=4096;			/* skip ipl loader */
	
//	sys_printf("nand_pos = %d\n", nand_pos);

	curr_pgd=t->pgd;  /* repoint page table directory while loading */
	while(1) {
		nand_load(nand_pos,4096,buf);
//		sys_printf("read 4k nand\n");
		if (!parse_srec(buf,&ofs)) {
			break;
		}
		nand_pos+=4096;
	}
	curr_pgd=current->pgd;  /* restore page table direcotry */
	return 0;
}
