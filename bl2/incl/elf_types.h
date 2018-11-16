
typedef unsigned int 		elf32_addr;
typedef unsigned short int	elf32_half;
typedef unsigned int		elf32_off;
typedef int			elf32_sword;
typedef unsigned int		elf32_word;

#define EI_NIDENT	16

typedef struct {
	unsigned char	e_ident[EI_NIDENT];
	elf32_half	e_type;
	elf32_half	e_machine;
	elf32_word	e_version;
	elf32_addr	e_entry;
	elf32_off	e_phoff;
	elf32_off	e_shoff;
	elf32_word	e_flags;
	elf32_half	e_ehsize;
	elf32_half	e_phentsize;
	elf32_half	e_phnum;
	elf32_half	e_shentsize;
	elf32_half	e_shnum;
	elf32_half	e_shstrndx;
} elf32_ehdr;

// Elf Types
#define ET_NONE		0
#define ET_REL		1
#define ET_EXEC		2
#define ET_DYN		3
#define ET_CORE		4
#define ET_LOPROC	0xff00
#define ET_HIPROC	0xffff

// Elf Machine
#define EM_NONE		0	// No machine
#define EM_M32		1	// AT&T WE 32100
#define EM_SPARC	2	// SPARC
#define EM_386		3	// Intel 80386
#define EM_68K		4	// Motorola 68000
#define EM_88K		5	// Motorola 88000
#define EM_860		7	// Intel 80860
#define EM_MIPS		8	// MIPS RS3000

// Elf Version
#define	EV_NONE		0	// Invalid version
#define EV_CURRENT	1	// Current version


struct e_ident {
	unsigned char elf_mag[4];
	unsigned char elf_class;
	unsigned char elf_data;
	unsigned char elf_version;
};

#define ELF_MAG 	{0x7f,'E','L','F'}

#define	EC_NONE		0
#define EC_32		1
#define EC_64		2

#define ED_NONE		0	// Invalid data encoding
#define	ED_2_LSB	1	// Litle endian
#define ED_2_MSB	2	// Big endian

#define SHN_UNDEF	0
#define SHN_LORESERVE	0xff00
#define SHN_LOPROC	0xff00
#define SHN_HIPROC	0xff1f
#define SHN_ABS		0xfff1
#define SHN_COMMON	0xfff2
#define SHN_HIRESERVE	0xffff


typedef struct {
	elf32_word	sh_name;
	elf32_word	sh_type;
	elf32_word	sh_flags;
	elf32_addr	sh_addr;
	elf32_off	sh_offset;
	elf32_word	sh_size;
	elf32_word	sh_link;
	elf32_word	sh_info;
	elf32_word	sh_addralign;
	elf32_word	sh_entsize;
} elf32_shdr;

// Section header type
#define SHT_NULL		0
#define SHT_PROGBITS		1
#define SHT_SYMTAB		2
#define SHT_STRTAB		3
#define SHT_RELA		4
#define SHT_HASH		5
#define SHT_DYNAMIC		6
#define SHT_NOTE		7
#define SHT_NOBITS		8
#define SHT_REL			9
#define SHT_SHLIB		10
#define SHT_DYNSYM		11
#define SHT_LOPROC		0x70000000
#define SHT_HIPROC		0x7fffffff
#define SHT_LOUSER		0x80000000
#define SHT_HIUSER		0xffffffff

// Section header flags
#define SHF_WRITE		0x1
#define SHF_ALLOC		0x2
#define SHF_EXECINSTR		0x4
#define SHF_MASKPROC		0xf0000000


typedef struct {
	elf32_word	st_name;
	elf32_addr	st_value;
	elf32_word	st_size;
	unsigned char	st_info;
	unsigned char	st_other;
	elf32_half	st_shndx;
} elf32_sym;

// Section header info STB | STT
#define STB_LOCAL	0x00
#define STB_GLOBAL	0x10
#define STB_WEAK	0x20
#define STB_LOPROC	0xd0
#define STB_HIPROC	0xf0

#define STT_NOTYPE	0x00
#define STT_OBJECT	0x01
#define STT_FUNC	0x02
#define STT_SECTION	0x03
#define STT_FILE	0x04
#define STT_LOPROC	0x0d
#define STT_HIPROC	0x0f

typedef struct {
	elf32_addr	r_offset;
	elf32_word	r_info;
} elf32_rel;

typedef struct {
	elf32_addr	r_offset;
	elf32_word	r_info;
	elf32_sword	r_addend;
} elf32_rela;


typedef struct {
	elf32_word	p_type;
	elf32_off	p_offset;
	elf32_addr	p_vaddr;
	elf32_addr	p_paddr;
	elf32_word	p_filesz;
	elf32_word	p_memsz;
	elf32_word	p_flags;
	elf32_word	p_align;
} elf32_phdr;

// P type
#define PT_NULL		0
#define PT_LOAD		1
#define PT_DYNAMIC	2
#define PT_INTERP	3
#define PT_NOTE		4
#define PT_SHLIB	5
#define	PT_PHDR		6
#define PT_LOPROC	0x70000000
#define PT_HIPROC	0x7fffffff

