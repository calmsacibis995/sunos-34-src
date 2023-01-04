/* @(#)vhangup.c 1.1 86/09/24 SMI; from UCB 4.3 82/12/29 */

#include "SYS.h"

#define SYS_vhangup 76

SYSCALL(vhangup)
	RET
