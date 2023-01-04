#ifdef sccsid
static  char sccsid[] = "@(#)minitfp.c 1.1 86/09/24 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <signal.h>
#include "fpcrttypes.h"

#ifdef EMULATOR
extern Emulator() ;
#endif

int EMTSignalled ;

int EMTSpecial (sig, code, scp)
int sig, code ;
struct sigcontext *scp ;

/*	Special EMT handler that sets EMTSignalled if invoked and 
	increments pc by 6 = length of instruction
		fmoveml	<zeros>,fpcr/fpsr
 */

{
EMTSignalled = 1 ;
scp->sc_pc += 6 ;
}

int minitfp_()

/*
 *	Procedure to determine if a physical 68881 is present and
 *	set 68881 status accordingly.  Also returns 0 if absent, 1 if present.
 */

{
struct sigvec new,old ;

if (fp_state_mc68881 != fp_unknown) 
	{
	if (fp_state_mc68881 == fp_enabled) fp_switch = fp_mc68881 ;
	return((fp_state_mc68881==fp_enabled) ? 1 : 0) ;
	}
new.sv_handler = EMTSpecial ;
new.sv_mask = 0 ;
new.sv_onstack = 0 ;
sigvec( SIGEMT, &new, &old ) ;
EMTSignalled = 0 ;
Mdefault() ;
sigvec( SIGEMT, &old, &new ) ;
#ifdef EMULATOR
new.sv_handler = Emulator ;
sigvec( SIGEMT, &new, &old ) ;
Mdefault() ;
fp_state_mc68881 = fp_enabled ;
#else
if (EMTSignalled) fp_state_mc68881 = fp_absent ; else fp_state_mc68881 = fp_enabled ;
#endif
if (fp_state_mc68881 == fp_enabled) fp_switch = fp_mc68881 ;
return((fp_state_mc68881==fp_enabled) ? 1 : 0) ;
}
