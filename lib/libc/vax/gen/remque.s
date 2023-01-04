	.data
	.asciz	"@(#)remque.s 1.1 86/09/24 SMI"; /* from UCB 4.1 82/12/15 */
	.text

/* remque(entry) */

#include "DEFS.h"

ENTRY(remque)
	remque	*4(ap),r0
	ret
