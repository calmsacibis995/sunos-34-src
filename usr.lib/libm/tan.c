/*
static	char sccsid[] = "@(#)tan.c 1.1 86/09/25 Copyr 1986 Sun Micro";
*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <errno.h>
#include <math.h>
#include "fpcrttypes.h"
#include "libmdefs.h"

double
tan(x)
double x;
{

if (fp_switch == fp_software)
	{ /* system v error handling */
               	double result ;

	if (fabs(x) == HUGE)
		{
        int neg = (x < 0.0 ); /* tan of negative argument */
        struct exception exc;
        
	exc.type = DOMAIN;
        exc.name = "tan" ;
        exc.arg1 = x;
        exc.retval = neg ? -0.0 : 0.0;
        if (!matherr(&exc)) {
                (void) write(2, "tan: DOMAIN error\n", 18);
                errno = EDOM;
                }
        return (exc.retval);
		} /* infinite arg */
        return(_vtand(x)) ;
	}
else
	{ /* bypass system v error handling */
	return(_vtand(x)) ;
	}
}
