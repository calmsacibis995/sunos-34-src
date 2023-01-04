static char	sccsid[] = "@(#)pem.c 1.1 9/25/86 Copyright Sun Microsystems";

/*
 * Don't change the order of the include files!
 */
#include <sys/types.h>
#include <machdep.h>
#include <sunromvec.h>
#include <s2addrs.h>
#include <token.h>
#include "video120.h"


/* 
 * Function "pem" (print error messages) turns the print-error-message flag 
 * "prterrs" on (1) or off (0). By default, error messages are printed.
 */
pem()
{
  extern char errmsg[]; /* defined in .../stand/src/video120.diag/mm.c */
  extern long prterrs;  /* defined in .../stand/src/video120.diag/mm.c */

  /*
   * Read user-specified parameter value and determine whether or not the 
   * value is "legal".
   */
  prterrs=eattoken(0x1, 0x1, 0x1, 10); /* default: prterrs on */
  if (inrange(prterrs, 0x0, 0x1) == FALSE) {
    strcpy(errmsg, 
           "Value out of range.  0=don't print errors, 1=print errors");
    return('q');
  }
  return(0);
}


