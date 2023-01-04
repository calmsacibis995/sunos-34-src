static char	sccsid[] = "@(#)loop.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "loop" allows current sequence of test(s) to 
 * be executed a user-specified number of times or forever.
 */
loop()
{
  extern long lflag;
  extern long lcount;

  if (lflag <= 0) { /* Read user-specified parameter value. */
    lcount=eattoken(0x0, 0x0, INFINITY, 10);
    lflag=lcount;
  }
  if (lcount >= INFINITY) { /* Execute the test sequence forever. */
    printf("End of one test sequence.\n\n");
    token=tokens;
    return(0);
  } else { /* Execute the test sequence a user-specified number of times. */
    if ( (lflag > 0) && ((--lcount) > 0) ) { /* more than one pass is left */
      printf("End of test sequence number: %d\n\n", (lflag-lcount));
      token=tokens;
      return(0);
    } else if (lflag > 0) { /* the last pass */
      printf("End of test sequence number: %d\n\n", (lflag-lcount));
      lflag=0;
      *token=NULL;
      return(0);
    } else { /* User error: No "loop_count" parameter or a negative one. */
      printf("End of test sequence number: 1\n\n");
      lflag=0;
      *token=NULL;
      return(0);
    }
  }
}


