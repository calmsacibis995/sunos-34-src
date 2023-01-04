/* @(#)wait3.c 1.1 86/09/24 SMI; from UCB 4.2 12/28/82 */

/*
 * C library -- wait3
 *
 * pid = wait3(&status, flags, &rusage);
 *
 * pid == -1 if error
 * status indicates fate of process, if given
 * flags may indicate process is not to hang or
 * that untraced stopped children are to be reported.
 * rusage optionally RET detailed resource usage information
 */
#include "SYS.h"

#define	SYS_wait3	SYS_wait

ENTRY(wait3)
#if vax
	movl	8(ap),r0	/* make it easy for system to get */
	movl	12(ap),r1	/* these extra arguments */
	bispsw	$0xf		/* flags wait3() */
	chmk	$SYS_wait3
	bcc 	noerror
	jmp	cerror
noerror:
	tstl	4(ap)		/* status desired? */
	beql	nostatus	/* no */
	movl	r1,*4(ap)	/* store child's status */
#endif
#if sun
	movl	sp@(8),d0	| make it easy for system to get
	movl	sp@(12),d1	| these extra arguments
	movl	#SYS_wait3,sp@-
	orb	#0x1f,cc
	trap	#0
	bcc 	noerror
	jmp 	cerror
noerror:
	tstl	sp@(4)		| status desired?
	beq	nostatus	| no
	movl	sp@(4),a0
	movl	d1,a0@		| store child's status
#endif
nostatus:
	RET
