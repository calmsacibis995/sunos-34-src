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
*               		/dev/tty11
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
*                       "first.", "second.", "third.", or "fourth." may be
*                       inserted before any of the above keys to indicate
*                       which board to test. For example:
*
*                               first.single8  (first 8 ports of systech board 1, tty00-07)
*                               second.single8 (first 8 ports of systech board 2, tty10-17)
*                               third.single8  (first 8 ports of systech board 3, tty20-27)
*                               fourth.single8 (first 8 ports of systech board 4, tty30-37)
*/

#include <sys/types.h>
#include <sys/file.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sgtty.h>
#include <ctype.h>
#include <signal.h>

#define TESTED_OK	0		/* tested OK! */
#define INTERRUPT	20		/* user interrupt */
#define PATBEG		0x00		/* start of test pattern */
#define PATEND		0xff		/* end of test pattern */
#define RECEIVE_WAIT_TIME 200

#define FALSE                   0
#define TRUE                    ~FALSE
#define INFO                    0
#define WARNING                 1
#define FATAL                   2
#define ERROR                   3
#define NO_SD_LOG_DIR           1
#define NO_OPEN_LOG             2
#define NO_PORTS_SELECTED	3
#define NO_SD_PORTS_SELECTED	4
#define DEV_NOT_OPEN		5
#define WRITE_FAILED		6
#define READ_FAILED		7
#define NOT_READY		8
#define DATA_ERROR		9
#define END_ERROR               10
#define USAGE_ERROR             99
#define TEST_NAME               "sptest"
#define LOGFILE_NAME            "log.sysrtn.XXXXXX"

/*
*	Globals
*/

char device_name[15];
char *device = device_name;
int  atn = FALSE;
int sending_atn_msg = FALSE;
int  debug = FALSE;
int  pass = 0;
int errors = 0;
int verbose = TRUE;
int  stop_on_error = TRUE;

int failed = 0, logfd = 0;
char logfile_name[50];
char *logfile = logfile_name;
extern char *getenv();

int exec_by_sysdiag = FALSE;
int verify = FALSE;
int load_test = FALSE;
int simulate_error = 0;
char perror_msg_buffer[30];
char *perror_msg = perror_msg_buffer;
char tmp_msg_buffer[30];
char *tmp_msg = tmp_msg_buffer;
char msg_buffer[200];
char *msg = msg_buffer;
int  retry_cmp_error = FALSE;
int  display_read_data = FALSE;

static char     sccsid[] = "@(#)sptest.c 1.1 9/25/86 Copyright 1985 Sun Micro";

int	bleft;				/* # of bytes left in expand buffer */
char	*srcbuf;			/* address of source buffer */
char	*desbuf;			/* address of destination buffer */
char	*port_ptr;			/* address of device line */
char	*exbuf;				/* address of expand buffer */
char 	*root;				/* address of most recent root name */
unsigned char *srcnam,*rcvnam;		/* source and receiver device names */
unsigned char 	board;
unsigned char *devnam;			/* name of port under test */

int	overflow();			/* overflow checking routine */
int	exrange();			/* range expansion routine */
int	exlist();			/* list expansion routine */
int	strinc();			/* increment a string routine */

static int srcfd = 0, rcvfd = 0;	/* file decriptor for port */
static int swapflag;			/* port definition swap flag */
static short srcsav,rcvsav;		/* save existing tty mode */
static int return_code = 0;		/* return code on exit */

static struct sgttyb srclist;		/* list of saved port parameters */
static struct sgttyb rcvlist;		/* list of saved port parameters */

char serial_port_names[50];
char *serial_ports = serial_port_names;
char test_ports[50];
char *port_to_test = test_ports;
char i_string[2];
int  device_1 = FALSE;
int  device_2 = FALSE;
int  keyword = FALSE;
int  send_characters = 3000;
int  characters;
int  ports_to_test;
 
extern int siodat();			/* data test for serial ports */

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
  "first.single8","/dev/tty00-7","/dev/tty00-7",
  "first.single14","/dev/tty00-9,a-d","/dev/tty00-9,a-d",
  "first.double8","/dev/tty00,1,3,5","/dev/tty07,2,4,6",
  "first.double14","/dev/tty00,1,3,5,8,9,b","/dev/tty07,2,4,6,d,a,c",
  "first.all","/dev/tty00-9,a-f","/dev/tty00-9,a-f",
  "first.pairs","/dev/tty00,1,3,5,8,9,b,d","/dev/tty07,2,4,6,f,a,c,e",
  "second.single8","/dev/tty10-7","/dev/tty10-7",
  "second.single14","/dev/tty10-9,a-d","/dev/tty10-9,a-d",
  "second.double8","/dev/tty10,1,3,5","/dev/tty17,2,4,6",
  "second.double14","/dev/tty10,1,3,5,8,9,b","/dev/tty17,2,4,6,d,a,c",
  "second.all","/dev/tty10-9,a-f","/dev/tty10-9,a-f",
  "second.pairs","/dev/tty10,1,3,5,8,9,b,d","/dev/tty17,2,4,6,f,a,c,e",
  "third.single8","/dev/tty20-7","/dev/tty20-7",
  "third.single14","/dev/tty20-9,a-d","/dev/tty20-9,a-d",
  "third.double8","/dev/tty20,1,3,5","/dev/tty27,2,4,6",
  "third.double14","/dev/tty20,1,3,5,8,9,b","/dev/tty27,2,4,6,d,a,c",
  "third.all","/dev/tty20-9,a-f","/dev/tty20-9,a-f",
  "third.pairs","/dev/tty20,1,3,5,8,9,b,d","/dev/tty27,2,4,6,f,a,c,e",
  "fourth.single8","/dev/tty30-7","/dev/tty30-7",
  "fourth.single14","/dev/tty30-9,a-d","/dev/tty30-9,a-d",
  "fourth.double8","/dev/tty30,1,3,5","/dev/tty37,2,4,6",
  "fourth.double14","/dev/tty30,1,3,5,8,9,b","/dev/tty37,2,4,6,d,a,c",
  "fourth.all","/dev/tty30-9,a-f","/dev/tty30-9,a-f",
  "fourth.pairs","/dev/tty30,1,3,5,8,9,b,d","/dev/tty37,2,4,6,f,a,c,e",
  "\0","\0","\0"
};

main(argc,argv)
  int argc;
  char *argv[];

{
   int arrcount, match;
   extern finish();
   extern receive_timeout();

   int i;				/* general purpose loop variable */
   int return_code = 0;			/* return code on exit */
   int status;				/* intermediate error status */
   unsigned char c,*ptr;		/* general character and pointer */
   unsigned char srcbuf[1024];		/* expansion buffer for source names */
   unsigned char rcvbuf[1024];		/* expansion buffer for rcvr names */

   signal(SIGHUP,finish);		/* interrupt, hangup */
   signal(SIGTERM,finish);		/* interrupt, software termination */
   signal(SIGINT,finish);		/* interrupt */
   signal(SIGALRM, receive_timeout);	/* no receive message */

   if (getenv("SD_LOG_DIRECTORY")) exec_by_sysdiag = TRUE;
   if (getenv("SD_LOAD_TEST"))
        if (strcmp(getenv("SD_LOAD_TEST"), "yes") == 0) load_test = TRUE;
   if (getenv("SUN_MANUFACTURING")) {
      if (strcmp(getenv("SUN_MANUFACTURING"), "yes") == 0) {
         display_read_data = TRUE;
         retry_cmp_error = TRUE;
      }  
      if (strcmp(getenv("RUN_ON_ERROR"), "enabled") == 0) {
         stop_on_error = FALSE;
      }  
   }

   sprintf(perror_msg, "%s: perror says", TEST_NAME);
   strcpy(device, "");

   if (argc > 1)
      for(arrcount=1; arrcount < argc; arrcount++) {
                        match = 0;
                        if (strcmp(argv[arrcount], "sd") == 0) {
                                match = TRUE;
                                exec_by_sysdiag = TRUE;
                                verbose = FALSE;
                        }
                        if (strncmp(argv[arrcount], "atn", 3) == 0) {
                                match = TRUE;
                                atn = TRUE;
                                if (argv[arrcount][3] != 't')
                                    sending_atn_msg = TRUE;
                                verbose = FALSE;
                        }
                        if (strcmp(argv[arrcount], "d") == 0) {
                                match = TRUE;
                                debug = TRUE;
                                verbose = TRUE;
                        }
                        if (strcmp(argv[arrcount], "dd") == 0) {
                                match = TRUE;
                                display_read_data = TRUE;
                        }
                        if (strcmp(argv[arrcount], "v") == 0) {
                                match = TRUE;
                                verify = TRUE;
                        }
                        if (strcmp(argv[arrcount], "lt") == 0) {
                                match = TRUE;
                                load_test = TRUE;
                        }
                        if (strcmp(argv[arrcount], "re") == 0) {
                                match = TRUE;
                                stop_on_error = FALSE;
                                retry_cmp_error = TRUE;
                        }
                        if (argv[arrcount][0] == 'e') {
                                simulate_error = atoi(&argv[arrcount][1]);
                                if (simulate_error > 0 &&
						simulate_error < END_ERROR) {
                                   match = TRUE;
                                }
                        }
                        if (strncmp(argv[arrcount], "/dev/tty", 8) == 0 && 
								!keyword) { 
				match = TRUE;
				if (!device_1) {
				   device_1 = TRUE;
           			   expars(argv[arrcount],srcbuf);
           			   expars(argv[arrcount],rcvbuf);
                                   if (verbose) printf("%s: Testing %s.\n",
                                                     TEST_NAME, argv[arrcount]);
                                }
				else {
				   if (!device_2) {
				      device_2 = TRUE;
				      expars(argv[arrcount],rcvbuf);
                                      if (verbose) printf("%s: Testing %s.\n",
                                                     TEST_NAME, argv[arrcount]);
				   }
				   else match = FALSE;
                                }
                        }                           /* search for key words */ 
                        if (!match && !device_1 && !keyword) {
                           	for(i=0; *(pkg[i].key) != '\0';i++) {
                              	   if (!strcmp(argv[arrcount],pkg[i].key)){
               		               expars(pkg[i].src,srcbuf);
               		               expars(pkg[i].rcv,rcvbuf);
				       keyword = TRUE;
				       match = TRUE;
                                       if (verbose) printf("%s: Testing %s.\n",
                                                     TEST_NAME, argv[arrcount]);
               		               break;
                                    }
				}
                        }
                        if (!match) {
                           printf("Usage: %s [v] [sd/atn] [device] [device] [re] [lt] [dd] [d] [e{1-%d}]\n", TEST_NAME, END_ERROR - 1);
                           exit(USAGE_ERROR);
                        }
      }  

   if (verify) {               /* verify mode */
        if (verbose) printf("%s: Verify mode.\n", TEST_NAME);
        exit(0);
   }

   if ((!device_1 && !keyword) || simulate_error == NO_PORTS_SELECTED ||
				  simulate_error == NO_SD_PORTS_SELECTED) {
      if (exec_by_sysdiag && simulate_error != NO_PORTS_SELECTED) 
	 syspars(srcbuf, rcvbuf);
      else throwup(-NO_PORTS_SELECTED, "%s: No serial ports selected.",
	 TEST_NAME);
   }

                     			/* count ports to test */
   srcnam = srcbuf;
   rcvnam = rcvbuf;
   while ((*srcnam != '\0') && (*rcvnam != '\0')) {
      ports_to_test++;
      if (strcmp(srcnam,rcvnam)) ports_to_test++;
   while(*srcnam++ != '\0');
   while(*rcvnam++ != '\0');
   }
                     		/* adjust number of characters to send */
   send_characters = send_characters / ports_to_test;
   if (send_characters < 511) send_characters = 511;

   if (atn || verbose) {
       if (atn) send_msg_to_atn(INFO, "Started.");
       throwup(0, "%s: Started.", TEST_NAME);
   }

   while (atn || pass == 0) {
      pass++;
      srcnam = srcbuf;			/* point at start of source devices */
      rcvnam = rcvbuf;			/* point at start of receive devices */
      while ((*srcnam != '\0') && (*rcvnam != '\0')) {
         status = siodat(srcnam,rcvnam);	/* test named port */
         return_code |= status;		/* "or" in all error statuses */
         if (strcmp(srcnam,rcvnam)) {     /* if testing two different ports, */
                                        /* then reverse the direction */
					/* to test input and output for both */
            if (!load_test && !verbose) sleep (10);
            status = siodat(rcvnam,srcnam);/* test named port (reverse order) */
            return_code |= status;         /* "or" in all error statuses */
         }
	 if (load_test) break;
	 if (!verbose) sleep (10);
         while(*srcnam++ != '\0');	/* advance to next source name */
         while(*rcvnam++ != '\0');	/* advance to next receiver name */
      }
      if (load_test || return_code != TESTED_OK) break;
      if (atn || verbose)   
         printf("%s: pass %d, errors %d.\n", TEST_NAME, pass, errors);
   }
   if (atn || verbose)
       throwup(0, "%s: Stopped, pass %d, errors %d.", TEST_NAME, pass, errors);
   exit(return_code);				/* exit with status */
}

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

siodat(srcdev,rcvdev)
unsigned char *srcdev,*rcvdev;		/* device names */
{
  int i, l;				/* general purpose loop counter */
  long count;				/* character count for port read */
  unsigned char c,*ptr;			/* general character and pointer */
  unsigned char pattern[256];		/* pattern written to port */
  extern clean_up();

  swapflag = 0;			/* clear swap flag */

/*
*	Open requested port
*/
  if (simulate_error == DEV_NOT_OPEN) strcpy(srcdev, "/dev/tty.invalid");

  for (;;) {				/* provide outside master loop */
    devnam = srcdev;			/* point at attempted device name */
    if ((srcfd = open(srcdev,O_RDWR,7)) == -1) {
      perror(perror_msg);
      return_code = DEV_NOT_OPEN;	/* set error status */
      break;				/* report and exit */
    }
    devnam = rcvdev;			/* point at attempted device name */
    if (!strcmp(srcdev,rcvdev)) {
      rcvfd = srcfd;			/* source and receive same, set */
    					/* ptrs the same for loopback */
    }
					/* different, open receive port too */
    else if ((rcvfd = open(rcvdev,O_RDWR,7)) == -1) {
      perror(perror_msg);
      return_code = DEV_NOT_OPEN;	/* set error status */
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
    board = (*(srcdev +8));
    switch (board) {
      case '0':
      case '1':
      case '2':
      case '3':
              characters = send_characters * 3;
	      break;
      default:
              characters = send_characters;
    }
    if (debug) 
       printf("Ports = %d, testing '%s', board %c, sending %d characters.\n",
	       ports_to_test, srcdev, board, characters);

    if (load_test || debug) characters = 100;
    c = PATBEG;
    for (l = 0; l <= characters; l++) {
      pattern[0] = c;
      if (simulate_error == DATA_ERROR) pattern[0] = c + 1;
      else if (simulate_error == WRITE_FAILED) close(srcfd);

/*      printf("%x,", c);			*/

      if (simulate_error != NOT_READY) {
         if ((write(srcfd,pattern,1)) == -1) {
            perror(perror_msg);
	    return_code = WRITE_FAILED;
	    errors++;
	    break;
         }
      }
      pattern[0] = '\0';		/* clear pattern */

      if (simulate_error == READ_FAILED) close(rcvfd);

      if (load_test || verbose) alarm(4);
      else alarm(RECEIVE_WAIT_TIME);

      if ((read(rcvfd, pattern, 1)) != 1) {
	 alarm(0);   
	 perror(perror_msg);
	 return_code = READ_FAILED;
	 errors++;   
	 break;
      }
      alarm(0);
      if (c != pattern[0]) {
        return_code = DATA_ERROR;	/* wrong data, flag error */
	errors++;
        break;				/* exit pattern loop */
      }
/*      if ((l % 512) == 0 && !verbose) sleep (10);  */
      c++;
    }					/* loop through each data pattern */
    break;				/* done, report test */
  }

/*
*	Display test results
*/

  strcpy(device, devnam);

  switch (return_code) {

    case TESTED_OK:
      if (verbose) throwup(0, "%s: Device '%s' tested OK.", TEST_NAME, devnam);
      break;    

    case DEV_NOT_OPEN:

      if (atn) send_msg_to_atn(FATAL, "Couldn't open file '%s', pass %d, errors %d.", devnam, pass, errors);
      throwup(verbose? DEV_NOT_OPEN:-DEV_NOT_OPEN, "%s: Couldn't open file '%s',%s errors %d.", TEST_NAME, devnam, print_pass()? tmp_msg:"", errors);
      break;

    case WRITE_FAILED:

      if (atn) send_msg_to_atn
	 (ERROR, "Transmit failed on '%s', pass %d, errors %d.",
		  devnam, pass, errors);
      throwup(verbose? WRITE_FAILED:-WRITE_FAILED,
	 "ERROR: %s, transmit failed on '%s',%s errors %d.",
	 TEST_NAME, devnam, print_pass()? tmp_msg:"", errors);
      break;

    case READ_FAILED:

      if (atn) send_msg_to_atn
	 (ERROR, "Receive failed on '%s', pass %d, errors %d.",
		  devnam, pass, errors);
      throwup(verbose? READ_FAILED:-READ_FAILED,
	 "ERROR: %s, receive failed on '%s',%s errors %d.",
	  TEST_NAME, devnam, print_pass()? tmp_msg:"", errors);
      break;

    case DATA_ERROR:

      if (atn) send_msg_to_atn(ERROR, "Data error on device '%s', exp = 0x%x, actual = 0x%x, pass %d, errors %d.", devnam, c, pattern[0], pass, errors);
      throwup(verbose? DATA_ERROR:-DATA_ERROR, "ERROR: %s, data error on device '%s', exp = 0x%x, actual = 0x%x,%s errors %d.", TEST_NAME, devnam, c, pattern[0], print_pass()? tmp_msg:"", errors);
      break;
  }

  clean_up();				/* clean up ports and close devices */
  return(return_code);			/* return status to caller */
}

/******************************************************************************
*
*	name		"syspars"
*
*	synopsis	status = syspars(source, destination)
*
*			status	=>	0 = success, non-zero = error
*
*			source	=>	address of buffer to hold expanded
*					list of source ports 
*
*			destination =>	address of buffer to hold expanded
*					list of destination ports
*
*
*	description	The sysdiag environmental variables "SERIAL_PORTS_n"
*			are parsed into the source and destination fields.
*			Example:
*
*				SERIAL_PORTS_1 = a
*
*			expands to:
*
*				source         destination
*				------         -----------
*				/dev/ttya	/dev/ttya
*
*			and
*				SERIAL_PORTS_1 = a-b
*
*			expands to:
*
*				source         destination
*				------         -----------
*				/dev/ttya      /dev/ttyb
*
*/


/*
*	syspars program
*/

syspars(source,destination)
  char	*source,*destination;	/* ptrs source and destination buffers */

{
  int i;				/* general purpose loop variable */
  unsigned char port,*ptr;		/* port character and pointer */


/*
*	Initialize
*/

  srcbuf = source;			/* global ptr to source line */
  desbuf = destination;			/* global ptr to destination buffer */

/*
*	get sysdiag ports from "SERIAL_PORTS_n" envirmental variables.
*/

  for (i = 1; i < 100; i++) {
     sprintf(serial_ports,"SERIAL_PORTS_%d", i);
     if (getenv(serial_ports) && simulate_error != NO_SD_PORTS_SELECTED) {
        strcpy(port_to_test, "/dev/tty");
        strcat(port_to_test, (getenv(serial_ports)));
        if (debug) printf("%s, port(s) to test = %s \n",
	   serial_ports, port_to_test);
        port_ptr = port_to_test;

        while ((*srcbuf++ = *port_ptr++) != '\0') {
	   if (*(port_ptr -1) == '-') {
	      (*(srcbuf -1)) = '\0';
	      *desbuf--;
	      if (*(desbuf -1) != 'y') *desbuf--;
              while (*port_ptr != '\0') {
		 *desbuf++ = *port_ptr++;
	      }
           break;
           }
	   else {
	      *desbuf++ = (*(port_ptr -1));
           }
        }
	*desbuf++ = '\0';  	/* mark end of device */

/* Check source port for device 0-3, if yes change 'tty' to 'ttys' */

        if (*(srcbuf -3) == 'y') {
	   port = (*(srcbuf -2));
	   switch (port) {
	      case '0':
	      case '1':
	      case '2':
	      case '3':
			(*(srcbuf -2)) = 's';
			(*(srcbuf -1)) = port;
			*srcbuf++ = '\0';       /* mark end of device */
			break;
	   }
	}
/* Check destination port for device 0-3, if yes change 'tty' to 'ttys' */

        if (*(desbuf -3) == 'y') {
	   port = (*(desbuf -2));
	   switch (port) {
	      case '0':
	      case '1':
	      case '2':
	      case '3':
			(*(desbuf -2)) = 's';
			(*(desbuf -1)) = port;
			*desbuf++ = '\0';       /* mark end of device */
			break;
	   }
	}
     }
     else {
	if (i == 1) {
	   if (atn) send_msg_to_atn(FATAL, "No serial ports selected.");
	   throwup(-NO_SD_PORTS_SELECTED, "%s: No serial ports selected.%s",
			                  TEST_NAME, debug? serial_ports : "");
        }
	break;
     }
  }
}

/******************************************************************************
*
*	name		"expars.c"
*
*	synopsis	status = expars(source, destination, limit)
*
*			status	=>	0 = success, non-zero = error
*
*			source	=>	set of arguments or ranges to be
*					parsed and expanded
*
*			destination =>	address of buffer to hold expanded
*					list of arguments
*
*			limit	=>	size of expand buffer
*
*
*
*	description	A single string (usually a command line argument) is
*			is parsed and expanded according to its content. If
*			a range is given, a table of device names is
*			 constructed.  The same applies to a a list.
*			  Construction  uses the full root, replacing
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
*			conditions can be combined:
*
*				"/dev/tty00-4,8,a,c-e"
*
*			expands to:
*			
*
*				/dev/tty00
*				/dev/tty01
*				/dev/tty03
*				/dev/tty04
*				/dev/tty08
*				/dev/tty0a
*				/dev/tty0c
*				/dev/tty0d
*				/dev/tty0e
*
*/


/*
*	expars program
*/

expars(compact,expand,limit)
  char	*compact,*expand;		/* ptrs source and expand buffers */
  int	limit;				/* size of expand buffer */

{
  int i;				/* general purpose loop variable */
  int status;				/* error status */
  unsigned char c,*ptr;			/* general character and pointer */


/*
*	Initialize
*/

  status = 0;				/* assume no error */
  bleft = limit-2;			/* set bytes left less a margin */
  srcbuf = compact;			/* global ptr to source line */
  exbuf = expand;			/* global ptr to expand buffer */
  root = expand;			/* intial root name at expand buffer */
  *expand = '\0';			/* set "null" expand buffer */

/*
*	Parse source line
*/

    while ((*exbuf++ = *srcbuf++) != '\0') {
					/* source line is '\0' terminated */
      if (--bleft == 0) status = -1;	/* prevent buffer overflow */
      
      switch (*(srcbuf-1)) {

        case '-':			/* range list */
          status = exrange();
          break;

        case ',':			/* item list */
          status = exlist();
          break;
      }
      if (status) break;		/* error, stop parsing */
    }
  if (!status) *exbuf = '\0';		/* mark end of buffer */             
  return(status);			/* exit with status */
}

/*
*	Expand range
*/

exrange()
{
  int	upsize;				/* size of upper limit */

  *(exbuf-1) = '\0';			/* tag end root name (overwrite '-') */
  for(upsize=0; (isalnum(*srcbuf++)); upsize++);
					/* get size of upper limit */
  if (upsize == 0) return(-1);		/* no upper limit, return error */
  while ((strncmp(srcbuf-1-upsize,exbuf-1-upsize,upsize)) > 0) {
					/* expand until ranges meet */
    while ((*exbuf++ = *root++) != '\0') {
					/* copy previous root name */
    if (--bleft == 0) return(-1);	/* prevent buffer overflow */
    }
    strinc(exbuf-1-upsize);		/* yet upper limit, increment */
  }
  srcbuf--;				/* end of upper range in source line */
  exbuf--;				/* end of latest name in buffer */
  return(0);				/* return success */
}

/*
*	Expand list
*/

exlist()
{
  int	newsize;			/* size of new item */

  *(exbuf-1) = '\0';			/* tag end root name (overwrite ',') */
  for(newsize=0; (isalnum(*srcbuf++)); newsize++);
					/* get size of new item */
  if (newsize == 0) return(-1);		/* no new item, return error */
  while ((*exbuf++ = *root++) != '\0') {
					/* copy previous root name */
    if (--bleft == 0) return(-1);	/* prevent buffer overflow */
  }
  srcbuf -= newsize + 1;		/* point at new item replacement */
  exbuf -= newsize + 1;			/* point at old part to replace */
  while (newsize--) {
    *exbuf++ = *srcbuf++;		/* replace old part of item name */
  }
  return(0);				/* return success */
}


/*
*	Increment a string
*/

strinc(ptr)
char	*ptr;				/* string to increment */
{
  int	size;				/* size of string */

  for (size=0; *ptr++ != '\0'; size++);	/* measure string */
  ptr--;				/* back up to end of string */
  while(size--) {
    ptr--;				/* back up to next character */
    (*ptr)++;				/* increment last character */

    switch (*ptr) {			/* check for "rollover" of range */
      case '9'+1:			/* end of numeric range? */
        *ptr = '0';			/* "roll over" for carry */
        break;				/* try again on next char */
      case 'Z'+1:			/* end of upper case alpha range? */
        *ptr = 'A';			/* "roll over" for carry */
        break;				/* try again on next char */
      case 'z'+1:			/* end of lower case range? */
        *ptr = 'a';			/* "roll over" for carry */
        break;				/* try again on next char */
      default:
        return(0);			/* done, exit */
    }
  }
  return(-1);				/* no more places for carry, error */
}

/*
                Note:   Not a standard "throwup" routine.
*/

throwup(where, fmt, a, b, c, d, e, f, g, h, i)
int     where;
char    *fmt;
u_long  a, b, c, d, e, f, g, h, i;
{
  char *attempt_log_msg = "Was attempting to log the following message:\n";
  extern char   *mktemp();
  int           clock;
  char          fmt_msg_buffer[200];
  char          *fmt_msg = fmt_msg_buffer;

  clock = time(0);
  sprintf(fmt_msg, fmt, a, b, c, d, e, f, g, h, i);

/*** not standard ***/
  if (where < 0) clean_up();
/*** not standard ***/

  if (!logfd){
     if (simulate_error == NO_OPEN_LOG) {
        strcpy(logfile, "not/valid/log");
     }   
     else {
        if (getenv("SD_LOG_DIRECTORY") && simulate_error != NO_SD_LOG_DIR) {
           strcpy(logfile, (getenv("SD_LOG_DIRECTORY")));
           strcat(logfile, "/");
           strcat(logfile, LOGFILE_NAME);
        }
        else {
           sprintf(msg, "No log file environmental variable.");
           if (atn) send_msg_to_atn(FATAL, msg);
           fprintf(stderr, "%s: %s\n", TEST_NAME, msg);
           fprintf(stderr, "%s: %s", TEST_NAME, attempt_log_msg);
           fprintf(stderr, "%s %s", fmt_msg, ctime(&clock));
           exit(NO_SD_LOG_DIR);
        }
     }
     if ((logfd = open(mktemp(logfile),O_WRONLY|O_CREAT|O_APPEND ,0644)) <0){
        perror(perror_msg);
        sprintf(msg, "Couldn't open logfile '%s'.", logfile);
        if (atn) send_msg_to_atn(FATAL, msg);
        fprintf(stderr, "%s: %s\n", TEST_NAME, msg);
        fprintf(stderr, "%s: %s", TEST_NAME, attempt_log_msg);
        fprintf(stderr, "%s %s", fmt_msg, ctime(&clock));
        exit(NO_OPEN_LOG);
     }
     else dup2(logfd, 2);               /* set logfile as stderr */
  }   
  fprintf(stderr, "%s %s", fmt_msg, ctime(&clock));
  printf("%s %s", fmt_msg, ctime(&clock));
   
  fflush(stderr);
  fsync(2);
  if (where < 0) exit(-where);
  failed = where;
}

print_pass()
{
   if (atn) {
      sprintf(tmp_msg, " pass %d,", pass);
      return TRUE;
   }
   else return FALSE;
}

clean_up()
{
  if (rcvfd > 0) {
     if (swapflag > 2) {
       ioctl(rcvfd,TIOCNXCL,0);		/* release EXCLUSIVE use of port */
     }
     if (swapflag > 3) {
       rcvlist.sg_flags = rcvsav;	/* restore original tty mode */
       ioctl(rcvfd,TIOCSETP,&rcvlist);	/* restore old port parameters */
     }
     close(rcvfd);				/* close device */
     if (debug) printf("Close receive port '%s'.\n", rcvnam);
  }

  if (srcfd > 0) {
     if (swapflag > 0) {
       ioctl(srcfd,TIOCNXCL,0);		/* release EXCLUSIVE use of port */
     }
     if (swapflag > 1) {
       srclist.sg_flags = srcsav;	/* restore original tty mode */
       ioctl(srcfd,TIOCSETP,&srclist);	/* restore old port parameters */
     }
     close(srcfd);				/* close device */
     if (debug) printf("Close transmit port '%s'.\n", srcnam);
  }
}

receive_timeout()
{

  strcpy(device, devnam);

  if (atn) send_msg_to_atn(FATAL, "Device '%s' does not respond. Check loopback connector, pass %d, errors %d.", devnam, pass, errors);
  throwup(-NOT_READY, "%s: Device '%s' does not respond. Check loopback connector,%s errors %d.", TEST_NAME, devnam, print_pass()? tmp_msg:"", errors);

}

/*
*	Clean up ports and exit 
*/

finish()
{
   clean_up();
   if (atn || verbose) {
      strcpy(device, "");
      sprintf(msg, "Stopped, pass %d, errors %d.", pass, errors);
      throwup(0, "%s: %s", TEST_NAME, msg);
      if (atn ) {
         send_msg_to_atn(INFO, msg);
         exit(0);
      }
   }
   exit(INTERRUPT);
}

#ifdef ATN_VERSION
#include "atnrtns.c"    /* ATN routines */
#else
send_msg_to_atn()
{
printf("This is not the ATN version!\n");
}
#endif
