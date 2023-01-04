	.data
/*	.asciz	"@(#)_setjmp.s 1.1 86/09/24 SMI"	*/
	.text

#include "DEFS.h"
|setjmp, longjmp
|
|	longjmp(a, v)
|causes a "return(v)" from the
|last call to
|
|	setjmp(v)
|by restoring all the registers and
|adjusting the stack
|
|jmp_buf is set up as:
|
|	_________________
|	|	pc	|
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

ENTRY(_setjmp)
	movl	PARAM,a0	/* pointer to jmp_buf */
	movl	PARAM0,a0@	/* pc */
	clrl	a0@(4)
	clrl	a0@(8)
	moveml	#0xFCFC,a0@(12)	/* d2-d7, a2-a7 */
	clrl	d0		/* return 0 */
	RET

ENTRY(_longjmp)
	movl	PARAM,a0	/* pointer to jmp_buf */
	movl	PARAM2,d0	/* value returned */
	bne	1$
	moveq	#1,d0
1$:
	moveml	a0@(12),#0xFCFC	/* restore d2-d7, a2-a7 */
	movl	a0@,sp@		/* restore pc of call to setjmp to stack */
	rts
