
#include <sys.h>
#include "yaffs_guts.h"
#include "yaffs_osglue.h"

#include <string.h>
#include <elf.h>

extern unsigned long __bss_start__;
extern unsigned long __bss_end__;

void set_asid(unsigned int asid);

struct address_space kernel_asp = {
.pgd	= 0,
.id	= 0,
.brk	= 0
};

struct task main_task = {
.name	=	"init_main:000",
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
	struct driver *drv_wdg=driver_lookup("wdg");
	if (!drv_wdg) return;
	drv_wdg->ops->open(drv_wdg->instance,0,0);
}

void *sys_sbrk(struct task *t, long int incr) {
	unsigned long int cur=t->asp->brk;
	if (incr<0) {
		sys_printf("sbrk reduce: check for page free\n");
		return 0;
	}
	t->asp->brk+=incr;
	return (void *)cur;
}

int sys_brk(struct task *t, void *nbrk) {
	if (((unsigned long int)nbrk)<0x10000000) return -1;

	if (t->asp->brk>((unsigned long int)nbrk)) {
		sys_printf("brk reduce: check for page free\n");
		return 0;
	}
	t->asp->brk=((unsigned long int)nbrk);
	return 0;
}


extern unsigned long int *curr_pgd;

int load_init(struct task *t) {
	unsigned long brk_save;
	unsigned int brk=0;

	struct elf_phdr phdr;

	int rc=elf_get_first_phdr("/init",&phdr);
	if (rc<0) {
		sys_printf("could not open init\n");
		return -1;
	}

	t->asp->brk=0x10000000;
	t->asp->mmap_vaddr=0x10000000;
	brk_save=current->asp->brk;
	current->asp->brk=t->asp->brk;

	curr_pgd=t->asp->pgd;  /* repoint page table directory while loading */
	set_asid(t->asp->id);

	init_tlb();

	while(1) {
		if (brk<phdr.p.p_vaddr+phdr.p.p_memsz) {
			brk=phdr.p.p_vaddr+phdr.p.p_memsz;
			sys_brk(current,(void *)brk);
		}
		if (phdr.p.p_memsz) {
			sys_lseek(phdr.fd,phdr.p.p_offset,SEEK_SET);
			DEBUGP(DSYS_LOAD,DLEV_INFO, "load: at %x size %x\n", phdr.p.p_vaddr, phdr.p.p_memsz);
			sys_read(phdr.fd,(void *)phdr.p.p_vaddr,phdr.p.p_filesz);
		}
		if (phdr.p.p_memsz-phdr.p.p_filesz) {
			DEBUGP(DSYS_LOAD,DLEV_INFO, "zero out %d bytes at %x\n",
				phdr.p.p_memsz-phdr.p.p_filesz,
				phdr.p.p_vaddr+phdr.p.p_memsz);
			memset((void *)phdr.p.p_vaddr+phdr.p.p_filesz,0,phdr.p.p_memsz-phdr.p.p_filesz);
		}

		rc=elf_get_next_phdr(&phdr);
		if (rc!=1) break;
	}

	elf_close(&phdr);

	t->asp->brk=current->asp->brk;
	current->asp->brk=brk_save;
	curr_pgd=current->asp->pgd;  /* restore page table direcotry */
	set_asid(current->asp->id);
	return 0;
}

int load_binary(char *path) {
	unsigned int brk=0;

	struct elf_phdr phdr;

	int rc=elf_get_first_phdr(path,&phdr);
	if (rc<0) {
		sys_printf("could not open %s\n", path);
		return -1;
	}

	current->asp->brk=0x10000000;
	current->asp->mmap_vaddr=0x10000000;

	while(1) {
		if (brk<phdr.p.p_vaddr+phdr.p.p_memsz) {
			brk=phdr.p.p_vaddr+phdr.p.p_memsz;
			sys_brk(current,(void *)brk);
		}
		if (phdr.p.p_memsz) {
			sys_lseek(phdr.fd,phdr.p.p_offset,SEEK_SET);
			DEBUGP(DSYS_LOAD,DLEV_INFO, "load: at %x size %x\n", phdr.p.p_vaddr, phdr.p.p_memsz);
			sys_read(phdr.fd,(void *)phdr.p.p_vaddr,phdr.p.p_filesz);
		}
		if (phdr.p.p_memsz-phdr.p.p_filesz) {
			DEBUGP(DSYS_LOAD,DLEV_INFO, "zero out %d bytes at %x\n",
				phdr.p.p_memsz-phdr.p.p_filesz,
				phdr.p.p_vaddr+phdr.p.p_memsz);
			memset((void *)phdr.p.p_vaddr+phdr.p.p_filesz,0,phdr.p.p_memsz-phdr.p.p_filesz);
		}

		rc=elf_get_next_phdr(&phdr);
		if (rc!=1) break;
	}

	elf_close(&phdr);

	return 0;
}
