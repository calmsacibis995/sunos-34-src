static char	sccsid[] = "@(#)wordeh.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "wordeh" (word error handler) handles word memory errors.
 */
wordeh(tstname, waddr, wexp, wobs)
  char           *tstname;
  unsigned short *waddr;
  unsigned short wexp;
  unsigned short wobs;
{
  extern char errmsg[];     /* defined in .../stand/src/video120.diag/mm.c */
  extern long errmode;      /* defined in .../stand/src/video120.diag/mm.c */
  extern long prterrs;      /* defined in .../stand/src/video120.diag/mm.c */
  extern long nwrders;      /* defined in .../stand/src/video120.diag/mm.c */
  extern WRDNODE worders[]; /* defined in .../stand/src/video120.diag/mm.c */

  int  c=0;

  if (prterrs == TRUE) {
    printf("\n%s failed at address 0x%x.\n", tstname, waddr);
    printf("  Expected=0x%x  Observed=0x%x  XOR=0x%x\n", 
            wexp, wobs, (wexp^wobs));
  }
  if (++nwrders < MAXERRS) { /* store word error if space available */
    strcpy(worders[nwrders].tstname, tstname);
    worders[nwrders].addr=waddr;
    worders[nwrders].exp=wexp;
    worders[nwrders].obs=wobs;
  } else {
    strcpy(errmsg, "Over 100 word memory errors have been found already.");
  }
  switch(errmode) {
  case 0: /* continue */
    return(0);
    break;
  case 1: /* wait (stop) on error */
    while( (c=maygetchar()) < 0) { /* wait until user hits */
      ;                            /* any key to continue  */
    }
    return( ((c == 'q') ? 'q' : 'c') ); /* 'q' ==> quit, 'c' ==> continue */
    break;
  case 2: /* scopeloop on error */
  case 3: /* scopeloop and wait (stop) on error */
    return( ((wsloop(waddr, wexp, wobs) == 'q') ? 'q' : 'c') );
    break;
  default: /* illegal error mode */
    strcpy(errmsg, "Illegal error mode.  0=no stop/scopeloop, 1=stop,");
    strcat(errmsg, "2=scopeloop, 3=stop/scopeloop");
    return('q');
    break;
  }
}


