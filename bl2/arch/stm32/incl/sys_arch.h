

static inline __attribute__ ((always_inline))
unsigned long int save_cpu_flags(void) {
	unsigned long int flags,dum;
	__asm__ __volatile__ (	"mrs\t%1,PRIMASK\n\t"
				"lsl\t%1,%1,31\n\t"
				"mrs\t%0,BASEPRI\n\t"
				"orr\t%0,%0,%1\n\t"
				: "=r" (flags), "=r" (dum));
	return flags;
}

static inline __attribute__ ((always_inline))
void restore_cpu_flags(unsigned long int flags) {

	if (flags&0x80000000) {
		flags&=~0x80000000;
		__asm__ __volatile__ (	"cpsid\ti\n\t"
					"msr\tBASEPRI,%0\n\t"
					: : "Jr" (flags));
	} else {
		__asm__ __volatile__ (	"cpsie\ti\n\t"
					"msr\tBASEPRI,%0\n\t"
					: : "Jr" (flags));
	}
}


static inline __attribute__ ((always_inline))
unsigned long int disable_interrupts(void) {
	unsigned long int flags,dum;
	__asm__ __volatile__ (	"mrs\t%1,PRIMASK\n\t"
				"lsl\t%1,%1,31\n\t"
				"mrs\t%0,BASEPRI\n\t"
				"orr\t%0,%0,%1\n\t"
				"cpsid\ti\n\t"
				: "=r" (flags), "=r" (dum));
	return flags;
}

static inline __attribute__ ((always_inline))
void enable_interrupts(void) {
	__asm__ __volatile__ (	"cpsie\ti\n\t" );
}

static inline __attribute__ ((always_inline))
void clear_all_interrupts(void) {
	unsigned long int dum;
	__asm__ __volatile__ (	"cpsie\ti\n\t"
				"and\t%0,%0,#0\n\t"
				"msr\tBASEPRI,%0\n\t"
				: "=r" (dum));
}

