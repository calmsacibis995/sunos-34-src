	.data
	.asciz	"@(#)abs.s 1.1 86/09/24 SMI"; /* from UCB 4.1 83/06/27 */
	.text

/* abs - int absolute value */

#include "DEFS.h"

ENTRY(abs)
	movl	4(ap),r0
	bgeq	1f
	mnegl	r0,r0
1:
	ret
