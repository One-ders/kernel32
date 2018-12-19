

#define KMEM_GET_MEMINFO	0x1000
#define KMEM_GET_FIRST_PSMAP	0x1001
#define KMEM_GET_NEXT_PSMAP	0x1002
#define KMEM_DUMP_TLB		0x1003
#define KMEM_FLUSH_TLB		0x1004

struct meminfo {
	unsigned int totalRam;
	unsigned int freePages;
	unsigned int sharedPages;
};

struct ps_memmap {
	char *ps_name;
	unsigned int vaddr;
	unsigned int pte;
	unsigned int sh_data;
};
