/*
static	char sccsid[] = "@(#)hypot.c 1.1 86/09/25 Copyr 1986 Sun Micro";
*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <errno.h>
#include <math.h>
#include "fpcrttypes.h"
#include "libmdefs.h"

double
hypot(x,y)
double x,y;
{

if (fp_switch == fp_software)
	{ /* system v error handling */
	double result ;

	result = _vlength2d(x,y) ;
	if (result == HUGE) 
		{
        struct exception exc;
 
	exc.arg1 = x ; exc.arg2 = y ;
	exc.type = OVERFLOW;
        exc.name = "hypot";
        exc.retval = HUGE;
        if (!matherr(&exc))
                errno = ERANGE;
        return (exc.retval);
		}
	return(result) ;
}
else
	{ /* bypass system v error handling */
	return(_vlength2d(x,y)) ;
	}
}
