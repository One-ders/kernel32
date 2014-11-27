
#include "config.h"
#include <regdef.h>


	.set noreorder

	.globl _start
	.text

	.org 0x00000000
tlb_refill_user:
1:	j	1b
	.org 0x00000100
cache_error:
2:	j	2b
	.org 0x00000180
general_exception:
	.set	push
	.set	noat
	mfc0	k1, CP0_CAUSE
	andi	k1,k1,0x7c
	lw   	k0,exception_handlers(k1)
	jr	k0
	.set	pop

	.org 0x1000
exception_handlers:
	.word _handle_irq
	.word _handle_tlb1
	.word _handle_tlb2
	.fill	0x400

	.align 5
_handle_irq:
	.set push
	.set noat
	mfc0	k0, CP0_STATUS
	and	k0, ST0_IE
	bnez	k0, 1f
	nop
4:	j	4b
	nop
	eret
1:
#	mfc0	k0, CP0_STATUS
#	sll	k0,3
#	bltz	k0,1f
	move	k1,sp
#3:	j	3b
	move 	k0,sp
	subu	sp,k1,(32*4)+(5*4)
	sw	k0,29*4(sp)
	sw	v1,3*4(sp)
	mfc0	v1,CP0_STATUS
	sw	v0,2*4(sp)
	sw	v1,32*4(sp)
	sw	a0,4*4(sp)
	mfc0	v1,CP0_CAUSE
	sw	a1,5*4(sp)
	sw	v1,33*4(sp)
	sw	a2,6*4(sp)
	mfc0	v1,CP0_EPC
	sw	a3,7*4(sp)
	sw	v1,34*4(sp)
	sw	t9,25*4(sp)
	sw	gp,28*4(sp)
	sw	ra,31*4(sp)
#
	sw	$1,1*4(sp)
#
	mfhi	v1
	sw	v1,36*4(sp)
	mflo	v1
	sw	v1,35*4(sp)
	sw	t0,8*4(sp)
	sw	t1,9*4(sp)
	sw	t2,10*4(sp)
	sw	t3,11*4(sp)
	sw	t4,12*4(sp)
	sw	t5,13*4(sp)
	sw	t6,14*4(sp)
	sw	t7,15*4(sp)
	sw	t8,24*4(sp)
#
	sw	s0,16*4(sp)
	sw	s1,17*4(sp)
	sw	s2,18*4(sp)
	sw	s3,19*4(sp)
	sw	s4,20*4(sp)
	sw	s5,21*4(sp)
	sw	s6,22*4(sp)
	sw	s7,23*4(sp)
	sw	fp,30*4(sp)
#
	mfc0	t0,CP0_STATUS
#	li	t1,(ST0_CU0 | 0x1f)
	li	t1,0x1f
	or	t0,t1
	xori	t0,0x1f
	mtc0	t0,CP0_STATUS

#	lw	s0,
#	sw	sp,
	move	a0,sp
	la	ra, ret_from_irq
	j	irq_dispatch
	nop
	.set	pop

ret_from_irq:
	.set noat

# RESTORE_TEMP
	lw	t8,35*4(sp)
	mtlo	t8
	lw	t8,36*4(sp)
	mthi	t8
	lw	t0,8*4(sp)
	lw	t1,9*4(sp)
	lw	t2,10*4(sp)
	lw	t3,11*4(sp)
	lw	t4,12*4(sp)
	lw	t5,13*4(sp)
	lw	t6,14*4(sp)
	lw	t7,15*4(sp)
	lw	t8,24*4(sp)
# RESTORE_AT
	lw	$1,1*4(sp)
# RESTORE_STATIC
	lw	s0,16*4(sp)
	lw	s1,17*4(sp)
	lw	s2,18*4(sp)
	lw	s3,19*4(sp)
	lw	s4,20*4(sp)
	lw	s5,21*4(sp)
	lw	s6,22*4(sp)
	lw	s7,23*4(sp)
	lw	fp,30*4(sp)
# RESTORE_SOME
	mfc0	a0,CP0_STATUS
	ori	a0,0x1f
	xori	a0,0x1f
	mtc0	a0,CP0_STATUS
	li	v1,0xff00
	and	a0,v1
	lw	v0,32*4(sp)
	nor	v1,zero,v1
	and	v0,v1
	or	v0,a0
	mtc0	v0,CP0_STATUS
	lw	v1,34*4(sp)
	mtc0	v1,CP0_EPC
	lw	ra,31*4(sp)
	lw	gp,28*4(sp)
	lw	t9,9*4(sp)
	lw	a3,7*4(sp)
	lw	a2,6*4(sp)
	lw	a1,5*4(sp)
	lw	a0,4*4(sp)
	lw	v1,3*4(sp)
	lw	v0,2*4(sp)
# RESTORE_SP_AND_RET
	lw	sp,29*4(sp)
	eret
	.set at


_handle_tlb1:
5:	j	5b
	ssnop

_handle_tlb2:
	mfc0	k1,CP0_BADVADDR
6:	j	6b
	ssnop

	.org 0x4000
_start:
        /* Initialize GOT pointer.
        */
        bal     1f
        nop
        .word   _GLOBAL_OFFSET_TABLE_
        1:
        move    gp, ra
        lw      t1, 0(ra)
        move    gp, t1
        /* Set up temporary stack.
         */
        la      sp, 0x84000000
        la      t9, c_start
        j       t9
        nop

