	.data
	.asciz	"@(#)stack.s 1.1 86/09/25"
	.even
	.text
|
| Copyright (c) 1985 by Sun Microsystems, Inc.
|

|
| The stack for diag is allocated after the end of the program
|
	STACKSIZE = 0x8000

	.globl	_start, _end, _main
_start:
	movl	#_end,a0
	addl	#STACKSIZE,a0
	movl	sp,a0@-
	movl	a0,sp
	jsr	_main
	movl	sp@+,a0
	movl	a0,sp
	rts
