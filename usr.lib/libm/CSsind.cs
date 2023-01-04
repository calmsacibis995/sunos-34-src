#ifndef lint
static	char sccsid[] = "@(#)CSsind.cs 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
	C program for floating point sin/cos.
*/

#include "libmdefs.h"

double
CScosd(arg)
double arg;
{
	double sinus();
	return(sinus(arg, 1));
}

double
CSsind(arg)
double arg;
{
	double sinus();
	return(sinus(arg, 0));
}

static double sinx( x )
double x ;
{
static double
S0     = -1.6666666666666463126E-1    , /*Hex  2^ -3   * -1.555555555550C */
S1     =  8.3333333332992771264E-3    , /*Hex  2^ -7   *  1.111111110C461 */
S2     = -1.9841269816180999116E-4    , /*Hex  2^-13   * -1.A01A019746345 */
S3     =  2.7557309793219876880E-6    , /*Hex  2^-19   *  1.71DE3209CDCD9 */
S4     = -2.5050225177523807003E-8    , /*Hex  2^-26   * -1.AE5C0E319A4EF */
S5     =  1.5868926979889205164E-10   ; /*Hex  2^-33   *  1.5CF61DF672B13 */
	double z ;
	z = x*x;
	z = x * z*(S0+z*(S1+z*(S2+z*(S3+z*(S4+z*S5))))) ;
	return(x + z) ;
}

static double cosx( x )
double x ;
{
static double
c0     =  4.1666666666666504759E-2    , /*Hex  2^ -5   *  1.555555555553E */
c1     = -1.3888888888865301516E-3    , /*Hex  2^-10   * -1.6C16C16C14199 */
c2     =  2.4801587269650015769E-5    , /*Hex  2^-16   *  1.A01A01971CAEB */
c3     = -2.7557304623183959811E-7    , /*Hex  2^-22   * -1.27E4F1314AD1A */
c4     =  2.0873958177697780076E-9    , /*Hex  2^-29   *  1.1EE3B60DDDC8C */
c5     = -1.1250289076471311557E-11   ; /*Hex  2^-37   * -1.8BD5986B2A52E */

	double z, approx ;
	z = x*x;
	approx = z * z * (((((c5*z+c4)*z+c3)*z+c2)*z+c1)*z+c0) ;
	if (z < 0.5)
		{
		approx = 0.5 * z - approx ;
		return(1.0 - approx) ;
		}
	else
		{
		approx = 0.5 * (z - 1.0) - approx ;
		return(0.5 - approx) ;
		}
}

static double
sinus(arg, quad)
double arg;
int quad;
{
	double sinx(), cosx() ;
	double x;
	int k;

	switch (dclass(arg))
	{
	case zero:
	case subnormal:
		if (quad == 0) return(arg) ; else return(1.0) ;
	case inf: return(dnan(nantrig)) ;
	case qnan: return(arg) ;
	case snan: return(dquietnan(arg)) ;
	}

	x = dtrigarg( arg, &k ) ;
	switch ((quad+k) & 3)
	{
	case 0: return(sinx(x)) ;
	case 1: return(cosx(x)) ;
	case 2: return(-sinx(x)) ;
	case 3: return(-cosx(x)) ;
	}
}
