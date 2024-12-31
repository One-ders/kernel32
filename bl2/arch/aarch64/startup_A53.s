
//  Arm Programmer's Guide:  https://developer.arm.com/documentation/den0024/a/
.section ".text.boot"  // Make sure the linker puts this at the start of the kernel image

.global _start  // Execution starts here
.global do_switch

_start:
	// Check processor ID is zero (executing on main core), else hang
	mrs     x1, mpidr_el1
	and     x1, x1, #0xff
	cbz     x1, 2f
	b       1f
	// We're not on the main core, so hang in an infinite wait loop
1:	wfi
	b       1b
2:	// We're on the main core!
	// set top of stack just before our code (stack grows to a lower address per AAPCS64)
#	ldr     x1, =_start
	ldr     x1, =0x400000

	mov	x2,x0
	// set up EL1
	mrs     x0, CurrentEL
	and     x0, x0, #12 // clear reserved bits

	// running at EL3?
	cmp     x0, #12
	bne     5f
	// should never be executed, just for completeness
	mov     x3, #0x5b1 // TWE(13)=0,TWI(12)=0,ST(11)=0
			   // RW(10)=1,SIF(9)=0,HCE(8)=1
			   // SMD(7)=1,EA(3)=0,FIQ(2)=0,IRQ(1)=0
			   // NS(0)=1
	msr     scr_el3, x3
	mov     x3, #0x3c9 // EL2.h
	msr     spsr_el3, x3
	msr	cptr_el3,xzr;
	adr     x3, 5f     // to label 5f
	msr     elr_el3, x3
	eret

	// running at EL2?
5:	cmp     x0, #4
	beq     5f
	msr     sp_el1, x1
	// enable CNTP for EL1
	mrs     x0, cnthctl_el2
	orr     x0, x0, #3
	msr     cnthctl_el2, x0
	msr     cntvoff_el2, xzr
	// enable AArch64 in EL1
	mov     x0, #(1 << 31)      // AArch64
	orr     x0, x0, #(1 << 1)   // SWIO hardwired on Pi3
	msr     hcr_el2, x0
	mrs     x0, hcr_el2
	// Setup SCTLR access
	mov     x3, #0x0800
	movk    x3, #0x30d0, lsl #16
	msr     sctlr_el1, x3
	// set up exception handlers
	ldr     x3, =_vectors
	msr     vbar_el1, x3
	// change execution level to EL1
	mov     x3, #0x3c5          // to EL1 with SP_EL1
	msr     spsr_el2, x3
	msr	cptr_el2, xzr
	adr     x3, 5f
	msr     elr_el2, x3
	eret

5:	mov     sp, x1

#	// Clean the BSS section
	ldr     x3, =__bss_start__   // Start address
	b LoopFillZerobss
	/* Zero fill the bss segment. */
FillZerobss:
	str     xzr, [x3], #8

LoopFillZerobss:
	ldr     x4, = __bss_end__
	cmp     x3,x4
	bcc     FillZerobss

	// dont trap on SIMD/Float
	mrs    x1, cpacr_el1
	mov    x0, #(3 << 20)
	orr    x0, x1, x0
	msr    cpacr_el1, x0

//	//	setup the mini uart
	bl	arch_fixups
	// Jump to our main() routine in C (make sure it doesn't return)
	bl      start_up
	// In case it does return, halt the master core too
	b       1b

_interrupt_entry:
	sub sp, sp, #800
	stp x0,x1,[sp,#16]
#	mrs x0,spsr_el1
	mrs x0,esr_el1
	mrs x1,elr_el1
	stp x0,x1,[sp,#0]
	stp x2,x3,[sp,#32]
	stp x4,x5,[sp,#48]
	stp x6,x7,[sp,#64]
	stp x8,x9,[sp,#80]
	stp x10,x11,[sp,#96]
	stp x12,x13,[sp,#112]
	stp x14,x15,[sp,#128]
	stp x16,x17,[sp,#144]
	stp x18,x19,[sp,#160]
	stp x20,x21,[sp,#176]
	stp x22,x23,[sp,#192]
	stp x24,x25,[sp,#208]
	stp x26,x27,[sp,#224]
	stp x28,x29,[sp,#240]
	stp x30,xzr,[sp,#256]


	stp q0, q1, [sp, #272]
	stp q2, q3, [sp, #304]
	stp q4, q5, [sp, #336]
	stp q6, q7, [sp, #368]
	stp q8, q9, [sp, #400]
	stp q10, q11, [sp, #432]
	stp q12, q13, [sp, #464]
	stp q14, q15, [sp, #496]
	stp q16, q17, [sp, #528]
	stp q18, q19, [sp, #560]
	stp q20, q21, [sp, #592]
	stp q22, q23, [sp, #624]
	stp q24, q25, [sp, #656]
	stp q26, q27, [sp, #688]
	stp q28, q29, [sp, #720]
	stp q30, q31, [sp, #752]

	mrs x0, FPCR
	mrs x1, FPSR
	str x0, [sp, #784]
	str x1, [sp, #792]

	mov x0,sp
	bl irq_dispatch

ret_from_irq:
	adr	x0, switch_flag
	ldr	x1, [x0]
	cbz	x1, 2f
	str	xzr,[x0]
	mov	x0,sp		// sp to x0
	bl	handle_switch
	adr	x1,current
	ldr	x2,[x1]		// x2 == ptr to current
	mov	x3,sp		// sp to x3
	str	x3, [x2, #8]	// sp to current stack data
	str	x0,[x1]		// update current
	ldr	x3,[x0, #8]	// read new thread stack pointer
	mov	sp, x3
	add	x3,sp,#800
	str	x3,[x0, #8]
	mov	w3,#1		// store 32 bit
	str	w3,[x0,#5*8]   // task state == 1 (running)

2:
	ldr x0,  [sp, #784]
	ldr x1,  [sp, #792]
	msr FPCR, x0
	msr FPSR, x1

	ldp q30, q31, [sp, #752]
	ldp q28, q29, [sp, #720]
	ldp q26, q27, [sp, #688]
	ldp q24, q25, [sp, #656]
	ldp q22, q23, [sp, #624]
	ldp q20, q21, [sp, #592]
	ldp q18, q19, [sp, #560]
	ldp q16, q17, [sp, #528]
	ldp q14, q15, [sp, #496]
	ldp q12, q13, [sp, #464]
	ldp q10, q11, [sp, #432]
	ldp q8, q9, [sp, #400]
	ldp q6, q7, [sp, #368]
	ldp q4, q5, [sp, #336]
	ldp q2, q3, [sp, #304]
	ldp q0, q1, [sp, #272]

	ldp x0,x1,[sp,#0]
	msr elr_el1,x1
	msr esr_el1,x0
	ldp x0,x1,[sp,#16]
	ldp x2,x3,[sp,#32]
	ldp x4,x5,[sp,#48]
	ldp x6,x7,[sp,#64]
	ldp x8,x9,[sp,#80]
	ldp x10,x11,[sp,#96]
	ldp x12,x13,[sp,#112]
	ldp x14,x15,[sp,#128]
	ldp x16,x17,[sp,#144]
	ldp x18,x19,[sp,#160]
	ldp x20,x21,[sp,#176]
	ldp x22,x23,[sp,#192]
	ldp x24,x25,[sp,#208]
	ldp x26,x27,[sp,#224]
	ldp x28,x29,[sp,#240]
	ldp x30,xzr,[sp,#256]
	add sp,sp,#800
	eret

_interrupt_entry_2:
	sub sp, sp, #800
	stp x0,x1,[sp,#16]
#	mrs x0,spsr_el1
	mrs x0,esr_el1
	mrs x1,elr_el1
	stp x0,x1,[sp,#0]
	stp x2,x3,[sp,#32]
	stp x4,x5,[sp,#48]
	stp x6,x7,[sp,#64]
	stp x8,x9,[sp,#80]
	stp x10,x11,[sp,#96]
	stp x12,x13,[sp,#112]
	stp x14,x15,[sp,#128]
	stp x16,x17,[sp,#144]
	stp x18,x19,[sp,#160]
	stp x20,x21,[sp,#176]
	stp x22,x23,[sp,#192]
	stp x24,x25,[sp,#208]
	stp x26,x27,[sp,#224]
	stp x28,x29,[sp,#240]
	stp x30,xzr,[sp,#256]


	stp q0, q1, [sp, #272]
	stp q2, q3, [sp, #304]
	stp q4, q5, [sp, #336]
	stp q6, q7, [sp, #368]
	stp q8, q9, [sp, #400]
	stp q10, q11, [sp, #432]
	stp q12, q13, [sp, #464]
	stp q14, q15, [sp, #496]
	stp q16, q17, [sp, #528]
	stp q18, q19, [sp, #560]
	stp q20, q21, [sp, #592]
	stp q22, q23, [sp, #624]
	stp q24, q25, [sp, #656]
	stp q26, q27, [sp, #688]
	stp q28, q29, [sp, #720]
	stp q30, q31, [sp, #752]

	mrs x0, FPCR
	mrs x1, FPSR
	str x0, [sp, #784]
	str x1, [sp, #792]

	mov x0,sp
	bl irq_dispatch

ret_from_irq_2:
	adr	x0, switch_flag
	ldr	x1, [x0]
	cbz	x1, 2f
	str	xzr,[x0]
	mov	x0,sp		// sp to x0
	bl	handle_switch
	adr	x1,current
	ldr	x2,[x1]		// x2 == ptr to current
	mov	x3,sp		// sp to x3
	str	x3, [x2, #8]	// sp to current stack data
	str	x0,[x1]		// update current
	ldr	x3,[x0, #8]	// read new thread stack pointer
	mov	sp, x3
	add	x3,sp,#800
	str	x3,[x0, #8]
	mov	w3,#1		// store 32 bit
	str	w3,[x0,#5*8]   // task state == 1 (running)

2:
	ldr x0,  [sp, #784]
	ldr x1,  [sp, #792]
	msr FPCR, x0
	msr FPSR, x1

	ldp q30, q31, [sp, #752]
	ldp q28, q29, [sp, #720]
	ldp q26, q27, [sp, #688]
	ldp q24, q25, [sp, #656]
	ldp q22, q23, [sp, #624]
	ldp q20, q21, [sp, #592]
	ldp q18, q19, [sp, #560]
	ldp q16, q17, [sp, #528]
	ldp q14, q15, [sp, #496]
	ldp q12, q13, [sp, #464]
	ldp q10, q11, [sp, #432]
	ldp q8, q9, [sp, #400]
	ldp q6, q7, [sp, #368]
	ldp q4, q5, [sp, #336]
	ldp q2, q3, [sp, #304]
	ldp q0, q1, [sp, #272]

	ldp x0,x1,[sp,#0]
	msr elr_el1,x1
	msr esr_el1,x0
	ldp x0,x1,[sp,#16]
	ldp x2,x3,[sp,#32]
	ldp x4,x5,[sp,#48]
	ldp x6,x7,[sp,#64]
	ldp x8,x9,[sp,#80]
	ldp x10,x11,[sp,#96]
	ldp x12,x13,[sp,#112]
	ldp x14,x15,[sp,#128]
	ldp x16,x17,[sp,#144]
	ldp x18,x19,[sp,#160]
	ldp x20,x21,[sp,#176]
	ldp x22,x23,[sp,#192]
	ldp x24,x25,[sp,#208]
	ldp x26,x27,[sp,#224]
	ldp x28,x29,[sp,#240]
	ldp x30,xzr,[sp,#256]
	add sp,sp,#800
	eret


_handle_trap:
	sub sp, sp, #800
	stp x0,x1,[sp,#16]
#	mrs x0,spsr_el1
	mrs x0,esr_el1
	mrs x1,elr_el1
	stp x0,x1,[sp,#0]
	stp x2,x3,[sp,#32]
	stp x4,x5,[sp,#48]
	stp x6,x7,[sp,#64]
	stp x8,x9,[sp,#80]
	stp x10,x11,[sp,#96]
	stp x12,x13,[sp,#112]
	stp x14,x15,[sp,#128]
	stp x16,x17,[sp,#144]
	stp x18,x19,[sp,#160]
	stp x20,x21,[sp,#176]
	stp x22,x23,[sp,#192]
	stp x24,x25,[sp,#208]
	stp x26,x27,[sp,#224]
	stp x28,x29,[sp,#240]
	stp x30,xzr,[sp,#256]


	stp q0, q1, [sp, #272]
	stp q2, q3, [sp, #304]
	stp q4, q5, [sp, #336]
	stp q6, q7, [sp, #368]
	stp q8, q9, [sp, #400]
	stp q10, q11, [sp, #432]
	stp q12, q13, [sp, #464]
	stp q14, q15, [sp, #496]
	stp q16, q17, [sp, #528]
	stp q18, q19, [sp, #560]
	stp q20, q21, [sp, #592]
	stp q22, q23, [sp, #624]
	stp q24, q25, [sp, #656]
	stp q26, q27, [sp, #688]
	stp q28, q29, [sp, #720]
	stp q30, q31, [sp, #752]

	mrs x0, FPCR
	mrs x1, FPSR
	str x0, [sp, #784]
	str x1, [sp, #792]

	mov x0,sp
	mrs     x1, esr_el1
	mrs     x2, elr_el1
	mrs     x3, spsr_el1
	mrs     x4, far_el1
	mrs     x5, sctlr_el1
	bl handle_trap

ret_from_irq_1:
	adr	x0, switch_flag
	ldr	x1, [x0]
	cbz	x1, 2f
	str	xzr,[x0]
	mov	x0,sp		// sp to x0
	bl	handle_switch
	adr	x1,current
	ldr	x2,[x1]		// x2 == ptr to current
	mov	x3,sp		// sp to x3
	str	x3, [x2, #8]	// sp to current stack data
	str	x0,[x1]		// update current
	ldr	x3,[x0, #8]	// read new thread stack pointer
	mov	sp, x3
	add	x3,sp,#800
	str	x3,[x0, #8]
	mov	w3,#1		// store 32 bit
	str	w3,[x0,#5*8]   // task state == 1 (running)

2:
	ldr x0,  [sp, #784]
	ldr x1,  [sp, #792]
	msr FPCR, x0
	msr FPSR, x1

	ldp q30, q31, [sp, #752]
	ldp q28, q29, [sp, #720]
	ldp q26, q27, [sp, #688]
	ldp q24, q25, [sp, #656]
	ldp q22, q23, [sp, #624]
	ldp q20, q21, [sp, #592]
	ldp q18, q19, [sp, #560]
	ldp q16, q17, [sp, #528]
	ldp q14, q15, [sp, #496]
	ldp q12, q13, [sp, #464]
	ldp q10, q11, [sp, #432]
	ldp q8, q9, [sp, #400]
	ldp q6, q7, [sp, #368]
	ldp q4, q5, [sp, #336]
	ldp q2, q3, [sp, #304]
	ldp q0, q1, [sp, #272]

	ldp x0,x1,[sp,#0]
	msr elr_el1,x1
	msr esr_el1,x0
	ldp x0,x1,[sp,#16]
	ldp x2,x3,[sp,#32]
	ldp x4,x5,[sp,#48]
	ldp x6,x7,[sp,#64]
	ldp x8,x9,[sp,#80]
	ldp x10,x11,[sp,#96]
	ldp x12,x13,[sp,#112]
	ldp x14,x15,[sp,#128]
	ldp x16,x17,[sp,#144]
	ldp x18,x19,[sp,#160]
	ldp x20,x21,[sp,#176]
	ldp x22,x23,[sp,#192]
	ldp x24,x25,[sp,#208]
	ldp x26,x27,[sp,#224]
	ldp x28,x29,[sp,#240]
	ldp x30,xzr,[sp,#256]
	add sp,sp,#800
	eret

_handle_trap2:
	sub sp, sp, #800
	stp x0,x1,[sp,#16]
#	mrs x0,spsr_el1
	mrs x0,esr_el1
	mrs x1,elr_el1
	stp x0,x1,[sp,#0]
	stp x2,x3,[sp,#32]
	stp x4,x5,[sp,#48]
	stp x6,x7,[sp,#64]
	stp x8,x9,[sp,#80]
	stp x10,x11,[sp,#96]
	stp x12,x13,[sp,#112]
	stp x14,x15,[sp,#128]
	stp x16,x17,[sp,#144]
	stp x18,x19,[sp,#160]
	stp x20,x21,[sp,#176]
	stp x22,x23,[sp,#192]
	stp x24,x25,[sp,#208]
	stp x26,x27,[sp,#224]
	stp x28,x29,[sp,#240]
	stp x30,xzr,[sp,#256]


	stp q0, q1, [sp, #272]
	stp q2, q3, [sp, #304]
	stp q4, q5, [sp, #336]
	stp q6, q7, [sp, #368]
	stp q8, q9, [sp, #400]
	stp q10, q11, [sp, #432]
	stp q12, q13, [sp, #464]
	stp q14, q15, [sp, #496]
	stp q16, q17, [sp, #528]
	stp q18, q19, [sp, #560]
	stp q20, q21, [sp, #592]
	stp q22, q23, [sp, #624]
	stp q24, q25, [sp, #656]
	stp q26, q27, [sp, #688]
	stp q28, q29, [sp, #720]
	stp q30, q31, [sp, #752]

	mrs x0, FPCR
	mrs x1, FPSR
	str x0, [sp, #784]
	str x1, [sp, #792]

	mov x0,sp
	mrs     x1, esr_el1
	mrs     x2, elr_el1
	mrs     x3, spsr_el1
	mrs     x4, far_el1
	mrs     x5, sctlr_el1
	bl handle_trap

ret_from_irq_3:
	adr	x0, switch_flag
	ldr	x1, [x0]
	cbz	x1, 2f
	str	xzr,[x0]
	mov	x0,sp		// sp to x0
	bl	handle_switch
	adr	x1,current
	ldr	x2,[x1]		// x2 == ptr to current
	mov	x3,sp		// sp to x3
	str	x3, [x2, #8]	// sp to current stack data
	str	x0,[x1]		// update current
	ldr	x3,[x0, #8]	// read new thread stack pointer
	mov	sp, x3
	add	x3,sp,#800
	str	x3,[x0, #8]
	mov	w3,#1		// store 32 bit
	str	w3,[x0,#5*8]   // task state == 1 (running)

2:
	ldr x0,  [sp, #784]
	ldr x1,  [sp, #792]
	msr FPCR, x0
	msr FPSR, x1

	ldp q30, q31, [sp, #752]
	ldp q28, q29, [sp, #720]
	ldp q26, q27, [sp, #688]
	ldp q24, q25, [sp, #656]
	ldp q22, q23, [sp, #624]
	ldp q20, q21, [sp, #592]
	ldp q18, q19, [sp, #560]
	ldp q16, q17, [sp, #528]
	ldp q14, q15, [sp, #496]
	ldp q12, q13, [sp, #464]
	ldp q10, q11, [sp, #432]
	ldp q8, q9, [sp, #400]
	ldp q6, q7, [sp, #368]
	ldp q4, q5, [sp, #336]
	ldp q2, q3, [sp, #304]
	ldp q0, q1, [sp, #272]

	ldp x0,x1,[sp,#0]
	msr elr_el1,x1
	msr esr_el1,x0
	ldp x0,x1,[sp,#16]
	ldp x2,x3,[sp,#32]
	ldp x4,x5,[sp,#48]
	ldp x6,x7,[sp,#64]
	ldp x8,x9,[sp,#80]
	ldp x10,x11,[sp,#96]
	ldp x12,x13,[sp,#112]
	ldp x14,x15,[sp,#128]
	ldp x16,x17,[sp,#144]
	ldp x18,x19,[sp,#160]
	ldp x20,x21,[sp,#176]
	ldp x22,x23,[sp,#192]
	ldp x24,x25,[sp,#208]
	ldp x26,x27,[sp,#224]
	ldp x28,x29,[sp,#240]
	ldp x30,xzr,[sp,#256]
	add sp,sp,#800
	eret

do_switch:
	msr daifset,#2   // irq off
	sub sp,sp,#800   // allocate a full frame
	stp x0,x1,[sp,#16]
#	mrs x0,spsr_el1
	mrs x0,esr_el1
	mrs x1,elr_el1
	stp x0,x1,[sp,#0]
	stp x2,x3,[sp,#32]
	stp x4,x5,[sp,#48]
	stp x6,x7,[sp,#64]
	stp x8,x9,[sp,#80]
	stp x10,x11,[sp,#96]
	stp x12,x13,[sp,#112]
	stp x14,x15,[sp,#128]
	stp x16,x17,[sp,#144]
	stp x18,x19,[sp,#160]
	stp x20,x21,[sp,#176]
	stp x22,x23,[sp,#192]
	stp x24,x25,[sp,#208]
	stp x26,x27,[sp,#224]
	stp x28,x29,[sp,#240]
	stp x30,xzr,[sp,#256]


	stp q0, q1, [sp, #272]
	stp q2, q3, [sp, #304]
	stp q4, q5, [sp, #336]
	stp q6, q7, [sp, #368]
	stp q8, q9, [sp, #400]
	stp q10, q11, [sp, #432]
	stp q12, q13, [sp, #464]
	stp q14, q15, [sp, #496]
	stp q16, q17, [sp, #528]
	stp q18, q19, [sp, #560]
	stp q20, q21, [sp, #592]
	stp q22, q23, [sp, #624]
	stp q24, q25, [sp, #656]
	stp q26, q27, [sp, #688]
	stp q28, q29, [sp, #720]
	stp q30, q31, [sp, #752]

	mrs x0, FPCR
	mrs x1, FPSR
	str x0, [sp, #784]
	str x1, [sp, #792]

	mov x0,sp
	b	ret_from_irq




.balign	2048
_vectors:
	// Exception from the current EL while using SP_EL0
	// synchronous  @ 0x0000
	b	_handle_trap

#	mov     x0, #0
#	mrs     x1, esr_el1
#	mrs     x2, elr_el1
#	mrs     x3, spsr_el1
#	mrs     x4, far_el1
#	mrs     x5, sctlr_el1
#	b       exc_handler

.balign  0x80
	// IRQ	@ 0x0080
	b	_interrupt_entry

.balign  0x80
	// FIQ @ 0x0100
	mov     x0, #2
	mrs     x1, esr_el1
	mrs     x2, elr_el1
	mrs     x3, spsr_el1
	mrs     x4, far_el1
	b       exc_handler

.balign	0x80
	// SError @ 0x0180
	mov     x0, #3
	mrs     x1, esr_el1
	mrs     x2, elr_el1
	mrs     x3, spsr_el1
	mrs     x4, far_el1
	b       exc_handler

.balign	0x80
	// Exception from the current EL while using SP_ELx
	// synchronous @ 0x0200
	b	_handle_trap2
#	mov     x0, #0
#	mrs     x1, esr_el1
#	mrs     x2, elr_el1
#	mrs     x3, spsr_el1
#	mrs     x4, far_el1
#	mrs     x5, sctlr_el1
#	b       exc_handler

.balign  0x80
	// IRQ @ 0x0280
	b	_interrupt_entry_2

.balign  0x80
	// FIQ @ 0x0300
	mov     x0, #2
	mrs     x1, esr_el1
	mrs     x2, elr_el1
	mrs     x3, spsr_el1
	mrs     x4, far_el1
	b       exc_handler

.balign	0x80
	// SError @ 0x0380
	mov     x0, #3
	mrs     x1, esr_el1
	mrs     x2, elr_el1
	mrs     x3, spsr_el1
	mrs     x4, far_el1
	b       exc_handler

.balign	0x80
	// Exception from a lower EL and at least one lower EL is AArch64
	// synchronous @ 0x0400
	mov     x0, #0
	mrs     x1, esr_el1
	mrs     x2, elr_el1
	mrs     x3, spsr_el1
	mrs     x4, far_el1
	mrs     x5, sctlr_el1
	b       exc_handler

.balign  0x80
	// IRQ @ 0x480
#	b	_interrupt_entry
	mov     x0, #1
	mrs     x1, esr_el1
	mrs     x2, elr_el1
	mrs     x3, spsr_el1
	mrs     x4, far_el1
	mrs     x5, sctlr_el1
	b       exc_handler

.balign  0x80
	// FIQ @ 0x0500
	mov     x0, #2
	mrs     x1, esr_el1
	mrs     x2, elr_el1
	mrs     x3, spsr_el1
	mrs     x4, far_el1
	b       exc_handler

.balign	0x80
	// SError @ 0x0580
	mov     x0, #3
	mrs     x1, esr_el1
	mrs     x2, elr_el1
	mrs     x3, spsr_el1
	mrs     x4, far_el1
	b       exc_handler

.balign	0x80
	// Exception from a lower EL and all lower ELs are AArch32
	// synchronous @ 0x0600
	mov     x0, #0
	mrs     x1, esr_el1
	mrs     x2, elr_el1
	mrs     x3, spsr_el1
	mrs     x4, far_el1
	mrs     x5, sctlr_el1
	b       exc_handler

.balign  0x80
	// IRQ @ 0x680
	mov     x0, #1
	mrs     x1, esr_el1
	mrs     x2, elr_el1
	mrs     x3, spsr_el1
	mrs     x4, far_el1
	b       exc_handler
#	b	_interrupt_entry

.balign  0x80
	// FIQ @ 0x0700
	mov     x0, #2
	mrs     x1, esr_el1
	mrs     x2, elr_el1
	mrs     x3, spsr_el1
	mrs     x4, far_el1
	b       exc_handler

.balign	0x80
	// SError @ 0x0780
	mov     x0, #3
	mrs     x1, esr_el1
	mrs     x2, elr_el1
	mrs     x3, spsr_el1
	mrs     x4, far_el1
	b       exc_handler

