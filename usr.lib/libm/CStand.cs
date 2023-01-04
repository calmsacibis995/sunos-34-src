#ifndef lint
static	char sccsid[] = "@(#)CStand.cs 1.1 86/09/25 SMI"; /* from UCB 4.1 12/25/82 */
#endif

/*
	floating point tangent

	A series is used after range reduction.
*/

#include "libmdefs.h"
 
static double p1	 = -0.13338350006421960681e+0;
static double p2	 =  0.34248878235890589960e-2;
static double p3	 = -0.17861707342254426711e-4;
static double q1	 = -0.46671683339755294240e+0;
static double q2	 =  0.25663832289440112864e-1;
static double q3	 = -0.31181531907010027307e-3;
static double q4	 =  0.49819433993786512270e-6;

static double
tanx(x)
double x ;
{
	double x2, p, q ;
	x2 = x * x ;
	p = (p3 * x2 + p2) * x2 + p1 ;
	q = ((q4 * x2 + q3) * x2 + q2) * x2 + q1 ;
	return( x + x * x2 * (p-q)/(1.0+x2*q)) ;
}

double
CStand(arg)
double arg;
{
	double dtrigarg() ;
	double tanx() ;
	double x ; int k;

	switch(dclass(arg))
	{
	case zero:
	case subnormal : 
	case qnan : return(arg) ;
	case inf : return(dnan(nantrig)) ;
	case snan : return(dquietnan(arg)) ;
	}

	x = dtrigarg( arg, &k ) ;
	if ((k & 1) == 0 ) return(tanx(x)) ; else return(-1.0/tanx(x)) ;
}
