/*
static	char sccsid[] = "@(#)atan.c 1.1 86/09/25 Copyr 1986 Sun Micro";
*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <errno.h>
#include <math.h>
#include "fpcrttypes.h"
#include "libmdefs.h"

double
atan(x)
double x;
{
return(_vatand(x)) ;
}

double
atan2(x,y)
double x,y;
{

if (fp_switch == fp_software)
	{ /* system v error handling */
	double result ;

        if (!x && !y) {
        	struct exception exc;
	
		exc.arg1 = x ; exc.arg2 = y ;
                exc.type = DOMAIN;
                exc.name = "atan2";
                exc.retval = 0.0;
                if (!matherr(&exc)) {
                        (void) write(2, "atan2: DOMAIN error\n", 20);
                        errno = EDOM;
                }
                return (exc.retval);
		}
	return(_vatan2d(x,y)) ;
	}
else
	{ /* bypass system v error handling */
	return(_vatan2d(x,y)) ;
	}
}
