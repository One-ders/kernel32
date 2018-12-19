#include "regdef.h"

	.set noreorder
	.globl _start
	.text

	.org 0x0000
_start:
	bal	1f
	nop
	.word _GLOBAL_OFFSET_TABLE_
1:
	move	gp,ra
	lw	t1, 0(ra)
	move	gp,t1
	la 	t9,c_start
	j	t9
	nop
	
