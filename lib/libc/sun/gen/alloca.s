	.data
/*	.asciz	"@(#)alloca.s 1.1 86/09/24 SMI"	*/
	.text

| like alloc, but automatic 
| automatic free in return 

#include "DEFS.h"

ENTRY(alloca)
#ifdef PROF
	unlk	a6
#endif
	movl    sp@,a0		| save return addr
	movl    sp@(4),d0	| align on longword boundary
	addqw   #3,d0
	andl	#0xfffffffc,d0
	subl    d0,a7		| adjust stack 
	movl    sp,d0
	addqw   #8,d0		| remember ret addr and argument 
	jmp     a0@
