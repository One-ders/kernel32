#include "io.h"
#include "sys.h"
#include "string.h"
#include <mmu.h>

#define U_STACK_TOP     0x80000000
#define U_STACK_BOT     0x70000000
#define U_HEAP_START    0x10000000
#define U_INSTR_END     0x01000000
#define U_INSTR_START   0x00001000


int dump_tlb(void);
static void flush_tlb_page(unsigned int vaddr, unsigned int pid);


struct sh_page {
        unsigned long int pte_pfn;
        int             ref;
};

unsigned long int *curr_pgd;

static struct sh_page *sh_p_base;
static int current_sh_index;


void set_asid(unsigned int asid) {
        asm volatile ("mtc0\t%z0, $10\n\t"
                        : : "Jr" (asid));
}

void set_c0_hi(unsigned int val) {
        asm volatile ("mtc0\t%z0, $10\n\t"
                        : : "Jr" (val));
}

unsigned int read_c0_index(void) {
        unsigned int ret=0;
        __asm__ __volatile__("mfc0\t%z0, $0\n\t"
                : "=r" (ret));
        return ret;
}

void set_c0_index(unsigned int index) {
        asm volatile ("mtc0\t%z0, $0\n\t"
                        : : "Jr" (index));
}


unsigned int read_c0_random(void) {
        unsigned int ret=0;
        __asm__ __volatile__("mfc0\t%z0, $1\n\t"
                : "=r" (ret));
        return ret;
}

unsigned int read_c0_lo0(void) {
        unsigned int ret=0;
        __asm__ __volatile__("mfc0\t%z0, $2\n\t"
                : "=r" (ret));
        return ret;
}

void set_c0_lo0(unsigned int lo0) {
        asm volatile ("mtc0\t%z0, $2\n\t"
                        : : "Jr" (lo0));
}


unsigned int read_c0_lo1(void) {
        unsigned int ret=0;
        __asm__ __volatile__("mfc0\t%z0, $3\n\t"
                : "=r" (ret));
        return ret;
}

void set_c0_lo1(unsigned int lo1) {
        asm volatile ("mtc0\t%z0, $3\n\t"
                        : : "Jr" (lo1));
}

unsigned int read_c0_context(void) {
        unsigned int ret=0;
        __asm__ __volatile__("mfc0\t%z0, $4\n\t"
                : "=r" (ret));
        return ret;
}

unsigned int read_c0_pagemask(void) {
        unsigned int ret=0;
        __asm__ __volatile__("mfc0\t%z0, $5\n\t"
                : "=r" (ret));
        return ret;
}

void set_c0_pagemask(unsigned int mask) {
        asm volatile ("mtc0\t%z0, $5\n\t"
                        : : "Jr" (mask));
}

unsigned int read_c0_wired(void) {
        unsigned int ret=0;
        __asm__ __volatile__("mfc0\t%z0, $6\n\t"
                : "=r" (ret));
        return ret;
}

unsigned int read_c0_badvaddr(void) {
        unsigned int ret=0;
        __asm__ __volatile__("mfc0\t%z0, $8\n\t"
                : "=r" (ret));
        return ret;
}

unsigned int read_c0_hi(void) {
        unsigned int ret=0;
        __asm__ __volatile__("mfc0\t%z0, $10\n\t"
                : "=r" (ret));
        return ret;
}

unsigned int read_c0_epc(void) {
        unsigned int ret=0;
        __asm__ __volatile__("mfc0\t%z0, $14\n\t"
                : "=r" (ret));
        return ret;
}

unsigned int read_c0_prid(void) {
        unsigned int ret=0;
        __asm__ __volatile__("mfc0\t%z0, $15,0\n\t"
                : "=r" (ret));
        return ret;
}


unsigned int read_c0_config1(void) {
        unsigned int ret=0;
        __asm__ __volatile__("mfc0\t%z0, $16,1\n\t"
                : "=r" (ret));
        return ret;
}



int get_sh_page() {
        int ix=current_sh_index;
        if (!sh_p_base) {
                sh_p_base=(struct sh_page *)get_page();
                current_sh_index=0;
                memset(sh_p_base,0,4096);
        }
        if (current_sh_index+1>511) {
                return -1;
        }
        current_sh_index++;
        return ix;
}

int count_shared_pages() {
	int res=0;
	int i;
	for (i=0;i<current_sh_index;i++) {
		if (sh_p_base[i].ref>0) res++;
	}
	return res;
}

unsigned int get_pfn_sh(unsigned int ix) {
	struct sh_page *shp=&sh_p_base[ix];
	return shp->pte_pfn | shp->ref;
}

unsigned long int mk_shared_pte(unsigned long int pte_pfn) {
        int p_ix=get_sh_page();
        struct sh_page *shp;
        DEBUGP(DSYS_MEM,DLEV_INFO,"mk_shared_pte: pte %x, p_ix %x\n", pte_pfn, p_ix);
        if (p_ix<0) return 0;
        shp=&sh_p_base[p_ix];
        shp->pte_pfn=pte_pfn&~0x3f;
        shp->ref=1;
        return P_SHARED|(p_ix<<6);
}

unsigned long int del_shared_pte(unsigned long int pte_spfn) {
        int p_ix=(pte_spfn&0x7fffffff)>>6;
        DEBUGP(DSYS_MEM, DLEV_INFO, ">>>>>>>>>>>del_shared_pte: p_ix %x, pte_spfn %x\n", p_ix, pte_spfn);
        if (sh_p_base[p_ix].ref) {
		sys_printf("del_shared_pte, called for non free slot\n");
		return 0;
	}
	while((current_sh_index>0)&&(sh_p_base[current_sh_index-1].ref==0)) {
		current_sh_index--;
		DEBUGP(DSYS_MEM, DLEV_INFO,"del_shared_pte: currix decr: new %x\n", current_sh_index);
	}
        return 0;
}

unsigned long int incr_ref(unsigned long int pte_spfn) {
        struct sh_page *shp=&sh_p_base[(pte_spfn&0x7fffffff)>>6];
        shp->ref++;
        DEBUGP(DSYS_MEM,DLEV_INFO,"incr_ref: spfn %x, ref %x\n", pte_spfn, shp->ref);
        return shp->ref;
}

unsigned long int decr_ref(unsigned long int pte_spfn) {
        struct sh_page *shp=&sh_p_base[(pte_spfn&0x7fffffff)>>6];
        shp->ref--;
        DEBUGP(DSYS_MEM,DLEV_INFO,"dec_ref: spfn %x, ref %x\n", pte_spfn, shp->ref);
        return shp->ref;
}

unsigned long int get_pte_from_shared(unsigned long int pte_spfn) {
        struct sh_page *shp=&sh_p_base[(pte_spfn&0x7fffffff)>>6];
        int pte_flags=pte_spfn&0x3f;
        DEBUGP(DSYS_MEM,DLEV_INFO,"pte_from_shared: spfn %x, pte %x\n", pte_spfn, shp->pte_pfn|pte_flags);
        return shp->pte_pfn|pte_flags;
}

unsigned long int dealloc_pte_page(unsigned int pte) {
	unsigned long int paddr=0;
	if (pte&P_SHARED) {
		int real_pte=get_pte_from_shared(pte);
		paddr=(0x80000000|((real_pte>>6)<<12));
		if (decr_ref(pte)<1) {
			put_page((void *)paddr);
			del_shared_pte(pte);
		}
	} else {
		paddr=(0x80000000|((pte>>6)<<12));
		put_page((void *)paddr);
	}
	return paddr;
}


void handle_TLB1(void *sp) {   // a write to a non dirty pte (write protected
	struct fullframe *ff=(struct fullframe *)sp;
	unsigned long int badvaddr=read_c0_badvaddr();
	unsigned int pgd_index=badvaddr>>PT_SHIFT;
	unsigned int pt_index=(badvaddr&P_MASK)>>P_SHIFT;
	unsigned int pt_eindex=pt_index&0xfffffffe;
	unsigned long int *pt, pte;
	unsigned long int pte0, pte1;
	unsigned long int p, orig_p;
	unsigned long int c0_index;

	if (!curr_pgd) {
		io_setpolled(1);
		sys_printf("handle_TLB1: super error, no curr_pgd\n");
		while(1);
	}
	DEBUGP(DSYS_MEM,DLEV_INFO,">>>>TLB1: handle_tlb_miss: tlb_index %x, tlb_hi %x\n",
                        read_c0_index(), read_c0_hi());
	DEBUGP(DSYS_MEM,DLEV_INFO,"TLB1: curr_pgd at %x\n",curr_pgd);

	DEBUGP(DSYS_MEM,DLEV_INFO,"TLB1: tlb_miss for %x\n", badvaddr);
	pt=(unsigned long int *)curr_pgd[pgd_index];
	if (!pt) {
		io_setpolled(1);

	asm("tlbp");
		sys_printf(">>>>TLB1: handle_tlb_miss: tlb_index %x, tlb_hi %x\n",
                        read_c0_index(), read_c0_hi());
		sys_printf("TLB1: curr_pgd at %x\n",curr_pgd);

		sys_printf("TLB1: tlb_miss for %x\n", badvaddr);
		sys_printf("no pt at index %x\n",pgd_index);
		while(1);
	}
	DEBUGP(DSYS_MEM,DLEV_INFO, "TLB1: page_table at %x\n", pt);

	DEBUGP(DSYS_MEM,DLEV_INFO,"lookup page table index %x, in page table %x\n", pt_index, pt);

	pte=pt[pt_index];
	if (!pte) {
		io_setpolled(1);
		sys_printf("no pte at  %x\n", pt_index);
		while(1);
	} else {
		DEBUGP(DSYS_MEM,DLEV_INFO,"pte is %x\n",pte);
	}

	if (pte&P_DIRTY) {
		pte0=pt[pt_eindex];
		pte1=pt[pt_eindex+1];
		io_setpolled(1);
	asm("tlbp");
		DEBUGP(DSYS_MEM,DLEV_INFO,"TLB1: tlb_miss for %x\n", badvaddr);
		DEBUGP(DSYS_MEM, DLEV_INFO, "low pt_ix %x\n", pt_eindex);
		DEBUGP(DSYS_MEM, DLEV_INFO, "got tlb miss for valid pte %x, index %x\n", pte,read_c0_index());

		DEBUGP(DSYS_MEM, DLEV_INFO, "lo0=%x, lo1=%x, want %x:%x\n",read_c0_lo0(),read_c0_lo1(),pte0,pte1);

		dump_tlb();
		while(1);
		return;
        }

	if (pte&P_SHARED) {
		orig_p=get_pte_from_shared(pte);
		if (decr_ref(pte)<1) {
			del_shared_pte(pte);
			orig_p|=(P_CACHEABLE|P_DIRTY|P_VALID);
		} else {
			orig_p=orig_p>>6;
			orig_p=orig_p<<12;
			orig_p|=0x80000000;
			p=(unsigned long int)get_page();
			DEBUGP(DSYS_MEM, DLEV_INFO, "copy page data from %x to %x\n", orig_p, p);
			memcpy((void *)p,(void *)orig_p,4096);
			// free(orig_p)
			p&=~0x80000000;
			orig_p=(((p>>12)<<6)|(P_CACHEABLE|P_DIRTY|P_VALID));
		}

//              sys_printf("allocating page(%x) at pt_index %x\n",p,pt_index);
//              if ((badvaddr>=U_INSTR_START)&&(badvaddr<U_INSTR_END)) {
//                      pt[pt_index]=pte=((p>>6)|(P_CACHEABLE|P_VALID));
//              } else {
		pt[pt_index]=orig_p;
		DEBUGP(DSYS_MEM,DLEV_INFO,"write pte %x, index %x\n", orig_p,read_c0_index());
//              }

		pte0=pt[pt_eindex];
		pte1=pt[pt_eindex+1];
		if (pte0&P_SHARED) {
			set_c0_lo0(get_pte_from_shared(pte0));
		} else {
			set_c0_lo0(pte0);
		}

		if (pte1&P_SHARED) {
			set_c0_lo1(get_pte_from_shared(pte1));
		} else {
			set_c0_lo1(pte1);
		}
	} else {
		sys_printf("page is not shared\n");
        }

	asm("tlbp");

	c0_index=read_c0_index();
	if ((c0_index&0x80000000)==0) {
		asm("tlbwi");
	} else {
		asm("tlbwr");
	}
}

void handle_tlb_miss(void *sp) {
        struct fullframe *ff=(struct fullframe *)sp;
        unsigned long int badvaddr=read_c0_badvaddr();
        unsigned int pgd_index=badvaddr>>PT_SHIFT;
        unsigned int pt_index=(badvaddr&P_MASK)>>P_SHIFT;
        unsigned int pt_eindex=pt_index&0xfffffffe;
        unsigned long int *pt, pte;
        unsigned long int pte0, pte1;
	unsigned long int c0_hi;
	unsigned long c0_index;
	int wi=0;

        if (!curr_pgd) {
                sys_printf("handle_tlb_miss: super error, no curr_pgd for addr %x\n",badvaddr);
                while(1);
        }
//      sys_printf("curr_pgd at %x\n",curr_pgd);

        
        c0_hi=read_c0_hi();
	c0_index=read_c0_index();
	DEBUGP(DSYS_MEM,DLEV_INFO,">>>>tlb_miss for %x\n", badvaddr);
        DEBUGP(DSYS_MEM,DLEV_INFO,"handle_tlb_miss: tlb_index %x, tlb_hi %x\n",
                        read_c0_index(), c0_hi);
        DEBUGP(DSYS_MEM,DLEV_INFO,"lo0=%x, lo1=%x\n",read_c0_lo0(),read_c0_lo1());

	if ((read_c0_index()&0x80000000)==0) {
		DEBUGP(DSYS_MEM,DLEV_INFO,"tlb_miss for %x\n", badvaddr);
		DEBUGP(DSYS_MEM, DLEV_INFO, "low pt_ix %x\n", pt_eindex);
        	DEBUGP(DSYS_MEM,DLEV_INFO,"handle_tlb_miss: tlb_index %x, tlb_hi %x\n",
                        read_c0_index(), read_c0_hi());

		DEBUGP(DSYS_MEM, DLEV_INFO, "lo0=%x, lo1=%x\n",read_c0_lo0(),read_c0_lo1());

		set_c0_hi(c0_hi);
		set_c0_index(c0_index);
	}

        if (((badvaddr<U_STACK_TOP)&&(badvaddr>=U_STACK_BOT)) || 
            ((badvaddr<current->asp->brk) && (badvaddr>=U_HEAP_START)) ||
	    ((badvaddr<U_HEAP_START)&&(badvaddr>=current->asp->mmap_vaddr)) ||
            ((badvaddr<U_INSTR_END)&&(badvaddr>=U_INSTR_START))) {
        } else {
		io_setpolled(1);
                sys_printf("User: Segmentation violation\n");
	        sys_printf("invalid access from 0x%08x, address 0x%08x, sp 0x%08x, current=%s\n",
                                   read_c0_epc(), read_c0_badvaddr(), sp, current->name);
                while(1);
        }


        pt=(unsigned long int *)curr_pgd[pgd_index];
        if (!pt) {
                DEBUGP(DSYS_MEM,DLEV_INFO,"allocating page table at pgd_index %x\n",pgd_index);
                pt=(unsigned long int *)get_page();
                memset(pt,0,4096);
                curr_pgd[pgd_index]=(unsigned long int)pt;
        }
        DEBUGP(DSYS_MEM,DLEV_INFO,"page_table at %x\n", pt);

        DEBUGP(DSYS_MEM,DLEV_INFO,"lookup page table index %x\n", pt_index);
        pte=pt[pt_index];
        if (pte) { // page in tree
                DEBUGP(DSYS_MEM,DLEV_INFO,"found page in tree\n");
        }
        if (!pte) {
                unsigned long int p=(unsigned long int)get_page();
                memset((void *)p,0,4096);
                p&=~0x80000000;  // to physical
//              if ((badvaddr>=U_INSTR_START)&&(badvaddr<U_INSTR_END)) {
//                      pt[pt_index]=pte=(p>>6)|(P_CACHEABLE|P_VALID);
//              } else {
                        pt[pt_index]=pte=((p>>12)<<6)|(P_CACHEABLE|P_DIRTY|P_VALID);
//              }
//              sys_printf("allocating page(%x) at pt_index %x\n",p,pt_index);
        }

        pte0=pt[pt_eindex];
        pte1=pt[pt_eindex+1];
        DEBUGP(DSYS_MEM,DLEV_INFO,"new pte is %x, pte0 %x, pte1 %x\n", pte, pte0, pte1);
        if (pte0&P_SHARED) {
                set_c0_lo0(get_pte_from_shared(pte0));
        } else {
                set_c0_lo0(pte0);
        }

        if (pte1&P_SHARED) {
                set_c0_lo1(get_pte_from_shared(pte1));
        } else {
                set_c0_lo1(pte1);
        }

	asm("tlbp");

	c0_index=read_c0_index();
	if ((c0_index&0x80000000)==0) {
		asm("tlbwi");
	} else {
		asm("tlbwr");
	}
}

void handle_TLBL(void *sp) {
        struct fullframe *ff=(struct fullframe *)sp;
        unsigned long int badvaddr=read_c0_badvaddr();
        unsigned int pgd_index=badvaddr>>PT_SHIFT;
        unsigned int pt_index=(badvaddr&P_MASK)>>P_SHIFT;
        unsigned int pt_eindex=pt_index&0xfffffffe;
        unsigned long int *pt, pte;
        unsigned long int pte0, pte1;
	unsigned long int c0_hi;
	unsigned long int c0_index;

	c0_index=read_c0_index();
	c0_hi=read_c0_hi();
#if 0
        sys_printf("handle tlb load trap, badvaddr %x\n",badvaddr);
#endif
        if (!curr_pgd) {
		io_setpolled(1);
		sys_printf("handle_TLBL: at 0x%08x, address 0x%08x, sp=0x%08x, current=%s\n",
                      read_c0_epc(), read_c0_badvaddr(),sp, current->name);
                while(1);
        }
//      sys_printf("TLBL: curr_pgd at %x\n",curr_pgd);

        DEBUGP(DSYS_MEM,DLEV_INFO,">>>>>TLBL: tlb_miss for %x\n", badvaddr);
        DEBUGP(DSYS_MEM,DLEV_INFO,"handle_TLBL_miss: tlb_index %x, tlb_hi %x\n",
                        read_c0_index(), read_c0_hi());
        DEBUGP(DSYS_MEM,DLEV_INFO,"lo0=%x, lo1=%x\n",read_c0_lo0(),read_c0_lo1());
        pt=(unsigned long int *)curr_pgd[pgd_index];
        if (!pt) {
                DEBUGP(DSYS_MEM,DLEV_INFO,"allocating page table at pgd_index %x\n",pgd_index);
                pt=(unsigned long int *)get_page();
                memset(pt,0,4096);
                curr_pgd[pgd_index]=(unsigned long int)pt;
        }
//      sys_printf("TLBL: page_table at %x\n", pt);
//
//      sys_printf("lookup page table index %x\n", pt_index);
        pte=pt[pt_index];
        if (pte) { // page in tree
                DEBUGP(DSYS_MEM,DLEV_INFO,"found page in tree\n");
        }
        if (!pte) {
                unsigned long int p=(unsigned long int)get_page();
                memset((void *)p,0,4096);
                p&=~0x80000000;
                DEBUGP(DSYS_MEM,DLEV_INFO,"allocating page(%x) at pt_index %x\n",p,pt_index);
//              if ((badvaddr>=U_INSTR_START)&&(badvaddr<U_INSTR_END)) {
//                      pt[pt_index]=pte=((p>>6)|(P_CACHEABLE|P_VALID));
//              } else {
                        pt[pt_index]=pte=(((p>>12)<<6)|(P_CACHEABLE|P_DIRTY|P_VALID));
//              }
        }


        pte0=pt[pt_eindex];
        pte1=pt[pt_eindex+1];
        DEBUGP(DSYS_MEM,DLEV_INFO,"new pte is %x, pte0 %x, pte1 %x\n", pte, pte0, pte1);

        if (pte0&P_SHARED) {
                set_c0_lo0(get_pte_from_shared(pte0));
        } else {
                set_c0_lo0(pte0);
        }

        if (pte1&P_SHARED) {
                set_c0_lo1(get_pte_from_shared(pte1));
        } else {
                set_c0_lo1(pte1);
        }

	asm("tlbp");
	if ((read_c0_index()&0x80000000)==0) {
		asm("tlbwi");
	} else {
        	asm("tlbwr");
	}
}

void handle_TLBS(void *sp) { // write to mem
        struct fullframe *ff=(struct fullframe *)sp;
        unsigned long int badvaddr=read_c0_badvaddr();
        unsigned int pgd_index=badvaddr>>PT_SHIFT;
        unsigned int pt_index=(badvaddr&P_MASK)>>P_SHIFT;
        unsigned int pt_eindex=pt_index&0xfffffffe;
        unsigned long int *pt, pte;
        unsigned long int pte0, pte1;
	unsigned long int c0_hi;
	unsigned long int c0_index;

        if (!curr_pgd) {
                sys_printf("handle_TLBS: super error, no curr_pgd\n");
                while(1);
        }
//      sys_printf("TLBS: curr_pgd at %x\n",curr_pgd);

	c0_index=read_c0_index();
	c0_hi=read_c0_hi();

        DEBUGP(DSYS_MEM,DLEV_INFO, ">>>>>>TLBS: tlb_miss for %x\n", badvaddr);
        DEBUGP(DSYS_MEM,DLEV_INFO, "TLBS : tlb_index %x, tlb_hi %x\n",
                        read_c0_index(), read_c0_hi());
        DEBUGP(DSYS_MEM,DLEV_INFO, "lo0=%x, lo1=%x\n",read_c0_lo0(),read_c0_lo1());
        pt=(unsigned long int *)curr_pgd[pgd_index];
        if (!pt) {
                DEBUGP(DSYS_MEM,DLEV_INFO, "allocating page table at pgd_index %x\n",pgd_index);
                pt=(unsigned long int *)get_page();
                memset(pt,0,4096);
                curr_pgd[pgd_index]=(unsigned long int)pt;
        }
        DEBUGP(DSYS_MEM,DLEV_INFO, "TLBS: page_table at %x\n", pt);

//      sys_printf("lookup page table index %x\n", pt_index);
        DEBUGP(DSYS_MEM,DLEV_INFO, "page_table at %x\n", pt);

        DEBUGP(DSYS_MEM,DLEV_INFO, "lookup page table index %x\n", pt_index);
        pte=pt[pt_index];
        if (pte) { // page in tree
                DEBUGP(DSYS_MEM,DLEV_INFO, "found page in tree, pte %x\n",pte);
        }
        if (!pte) {
                unsigned long int p=(unsigned long int)get_page();
                memset((void *)p,0,4096);
                p&=~0x80000000;
                DEBUGP(DSYS_MEM,DLEV_INFO, "allocating page(%x) at pt_index %x\n",p,pt_index);
//              if ((badvaddr>=U_INSTR_START)&&(badvaddr<U_INSTR_END)) {
//                      pt[pt_index]=pte=((p>>6)|(P_CACHEABLE|P_VALID));
//              } else {
                       pt[pt_index]=pte=(((p>>12)<<6)|(P_CACHEABLE|P_DIRTY|P_VALID));
//              }
        }


	if ((read_c0_index()&0x80000000)==0) {
		DEBUGP(DSYS_MEM,DLEV_INFO,"tlb_TLBS for %x\n", badvaddr);
		DEBUGP(DSYS_MEM, DLEV_INFO, "low pt_ix %x\n", pt_eindex);
		DEBUGP(DSYS_MEM, DLEV_INFO, "got tlb miss for valid pte %x, index %x\n", pte,read_c0_index());
        	DEBUGP(DSYS_MEM,DLEV_INFO,"handle_tlb_miss: tlb_index %x, tlb_hi %x\n",
                        read_c0_index(), read_c0_hi());

		DEBUGP(DSYS_MEM, DLEV_INFO, "lo0=%x, lo1=%x\n",read_c0_lo0(),read_c0_lo1());

		set_c0_index(c0_index);
		set_c0_hi(c0_hi);
	}

        pte0=pt[pt_eindex];
        pte1=pt[pt_eindex+1];
        DEBUGP(DSYS_MEM,DLEV_INFO, "new pte is %x, pte0 %x, pte1 %x\n", pte, pte0, pte1);
        if (pte0&P_SHARED) {
                set_c0_lo0(get_pte_from_shared(pte0));
        } else {
                set_c0_lo0(pte0);
        }

        if (pte1&P_SHARED) {
                set_c0_lo1(get_pte_from_shared(pte1));
        } else {
                set_c0_lo1(pte1);
        }

	asm("tlbp");
	if ((read_c0_index()&0x80000000)==0) {
		asm("tlbwi");
	} else {
        	asm("tlbwr");
	}
}

int mapmem(struct task *t, unsigned long int vaddr, unsigned long int paddr, unsigned int attr) {
        unsigned int pgd_index=vaddr>>PT_SHIFT;
        unsigned int pt_index=(vaddr&P_MASK)>>P_SHIFT;
        unsigned long int *pt, pte;
        
        pt=(unsigned long int *)t->asp->pgd[pgd_index];
        if (!pt) {
//              sys_printf("allocating page table at pgd_index %x\n",pgd_index);
                pt=(unsigned long int *)get_page();
                memset(pt,0,4096);
                t->asp->pgd[pgd_index]=(unsigned long int)pt;
        }
        pte=pt[pt_index];
        if (pte) {
                sys_printf("mapmem: already mapped %x\n", vaddr);
                return -1;
        }
        
        pte=pt[pt_index]=(paddr>>6)|attr|P_VALID;
        
        return 0;
}

int unmapmem(struct task *t, unsigned long int vaddr) {
        unsigned int pgd_index=vaddr>>PT_SHIFT;
        unsigned int pt_index=(vaddr&P_MASK)>>P_SHIFT;
        unsigned long int *pt, pte;
	unsigned long int cpu_flags;

	cpu_flags=disable_interrupts();
        pt=(unsigned long int *)t->asp->pgd[pgd_index];
        if (!pt) {
		restore_cpu_flags(cpu_flags);
		return 0;
	}

        pte=pt[pt_index];
        if (!pte) {
		restore_cpu_flags(cpu_flags);
                return 0;
        }

	dealloc_pte_page(pte);

        pt[pt_index]=0;
	flush_tlb_page(vaddr,t->asp->id);
	restore_cpu_flags(cpu_flags);

        return 0;
}

#define UNIQUE_ENTRYHI(idx) (0x80000000 + ((idx) << (PAGE_SHIFT + 1)))

static void flush_tlb_page(unsigned int vaddr, unsigned int pid) {
        int index;
	int cpu_flags;
        DEBUGP(DSYS_MEM,DLEV_INFO,"flush_tlb: %x, pid %d\n", vaddr, pid);

	cpu_flags=disable_interrupts();
        set_c0_hi((vaddr&0xffffe000)|pid);
        asm("tlbp");
        index=read_c0_index();
        DEBUGP(DSYS_MEM,DLEV_INFO, "got index: %x\n",index);
        if (index&0x80000000) {
                // no mapping exist
                goto out;
        }

//      sys_printf("flush flush\n");
        set_c0_hi(UNIQUE_ENTRYHI(index));
        if (vaddr&(1<<12)) {
                set_c0_lo1(0);
        } else {
                set_c0_lo0(0);
        }
        asm("tlbwi");

out:
        set_asid(current->id);
	restore_cpu_flags(cpu_flags);
}

static int share_pt_pages(unsigned int pgd_index,struct task *to,
                                        struct task *from) {
        unsigned long int *pgd_from=from->asp->pgd;
        unsigned long int *pgd_to=to->asp->pgd;
        unsigned int pt_index;
        unsigned long int *pt_from;
        unsigned long int *pt_to;

        pt_from=(unsigned long int *)pgd_from[pgd_index];
        pt_to  =(unsigned long int *)pgd_to  [pgd_index];

        if (!pt_to) {
                pt_to=(unsigned long *)get_page();
                memset(pt_to,0,4096);
                pgd_to[pgd_index]=(unsigned long int)pt_to;
        }

        for(pt_index=0;pt_index<1024;pt_index++) {
                unsigned long int p=pt_from[pt_index];
                if (p) {
			unsigned long int vaddr=(pgd_index<<PT_SHIFT)|(pt_index<<P_SHIFT);
                        if (p&P_SHARED) {
                                incr_ref(p);
                                pt_to[pt_index]=p|(P_CACHEABLE|P_VALID);
                                flush_tlb_page(vaddr,to->asp->id);
                        } else {
                                unsigned long int spte;
                                pt_to[pt_index]=(spte=mk_shared_pte(p))|(P_CACHEABLE|P_VALID);
                                if (!spte) {
                                        sys_printf("share_page error\n");
                                        return -1;
                                }
                                incr_ref(spte);
                                pt_from[pt_index]=spte|(P_CACHEABLE|P_VALID);
                                flush_tlb_page(vaddr,from->asp->id);
                                flush_tlb_page(vaddr,to->asp->id);
                        }
                }
        }
        return 0;
}

int share_process_pages(struct task *to, struct task *from) {
        unsigned long int *pgd_from=from->asp->pgd;
        int i;

        for(i=0;i<1024;i++) {
                if (pgd_from[i]) {
                        if (share_pt_pages(i,to,from)<0) {
                                return -1;
                        }
                }
        }
        return 0;
}

int free_asp_pages(struct address_space *asp) {
        int i;
        unsigned long int *pgd=asp->pgd;
        for(i=0;i<1024;i++) {
                unsigned long int *pt;
                if (pt=(unsigned long int *)pgd[i]) {
			unsigned int j;
			unsigned int pte;
			for(j=0;j<1024;j++) {
				if (pte=pt[j]) {
					unsigned long int vaddr=
						(i<<PT_SHIFT)|(j<<P_SHIFT);
					dealloc_pte_page(pte);
                                	flush_tlb_page(vaddr,asp->id);
				}
			}
             		put_page((void *)pt);
			pgd[i]=0;
                }
        }
        return 0;
}

void handle_address_err_load(void *sp) { 
	io_setpolled(1);
	sys_printf("address read error: at 0x%08x, address 0x%08x, sp=0x%08x, current=%s\n",
                      read_c0_epc(), read_c0_badvaddr(),sp, current->name);

	while(1);
}

void handle_address_err_store(void *sp) { 
	io_setpolled(1);
	sys_printf("address write error: at 0x%08x, address 0x%08x, sp=0x%08x, current=%s\n",
                      read_c0_epc(), read_c0_badvaddr(),sp, current->name);

	while(1);
}
