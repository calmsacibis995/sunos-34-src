/*
static	char sccsid[] = "@(#)pow.c 1.1 86/09/25 Copyr 1986 Sun Micro";
*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <errno.h>
#include <math.h>
#include "fpcrttypes.h"
#include "libmdefs.h"

double
pow(x,y)
double x,y;
{

if (fp_switch == fp_software)
	{ /* system v error handling */

	double result ;
	struct exception exc;

	if (y == 1) /* easy special case */
		goto bypass ;
	exc.name = "pow";
	exc.arg1 = x;
	exc.arg2 = y;
	exc.retval = 0.0;
	if (!x) {
		if (y > 0)
			goto bypass ;
		goto domain;
		}
	if (x < 0.0) 
		{ /* domain error if x neg and y not integral */
		if ((dclass(y) == normal) && (d_integral(y) == 0)) goto domain ;
		}
	result = _vpowd(x,y) ;
	if (fabs(result) == HUGE)
		{
                        exc.type = OVERFLOW;
                        exc.retval = (result < 0.0) ? -HUGE : HUGE;
                        if (!matherr(&exc))
                                errno = ERANGE;
                        return (exc.retval);
		} ;
	if (fabs(result) < 2.225073858507201383E-308)
		{
                exc.type = UNDERFLOW;
                exc.retval = result ;
		if (result > 0.0 ) exc.retval = 0.0 ;
		if (result < 0.0 ) exc.retval = -0.0 ;
                if (!matherr(&exc))
                        errno = ERANGE;
                return (exc.retval);
		} ;
	return(result) ;
	
domain:
	exc.type = DOMAIN;
	if (!matherr(&exc)) {
		(void) write(2, "pow: DOMAIN error\n", 18);
		errno = EDOM;
	}
	return (exc.retval);
}
else
	{ /* bypass system v error handling */
bypass:
	return(_vpowd(x,y)) ;
	}
}
