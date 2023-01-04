static char	sccsid[] = "@(#)wsloop.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "wsloop" (word scopeloop) continues to write a value to a word,
 * read back the word's value and compare the value read with an expected 
 * value at a particular word address until the user presses 'q' (quit) or
 * 'n' (next address).  If the stop-on-error-flag is also set ("errmode" is
 * equal to 3), function "wsloop" also waits for for the user to press any
 * key after each write-read-compare cycle.
 */
wsloop(waddr, wexp, wobs)
  unsigned short *waddr;
  unsigned short wexp;
  unsigned short wobs;
{
  extern long errmode;  /* defined in .../stand/src/video120.diag/mm.c */
  extern long testcrm;  /* defined in .../stand/src/video120.diag/mm.c */

  int  c=0;
  unsigned short mask=0176; /* only bits 1-6 of control register are tested */

  printf("\nEntering scopeloop\n");
  while( (c != 'q') && (c != 'n') ) {
    c=maygetchar();
    /* 
     * Special masking is required when testing the memorability of the 
     * control register.
     */
    if (testcrm == 1) { 
      *waddr=((unsigned short) (wexp | 0100000)); /* leave screen on */
      wobs=(*waddr&mask);
    } else {
      *waddr=wexp;
      wobs=(*waddr);
    }
    printf("  Address 0x%x,  Expected=0x%x  Observed=0x%x  XOR=0x%x\n", 
            waddr, wexp, wobs, (wexp^wobs));
    while( (errmode == 3) && ((c=maygetchar()) < 0) ) { /* scopeloop and */
        ;                                               /* wait on error */
    }
  }
  printf("Exited scopeloop\n");
  return(c);
}

