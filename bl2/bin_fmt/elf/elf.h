#include "elf_types.h"

int read_elf_32_header(int fd, elf32_ehdr *ehdr);
int read_elf_64_header(int fd, elf64_ehdr *ehdr);
int read_elf_32_phdr(int fd, elf32_phdr *phdr, int offset);
int read_elf_64_phdr(int fd, elf64_phdr *phdr, int offset);
int read_elf_32_shdr(int fd, elf32_shdr *shdr, int offset);
int read_elf_64_shdr(int fd, elf64_shdr *shdr, int offset);

struct elf_phdr {
	int fd;
	int eclass;
	int total;
	int num;
	int sz;
	unsigned int hdr_offset;
	union {
		elf64_phdr p64;
		elf32_phdr p32;
	};
};

int elf_get_first_phdr(const char *,struct elf_phdr *, unsigned int long *entry_vaddr);
int elf_get_next_phdr(struct elf_phdr *);
int elf_close(struct elf_phdr *);
