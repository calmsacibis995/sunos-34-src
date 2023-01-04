/* @(#)brk.c 1.1 86/09/24 SMI; from UCB 4.1 82/12/04 */

#include "SYS.h"

#define	SYS_brk		17

	.globl	curbrk
#ifdef vax
SYSCALL(brk)
	movl	4(ap),curbrk
#endif
#ifdef sun
ENTRY(brk)
	movl	PARAM,d0	| round up new break to a multiple of wordsize
	addql	#3,d0
	moveq	#~3,d1
	andl	d1,d0
	movl	d0,PARAM
	pea	SYS_brk
	trap	#0
	jcs	cerror
	movl	PARAM,curbrk
#endif
	RET
