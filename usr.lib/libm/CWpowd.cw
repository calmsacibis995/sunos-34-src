#ifndef lint
static  char sccsid[] = "@(#)CWpowd.cw 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "libmdefs.h"

/*
	computes a^b.
	uses log and exp
*/

double W_pow_core( x, y ) /* computes exp(y * log(x)) */ 
double x, y ;
{
double CWexpd() ; double CWlogd () ; double CMsqrtd () ;
double result, sigx, powerofx, expfactor, fracny, scalefactor, pwr ;
double y0, yi, yf, yf1, ny, exparg ;
int iys, negpower, expx, inty ;
unsigned mask, uys ;

y0 = y ;
negpower = dminus(y) ;	/* This enforces 1/x = x**-1. */
y = fabs(y) ;
iys = y ; /* The integer part of separate power of y. */
yf = y - iys ; /* Fractional power of y. */
if ((fabs(yf) <= 1.0) & (dclass(y) <= normal)) 
	/* Second test required for Sky board which otherwise 
		goes into a very long loop. */
	{
	yi = 0.0 ; /* Integer part is all in ys. */
	if (fabs(yf) >= 0.5) if (dminus(yf))
		{
		if (yf <= -0.5) { yf = yf + 1.0 ; iys = iys - 1 ; }
		}
		else
		{
		if (yf > 0.5) { yf = yf - 1.0 ; iys = iys + 1 ; }
		}
	}
	else
	{ /* Separate part is bigger than 32 bit integer so don't do it. */
	iys = 0 ;
	yi = d_intfrac( y, &yf ) ; /* yi may be infinity or nan. */
	}
powerofx = 1.0 ;
if (iys > 0) while (1)
	{ /* Compute x** iys */
	uys = iys ;
	pwr = x ;
	mask = 1 ;
	while (mask <= uys)
		{
		if ((mask & uys) != 0) powerofx = powerofx * pwr ;
		pwr = pwr * pwr ;
		mask = mask << 1 ;
		}
	if ((dclass(powerofx) == normal) | (dminus(yf) == 0)) break ;
		/* Unnecessary over/underflow may have caused trouble
			that may be averted. */
	iys = iys - 1 ;
	yf = yf + 1.0 ;
	powerofx = 1.0 ;
	}
if (fabs(yf) > 0.25) 
	{
	if (dminus(yf))
		{ /* Divide by square root. */
		pwr = powerofx / CMsqrtd(x) ;
		yf1 = yf + 0.5 ;
		}
		else
		{ /* Multiply by square root. */
		pwr = powerofx * CMsqrtd(x) ;
		yf1 = yf - 0.5 ;
		}
	if ((dclass(pwr) == normal) | (dminus(yf1) == dminus(yf)))
		{ /* Accept square root. */
		yf = yf1 ;
		powerofx = pwr ;
		iys = 1 ; /* This fools the following test for the case sqrt(0). */}
	}
y = yi + yf ; /* iys is accounted for in powerofx. */
if (dclass(y) == zero) 
	{
	if (iys == 0) switch (dclass(x))
		{ /* This tests for 0.0**0.0, inf**0.0, nan**0.0 */
		case snan:
		case qnan: 
		case inf:
		case zero: return(CWexpd(y0*CWlogd(x))) ;
		}
	expfactor = 1.0 ; 
	}
	else
	{
switch (dclass(x))
	{
	case snan:
	case qnan:
	case inf:
	case zero:	return(CWexpd(y0*CWlogd(x))) ;
	case subnormal:
	case normal:
	expx = d_powexp( x ) ; /* expx gets the exponent of x */
	sigx = ldexp( x, -expx ) ; /* sqrt(0.5) <= sigx <= sqrt(2.0) */
	}
ny = expx * y ;
switch (dclass(ny))
	{
	case snan:
	case qnan:
	case inf:	return(CWexpd(y0*CWlogd(x))) ;
	case zero: 	inty = 0 ; fracny = 0.0 ; break ;	
	case subnormal: inty = 0 ; fracny = ny ; break ;
	case normal: 	scalefactor = d_intfrac( ny, &fracny ) ; 
		if (fabs(fracny) > 0.5)
			if (dminus(fracny)) 
				{ 
				fracny = fracny + 1.0 ; 
				scalefactor = scalefactor - 1.0 ;
				}
				else
				{
				fracny = fracny - 1.0 ;
				scalefactor = scalefactor + 1.0 ;
				}
		if (dclass(scalefactor) == zero) inty = 0 ; else
		if (d_powexp(scalefactor) <= 30) inty = scalefactor ; else
		if (dminus(scalefactor)) inty = 0x80000001 ; else 
			inty = 0x7fffffff ;
	}
expfactor =  CWexpd( fracny * 0.69314718055994530941 + y * CWlogd(sigx) ) ;
expfactor = ldexp( expfactor, inty ) ;
}
result = powerofx * expfactor ;
 if (negpower) 
       {
       result = 1.0 / result ;
       if (result == 0.0)
               { /* recompute in case overflow caused trouble */
               result = W_pow_core( x, -0.5 * y0 ) ;
               result = 1.0 / result ;
               result = result * result ;
               }
       }
 return(result) ;
}

double
CWpowd(arg1,arg2)
double arg1, arg2;
{
	double W_pow_core() ;
	int l ;

	if(dminus(arg1) != 0) 
		{
		if (dclass(arg2) == normal) l = d_integral(arg2);
			else l = 2 ;
		switch (l)
			{
			case 1:return(-W_pow_core(-arg1,arg2)) ;
			case 2:return( W_pow_core(-arg1,arg2)) ;
			}
		}
	return( W_pow_core( arg1,arg2)) ;
}

double W_pow_fast( x, y ) /* computes exp(y * log(x)) for single precision */ 
double x, y ;
{
return( CWexpd( y * CWlogd(x) ) ) ;
}
