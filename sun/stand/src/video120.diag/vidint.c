static char	sccsid[] = "@(#)vidint.c 1.1 9/25/86 Copyright Sun Microsystems";

/*
 * Don't change the order of the include files!
 */
#include <sys/types.h>
#include <machdep.h>
#include <sunromvec.h>
#include <s2addrs.h>
#include <token.h>
#include <reentrant.h>
#include "video120.h"

/* 
 * Function "vidint" (video interrupt) performs several checks to ensure
 * that the video board interrupt mechanism is working properly.
 */
reentrant(vidint)
{
  extern char errmsg[]; /* defined in .../stand/src/video120.diag/mm.c */
  extern long intrupt;  /* defined in .../stand/src/video120.diag/mm.c */

  unsigned short *craddr=((unsigned short *) CRADDR);
  unsigned short mask13=020000; /* bit 13 generates a video board interrupt */

  /*
   * Under normal conditions, bit 12 of the control register mimics the
   * behavior of bit 13 of the control register.
   */
  if ( ((*craddr>>12)&01) == 0) { /* is bit 12 set? */
    printf("\nError: Bit 12 of the control register should equal '1', ");
    printf("but it equals '0'.\n");
  }
  *craddr&=(~mask13); /* clear bit 13; disable video board interrupt*/
  if ( ((*craddr>>13)&01) == 1) { /* is bit 13 clear? */
    printf("\nError: Bit 13 of the control register should equal '0', ");
    printf("but it equals '1'.\n");
  }
  if ( ((*craddr>>12)&01) == 1) { /* is bit 12 clear? */
    printf("\nError: Bit 12 of the control register should equal '0', ");
    printf("but it equals '1'.\n");
  }
  intrupt=1; /* flag indicates that interrupt routine was executed */
  return(0);
}


