static char	sccsid[] = "@(#)quit.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "quit" restores the original video interrupt vector and resets
 * the CPU interrupt priority level before terminating the diagnostic.
 */
quit() 
{
  extern int (*oldvctr)();

  ex_vector->e_int[4]=oldvctr;
  asm("	movw	#0x2600, sr");
  exit(0);
}


