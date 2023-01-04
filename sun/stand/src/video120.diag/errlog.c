static char	sccsid[] = "@(#)errlog.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "errlog" (error log) displays all of the error messages which
 * have been logged thus far.
 */
errlog()
{
  extern char errmsg[]; /* defined in .../stand/src/video120.diag/mm.c */
  extern long prterrs;  /* defined in .../stand/src/video120.diag/mm.c */
  extern long nwrders;  /* defined in .../stand/src/video120.diag/mm.c */
  extern long nbyters;  /* defined in .../stand/src/video120.diag/mm.c */
  extern long nsccers;  /* defined in .../stand/src/video120.diag/mm.c */
  extern BYTNODE byteers[MAXERRS]; /* defined in 
                                      .../stand/src/video120.diag/mm.c */
  extern WRDNODE worders[MAXERRS]; /* defined in 
                                      .../stand/src/video120.diag/mm.c */
  extern SCCNODE sccers[MAXERRS];  /* defined in 
                                      .../stand/src/video120.diag/mm.c */
 
  long i=0;
  long count=0;
  char *tstname="Error Log Display";

  if (prterrs == TRUE) {
    printf("\n%s\n", tstname);
  }
  printf("\nSuspend the error log display by pressing any character.\n");
  printf("\nRestart the error log display by pressing any character.\n");
  printf("\n\nTerminate the error log display by pressing 'q'.\n");
  delay(5000);
  if (pause() == 'q') {
    strcpy(errmsg, tstname);
    strcat(errmsg, " terminated by user.");
    return('q');
  }
  /*
   * byte memory errors 
   */
  if (nbyters > MAXERRS) {
    count=MAXERRS;
    printf("\nOnly the first %d byte memory erros will be displayed\n", count);
  } else {
    count=nbyters;
  }
  if (count == 0) {
    printf("\nThere were no byte memory errors.\n");
    delay(5000);
    if (pause() == 'q') {
      strcpy(errmsg, tstname);
      strcat(errmsg, " terminated by user.");
      return('q');
    }
  } else {
    for(i=1; i < count; i++) {
      printf("\n%s failed at address 0x%x.\n", byteers[i].tstname,
              byteers[i].addr);
      printf("  Expected=0x%x  Observed=0x%x  XOR=0x%x\n", 
              byteers[i].exp, byteers[i].obs, 
              (byteers[i].exp^byteers[i].obs));
      if (pause() == 'q') {
        strcpy(errmsg, tstname);
        strcat(errmsg, " terminated by user.");
        return('q');
      }
    }
  }
  /*
   * word memory errors 
   */
  if (nwrders > MAXERRS) {
    count=MAXERRS;
    printf("\nOnly the first %d word memory erros will be displayed\n", count);
  } else {
    count=nwrders;
  }
  if (count == 0) {
    printf("\nThere were no word memory errors.\n");
    delay(5000);
    if (pause() == 'q') {
      strcpy(errmsg, tstname);
      strcat(errmsg, " terminated by user.");
      return('q');
    }
  } else {
    for(i=1; i < count; i++) {
      printf("\n%s failed at address 0x%x.\n", worders[i].tstname,
              worders[i].addr);
      printf("  Expected=0x%x  Observed=0x%x  XOR=0x%x\n", 
              worders[i].exp, worders[i].obs, 
              (worders[i].exp^worders[i].obs));
      if (pause() == 'q') {
        strcpy(errmsg, tstname);
        strcat(errmsg, " terminated by user.");
        return('q');
      }
    }
  }
  /*
   * serial communication controller errors 
   */
  if (nsccers > MAXERRS) {
    count=MAXERRS;
    printf("\nOnly the first %d scc erros will be displayed\n", count);
  } else {
    count=nsccers;
  }
  if (count == 0) {
    printf("\nThere were no scc errors.\n");
    delay(5000);
    if (pause() == 'q') {
      strcpy(errmsg, tstname);
      strcat(errmsg, " terminated by user.");
      return('q');
    }
  } else {
    for(i=1; i < count; i++) {
      printf("\n%s failed.\n", sccers[i].tstname);
      printf("  Expected=0x%x  Observed=0x%x  XOR=0x%x\n", 
              sccers[i].exp, sccers[i].obs, 
              (sccers[i].exp^sccers[i].obs));
      if (pause() == 'q') {
        strcpy(errmsg, tstname);
        strcat(errmsg, " terminated by user.");
        return('q');
      }
    }
  }
  delay(5000);
  if (pause() == 'q') {
    strcpy(errmsg, tstname);
    strcat(errmsg, " terminated by user.");
    return('q');
  } else {
    return(0);
  }
}


