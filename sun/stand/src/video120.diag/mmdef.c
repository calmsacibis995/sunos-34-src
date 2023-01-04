static char	sccsid[] = "@(#)mmdef.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "mmdef" (main menu default test) executes the default video
 * board tests.
 */
mmdef() 
{
/* 
 * The following external variable(s) are defined 
 * in .../stand/src/video120.diag/mm.c.
 */
extern char mdefcmd[];

  tokenparse(mdefcmd);
  return(0);
}
