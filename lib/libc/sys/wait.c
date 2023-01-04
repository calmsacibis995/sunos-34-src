/* @(#)wait.c 1.1 86/09/24 SMI; from UCB 4.1 82/12/28 */

#include "SYS.h"

SYSCALL(wait)
#if vax
	tstl	4(ap)
	jeql	1f
	movl	r1,*4(ap)
1:
#endif
#if sun
	tstl	PARAM
	beqs	2$
	movl	PARAM,a0
	movl	d1,a0@
2$:
#endif
	RET
