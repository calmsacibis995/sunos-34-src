static char	sccsid[] = "@(#)cp.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "cp" (check parity) turns the parity checking flag "paritee" 
 * on (1) or off (0). By default, parity checking is on.
 */
cp()
{
  extern char errmsg[]; /* defined in .../stand/src/video120.diag/mm.c */
  extern long paritee;  /* defined in .../stand/src/video120.diag/mm.c */

  /*
   * Read user-specified parameter value and determine whether or not the 
   * value is "legal".
   */
  paritee=eattoken(0x1, 0x1, 0x1, 10); /* default: parity on */
  if (inrange(paritee, 0x0, 0x1) == FALSE) {
    strcpy(errmsg, 
           "Value out of range.  0=no parity checking, 1=parity checking");
    return('q');
  }
  if (paritee == 0) {
    parity(PAR_GEN, !(PAR_CHECK)); /* parity checking off */
  } else {
    parity(PAR_GEN, PAR_CHECK);    /* parity checking on  */
  }
  return(0);
}


