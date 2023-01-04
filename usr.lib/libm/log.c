/*
static	char sccsid[] = "@(#)log.c 1.1 86/09/25 Copyr 1986 Sun Micro";
*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <errno.h>
#include <math.h>
#include "fpcrttypes.h"
#include "libmdefs.h"

static double
log_error(x, f_name, name_len)
double x;
char *f_name;
unsigned int name_len;
{
        register char *err_type;
        unsigned int mess_len;
        struct exception exc;
 
        exc.name = f_name;
        exc.retval = -HUGE;
        exc.arg1 = x;
        if (x) {
                exc.type = DOMAIN;
                err_type = ": DOMAIN error\n";
                mess_len = 15;
        } else {
                exc.type = SING;
                err_type = ": SING error\n";
                mess_len = 13;
        }
        if (!matherr(&exc)) {
                (void) write(2, f_name, name_len);
                (void) write(2, err_type, mess_len);
                errno = EDOM;
        }
        return (exc.retval);
}

double
log(x)
double x;

{

if (fp_switch == fp_software)
	{ /* system v error handling */
        if (x <= 0)
		{
                return (log_error(x, "log", 3));
		}
	else
		{
		return(_vlogd(x)) ;
		}
	}
else
	{ /* bypass system v error handling */
	return(_vlogd(x)) ;
	}
}

double
log10(x)
double x;

{

if (fp_switch == fp_software)
	{ /* system v error handling */
        if (x <= 0)
		{
                return (log_error(x, "log10", 5));
		}
	else
		{
		return(_vlog10d(x)) ;
		}
	}
else
	{ /* bypass system v error handling */
	return(_vlog10d(x)) ;
	}
}
