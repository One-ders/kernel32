
#include "config.h"
#include <regdef.h>


	.set noreorder

	.globl _start
	.globl do_switch
	.text

	.org 0x00000000
tlb_refill_user:
	.set	push
	.set	noat
#	get_pgde
	mfc0	k0, CP0_BADVADDR
	lui	k1,%hi(curr_pgd)
	addiu	k1,%lo(curr_pgd)
	lw	k1, 0(k1)
	srl	k0,k0,PT_SHIFT
	sll	k0,k0,2
	addu	k1,k1,k0
	lw	k1,0(k1)
	beqz	k1,_do_page_fault
	ssnop
	mfc0	k0, CP0_BADVADDR
	ssnop
	ssnop
	srl	k0,k0,P_SHIFT
	andi	k0,(P_MASK>>P_SHIFT)
	sll	k0,k0,2
	addu	k1,k1,k0
	lw	k1,0(k1)
	beqz	k1,_do_page_fault
	ssnop
	j _do_page_fault
	ssnop
1:	j	1b
	nop
	.set	pop
	.org 0x00000100
cache_error:
2:	j	2b
	nop
	.org 0x00000180
general_exception:
	.set	push
	.set	noat
	mfc0	k1, CP0_CAUSE
	andi	k1,k1,0x7c
	lui	k0,%hi(exception_handlers)
	addiu	k0,k0,%lo(exception_handlers)
	addu	k0,k0,k1
	lw   	k0,0(k0)
	jr	k0
	nop
	.set	pop

	.org 0x1000
exception_handlers:
	.word _handle_irq
	.word _handle_tlb1
	.word _handle_tlb2
	.word _handle_tlb3
	.word _handle_AdEL
	.word _handle_AdES
	.word _handle_IBE
	.word _handle_DBE
	.word _handle_Sys
	.word _handle_Bp
	.word _handle_RI
	.word _handle_CpU
	.word _handle_Ov
	.word _handle_Tr
	.word 0
	.word _handle_FPE
	.word 0
	.word 0
	.word _handle_c2E
	.word 0
	.word 0
	.word 0
	.word _handle_MDMX
	.word _handle_WATCH
	.word _handle_Mcheck
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word _handle_CacheErr
	.word 0

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
1:
	lui	k0,%hi(irq_lev)
	addiu	k0,k0,%lo(irq_lev)
	lw	k1,0(k0)
	bnez	k1,3f
	nop
	lui	k1,%hi(current)
	addiu	k1,k1,%lo(current)
	lw	k0,0(k1)
	lw	k1,1*4(k0)  # get currents kstack
	j	4f
	nop
3:	move	k1,sp	    # continue on actual stack
4: 	move 	k0,sp
	subu	sp,k1,(32*4)+(5*4) # allocate space for regs
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
# set irq_lev
	lui	k1,%hi(irq_lev)
	addiu	k1,k1,%lo(irq_lev)
	lw	k0,0(k1)
	sw	k0,0*4(sp)		# save current irq_lev to reg zero in frame
	ori	k0,k0,0xa
	sw	k0,0(k1)
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
#	Do not enable irq, here, do it per handler to prevent recursive loops
	lui	ra,%hi(ret_from_irq)
	addiu	ra,ra,%lo(ret_from_irq)
	j	irq_dispatch
	move	a0,sp

ret_from_irq:
# disable interrupts
	mfc0	t0,CP0_STATUS
	nop
	ori	t0,1f
	xori	t0,1f
	mtc0	t0,CP0_STATUS
	nop
# check for switch
	lui	t1,%hi(switch_flag)
	addiu	t1,t1,%lo(switch_flag)
	lw	t0,0(t1)
	beq	t0,zero,2f	# no switch, check if we are returning to thread
	nop
	sw	zero,0(t1)
	lui	ra,%hi(1f)
	addiu	ra,ra,%lo(1f)
	j	handle_switch
	move	a0,sp
1:	lui	k0,%hi(current)
	addiu	k0,k0,%lo(current)
	lw	k1,0(k0)
	sw	sp,1*4(k1)      # save current stack pointer
	sw	v0,0(k0)	# update current
	lui	k0,%hi(curr_pgd)
	addiu	k0,k0,%lo(curr_pgd)
	lw	v1,10*4(v0)	# read address space
	lw	v1,0*4(v1)	# read page table dir
	sw	v1,0(k0)
	lw	v1,10*4(v0)	# read address space
	lw	v1,1*4(v1)	# read address space id
	mtc0	v1,CP0_ENTRYHI
	ssnop
	ssnop
#
	lw	sp,1*4(v0)      # get stack pointer for new thread
	addiu	k0,sp,(32*4)+(5*4) # return space for regs
	sw	k0,1*4(v0)	# update stored stack ptr
	li	k1,1
	sw	k1,5*4(v0)	# set currents state to running
#
	lw	k1,29*4(sp)	# if stack to be restore==frame, its not returning
	beq	k0,k1,3f	# check if running on syscall or irq.
	nop
	lui	t1,%hi(irq_lev)  # returning to thread, irq_lev to 0
	addiu	t1,t1,%lo(irq_lev)
	sw	zero,0(t1)
	j	5f		# restore context
	nop
2:	lui	v0,%hi(current)
	addiu	v0,%lo(current)
	lw	v0,0(v0)
	addiu	k0,sp,(32*4)+(5*4) # sp -  space for regs
	sw	k0,1*4(v0)
	lw	k1,29*4(sp)
	beq	k0,k1,3f	   # save sp==frame, not returning to thread
	nop
	lui	t1,%hi(irq_lev)
	addiu	t1,t1,%lo(irq_lev) # return to thread, update irq_lev
	sw	zero,0(t1)
	j	5f			# return to context
	nop
3:  	lw	k1,0*4(sp)	# read irq_lev on prev. frame
	lui	t1,%hi(irq_lev)
	addiu	t1,t1,%lo(irq_lev)
	sw	k1,0(t1)
# RESTORE_TEMP
5:	lw	t8,35*4(sp)
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
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	li	v1,0xff00
	and	a0,v1
	lw	v0,32*4(sp)
	nor	v1,zero,v1
	and	v0,v1
	or	v0,a0
	mtc0	v0,CP0_STATUS
	nop
	lw	v1,34*4(sp)
	mtc0	v1,CP0_EPC
	lw	ra,31*4(sp)
	lw	gp,28*4(sp)
	lw	t9,25*4(sp)
	lw	a3,7*4(sp)
	lw	a2,6*4(sp)
	lw	a1,5*4(sp)
	lw	a0,4*4(sp)
	lw	v1,3*4(sp)
	lw	v0,2*4(sp)
# RESTORE_SP_AND_RET
	lw	sp,29*4(sp)
	eret
	ssnop
	.set pop

# write on write protected page
_handle_tlb1:
	.set push
	.set noat
	lui	k0,%hi(irq_lev)
	addiu	k0,k0,%lo(irq_lev)
	lw	k0,0(k0)
	bnez	k0,3f
	nop
	lui	k1,%hi(current)
	addiu	k1,k1,%lo(current)
	lw	k0,0(k1)
	lw	k1,1*4(k0)
	j	4f
	nop
3:	move	k1,sp
4: 	move 	k0,sp
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
	lui	ra,%hi(ret_from_tlb1)
	addiu	ra,ra,%lo(ret_from_tlb1)
	j	handle_TLB1
	move	a0,sp
	.set	pop

ret_from_tlb1:
	.set push
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
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	li	v1,0xff00
	and	a0,v1
	lw	v0,32*4(sp)
	nor	v1,zero,v1
	and	v0,v1
	or	v0,a0
	mtc0	v0,CP0_STATUS
	nop
	nop
	nop
	nop
	nop
	lw	v1,34*4(sp)
	mtc0	v1,CP0_EPC
	lw	ra,31*4(sp)
	lw	gp,28*4(sp)
	lw	t9,25*4(sp)
	lw	a3,7*4(sp)
	lw	a2,6*4(sp)
	lw	a1,5*4(sp)
	lw	a0,4*4(sp)
	lw	v1,3*4(sp)
	lw	v0,2*4(sp)
# RESTORE_SP_AND_RET
	lw	sp,29*4(sp)
	eret
	ssnop
	.set pop

_handle_tlb2: // TLBL: load or instruction fetch , from page that is configured as invalid
	.set push
	.set noat
	lui	k0,%hi(irq_lev)
	addiu	k0,k0,%lo(irq_lev)
	lw	k0,0(k0)
	bnez	k0,3f
	nop
	lui	k1,%hi(current)
	addiu	k1,k1,%lo(current)
	lw	k0,0(k1)
	lw	k1,1*4(k0)
	j	4f
	nop
3:	move	k1,sp
4: 	move 	k0,sp
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
	lui	ra,%hi(ret_from_tlb2)
	addiu	ra,ra,%lo(ret_from_tlb2)
	j	handle_TLBL
	move	a0,sp
	.set	pop

ret_from_tlb2:
	.set push
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
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	li	v1,0xff00
	and	a0,v1
	lw	v0,32*4(sp)
	nor	v1,zero,v1
	and	v0,v1
	or	v0,a0
	mtc0	v0,CP0_STATUS
	nop
	nop
	nop
	nop
	nop
	lw	v1,34*4(sp)
	mtc0	v1,CP0_EPC
	lw	ra,31*4(sp)
	lw	gp,28*4(sp)
	lw	t9,25*4(sp)
	lw	a3,7*4(sp)
	lw	a2,6*4(sp)
	lw	a1,5*4(sp)
	lw	a0,4*4(sp)
	lw	v1,3*4(sp)
	lw	v0,2*4(sp)
# RESTORE_SP_AND_RET
	lw	sp,29*4(sp)
	eret
	ssnop
	.set pop

_handle_tlb3:   // TLBS a write, to a mmu ref with a non valid bit set
	.set push
	.set noat
	lui	k0,%hi(irq_lev)
	addiu	k0,k0,%lo(irq_lev)
	lw	k0,0(k0)
	bnez	k0,3f
	nop
	lui	k1,%hi(current)
	addiu	k1,k1,%lo(current)
	lw	k0,0(k1)
	lw	k1,1*4(k0)
	j	4f
	nop
3:	move	k1,sp
4: 	move 	k0,sp
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
	lui	ra,%hi(ret_from_tlb3)
	addiu	ra,ra,%lo(ret_from_tlb3)
	j	handle_TLBS
	move	a0,sp
	.set	pop

ret_from_tlb3:
	.set push
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
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	li	v1,0xff00
	and	a0,v1
	lw	v0,32*4(sp)
	nor	v1,zero,v1
	and	v0,v1
	or	v0,a0
	mtc0	v0,CP0_STATUS
	nop
	nop
	nop
	nop
	nop
	lw	v1,34*4(sp)
	mtc0	v1,CP0_EPC
	lw	ra,31*4(sp)
	lw	gp,28*4(sp)
	lw	t9,25*4(sp)
	lw	a3,7*4(sp)
	lw	a2,6*4(sp)
	lw	a1,5*4(sp)
	lw	a0,4*4(sp)
	lw	v1,3*4(sp)
	lw	v0,2*4(sp)
# RESTORE_SP_AND_RET
	lw	sp,29*4(sp)
	eret
	ssnop
	.set pop

// Load or instruction fetch address exception error
// 1. bad alignment
// 2. user mode try to access kernel or supvisor mode
_handle_AdEL:
	.set push
	.set noat
	lui	k0,%hi(irq_lev)
	addiu	k0,k0,%lo(irq_lev)
	lw	k0,0(k0)
	bnez	k0,3f
	nop
	lui	k1,%hi(current)
	addiu	k1,k1,%lo(current)
	lw	k0,0(k1)
	lw	k1,1*4(k0) # get currents kstack
	j	4f
	nop
3:	move	k1,sp	    # continue on actual stack
4: 	move 	k0,sp
	subu	sp,k1,(32*4)+(5*4)
	sw	k0,29*4(sp) # store interrupted stack ptr
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
# increment irq_lev
	lui	k1,%hi(irq_lev)
	addiu	k1,k1,%lo(irq_lev)
	lw	k0,0(k1)
	sw	k0,0*4(sp)	# save irq_lev as register zero
	li	k0,1
	sw	k0,0(k1)
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
	lui	ra,%hi(ret_from_irq)
	addiu	ra, ra, %lo(ret_from_irq)
	j	handle_address_err_load
	move	a0,sp


// Store address exception error
// 1. bad alignment
// 2. user mode try to access kernel or supvisor mode
_handle_AdES:
	.set push
	.set noat
	lui	k0,%hi(irq_lev)
	addiu	k0,k0,%lo(irq_lev)
	lw	k0,0(k0)
	bnez	k0,3f
	nop
	lui	k1,%hi(current)
	addiu	k1,k1,%lo(current)
	lw	k0,0(k1)
	lw	k1,1*4(k0) # get currents kstack
	j	4f
	nop
3:	move	k1,sp	    # continue on actual stack
6:	j	6b
	nop
4: 	move 	k0,sp
	subu	sp,k1,(32*4)+(5*4)
	sw	k0,29*4(sp) # store interrupted stack ptr
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
# increment irq_lev
	lui	k1,%hi(irq_lev)
	addiu	k1,k1,%lo(irq_lev)
	lw	k0,0(k1)
	sw	k0,0*4(sp)	# save irq_lev as register zero
	li	k0,1
	sw	k0,0(k1)
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
	lui	ra,%hi(ret_from_irq)
	addiu	ra, ra, %lo(ret_from_irq)
	j	handle_address_err_store
	move	a0,sp

_handle_IBE:    //
8:	j	8b
	ssnop

_handle_DBE:
9:	j	9b
	ssnop

_handle_Sys:
	.set push
	.set noat
	lui	k0,%hi(irq_lev)
	addiu	k0,k0,%lo(irq_lev)
	lw	k0,0(k0)
	bnez	k0,3f
	nop
	lui	k1,%hi(current)
	addiu	k1,k1,%lo(current)
	lw	k0,0(k1)
	lw	k1,1*4(k0) # get currents kstack
	j	4f
	nop
3:	move	k1,sp	    # continue on actual stack
6:	j	6b
	nop
4: 	move 	k0,sp
	subu	sp,k1,(32*4)+(5*4)
	sw	k0,29*4(sp) # store interrupted stack ptr
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
	addi	v1,v1,4        # step over trap instruction
	sw	v1,34*4(sp)
	sw	t9,25*4(sp)
	sw	gp,28*4(sp)
	sw	ra,31*4(sp)
# increment irq_lev
	lui	k1,%hi(irq_lev)
	addiu	k1,k1,%lo(irq_lev)
	lw	k0,0(k1)
	sw	k0,0*4(sp)	# save irq_lev as register zero
	li	k0,1
	sw	k0,0(k1)
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
#	enable irq
	mfc0	t0,CP0_STATUS
	nop
	ori 	t0,0x1f
	xori	t0,0x1e
	mtc0	t0,CP0_STATUS
	ssnop
	ssnop
	ssnop
	ssnop
#
	lui	ra,%hi(ret_from_sys)
	addiu	ra, ra, %lo(ret_from_sys)
	j	handle_syscall
	move	a0,sp


ret_from_sys:
# disable interrupts
	mfc0	t0,CP0_STATUS
	nop
	ori	t0,1f
	xori	t0,1f
	mtc0	t0,CP0_STATUS
	nop
# check for switch
	lui	t1,%hi(switch_flag)
	addiu	t1,t1,%lo(switch_flag)
	lw	t0,0(t1)
	beq	t0,zero,2f	# no switch, check if we are returning to thread
	nop
	sw	zero,0(t1)
	lui	ra,%hi(1f)
	addiu	ra,ra,%lo(1f)
	j	handle_switch
	move	a0,sp
1:	lui	k0,%hi(current)
	addiu	k0,k0,%lo(current)
	lw	k1,0(k0)
	sw	sp,1*4(k1)      # save current stack pointer
	sw	v0,0(k0)	# update current
	lui	k0,%hi(curr_pgd)
	addiu	k0,k0,%lo(curr_pgd)
	lw	v1,10*4(v0)	# read address space
	lw	v1,0*4(v1)	# read page table dir
	sw	v1,0(k0)
	lw	v1,10*4(v0)	# read address space
	lw	v1,1*4(v1)	# read address space id
	mtc0	v1,CP0_ENTRYHI
	ssnop
	ssnop
#
	lw	sp,1*4(v0)      # get stack pointer for new thread
	addiu	k0,sp,(32*4)+(5*4) # return space for regs
	sw	k0,1*4(v0)	# update stored stack ptr	
	li	k1,1
	sw	k1,5*4(v0)	# set currents state to running
#
	lw	k1,29*4(sp)	# if stack to be restore==frame, its not returning
	beq	k0,k1,3f	# check if running on syscall or irq.
	nop
	lui	t1,%hi(irq_lev)  # returning to thread, irq_lev to 0
	addiu	t1,t1,%lo(irq_lev)
	sw	zero,0(t1)
	j	5f		# restore context
	nop
2:	lui	v0,%hi(current)
	addiu	v0,%lo(current)
	lw	v0,0(v0)
	addiu	k0,sp,(32*4)+(5*4) # sp -  space for regs
	sw	k0,1*4(v0)
	lw	k1,29*4(sp)
	beq	k0,k1,3f	   # save sp==frame, not returning to thread
	nop
	lui	t1,%hi(irq_lev)
	addiu	t1,t1,%lo(irq_lev) # return to thread, update irq_lev
	sw	zero,0(t1)
	j	5f			# return to context
	nop
3:  	lw	k1,0*4(sp)	# read irq_lev on prev. frame
	lui	t1,%hi(irq_lev)
	addiu	t1,t1,%lo(irq_lev)
	sw	k1,0(t1)
#	j	5f
#	nop
# RESTORE_TEMP
5:	lw	t8,35*4(sp)
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
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	li	v1,0xff00
	and	a0,v1
	lw	v0,32*4(sp)
	nor	v1,zero,v1
	and	v0,v1
	or	v0,a0
	mtc0	v0,CP0_STATUS
	nop
	lw	v1,34*4(sp)
	mtc0	v1,CP0_EPC
	lw	ra,31*4(sp)
	lw	gp,28*4(sp)
	lw	t9,25*4(sp)
	lw	a3,7*4(sp)
	lw	a2,6*4(sp)
	lw	a1,5*4(sp)
	lw	a0,4*4(sp)
	lw	v1,3*4(sp)
	lw	v0,2*4(sp)
# RESTORE_SP_AND_RET
	lw	sp,29*4(sp)
	eret
	ssnop
	.set pop

_do_page_fault:
	.set push
	.set noat
	lui	k0,%hi(irq_lev)
	addiu	k0,k0,%lo(irq_lev)
	lw	k0,0(k0)
	bnez	k0,3f
	nop
	lui	k1,%hi(current)
	addiu	k1,k1,%lo(current)
	lw	k0,0(k1)
	lw	k1,1*4(k0)	# k1 <- inpteruppted stack pointer
	j	4f
	nop
3:	move	k1,sp
4: 	move 	k0,sp
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
	move	a0,sp
	lui	ra,%hi(ret_from_tlb_miss_handler)
	addiu	ra,ra, %lo(ret_from_tlb_miss_handler)
	j	handle_tlb_miss
	nop
	.set	pop

ret_from_tlb_miss_handler:
	.set push
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
	ssnop
	ssnop
	ssnop
	ssnop
	ssnop
	li	v1,0xff00
	and	a0,v1
	lw	v0,32*4(sp)
	nor	v1,zero,v1
	and	v0,v1
	or	v0,a0
	mtc0	v0,CP0_STATUS
	nop
	nop
	nop
	nop
	nop
	lw	v1,34*4(sp)
	mtc0	v1,CP0_EPC
	lw	ra,31*4(sp)
	lw	gp,28*4(sp)
	lw	t9,25*4(sp)
	lw	a3,7*4(sp)
	lw	a2,6*4(sp)
	lw	a1,5*4(sp)
	lw	a0,4*4(sp)
	lw	v1,3*4(sp)
	lw	v0,2*4(sp)
# RESTORE_SP_AND_RET
	lw	sp,29*4(sp)
	eret
	ssnop
	.set pop

#_handle_switch:
do_switch:
	.set push
	.set noat
# disable interrupt and set exception level
	mfc0	k0,CP0_STATUS
	nop
	ori	k0,k0,0x13
	xori	k0,k0,0x11
	mtc0	k0,CP0_STATUS
	ssnop
	ssnop
	ssnop
# save regs
	move	k1,sp	    # continue on actual stack
	subu	sp,k1,(32*4)+(5*4) # allocate space for regs
	sw	k1,29*4(sp)
	sw	v1,3*4(sp)
	sw	v0,2*4(sp)
	sw	k0,32*4(sp)
	sw	a0,4*4(sp)
#	mfc0	v1,CP0_CAUSE
	li	v1,0x20
	sw	a1,5*4(sp)
	sw	v1,33*4(sp)
	sw	a2,6*4(sp)
#	mfc0	v1,CP0_EPC
	sw	a3,7*4(sp)
	sw	ra,34*4(sp)
	sw	t9,25*4(sp)
	sw	gp,28*4(sp)
	sw	ra,31*4(sp)
	lui	k1,%hi(irq_lev)
	addiu	k1,k1,%lo(irq_lev)
	lw	k0,0(k1)
	sw	k0,0*4(sp)	# save irq_lev as register zero
	li	k0,0xa
	sw	k0,0(k1)
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

	j	ret_from_irq
	move	a0,sp
	.set pop

_handle_Bp:
6:	j	6b
	nop

_handle_RI:
	mfc0	k1,CP0_EPC
	ssnop
	ssnop
	lui	k0,0x7c03
	lw	k1,(k1)
	ori	k0,0xe83b
	bne	k0,k1,6f
	nop		/* emul: rdhwr v1,$29 */
	mfc0	k0,CP0_EPC
	lui	k1,%hi(hwr_ulr)
	addiu	k1,k1,%lo(hwr_ulr)
	lw	v1,0(k1)
	addiu	k0,4
	mtc0	k0,CP0_EPC
	ssnop
	ssnop
	eret
	ssnop
	ssnop
6:	j	6b
	ssnop

_handle_CpU:
6:	j	6b
	nop

_handle_Ov:
6:	j	6b
	nop

_handle_Tr:
6:	j	6b
	nop

_handle_FPE:
6:	j	6b
	nop

_handle_c2E:
6:	j	6b
	nop

_handle_MDMX:
6:	j	6b
	nop

_handle_WATCH:
6:	j	6b
	nop

_handle_Mcheck:
6:	j	6b
	nop

_handle_CacheErr:
6:	j	6b
	nop


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
        j	uboot_start
        nop

