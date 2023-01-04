#ifdef sccsid
static	char sccsid[] = "@(#)finitfp.c 1.1 86/09/24 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "fpcrtdefs.h"
#include "fpcrttypes.h"

finitfp_()
{
fp_switch = fp_software ;
fp_state_software = fp_enabled ;
Fmode = ROUNDTODOUBLE ; Fstatus = 0 ;
return(1) ;
}
