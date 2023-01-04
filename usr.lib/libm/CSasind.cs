#ifdef sccsid
static  char sccsid[] = "@(#)CSasind.cs 1.1 86/09/25 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
	asin(arg) and acos(arg) return the arcsin, arccos,
	respectively of their arguments.

	Arctan is called after appropriate range reduction.
*/

#include "libmdefs.h"
static double pio2	= 1.570796326794896619;

double
CSasind(arg) double arg; {

	double temp, p, q, xsq, CSatand(), CFsqrtd() ;

	double p1 = -0.27368494524164255994e+2 ;
	double p2 =  0.57208227877891731407e+2 ;
	double p3 = -0.39688862997504877339e+2 ;
	double p4 =  0.10152522233806463645e+2 ;
	double p5 = -0.69674573447350646411e+0 ;
	double q0 = -0.16421096714498560795e+3 ;
	double q1 =  0.41714430248260412556e+3 ;
	double q2 = -0.38186303361750149284e+3 ;
	double q3 =  0.15095270841030604719e+3 ;
	double q4 = -0.23823859153670238830e+2 ;

	switch (dclass(arg))
	{
	case zero:
	case subnormal:
	case qnan: return(arg) ;
	case normal : if (fabs(arg) <= 1.0) break ;
	case inf:  return(dnan(naninvtrig)) ;
	case snan: return(dquietnan(arg)) ;
	}

	if(fabs(arg)> 0.5)
		{
		temp = CSatand(CFsqrtd((1.0 - arg)*(1.0+arg))/arg);
		if (dminus(arg)) return( -pio2 - temp ) ; 
			else return( pio2 - temp ) ;
		}
	else
		{
		xsq = arg*arg ;
		p = (((p5 * xsq + p4) * xsq + p3) * xsq + p2) * xsq + p1 ;
		q = ((((xsq + q4) * xsq + q3) * xsq + q2) * xsq + q1) * xsq + q0 ;
		return( arg + arg * xsq *(p/q)) ;
		}

}

double
CSacosd(arg) double arg; {

	double CSasind(), CFsqrtd () ;

	if (arg >= 0.7) return(2.0*CSasind(CFsqrtd(0.5-0.5*arg)))  ; else
	return(pio2 - CSasind(arg));
}
