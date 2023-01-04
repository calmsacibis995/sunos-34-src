	.data
	.asciz	"@(#)abort.s 1.1 86/09/24 SMI"; /* from UCB 4.1 83/06/27 */
	.text

/* C library -- abort */

#include "DEFS.h"

ENTRY(abort)
	halt
	clrl	r0
	ret
