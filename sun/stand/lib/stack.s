 

|
| @(#)stack.s 1.1 9/25/86 Copyright Sun Micro
|
| Allocate a big stack for diag
|
	.globl	start, _end, _main
start:
	movl	#_end,a0
	addl	#0x800,a0
	movl	sp,a0@-
	movl	a0,usp
	movl	#_end,a0
	addl	#0x1000,a0
	movl	sp,a0@-
	movl	a0,sp
	jsr	_main
	movl	sp@+,a0
	movl	a0,sp
	rts
