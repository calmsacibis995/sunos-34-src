#ifdef sccsid
static	char sccsid[] = "@(#)CSexpd.cs 1.1 86/09/25 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "libmdefs.h"

/*
	exp returns the exponential function of its
	floating-point argument.
*/

static double 
	s1 = -0.15131331002160550e+05, 
	s2 = 0.25218885003600440e+04 ,
	s3 = -0.42026975276143628e+03,
	s4 = 0.28013483791305344e+02 ,
	t1 = -0.30262662004321101e+05,
	t2 = 0.15131331002160550e+05,
	t3 = -0.33624280058829163e+04,
	t4 = 0.42026975276143628e+03,
	t5 = -0.30013483791305344e+02,
 	loge2a = 0.6931471804855391383e0,
 	loge2b = 7.440617109802979273e-11,
	twom27 =  7.450580596923828125e-9,
	twom54 = 5.551115123125782702e-17;
	
static 	void expapprox ( x, pn, py1, pyc)
	double x, *py1, *pyc ;
	int *pn ;
/*
c	expapprox
c	computes n = nint(x * log2e)
c	computes y1 + yc =
c		(2**-n)*exp(x) - 1 = exp( x - n * loge(2) ) - 1
*/
{
	double y1, yc ;
	int n ;
	double y, dy, dn, rint() ;

	dn = rint( x * 1.44269504088896340735e0) ;
	n = dn;
	y1 = (x - dn * loge2a) ;
	dy = dn * loge2b;
	y = y1 - dy;
	yc = y * y * (s1+y*(s2+y*(s3+y*(s4-y))))/ (t1+y*(t2+y*(t3+y*(t4+y*(t5+y))))) -dy;
	*pn = n ;
	*py1 = y1 ;
	*pyc = yc ;
}


double
CSexpd(x)
double x;
{

	double y, yc ;
	int n ;

	switch (dclass(x))
	{
	case zero: 
	case subnormal: return(1.0) ;
	case normal: 
		if (fabs(x) < 747.0) break ;
	case inf:
		if (dminus(x) == 0) return(dinf()) ; else return(0.0) ;
	case qnan: return(x) ;
	case snan: return(dquietnan(x)) ;
	}


	if (fabs(x) <= twom27) return(1.0 + x) ;
	expapprox( x, &n, &y, &yc) ;
	y += yc ; y += 1.0 ;
	if (n == 0) return(y) ;
	return ( scalb(y,n)) ;

}

double
CSexp1d(x)
double x;
{
	double y, p, yc ;
	int n ;

	switch (dclass(x))
	{
	case zero: 
	case subnormal: return(x) ;
	case normal: 
		if (fabs(x) < 747.0) break ;
	case inf:
		if (dminus(x) == 0) return(dinf()) ; else return(-1.0) ;
	case qnan: return(x) ;
	case snan: return(dquietnan(x)) ;
	}


	if (abs(x) <= twom54) return(x) ;
	expapprox( x, &n, &y, &yc) ;
	switch (n) 
		{
		case -1: 
			y += yc ;
			return(0.5 * y - 0.5) ;
		case  0:
			return(y + yc) ;
		case  1:
			y += 0.5 ;
			return( 2.0 * (y + yc)) ;
		}
	y = y + yc ;
	if (n < 0 | n > 64) 
		{
		y += 1.0 ;
		return( scalb(y,n) - 1.0) ;
		}
	p = 1.0 ;
	p = scalb(p,n) ;
	if (n <= 53)  return((p - 1.0) + p * y) ;
	return(p + (p * y - 1.0)) ;
}
