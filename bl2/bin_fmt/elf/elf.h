#include "elf_types.h"

int read_elf_header(int fd, elf32_ehdr *ehdr);
int read_elf_phdr(int fd, elf32_phdr *phdr, int offset);
int read_elf_shdr(int fd, elf32_shdr *shdr, int offset);

struct elf_phdr {
	int fd;
	int total;
	int num;
	int sz;
	int hdr_offset;
	elf32_phdr p;
};

int elf_get_first_phdr(const char *,struct elf_phdr *);
int elf_get_next_phdr(struct elf_phdr *);
int elf_close(struct elf_phdr *);
