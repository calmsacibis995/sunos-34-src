static char	sccsid[] = "@(#)crdef.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "crdef" (control register default test) executes the default
 * control register tests.
 */
crdef() 
{
/* 
 * The following external variable(s) are defined 
 * in .../stand/src/video120.diag/crm.c.
 */
extern char cdefcmd[];

  tokenparse(cdefcmd);
  return(0);
}
