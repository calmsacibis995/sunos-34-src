/*
static	char sccsid[] = "@(#)asin.c 1.1 86/09/25 Copyr 1986 Sun Micro";
*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <errno.h>
#include <math.h>
#include "fpcrttypes.h"
#include "libmdefs.h"

double
asin(x)
double x;
{

if (fp_switch == fp_software)
	{ /* system v error handling */
	if (fabs(x) > 1.0) 
		{ /* error case */
                struct exception exc;

                exc.arg1 = x ;
		exc.type = DOMAIN;
                exc.name = "asin";
                exc.retval = 0.0;
                if (!matherr(&exc)) {
                        (void) write(2, exc.name, 4);
                        (void) write(2, ": DOMAIN error\n", 15);
                        errno = EDOM;
                }
                return (exc.retval);
		}
	else
		{
		return(_vasind(x)) ;
		}
	}
else
	{ /* bypass system v error handling */
	return(_vasind(x)) ;
	}
}

double
acos(x)
double x;
{

if (fp_switch == fp_software)
	{ /* system v error handling */
	if (fabs(x) > 1.0) 
		{ /* error case */
                struct exception exc;

                exc.arg1 = x ;
		exc.type = DOMAIN;
                exc.name = "acos";
                exc.retval = 0.0;
                if (!matherr(&exc)) {
                        (void) write(2, exc.name, 4);
                        (void) write(2, ": DOMAIN error\n", 15);
                        errno = EDOM;
                }
                return (exc.retval);
		}
	else
		{
		return(_vacosd(x)) ;
		}
	}
else
	{ /* bypass system v error handling */
	return(_vacosd(x)) ;
	}
}
