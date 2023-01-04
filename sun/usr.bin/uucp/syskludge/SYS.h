/* SYS.h 1.1 86/09/25 */

#include <syscall.h>


	.globl	cerror
#ifdef PROF
	.globl  mcount
#define	ENTRY(x)	.globl _/**/x;\
		 _/**/x: link a6,#0;\
			 lea 1$,a0;\
		 .data; 1$: .long 0; .text;\
			 jsr mcount;\
			 unlk a6
#else not PROF
#define	ENTRY(x)	.globl _/**/x; _/**/x:
#endif not PROF

#define PARAM	sp@(4)
#define PARAM2	sp@(8)
#define RET	rts
#define	SYSCALL(x) \
err:	jmp	cerror; \
	ENTRY(x); \
	movl	PARAM,sp@-; \
	jsr	__fixf; \
	addl	#4,sp; \
	movl	d0,PARAM; \
	pea	SYS_/**/x; \
	trap	#0; \
	jcs	err
#define	SYSCALL2(x) \
err:	jmp	cerror; \
	ENTRY(x); \
	movl	PARAM,sp@-; \
	jsr	__fixf; \
	addl	#4,sp; \
	movl	d0,PARAM; \
	movl	PARAM2,sp@-; \
	jsr	__fixf2; \
	addl	#4,sp; \
	movl	d0,PARAM2; \
	pea	SYS_/**/x; \
	trap	#0; \
	jcs	err
#define	SYSCALLS(x) \
err:	jmp	cerror; \
	ENTRY(x); \
	movl	PARAM,sp@-; \
	jsr	__savfile; \
	addl	#4,sp; \
	movl	d0,PARAM; \
	pea	SYS_/**/x; \
	trap	#0; \
	jcs	err
#define	PSEUDO(x,y)	ENTRY(x); pea SYS_/**/y; trap #0
