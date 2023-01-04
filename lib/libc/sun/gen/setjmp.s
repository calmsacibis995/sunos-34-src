	.data
/*	.asciz	"@(#)setjmp.s 1.1 86/09/24 SMI"	*/
	.text

#include "DEFS.h"
|setjmp, longjmp
|
|	longjmp(a, v)
|causes a "return(v)" from the
|last call to
|
|	setjmp(v)
|by restoring d2-d7,a2-a7 (all the register variables) and
|adjusting the stack
|
|jmp_buf is set up as:
|
|	_________________
|	|	pc	|
|	-----------------
|	|    sigmask	|
|	-----------------
|	|   onsigstack	|
|	-----------------
|	|	d2	|
|	-----------------
|	|	...	|
|	-----------------
|	|	d7	|
|	-----------------
|	|	a2	|
|	-----------------
|	|	...	|
|	-----------------
|	|	a7	|
|	-----------------

	.text
	.globl	_sigblock

ENTRY(setjmp)
	pea	0
	jbsr	_sigblock
	addql	#4,sp
	movl	PARAM,a0	/* pointer to jmp_buf */
	movl	PARAM0,a0@	/* pc */
	movl	d0,a0@(4)	/* sigmask */
	clrl	a0@(8)		/* ### should be onsigstack ### */
#ifdef PROF
	unlk	a6
#endif PROF
	moveml	#0xFCFC,a0@(12)	/* d2-d7, a2-a7 */
	clrl	d0		/* return 0 */
	rts

ENTRY(longjmp)
	movl	PARAM,a0	/* pointer to jmp_buf */
	movl	a0@(4),sp@-
	jbsr	_sigsetmask
	addql	#4,sp
	movl	PARAM,a0	/* pointer to jmp_buf */
	movl	PARAM2,d0	/* value returned */
	bne	1$
	moveq	#1,d0
1$:
	/* ### should restore onsigstack value ### */
	movl	a0@,a1		/* a1 = return address (to setjmp call site) */
	moveml	a0@(12),#0xFCFC	/* restore d2-d7, a2-a7 */
|				/* note: at this point, a0@ is out of bounds */
	addql	#4,sp		/* delete return value word from stack */
	jmp	a1@
