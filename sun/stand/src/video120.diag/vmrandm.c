static char	sccsid[] = "@(#)vmrandm.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "vmrandm" (video memory random test) performs the "Random Pattern
 * Test".  The "Random Pattern Test" will write random numbers to the video
 * memory based on a "seed" (a number used to generate the first random 
 * number).  By default, the seed is the decimal integer seven (7).  The user
 * has the opportunity to specify a different seed, however.
 */
vmrandm()
{
  extern char errmsg[]; /* defined in .../stand/src/video120.diag/mm.c */
  extern long prterrs;  /* defined in .../stand/src/video120.diag/mm.c */
  extern long nwrders;  /* defined in .../stand/src/video120.diag/mm.c */
  extern long nbyters;  /* defined in .../stand/src/video120.diag/mm.c */
  extern long bowmode;  /* defined in .../stand/src/video120.diag/mm.c */

  int  c=0;
  long i=0;
  long j=0;
  long count=0;
  long perrs=0;
  long seed=7;
  char *tstname="Random Pattern Test";
  static   long  dummy[32]={3};
  unsigned char  *baddr=((unsigned char *) VMADDR);
  unsigned char  bobs=0;
  unsigned char  bexp=0;
  unsigned short *waddr=((unsigned short *) VMADDR);
  unsigned short wobs=0;
  unsigned short wexp=0;

  if (prterrs == TRUE) {
    printf("\n%s\n", tstname);
  }
  /*
   * Read user-specified parameter values and determine whether or not the 
   * values are "legal".
   */
  seed=eattoken(7, 7, 7, 10);
  count=eattoken(0x1, 0x1, INFINITY, 10);
  if (inrange(count, 0x1, INFINITY) == FALSE) {
    strcpy(errmsg, "'count' out of range.");
    return('q');
  }
  /*
   * Perform the "Random Test" 'count' times or forever.
   */
  for(i=0; i < count; i++) {
    initstate(seed, dummy, 128); /* set up seeded random sequence */
    if (bowmode == 0) { /* byte mode */
      baddr=((unsigned char *) VMADDR);
      for(j=0; j < MAXBYTES; j++, baddr++) { /* fill video memory */
      *baddr=((unsigned char) (random()));
        if ( (c=maygetchar()) > -1) { /* user pressed a key to stop test */
          strcpy(errmsg, tstname);
          strcat(errmsg, " terminated by user.");
          return('q');
        }
      }
      initstate(seed, dummy, 128); /* set up seeded random sequence */
      /*
       * Generate artificial errors by uncommenting the line below.
       * initstate(seed, dummy, 8);
       */
      baddr=((unsigned char *) VMADDR);
      for(j=0; j < MAXBYTES; j++, baddr++) { /* expected=observed? */
        if ( (bobs=(*baddr)) != (bexp=((unsigned char) (random()))) ) {
          perrs++;
          c=byteeh(tstname, baddr, bexp, bobs);
	}
        if ( (c == 'q') || ((c != 'c') && (c=maygetchar()) > -1) ) {
          strcpy(errmsg, tstname);
          strcat(errmsg, " terminated by user.");
          return('q');
        }
      }
    } else { /* word mode */
      waddr=((unsigned short *) VMADDR);
      for(j=0; j < MAXWORDS; j++, waddr++) {  /* fill video memory */
        *waddr=((unsigned short) (random()));
        if ( (c=maygetchar()) > -1) { /* user pressed a key to stop test */
          strcpy(errmsg, tstname);
          strcat(errmsg, " terminated by user.");
          return('q');
        }
      }
      initstate(seed, dummy, 128); /* set up seeded random sequence */
      /*
       * Generate artificial errors by uncommenting the line below.
       * initstate(seed, dummy, 8);
       */
      waddr=((unsigned short *) VMADDR);
      for(j=0; j < MAXWORDS; j++, waddr++) { /* expected=observed? */
        if ( (wobs=(*waddr)) != (wexp=((unsigned short) (random()))) ) {
          perrs++;
          c=wordeh(tstname, waddr, wexp, wobs);
	}
        if ( (c == 'q') || ((c != 'c') && (c=maygetchar()) > -1) ) {
          strcpy(errmsg, tstname);
          strcat(errmsg, " terminated by user.");
          return('q');
        }
      }
    }
    printf("Pass %ld of %s.\n", i+1, tstname);
    printf("Total of %ld errors  :  %ld word errors  :  %ld byte errors\n",
            perrs, nwrders, nbyters);
    delay(5000);
    if (pause() == 'q') {
      strcpy(errmsg, tstname);
      strcat(errmsg, " terminated by user.");
      return('q');
    }
    if (i >= INFINITY) { /* Since the test is to    */
      i=(-1);            /* run forever, reset 'i'. */
    }
  } /* end of "i" loop */
  return(0);
}


