
#include <../mmu.h>

static inline __attribute__ ((always_inline))
unsigned long int save_cpu_flags(void) {
	unsigned long int flags;

	asm volatile(
		"mrs	%0, daif		// arch_local_save_flags"
		: "=r" (flags)
		:
		: "memory");
	return flags;
}

static inline __attribute__ ((always_inline))
void restore_cpu_flags(unsigned long int flags) {
	asm volatile(
		"msr	daif, %0		// arch_local_irq_restore"
		:
		: "r" (flags)
		: "memory");
}


static inline __attribute__ ((always_inline))
unsigned long int disable_interrupts(void) {
	unsigned long int flags;

	asm volatile(
		"mrs	%0, daif		// arch_local_irq_save\n"
		"msr	daifset, #2"
		: "=r" (flags)
		:
		: "memory");
	return flags;
}

static inline __attribute__ ((always_inline))
void enable_interrupts(void) {
	__asm__ __volatile__ (	"msr\tdaifclr,#2\n\t" );
}

static inline __attribute__ ((always_inline))
void clear_all_interrupts(void) {
	unsigned long int dum;

	enable_interrupts();
}


