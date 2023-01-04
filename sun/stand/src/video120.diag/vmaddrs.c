static char	sccsid[] = "@(#)vmaddrs.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "vmaddrs" (video memory address test) performs the "Address Test".
 * The "Address Test" will actually perform two tests of the video memory.
 * In the first test, the low order bits of a given address will be writen
 * to the memory location corresponding to that address.  In the second test, 
 * the ones complement of the low order bits of a given address will be
 * written to the memory location corresponding to that address.
 */
vmaddrs()
{
  extern char errmsg[]; /* defined in .../stand/src/video120.diag/mm.c */
  extern long prterrs;  /* defined in .../stand/src/video120.diag/mm.c */
  extern long nwrders;  /* defined in .../stand/src/video120.diag/mm.c */
  extern long nbyters;  /* defined in .../stand/src/video120.diag/mm.c */
  extern long bowmode;  /* defined in .../stand/src/video120.diag/mm.c */

  int  c=0;
  long i=0;
  long j=0;
  long k=0;
  long count=0;
  long perrs=0;
  char *tstname="Address Test";
  unsigned char  *baddr=((unsigned char *) VMADDR);
  unsigned char  bobs=0;
  unsigned char  bexp=0;
  unsigned char  bmask=0277;
  unsigned short *waddr=((unsigned short *) VMADDR);
  unsigned short wobs=0;
  unsigned short wexp=0;
  unsigned short smask=0277;

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
   * Perform the "Address Test" 'count' times or forever.
   */
  for(i=0; i < count; i++) {
    for(j=0; j < 2; j++) {
      if (bowmode == 0) { /* byte mode */
        baddr=((unsigned char *) VMADDR);
        for(k=0; k < MAXBYTES; k++, baddr++) { /* fill video memory */
          if (j == 0) { /* write low order bits */
            *baddr=((unsigned char) (((unsigned char) baddr) & bmask));
	  } else { /* write ones complement of low order bits */
            *baddr=((unsigned char) ((~((unsigned char) baddr)) & bmask));
	  }
          if ( (c=maygetchar()) > -1) { /* user pressed a key to stop test */
            strcpy(errmsg, tstname);
            strcat(errmsg, " terminated by user.");
            return('q');
          }
        }
        /*
         * Generate artificial errors by uncommenting the line below.
         * mask=0;
         */
        baddr=((unsigned char *) VMADDR);
        for(k=0; k < MAXBYTES; k++, baddr++) { /* expected=observed? */
          if (j == 0) { /* write low order bits */
            bexp=((unsigned char) (((unsigned char) baddr) & bmask));
	  } else { /* write ones complement of low order bits */
            bexp=((unsigned char) ((~((unsigned char) baddr)) & bmask));
	  }
          if ( (bobs=(*baddr)) != bexp) {
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
        for(k=0; k < MAXWORDS; k++, waddr++) {  /* fill video memory */
          if (j == 0) { /* write low order bits */
            *waddr=((unsigned short) (((unsigned short) waddr) & smask));
	  } else { /* write ones complement of low order bits */
            *waddr=((unsigned short) ((~((unsigned short) waddr)) & smask));
	  }
          if ( (c=maygetchar()) > -1) { /* user pressed a key to stop test */
            strcpy(errmsg, tstname);
            strcat(errmsg, " terminated by user.");
            return('q');
          }
        }
        /*
         * Generate artificial errors by uncommenting the line below.
         * mask=0;
         */
        waddr=((unsigned short *) VMADDR);
        for(k=0; k < MAXWORDS; k++, waddr++) { /* expected=observed? */
          if (j == 0) { /* write low order bits */
            wexp=((unsigned short) (((unsigned short) waddr) & smask));
	  } else { /* write ones complement of low order bits */
            wexp=((unsigned short) ((~((unsigned short) waddr)) & smask));
	  }
          if ( (wobs=(*waddr)) != wexp) {
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


