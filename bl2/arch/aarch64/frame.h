typedef struct uint128b {
	unsigned long int b128[2];
} u128_t;

/*
 * ARM64 Exception / Trap Frame Layout (S_FRAME_SIZE = 288 / 0x120 bytes)
 *
 * Stack pointer (SP_EL1) must remain 16-byte aligned at all times.
 *
 * Byte Offset | Index      | Register / Field | Notes
 * ------------+------------+------------------+---------------------------------------
 * 0x000-0x0ef | regs[0..29]| x0 - x29         | General purpose registers (30 * 8B)
 * 0x0f0       | regs[30]   | x30 (LR)         | Return address
 * 0x0f8       | sp_el0     | SP_EL0           | Saved/restored only if (spsr & 1) == 0
 * 0x100       | elr_el1    | ELR_EL1          | Exception return address (PC)
 * 0x108       | spsr_el1   | SPSR_EL1         | 0x0 = EL0t (user), 0x5 = EL1h (kernel)
 * 0x110       | irq_lev    | irq_lev (32-bit) | Nested IRQ depth (upper 32b unused)
 * 0x118       | pad        | 64-bit pad/free  | Keeps 16B alignment (usable for TPIDR)
 */

#define S_FRAME_SIZE    288     /* 18 * 16 bytes */

#ifndef __ASSEMBLER__
struct trap_frame {
    unsigned long regs[31];     /* 0x000: x0..x30 */
    unsigned long sp_el0;       /* 0x0f8: valid when (spsr_el1 & 1) == 0 */
    unsigned long elr_el1;      /* 0x100 */
    unsigned long spsr_el1;     /* 0x108 */
    unsigned int  irq_lev;      /* 0x110 */
    unsigned int  _pad0;        /* 0x114 */
    unsigned long _free;        /* 0x118: 16-byte alignment padding */
};
#endif


#if 0
struct fullframe {
	unsigned long int x0;	// sp + 0x00
	unsigned long int x1;	// sp + 0x08
	unsigned long int x2;	// sp + 0x10
	unsigned long int x3;	// sp + 0x18
	unsigned long int x4;	// sp + 0x20
	unsigned long int x5;	// sp + 0x28
	unsigned long int x6;	// sp + 0x30
	unsigned long int x7;	// sp + 0x38
	unsigned long int x8;	// sp + 0x40
	unsigned long int x9;	// sp + 0x48
	unsigned long int x10;	// sp + 0x50
	unsigned long int x11;	// sp + 0x58
	unsigned long int x12;	// sp + 0x60
	unsigned long int x13;	// sp + 0x68
	unsigned long int x14;	// sp + 0x70
	unsigned long int x15;	// sp + 0x78
	unsigned long int x16;	// sp + 0x80
	unsigned long int x17;	// sp + 0x88
	unsigned long int x18;	// sp + 0x90
	unsigned long int x19;	// sp + 0x98
	unsigned long int x20;	// sp + 0xa0
	unsigned long int x21;	// sp + 0xa8
	unsigned long int x22;	// sp + 0xb0
	unsigned long int x23;	// sp + 0xb8
	unsigned long int x24;	// sp + 0xc0
	unsigned long int x25;	// sp + 0xc8
	unsigned long int x26;	// sp + 0xd0
	unsigned long int x27;	// sp + 0xd8
	unsigned long int x28;  // sp + 0xe0
	unsigned long int x29;	// sp + 0xe8
	unsigned long int x30;	// sp + 0xf0
	unsigned long int sp;	// sp + 0xf8, user stack fd 1 frame
	unsigned long int elr_el1; // sp + 0x100
	unsigned long int spsr_el1;// sp + 0x108
	unsigned int	irq_lev;  // sp + 0x110
	unsigned int    pad0;	// sp + 0x114
	unsigned long int pad1  // sp + 0x118
};
#endif

#if 0
struct fullframe {
	unsigned long int esr_el1;
	unsigned long int slr_el1;
	unsigned long int x0;
	unsigned long int x1;
	unsigned long int x2;
	unsigned long int x3;
	unsigned long int x4;
	unsigned long int x5;
	unsigned long int x6;
	unsigned long int x7;
	unsigned long int x8;
	unsigned long int x9;
	unsigned long int x10;
	unsigned long int x11;
	unsigned long int x12;
	unsigned long int x13;
	unsigned long int x14;
	unsigned long int x15;
	unsigned long int x16;
	unsigned long int x17;
	unsigned long int x18;
	unsigned long int x19;
	unsigned long int x20;
	unsigned long int x21;
	unsigned long int x22;
	unsigned long int x23;
	unsigned long int x24;
	unsigned long int x25;
	unsigned long int x26;
	unsigned long int x27;
	unsigned long int x28;
	unsigned long int x29;
	unsigned long int x30;
	unsigned long int dum;
	u128_t q0;
	u128_t q1;
	u128_t q2;
	u128_t q3;
	u128_t q4;
	u128_t q5;
	u128_t q6;
	u128_t q7;
	u128_t q8;
	u128_t q9;
	u128_t q10;
	u128_t q11;
	u128_t q12;
	u128_t q13;
	u128_t q14;
	u128_t q15;
	u128_t q16;
	u128_t q17;
	u128_t q18;
	u128_t q19;
	u128_t q20;
	u128_t q21;
	u128_t q22;
	u128_t q23;
	u128_t q24;
	u128_t q25;
	u128_t q26;
	u128_t q27;
	u128_t q28;
	u128_t q29;
	u128_t q30;
	u128_t q31;
	unsigned long int fpcr;
	unsigned long int fpsr;
};
#endif
