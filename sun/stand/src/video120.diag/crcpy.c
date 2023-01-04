static char	sccsid[] = "@(#)crcpy.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "crcpy" (control register copy test) performs the 
 * "Copy Enable/Disable Test".  The "Copy Enable/Disable Test" will determine 
 * whether or not the read-modify-write cycles to the main memory shadow buffer
 * also affect the video memory given the proper conditions.  That is, whether
 * or not the contents of the main memory shadow buffer are written to the 
 * video memory and, subsequently, are displayed on the screen given the proper
 * conditions.
 */
crcpy()
{
  extern end;
  extern char errmsg[]; /* defined in .../stand/src/video120.diag/mm.c */
  extern long prterrs;  /* defined in .../stand/src/video120.diag/mm.c */

  int  c=0;
  long i=0;
  long count=1;
  long start=0;
  long xstart=0;
  long end=0xffff; /* 65635 */
  long incrmnt=1;
  long perrs=0;
  long nbrwords=0;
  char *tstname="Copy Enable/Disable Test";
  unsigned long endofx=((unsigned long) 
    ((((unsigned long) &end)+0x1000+0x20000-1)&(~(0x20000 - 1))));
  unsigned short *shadbuf=((unsigned short *) endofx);
  unsigned short *craddr=((unsigned short *) CRADDR);
  unsigned short *waddr=((unsigned short *) VMADDR);
  unsigned short obs=0;
  unsigned short mask14=040000; /* bit 14 turns on copy mode */
  unsigned short mask1_6=0177601; /* bits 1-6 hold copy mode base address */

  if (prterrs == TRUE) {
    printf("\n%s\n", tstname);
  }
  /*
   * Read user-specified parameter values and determine whether or not the 
   * values are "legal".
   */
  start=eattoken(0x0, 0x0, 0xffff, 16);
  if (inrange(start, 0x0000, 0xffff) == FALSE) {
    strcpy(errmsg, "'beg_val' out of range.  Values 0x0000-0xffff are okay.");
    return('q');
  }
  xstart=start;
  end=eattoken(0xffff, 0xffff, 0xffff, 16);
  if (inrange(end, 0x0000, 0xffff) == FALSE) {
    strcpy(errmsg, "'end_val' out of range.  Values 0x0000-0xffff are okay.");
    return('q');
  }
  if (start > end) {
    strcpy(errmsg, "'beg_val' cannot be greater than 'end_val'.");
    return('q');
  }
  incrmnt=eattoken(0x1, 0x1, 0x10000, 16);
  if (inrange(incrmnt, 0x0000, 0xffff) == FALSE) {
    strcpy(errmsg, "'inc' out of range.  Values 0x0000-0xffff are okay.");
    return('q');
  }
  count=eattoken(0x1, 0x1, INFINITY, 10);
  if (inrange(count, 0x1, INFINITY) == FALSE) {
    strcpy(errmsg, "'count' out of range.");
    return('q');
  }
  /*
   * Perform the "Copy Enable/Disable Test" 'count' times or forever.
   */
  for(i=0; i < count; i++) {
    shadbuf=((unsigned short *) endofx);
    /* 
     * Set bit 14 and the copy-mode-base-address bits (bits 01-06) of the
     * control register.
     */
    *craddr=((unsigned short) (((endofx>>16)&(077)) | 
             ((*craddr&mask1_6) | mask14)));
    start=xstart-incrmnt;
    /*
     * Fill shadow buffer.
     */
    for(nbrwords=0; nbrwords < MAXWORDS; nbrwords++, shadbuf++) {
      if ( (start+incrmnt) > end) {
        start=xstart;
      } else {
        start+=incrmnt;
      }
      *shadbuf=((unsigned short) start); /* write value to shadow buffer. */
    }
    *craddr&=(~mask14); /* clear bit 14; turn copy mode off */
    shadbuf=((unsigned short *) endofx);
    waddr=((unsigned short *) VMADDR);
    start=xstart-incrmnt;
    /*
     * Generate artificial errors by uncommenting the line below.
     * start+=1;
     */
    /*
     * Check values in video memory and shadow buffer.
     */
    for(nbrwords=0; nbrwords < MAXWORDS; nbrwords++, shadbuf++, waddr++) {
      if ( (start+incrmnt) > end) {
        start=xstart;
      } else {
        start+=incrmnt;
      }
      if ( (obs=(*waddr)) != ((unsigned short) start) ) { /* check video */
        perrs++;                                          /* memory      */
        c=wordeh(tstname, waddr, ((unsigned short) start), obs);
      }
      if ( (c == 'q') || ((c != 'c') && (c=maygetchar()) > -1) ) {
        strcpy(errmsg, tstname);
        strcat(errmsg, " terminated by user.");
        return('q');
      }
      if ( (obs=(*shadbuf)) != ((unsigned short) start) ) { /* check shadow */
        perrs++;                                            /* buffer       */
        c=wordeh(tstname, shadbuf, ((unsigned short) start), obs);
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
    if (i >= INFINITY) { /* Since the test is to     */
      i=(-1);            /* run forever, reset 'i'.  */
    }
  }
  return(0);
}


