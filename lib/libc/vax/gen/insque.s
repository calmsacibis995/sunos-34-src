	.data
	.asciz	"@(#)insque.s 1.1 86/09/24 SMI"; /* from UCB 4.1 82/12/15 */
	.text

/* insque(new, pred) */

#include "DEFS.h"

ENTRY(insque)
	insque	*4(ap), *8(ap)
	ret
