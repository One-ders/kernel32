#include <io.h>
#include <sys.h>
#include <string.h>


static unsigned long free_page_map[SDRAM_SIZE/(PAGE_SIZE*8*sizeof(unsigned long))]
;

#define SDRAM_START 0x0000000000000000
#define PHYS2PAGE(a)    ((((unsigned long int)(a))&0x7fffffff)>>12)
#define set_in_use(a,b) ((a)[((unsigned long int)(b))/64]&=~(1<<(((unsigned long int)(b))%64)))
#define set_free(a,b) ((a)[((unsigned long int)(b))/64]|=(1<<(((unsigned long int)(b))%64)))

extern unsigned long __bss_start__;
extern unsigned long __bss_end__;

struct page_info  {
        unsigned int    pfn;
        int             ref_count;
};

void build_free_page_list() {
        unsigned long int mptr=SDRAM_START;
        int i;

        for(i=0;i<(sizeof(free_page_map)/sizeof(unsigned long int));i++) {
                free_page_map[i]=~0;
        }

        while(mptr<((unsigned long int)&__bss_end__)) {
                set_in_use(free_page_map,PHYS2PAGE(mptr));
#if 1
                sys_printf("build_free_page_list: set %x non-free, map %x\n",mptr, free_page_map[0]);
                sys_printf("map index %x, mapmask %x\n",PHYS2PAGE(mptr)/64,
                                ~(1<<((PHYS2PAGE(mptr))%64)));
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
                      sys_printf("found free page att index %x, bit %x\n",
                                      i,j);
                        j--;
                        free_page_map[i]&=~(1<<j);
                      sys_printf("return page %x\n",(SDRAM_START + (i*64*4096)
+ (j*4096)));
                        return (void *)(SDRAM_START + (i*64*4096) + (j*4096));
                }
        }
        return 0;
}

void put_page(void *p) {
	sys_printf("put_page %x\n", p);
        set_free(free_page_map,PHYS2PAGE(p));
	return;
}

int count_free_pages(void) {
	int i;
	int res=0;
	for(i=0;i<(sizeof(free_page_map)/sizeof(unsigned long int));i++) {
		if (free_page_map[i]) {
			if (free_page_map[i]==0xffffffffffffffffL) {
				res+=64;
			} else {
				int j;
				for(j=0;j<63;j++) {
					if ((1<<j)&free_page_map[i]) {
						res++;
					}
				}
			}
		}
	}
	return res;
}


int flush_tlb(void) {
	return 0;
}

void *get_pages(unsigned int order) {
	return 0;
}

int dump_tlb(void) {
	return 0;
}

