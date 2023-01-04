/*	@(#)SYS.h	1.1 9/24/86 SMI 86/01/17; from UCB 4.1 06/09/83 */

#include <syscall.h>

#ifdef PLAIN_CALL
#define build_name(x,y) x/**/y/**/_plain
#else
#define build_name(x,y) x/**/y
#endif

#if vax
#ifdef PROF
#define	ENTRY(x)	.globl build_name(_,x); .align 2; build_name(_,x): .word 0; \
			.data; 1:; .long 0; .text; moval 1b,r0; jsb mcount
#else
#define	ENTRY(x)	.globl build_name(_,x); .align 2; build_name(_,x): .word 0
#endif PROF
#define	SYSCALL(x)	err: jmp cerror; ENTRY(x); chmk $SYS_/**/x; jcs err
#define	BSDSYSCALL(x)	err: jmp cerror; ENTRY(build_name(_,x)); chmk $SYS_/**/x; jcs err
#define	PSEUDO(x,y)	ENTRY(x); chmk $SYS_/**/y
#define	CALL(x,y)	calls $x, build_name(_,y)
#define	RET		ret

	.globl	cerror
#endif

#if sun
	.globl	cerror
#ifdef PROF
	.globl  mcount
#define	ENTRY(x)	.globl build_name(_,x);\
		 build_name(_,x): link a6,#0;\
			 lea build_name(x,1),a0;\
		 .data; build_name(x,1): .long 0; .text;\
			 jsr mcount;\
			 unlk a6
#else
#define	ENTRY(x)	.globl build_name(_,x); build_name(_,x):
#endif

#define PARAM		sp@(4)
#define PARAM2		sp@(8)
#define	PSEUDO(x,y)	ENTRY(x); pea SYS_/**/y; trap #0
#define	SYSCALL(x)	err: jmp cerror; ENTRY(x); pea SYS_/**/x; trap #0; jcs err
#define	BSDSYSCALL(x)	err: jmp cerror; ENTRY(build_name(_,x)); pea SYS_/**/x; trap #0; jcs err
#define RET		rts
#endif
