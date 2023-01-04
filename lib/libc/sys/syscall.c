/* @(#)syscall.c 1.1 86/09/24 SMI; from UCB 4.2 83/06/27 */

#include "SYS.h"

ENTRY(syscall)
#if vax
	movl	4(ap),r0	/* syscall number */
	subl3	$1,(ap)+,(ap)	/* one fewer arguments */
	chmk	r0
	RET
#endif
#if sun
	movl	sp@(4),d0
	movl	sp@,sp@(4)
	movl	d0,sp@
	trap	#0
	bcc	noerror
	movl	sp@,a0
	movl	a0,sp@-
	jmp	cerror
noerror:
	movl	sp@,a0
	jra	a0@
#endif
