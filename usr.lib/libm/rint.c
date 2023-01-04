#ifdef sccsid
static  char sccsid[] = "@(#)rint.c 1.1 86/09/25 SMI"; /* 4.2	9/11/85 */
#endif

/*
 * algorithm for rint(x) in pseudo-pascal form ...
 *
 * real rint(x): real x;
 *	... delivers integer nearest x in direction of prevailing rounding
 *	... mode
 * const	L = (last consecutive integer)/2
 * 	  = 2**55; for VAX D
 * 	  = 2**52; for IEEE 754 Double
 * real	s,t;
 * begin
 * 	if x != x then return x;		... NaN
 * 	if |x| >= L then return x;		... already an integer
 * 	s := copysign(L,x);
 * 	t := x + s;				... = (x+s) rounded to integer
 * 	return t - s
 * end;
 *
 * Note: Inexact will be signaled if x is not an integer, as is
 *	customary for IEEE 754.  No other signal can be emitted.
 */
#ifdef VAX
static long Lx[] = {0x5c00,0x0};		/* 2**55 */
#define L *(double *) Lx
#else	/* IEEE double */
static double L = 4503599627370496.0E0;		/* 2**52 */
#endif
double
rint(x)
double x;
{
	double s,t,one = 1.0,copysign();
#ifndef VAX
	if (x != x)				/* NaN */
		return (x);
#endif
	if (copysign(x,one) >= L)		/* already an integer */
	    return (x);
	s = copysign(L,x);
	t = x + s;				/* x+s rounded to integer */
	return (t - s);
}
