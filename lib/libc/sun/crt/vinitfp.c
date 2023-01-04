#ifdef sccsid
static	char sccsid[] = "@(#)vinitfp.c 1.1 86/09/24 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "fpcrttypes.h"

vinitfp_()

{
if (winitfp_() == 1)
	{
	return(1) ;
	}
if (minitfp_() == 1)
	{
	return(1) ;
	}
if (sinitfp_() == 1)
	{
	return(1) ;
	}
if (finitfp_() == 1)
	{
	return(1) ;
	}
return(0) ;
}
