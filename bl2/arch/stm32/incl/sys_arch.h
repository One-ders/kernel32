
static inline __attribute__ ((always_inline))
void disable_interrupts(void) {
	asm("cpsid i");
}

static inline __attribute__ ((always_inline))
void enable_interrupts(void) {
	asm("cpsie i");
}

