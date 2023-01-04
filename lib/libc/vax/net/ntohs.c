	.data
	.asciz	"@(#)ntohs.c 1.1 86/09/24 SMI"; /* from UCB 4.1 82/12/15 */
	.text

/* hostorder = ntohs(netorder) */

#include "DEFS.h"

ENTRY(ntohs)
	rotl	$8,4(ap),r0
	movb	5(ap),r0
	movzwl	r0,r0
	ret
