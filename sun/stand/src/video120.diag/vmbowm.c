static char	sccsid[] = "@(#)vmbowm.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "vmbowm" (video memory byte or word mode) turns the 
 * byte-or-word-mode flag "bowmode" on (1) or off (0). By default,
 * the mode is "word".
 */
vmbowm()
{
  extern char errmsg[]; /* defined in .../stand/src/video120.diag/mm.c */
  extern long bowmode;  /* defined in .../stand/src/video120.diag/mm.c */

  /*
   * Read user-specified parameter value and determine whether or not the 
   * value is "legal".
   */
  bowmode=eattoken(0x1, 0x1, 0x1, 10); /* default: byte-or-word-mode on */
  if (inrange(bowmode, 0x0, 0x1) == FALSE) {
    strcpy(errmsg, 
           "Value out of range.  0=byte mode, 1=word mode");
    return('q');
  }
  return(0);
}


