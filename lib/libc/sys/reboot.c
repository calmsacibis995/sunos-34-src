/* @(#)reboot.c 1.1 86/09/24 SMI; from UCB 4.1 82/12/04 */

#include "SYS.h"

SYSCALL(reboot)
#if vax
	halt
#endif
#if sun
	stop	#0
#endif
