static char	sccsid[] = "@(#)gotomm.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "gotomm" (go to Main Menu) is a dummy function. Given the
 * existing menu implementation with Sun-2/120 diagnostics, this kludge
 * is necessary to allow the user to return to the Main Menu.  (Nice!)
 */
gotomm() 
{

  return('p'); /* 'p' implies return to the Main Menu */
}


