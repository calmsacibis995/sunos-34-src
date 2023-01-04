static char	sccsid[] = "@(#)crint.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "crint" (control register interrupt test) performs the
 * "Interrupt Enable/Disable Test".  The "Interrupt Enable/Disable Test"
 * determines whether or not the video board can interrupt the CPU.
 */
crint()
{
  extern char errmsg[]; /* defined in .../stand/src/video120.diag/mm.c */
  extern long prterrs;  /* defined in .../stand/src/video120.diag/mm.c */
  extern long intrupt;  /* defined in .../stand/src/video120.diag/mm.c */

  int  c=0;
  long i=0;
  long count=1;
  char *tstname="Interrupt Enable/Disable Test";
  unsigned short *craddr=((unsigned short *) CRADDR);
  unsigned short mask13=0020000; /* bit 13 generates a video board interrupt */

  if (prterrs == TRUE) {
    printf("\n%s\n", tstname);
  }
  /*
   * Read user-specified parameter value and determine whether or not the 
   * value is "legal".
   */
  count=eattoken(0x1, 0x1, INFINITY, 10);
  if (inrange(count, 0x1, INFINITY) == FALSE) {
    strcpy(errmsg, "'count' out of range.");
    return('q');
  }
  /*
   * Perform the "Interrupt Enable/Disable Test" 'count' times or forever.
   */
  for(i=0; i < count; i++) {
    intrupt=0;
    *craddr|=mask13; /* generate a video board interrupt */
    delay(1000); /* give interrupt routine more than enough time to execute */
    if (intrupt == 1) { /* was interrupt routine executed? */
      printf("\nThe video interrupt routine was executed.\n");
    } else {
      printf("\nThe video interrupt routine was NOT executed.\n");
    }
    delay(5000);
    if (pause() == 'q') {
      strcpy(errmsg, tstname);
      strcat(errmsg, " terminated by user.");
      return('q');
    }
    if (i >= INFINITY) { /* Since the test is to     */
      i=(-1);            /* run forever, reset 'i'.  */
    }
  }
  return(0);
}


