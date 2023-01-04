static char	sccsid[] = "@(#)vmconst.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "vmconst" (video memory constant test) performs the
 * "Constant Pattern Test".  The "Constant Pattern Test" writes a constant
 * value to each byte (or word) in the video memory, reads back the contents
 * of each byte (or word) and compares the values read back with the expected 
 * value before displaying a message indicating the success or failure of the
 * test.  By default, the value 0x5555 is written; however, the user can specify
 * a different value.  The value of flag 'bowmode' (byte or word mode) 
 * determines whether bytes or words are written.
 */
vmconst()
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
  long pattern=0;
  char *tstname="Constant Pattern Test";
  unsigned char  *baddr=((unsigned char *) VMADDR);
  unsigned char  bpat=0x55; /* default byte pattern */
  unsigned char  bobs=0;
  unsigned short *waddr=((unsigned short *) VMADDR);
  unsigned short wpat=0x5555; /* default byte pattern */
  unsigned short wobs=0;

  if (prterrs == TRUE) {
    printf("\n%s\n", tstname);
  }
  /*
   * Read user-specified parameter values and determine whether or not the 
   * values are "legal".
   */
  if (bowmode == 0) { /* byte mode */
    pattern=eattoken(0x55, 0x55, 0x55, 16);
    if (inrange(pattern, 0x00, 0xff) == FALSE) {
      strcpy(errmsg, 
        "'pattern' out of range for byte mode.  Values 0x00-0xff are okay.");
      return('q');
    }
    bpat=((unsigned char) pattern);
  } else { /* word mode */
    pattern=eattoken(0x5555, 0x5555, 0x5555, 16);
    if (inrange(pattern, 0x0000, 0xffff) == FALSE) {
      strcpy(errmsg, "'pattern' out of range for word mode.");
      strcat(errmsg, "  Values 0x0000-0xffff are okay.");
      return('q');
    }
    wpat=((unsigned short) pattern);
  }
  count=eattoken(0x1, 0x1, INFINITY, 10);
  if (inrange(count, 0x1, INFINITY) == FALSE) {
    strcpy(errmsg, "'count' out of range.");
    return('q');
  }
  /*
   * Perform the "Constant Pattern Test" 'count' times or forever.
   */
  for(i=0; i < count; i++) {
    if (bowmode == 0) { /* byte mode */
      baddr=((unsigned char *) VMADDR);
      for(j=0; j < MAXBYTES; j++, baddr++) { /* fill video memory */
        *baddr=bpat;
        if ( (c=maygetchar()) > -1) { /* user pressed a key to stop test */
          strcpy(errmsg, tstname);
          strcat(errmsg, " terminated by user.");
          return('q');
        }
      }
      /*
       * Generate artificial errors by uncommenting the line below.
       * bpat=((unsigned char) 0000);
       */
      baddr=((unsigned char *) VMADDR);
      for(j=0; j < MAXBYTES; j++, baddr++) { /* expected=observed? */
        if ( (bobs=(*baddr)) != bpat) {
          perrs++;
          c=byteeh(tstname, baddr, bpat, bobs);
        }
        if ( (c == 'q') || ((c != 'c') && (c=maygetchar()) > -1) ) {
          strcpy(errmsg, tstname);
          strcat(errmsg, " terminated by user.");
          return('q');
        }
      }
    } else { /* word mode */
      waddr=((unsigned short *) VMADDR);
      for(j=0; j < MAXWORDS; j++, waddr++) { /* fill video memory */
        *waddr=wpat;
        if ( (c=maygetchar()) > -1) { /* user pressed a key to stop test */
          strcpy(errmsg, tstname);
          strcat(errmsg, " terminated by user.");
          return('q');
        }
      }
      /*
       * Generate artificial errors by uncommenting the line below.
       * wpat=((unsigned short) 00000000);
       */
      waddr=((unsigned short *) VMADDR);
      for(j=0; j < MAXWORDS; j++, waddr++) { /* expected=observed? */
        if ( (wobs=(*waddr)) != wpat) {
          perrs++;
          c=wordeh(tstname, waddr, wpat, wobs);
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


