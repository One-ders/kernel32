
//  Arm Programmer's Guide:  https://developer.arm.com/documentation/den0024/a/
.section ".text.boot"  // Make sure the linker puts this at the start of the kernel image

.global _start  // Execution starts here

_start:
	// Check processor ID is zero (executing on main core), else hang
	mrs     x1, mpidr_el1
	and     x1, x1, #0xff
	cbz     x1, 2f
	b       1f
	// We're not on the main core, so hang in an infinite wait loop
1:	wfe
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
	mov     x3, #0x5b1
	msr     scr_el3, x3
	mov     x3, #0x3c9
	msr     spsr_el3, x3
	adr     x3, 5f
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
	mov     x3, #0x3c4
	msr     spsr_el2, x3
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
	sub sp, sp, #192
	stp x0,x1,[sp,#0]
	stp x2,x3,[sp,#16]
	stp x4,x5,[sp,#32]
	stp x6,x7,[sp,#48]
	stp x8,x9,[sp,#64]
	stp x10,x11,[sp,#80]
	stp x12,x13,[sp,#96]
	stp x14,x15,[sp,#112]
	stp x16,x17,[sp,#128]
	stp x18,x29,[sp,#144]
	stp x30,xzr,[sp,#160]

	mrs x0,esr_el1
	mrs x1,far_el1
	stp x0,x1,[sp,#176]

	mov x0,sp
	bl irq_dispatch

	ldp x0,x1,[sp,#0]
	ldp x2,x3,[sp,#16]
	ldp x4,x5,[sp,#32]
	ldp x6,x7,[sp,#48]
	ldp x8,x9,[sp,#64]
	ldp x10,x11,[sp,#80]
	ldp x12,x13,[sp,#96]
	ldp x14,x15,[sp,#112]
	ldp x16,x17,[sp,#128]
	ldp x18,x29,[sp,#144]
	ldp x30,xzr,[sp,#160]
	add sp,sp,#192
	eret


    .align 11
_vectors:
    // synchronous
    .align  7
    mov     x0, #0
    mrs     x1, esr_el1
    mrs     x2, elr_el1
    mrs     x3, spsr_el1
    mrs     x4, far_el1
    mrs     x5, sctlr_el1
    b       exc_handler

// IRQ
.balign  0x80
	b	_interrupt_entry

    // FIQ
    .align  7
    mov     x0, #2
    mrs     x1, esr_el1
    mrs     x2, elr_el1
    mrs     x3, spsr_el1
    mrs     x4, far_el1
    b       exc_handler

    // SError
    .align  7
    mov     x0, #3
    mrs     x1, esr_el1
    mrs     x2, elr_el1
    mrs     x3, spsr_el1
    mrs     x4, far_el1
    b       exc_handler


