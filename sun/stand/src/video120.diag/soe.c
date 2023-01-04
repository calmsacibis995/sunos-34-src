static char	sccsid[] = "@(#)soe.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "soe" (scopeloop on error) turns the scopeloop-on-error flag "
 * errmode" on (1) or off (0). By default, scopeloop-on-error is off.
 */
soe()
{
  extern char errmsg[]; /* defined in .../stand/src/video120.diag/mm.c */
  extern long errmode;  /* defined in .../stand/src/video120.diag/mm.c */

  long tmp=0;

  /*
   * Read user-specified parameter value and determine whether or not the 
   * value is "legal".
   */
  tmp=eattoken(0x0, 0x0, 0x0, 10); /* default: scopeloop off */
  if (inrange(tmp, 0x0, 0x1) == FALSE) {
    strcpy(errmsg, 
           "Value out of range.  0=no scopeloop, 1=scopeloop");
    return('q');
  }
  if (tmp == 0) {
    errmode &= (~2); /* turn scopeloop off only */
  } else {
    errmode |= 2;    /* turn scopeloop on       */
  }
  return(0);
}


