static char	sccsid[] = "@(#)byteeh.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "byteeh" (byte error handler) handles byte memory errors.
 */
byteeh(tstname, baddr, bexp, bobs)
  char          *tstname;
  unsigned char *baddr;
  unsigned char bexp;
  unsigned char bobs;
{
  extern char errmsg[];     /* defined in .../stand/src/video120.diag/mm.c */
  extern long errmode;      /* defined in .../stand/src/video120.diag/mm.c */
  extern long prterrs;      /* defined in .../stand/src/video120.diag/mm.c */
  extern long nbyters;      /* defined in .../stand/src/video120.diag/mm.c */
  extern BYTNODE byteers[]; /* defined in .../stand/src/video120.diag/mm.c */

  int c=0;

  if (prterrs == TRUE) {
    printf("\n%s failed at address 0x%x.\n", tstname, baddr);
    printf("  Expected=0x%x  Observed=0x%x  XOR=0x%x\n", 
            bexp, bobs, (bexp^bobs));
  }
  if (++nbyters < MAXERRS) { /* store byte error if space available */
    strcpy(byteers[nbyters].tstname, tstname);
    byteers[nbyters].addr=baddr;
    byteers[nbyters].exp=bexp;
    byteers[nbyters].obs=bobs;
  } else {
    strcpy(errmsg, "Over 100 byte memory errors have been found already.");
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
    return( ((bsloop(baddr, bexp, bobs) == 'q') ? 'q' : 'c') );
    break;
  default: /* illegal error mode */
    strcpy(errmsg, "Illegal error mode.  0=no stop/scopeloop, 1=stop,");
    strcat(errmsg, "2=scopeloop, 3=stop/scopeloop");
    return('q');
    break;
  }
}


