/* @(#)ptrace.c 1.1 86/09/24 SMI; from UCB 4.2 83/06/30 */

#include "SYS.h"

#define	SYS_ptrace	26

ENTRY(ptrace)
	clrl	_errno
#if vax
	chmk	$SYS_ptrace
#endif
#if sun
	pea	SYS_ptrace
	trap	#0
#endif
	jcs	err
	RET
err:
	jmp	cerror
