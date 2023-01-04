static char	sccsid[] = "@(#)prompt.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "prompt" prints the command prompt (Command ==>).
 */
prompt() 
{

  printf("Command ==>");
}
