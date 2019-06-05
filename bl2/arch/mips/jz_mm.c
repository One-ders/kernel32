#include <io.h>
#include <sys.h>
#include <string.h>

static unsigned long free_page_map[SDRAM_SIZE/(PAGE_SIZE*8*sizeof(unsigned long))]
;
#define PHYS2PAGE(a)    ((((unsigned long int)(a))&0x7fffffff)>>12)
#define set_in_use(a,b) ((a)[((unsigned long int)(b))/32]&=~(1<<(((unsigned long int)(b))%32)))
#define set_free(a,b) ((a)[((unsigned long int)(b))/32]|=(1<<(((unsigned long int)(b))%32)))

extern unsigned long __bss_start__;
extern unsigned long __bss_end__;

struct page_info  {
	unsigned int	pfn;
	int		ref_count;
};


void build_free_page_list() {
	unsigned long int mptr=0x80000000;
	int i;
//	sys_printf("build_free_page_list\n");

	for(i=0;i<(sizeof(free_page_map)/sizeof(unsigned long int));i++) {
		free_page_map[i]=~0;
	}

	while(mptr<((unsigned long int)&__bss_end__)) {
		set_in_use(free_page_map,PHYS2PAGE(mptr));
#if 0
		sys_printf("build_free_page_list: set %x non-free, map %x\n",mptr, free_page_map[0]);
		sys_printf("map index %x, mapmask %x\n",PHYS2PAGE(mptr)/32,
				~(1<<((PHYS2PAGE(mptr))%32)));
#endif
		mptr+=4096;
	}
}


void *get_page(void) {
	int i;
	for(i=0;i<(sizeof(free_page_map)/sizeof(unsigned long int));i++) {
		if (free_page_map[i]) {
			int j=ffsl(free_page_map[i]);
			if (!j) return 0;
//                      sys_printf("found free page att index %x, bit %x\n",
//                                      i,j);
			j--;
			free_page_map[i]&=~(1<<j);
//			sys_printf("return page %x\n",(0x80000000 + (i*32*4096) + (j*4096)));
			return (void *)(0x80000000 + (i*32*4096) + (j*4096));
		}
	}
	return 0;
}

int count_free_pages(void) {
	int i;
	int res=0;
	for(i=0;i<(sizeof(free_page_map)/sizeof(unsigned long int));i++) {
		if (free_page_map[i]) {
			if (free_page_map[i]==0xffffffff) {
				res+=32;
			} else {
				int j;
				for(j=0;j<31;j++) {
					if ((1<<j)&free_page_map[i]) {
						res++;
					}
				}
			}
		}
	}
	return res;
}

void put_page(void *p) {
//	sys_printf("put_page %x\n", p);
	set_free(free_page_map,PHYS2PAGE(p));
}

void *get_pages(unsigned int order) {
	int i;
	int num=1<<order;
//	int pmask=(1<<num)-1;
	if (!num) return 0;
	int need_slots=((num-1)/32)+1;
	int have_slots=0;
	int start_slot=0;
	int last_slot=0;

	DEBUGP(DSYS_MEM,DLEV_INFO,"get_pages: num=%d,need_slots=%x\n", num, need_slots);
	for(i=0;i<(sizeof(free_page_map)/sizeof(unsigned long int));i++) {
		if (free_page_map[i]==0xffffffff) {
			if (!have_slots) {
				start_slot=i;
			}
			have_slots++;
			if (have_slots==need_slots) {
				break;
			}
		} else {
			have_slots=0;
		}
	}

	DEBUGP(DSYS_MEM,DLEV_INFO,"have_slots %d, start_slot %d, need %d, have %d\n",
			have_slots, start_slot, num, 32*have_slots);

	if (have_slots!=need_slots) return 0;
	last_slot=start_slot+have_slots-1;
	for(i=start_slot;i<last_slot;i++) {
		free_page_map[i]=0;
		num-=32;
	}
	for(i=0;i<num;i++) {
		free_page_map[last_slot]&=~(1<<i);
	}


	return (void *)(0x80000000 + (start_slot*32*4096));
}

unsigned int virt_to_phys(void *v) {
//	return (unsigned int) v;
	return ((unsigned int)v&~0x80000000);
}

/*******************************************************/

#define cache32_unroll32(base,op)                                       \
        __asm__ __volatile__(                                           \
        "       .set push                                       \n"     \
        "       .set noreorder                                  \n"     \
        "       .set mips3                                      \n"     \
        "       cache %1, 0x000(%0); cache %1, 0x020(%0)        \n"     \
        "       cache %1, 0x040(%0); cache %1, 0x060(%0)        \n"     \
        "       cache %1, 0x080(%0); cache %1, 0x0a0(%0)        \n"     \
        "       cache %1, 0x0c0(%0); cache %1, 0x0e0(%0)        \n"     \
        "       cache %1, 0x100(%0); cache %1, 0x120(%0)        \n"     \
        "       cache %1, 0x140(%0); cache %1, 0x160(%0)        \n"     \
        "       cache %1, 0x180(%0); cache %1, 0x1a0(%0)        \n"     \
        "       cache %1, 0x1c0(%0); cache %1, 0x1e0(%0)        \n"     \
        "       cache %1, 0x200(%0); cache %1, 0x220(%0)        \n"     \
        "       cache %1, 0x240(%0); cache %1, 0x260(%0)        \n"     \
        "       cache %1, 0x280(%0); cache %1, 0x2a0(%0)        \n"     \
        "       cache %1, 0x2c0(%0); cache %1, 0x2e0(%0)        \n"     \
        "       cache %1, 0x300(%0); cache %1, 0x320(%0)        \n"     \
        "       cache %1, 0x340(%0); cache %1, 0x360(%0)        \n"     \
        "       cache %1, 0x380(%0); cache %1, 0x3a0(%0)        \n"     \
        "       cache %1, 0x3c0(%0); cache %1, 0x3e0(%0)        \n"     \
        "       .set pop                                        \n"     \
                :                                                       \
                : "r" (base),                                           \
                  "i" (op));

#define cache_op(op,addr)                                               \
        __asm__ __volatile__(                                           \
        "       .set    push                                    \n"     \
        "       .set    noreorder                               \n"     \
        "       .set    mips3\n\t                               \n"     \
        "       cache   %0, %1                                  \n"     \
        "       .set    pop                                     \n"     \
        :                                                               \
        : "i" (op), "R" (*(unsigned char *)(addr)))




#define MIPS_CPU_PREFETCH       0x00040000 /* CPU has usable prefetch */


unsigned int read_c0_prid(void);
unsigned int read_c0_config1(void);
void set_c0_index(unsigned int index);
void set_c0_lo0(unsigned int value);
void set_c0_lo1(unsigned int value);
void set_c0_hi(unsigned int value);
void set_c0_pagemask(unsigned int value);
unsigned int read_c0_index();
unsigned int read_c0_random();
unsigned int read_c0_pagemask();
unsigned int read_c0_wired();
unsigned int read_c0_lo0(void);
unsigned int read_c0_lo1(void);
unsigned int read_c0_hi(void);

struct cache_desc {
        unsigned int waysize;   /* Bytes per way */
        unsigned short sets;    /* Number of lines per set */
        unsigned char ways;     /* Number of ways */
        unsigned char linesz;   /* Size of line in bytes */
        unsigned char waybit;   /* Bits to select in a cache set */
        unsigned char flags;    /* Flags describing cache properties */
};


/*
 * Flag definitions
 */
#define MIPS_CACHE_NOT_PRESENT  0x00000001
#define MIPS_CACHE_VTAG         0x00000002      /* Virtually tagged cache */
#define MIPS_CACHE_ALIASES      0x00000004      /* Cache could have aliases */
#define MIPS_CACHE_IC_F_DC      0x00000008      /* Ic can refill from D-cache */
#define MIPS_IC_SNOOPS_REMOTE   0x00000010      /* Ic snoops remote stores */
#define MIPS_CACHE_PINDEX       0x00000020      /* Physically indexed cache */





struct cpuinfo_mips {
        unsigned long           udelay_val;
        unsigned long           asid_cache;

        /*
         * Capability and feature descriptor structure for MIPS CPU
         */
        unsigned long           options;
        unsigned long           ases;
        unsigned int            processor_id;
        unsigned int            fpu_id;
        unsigned int            cputype;
        int                     isa_level;
        int                     tlbsize;
        struct cache_desc       icache; /* Primary I-cache */
        struct cache_desc       dcache; /* Primary D or combined I/D cache */
        struct cache_desc       scache; /* Secondary cache */
        struct cache_desc       tcache; /* Tertiary/split secondary cache */
        int                     srsets; /* Shadow register sets */
        void                    *data;  /* Additional data */
};



struct cpuinfo_mips current_cpu_data;

#define cpu_has_vtag_icache	(current_cpu_data.icache.flags & MIPS_CACHE_VTAG)

static unsigned long icache_size;
static unsigned long dcache_size;

static char *way_string[] = { NULL, "direct mapped", "2-way",
        "3-way", "4-way", "5-way", "6-way", "7-way", "8-way"
};


void probe_pcache(void) {
	struct cpuinfo_mips *c = &current_cpu_data;
	unsigned int prid = read_c0_prid();
	unsigned long config1;
	unsigned int lsize;


	config1 = read_c0_config1();
	config1 = (config1 >> 22) & 0x07;
	if (config1 == 0x07)
		config1 = 10;
	else
		config1 = config1 + 11;
	config1 += 2;
	icache_size = (1 << config1);
	c->icache.linesz = 32;
	c->icache.ways = 4;
	c->icache.waybit = ffs(icache_size / c->icache.ways);

	config1 = read_c0_config1();
	config1 = (config1 >> 13) & 0x07;
	if (config1 == 0x07)
		config1 = 10;
	else
		config1 = config1 + 11;
	config1 += 2;
	dcache_size = (1 << config1);
	c->dcache.linesz = 32;
	c->dcache.ways = 4;
	c->dcache.waybit = ffs(dcache_size / c->dcache.ways);

	c->dcache.flags = 0;
	c->options |= MIPS_CPU_PREFETCH;

	/* compute a couple of other cache variables */
	c->icache.waysize = icache_size / c->icache.ways;
	c->dcache.waysize = dcache_size / c->dcache.ways;

	c->icache.sets = c->icache.linesz ?
		icache_size / (c->icache.linesz * c->icache.ways) : 0;
	c->dcache.sets = c->dcache.linesz ?
		dcache_size / (c->dcache.linesz * c->dcache.ways) : 0;

	if (c->dcache.waysize > PAGE_SIZE)
		c->dcache.flags |= MIPS_CACHE_ALIASES;

	sys_printf("Primary instruction cache %dkB, %s, %s, linesize %d bytes.\n",
		icache_size >> 10,
		cpu_has_vtag_icache ? "VIVT" : "VIPT",
		way_string[c->icache.ways], c->icache.linesz);

	sys_printf("Primary data cache %dkB, %s, %s, %s, linesize %d bytes\n",
		dcache_size >> 10, way_string[c->dcache.ways],
		(c->dcache.flags & MIPS_CACHE_PINDEX) ? "PIPT" : "VIPT",
		(c->dcache.flags & MIPS_CACHE_ALIASES) ?
			"cache aliases" : "no aliases",
		c->dcache.linesz);
}





#define SYNC_WB() __asm__ __volatile__ ("sync")
#define INDEX_BASE      KSEG0





static inline void blast_dcache32(void) {
        unsigned long start = INDEX_BASE;
        unsigned long end = start + current_cpu_data.dcache.waysize;
        unsigned long ws_inc = 1UL << current_cpu_data.dcache.waybit;
        unsigned long ws_end = current_cpu_data.dcache.ways <<
                               current_cpu_data.dcache.waybit;
        unsigned long ws, addr;

        for (ws = 0; ws < ws_end; ws += ws_inc)
                for (addr = start; addr < end; addr += 0x400)
                        cache32_unroll32(addr|ws,Index_Writeback_Inv_D);

        SYNC_WB();
}

static inline void blast_dcache_range(unsigned long start,
                                      unsigned long end) {
	unsigned long lsize = 32;
	unsigned long addr = start & ~(lsize - 1);
	unsigned long aend = (end - 1) & ~(lsize - 1);

	while (1) {
		cache_op(Hit_Writeback_Inv_D, addr);
		if (addr == aend)
			break;
		addr += lsize;
	}
	SYNC_WB();
}

void dma_cache_wback_inv(unsigned long addr, unsigned long size)
{
        /* Catch bad driver code */
        if (size == 0) {
		sys_printf("r4k_dma_cache_wback_inv\n");
		while(1);
	}

#if 0
        if (cpu_has_inclusive_pcaches) {
                if (size >= scache_size)
                        r4k_blast_scache();
                else
                        blast_scache_range(addr, addr + size);
                return;
        }

        /*
         * Either no secondary cache or the available caches don't have the
         * subset property so we have to flush the primary caches
         * explicitly
         */
#endif
        if (size >= dcache_size) {
                blast_dcache32();
        } else {
//                R4600_HIT_CACHEOP_WAR_IMPL;
                blast_dcache_range(addr, addr + size);
        }

//        bc_wback_inv(addr, size);
}

void dma_cache_wback(unsigned long addr, unsigned long size) {
	dma_cache_wback_inv(addr,size);
}

/*************************************************************/

int dump_tlb(void) {
	int top_tlb_entry;
	int i;
	unsigned long int cpu_flags;

	top_tlb_entry=(read_c0_config1()>>25)&0x3f;

	sys_printf("JZ tlb has %d entries\n", top_tlb_entry+1);

	cpu_flags=disable_interrupts();
	sys_printf("index_register %x\n", read_c0_index());
	sys_printf("random_register %x\n", read_c0_random());
	sys_printf("pageMask: %x\n", read_c0_pagemask());
	sys_printf("Wired register: %x\n", read_c0_wired());
	
	for(i=0;i<=top_tlb_entry;i++) {
		unsigned long int entryHi;
		unsigned long int entryLo0;
		unsigned long int entryLo1;

		set_c0_index(i);
		asm("tlbr");
		entryHi=read_c0_hi();
		entryLo0=read_c0_lo0();
		entryLo1=read_c0_lo1();
		set_c0_hi(current->asp->id);
		sys_printf("%04d hi(vaddr):%08x lo0:%08x, lo1:%08x\n",
			i,entryHi,entryLo0,entryLo1);
	}
	restore_cpu_flags(cpu_flags);
	return 0;
}

#define UNIQUE_ENTRYHI(idx) (0x80000000 + ((idx) << (PAGE_SHIFT + 1)))



int flush_tlb(void) {
	int top_tlb_entry;
	int i;
	unsigned int old_hi;
	unsigned long int cpu_flags=disable_interrupts();

	top_tlb_entry=(read_c0_config1()>>25)&0x3f;

	old_hi=read_c0_hi();
	set_c0_lo0(0);
	set_c0_lo1(0);

	for(i=0;i<=top_tlb_entry;i++) {
		unsigned long int hi;
		hi=UNIQUE_ENTRYHI(i);
		set_c0_hi(hi);
		set_c0_index(i);
		asm("ssnop;ssnop;ssnop;ssnop;ssnop");
		asm("tlbwi");
	}
	asm("ssnop;ssnop;ssnop;ssnop;ssnop");
	set_c0_hi(old_hi);
	restore_cpu_flags(cpu_flags);
	return 0;
}

void init_tlb() {
	set_c0_pagemask(0);
	flush_tlb();
}
