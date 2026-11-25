#include "io.h"
#include "sys.h"
#include "string.h"
#include <mmu.h>

int unmapmem(struct task *t, unsigned long int vaddr) {
	return 0;
}

int count_shared_pages() {
	return 0;
}

unsigned int get_pfn_sh(unsigned int ix) {
	return 0;
}

void flush_tlb() {
	__asm__ __volatile__ (
		"tlbi   vmalle1is \n\t" // Flush all TLB entries for EL1 & EL0 (Inner Shareable)
		"dsb    ish       \n\t" // Data Synchronization Barrier
		"isb              \n\t" // Instruction Synchronization Barrier
		:
		:
		: "memory"
	);
}

void init_tlb() {
	flush_tlb();
	}
