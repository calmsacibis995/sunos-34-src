#ifndef lint
static	char sccsid[] = "@(#)tanh.c 1.1 86/09/25 SMI"; /* from UCB 4.1 12/25/82 */
#endif

/*
	tanh(arg) computes the hyperbolic tangent of its floating
	point argument.

	sinh and cosh are called except for large arguments, which
	would cause overflow improperly.
*/

#include "mathdef.h"

double
tanh(arg)
double arg;
{
	double t ;

	switch (dclass(arg))
	{
	case zero:
	case subnormal:
	case qnan : return(arg) ;
	case normal: if (fabs(arg) < 21.0) break ;
	case inf : if (dminus(arg) == 0) return(1.0) ; else return(-1.0) ;
	case snan : return(dquietnan(arg)) ;
	}

	if (fabs(arg) <= 0.5) return(sinh(arg)/cosh(arg));
	t = exp(2.0*fabs(arg)) ;
	t = (t-1.0)/(t+1.0) ;
	if (dminus(arg) == 0) return(t) ; else return(-t) ;
}
