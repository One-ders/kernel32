
	.globl	ffs
	.globl	ffsl
	.text
	.thumb
ffs:
ffsl:
	.cpu cortex-m4

	mov	r2,#32
	rsb	r1,r0,#0
	and	r0,r1,r0
	clz	r1,r0
	sub	r0,r2,r1
	bx	lr
