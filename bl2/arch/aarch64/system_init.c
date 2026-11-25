
#include <sys.h>
#include <string.h>
#include <devices.h>
#include "sys_const.h"

#include "dtb.h"
#include "yaffs_guts.h"
#include "yaffs_osglue.h"

#include <elf.h>

//#include "mm_A53.h"

extern void switch_user_asp(struct address_space  *);
extern void serial_puts(char *s);

//extern unsigned long *curr_pgd;

void _exit(int status) {
}

extern unsigned long __bss_start__;
extern unsigned long __bss_end__;

//void set_asid(unsigned int asid);

static unsigned long idle_stack[PAGE_SIZE / sizeof(unsigned long)] __attribute__((aligned(16)));

struct address_space kernel_asp = {
.pgd	= 0,
.id	= 0,
.brk	= 0
};

struct task idle_task = {
        .name       = "idle:000",
        .sp         = (void *)((unsigned long)idle_stack + sizeof(idle_stack)),
        .id         = 256,
        .next       = 0,
        .next2      = 0,
        .state      = 1,
        .prio_flags = 4,
        .estack     = (void *)((unsigned long)idle_stack + sizeof(idle_stack) - 0x800),
        .asp        = &kernel_asp
};

//extern int __usr_main(int argc, char **argv);
//void *usr_init=(void *)0xffff000000400000;
//void *usr_init=(void *)0x400000;

void build_free_page_list(void);

//#define SDRAM_START 0x000000000000000UL



void init_sys_arch(void) {
//	unsigned long int psize;
	parse_dtb(VA_START);
	build_free_page_list();
//	psize=(SDRAM_SIZE+SDRAM_START)-__bss_end__;
//	sys_printf("psize=%x\n",psize);
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
        if (((unsigned long int)nbrk)<0x01000000) return -1;

        if (t->asp->brk>((unsigned long int)nbrk)) {
                sys_printf("brk reduce: check for page free\n");
                return 0;
        }
        t->asp->brk=((unsigned long int)nbrk);
        return 0;
}

int load_init(struct task *t,unsigned long int *entry_vaddr) {
	unsigned long brk_save;
	unsigned int brk=0;
	struct address_space *asp_save;

	struct elf_phdr phdr;

	int rc=elf_get_first_phdr("/init",&phdr,entry_vaddr);
	if (rc<0) {
		sys_printf("could not open init\n");
		return -1;
	}

	t->asp->brk=0x1000000;
	t->asp->mmap_vaddr=0x1000000;
	asp_save=current->asp;
	current->asp=t->asp; 	/* repoint page table directory while loading */
	switch_user_asp(t->asp);

	while(1) {
		if (brk<phdr.p64.p_vaddr+phdr.p64.p_memsz) {
			brk=phdr.p64.p_vaddr+phdr.p64.p_memsz;
			sys_brk(current,(void *)brk);
		}
		if (phdr.p64.p_memsz) {
			sys_lseek(phdr.fd,phdr.p64.p_offset,SEEK_SET);
			DEBUGP(DSYS_LOAD,DLEV_INFO, "load: at %x size %x\n", phdr.p64.p_vaddr, phdr.p64.p_memsz);
			sys_read(phdr.fd,(void *)phdr.p64.p_vaddr,phdr.p64.p_filesz);
		}
		if (phdr.p64.p_memsz-phdr.p64.p_filesz) {
			DEBUGP(DSYS_LOAD,DLEV_INFO, "zero out %d bytes at %x\n",
					phdr.p64.p_memsz-phdr.p64.p_filesz,
					phdr.p64.p_vaddr+phdr.p64.p_memsz);
			memset((void *)phdr.p64.p_vaddr+phdr.p64.p_filesz,0,phdr.p64.p_memsz-phdr.p64.p_filesz);
		}

		rc=elf_get_next_phdr(&phdr);
		if (rc!=1) break;
	}

	elf_close(&phdr);

	current->asp=asp_save; 	/* restore page table direcotry */
	switch_user_asp(current->asp);

	return 0;
}

int load_binary(char *path, unsigned long int *entry_vaddr) {
	unsigned int brk=0;
	struct elf_phdr phdr;
	struct address_space *asp_save;

	int rc=elf_get_first_phdr(path,&phdr,entry_vaddr);
	if (rc<0) {
		sys_printf("could not open %s\n", path);
		return -1;
	}

	current->asp->brk=0x1000000;
	current->asp->mmap_vaddr=0x1000000;

//	switch_user_asp(t->asp);

	while(1) {
		if (brk<phdr.p64.p_vaddr+phdr.p64.p_memsz) {
			brk=phdr.p64.p_vaddr+phdr.p64.p_memsz;
			sys_brk(current,(void *)brk);
		}
		if (phdr.p64.p_memsz) {
			sys_lseek(phdr.fd,phdr.p64.p_offset,SEEK_SET);
			DEBUGP(DSYS_LOAD,DLEV_INFO, "load: at %x size %x\n", phdr.p64.p_vaddr, phdr.p64.p_memsz);
			sys_read(phdr.fd,(void *)phdr.p64.p_vaddr,phdr.p64.p_filesz);
		}
		if (phdr.p64.p_memsz-phdr.p64.p_filesz) {
			DEBUGP(DSYS_LOAD,DLEV_INFO, "zero out %d bytes at %x\n",
				phdr.p64.p_memsz-phdr.p64.p_filesz,
				phdr.p64.p_vaddr+phdr.p64.p_memsz);
				memset((void *)phdr.p64.p_vaddr+phdr.p64.p_filesz,0,phdr.p64.p_memsz-phdr.p64.p_filesz);
		}

		rc=elf_get_next_phdr(&phdr);
		if (rc!=1) break;
	}

	elf_close(&phdr);

	return 0;
}

void board_reboot(void) {
}
