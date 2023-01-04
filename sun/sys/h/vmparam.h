/*	@(#)vmparam.h 1.1 86/09/25 SMI; from UCB 4.13 82/12/17	*/

/*
 * Machine dependent constants
 */
#ifdef KERNEL
#include "../machine/vmparam.h"
#else
#include <machine/vmparam.h>
#endif

#if defined(KERNEL) && !defined(LOCORE)
int	klseql;
int	klsdist;
int	klin;
int	kltxt;
int	klout;
#endif
