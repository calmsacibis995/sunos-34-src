/*
static	char sccsid[] = "@(#)exp.c 1.1 86/09/25 Copyr 1986 Sun Micro";
*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <errno.h>
#include <math.h>
#include "fpcrttypes.h"
#include "libmdefs.h"

double
exp(x)
double x;
{

if (fp_switch == fp_software)
	{ /* system v error handling */
               	double result ;
		struct exception exc;

               	result = _vexpd(x) ;
		if (result == HUGE) 
			{ /* overflow */
	                exc.arg1 = x ; exc.name = "exp" ;
			exc.type = OVERFLOW;
	                exc.retval = HUGE;
	                if (!matherr(&exc))
	                        errno = ERANGE;
	                return (exc.retval);
			} /* overflow */
		else
		if (result < 2.225073858507201383E-308)
			{ /* underflow */
	                exc.arg1 = x ; exc.name = "exp" ;
	                exc.type = UNDERFLOW;
	                exc.retval = 0.0;
	                if (!matherr(&exc))
	                        errno = ERANGE;
	                return (exc.retval);
			} /* underflow */
		else
			return(result) ;

	}
else
	{ /* bypass system v error handling */
	return(_vexpd(x)) ;
	}
}
