#ifndef lint
static  char sccsid[] = "@(#)CStanhd.cs 1.1 86/09/25 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
	tanh(arg) computes the hyperbolic tangent of its floating
	point argument.

*/

#include "libmdefs.h"

double CSexp1d () ;

double
CStanhd(arg)
double arg;
{
	double t ;

	switch (dclass(arg))
	{
	case zero:
	case subnormal:
	case qnan : return(arg) ;
	case inf : if (dminus(arg) == 0) return(1.0) ; else return(-1.0) ;
	case snan : return(dquietnan(arg)) ;
	}

	t = CSexp1d(-2.0*fabs(arg)) ;
	t = -t/(2.0+t) ;
	if (dminus(arg) == 0) return(t) ; else return(-t) ;
}
