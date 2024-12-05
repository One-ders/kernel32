#include <../mmu.h>

#define A72

static inline __attribute__ ((always_inline))
unsigned long int save_cpu_flags() {
	unsigned long int flags;
//	__asm__ __volatile__("mfc0\t%z0, $12\n\t"
//				: "=r" (flags));
	return flags;
}

static inline __attribute__ ((always_inline))
void	restore_cpu_flags(unsigned long int flags) {

//	__asm__ __volatile__("mtc0\t%z0, $12\n\t"
//				: : "Jr" (flags));
}

static inline __attribute__ ((always_inline))
unsigned long int disable_interrupts(void) {
	unsigned long int flags,dum;
//	__asm__ __volatile__(	"mfc0\t%0,$12\n\t"
//				"ori\t%1,%0,1\n\t"
//				"xori\t%1,1\n\t"
//				"mtc0\t%1,$12\n\t"
//				: "=r" (flags), "=r" (dum)) ;
	return flags;
}

#if 1
static inline __attribute__ ((always_inline))
void enable_interrupts(void) {
//	asm(	"mfc0	$t0,$12\n\t"
//		"ori	$t0,0x1f\n\t"
//		"xori	$t0,0x1e\n\t"
//		"mtc0	$t0,$12\n\t");
}
#endif

static inline __attribute__ ((always_inline))
void clear_all_interrupts(void) {
//	asm(	"mfc0	$t0,$12\n\t"
//		"ori	$t0,0x1f\n\t"
//		"xori	$t0,0x1e\n\t"
//		"mtc0	$t0,$12\n\t");
}
