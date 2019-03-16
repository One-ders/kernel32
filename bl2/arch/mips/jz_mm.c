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

	if (!have_slots) return 0;
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

unsigned int read_c0_config1(void);
void set_c0_index(unsigned int index);
void set_c0_lo0(unsigned int value);
void set_c0_lo1(unsigned int value);
void set_c0_hi(unsigned int value);
unsigned int read_c0_index();
unsigned int read_c0_random();
unsigned int read_c0_pagemask();
unsigned int read_c0_wired();
unsigned int read_c0_lo0(void);
unsigned int read_c0_lo1(void);
unsigned int read_c0_hi(void);


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

sys_printf("looping the tlb\n");
	for(i=0;i<=top_tlb_entry;i++) {
		unsigned long int hi;
		sys_printf("loop entry %d\n",i);
		hi=UNIQUE_ENTRYHI(i);
		sys_printf("loop entry %d to %x \n",i,hi);
		set_c0_hi(hi);
sys_printf("wait after set hi\n");
		set_c0_index(i);
		asm("ssnop;ssnop;ssnop;ssnop;ssnop");
sys_printf("wait after set ix\n");
		asm("tlbwi");
	}
sys_printf("loop done\n");
	asm("ssnop;ssnop;ssnop;ssnop;ssnop");
	set_c0_hi(old_hi);
sys_printf("wait after set hi, enable irq\n");
	restore_cpu_flags(cpu_flags);
	return 0;
}

void init_tlb() {

	set_c0_pagemask(0);
	flush_tlb();

}
