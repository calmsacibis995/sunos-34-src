/* @(#)sbrk.c 1.1 86/09/24 SMI; from UCB 4.1 82/12/04 */

#include "SYS.h"

#define	SYS_brk		17

	.globl	curbrk
	.globl	_end
	.data
curbrk:	.long	_end
	.text

ENTRY(sbrk)
#if vax
	addl3	curbrk,4(ap),-(sp)
	pushl	$1
	movl	ap,r3
	movl	sp,ap
	chmk	$SYS_brk
	jcs 	err
	movl	curbrk,r0
	addl2	4(r3),curbrk
#endif
#if sun
	movl	PARAM,d0
	addql	#3,d0		| round up request to a multiple of wordsize
	moveq	#~3,d1
	andl	d1,d0
	movl	d0,a0
	movl	curbrk,d0
	addql	#3,d0		| round up curbrk to a multiple of wordsize
	andl	d1,d0
	movl	d0,curbrk
	addl	d0,a0
	movl	a0,PARAM	| PARAM = curbrk + request (new break setting)
	pea	SYS_brk
	trap	#0
	jcs	err
	movl	curbrk,d0
	movl	PARAM,curbrk
#endif
	RET
err:
	jmp	cerror
