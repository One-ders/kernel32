
#include <regdef.h>

	.globl ffs
	.globl ffsl
	.text
	.set	noreorder
ffs:
ffsl:
	.set	push
	.set	mips32
	li	v1, 32
	negu	a1, a0
	and	a0, a1
	clz	v0, a0
	j	ra
	 subu	v0, v1, v0
	.set	pop
