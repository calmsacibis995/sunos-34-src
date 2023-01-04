static char	sccsid[] = "@(#)vmchekr.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "vmchekr" (video memory checker test) performs the "Checker Test".
 * The Checker Pattern test will actually perform multiple tests of the video
 * memory.  For the sake of notational convenience, let me introduce the 
 * following.  Given a binary pattern X, let ~X be the ones complement of that 
 * binary pattern.  For instance, if binary pattern X is 0, the ones complement
 * of that pattern (~X) is 1.  Initially, the "Checker Pattern Test" will write
 * the alternating sequence of "X ~X" throughout the video memory.  In the 
 * second pass, the "Checker Pattern Test" will write the alternating sequence
 * of "X X ~X ~X" in the video memory.  In the third pass, the "Checker Pattern
 * Test" will write the alternating sequence of "X X X ~X ~X ~X" throughout the
 * video memory.  This process will continue until one half of the video memory
 * is filled with X's and the other half is filled with ~X's.
 */
vmchekr()
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
  long l=0;
  long m=0;
  long count=0;
  long perrs=0;
  long pattern=0;
  char *tstname="Checker Pattern Test";
  unsigned char  *baddr=((unsigned char *) VMADDR);
  unsigned char  bobs=0;
  unsigned char  bpat=0;
  unsigned short *waddr=((unsigned short *) VMADDR);
  unsigned short wobs=0;
  unsigned short wpat=0;

  /*
   * Read user-specified parameter values and determine whether or not the 
   * values are "legal".
   */
  if (bowmode == 0) { /* byte mode */
    pattern=eattoken(0xa0, 0xa0, 0xa0, 16);
    if (inrange(pattern, 0x00, 0xff) == FALSE) {
      strcpy(errmsg, 
        "'pattern' out of range for byte mode.  Values 0x00-0xff are okay.");
      return('q');
    }
    bpat=((unsigned char) pattern);
  } else { /* word mode */
    pattern=eattoken(0xa0a0, 0xa0a0, 0xa0a0, 16);
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
   * Perform the "Checker Test" 'count' times or forever.
   */
  for(i=0; i < count; i++) {
    if (bowmode == 0) { /* byte mode */
      for(j=1; j < MAXBYTES/(sizeof(unsigned char)); j*=2) {
        for(k=0; k < 2; k++) { /* fill video memory */
          baddr=(((unsigned char *) VMADDR)+(j*k));
          for(l=MAXBYTES/(j*(sizeof(unsigned char))*2); l != 0; l--) {
            for(m=j; m != 0; m--, baddr++) {
              *baddr=bpat;
              if ( (c=maygetchar()) > -1) { /* user hit a key to stop test */
                strcpy(errmsg, tstname);
                strcat(errmsg, " terminated by user.");
                return('q');
              }
	    } /* end of "m" loop */
            baddr+=j;
	  } /* end of "l" loop */
          bpat=(~bpat); /* invert current pattern */
        } /* end of "k" loop */
        /*
         * Generate artificial errors by uncommenting the line below.
         * bpat=(~bpat);
         */
        for(k=0; k < 2; k++) { /* expected=observed? */
          baddr=(((unsigned char *) VMADDR)+(j*k));
          for(l=MAXBYTES/(j*(sizeof(unsigned char))*2); l != 0; l--) {
            for(m=j; m != 0; m--, baddr++) {
              if ( (bobs=(*baddr)) != bpat) {
                perrs++;
                c=byteeh(tstname, baddr, bpat, bobs);
              }
              if ( (c=maygetchar()) > -1) { /* user hit a key to stop test */
                strcpy(errmsg, tstname);
                strcat(errmsg, " terminated by user.");
                return('q');
              }
	    } /* end of "m" loop */
            baddr+=j;
	  } /* end of "l" loop */
          bpat=(~bpat); /* invert current pattern */
        } /* end of "k" loop */
      } /* end of "j" loop */
    } else { /* word mode */
      for(j=1; j < MAXBYTES/(sizeof(unsigned short)); j*=2) {
        for(k=0; k < 2; k++) { /* fill video memory */
          waddr=(((unsigned short *) VMADDR)+(j*k));
          for(l=MAXBYTES/(j*(sizeof(unsigned short))*2); l != 0; l--) {
            for(m=j; m != 0; m--, waddr++) {
              *waddr=wpat;
              if ( (c=maygetchar()) > -1) { /* user hit a key to stop test */
                strcpy(errmsg, tstname);
                strcat(errmsg, " terminated by user.");
                return('q');
              }
	    } /* end of "m" loop */
            waddr+=j;
	  } /* end of "l" loop */
          wpat=(~wpat); /* invert current pattern */
        } /* end of "k" loop */
        /*
         * Generate artificial errors by uncommenting the line below.
         * wpat=(~wpat);
         */
        for(k=0; k < 2; k++) { /* expected=observed? */
          waddr=(((unsigned short *) VMADDR)+(j*k));
          for(l=MAXBYTES/(j*(sizeof(unsigned short))*2); l != 0; l--) {
            for(m=j; m != 0; m--, waddr++) {
              if ( (wobs=(*waddr)) != wpat) {
                perrs++;
                c=byteeh(tstname, waddr, wpat, wobs);
              }
              if ( (c=maygetchar()) > -1) { /* user hit a key to stop test */
                strcpy(errmsg, tstname);
                strcat(errmsg, " terminated by user.");
                return('q');
              }
	    } /* end of "m" loop */
            waddr+=j;
	  } /* end of "l" loop */
          wpat=(~wpat); /* invert current pattern */
        } /* end of "k" loop */
      } /* end of "j" loop */
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


