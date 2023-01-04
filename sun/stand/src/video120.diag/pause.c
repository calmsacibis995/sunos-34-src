static char	sccsid[] = "@(#)pause.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "pause" suspends the execution of the program if the user 
 * has pressed any key except 'q' until the user presses another key to 
 * continue.  If the user did press 'q', function "pause" returns immediately.
 */
pause()
{
  int  c='\0';

  if ( (c=maygetchar()) > -1) { /* user suspended the program */
    if (c != 'q') {
      c=getchar(); /* wait until user hits any key to continue  */
    }
    return(c);
  }
  return(0);
}


