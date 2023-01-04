/*
static	char sccsid[] = "@(#)sinh.c 1.1 86/09/25 Copyr 1986 Sun Micro";
*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <errno.h>
#include <math.h>
#include "fpcrttypes.h"
#include "libmdefs.h"

double
sinh(x)
double x;
{

if (fp_switch == fp_software)
	{ /* system v error handling */
               	double result ;

               	result = _vsinhd(x) ;
		if (fabs(result) == HUGE) 
			{ /* overflow */
        int neg = (x < 0.0 ); /* sinh of negative argument */
        struct exception exc;
 
        exc.type = OVERFLOW;
        exc.name = "sinh" ;
        exc.arg1 = x;
        exc.retval = neg ? -HUGE : HUGE;
        if (!matherr(&exc))
                errno = ERANGE;
        return (exc.retval);
			} /* overflow */
		else
			return(result) ;

	}
else
	{ /* bypass system v error handling */
	return(_vsinhd(x)) ;
	}
}

double
cosh(x)
double x;
{

if (fp_switch == fp_software)
	{ /* system v error handling */
               	double result ;

               	result = _vcoshd(x) ;
		if (result == HUGE) 
			{ /* overflow */
        struct exception exc;
 
        exc.type = OVERFLOW;
        exc.name = "cosh" ;
        exc.arg1 = x;
        exc.retval = HUGE;
        if (!matherr(&exc))
                errno = ERANGE;
        return (exc.retval);
			} /* overflow */
		else
			return(result) ;

	}
else
	{ /* bypass system v error handling */
	return(_vcoshd(x)) ;
	}
}
