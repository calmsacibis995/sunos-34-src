	.data
	.asciz	"@(#)sigtramp.c 1.1 86/09/24 Copyr 1985 Sun Micro"
	.even
	.text

|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

	.globl	__sigtramp, __sigfunc
__sigtramp:
	moveml	#0xC0C0,sp@-	/* save C scratch regs */
	jsr	fp_save		| Save floating point state.
	movl	sp@(16+FPSAVESIZE),d0	/* get signal number */
	lsll	#2,d0		/* scale for index */
	movl	#__sigfunc,a0	/* get array of func ptrs */
	movl	a0@(0,d0:l),a0	/* get func */
	movl	sp@(0+24+FPSAVESIZE),sp@-	/* push scp address */
	movl	sp@(4+20+FPSAVESIZE),sp@-	/* push code */
	movl	sp@(8+16+FPSAVESIZE),sp@-	/* push signal number */
	jsr	a0@		/* call handler */
	addl	#12,sp		/* pop args */
	jsr	fp_restore	| Restore floating point state.
	moveml	sp@+,#0x0303	/* restore regs */
	addl	#8,sp		/* pop signo and code */
	pea	139
	trap	#0
	/*NOTREACHED*/
