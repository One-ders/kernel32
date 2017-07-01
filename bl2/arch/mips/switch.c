#include "io.h"
#include "sys.h"
#include <mmu.h>

unsigned int irq_lev=0;
unsigned int switch_flag=0;

void set_asid(unsigned int asid) {
	asm volatile ("mtc0\t%z0, $10\n\t"
			: : "Jr" (asid));
}


#if 0
struct task *t_array[256];

void init_task_list() {
	int i;
	for(i=0;i<255;i++) {
		t_array[i]=0;
	}
}

int allocate_task_id(struct task *t) {
	int i;
	for(i=0;i<255;i++) {
		if (!t_array[i]) {
			t_array[i]=t;
			t->id=i;
			set_asid(i);
			return i;
		}
	}
	return -1;
}
#endif

unsigned long int *curr_pgd;

struct task *create_user_context(void) {
	struct task *t=(struct task *)get_page();
	unsigned long int *t_pgd=(unsigned long int *)get_page();
	struct address_space *asp=(struct address_space *)(t+1);
	memset(t,0,sizeof(struct task));
	memset(t_pgd,0,P_SIZE);
	memset(asp,0,sizeof(*asp));
	t->sp=(void *)(((char *)t)+4096);
//	sys_printf("got page at %x\n",t);
	if (allocate_task_id(t)<0) {
		sys_printf("proc table full\n");
		put_page(t);
		put_page(t_pgd);
		return 0;
	}
	t->asp=asp;
	asp->ref=1;
	asp->id=t->id;
//	set_asid(asp->id);
	asp->pgd=t_pgd;
	return t;
}

unsigned long int handle_switch(unsigned long int *v_sp) {
	int i=0;
	struct task *t;
	sys_irqs++;
//	sys_printf("handle_switch: sp %x\n", v_sp);

//	disable_interrupts();
	while(i<MAX_PRIO) {
		t=ready[i];
		if (t) {
			ready[i]=t->next;
			t->next=0;
			break;
		}
		i++;
	}
	if (t==current) {
		ASSERT(0);
	}
//	enable_interrupts();

	if (!t) {
		ASSERT(0);
	}
	if (t==current) {
		ASSERT(0);
	}

	if (current->blocker.wake) {
		/* IO blocked task, is ready, so    */
		/* fall through and set state ready */
		current->blocker.wake=0;
		current->state=TASK_STATE_RUNNING;
//		sys_printf("in pendsv: readying blocked task\n");
	}
	/* Have next, move away current if still here, 
	   timer and blocker move themselves */
	if (current->state==TASK_STATE_RUNNING) {
		int prio=current->prio_flags&0xf;
		ASSERT(!current->next);
		CLR_TMARK(current);
		if (prio>4) prio=4;
//		disable_interrupts();
		current->state=TASK_STATE_READY;
		if (ready[prio]) {
			ready_last[prio]->next=current;
		} else {
			ready[prio]=current;
		}
		ready_last[prio]=current;
//		enable_interrupts();
	}

        DEBUGP(DLEV_SCHED,"switch in task: %s\n", t->name);

//	sys_printf("in switch: returning %x\n", t);
	return (unsigned long int)t;
}

unsigned int read_c0_index(void) {
        unsigned int ret=0;
        __asm__ __volatile__("mfc0\t%z0, $0\n\t"
                : "=r" (ret));
        return ret;
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

#define U_STACK_TOP 0x80000000
#define U_STACK_BOT 0x70000000
#define U_HEAP_START 0x10000000
#define U_INSTR_END  0x01000000
#define U_INSTR_START 0x00001000


void handle_tlb_miss(void *sp) {
	struct fullframe *ff=(struct fullframe *)sp;
	unsigned long int badvaddr=read_c0_badvaddr();
	unsigned int pgd_index=badvaddr>>PT_SHIFT;
	unsigned int pt_index=(badvaddr&P_MASK)>>P_SHIFT;
	unsigned long int *pt, pte;

	if (!curr_pgd) {
		sys_printf("handle_tlb_miss: super error, no curr_pgd for addr %x\n",badvaddr);
		while(1);
	}
//	sys_printf("curr_pgd at %x\n",curr_pgd);

//	sys_printf("tlb_miss for %x\n", badvaddr);
	if ((badvaddr<U_STACK_TOP) || (badvaddr>= U_STACK_BOT) ||
            (badvaddr<current->asp->brk) || (badvaddr>= U_HEAP_START) ||
            (badvaddr<U_INSTR_END) || (badvaddr>= U_INSTR_START)) {
	} else {
		sys_printf("User: Seqmentation violation\n");
		while(1);
	}


	pt=(unsigned long int *)curr_pgd[pgd_index];
	if (!pt) {
//		sys_printf("allocating page table at pgd_index %x\n",pgd_index);
		pt=(unsigned long int *)get_page();
		memset(pt,0,4096);
		curr_pgd[pgd_index]=(unsigned long int)pt;
	}
//	sys_printf("page_table at %x\n", pt);

//	sys_printf("lookup page table index %x\n", pt_index);
	pte=pt[pt_index];
	if (!pte) {
		unsigned long int p=(unsigned long int)get_page();
		memset((void *)p,0,4096);
		p&=~0x80000000;  // to physical
//		if ((badvaddr>=U_INSTR_START)&&(badvaddr<U_INSTR_END)) {
//			pt[pt_index]=pte=(p>>6)|(P_CACHEABLE|P_VALID);
//		} else {
			pt[pt_index]=pte=(p>>6)|(P_CACHEABLE|P_DIRTY|P_VALID);
//		}
//		sys_printf("allocating page(%x) at pt_index %x\n",p,pt_index);
	}

	if (badvaddr&(1<<12)) {
		set_c0_lo0(0);
		set_c0_lo1(pte);
	} else {
		set_c0_lo0(pte);
		set_c0_lo1(0);
	}

#if 0
	sys_printf("handle_tlb_miss: tlb_index %x, tlb_random %x, tlb_lo0 %x\n",
			read_c0_index(),read_c0_random(),read_c0_lo0());

	sys_printf("handle_tlb_miss: tlb_lo1 %x, tlb_context %x, tlb_pagemask %x\n",
			read_c0_lo1(), read_c0_context(), read_c0_pagemask());

	sys_printf("handle_tlb_miss: tlb_wired %x, tlb_hi %x\n",
			read_c0_wired(), read_c0_hi());
#endif

	asm("tlbwr");
}

void handle_TLBL(void *sp) {
	struct fullframe *ff=(struct fullframe *)sp;
	unsigned long int badvaddr=read_c0_badvaddr();
	unsigned int pgd_index=badvaddr>>PT_SHIFT;
	unsigned int pt_index=(badvaddr&P_MASK)>>P_SHIFT;
	unsigned long int *pt, pte;
#if 0
	sys_printf("handle tlb load trap, badvaddr %x\n",badvaddr);
#endif
	if (!curr_pgd) {
		sys_printf("handle_TLBL: super error, no curr_pgd for addr %x\n",badvaddr);
		while(1);
	}
//	sys_printf("TLBL: curr_pgd at %x\n",curr_pgd);

//	sys_printf("TLBL: tlb_miss for %x\n", badvaddr);
	pt=(unsigned long int *)curr_pgd[pgd_index];
	if (!pt) {
		sys_printf("allocating page table at pgd_index %x\n",pgd_index);
		pt=(unsigned long int *)get_page();
		memset(pt,0,4096);
		curr_pgd[pgd_index]=(unsigned long int)pt;
	}
//	sys_printf("TLBL: page_table at %x\n", pt);
//
//	sys_printf("lookup page table index %x\n", pt_index);
	pte=pt[pt_index];
	if (!pte) {
		unsigned long int p=(unsigned long int)get_page();
		memset(p,0,4096);
		p&=~0x80000000;
		sys_printf("allocating page(%x) at pt_index %x\n",p,pt_index);
//		if ((badvaddr>=U_INSTR_START)&&(badvaddr<U_INSTR_END)) {
//			pt[pt_index]=pte=((p>>6)|(P_CACHEABLE|P_VALID));
//		} else {
			pt[pt_index]=pte=((p>>6)|(P_CACHEABLE|P_DIRTY|P_VALID));
//		}
	}

	if (badvaddr&(1<<12)) {
		unsigned long int ptep=pt[pt_index-1];
		if (ptep) {
			set_c0_lo0(ptep);
		} else {
			set_c0_lo0(0);
		}
		set_c0_lo1(pte);
	} else {
		unsigned long int ptea=pt[pt_index+1];
		set_c0_lo0(pte);
		if (ptea) {
			set_c0_lo1(ptea);
		} else {
			set_c0_lo1(0);
		}
	}

#if 0
	sys_printf("handle_tlbl_miss: tlb_index %x, tlb_random %x, tlb_lo0 %x\n",
			read_c0_index(),read_c0_random(),read_c0_lo0());

	sys_printf("handle_tlbl_miss: tlb_lo1 %x, tlb_context %x, tlb_pagemask %x\n",
			read_c0_lo1(), read_c0_context(), read_c0_pagemask());

	sys_printf("handle_tlbl_miss: tlb_wired %x, tlb_hi %x\n",
			read_c0_wired(), read_c0_hi());
#endif

//	asm("tlbwi");
	asm("tlbwr");
}

void handle_TLBS(void *sp) {
	struct fullframe *ff=(struct fullframe *)sp;
	unsigned long int badvaddr=read_c0_badvaddr();
	unsigned int pgd_index=badvaddr>>PT_SHIFT;
	unsigned int pt_index=(badvaddr&P_MASK)>>P_SHIFT;
	unsigned long int *pt, pte;

	if (!curr_pgd) {
		sys_printf("handle_TLBS: super error, no curr_pgd\n");
		while(1);
	}
//	sys_printf("TLBS: curr_pgd at %x\n",curr_pgd);

//	sys_printf("TLBS: tlb_miss for %x\n", badvaddr);
	pt=(unsigned long int *)curr_pgd[pgd_index];
	if (!pt) {
//		sys_printf("allocating page table at pgd_index %x\n",pgd_index);
		pt=(unsigned long int *)get_page();
		memset(pt,0,4096);
		curr_pgd[pgd_index]=(unsigned long int)pt;
	}
//	sys_printf("TLBS: page_table at %x\n", pt);

//	sys_printf("lookup page table index %x\n", pt_index);
	pte=pt[pt_index];
	if (!pte) {
		unsigned long int p=(unsigned long int)get_page();
		memset(p,0,4096);
		p&=~0x80000000;
//		sys_printf("allocating page(%x) at pt_index %x\n",p,pt_index);
//		if ((badvaddr>=U_INSTR_START)&&(badvaddr<U_INSTR_END)) {
//			pt[pt_index]=pte=((p>>6)|(P_CACHEABLE|P_VALID));
//		} else {
			pt[pt_index]=pte=((p>>6)|(P_CACHEABLE|P_DIRTY|P_VALID));
//		}
	}

	if (badvaddr&(1<<12)) {
		unsigned long int ptep=pt[pt_index-1];
		if (ptep) {
			set_c0_lo0(ptep);
		} else {
			set_c0_lo0(0);
		}
		set_c0_lo1(pte);
	} else {
		unsigned long int ptea=pt[pt_index+1];
		set_c0_lo0(pte);
		if (ptea) {
			set_c0_lo1(ptea);
		} else {
			set_c0_lo1(0);
		}
	}

#if 0
	sys_printf("handle_tlbs_miss: tlb_index %x, tlb_random %x, tlb_lo0 %x\n",
			read_c0_index(),read_c0_random(),read_c0_lo0());

	sys_printf("handle_tlbs_miss: tlb_lo1 %x, tlb_context %x, tlb_pagemask %x\n",
			read_c0_lo1(), read_c0_context(), read_c0_pagemask());

	sys_printf("handle_tlbs_miss: tlb_wired %x, tlb_hi %x\n",
			read_c0_wired(), read_c0_hi());
#endif

//	asm("tlbwi");
	asm("tlbwr");
}

int mapmem(struct task *t, unsigned long int vaddr, unsigned long int paddr, unsigned int attr) {
	unsigned int pgd_index=vaddr>>PT_SHIFT;
	unsigned int pt_index=(vaddr&P_MASK)>>P_SHIFT;
	unsigned long int *pt, pte;

	pt=(unsigned long int *)t->asp->pgd[pgd_index];
	if (!pt) {
//		sys_printf("allocating page table at pgd_index %x\n",pgd_index);
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

#if 0
	if (vaddr&(1<<12)) {
		unsigned long int ptep=pt[pt_index-1];
		if (ptep) {
			set_c0_lo0(ptep);
		} else {
			set_c0_lo0(0);
		}
		set_c0_lo1(pte);
	} else {
		unsigned long int ptea=pt[pt_index+1];
		set_c0_lo0(pte);
		if (ptea) {
			set_c0_lo1(ptea);
		} else {
			set_c0_lo1(0);
		}
	}
	asm("tlbwr");
#endif
	return 0;
}

int task_sleepable(void) {
	return (irq_lev==1?1:0);
}


void switch_on_return(void) {
	switch_flag=1;
}

void switch_now(void) {
	switch_flag=1;
	do_switch();
}

void init_switcher(void) {
//	init_task_list();
}

void UsageFault_Handler(void) {
}

void Error_Handler_c(void *sp);

void HardFault_Handler(void) {
}

void Error_Handler_c(void *sp_v) {
}

void *SVC_Handler_c(unsigned long int *sp);
