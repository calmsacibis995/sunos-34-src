	.data
	.asciz	"@(#)fabs.s 1.1 86/09/24 SMI"; /* from UCB 4.1 83/06/27 */
	.text

/* fabs - floating absolute value */

#include "DEFS.h"

ENTRY(fabs)
	movd	4(ap),r0
	bgeq	1f
	mnegd	r0,r0
1:
	ret
