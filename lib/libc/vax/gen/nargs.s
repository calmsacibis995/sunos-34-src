	.data
	.asciz	"@(#)nargs.s 1.1 86/09/24 SMI"; /* from UCB 4.1 83/06/27 */
	.text

/* C library -- nargs */

#include "DEFS.h"

ENTRY(nargs)
	movzbl	*8(fp),r0	/* 8(fp) is old ap */
	ret
