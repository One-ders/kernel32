
#include <sys.h>
#include <string.h>

extern unsigned long __bss_start__;
extern unsigned long __bss_end__;

void set_asid(unsigned int asid);
void nand_load(int offs, int size, unsigned char *dst);

struct address_space kernel_asp = {
.pgd	= 0,
.id	= 0,
.brk	= 0
};

struct task main_task = {
.name	=	"init_main",
.sp	=	(void *)(0x84000000-0x2000),
.id	=	256,
.next	=	0,
.next2	=	0,
.state	=	1,
.prio_flags=	3,
.estack	=	(void *)(0x84000000-0x3000),
.asp	=	&kernel_asp
};

void *usr_init=(void *)0x40000;

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

void *sys_sbrk(struct task *t, long int incr) {
	unsigned long int cur=t->asp->brk;
	if (incr<0) {
		sys_printf("sbrk reduce: check for page free\n");
	}
	t->asp->brk+=incr;
	return (void *)cur;
}

int sys_brk(struct task *t, void *nbrk) {
	if (((unsigned long int)nbrk)<0x10000000) return -1;

	if (t->asp->brk>((unsigned long int)nbrk)) {
		sys_printf("brk reduce: check for page free\n");
	}
	t->asp->brk=((unsigned long int)nbrk);
	return 0;
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

int handle_srec_memline(unsigned char *r, int len, struct task *t) {
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
	sys_brk(t,p);
	for (i=4;i<(len-1);i++) {
		if ((p>((unsigned char *)0x10000000))&&
			(p<((unsigned char *)0x70000000))) {
			sys_sbrk(t,1);
//			sys_printf("sys_break = 0x%x\n", sbrk(t,0));
		}
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

//	sys_printf("start_addr: %x\n", addr);
}


static unsigned char sbuf[256];

int parse_srec(unsigned char *srec_buf, int *ofs, struct task *t) {
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
		sys_printf("parse_srec: weird srec leaving, buf char %c(%x)\n",srec_buf[0],srec_buf[0]);
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
			handle_srec_memline(r,len,t);
			break;
		case '7':
			handle_srec_start_addr(r,len);
			return 0;
		default:;
			sys_printf("srec type yeahh %d\n", type);
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
	unsigned long brk_save;
	int ofs=0;
	
	nand_pos=nand_pos&~0x80000000;	/* mask away ram offset */
	nand_pos=(nand_pos+4095)&~4095; /* align up to nearest 4k pos */
	nand_pos+=4096;			/* skip ipl loader */
	
//	sys_printf("nand_pos = %d\n", nand_pos);

	t->asp->brk=0x10000000;
	t->asp->mmap_vaddr=0x10000000;
	brk_save=current->asp->brk;
	current->asp->brk=t->asp->brk;

	curr_pgd=t->asp->pgd;  /* repoint page table directory while loading */
	set_asid(t->asp->id);
	while(1) {
		nand_load(nand_pos,4096,buf);
//		sys_printf("read 4k nand at %x: %x, %x, %x\n",\
				nand_pos,buf[0],buf[1],buf[2]);
		if (!parse_srec(buf,&ofs,t)) {
			break;
		}
		nand_pos+=4096;
	}
	current->asp->brk=brk_save;
	curr_pgd=current->asp->pgd;  /* restore page table direcotry */
	set_asid(current->asp->id);
	return 0;
}
