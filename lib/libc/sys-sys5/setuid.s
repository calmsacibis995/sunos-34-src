/*	@(#)setuid.s 1.1 86/09/24 SMI	*/

#include "SYS.h"

#define SYS_setuid	23
SYSCALL(setuid)
	RET
