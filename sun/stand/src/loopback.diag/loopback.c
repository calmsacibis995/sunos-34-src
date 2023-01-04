/******************************************************************************
*
*	name		"loopback.c"
*
*	synopsis	(cmd line) "loopback <source> <receiver>"
*
*			source	=>	full name of source device
*					e.g. "/dev/ttya" or "/dev/tty00-07"
*
*			receiver =>	full name of receiver device
*					(in the case of paired loopback cable)
*					e.g. "/dev/ttyb" or "/dev/tty07,02,04"
*
*	description	Writes data (0-FFh) to the source device
*			and then reads it back from the receiver device
*			verifying the data after each byte sent.
*			Status is returned in the error byte, 0 being
*			successful and non-zero being an error status (see
*			siodat.c)
*			If no receiver is given, it is assumed to be the 
*			same as the source.  If a range is given, a table
*			of device names is constructed.  The same applies to
*			a list.  Construction  uses the full root, replacing
*			each instance with the number of bytes given in the
*			range or list.  Example:
*
*				"/dev/tty00-3"
*
*			expands to:
*
*				/dev/tty00
*				/dev/tty01
*				/dev/tty02
*				/dev/tty03
*
*			and
*				"/dev/tty00,2,7,11"
*
*			expands to:
*
*				/dev/tty00
*				/dev/tty02
*				/dev/tty07
*				/dev/tty11
*
*			 
*			In addition, some special keys are provided:
*
*				single8	(first 8 systech ports)
*				single14(first 14 systech ports)
*				double8 (same as single8, but double port loop)
*				double14(same as single14, double port loop)
*				all (all 16 systech ports, single loopback)
*				pairs (all 16 systech ports, double loop)
*
*/

#ifndef lint
static	char sccsid[] = "@(#)loopback.c 1.1 86/09/25 SMI";
#endif

#include <file.h>
#include <stdio.h>
#include <ioctl.h>
#include <sgtty.h>

extern int siodat();			/* data test for serial ports */
extern char *expars();			/* command line expand, parse */

struct pkglst {
  char	*key;				/* keyword to select device list */
  char  *src;				/* lst of src devices */
  char  *rcv;				/* lst of receiver devices */
} pkg[] = {

  "single8","/dev/tty00-7","/dev/tty00-7",
  "single14","/dev/tty00-9,a-d","/dev/tty00-9,a-d",
  "double8","/dev/tty00,1,3,5","/dev/tty07,2,4,6",
  "double14","/dev/tty00,1,3,5,8,9,b","/dev/tty07,2,4,6,d,a,c",
  "all","/dev/tty00-9,a-f","/dev/tty00-9,a-f",
  "pairs","/dev/tty00,1,3,5,8,9,b,d","/dev/tty07,2,4,6,f,a,c,e",
  "\0","\0","\0"
};

main(argc,argv)
  int argc;
  char *argv[];

{
  int i;				/* general purpose loop variable */
  int error;				/* error flag on exit */
  int status;				/* intermediate error status */
  unsigned char c,*ptr;			/* general character and pointer */
  unsigned char *srcnam,*rcvnam;	/* source and receiver device names */
  unsigned char srcbuf[1024];		/* expansion buffer for source names */
  unsigned char rcvbuf[1024];		/* expansion buffer for rcvr names */

 
/*
*	Tell 'em who you are
*/

  printf("\n\"LOOPBACK\" SERIAL PORT DATA PATH TEST     VER 1.04\n\n\n");

/*
*	Get device name from user
*/

  if (argc == 1) {			/* Any device specified? */
    printf("Usage:  \"loopback <source device> <receiving device>\"\n\n");
					/* No device, just exit */
    exit(0);
  }

  if (argc > 1) {			/* Source device specified? */
    expars(argv[1],srcbuf);		/* Yes, get source device(s) list*/
  }

  if (argc > 2) {			/* Receiver device specified? */
    expars(argv[2],rcvbuf);		/* Yes, get receiver device(s) list*/
  }
  else expars(argv[1],rcvbuf);		/* No, copy source list as receive */

/*
*	Adjust "key" device names to real device names
*/

  for(i=0; *(pkg[i].key) != '\0';i++) {	/* search for key words */
    if (!strcmp(argv[1],pkg[i].key)) {	/* found a match, expand as dev list */
      expars(pkg[i].src,srcbuf);	/* Yes, get source device(s) list*/
      expars(pkg[i].rcv,rcvbuf);	/* Yes, get receiver device(s) list*/
      break;				/* Match found, don't search more */
    }
  }

/*
*	Test each set of serial ports with data (0-FFH)
*/

  srcnam = srcbuf;			/* point at start of source devices */
  rcvnam = rcvbuf;			/* point at start of receive devices */
  while ((*srcnam != '\0') && (*rcvnam != '\0')) {
    status = siodat(srcnam,rcvnam);	/* test named port */
    error |= status;			/* "or" in all error statuses */
    if (strcmp(srcnam,rcvnam)) {	/* if testing two different ports, */
					/* then reverse the direction */
					/* to test input and output for both */
      status = siodat(rcvnam,srcnam);	/* test named port (reverse order) */
      error |= status;			/* "or" in all error statuses */
    }
    while(*srcnam++ != '\0');		/* advance to next source name */
    while(*rcvnam++ != '\0');		/* advance to next receiver name */
  }
  exit(error);				/* exit with status */
}
