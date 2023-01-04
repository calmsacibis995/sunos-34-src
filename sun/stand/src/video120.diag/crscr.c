static char	sccsid[] = "@(#)crscr.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "crscr" (control register screen test) performs the
 * "Screen (Display) Enable/Disable Test".  The "Screen (Display) 
 * Enable/Disable Test" turns the screen off and then back on again.
 */
crscr()
{
  extern char errmsg[]; /* defined in .../stand/src/video120.diag/mm.c */
  extern long prterrs;  /* defined in .../stand/src/video120.diag/mm.c */

  long i=0;
  long count=0x1;
  char *tstname="Screen (Display) Enable/Disable Test";
  unsigned short *craddr=((unsigned short *) CRADDR);

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
   * Perform the "Screen (Display) Enable/Disable Test" 'count' times or
   * forever.
   */
  for(i=0; i < count; i++) {
    printf("\nThe screen is about to be turned off for a few seconds.\n");
    delay(5000);
    if (pause() == 'q') {
      strcpy(errmsg, tstname);
      strcat(errmsg, " terminated by user.");
      return('q');
    }
    *craddr&=0077777;
    delay(5000);
    if (pause() == 'q') {
      strcpy(errmsg, tstname);
      strcat(errmsg, " terminated by user.");
      return('q');
    }
    *craddr|=0100000;
    printf("\014"); /* clear screen */
    printf("\nThe screen should have just gone off and then back on again.\n");
    delay(5000);
    if (pause() == 'q') {
      strcpy(errmsg, tstname);
      strcat(errmsg, " terminated by user.");
      return('q');
    }
    if (i >= INFINITY) { /* Since the test is to    */
      i=(-1);            /* run forever, reset 'i'. */
    }
  }
  return(0);
}


