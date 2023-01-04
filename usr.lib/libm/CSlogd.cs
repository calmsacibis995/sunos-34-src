#ifdef sccsid
static	char sccsid[] = "@(#)CSlogd.cs 1.1 86/09/25 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
	log returns the natural logarithm of its floating
	point argument.
*/

# include "libmdefs.h"

static double loge2a = 0.6931471804855391383 ;
static double loge2b = 7.440617109802979273e-11 ;

static double s1 =     /* 3fe55555    55555592 (*/  0.66666666666667340e+00 ;
static double s2 =     /* 3fd99999    9997ff24 (*/  0.39999999999416702e+00 ;
static double s3 =     /* 3fd24924    941e07b4 (*/  0.28571428742008753e+00 ;
static double s4 =     /* 3fcc71c5    2150bea6 (*/  0.22222198607186278e+00 ;
static double s5 =     /* 3fc74663    cc94342f (*/  0.18183562745289936e+00 ;
static double s6 =     /* 3fc39a1e    c014045b (*/  0.15314087275331442e+00 ;
static double s7 =     /* 3fc2f039    f0085122 (*/  0.14795612545334175e+00 ;

static double logapprox(x)
double x ;
{
        double z, z2 ;

	z = x/(2.0e0+x) ;
        z2 = z * z ;
        return(z * ( z2*(s1+z2*(s2+z2*(s3+z2*(s4+z2*(s5+z2*(s6+z2*s7)))))) - x )) ;
}

double CSlogd(arg)
double arg;
{
	double x, z ;
	int n;

	switch (dclass(arg))
	{
	case zero: return(-dinf()) ;
	case subnormal:
	case normal:
		if (dminus(arg) != 0) return(dnan(nanlog)) ; break ;
	case inf:
		if (dminus(arg) != 0) return(dnan(nanlog)) ;
			else return(arg) ;
	case qnan: return(arg) ;
	case snan: return(dquietnan(arg)) ;
	}

	n = -d_powexp( arg ) ;
	x = FFscaled_( &arg, &n ) - 1.0 ;
	n = -n ;

        z = n * loge2b + logapprox(x) ;
	z += x ;
	return(n * loge2a + z) ;
}

double CSlog1d(x)
double x;
{
        double twom54     = 5.551115123125782702e-17;
        double sqrthalfm1 = -2.928932188134524828e-1;
        double sqrt2m1    =  4.142135623730950345e-1;
        double sqrteighthm1 = -6.464466094067262691e-1;
        double sqrt8m1      = 1.828427124746190069e0;
        double loge2amhalf = 1.931471804855391383e-1;
        double loge2afrom1 = 3.068528195144608617e-1;
        double loge2b = 7.440617109802979273e-11;

	double CSlogd(), logapprox() ;
	double x2, z ;

	switch (dclass(x))
	{
	case zero:
	case subnormal:
	case qnan: return(x) ;
	case normal:
		if (x > -1.0) break ;
		if (x == -1.0) 
			return(-dinf()) ;
		return(dnan(nanlog)) ;
	case inf:
		if (dminus(x) != 0) return(dnan(nanlog)) ;
			else return(x) ;
	case snan: return(dquietnan(x)) ;
	}

        if (x > twom54) 
                {
		if (x < sqrt2m1  )   return(x + logapprox(x)) ;
                if (x < sqrt8m1  )  
			{
			x2 = 0.5 * x ;
			z = logapprox ( x2 - 0.5) + loge2b ;
			z += loge2amhalf ;
			return( x2 + z ) ;
       			}
        	}
	else
                {
		if (x >= -twom54)    return(x) ;
                if (x > sqrthalfm1)  return(x + logapprox(x)) ;
                if (x > sqrteighthm1)
			{
			x2 = x + x ;
			z = logapprox ( x2 + 1.0) - loge2b ;
			z += loge2afrom1 ;
			return(x2 + z) ;
       			}
        	}
	return(CSlogd( x + 1.0)) ;
}
