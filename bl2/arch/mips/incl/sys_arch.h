
static inline __attribute__ ((always_inline))
void disable_interrupts(void) {
	asm(	"mfc0	$t0,$12\n\t"
		"ori 	$t0,1\n\t"
		"xori	$t0,1\n\t"
		"mtc0	$t0,$12\n\t");
}

static inline __attribute__ ((always_inline))
void enable_interrupts(void) {
	asm(	"mfc0	$t0,$12\n\t"
		"ori	$t0,1\n\t"
		"xori	$t0,4\n\t"
		"mtc0	$t0,$12\n\t");
}

