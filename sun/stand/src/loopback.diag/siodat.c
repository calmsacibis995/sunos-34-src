/******************************************************************************
*
*	Test data path for named serial ports (source and receive)
*
*	synopsis	error = siodat(source, receive)
*			error = error status or 0 if OK
*			source = full device name as it appears in /dev
*			receive = full device name as it appears in /dev
*
*	Siodat writes data to source port and then reads it from the receive
*	port for verification.  The entire byte range (0 - FFH) is written
*	and read.  Test stops on first error.  Status is returned.
*
*/
#ifndef lint
static  char sccsid[] = "@(#)siodat.c 1.1 86/09/25 SMI"; 
#endif
 
#include <file.h>
#include <stdio.h>
#include <ioctl.h>
#include <sgtty.h>
#include <signal.h>

#define TSTOK		0		/* tested OK! */
#define NOTRDY		1		/* device not ready */
#define DATERR		2		/* data error */
#define NOTFND		3		/* device name not found */
#define INTERR		4		/* user interrupt */
#define PATBEG		0x00		/* start of test pattern */
#define PATEND		0xff		/* end of test pattern */

static int srcfd,rcvfd;			/* file decriptor for port */
static int swapflag;			/* port definition swap flag */
static short srcsav,rcvsav;		/* save existing tty mode */
static int error;			/* error flag on exit */

static struct sgttyb srclist;		/* list of saved port parameters */
static struct sgttyb rcvlist;		/* list of saved port parameters */

int	finish();			/* interrupt handler */

siodat(srcdev,rcvdev)
unsigned char *srcdev,*rcvdev;		/* device names */
{
  int i;				/* general purpose loop counter */
  long count;				/* character count for port read */
  unsigned char c,*ptr;			/* general character and pointer */
  unsigned char pattern[256];		/* pattern written to port */
  unsigned char *devnam;		/* name of port under test */

/*
*	Provide for user interrupt (^C)
*/

  swapflag = 0;				/* clear swap flag */
  signal(SIGHUP,finish);		/* interrupt, hangup */
  signal(SIGTERM,finish);		/* interrupt, software termination */
  signal(SIGINT,finish);		/* interrupt */

/*
*	Open requested port
*/

  for (;;) {				/* provide outside master loop */
    devnam = srcdev;			/* point at attempted device name */
    if ((srcfd = open(srcdev,O_RDWR,7)) == -1) {
      error = NOTFND;			/* set error status */
      break;				/* report and exit */
    }
    devnam = rcvdev;			/* point at attempted device name */
    if (!strcmp(srcdev,rcvdev)) {
      rcvfd = srcfd;			/* source and receive same, set */
    					/* ptrs the same for loopback */
    }
    else if ((rcvfd= open(rcvdev,O_RDWR,7)) == -1) {
					/* different, open receive port too */
      error = NOTFND;			/* set error status */
      break;				/* report and exit */
    }
    devnam = srcdev;			/* use source name for error msgs */

/*
*	Adjust sio port parameters
*/
    
    ioctl(srcfd,TIOCGETP,&srclist);	/* get current port parameters */
    ioctl(srcfd,TIOCEXCL,0);		/* get EXCLUSIVE use of port */
    swapflag = 1;			/* mark port exclusive */
    srcsav = srclist.sg_flags;		/* save existing tty mode */
    srclist.sg_flags = RAW + ANYP;	/* RAW, instant input */
					/* no parity */
					/* no echo */ 
    ioctl(srcfd,TIOCSETP,&srclist);	/* set new port parameters */
    swapflag = 2;			/* flag source port swapped */

    ioctl(rcvfd,TIOCGETP,&rcvlist);	/* get current port parameters */
    ioctl(rcvfd,TIOCEXCL,0);		/* get EXCLUSIVE use of port */
    swapflag = 3;			/* mark port exclusive */
    rcvsav = rcvlist.sg_flags;		/* save existing tty mode */
    rcvlist.sg_flags = RAW + ANYP;	/* RAW, instant input */
					/* no parity */
					/* no echo */ 
    ioctl(rcvfd,TIOCSETP,&rcvlist);	/* set new port parameters */
    swapflag = 4;			/* flag receiver port swapped */

/*
*	Set up for test
*/
    printf("Testing...\n");
    error = INTERR;			/* flag for possible user interrupt */
    strcpy (pattern,"!");		/* initialize pattern buffer */
  
/*
*	Clean input and output of any garbage
*/

    for(i=0; i<1000; i++) {
      ioctl(rcvfd,FIONREAD,&count);	/* check for garbage */
      read(rcvfd,pattern,(int) count);	/* clean it off */
    }

/*
*	Write data pattern to port
*/
  
    for (c = PATBEG; c < PATEND; c++) {	/* rotate through pattern */
      pattern[0] = c;			/* store c in buffer */
      write(srcfd,pattern,1);		/* write buffer to device */
      pattern[0] = '\0';		/* clear pattern */
      for (i=0; i<1000; i++) {
        ioctl(rcvfd,FIONREAD,&count);	/* wait for data to arrive */
        if (count != 0) break;		/* got it, read it in */
      }
      if (count == 0) {
        error = NOTRDY;			/* no response, flag error */
        break;				/* exit pattern loop */
      }
      read(rcvfd,pattern,1);		/* get back data */
      if (c != pattern[0]) {
        error = DATERR;			/* wrong data, flag error */
        break;				/* exit pattern loop */
      }
    }					/* loop through each data pattern */
    if (error == INTERR) error = TSTOK;	/* set successful status */
    break;				/* done, report test */
  }

/*
*	Display test results
*/

  printf("Device \"%s\" ",devnam);	/* display status hdr w/device name */
  switch (error) {
    case TSTOK:				/* tested OK */
      printf("tested OK.");
      break;
    case NOTRDY:
      printf("does not respond.  Check loopback connector.");
      break;
    case DATERR:
      printf("data does not match.  Wrote: %xH  Read: %xH  Xor: %xH", c, pattern[0], pattern[0]);
      break;
    case NOTFND:
      printf("not found.  Check \"/dev\" directory.");
      break;



  }
  printf("\n");				/* clear after message */
  finish();				/* clean up ports and close devices */
  return(error);			/* return status to caller */
}

/*
*	Clean up ports and exit (on interrupt)
*/

finish()
{
  if (swapflag > 2) {
    ioctl(rcvfd,TIOCNXCL,0);		/* release EXCLUSIVE use of port */
  }
  if (swapflag > 3) {
    rcvlist.sg_flags = rcvsav;		/* restore original tty mode */
    ioctl(rcvfd,TIOCSETP,&rcvlist);	/* restore old port parameters */
  }
  close(rcvfd);				/* close device */
  if (swapflag > 0) {
    ioctl(srcfd,TIOCNXCL,0);		/* release EXCLUSIVE use of port */
  }
  if (swapflag > 1) {
    srclist.sg_flags = srcsav;		/* restore original tty mode */
    ioctl(srcfd,TIOCSETP,&srclist);	/* restore old port parameters */
  }
  close(srcfd);				/* close device */
  if (error == INTERR) {
    printf("\nCancelled\n");
    exit(error);	/* user interrupt, just exit */
  }
  else return;
}
