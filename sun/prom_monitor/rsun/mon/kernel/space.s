| 	@(#)space.s 1.4 83/09/19 Copyright (c) 1983 by Sun Microsystems, Inc.
|
|	space.s -- the final frontier.
|
|	This module provides access from 'C' to the movs instruction
|	on the 68000.  It could be provided with an asm.sed script but
|	those don't always work and are horrible.
|
|	Better actual idea is to change the 'C' optimizer to do this
|	if you feed it a flag.
|
|	We save and restore the function code control registers since
|	we might be getting called from an interrupt routine, and don't
|	want to either (1) force interrupt routines to save & restore
|	these regs, or (2) clobber the mainline's registers.
|

	.globl	_getsb, _getsw, _getsl
	.globl	_putsb, _putsw, _putsl


_getsb:	movl	sp@(8),a1	| Get space number
	movc	sfc,d1		| Save old source function code
	movc	a1,sfc		| Set into function code register
	movl	sp@(4),a0	| Get address to touch
	moveq	#0,d0		| Clear result register
	movsb	a0@,d0		| Pick up a byte
	movc	d1,sfc		| Restore old source function code
	rts

_getsw:	movl	sp@(8),a1	| Get space number
	movc	sfc,d1		| Save old source function code
	movc	a1,sfc		| Set into function code register
	movl	sp@(4),a0	| Get address to touch
	moveq	#0,d0		| Clear result register
	movsw	a0@,d0		| Pick up a word
	movc	d1,sfc		| Restore old source function code
	rts

_getsl:	movl	sp@(8),a1	| Get space number
	movc	sfc,d1		| Save old source function code
	movc	a1,sfc		| Set into function code register
	movl	sp@(4),a0	| Get address to touch
	movsl	a0@,d0		| Pick up a long
	movc	d1,sfc		| Restore old source function code
	rts

_putsb:	movl	sp@(8),a1	| Get space number
	movc	dfc,d1		| Save old dest function code
	movc	a1,dfc		| Set into function code register
	movl	sp@(4),a0	| Get address to touch
	movl	sp@(12),d0	| Get value to store
	movsb	d0,a0@		| Store a byte
	movc	d1,dfc		| Restore old dest function code
	rts

_putsw:	movl	sp@(8),a1	| Get space number
	movc	dfc,d1		| Save old dest function code
	movc	a1,dfc		| Set into function code register
	movl	sp@(4),a0	| Get address to touch
	movl	sp@(12),d0	| Get value to store
	movsw	d0,a0@		| Store a word
	movc	d1,dfc		| Restore old dest function code
	rts

_putsl:	movl	sp@(8),a1	| Get space number
	movc	dfc,d1		| Save old dest function code
	movc	a1,dfc		| Set into function code register
	movl	sp@(4),a0	| Get address to touch
	movl	sp@(12),d0	| Get value to store
	movsl	d0,a0@		| Store a long
	movc	d1,dfc		| Restore old dest function code
	rts
