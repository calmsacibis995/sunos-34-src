static char	sccsid[] = "@(#)mmall.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "mmall" (main menu all) executes all of the video board tests.
 */
mmall() 
{
/* 
 * The following external variable(s) are defined 
 * in .../stand/src/video120.diag/mm.c.
 */
extern char mallcmd[];

  tokenparse(mallcmd);
  return(0);
}
