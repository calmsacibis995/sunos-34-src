/* @(#)setdomainname.c 1.1 86/09/24 SMI */

#include "SYS.h"

SYSCALL(setdomainname)
	RET		/* setdomainname(name, len) */
