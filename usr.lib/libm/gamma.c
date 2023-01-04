#ifdef sccsid
static  char sccsid[] = "@(#)gamma.c 1.1 86/09/25 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
	C program for floating point log gamma function

	gamma(x) computes the log of the absolute
	value of the gamma function.
	The sign of the gamma function is returned in the
	external quantity signgam.

	The coefficients for expansion around zero
	are #5243 from Hart & Cheney; for expansion
	around infinity they are #5404.

	Calls log and sin.
*/

#include <errno.h>
#include <math.h>

static double goobie	= 0.9189385332046727417803297;
static double pi	= 3.1415926535897932384626434;

#define M 6
#define N 8
static double p1[] = {
	0.83333333333333101837e-1,
	-.277777777735865004e-2,
	0.793650576493454e-3,
	-.5951896861197e-3,
	0.83645878922e-3,
	-.1633436431e-2,
};
static double p2[] = {
	-.42353689509744089647e5,
	-.20886861789269887364e5,
	-.87627102978521489560e4,
	-.20085274013072791214e4,
	-.43933044406002567613e3,
	-.50108693752970953015e2,
	-.67449507245925289918e1,
	0.0,
};
static double q2[] = {
	-.42353689509744090010e5,
	-.29803853309256649932e4,
	0.99403074150827709015e4,
	-.15286072737795220248e4,
	-.49902852662143904834e3,
	0.18949823415702801641e3,
	-.23081551524580124562e2,
	0.10000000000000000000e1,
};

double
gamma(arg)
double arg;
{
	double log(), pos(), neg(), asym();

	signgam = 1.;
	if(arg <= 0.) return(neg(arg));
	if(arg > 8.) return(asym(arg));
	return(log(pos(arg)));
}

static double
asym(x)
double x;
{
	double log();
	double n, argsq, result;
	int i;
        struct exception exc;

	argsq = 1./(x*x);
	for(n=0,i=M-1; i>=0; i--){
		n = n*argsq + p1[i];
	}
	result = ((x-.5)*log(x) - x + goobie + n/x);
	if (result == HUGE)
		{
                        exc.name = "gamma";
                        exc.arg1 = x;
                        exc.retval = HUGE;
                        exc.type = OVERFLOW;
        if (!matherr(&exc))
                errno = ERANGE;
        return (exc.retval);
		}
	return(result) ;
}

static double
neg(x)
double x;
{
	double temp, result;
	double log(), sin(), pos();
        struct exception exc;

                if (!modf(x = -x, &temp)) { /* SING if x is negative integer */
		        exc.name = "gamma";
		        exc.arg1 = -x;
		        exc.retval = HUGE;
       	             	exc.type = SING;
                        if (!matherr(&exc)) {
                                (void) write(2, "gamma: SING error\n", 18);
                                errno = EDOM;
                        }
                        return (exc.retval);
                }
	temp = sin(pi*x);
	if(temp < 0.) temp = -temp;
	else signgam = -1;
	result = (-log(x*pos(x)*temp/pi));
	if (abs(result) == HUGE)
		{
                        exc.name = "gamma";
                        exc.arg1 = x;
                        exc.retval = HUGE;
                        exc.type = OVERFLOW;
        if (!matherr(&exc))
                errno = ERANGE;
        return (exc.retval);
		}
	return(result) ;
}

static double
pos(arg)
double arg;
{
	double n, d, s;
	register i;

	if(arg < 2.) return(pos(arg+1.)/arg);
	if(arg > 3.) return((arg-1.)*pos(arg-1.));

	s = arg - 2.;
	for(n=0,d=0,i=N-1; i>=0; i--){
		n = n*s + p2[i];
		d = d*s + q2[i];
	}
	return(n/d);
}
