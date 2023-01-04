/*	@(#)_setpgrp.s 1.1 86/09/24 SMI; from UCB 4.1 82/12/04	*/

#include "SYS.h"

BSDSYSCALL(setpgrp)
	RET		/* _setpgrp(pid, pgrp); */
