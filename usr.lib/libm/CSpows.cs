#ifndef lint
static  char sccsid[] = "@(#)CSpows.cs 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "libmdefs.h"

double
CSpows(arg1,arg2)
double arg1, arg2;
{
	double S_pow_fast() ;
	double S_pow_core() ;
	int l ;

	if(dminus(arg1)) 
		{
		if (dclass(arg2) == normal) l = d_integral(arg2);
			else l = 2 ;
		switch (l)
			{
			case 1:return(-S_pow_fast(-arg1,arg2)) ;
			case 2:return( S_pow_fast(-arg1,arg2)) ;
			}
		}
	if (dclass(arg1) == zero)
		return( S_pow_core( arg1,arg2)) ; /* So -0.0**0.5 = -0.0. */
	else
		return( S_pow_fast( arg1,arg2)) ;
}
