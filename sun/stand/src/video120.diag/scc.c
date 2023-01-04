static char	sccsid[] = "@(#)scc.c 1.1 9/25/86 Copyright Sun Microsystems";

/*
 * Don't change the order of the include files!
 */
#include <sys/types.h>
#include <reentrant.h>
#include <machdep.h>
#include <s2addrs.h>
#include <sunromvec.h>
#include <token.h>
#include <zsreg.h>
#include "video120.h"

/*
 * Scc initialization sequence for testing purposes.
 */
unsigned char scctest[]={
   9, ZSWR9_RESET_WORLD,
   4, ZSWR4_PARITY_EVEN| ZSWR4_1_STOP| ZSWR4_X16_CLK,
   3, ZSWR3_RX_8,
   5, ZSWR5_RTS| ZSWR5_TX_8| ZSWR5_DTR,
   9, ZSWR9_NO_VECTOR,
  11, ZSWR11_TRXC_XMIT| ZSWR11_TRXC_OUT_ENA| ZSWR11_TXCLK_BAUD| 
      ZSWR11_RXCLK_BAUD,
  12, LOBAUD(9600),
  13, HIBAUD(9600),
  14, ZSWR14_BAUD_FROM_PCLK| ZSWR14_LOCAL_LOOPBACK,
   3, ZSWR3_RX_8| ZSWR3_RX_ENABLE,
   5, ZSWR5_RTS| ZSWR5_TX_ENABLE| ZSWR5_TX_8| ZSWR5_DTR,
  14, ZSWR14_BAUD_ENA| ZSWR14_BAUD_FROM_PCLK| ZSWR14_LOCAL_LOOPBACK,
   0, ZSWR0_RESET_STATUS| ZSWR0_RESET_ERRORS, 
   0, ZSWR0_RESET_STATUS| ZSWR0_RESET_ERRORS
};

/*
 * Scc initialization sequence for reset purposes.
 */
unsigned char sccreset[] = {
   0,  0,
   9,  ZSWR9_RESET_WORLD,
   0,  0,
   4,  ZSWR4_PARITY_EVEN| ZSWR4_1_STOP| ZSWR4_X16_CLK,
   3,  ZSWR3_RX_8,
   5,  ZSWR5_RTS| ZSWR5_TX_8| ZSWR5_DTR,
   9,  ZSWR9_NO_VECTOR,
  11,  ZSWR11_TRXC_XMIT| ZSWR11_TXCLK_BAUD| ZSWR11_RXCLK_BAUD,
  12,  LOBAUD(1200),
  13,  HIBAUD(1200),
  14,  ZSWR14_BAUD_FROM_PCLK,
   2,  EVEC_LEVEL6/4,
   3,  ZSWR3_RX_8| ZSWR3_RX_ENABLE,
   5,  ZSWR5_RTS| ZSWR5_TX_ENABLE| ZSWR5_TX_8| ZSWR5_DTR,
  14,  ZSWR14_BAUD_ENA| ZSWR14_BAUD_FROM_PCLK,
   0,  ZSWR0_RESET_STATUS,
   0,  ZSWR0_RESET_STATUS
};

static int (*savevec[8])(); /* place to store interrupt vectors during test */

/* 
 * Function "scc" (serial communications controller test) performs the
 * "Serial Communications Controller Test".
 */
scc()
{
  extern char errmsg[]; /* defined in .../stand/src/video120.diag/mm.c */
  extern long prterrs;  /* defined in .../stand/src/video120.diag/mm.c */
  extern long nsccers;  /* defined in .../stand/src/video120.diag/mm.c */

  long i=0;
  long count=0;
  long perrs=0;
  long chanl=1; /* 1=Channel A, 2=Channel B */
  long rcode=0; /* return code */
  unsigned long baud=0;
  unsigned long beg=0;
  unsigned long end=0;
  unsigned long incr=0;
  unsigned long mult=0;
  char *tstname="Serial Communication Controller Test";
  struct zscc_device *chA=((struct zscc_device *) SCCADDR)+1;
  struct zscc_device *chB=((struct zscc_device *) SCCADDR);
     
  if (prterrs == TRUE) {
    printf("\n%s\n", tstname);
  }
  /*
   * Read user-specified parameter values and determine whether or not the 
   * values are "legal".
   *
   * The second parameter of eattoken is the "default" baud rate.  A "default"
   * baud rate of "0" implies that the SCC test is to be run with baud rates
   * of 300 and 9600.  The third parameter of eattoken is the "forever" baud
   * rate.  A "forever" baud rate of "1" implies that the SCC test is to be
   * run with all of the following baud rates: 300, 600, 1200, 2400, 4800, 
   * 9600, 19200, 38400 and 76800. 
   */
  baud=eattoken(300, 0, 1, 10);
  if ( (baud != 0) && (baud != 1) && (okbaud(baud) != TRUE) ) {
    strcpy(errmsg, "Illegal value specified for 'baudrate'.");
    return('q');
  }
  count=eattoken(0x1, 0x1, INFINITY, 10);
  if (inrange(count, 0x1, INFINITY) == FALSE) {
    strcpy(errmsg, "'count' out of range.");
    return('q');
  }
  map(SCCADDR, PAGESIZE, 0x780000, PM_MEM);
  redefint(); /* redefine interrupt vectors during test */
  /*
   * Perform the "Serial Communications Controller Test" 'count' times or
   * forever.
   */
  for(i=0; i < count; i++) {
    /*
     * Test channel A and B with one or more baud rates.
     */
    for(chanl=1; chanl <= 2; chanl++) {
      if (baud == 0) { /* run test at 300 and 9600 baud */
        beg=300;
        end=9600;
        incr=9300;
        mult=1;
      } else if (baud == 1) { /* run the test at "all" baud rates */
        beg=300;
        end=MAXBAUD;
        incr=0;
        mult=2;
      } else { /* run test at default or user-specified baud rate */
        beg=baud;
        end=baud;
        incr=300;
        mult=1;
      }
      for(; beg <= end; beg=((beg+incr)*mult)) {
        sccinit(beg, ((chanl == 1) ? chA : chB)); /* initialize scc */
        rcode=sccloop(((chanl == 1) ? chA : chB), perrs); /* test scc */
        printf("\nSCC internal loop back test");
        printf(" %s when testing", ((rcode == 0) ? "WORKED" : "FAILED"));
        printf(" channel %c at baud rate %u.\n",((chanl==1)?'A' : 'B'), beg);
      }
    }
    resetscc(); /* reset scc after testing       */
    resetkb();  /* wake up the keyoard           */
    restrint(); /* restore old interrupt vectors */
    printf("\nPass %d of %s.\n", i+1, tstname);
    printf("Total of %d serial communication controller data errors.\n", 
            perrs);
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


/*
 * Function "okbaud" (okay baud) returns TRUE (1) if 'baud' is a legal
 * baud rate; otherwise, FALSE (0) is returned.
 */
okbaud(baud)
  unsigned long baud;
{

  return( ((baud ==   300) || (baud ==   600) || (baud ==  1200) || 
           (baud ==  2400) || (baud ==  4800) || (baud ==  9600) || 
           (baud == 19200) || (baud == 38400) || (baud == 76800)) 
           ? TRUE : FALSE); 
}


/*
 * Function "sccinit" (serial communication controller initialization)
 * changes the current baud rate before initializing the scc chip for
 * testing purposes.
 */
sccinit(baud, chptr)
  unsigned long baud;
  struct zscc_device *chptr;
{
  long i=0;

  scctest[BAUDHI]=HIBAUD(baud); /* set the   */
  scctest[BAUDLO]=LOBAUD(baud); /* baud rate */
  for (i=0; i < sizeof(scctest); i++) {   /* for channel    */
    chptr->zscc_control=scctest[i];       /* initialization */
    sccdelay(10);
  }
}


/* 
 * Function "sccdelay" (serial communication controller delay)
 * is a primitive routine for generating a delay.
 */
sccdelay(n)
  long n;
{

  if (n != 0) {
    while (--n != 0) {
      ;
    }
  }
}


/*
 * Function "sccloop" (serial communication controller loop) writes
 * each value from 0x00 through 0xff to the specified channel at the
 * current baud rate and reads back the value from the same channel
 * before comparing the expected and observed values.
 */
sccloop(chptr, perrs)
  struct zscc_device *chptr; /* channel pointer */
  long perrs;
{
  long   i=0;
  long   j=0;
  unsigned char exp=0;
  unsigned char obs=0;

  for (i=0; i <= 0xff; i++) { /* test with all possible values */
    /*
     * Don't write 0x2 to scc chip because, it rings the bell in the keyboard.
     */
    if (i != 2) {
      exp=((unsigned char) i);
      /*
       * Wait until transmitter is ready or until maximum wait time has passed.
       */
      for(j=WAIT_TIME; ((j>0) && (!(chptr->zscc_control&ZSRR0_TX_READY))); 
          j--) {
        sccdelay(10);
      }
      if (j <= 0) {
        printf("\nTransmitter was not ready within the allowable time.\n");
        return(-1);
      }
      chptr->zscc_data=exp; /* transmitter ready, write data */
      /*
       * Wait until receiver is ready or until maximum wait time has passed.
       */
      for(j=WAIT_TIME; ((j>0) && (!(chptr->zscc_control&ZSRR0_RX_READY))); 
          j--) {
        sccdelay(10);
      }
      if (j <= 0) {
        printf("\nReceiver was not ready within the allowable time.\n");
        return(-1);
      }
      /* 
       * Check if the expected value is equal to the observed.
       */	
      if ( (obs=chptr->zscc_data) != exp) {
        ++perrs;
        scceh("Serial Communication Controller", exp, obs);
      } 
    }
  }
  return(0);
}


/*
 * Function "scceh" (serial communication controller error handler) 
 * handles serial communication controller errors.
 */
scceh(tstname, exp, obs)
  char          *tstname;
  unsigned char exp;
  unsigned char obs;
{
  extern char errmsg[];     /* defined in .../stand/src/video120.diag/mm.c */
  extern long prterrs;      /* defined in .../stand/src/video120.diag/mm.c */
  extern SCCNODE sccers[];  /* defined in .../stand/src/video120.diag/mm.c */

  int c=0;

  if (prterrs == TRUE) {
    printf("\n%s failed.\n", tstname);
    printf("  Expected=0x%x  Observed=0x%x  XOR=0x%x\n", 
            exp, obs, (exp^obs));
  }
  if (++nsccers < MAXERRS) { /* store scc error if space available */
    strcpy(sccers[nsccers].tstname, tstname);
    sccers[nsccers].exp=exp;
    sccers[nsccers].obs=obs;
  } else {
    strcpy(errmsg, "Over 100 scc errors have been found already.");
  }
  return(0);
}


/*
 * Function "redefint" (redefine interrupt vectors) redefines the interrupt
 * vectors during testing.
 */
redefint()
{
  extern int dumyint();

  long i=0;

  for (i=1; i < 8; i++){
    savevec[i]=ex_vector->e_int[i]; /* save old vector   */
    ex_vector->e_int[i]=dumyint;    /* set up new vector */
  }
}

/*
 * Function "dumyint" (dummy interrupt routine) takes the place of
 * the original interrupt routines during testing.
 */
reentrant(dumyint)
{

}


/*
 * Function "resetscc" (reset serial communication controller) resets the
 * scc chip after testing has been completed.
 */
resetscc()
{
  struct zscc_device *chA=((struct zscc_device *) SCCADDR)+1;
  struct zscc_device *chB=((struct zscc_device *) SCCADDR);
  long i=0;

  for (i=0; i < sizeof(sccreset); i++) {  /* for channel */
    chA->zscc_control=sccreset[i];        /* A reset     */
    sccdelay(10);
  }
  for (i=4; i < sizeof(sccreset); i++) {  /* for channel */
    chB->zscc_control=sccreset[i];        /* B reset     */
    sccdelay(10);
  }
}

/*
 * Function "resetkb" (reset keyboard) wakes up the keyboard following testing.
 */
resetkb()
{
  struct zscc_device *chA=((struct zscc_device *) SCCADDR)+1;

  chA->zscc_data=KBD_CMD_RESET;
  sccdelay(10);
}

/*
 * Function "restrint" (restore interrupt vectors) restores the original
 * interrupt vectors following the test.
 */
restrint()
{
  long  i=0;

  for (i=1; i < 8; i++) { /* restore old interrupt vectors */
    ex_vector->e_int[i]=savevec[i];
  }
}
