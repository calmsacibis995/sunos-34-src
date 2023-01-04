static char	sccsid[] = "@(#)bsloop.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "bsloop" (byte scopeloop) continues to write a value to a byte,
 * read back the byte's value and compare the value read with an expected 
 * value at a particular byte address until the user presses 'q' (quit) or
 * 'n' (next address).  If the stop-on-error-flag is also set ("errmode" is
 * equal to 3), function "bsloop" also waits for for the user to press any
 * key after each write-read-compare cycle.
 */
bsloop(baddr, bexp, bobs)
  unsigned char *baddr;
  unsigned char bexp;
  unsigned char bobs;
{
  extern long errmode;  /* defined in .../stand/src/video120.diag/mm.c */

  int  c=0;

  printf("\nEntering scopeloop\n");
  while( (c != 'q') && (c != 'n') ) {
    c=maygetchar();
    *baddr=bexp;
    bobs=(*baddr);
    printf("  Address 0x%x,  Expected=0x%x  Observed=0x%x  XOR=0x%x\n", 
            baddr, bexp, bobs, (bexp^bobs));
    while( (errmode == 3) && ((c=maygetchar()) < 0) ) { /* scopeloop and */
        ;                                               /* wait on error */
    }
  }
  printf("Exited scopeloop\n");
  return(c);
}

