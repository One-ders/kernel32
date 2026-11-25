#include "sys_const.h"

.macro	handle_invalid_entry el, type
	kernel_entry \el
	mov     x0, #\type
	mrs     x1, esr_el1
	mrs     x2, elr_el1
	bl      show_invalid_entry_message
	b       err_hang
.endm

.macro  ventry  label
	.align  7
	b       \label
.endm

.macro  kernel_entry, el
	sub     sp, sp, #S_FRAME_SIZE
	stp     x0, x1, [sp, #16 * 0]
	stp     x2, x3, [sp, #16 * 1]
	stp     x4, x5, [sp, #16 * 2]
	stp     x6, x7, [sp, #16 * 3]
	stp     x8, x9, [sp, #16 * 4]
	stp     x10, x11, [sp, #16 * 5]
	stp     x12, x13, [sp, #16 * 6]
	stp     x14, x15, [sp, #16 * 7]
	stp     x16, x17, [sp, #16 * 8]
	stp     x18, x19, [sp, #16 * 9]
	stp     x20, x21, [sp, #16 * 10]
	stp     x22, x23, [sp, #16 * 11]
	stp     x24, x25, [sp, #16 * 12]
	stp     x26, x27, [sp, #16 * 13]
	stp     x28, x29, [sp, #16 * 14]

#if 0
	.if     \el == 0
	mrs     x21, sp_el0
	.endif /* \el == 0 */
#endif

	mrs	x22, elr_el1
	mrs	x23, spsr_el1

	and	x0,x23,#0x01	//
	cbnz	x0, 2f
	mrs	x21, sp_el0 	// save sp_el0 from the user thread

2:
	stp	x30, x21, [sp, #16 * 15]   // dword 30 & 31
	stp	x22, x23, [sp, #16 * 16]   // dword 32 & 33
	adrp	x0,irq_lev
	ldr	w1,[x0, :lo12:irq_lev]
	str	w1,[sp, #16 * 17]

.endm

/*
 * Exception entries
 */
.align	11
.globl	vectors
vectors:
//	ventry	sync_invalid_el1t
	ventry	el1_sync
//	ventry	irq_invalid_el1t
	ventry	el1_irq
	ventry	fiq_invalid_el1t
	ventry	error_invalid_el1t

//	ventry	sync_invalid_el1h
	ventry	el1_sync
	ventry	el1_irq
	ventry	fiq_invalid_el1h
	ventry	error_invalid_el1h

	ventry	el0_sync
	ventry	el0_irq
	ventry	fiq_invalid_el0_64
	ventry	error_invalid_el0_64

	ventry	sync_invalid_el0_32
	ventry	irq_invalid_el0_32
	ventry	fiq_invalid_el0_32
	ventry	error_invalid_el0_32

sync_invalid_el1t:
	handle_invalid_entry	1, SYNC_INVALID_EL1t

irq_invalid_el1t:
	handle_invalid_entry	1, IRQ_INVALID_EL1t

fiq_invalid_el1t:
	handle_invalid_entry	1, FIQ_INVALID_EL1t

error_invalid_el1t:
	handle_invalid_entry	1, ERROR_INVALID_EL1t


sync_invalid_el1h:
	handle_invalid_entry	1, SYNC_INVALID_EL1h

fiq_invalid_el1h:
	handle_invalid_entry	1, FIQ_INVALID_EL1h

error_invalid_el1h:
	handle_invalid_entry	1, ERROR_INVALID_EL1h


fiq_invalid_el0_64:
	handle_invalid_entry	0, FIQ_INVALID_EL0_64

error_invalid_el0_64:
	handle_invalid_entry	0, ERROR_INVALID_EL0_64


sync_invalid_el0_32:
	handle_invalid_entry	0, SYNC_INVALID_EL0_32

irq_invalid_el0_32:
	handle_invalid_entry	0, IRQ_INVALID_EL0_32

fiq_invalid_el0_32:
	handle_invalid_entry	0, FIQ_INVALID_EL0_32

error_invalid_el0_32:
	handle_invalid_entry	0, ERROR_INVALID_EL0_32


.globl kernel_exit
kernel_exit:
	adr	x0, switch_flag
	ldr	x1, [x0]	// read switch_flag
	cbz	x1, 1f		// jmp if not set
	str	xzr,[x0]	// else clear switch flag
	mov	x0,sp		// sp to arg1
	bl	handle_switch
	adr	x1,current
	ldr	x2,[x1]		// x2 = ptr to current
	mov	x3,sp		// x3 = sp
	str	x3, [x2, #8]	// save sp to current data
	str	x0,[x1]		// update current, x0 has next proc
	mov     x19, x0         // save next proc in callee-saved x19 across C call
        ldr     x0, [x19, #64]  // x0 = next->asp
        bl      switch_user_asp // load next->asp->pgd + ASID into TTBR0_EL1 (or EPD0=1 for idle)
        mov     x0, x19         // restore x0 = next proc
	ldr	x3, [x0, #8]	// read new thread stack p
	mov	sp,x3		// give new thread sp to SP_EL1
	add	x3,sp,#S_FRAME_SIZE
	str	x3,[x0,#8]	// update stack ptr - Frame
	mov	w2,#1
	str	w2,[x0,#5*8]	// task state == 1 (running)

1:
	ldr     w1, [sp, #16 * 17]      // läs sparad irq_lev från ramens word 34
	adrp    x0, irq_lev
	str     w1, [x0, :lo12:irq_lev] // återställ irq_lev för denna tråd/ram

	ldp     x22, x23, [sp, #16 * 16]   // word 32 & 33
	ldp     x30, x21, [sp, #16 * 15]	// word 30 & 31

	and	x0,x23,#0x01	//
	cbnz	x0, 2f
	msr     sp_el0, x21	// update sp_el0 for the user thread
2:
	msr     elr_el1, x22
	msr     spsr_el1, x23


	ldp     x0, x1, [sp, #16 * 0]
	ldp     x2, x3, [sp, #16 * 1]
	ldp     x4, x5, [sp, #16 * 2]
	ldp     x6, x7, [sp, #16 * 3]
	ldp     x8, x9, [sp, #16 * 4]
	ldp     x10, x11, [sp, #16 * 5]
	ldp     x12, x13, [sp, #16 * 6]
	ldp     x14, x15, [sp, #16 * 7]
	ldp     x16, x17, [sp, #16 * 8]
	ldp     x18, x19, [sp, #16 * 9]
	ldp     x20, x21, [sp, #16 * 10]
	ldp     x22, x23, [sp, #16 * 11]
	ldp     x24, x25, [sp, #16 * 12]
	ldp     x26, x27, [sp, #16 * 13]
	ldp     x28, x29, [sp, #16 * 14]
	add     sp, sp, #S_FRAME_SIZE
	eret

el1_irq:
	kernel_entry	1
	mov	x29,xzr  // nollställ fp för kernel call frame (gdb)
        adrp    x1, irq_lev
	ldr	w2, [x1, :lo12:irq_lev]
	orr	w2, w2, #0x2
	str	w2, [x1, :lo12:irq_lev]
	mov	x0,sp
	bl	irq_dispatch
	b	kernel_exit

el1_sync:
	kernel_entry	1
	mov	x29,xzr  // nollställ fp för kernel call frame (gdb)
	mov	x0,sp
	mrs	x1, esr_el1
	lsr	x24, x1, #ESR_ELx_EC_SHIFT
	cmp	x24, #ESR_ELx_EC_SVC64
	bne	check_dabt
//	enable irqs
	bl	handle_trap
//	bl	handle_syscall
	b	kernel_exit

el0_irq:
	kernel_entry	0
	mov     x29, xzr                // nollställ fp för gdb
        adrp    x1, irq_lev
	ldr	w2, [x1, :lo12:irq_lev]
	orr	w2, w2, #0x2
	str	w2, [x1, :lo12:irq_lev]
	mov	x0,sp
	bl	irq_dispatch
	b	kernel_exit

el0_sync:
	kernel_entry	0
	mov	x29,xzr  // nollställ fp för kernel call frame (gdb)
	adrp    x1, irq_lev
	mov     w2, #1
	str     w2, [x1, :lo12:irq_lev] // irq_lev = 1 (task_sleepable() == 1)
	mov	x0,sp
	mrs	x1, esr_el1
	lsr	x24, x1, #ESR_ELx_EC_SHIFT
	cmp	x24, #ESR_ELx_EC_SVC64
	bne	check_dabt
	msr	daifclr, #2	//	enable irqs
	bl	handle_syscall
	msr	daifset, #2
	b	kernel_exit

check_dabt:
	mrs	x2,elr_el1
	mrs	x3,spsr_el1
	mrs	x4,far_el1
	mrs	x5,sctlr_el1
	bl	handle_trap
	b	kernel_exit

//	cmp	x24, #ESR_ELx_EC_DABT_LOW
//	beq	handle_data_abort
//	cmp	x24, #ESR_ELx_EC_DABT_CUR
//	beq	handle_data_abort
//	cmp	x24, #ESR_ELx_EC_IABT_LOW
//	beq	handle_data_abort
//	b	err_call
err_call:
	handle_invalid_entry	0, SYNC_ERROR

.globl err_hang
err_hang: b err_hang

.globl irq_vector_init
irq_vector_init:
	adr	x0, vectors
	msr	vbar_el1, x0
	ret

