static char	sccsid[] = "@(#)sccdef.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "sccdef" (serial communications controller default test) executes 
 * the default serial communications controller tests.
 */
sccdef() 
{
/* 
 * The following external variable(s) are defined 
 * in .../stand/src/video120.diag/sccm.c.
 */
extern char sdefcmd[];

  tokenparse(sdefcmd);
  return(0);
}
