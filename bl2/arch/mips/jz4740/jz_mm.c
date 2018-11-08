#include <sys.h>
#include <string.h>

static unsigned long free_page_map[SDRAM_SIZE/(PAGE_SIZE*sizeof(unsigned long))]
;
#define PHYS2PAGE(a)    ((((unsigned long int)(a))&0x7fffffff)>>12)
#define set_in_use(a,b) ((a)[((unsigned long int)(b))/32]&=~(1<<(((unsigned long int)(b))%32)))
#define set_free(a,b) ((a)[((unsigned long int)(b))/32]|=(1<<(((unsigned long int)(b))%32)))

extern unsigned long __bss_start__;
extern unsigned long __bss_end__;


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
			return (void *)(0x80000000 + (i*32*4096) + (j*4096));
		}
	}
	return 0;
}

void put_page(void *p) {
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

	sys_printf("get_pages: num=%d,need_slots=%x\n", num, need_slots);
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

	sys_printf("have_slots %d, start_slot %d, need %d, have %d\n",
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
