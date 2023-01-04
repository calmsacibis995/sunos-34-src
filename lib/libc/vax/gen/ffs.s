	.data
	.asciz	"@(#)ffs.s 1.1 86/09/24 SMI"; /* from UCB 4.1 82/12/15 */
	.text

/* bit = ffs(value) */

#include "DEFS.h"

ENTRY(ffs)
	ffs	$0,$32,4(ap),r0
	bneq	1f
	mnegl	$1,r0
1:
	incl	r0
	ret
