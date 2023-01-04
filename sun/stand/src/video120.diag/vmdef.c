static char	sccsid[] = "@(#)vmdef.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "vmdef" (video memory default test) executes the default
 * video memory tests.
 */
vmdef() 
{
/* 
 * The following external variable(s) are defined 
 * in .../stand/src/video120.diag/vmm.c.
 */
extern char vdefcmd[];

  tokenparse(vdefcmd);
  return(0);
}
