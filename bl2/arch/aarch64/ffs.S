
	.globl	ffs
	.globl	ffsl
	.globl	ffsll
	.text
ffs:
ffsl:
ffsll:
	mov     x2, #64
	neg     x1, x0
	and     x0, x1, x0
	clz     x1, x0
	sub     x0, x2, x1
	ret                     // Return from function
