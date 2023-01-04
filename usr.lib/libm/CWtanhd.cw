#ifndef lint
static	char sccsid[] = "@(#)CWtanhd.cw 1.1 86/09/25 SMI"; /* from UCB 4.1 12/25/82 */
#endif

/*
	tanh(arg) computes the hyperbolic tangent of its floating
	point argument.

	sinh and cosh are called except for large arguments, which
	would cause overflow improperly.
*/

#include "libmdefs.h"

double CWexpd() ;

double
CWtanhd(arg)
double arg;
{
	double p0 = -0.16134119023996228053e4 ;
	double p1 = -0.99225929672236083313e2 ;
	double p2 = -0.96437492777225469787e0 ;
	double q0 =  0.48402357071988688686e4 ;
	double q1 =  0.22337720718962312926e4 ;
	double q2 =  0.11274474380534949335e3 ;
	
	double t ;
	double x2 ;

	switch (dclass(arg))
	{
	case zero:
	case subnormal:
	case qnan : return(arg) ;
	case normal: if (fabs(arg) < 21.0) break ;
	case inf : if (dminus(arg) == 0) return(1.0) ; else return(-1.0) ;
	case snan : return(dquietnan(arg)) ;
	}

	if (fabs(arg) <= 0.5) 
		{
		x2 = arg * arg ;
		return(arg + arg * x2 * ((p2*x2+p1)*x2+p0)/(((x2+q2)*x2+q1)*x2+q0) ) ;
		}
	t = CWexpd(2.0*fabs(arg)) ;
	t = (t-1.0)/(t+1.0) ;
	if (dminus(arg) == 0) return(t) ; else return(-t) ;
}
