	.data
	.asciz	"@(#)setjmp.s 1.1 86/09/25 Copyr 1984 Sun Micro"
	.even
	.text

|	Copyright (c) 1984 by Sun Microsystems, Inc.

#include "../machine/asm_linkage.h"

| Longjmp and setjmp implement non-local gotos
| using state vectors of type label_t (13 longs).
| Registers saved are the PC, d2-d7, and a2-a7.
SAVREGS = 0xFCFC

	ENTRY(setjmp)
	movl	sp@(4),a1		| get label_t address
	moveml	#SAVREGS,a1@(4)		| save data/address regs
	movl	sp@,a1@			| save PC of caller
	clrl	d0			| return zero
	rts

	ENTRY(longjmp)
	movl	sp@(4),a1		| get label_t address
	moveml	a1@(4),#SAVREGS		| restore data/address regs
| Note: we just changed stacks
	movl	a1@,sp@			| restore PC
	movl	#1,d0			| return one
	rts	

| syscall_setjmp just saves a6 and sp
| it is called from syscall, where speed is important

	ENTRY(syscall_setjmp)
	movl	sp@(4),a1		| get label_t address
	movl	a6,a1@(4+(10*4))	| save a6
	movl	sp,a1@(4+(11*4))	| save sp
	movl	sp@,a1@			| save PC of caller
	clrl	d0			| return zero
	rts
