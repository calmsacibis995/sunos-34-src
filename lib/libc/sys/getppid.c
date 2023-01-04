/* @(#)getppid.c 1.1 86/09/24 SMI; from UCB 4.1 82/12/04 */

#include "SYS.h"

PSEUDO(getppid,getpid)
#if vax
	movl	r1,r0
#endif
#if sun
	movl	d1,d0
#endif
	RET		/* ppid = getppid(); */
