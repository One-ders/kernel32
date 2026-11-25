#include <io.h>
#include <sys.h>
#include <string.h>

#include "sys_const.h"
#include "dtb.h"
#include <devices.h>

static unsigned long free_page_map[SDRAM_SIZE/(PAGE_SIZE*8*sizeof(unsigned long))]
;

#define SDRAM_START	0x0000000000000000UL
#define PHYS2PAGE(a)    ((((unsigned long int)a)&0x7fffffffUL)>>12)
#define VIRT2PAGE(a)    ((((unsigned long int)a)&0x7fffffffUL)>>12)
#define set_in_use(a,b) ((a)[(((unsigned long int)b)/64UL)]&=~(1UL<<(((unsigned long int)b)%64UL)))
#define set_free(a,b) ((a)[(((unsigned long int)b)/64)]|=(1UL<<(((unsigned long int)b)%64)))

extern unsigned long __bss_start__;
extern unsigned long __bss_end__;
extern unsigned long __static_mem_end__;
extern unsigned long __kernel_start__;

struct page_info  {
        unsigned int    pfn;
        int             ref_count;
};

static void reserve_phys_range(unsigned long start, unsigned long end) {
        unsigned long p = start & PAGE_MASK;
        unsigned long end_aligned = (end + PAGE_SIZE - 1) & PAGE_MASK;

        while (p < end_aligned) {
                set_in_use(free_page_map, PHYS2PAGE(p));
                p += PAGE_SIZE;
        }
}

void build_free_page_list(void) {
        unsigned long total_ram = phys_mem_size ? phys_mem_size : PHYS_MEMORY_SIZE;
        unsigned long total_pages = total_ram / PAGE_SIZE;
        unsigned long map_words = sizeof(free_page_map) / sizeof(unsigned long);

        /* 1. Markera allt upp till total_ram som fritt (~0), och allt ovanför som upptaget (0) */
        for (unsigned long i = 0; i < map_words; i++) {
                if ((i * 64) < total_pages)
                        free_page_map[i] = ~0UL;
                else
                        free_page_map[i] = 0UL;
        }

        /* 2. Reservera sida 0 (ARM boot-stub + Core 1-3 spin-tables vid 0xd8..0xf0) */
        reserve_phys_range(0x0, PAGE_SIZE);

        /* 3. Reservera kärnan, BSS, pg_dir_121 och pg_dir */
        reserve_phys_range((unsigned long)KV2P(&__kernel_start__),
                           (unsigned long)KV2P(&__static_mem_end__));

        /* 4. Reservera Device Tree Blob (DTB) */
        if (dtb_phys_addr && dtb_totalsize) {
                reserve_phys_range(dtb_phys_addr, dtb_phys_addr + dtb_totalsize);
        }

        /* 5. Reservera ramdisk / NAND-image (initrd) */
        if (initrd_phys_start && initrd_phys_end > initrd_phys_start) {
                reserve_phys_range(initrd_phys_start, initrd_phys_end);
        }

        /* 6. Reservera MMIO-fönstret om total_ram råkar sträcka sig upp mot DEVICE_BASE */
        if (total_ram > DEVICE_BASE) {
                reserve_phys_range(DEVICE_BASE, total_ram);
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
//                      sys_printf("return page %x\n",(SDRAM_START + (i*64*4096)
//+ (j*4096)));
                        return (void *)(VA_START + (i*64*4096) + (j*4096));
                }
        }
        return 0;
}

void put_page(void *p) {
//	sys_printf("put_page %x\n", p);
        set_free(free_page_map,VIRT2PAGE(p));
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

void *get_pages(unsigned int order) {
	int i;
	int num=1<<order;
//      int pmask=(1<<num)-1;
	if (!num) return 0;
	int need_slots=((num-1)/64)+1;
	int have_slots=0;
	int start_slot=0;
	int last_slot=0;

	DEBUGP(DSYS_MEM,DLEV_INFO,"get_pages: num=%d,need_slots=%x\n", num, need_slots);
	for(i=0;i<(sizeof(free_page_map)/sizeof(unsigned long int));i++) {
		if (free_page_map[i]==0xffffffffffffffff) {
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
			have_slots, start_slot, num, 64*have_slots);

	if (have_slots!=need_slots) return 0;
	last_slot=start_slot+have_slots-1;
	for(i=start_slot;i<last_slot;i++) {
		free_page_map[i]=0;
		num-=64;
	}
	for(i=0;i<num;i++) {
		free_page_map[last_slot]&=~(1<<i);
	}

	return (void *)(VA_START + (start_slot*64*4096));
}

static unsigned long *get_or_alloc_table(unsigned long *table, unsigned long index) {
	unsigned long entry = table[index];
	if (!(entry & 1UL)) {
		void *new_tbl = get_page();
		if (!new_tbl) return 0;
		memset(new_tbl, 0, PAGE_SIZE);
		unsigned long phys = (unsigned long)KV2P(new_tbl);
		table[index] = phys | PT_TABLE;
		return (unsigned long *)new_tbl;
	}
	unsigned long phys = entry & PAGE_MASK & 0x0000fffffffff000UL;
	return (unsigned long *)(VA_START + phys);
}

int map_page(unsigned long *pgd, unsigned long vaddr, unsigned long phys_addr, unsigned long flags) {
	unsigned long pgd_idx = (vaddr >> 39) & 0x1ffUL;
	unsigned long pud_idx = (vaddr >> 30) & 0x1ffUL;
	unsigned long pmd_idx = (vaddr >> 21) & 0x1ffUL;
	unsigned long pte_idx = (vaddr >> 12) & 0x1ffUL;

	unsigned long *pud = get_or_alloc_table(pgd, pgd_idx);
	if (!pud) return -1;

	unsigned long *pmd = get_or_alloc_table(pud, pud_idx);
	if (!pmd) return -1;

	unsigned long *pte = get_or_alloc_table(pmd, pmd_idx);
	if (!pte) return -1;

	pte[pte_idx] = (phys_addr & 0x0000fffffffff000UL) | flags;
        return 0;
}

#define PT_PROT_MASK (PT_AP_RO_EL0 | PT_PXN | PT_UXN)

void pmap_protect(unsigned long *pgd, unsigned long start_va, unsigned long end_va, unsigned long prot_flags)
{
        for (unsigned long va = start_va & PAGE_MASK; va < end_va; va += PAGE_SIZE) {
                unsigned long *pud = (unsigned long *)(VA_START + (pgd[(va >> 39) & 0x1ff] & 0x0000fffffffff000UL));
                if (!(pgd[(va >> 39) & 0x1ff] & 1UL)) continue;

                unsigned long *pmd = (unsigned long *)(VA_START + (pud[(va >> 30) & 0x1ff] & 0x0000fffffffff000UL));
                if (!(pud[(va >> 30) & 0x1ff] & 1UL)) continue;

                unsigned long *pte = (unsigned long *)(VA_START + (pmd[(va >> 21) & 0x1ff] & 0x0000fffffffff000UL));
                if (!(pmd[(va >> 21) & 0x1ff] & 1UL)) continue;

                unsigned long idx = (va >> 12) & 0x1ff;
                if (pte[idx] & 1UL) {
                        pte[idx] = (pte[idx] & ~PT_PROT_MASK) | prot_flags;
                }
        }
        asm volatile("tlbi vmalle1\n dsb ish\n isb\n" ::: "memory");
}

int share_process_pages(struct task *to, struct task *from) {
	unsigned long int *pgd = from->asp->pgd;
	unsigned long int *npgd= to->asp->pgd;
	int pgd_ix;

	for(pgd_ix=0;pgd_ix<512;pgd_ix++) {
		unsigned long int pud_val=pgd[pgd_ix];
		unsigned long int *pud=(unsigned long *)
				(VA_START + (pud_val & 0x0000fffffffff000UL));
		int pud_ix;
		if (!pud_val) continue;
		sys_printf("found pud(%lx) at pgd_ix %d\n", pud, pgd_ix);
		for(pud_ix=0;pud_ix<512;pud_ix++) {
			unsigned long int pmd_val=pud[pud_ix];
			unsigned long int *pmd=(unsigned long int *)
				(VA_START + (pmd_val & 0x0000fffffffff000UL));
			int pmd_ix;
			if (!pmd_val) continue;
			sys_printf("found pmd(%lx) at pud_ix %d\n", pmd, pud_ix);
			for(pmd_ix=0;pmd_ix<512;pmd_ix++) {
				unsigned long int pte_val=pmd[pmd_ix];
				unsigned long int *pte=(unsigned long int *)
					(VA_START + (pte_val & 0x0000fffffffff000UL));
				int pte_ix;
				if (!pte_val) continue;
				sys_printf("found pte(%lx) at pmd_ix %d\n", pte, pmd_ix);
				for(pte_ix=0;pte_ix<512;pte_ix++) {
					unsigned long int page_val=pte[pte_ix];
					void *p_addr;
					unsigned long phys;
					if (!page_val) continue;

					unsigned long int va = ((unsigned long)pgd_ix << 39) |
					((unsigned long)pud_ix << 30) |
					((unsigned long)pmd_ix << 21) |
					((unsigned long)pte_ix << 12);
					unsigned long int pa = page_val & 0x0000fffffffff000UL;
					unsigned long int flags = page_val & ~0x0000fffffffff000UL;

					sys_printf("  [%x:%x:%x:%x] VA 0x%lx -> PA 0x%lx (flags 0x%lx)\n",
						pgd_ix, pud_ix, pmd_ix, pte_ix, va, pa, flags);
					p_addr=get_page();
					memcpy(p_addr,(void *)(VA_START+pa),P_SIZE);
					phys = (unsigned long)KV2P(p_addr);
					map_page(npgd, va, phys, flags);
				}
			}
		}
	}

        return 0;
}


void switch_user_asp(struct address_space *asp) {
	unsigned long tcr;
	asm volatile("mrs %0, tcr_el1" : "=r"(tcr));

	if (!asp || !asp->pgd) {
		/* Kernel / idle task: disable TTBR0 walks (EPD0 = 1) */
		tcr |= (1UL << 7);
		asm volatile(
			"msr tcr_el1, %0\n"
			"isb\n"
			: : "r"(tcr) : "memory"
		);
		return;
	}

	unsigned long ttbr0 = ((unsigned long)(asp->id & 0xff) << 48) | (unsigned long)KV2P(asp->pgd);
	tcr &= ~(1UL << 7); /* EPD0 = 0: enable TTBR0 walks */

	asm volatile(
		"msr ttbr0_el1, %0\n"
		"msr tcr_el1, %1\n"
		"isb\n"
		: : "r"(ttbr0), "r"(tcr) : "memory"
	);
}

static inline void flush_tlb_asid(unsigned long asid) {
    unsigned long int val = asid << 48;
    asm volatile(
        "dsb ishst\n\t"      // Ensure all page table writes (zeroing PGD) are visible
        "tlbi aside1, %0\n\t"// Invalidate all EL1/EL0 TLB entries matching this ASID on this core
        "dsb ish\n\t"        // Wait for TLB invalidation to complete
        "isb\n\t"            // Synchronize instruction stream
        : : "r"(val) : "memory"
    );
}

int free_asp_pages(struct address_space *asp) {
	unsigned long int *pgd = asp->pgd;
	int pgd_ix;

	for(pgd_ix=0;pgd_ix<512;pgd_ix++) {
		unsigned long int pud_val=pgd[pgd_ix];
		unsigned long int *pud=(unsigned long *)
				(VA_START + (pud_val & 0x0000fffffffff000UL));
		int pud_ix;
		if (!pud_val) continue;
		sys_printf("found pud(%lx) at pgd_ix %d\n", pud, pgd_ix);
		for(pud_ix=0;pud_ix<512;pud_ix++) {
			unsigned long int pmd_val=pud[pud_ix];
			unsigned long int *pmd=(unsigned long int *)
				(VA_START + (pmd_val & 0x0000fffffffff000UL));
			int pmd_ix;
			if (!pmd_val) continue;
			sys_printf("found pmd(%lx) at pud_ix %d\n", pmd, pud_ix);
			for(pmd_ix=0;pmd_ix<512;pmd_ix++) {
				unsigned long int pte_val=pmd[pmd_ix];
				unsigned long int *pte=(unsigned long int *)
					(VA_START + (pte_val & 0x0000fffffffff000UL));
				int pte_ix;
				if (!pte_val) continue;
				sys_printf("found pte(%lx) at pmd_ix %d\n", pte, pmd_ix);
				for(pte_ix=0;pte_ix<512;pte_ix++) {
					unsigned long int page_val=pte[pte_ix];
					void *p_addr;
					unsigned long phys;
					if (!page_val) continue;

					unsigned long int va = ((unsigned long)pgd_ix << 39) |
					((unsigned long)pud_ix << 30) |
					((unsigned long)pmd_ix << 21) |
					((unsigned long)pte_ix << 12);
					unsigned long int pa = page_val & 0x0000fffffffff000UL;
					unsigned long int flags = page_val & ~0x0000fffffffff000UL;

					sys_printf("put page  [%x:%x:%x:%x] VA 0x%lx -> PA 0x%lx (flags 0x%lx)\n",
						pgd_ix, pud_ix, pmd_ix, pte_ix, va, pa, flags);
					put_page((void *)pa);
				}
				sys_printf("free pte table page at 0x%lx\n", pte);
				put_page(pte);
			}
			sys_printf("free pmd table page at 0x%lx\n", pmd);
			put_page(pmd);
		}
		sys_printf("free pud table page at 0x%lx, and zero out pgd_ix %d\n", pud,pgd_ix);
		put_page(pud);
		pgd[pgd_ix]=0;
	}

	flush_tlb_asid(asp->id);

        return 0;
}



int dump_tlb(void) {
	return 0;
}

