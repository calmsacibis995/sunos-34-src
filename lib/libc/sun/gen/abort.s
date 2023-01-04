	.data
/*	.asciz	"@(#)abort.s 1.1 86/09/24 SMI"	*/
	.text

|	abort -- re-invented Oct 82

	.globl	_abort
	.text
_abort:
	link	a6,#0	| to make adb happy
	stop	#0
	clrl	d0
	unlk	a6
	rts
