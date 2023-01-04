/*
static	char sccsid[] = "@(#)sin.c 1.1 86/09/25 Copyr 1986 Sun Micro";
*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <errno.h>
#include <math.h>
#include "fpcrttypes.h"
#include "libmdefs.h"

double
sin(x)
double x;
{

if (fp_switch == fp_software)
	{ /* system v error handling */
               	double result ;

	if (fabs(x) == HUGE)
		{
        int neg = (x < 0.0 ); /* sin of negative argument */
        struct exception exc;
        
	exc.type = DOMAIN;
        exc.name = "sin" ;
        exc.arg1 = x;
        exc.retval = neg ? -0.0 : 0.0;
        if (!matherr(&exc)) {
                (void) write(2, "sin: DOMAIN error\n", 18);
                errno = EDOM;
        	}
	return (exc.retval);
		} /* infinite arg */
        return(_vsind(x)) ;
	}
else
	{ /* bypass system v error handling */
	return(_vsind(x)) ;
	}
}

double
cos(x)
double x;
{

if (fp_switch == fp_software)
	{ /* system v error handling */
               	double result ;

        if (fabs(x) == HUGE)
                {
        struct exception exc;
        
        exc.type = DOMAIN;
        exc.name = "cos" ;
        exc.arg1 = x;
        exc.retval = 0.0;
        if (!matherr(&exc)) {
                (void) write(2, "cos: DOMAIN error\n", 18);
                errno = EDOM;
                }
        return (exc.retval);
                } /* infinite arg */
        return(_vcosd(x)) ;

	}
else
	{ /* bypass system v error handling */
	return(_vcosd(x)) ;
	}
}
