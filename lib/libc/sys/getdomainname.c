/* @(#)getdomainname.c 1.1 86/09/24 SMI */

#include "SYS.h"

SYSCALL(getdomainname)
	RET		/* len = getdomainname(buf, buflen) */
