static char	sccsid[] = "@(#)vmuniq.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "vmuniq" (video memory unique test) performs the "Uniqueness Test".
 * The Uniqueness test will write a series of unique numbers to the video 
 * memory.  Given the i-th memory location in the video memory, the value that
 * will be written to that location is i*constant.  The default value of 
 * constant is one (1).  The user can specify a different constant, however.
 */
vmuniq()
{
  extern char errmsg[]; /* defined in .../stand/src/video120.diag/mm.c */
  extern long prterrs;  /* defined in .../stand/src/video120.diag/mm.c */
  extern long nwrders;  /* defined in .../stand/src/video120.diag/mm.c */
  extern long nbyters;  /* defined in .../stand/src/video120.diag/mm.c */
  extern long bowmode;  /* defined in .../stand/src/video120.diag/mm.c */

  int  c=0;
  long i=0;
  long j=0;
  long addctr=0;
  long const=1;
  long count=0;
  long perrs=0;
  char *tstname="Uniqueness Test";
  unsigned char  *baddr=((unsigned char *) VMADDR);
  unsigned char  bobs=0;
  unsigned short *waddr=((unsigned short *) VMADDR);
  unsigned short wobs=0;

  if (prterrs == TRUE) {
    printf("\n%s\n", tstname);
  }
  /*
   * Read user-specified parameter values and determine whether or not the 
   * values are "legal".
   */
  if (bowmode == 0) { /* byte mode */
    const=eattoken(1, 1, 1, 10);
    if (inrange(const, 0, 255) == FALSE) {
      strcpy(errmsg, 
        "'constant' out of range for byte mode.  Values 0-255 are okay.");
      return('q');
    }
  } else { /* word mode */
    const=eattoken(1, 1, 1, 10);
    if (inrange(const, 0, 65535) == FALSE) {
      strcpy(errmsg, "'constant' out of range for word mode.");
      strcat(errmsg, "  Values 0-65535 are okay.");
      return('q');
    }
  }
  count=eattoken(0x1, 0x1, INFINITY, 10);
  if (inrange(count, 0x1, INFINITY) == FALSE) {
    strcpy(errmsg, "'count' out of range.");
    return('q');
  }
  /*
   * Perform the "Uniqueness Test" 'count' times or forever.
   */
  for(i=0; i < count; i++) {
    if (bowmode == 0) { /* byte mode */
      baddr=((unsigned char *) VMADDR);
      for(j=0; j < MAXBYTES; j++, addctr++, baddr++) { /* fill video memory */
        if ((addctr*const) > 255) { /* prevent overflow */
          addctr=0;
	}
        *baddr=((unsigned char) (addctr*const));
        if ( (c=maygetchar()) > -1) { /* user pressed a key to stop test */
          strcpy(errmsg, tstname);
          strcat(errmsg, " terminated by user.");
          return('q');
        }
      }
      addctr=0;
      /*
       * Generate artificial errors by uncommenting the line below.
       * addctr++;
       */
      baddr=((unsigned char *) VMADDR);
      for(j=0; j < MAXBYTES; j++, addctr++, baddr++) { /* expected=observed? */
        if ((addctr*const) > 255) { /* prevent overflow */
          addctr=0;
	}
        if ( (bobs=(*baddr)) != ((unsigned char) (addctr*const)) ) {
          perrs++;
          c=byteeh(tstname, baddr, ((unsigned char) (addctr*const)), bobs);
	}
        if ( (c == 'q') || ((c != 'c') && (c=maygetchar()) > -1) ) {
          strcpy(errmsg, tstname);
          strcat(errmsg, " terminated by user.");
          return('q');
        }
      }
    } else { /* word mode */
      waddr=((unsigned short *) VMADDR);
      for(j=0; j < MAXWORDS; j++, addctr++, waddr++) {  /* fill video memory */
        if ((addctr*const) > 65535) { /* prevent overflow */
          addctr=0;
	}
        *waddr=((unsigned short) (addctr*const));
        if ( (c=maygetchar()) > -1) { /* user pressed a key to stop test */
          strcpy(errmsg, tstname);
          strcat(errmsg, " terminated by user.");
          return('q');
        }
      }
      addctr=0;
      /*
       * Generate artificial errors by uncommenting the line below.
       * addctr++;
       */
      waddr=((unsigned short *) VMADDR);
      for(j=0; j < MAXWORDS; j++, addctr++, waddr++) { /* expected=observed? */
        if ((addctr*const) > 65535) { /* prevent overflow */
          addctr=0;
	}
        if ( (wobs=(*waddr)) != ((unsigned short) (addctr*const)) ) {
          perrs++;
          c=wordeh(tstname, waddr, ((unsigned short) (addctr*const)), wobs);
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


