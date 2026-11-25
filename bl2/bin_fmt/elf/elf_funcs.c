
#if 0
#include <stdio.h>
#include <unistd.h>
#endif

#include <string.h>
#include "sys.h"
#include "elf.h"

#include "yaffsfs.h"


int check_elf_magic(int fd) {
	unsigned char e[EI_NIDENT];
	int rc=sys_read(fd,e,sizeof(e));
	unsigned char elf_mag[]=ELF_MAG;
	if (rc!=sizeof(e)) return -1;
	if (memcmp(e,elf_mag,sizeof(elf_mag))!=0) return -1;
	return e[EI_CLASS];
}

int read_elf32_header(int fd, elf32_ehdr *ehdr) {
	if (ehdr) {
		int rc=sys_lseek(fd,0,SEEK_SET);
		rc=sys_read(fd,ehdr,sizeof(*ehdr));
		if (rc!=sizeof(*ehdr)) return -1;
		return 0;
	}
	return -1;
}

int read_elf64_header(int fd, elf64_ehdr *ehdr) {
	if (ehdr) {
		int rc=sys_lseek(fd,0,SEEK_SET);
		rc=sys_read(fd,ehdr,sizeof(*ehdr));
		if (rc!=sizeof(*ehdr)) return -1;
		return 0;
	}
	return -1;
}

int read_elf32_phdr(int fd, elf32_phdr *phdr, int offset) {

	int rc=sys_lseek(fd,offset,SEEK_SET);
	if (phdr) {
		int rc=sys_read(fd,phdr,sizeof(*phdr));
		if (rc!=sizeof(*phdr)) return -1;
		return 0;
	}
	return -1;
}

int read_elf64_phdr(int fd, elf64_phdr *phdr, int offset) {

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

int elf_get_first_phdr(const char *path,struct elf_phdr *phdr, unsigned long int *entry_vaddr) {
	int fd;
	int rc;
	int eclass;
	elf32_ehdr ehdr_32;
	elf64_ehdr ehdr_64;

	fd=sys_open(path,O_RDONLY,0);
	if (fd<0) return -1;

	eclass=check_elf_magic(fd);

	memset(phdr,0,sizeof(*phdr));
	if (eclass==ELFCLASS32) {
		memset(&ehdr_32,0,sizeof(ehdr_32));
		rc=read_elf32_header(fd,&ehdr_32);
		if (rc<0) return -1;

		if (entry_vaddr) {
			*entry_vaddr=ehdr_32.e_entry;
		}
		phdr->fd=fd;
		phdr->eclass=eclass;
		phdr->total=ehdr_32.e_phnum;
		phdr->num=0;
		phdr->sz=ehdr_32.e_phentsize;
		phdr->hdr_offset=ehdr_32.e_phoff;

		if (read_elf32_phdr(fd,&phdr->p32,phdr->hdr_offset)<0) {
			sys_close(fd);
			return -1;
		}

		phdr->num++;
		phdr->hdr_offset+=phdr->sz;

	} else if (eclass==ELFCLASS64) {
		memset(&ehdr_64,0,sizeof(ehdr_64));
		rc=read_elf64_header(fd,&ehdr_64);
		if (rc<0) return -1;

		if (entry_vaddr) {
			*entry_vaddr=ehdr_64.e_entry;
		}
		phdr->fd=fd;
		phdr->eclass=eclass;
		phdr->total=ehdr_64.e_phnum;
		phdr->num=0;
		phdr->sz=ehdr_64.e_phentsize;
		phdr->hdr_offset=ehdr_64.e_phoff;

		if (read_elf64_phdr(fd,&phdr->p64,phdr->hdr_offset)<0) {
			sys_close(fd);
			return -1;
		}

		phdr->num++;
		phdr->hdr_offset+=phdr->sz;

	} else {
		return -1;
	}
	return 1;
}

int elf_get_next_phdr(struct elf_phdr *phdr) {
	if (phdr->num<phdr->total) {
		if (phdr->eclass==ELFCLASS32) {
			if (read_elf32_phdr(phdr->fd, &phdr->p32, phdr->hdr_offset)<0) {
				return 0;
			}
			phdr->num++;
			phdr->hdr_offset+=phdr->sz;
		} else if (phdr->eclass==ELFCLASS64) {
			if (read_elf64_phdr(phdr->fd, &phdr->p64, phdr->hdr_offset)<0) {
				return 0;
			}
			phdr->num++;
			phdr->hdr_offset+=phdr->sz;
		} else {
			return -1;
		}

		return 1;
	}
	return -1;
}

int elf_close(struct elf_phdr *phdr) {
	sys_close(phdr->fd);
	return 0;
}

