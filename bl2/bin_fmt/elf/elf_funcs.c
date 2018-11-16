
#if 0
#include <stdio.h>
#include <unistd.h>
#endif

#include <string.h>
#include "sys.h"
#include "elf.h"

#include "yaffsfs.h"


int read_elf_header(int fd, elf32_ehdr *ehdr) {
	if (ehdr) {
		int rc=sys_read(fd,ehdr,sizeof(*ehdr));
		unsigned char elf_mag[]=ELF_MAG;
		if (rc!=sizeof(*ehdr)) return -1;
		if (memcmp(ehdr->e_ident,elf_mag,sizeof(elf_mag))!=0) return -1;
		return 0;
	}
	return -1;
}

int read_elf_phdr(int fd, elf32_phdr *phdr, int offset) {

	int rc=sys_lseek(fd,offset,SEEK_SET);
	if (phdr) {
		int rc=sys_read(fd,phdr,sizeof(*phdr));
		if (rc!=sizeof(*phdr)) return -1;
		return 0;
	}
	return -1;
}

int read_elf_shdr(int fd, elf32_shdr *shdr, int offset) {
	int rc=sys_lseek(fd,offset,SEEK_SET);
	if (shdr) {
		int rc=sys_read(fd,shdr,sizeof(*shdr));
		if (rc!=sizeof(*shdr)) return -1;
		return 0;
	}
	return -1;
}

int elf_get_first_phdr(const char *path,struct elf_phdr *phdr) {
	int fd;
	int rc;
	elf32_ehdr ehdr;

	fd=sys_open(path,O_RDONLY,0);
	if (fd<0) return -1;

	memset(&ehdr,0,sizeof(ehdr));
	rc=read_elf_header(fd,&ehdr);
	if (rc<0) return -1;

	memset(phdr,0,sizeof(*phdr));
	phdr->fd=fd;
	phdr->total=ehdr.e_phnum;
	phdr->num=0;
	phdr->sz=ehdr.e_phentsize;
	phdr->hdr_offset=ehdr.e_phoff;

	if (read_elf_phdr(fd,&phdr->p,phdr->hdr_offset)<0) {
		sys_close(fd);
		return -1;
	}

	phdr->num++;
	phdr->hdr_offset+=phdr->sz;

	return 1;
}
	
int elf_get_next_phdr(struct elf_phdr *phdr) {
	if (phdr->num<phdr->total) {
		if (read_elf_phdr(phdr->fd, &phdr->p, phdr->hdr_offset)<0) {
			return 0;
		}
		phdr->num++;
		phdr->hdr_offset+=phdr->sz;

		return 1;
	}
	return -1;
}

int elf_close(struct elf_phdr *phdr) {
	sys_close(phdr->fd);
	return 0;
}

