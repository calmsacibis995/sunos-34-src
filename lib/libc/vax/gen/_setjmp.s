	.data
	.asciz	"@(#)_setjmp.s 1.1 86/09/24 SMI"; /* from UCB 4.2 83/07/02 */
	.text
/*
 * C library -- _setjmp, _longjmp
 *
 *	_longjmp(a,v)
 * will generate a "return(v)" from
 * the last call to
 *	_setjmp(a)
 * by restoring registers from the stack,
 * The previous signal state is NOT restored.
 */

#include "DEFS.h"

ENTRY(_setjmp)
	movl	4(ap),r0
	movl	12(fp),(r0)		# save frame pointer of caller
	movl	16(fp),4(r0)		# save pc of caller
	clrl	r0
	ret

ENTRY(_longjmp)
	movl	8(ap),r0		# return(v)
	movl	4(ap),r1		# fetch buffer
	tstl	(r1)
	beql	botch
loop:
	bitw	$1,6(fp)		# r0 saved?
	beql	1f
	movl	r0,20(fp)
	bitw	$2,6(fp)		# was r1 saved?
	beql	2f
	movl	r1,24(fp)
	brb	2f
1:
	bitw	$2,6(fp)		# was r1 saved?
	beql	2f
	movl	r1,20(fp)
2:
	cmpl	(r1),12(fp)
	beql	done
	blssu	botch
	movl	$loop,16(fp)
	ret				# pop another frame

done:
	cmpb	*16(fp),reiins		# returning to an "rei"?
	bneq	1f
	movab	3f,16(fp)		# do return w/ psl-pc pop
	brw	2f
1:
	movab	4f,16(fp)		# do standard return
2:
	ret				# unwind stack before signals enabled
3:
	addl2	$8,sp			# compensate for PSL-PC push
4:
	jmp	*4(r1)			# done, return....

botch:
	pushl	$msgend-msg
	pushl	$msg
	pushl	$2
	calls	$3,_write
	halt

	.data
msg:	.ascii	"_longjmp botch\n"
msgend:
reiins:	rei
