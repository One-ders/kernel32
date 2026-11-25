
#include "sys_const.h"
//  Arm Programmer's Guide:  https://developer.arm.com/documentation/den0024/a/

.section ".text.boot" // Make sure the linker puts this at the start of the kernel image

.global _start  // Execution starts here
.global do_switch

_start:
	mov	x19,x0
	// Check processor ID is zero (executing on main core), else hang
	mrs     x1, mpidr_el1
	and     x1, x1, #0xff
	cbz     x1, 2f
	b       1f
	// We're not on the main core, so hang in an infinite wait loop
1:	wfi
	b       1b
2:	// We're on the main core!
	ldr	x0,=SCTLR_VALUE_MMU_DISABLED
	msr	sctlr_el1,x0

	// set top of stack just before our code (stack grows to a lower address per AAPCS64)
//	ldr     x1, =_start
//	ldr     x1, =0x400000

	// get current EL
	mrs     x0, CurrentEL
	and     x0, x0, #0x0c // clear reserved bits

	// running at EL3?
	cmp     x0, #0x0c      // b'1100 is el3
	bne     5f

// set up EL3
	mov     x3, #0x5b1 // TWE(13)=0,TWI(12)=0,ST(11)=0
			   // RW(10)=1,SIF(9)=0,HCE(8)=1
			   // SMD(7)=1,EA(3)=0,FIQ(2)=0,IRQ(1)=0
			   // NS(0)=1
	msr     scr_el3, x3

	mov     x3, #0x3c9 // EL2h(9), !I&F (0xc0), !D&A (0x300)
	msr     spsr_el3, x3
	msr	cptr_el3,xzr

	adr     x3,5f     // to label 5f
	msr     elr_el3,x3
	eret

	// running at EL2?
5:	cmp     x0, #4      // 0b0100 is el1
	beq     5f
	## Setup EL2
//	msr     sp_el1, x1
	// enable CNTP for EL1
	mrs     x0, cnthctl_el2
	orr     x0, x0, #3  	// EL1PCEN || EL1PCTEN
	msr     cnthctl_el2, x0

	msr     cntvoff_el2, xzr // Counter-timer Virtual offset
	// enable AArch64 in EL1
	mov     x0, #(1 << 31)      // AArch64
	orr     x0, x0, #(1 << 1)   // SWIO hardwired on Pi3
	msr     hcr_el2, x0
	mrs     x0, hcr_el2
	// Setup SCTLR access
	mov     x3, #0x0800
	movk    x3, #0x30d0, lsl #16
	msr     sctlr_el1, x3
//	// set up exception handlers
//	ldr     x3, =_vectors
//	msr     vbar_el1, x3
	// change execution level to EL1
	mov     x3, #0x3c5 // EL1h(5), !I&F (0xc0), !D&A (0x300)
	msr     spsr_el2, x3
	msr	cptr_el2, xzr
	adr     x3, 5f
	msr     elr_el2, x3
	eret

	// handle EL1
5:
//	mov     sp, x1

#	// Clean the BSS section
	adr	x0,__bss_start__   // Start address
	adr	x1,__bss_end__
	bl	_mclr

	// dont trap on SIMD/Float
	mrs    x1, cpacr_el1
	mov    x0, #(3 << 20)
	orr    x0, x1, x0
	msr    cpacr_el1, x0

	bl	__create_page_tables

	mov	x0, #VA_START
	add	sp, x0, #LOW_MEMORY

	adrp	x0, pg_dir_121
	msr	ttbr0_el1, x0

	adrp	x0, pg_dir
	msr	ttbr1_el1, x0

	ldr	x0,=(TCR_T0SZ | TCR_T1SZ | TCR_TG0_4K | TCR_TG1_4K)
	msr	tcr_el1,x0

	ldr	x0,=(MT_DEVICE_nGnRnE_FLAGS<<(8*MT_DEVICE_nGnRnE)) | \
		(MT_NORMAL_NC_FLAGS<<(8*MT_NORMAL_NC)) | \
		(MT_NORMAL_FLAGS<<(8*MT_NORMAL))
	msr	mair_el1,x0

//	ldr	x1,=arch_fixups

	mrs	x0, sctlr_el1
	orr	x0, x0, #SCTLR_MMU_ENABLED
	orr	x0, x0, #SCTLR_I_CACHE_ENABLED
	orr	x0, x0, #SCTLR_D_CACHE_ENABLED
	msr	sctlr_el1,x0
	isb

//	blr	x1

	ldr	x1,=high_entry
	br	x1

high_entry:
	mrs	x0,tcr_el1
	orr	x0,x0,#(1<<7) // EPD0 = 1: döda 1:1 boot-mappen (pg_dir_121) direkt
	msr	tcr_el1,x0
	isb
	tlbi	vmalle1
	dsb	ish
	isb


	ldr	x0,=dtb_phys_addr
	str	x19,[x0]

	bl	serial_init

	ldr	x1,=start_up
	br	x1


//	//	setup the mini uart
	// Jump to our main() routine in C (make sure it doesn't return)
//	bl      start_up
	// In case it does return, halt the master core too
	b       1b

.macro  create_pgd_entry, tbl, virt, tmp1, tmp2
	create_table_entry \tbl, \virt, PGD_SHIFT, \tmp1, \tmp2
	create_table_entry \tbl, \virt, PUD_SHIFT, \tmp1, \tmp2
.endm

.macro  create_table_entry, tbl, virt, shift, tmp1, tmp2
	lsr     \tmp1, \virt, #\shift
	and     \tmp1, \tmp1, #PTRS_PER_TABLE - 1     // table index
	add     \tmp2, \tbl, #PAGE_SIZE
	orr     \tmp2, \tmp2, #MM_TYPE_PAGE_TABLE
	str     \tmp2, [\tbl, \tmp1, lsl #3]
	add     \tbl, \tbl, #PAGE_SIZE                // next level table page
.endm

.macro  create_block_map, tbl, phys, start, end, flags, tmp1
	lsr     \start, \start, #SECTION_SHIFT
	and     \start, \start, #PTRS_PER_TABLE - 1     // table index
	lsr     \end, \end, #SECTION_SHIFT
	and     \end, \end, #PTRS_PER_TABLE - 1         // table end index
	lsr     \phys, \phys, #SECTION_SHIFT
	mov     \tmp1, #\flags
	orr     \phys, \tmp1, \phys, lsl #SECTION_SHIFT // table entry
9999:	str     \phys, [\tbl, \start, lsl #3]           // store the entry
	add     \start, \start, #1                      // next entry
	add     \phys, \phys, #SECTION_SIZE             // next block
	cmp     \start, \end
	b.ls    9999b
.endm

__create_page_tables:
	mov     x29, x30                                // save return address

	adrp    x0, pg_dir_121
	mov     x1, #PG_DIR_SIZE
	bl      _mclr

	adrp    x0, pg_dir_121
	mov     x1, #0
	create_pgd_entry x0, x1, x2, x3

	/* Mapping boot code*/
	mov     x1, xzr                 // start mapping from physical offset 0
	mov     x2, #0                  // first virtual address
	ldr     x3, =(DEVICE_BASE - SECTION_SIZE) // last virtual address
	create_block_map x0, x1, x2, x3, MMU_FLAGS, x4

	adrp    x0, pg_dir
	mov     x1, #PG_DIR_SIZE
	bl      _mclr

	adrp    x0, pg_dir
	mov     x1, #VA_START
	create_pgd_entry x0, x1, x2, x3

        /* Mapping kernel and init stack*/
        mov     x1, xzr                 // start mapping from physical offset 0
	mov     x2, #VA_START           // first virtual address
	ldr     x3, =(VA_START + DEVICE_BASE - SECTION_SIZE) // last virtual address
	create_block_map x0, x1, x2, x3, MMU_FLAGS, x4

	/* Mapping device memory*/
	mov     x1, #DEVICE_BASE      // start mapping from device base address
	ldr     x2, =(VA_START + DEVICE_BASE)          // first virtual address
	ldr     x3, =(VA_START + PHYS_MEMORY_SIZE - SECTION_SIZE) // last virtual address
	create_block_map x0, x1, x2, x3, MMU_DEVICE_FLAGS, x4

	mov     x30, x29                             // restore return address
	ret

_mclr:
	str	xzr,[x0],#8
	subs	x1,x1,#8
	b.gt	_mclr
	ret

do_switch:
	msr daifset,#2   // irq off
	sub sp,sp,#S_FRAME_SIZE   // allocate a full frame
	stp x0,x1,[sp,#0]
	mov x1,#0x3c5
	stp x30,x1,[sp,#256]
	stp x2,x3,[sp,#16]
	stp x4,x5,[sp,#32]
	stp x6,x7,[sp,#48]
	stp x8,x9,[sp,#64]
	stp x10,x11,[sp,#80]
	stp x12,x13,[sp,#96]
	stp x14,x15,[sp,#112]
	stp x16,x17,[sp,#128]
	stp x18,x19,[sp,#144]
	stp x20,x21,[sp,#160]
	stp x22,x23,[sp,#176]
	stp x24,x25,[sp,#192]
	stp x26,x27,[sp,#208]
	stp x28,x29,[sp,#224]

#if 0


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
#endif
	mov x0,sp
	stp x30,x0,[sp,#240]
	adrp    x0, irq_lev
	ldr     w1, [x0, :lo12:irq_lev]
	str     w1, [sp, #272]          // spara irq_lev i ramen (word 34)
	b kernel_exit




#if 0
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

#endif
