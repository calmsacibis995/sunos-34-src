#ifndef lint
static  char sccsid[] = "@(#)CWpows.cw 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "libmdefs.h"

double
CWpows(arg1,arg2)
double arg1, arg2;
{
	double W_pow_fast() ;
	double W_pow_core() ;
	int l ;

	if(dminus(arg1)) 
		{
		if (dclass(arg2) == normal) l = d_integral(arg2);
			else l = 2 ;
		switch (l)
			{
			case 1:return(-W_pow_fast(-arg1,arg2)) ;
			case 2:return( W_pow_fast(-arg1,arg2)) ;
			}
		}
	if (dclass(arg1) == zero)
		return( W_pow_core( arg1,arg2)) ; /* So -0.0**0.5 = -0.0. */
	else
		return( W_pow_fast( arg1,arg2)) ;
}
