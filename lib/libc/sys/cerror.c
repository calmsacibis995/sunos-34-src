/* @(#)cerror.c 1.1 86/09/24 SMI; from UCB 4.1 82/12/04 */

#include "SYS.h"

	.globl	_errno
	.data
_errno:	.long	0
	.text
cerror:
#if vax
	movl	r0,_errno
	mnegl	$1,r0
#endif
#if sun
	movl	d0,_errno
	moveq	#-1,d0
#endif
	RET
