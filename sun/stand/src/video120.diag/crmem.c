static char	sccsid[] = "@(#)crmem.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "crmem" (control register memory test) performs the "Memory Test 
 * Of The Control Register". The "Memory Test Of The Control Register" writes 
 * values to bits 01-06 of the control register, reads back the contents of 
 * the register and compares the observed value with the expected value before
 * displaying a message indicating the success or failure of the test.
 */
crmem()
{
  extern char errmsg[]; /* defined in .../stand/src/video120.diag/mm.c */
  extern long prterrs;  /* defined in .../stand/src/video120.diag/mm.c */
  extern long testcrm;  /* defined in .../stand/src/video120.diag/mm.c */

  int  c=0;
  long i=0;
  long count=0;
  long start=0;
  long end=0x7f; /* 127 */
  long incrmnt=1;
  long perrs=0;
  char *tstname="Memory Test Of The Control Register";
  unsigned short *craddr=((unsigned short *) CRADDR);
  unsigned short crpat=0; /* default pattern */
  unsigned short crobs=0;
  unsigned short mask=0176; /* only bits 1-6 of control register are tested */

  if (prterrs == TRUE) {
    printf("\n%s\n", tstname);
  }
  /*
   * Read user-specified parameter values and determine whether or not the 
   * values are "legal".
   */
  start=eattoken(0x0, 0x0, 0x7f, 16);
  if (inrange(start, 0x0000, 0x7f) == FALSE) {
    strcpy(errmsg, "'beg_val' out of range.  Values 0x0000-0x7f are okay.");
    return('q');
  }
  end=eattoken(0x7f, 0x7f, 0x7f, 16);
  if (inrange(end, 0x0000, 0x7f) == FALSE) {
    strcpy(errmsg, "'end_val' out of range.  Values 0x0000-0x7f are okay.");
    return('q');
  }
  if (start > end) {
    strcpy(errmsg, "'beg_val' cannot be greater than 'end_val'.");
    return('q');
  }
  incrmnt=eattoken(0x1, 0x1, 0x7f, 16);
  if (inrange(incrmnt, 0x0000, 0x7f) == FALSE) {
    strcpy(errmsg, "'inc' out of range.  Values 0x0000-0x7f are okay.");
    return('q');
  }
  count=eattoken(0x1, 0x1, INFINITY, 10);
  if (inrange(count, 0x1, INFINITY) == FALSE) {
    strcpy(errmsg, "'count' out of range.");
    return('q');
  }
  /*
   * Perform the "Memory Test Of The Control Register" 'count' times or
   * forever.
   */
  for(i=0; i < count; i++) {
    printf("\nNotice: Only bits 01-06 of the control register will ");
    printf("be tested.\n");
    delay(5000);
    if (pause() == 'q') {
      strcpy(errmsg, tstname);
      strcat(errmsg, " terminated by user.");
      return('q');
    }
    for(; start <= end; start+=incrmnt) {
      crpat=((unsigned short) (start | 0100000)); /* leave screen on */
      *craddr=crpat;
      /*
       * Generate artificial errors by uncommenting the line below.
       * crpat=((~crpat) | 0100000);
       */
      if ( (crobs=(*craddr&mask)) != (crpat&mask) ) {
          perrs++;
          testcrm=1; /* testing control register memory */
          c=wordeh(tstname, craddr, (crpat&mask), crobs);
          testcrm=0;
      }
      if ( (c == 'q') || ((c != 'c') && (c=maygetchar()) > -1) ) {
        strcpy(errmsg, tstname);
        strcat(errmsg, " terminated by user.");
        return('q');
      }
    }
    printf("Pass %ld of %s.\n", i+1, tstname);
    printf("%ld data errors\n", perrs);
    delay(5000);
    if (pause() == 'q') {
      strcpy(errmsg, tstname);
      strcat(errmsg, " terminated by user.");
      return('q');
    }
    if (i >= INFINITY) { /* Since the test is to    */
      i=(-1);            /* run forever, reset 'i'. */
    }
  }
  return(0);
}




