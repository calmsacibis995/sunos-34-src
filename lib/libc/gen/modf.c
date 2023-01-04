#ifndef lint
static	char sccsid[] = "@(#)modf.c 1.1 86/09/24 SMI"; /* from S5R2 1.5 */
#endif

/*LINTLIBRARY*/
/*
 * modf(value, iptr) returns the signed fractional part of value
 * and stores the integer part indirectly through iptr.
 *
 */

#include <nan.h>
#include <values.h>

double
modf(value, iptr)
double value; /* don't declare register, because of KILLNaN! */
register double *iptr;
{
	register double absvalue;

	KILLNaN(value); /* raise exception on Not-a-Number (3b only) */
	if ((absvalue = (value >= 0.0) ? value : -value) >= MAXPOWTWO)
		*iptr = value; /* it must be an integer */
	else {
		*iptr = absvalue + MAXPOWTWO; /* shift fraction off right */
		*iptr -= MAXPOWTWO; /* shift back without fraction */
		while (*iptr > absvalue) /* above arithmetic might round */
			*iptr -= 1.0; /* test again just to be sure */
		if (value < 0.0)
			*iptr = -*iptr;
	}
	return (value - *iptr); /* signed fractional part */
}
