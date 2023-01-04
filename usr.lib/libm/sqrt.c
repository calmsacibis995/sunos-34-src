/*
static	char sccsid[] = "@(#)sqrt.c 1.1 86/09/25 Copyr 1986 Sun Micro";
*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <errno.h>
#include <math.h>
#include "fpcrttypes.h"
#include "libmdefs.h"

double
sqrt(x)
double x;
{

if (fp_switch == fp_software)
	{ /* system v error handling */
	if (x < 0.0) 
		{ /* error case */
                struct exception exc;

                exc.type = DOMAIN;
                exc.name = "sqrt";
                exc.arg1 = x;
                exc.retval = 0.0;
                if (!matherr(&exc)) {
                        (void) write(2, "sqrt: DOMAIN error\n", 19);
                        errno = EDOM;
                }
                return (exc.retval);
		}
	else
		{
		return(_vsqrtd(x)) ;
		}
	}
else
	{ /* bypass system v error handling */
	return(_vsqrtd(x)) ;
	}
}
