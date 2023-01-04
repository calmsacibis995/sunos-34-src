	.data
	.asciz	"@(#)htonl.c 1.1 86/09/24 SMI"; /* from UCB 4.1 82/12/15 */
	.text

/* netorder = htonl(hostorder) */

#include "DEFS.h"

ENTRY(htonl)
	rotl	$-8,4(ap),r0
	insv	r0,$16,$8,r0
	movb	7(ap),r0
	ret
