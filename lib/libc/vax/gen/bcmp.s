	.data
	.asciz	"@(#)bcmp.s 1.1 86/09/24 SMI"; /* from UCB 4.2 83/01/14 */
	.text

/* bcmp(s1, s2, n) */

#include "DEFS.h"

ENTRY(bcmp)
	movl	4(ap),r1
	movl	8(ap),r3
	movl	12(ap),r4
1:
	movzwl	$65535,r0
	cmpl	r4,r0
	jleq	2f
	subl2	r0,r4
	cmpc3	r0,(r1),(r3)
	jeql	1b
	addl2	r4,r0
	ret
2:
	cmpc3	r4,(r1),(r3)
	ret
