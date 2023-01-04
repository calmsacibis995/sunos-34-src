#ifndef lint
static	char sccsid[] = "@(#)CWsinhd.cw 1.1 86/09/25 SMI"; /* from UCB 4.1 12/25/82 */
#endif

/*
	sinh(arg) returns the hyperbolic sine of its floating-
	point argument.

	The exponential function is called for arguments
	greater in magnitude than 0.5.

	A series is used for arguments smaller in magnitude than 0.5.
	The coefficients are #2029 from Hart & Cheney. (20.36D)

	cosh(arg) is computed from the exponential function for
	all arguments.
*/

#include "libmdefs.h"

double CWexpd() ;

static double p0  = -0.6307673640497716991184787251e+6;
static double p1  = -0.8991272022039509355398013511e+5;
static double p2  = -0.2894211355989563807284660366e+4;
static double p3  = -0.2630563213397497062819489e+2;
static double q0  = -0.6307673640497716991212077277e+6;
static double q1   = 0.1521517378790019070696485176e+5;
static double q2  = -0.173678953558233699533450911e+3;

double
CWsinhd(arg)
double arg;
{
	double t, argsq;
	register sign;

	switch (dclass(arg))
	{
	case zero:
	case subnormal:
	case inf :
	case qnan: return(arg) ;
	case snan : return(dquietnan(arg)) ;
	}

	if (fabs(arg) > 0.5) 
		{
		t = CWexpd(fabs(arg)) ;
		if (dminus(arg) == 0) return(0.5*(t-1.0/t)) ;
			else          return(0.5*(1.0/t-t)) ;
		}

	argsq = arg*arg;
	t = (((p3*argsq+p2)*argsq+p1)*argsq+p0)*arg;
	t /= (((argsq+q2)*argsq+q1)*argsq+q0);
	return(t);
}

double
CWcoshd(arg)
double arg;
{
	double t ;

	switch (dclass(arg))
	{
	case zero:
	case subnormal:
		return(1.0) ;
	case inf :
		return(fabs(arg)) ;
	case qnan : return(arg) ;
	case snan : return(dquietnan(arg)) ;
	}

	t = CWexpd(fabs(arg)) ;
	return(0.5*(t+1.0/t)) ;
}
