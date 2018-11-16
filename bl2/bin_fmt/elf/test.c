
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include "elf.h"





int main(int argc, char **argv) {
	int fd;
	int rc;

	elf32_ehdr ehdr;
	elf32_shdr shdr;
	elf32_phdr phdr;

	int i;

	if (argc!=2) {
		printf("give filename of file\n");
		return -1;
	}

	fd=open(argv[1],O_RDONLY);
	
	if (fd<0) {
		printf("could not open file %s\n", argv[1]);
		return -1;
	}

	memset(&ehdr,0,sizeof(ehdr));
	rc=read_elf_header(fd,&ehdr);

	if (rc<0) {
		printf("failed to read elf header\n");
		return -1;
	}

	printf("Elf header\n");
	printf("e_type		%04x\n",ehdr.e_type);
	printf("e_machine	%04x\n",ehdr.e_machine);
	printf("e_version	%08x\n",ehdr.e_version);
	printf("e_entry		%08x\n",ehdr.e_entry);
	printf("e_phoff		%08x\n",ehdr.e_phoff);
	printf("e_shoff		%08x\n",ehdr.e_shoff);
	printf("e_flags		%08x\n",ehdr.e_flags);
	printf("e_ehsize	%04x\n",ehdr.e_ehsize);
	printf("e_phentsize	%04x\n",ehdr.e_phentsize);
	printf("e_phnum		%04x\n",ehdr.e_phnum);
	printf("e_shentsize	%04x\n",ehdr.e_shentsize);
	printf("e_shnum		%04x\n",ehdr.e_shnum);
	printf("e_shstrndx	%04x\n",ehdr.e_shstrndx);


	i=0;
	while (i<ehdr.e_shnum) {
		rc=read_elf_shdr(fd,&shdr,ehdr.e_shoff+(ehdr.e_shentsize*i));
		i++;
		if (rc<0) {
			printf("failed to read shdr\n");
			continue;
		}
		printf("Elf Section header %d\n",i);
		printf("sh_name:	%08x\n",shdr.sh_name);
		printf("sh_type:	%08x\n",shdr.sh_type);
		printf("sh_flags:	%08x\n",shdr.sh_flags);
		printf("sh_addr:	%08x\n",shdr.sh_addr);
		printf("sh_offset:	%08x\n",shdr.sh_offset);
		printf("sh_size:	%08x\n",shdr.sh_size);
		printf("sh_link:	%08x\n",shdr.sh_link);
		printf("sh_info:	%08x\n",shdr.sh_info);
		printf("sh_addralign:	%08x\n",shdr.sh_addralign);
		printf("sh_entsize:	%08x\n",shdr.sh_entsize);
	}

	i=0;
	while (i<ehdr.e_phnum) {
		rc=read_elf_phdr(fd,&phdr,ehdr.e_phoff+(ehdr.e_phentsize*i));
		i++;
		if (rc<0) {
			printf("failed to read phder\n");
			continue;
		}
		printf("Elf Program header %d\n",i);
		printf("p_type:		%08x\n",phdr.p_type);
		printf("p_offset:	%08x\n",phdr.p_offset);
		printf("p_vaddr:	%08x\n",phdr.p_vaddr);
		printf("p_paddr:	%08x\n",phdr.p_paddr);
		printf("p_filesz:	%08x\n",phdr.p_filesz);
		printf("p_memsz:	%08x\n",phdr.p_memsz);
		printf("p_flags:	%08x\n",phdr.p_flags);
		printf("p_align:	%08x\n",phdr.p_align);
	}

}
