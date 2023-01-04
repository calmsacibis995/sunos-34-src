/* @(#)_exit.c 1.1 86/09/24 SMI; from UCB 4.1 82/12/04 */

#include "SYS.h"

#if vax
	.align	1
#endif
PSEUDO(_exit,exit)
			/* _exit(status) */
