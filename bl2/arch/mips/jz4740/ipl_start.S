
#include "config.h"
#include <regdef.h>


	.set noreorder

	.globl	_start
	.text
_start:
	.word JZ4740_BOOT_NAND_CFG
reset:
	/* Status register */
        /* 
         * CU0=UM=EXL=IE=0, BEV=ERL=1, IP2~7=1
         */
        li      t0, 0x0040FC04
        mtc0    t0, CP0_STATUS
	/* Cause register */
	/* IV=1, use the specical interrupt vector (0x200) */
        li      t1, 0x00800000
        mtc0    t1, CP0_CAUSE
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
	la      sp, 0x80004000
        la      t9, nand_boot
        j       t9
        nop
