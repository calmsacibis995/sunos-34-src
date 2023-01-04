#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/file.h>
#include <sys/wait.h>

/* #include <sys/ioctl.h>       */
#include "ioctl.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netdb.h>
#include "syncmode.h"
#include "syncstat.h"
/*
#include <sundev/syncmode.h>
#include <sundev/syncstat.h>
*/

/* #define FAKE_IT			0 */

#define NET_FILES_INCLUDED	0
#define FALSE                   0
#define TRUE                    ~FALSE
#define INFO                    0
#define WARNING                 1
#define FATAL                   2
#define ERROR                   3
#define NO_SD_LOG_DIR           1
#define NO_OPEN_LOG             2
#define NO_LOAD			3
#define NO_ATTACH		4
#define NO_LAYER		5
#define NO_LAYER_LOOPBACK	6
#define NO_SOCKET		7
#define NO_GETSYNC		8
#define NO_SETSYNC		9
#define NO_CHECK_SETSYNC	10
#define NO_BSC			11
#define NO_ASYNC		12
#define ILLEGAL_PROTOCOL	13
#define DEV_NOT_OPEN		14
#define WRITE_FAILED		15
#define READ_FAILED		16
#define RECEIVE_TIMEOUT		17
#define COMPARE_ERROR		18
#define NO_STAT_SOCKET		19
#define END_ERROR               20
#define NO_SD_PORTS		21
#define USAGE_ERROR             99

#define TEST_NAME               "dcptest"
#define LOGFILE_NAME            "log.dcptest.XXXXXX"

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

static char     sccsid[] = "@(#)dcptest.c 1.1 9/25/86 Copyright 1985 Sun Micro";

#define DCPBD_A 0xa0
#define DCPBD_B	0xb0
#define DCPBD_C	0xc0
#define DCPBD_D 0xd0

#define CHAN_0	0x0
#define CHAN_1  0x1
#define CHAN_2	0x2
#define CHAN_3	0x3

#define SDLC_PROTOCOL	0
#define BSC_PROTOCOL	1
#define ASYNC_PROTOCOL	2
#define LOOPBACK	3
#define MAX_LENGTH 	1018
#define RECEIVE_WAIT_TIME 200

u_char		pattern, protocol, port;
u_char		loopback_flag, load_dcp_kernal = FALSE;
int		fid;
int   		port_ptr = 1, int_port;
int		internal_loopback = FALSE;
int 		ttya_test = FALSE, status_dcp = FALSE;
int 		sock_type;
int		loop_count, max_frame_len, min_frame_len;
int		err_frame_no, good_frame_no;
int		ktoi();
int		no_receive_response();
static char	digits[] = "0123456789abcdef";
u_char		sbarray[1024],
		rbarray[1024];
char            tmpbuf[128], board, pattn_type, clock_type;
char		*lb_mptr, *nrzi_mptr, *txc_mptr, *rxc_mptr;
long		baudrate;
int 		frame_count;
int		load_dcp_a = FALSE;
int		load_dcp_b = FALSE;
int		load_dcp_c = FALSE;
int		load_dcp_d = FALSE;
int		attach_a[4];
int		attach_b[4];
int		attach_c[4];
int		attach_d[4];

struct		ifreq ifr;
struct 		syncmode sm;
struct syncmode *ssm = (struct syncmode *)ifr.ifr_data;
struct 		ss_dstats sd;
struct 		ss_estats se;

int 		s;
char *yesno[] = {
	"no",
	"yes",
	0,
};
char *txnames[] = {
	"txc",
	"rxc",
	"baud",
	"pll",
	0,
};

char *rxnames[] = {
	"rxc",
	"txc",
	"baud",
	"pll",
	0,
};

/* ------------------------------------------------------------------------
 * The main program to handle the command line
 * The format of the command line:
 * ------------------------------------------------------------------------
 */
main(argc, argv)
int	argc;		/* # of argument(s) */
char	*argv[];	/* pointer array to the arguments */
{
   int arrcount, match;
   extern finish();

   signal(SIGHUP, finish);
   signal(SIGTERM, finish);
   signal(SIGINT, finish);
   signal(SIGALRM, no_receive_response);

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
                                load_dcp_kernal = TRUE;
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
                        if (strcmp(argv[arrcount], "ttya") == 0) {
                                match = TRUE;
                                exec_by_sysdiag = FALSE;
				strcpy(device, "ttya");
				ttya_test = TRUE;
			}
                        if (strncmp(argv[arrcount], "dcp", 3) == 0) {
                                exec_by_sysdiag = FALSE;
				board = argv[arrcount][3];
                                port = argv[arrcount][4];
				int_port = atoi(&argv[arrcount][4]);
                                if (port >= '0' && port < '4' &&
                                   board >= 'a' && board < 'e') {
                                   match = TRUE;
                                   strcpy(device, argv[arrcount]);
                                   if (verbose) printf("%s: Testing %s.\n",
                                                        TEST_NAME, device);
                                }
                        }
                        if (strcmp(argv[arrcount], "i") == 0) {
                                match = TRUE;
                                internal_loopback = TRUE;
                        }
                        if (strcmp(argv[arrcount], "st") == 0) {
                                match = TRUE;
                                status_dcp = TRUE;
                        }
                        if (strcmp(argv[arrcount], "k") == 0) {
                                match = TRUE;
                                load_dcp_kernal = TRUE;
                        }
                        if (!match) {
                           printf ("Usage: %s [v] [sd/atn] [dcp{a-d}{0-3}] [i] [k] [re] [lt] [dd] [d] [e{1-%d}]\n", TEST_NAME, END_ERROR - 1);
                           exit(USAGE_ERROR);
                        }
      }  

   if (verify) {               /* verify mode */
        if (verbose) printf("%s: Verify mode.\n", TEST_NAME);
        exit(0);
   }
   init_parm();

   if (simulate_error != 0) {
      if (simulate_error == NO_LAYER_LOOPBACK) protocol = LOOPBACK;
      if (simulate_error == NO_BSC) protocol = BSC_PROTOCOL;
      if (simulate_error == NO_ASYNC) protocol = ASYNC_PROTOCOL;
      if (simulate_error == ILLEGAL_PROTOCOL) protocol = ILLEGAL_PROTOCOL;
   }

   if (atn || verbose) {
       if (atn) send_msg_to_atn(INFO, "Started.");
       if (exec_by_sysdiag) throwup(0, "%s: Started.", TEST_NAME);
       else throwup(0, "%s: Started on %s.", TEST_NAME, device);
   }

   if (debug) {
      printf("pattern= 0x%x, loop count= %d, ", pattern, loop_count);
      printf("min frame= %d, ", min_frame_len);
      printf("max frame= %d, ", max_frame_len);
      switch (protocol) {
	case SDLC_PROTOCOL:
	   printf("protocol= SDLC\n");
	   break;
	case BSC_PROTOCOL:
	   printf("protocol= BSC\n");
	   break;
	case ASYNC_PROTOCOL:
	   printf("protocol= ASYNC\n");
	   break;
	case LOOPBACK:
	   printf("protocol= LOOPBACK\n");
	   break;
	default:
	   printf("protocol= ILLEGAL\n");
      }
   }

   while (atn || pass == 0) {
      if (port_ptr == 1) pass++;

      while (a_port_to_test(port_ptr)) {
	
	 if (!ttya_test) set_environment();

         /* --------------------------------------------------------------
          * open the DCP driver
          * --------------------------------------------------------------
          */

         if (simulate_error == DEV_NOT_OPEN) strcpy(tmpbuf, "/dev/dcp.invalid");
         else sprintf(tmpbuf, "/dev/%s", device);
         if (debug) printf("opening %s\n", tmpbuf);
#ifdef FAKE_IT
if (ttya_test || simulate_error == DEV_NOT_OPEN)
#endif
	 if ((fid = open(tmpbuf, O_RDWR, 0)) == -1) {
	    perror(perror_msg);
	    if (atn) send_msg_to_atn(FATAL,
	       "Couldn't open file %s, pass %d, errors %d.",
	       tmpbuf, pass, errors);
            throwup(-DEV_NOT_OPEN, "%s: Couldn't open file %s,%s errors %d.",
	       TEST_NAME, tmpbuf, print_pass()? tmp_msg:"", errors);
         }
      
#ifdef FAKE_IT
if (ttya_test)
#endif
	 if (!status_dcp) exercise_dcp();

#ifndef FAKE_IT
	 if (!load_test && !verbose && !ttya_test && !status_dcp) sleep(3);
#endif
         if (atn || verbose) 
	     	    printf("%s: %s, pass %d, errors %d.\n",
	            TEST_NAME, device, pass, errors);
	 port_ptr++;
	 if (status_dcp || (debug && !ttya_test)) statistics();
	 if (s) close(s);
         if (fid) close(fid);
	 if (!exec_by_sysdiag) break;
      }
      port_ptr = 1;
      if (load_test) break;
   }
   if (atn || verbose) throwup(0, "%s: Stopped %s, pass %d, errors %d.",
                                   TEST_NAME, device, pass, errors);
}   

a_port_to_test(port_pointer)

int port_pointer;
{
   char dcp_port_names[20];
   char *dcp_ports = dcp_port_names;

   if (exec_by_sysdiag) {
      sprintf(dcp_ports,"DCP_PORTS_%d", port_pointer);
      if (getenv(dcp_ports)) {
         strcpy(device, "dcp");
         strcat(device, (getenv(dcp_ports)));
         board = device[3];
         port = device[4];
	 int_port = atoi(&device[4]);
         if (debug) printf("%s, port to test = %s, board %c, port %c.\n",
			    dcp_ports, device, board, port);
         return TRUE;
      }
      else {
         if (port_pointer == 1) {
            if (atn) send_msg_to_atn(FATAL, "No dcp port(s) selected.");
            throwup(-NO_SD_PORTS, "%s: No dcp port(s) selected.", TEST_NAME);
         }
         else return FALSE;
      }
   }
   else return TRUE;
}

set_environment()
{
	/* ----------------------------------------------------------------
	 * set the environment of the DCP board by the system function call.
	 * 1) download the DCP kernel, by dcpload command 
	 *    (dcpload dcpmon.image).
	 * 2) initialize the selected channel, by dcpattach command 
	 *    (dcpattach /dev/dcpXX).
	 * 3) configure the selected channel to the selected protocol, by
	 *    dcplayer command (dcplayer dcpXX YYY ZZZ).
	 * -----------------------------------------------------------------
	 */

   int sts, load_kernal, attach_dcp;

   switch (board) {
      case 'a':
		load_kernal = load_dcp_a;
		load_dcp_a = FALSE;
		attach_dcp = attach_a[int_port];
		attach_a[int_port] = FALSE;
		break;
      case 'b':
		load_kernal = load_dcp_b;
		load_dcp_b = FALSE;
		attach_dcp = attach_b[int_port];
		attach_b[int_port] = FALSE;
		break;
      case 'c':
		load_kernal = load_dcp_c;
		load_dcp_c = FALSE;
		attach_dcp = attach_c[int_port];
		attach_c[int_port] = FALSE;
		break;
      case 'd':
		load_kernal = load_dcp_d;
		load_dcp_d = FALSE;
		attach_dcp = attach_d[int_port];
		attach_d[int_port] = FALSE;
		break;
   }
   if (load_kernal) {

      if (simulate_error == NO_LOAD)
         sprintf(tmpbuf, 
            "/no/dcpload -b %c /usr/sunlink/dcp/dcpmon.image", board);
      else sprintf(tmpbuf, 
         "/usr/sunlink/dcp/dcpload -b %c /usr/sunlink/dcp/dcpmon.image", board);
      if (debug) printf("Executing %s\n", tmpbuf);
#ifdef FAKE_IT
if (simulate_error == NO_LOAD)
#endif
      if (system(tmpbuf)) couldnt_execute(NO_LOAD);
   }
   if (attach_dcp) {
      if (simulate_error == NO_ATTACH)
           sprintf(tmpbuf, "/no/dcpattach /dev/%s", device);
      else sprintf(tmpbuf, "/usr/sunlink/dcp/dcpattach /dev/%s > /dev/null", device);
      if (debug) printf("Executing %s\n", tmpbuf);
#ifdef FAKE_IT
if (simulate_error == NO_ATTACH)
#endif
      if (system(tmpbuf)) couldnt_execute(NO_ATTACH);
   }
   switch (protocol) {
      case SDLC_PROTOCOL:

         if (attach_dcp) {
	    if (simulate_error == NO_LAYER)
	       sprintf(tmpbuf, "/no/dcplayer %s zss%c", device, port);
	    else 
	       sprintf(tmpbuf, 
		  "/usr/sunlink/dcp/dcplayer %s zss%c", device, port);
            if (debug) printf("Executing %s\n", tmpbuf);
#ifdef FAKE_IT
if (simulate_error == NO_LAYER)
#endif
	    if (system(tmpbuf)) couldnt_execute(NO_LAYER);
	 }

	 if (simulate_error == NO_SOCKET) sock_type = 7;
	 else sock_type = SOCK_DGRAM;
         if ((s = socket(AF_INET, sock_type, 0)) < 0) {
	    perror("dcptest: socket, perror says");
            if (atn) send_msg_to_atn (FATAL, 
               "Couldn't open a socket for %s, pass %d, errors %d.",
                device, pass, errors);
            throwup(-NO_SOCKET, 
               "%s: Couldn't open a socket for %s,%s errors %d.",
                TEST_NAME, device, print_pass()? tmp_msg:"", errors);
	 }
	 if (simulate_error == NO_GETSYNC) s = 0;
	 strcpy(ifr.ifr_name, device);
#ifdef FAKE_IT
if (simulate_error == NO_GETSYNC)
#endif
	 if (ioctl(s, SIOCGETSYNC, &ifr)) {
	    perror("dcptest: SIOCGETSYNC, perror says");
            if (atn) send_msg_to_atn (FATAL, 
               "Couldn't get sync mode info for %s, pass %d, errors %d.",
                device, pass, errors);
            throwup(-NO_GETSYNC, 
               "%s: Couldn't get sync mode info for %s,%s errors %d.",
                TEST_NAME, device, print_pass()? tmp_msg:"", errors);
	 }
	 ssm->sm_baudrate = baudrate;
	 ssm->sm_txclock = TXC_IS_BAUD;
	 ssm->sm_rxclock = RXC_IS_BAUD;
	 ssm->sm_loopback = (char)1;

         if (debug) printf("clock type = %c\n", clock_type);

	 if (clock_type != '0') {
		lb_mptr = "no";
		ssm->sm_loopback = (char)0;
	 }

	 switch (clock_type) {

		case '2':
			txc_mptr = "txc";
			rxc_mptr = "rxc";
			ssm->sm_txclock = TXC_IS_TXC;
			ssm->sm_rxclock = RXC_IS_RXC;
			break;
		case '3':
			txc_mptr = "baud";
			rxc_mptr = "rxc";
			ssm->sm_txclock = TXC_IS_BAUD;
			ssm->sm_rxclock = RXC_IS_RXC;
			break;
		case '4':
			txc_mptr = "rxc";
			rxc_mptr = "rxc";
			ssm->sm_txclock = TXC_IS_RXC;
			ssm->sm_rxclock = RXC_IS_RXC;
			break;
	 }
/*
         if (debug) {
            printf("txc=%s, rxc=%s\n", txc_mptr, rxc_mptr);
            printf("syncinit %s %d loopback=%s nrzi=%s txc=%s rxc=%s",
                device, baudrate, lb_mptr, nrzi_mptr, txc_mptr, rxc_mptr);
            sprintf(tmpbuf, "/usr/src/sunlink/inr/syncinit %s %d loopback=%s nrzi=%s txc=%s rxc=%s", 
		device, baudrate, lb_mptr, nrzi_mptr, txc_mptr, rxc_mptr);
            printf("Executing %s\n", tmpbuf);
       	    system(tmpbuf);
         }
*/
	 if (simulate_error == NO_SETSYNC) s = 0;
	 strcpy(ifr.ifr_name, device);
#ifdef FAKE_IT
if (simulate_error == NO_SETSYNC)
#endif
	 if (ioctl(s, SIOCSETSYNC, &ifr)) {
	    perror("dcptest: SIOCSETSYNC, perror says");
            if (atn) send_msg_to_atn (FATAL, 
               "Couldn't set sync mode info for %s, pass %d, errors %d.",
                device, pass, errors);
            throwup(-NO_SETSYNC, 
               "%s: Couldn't set sync mode info for %s,%s errors %d.",
                TEST_NAME, device, print_pass()? tmp_msg:"", errors);
	 }
	 if (debug) {
            if (simulate_error == NO_CHECK_SETSYNC) s = 0;
	    strcpy(ifr.ifr_name, device);
#ifdef FAKE_IT
if (simulate_error == NO_CHECK_SETSYNC)
#endif
	    if (ioctl(s, SIOCGETSYNC, &ifr)) {
               perror("dcptest: SIOCGETSYNC, perror says");
               if (atn) send_msg_to_atn (FATAL, 
                      "Couldn't check SETSYNC for %s, pass %d, errors %d.",
                       device, pass, errors);
               throwup(-NO_CHECK_SETSYNC, 
                      "%s: Couldn't check SETSYNC for %s,%s errors %d.",
                       TEST_NAME, device, print_pass()? tmp_msg:"", errors);
	    }
	    printf
	      ("speed= %d, internal loopback= %s, nrzi= %s, txc= %s, rxc= %s\n",
		ssm->sm_baudrate, yesno[ssm->sm_loopback], yesno[ssm->sm_nrzi],
		txnames[ssm->sm_txclock], rxnames[ssm->sm_rxclock]);
	 }
	 break;

      case BSC_PROTOCOL:
         throwup(-NO_BSC, 
            "%s: BSC protocol is not implemented for %s,%s errors %d.",
             TEST_NAME, device, print_pass()? tmp_msg:"", errors);

      case ASYNC_PROTOCOL:
         throwup(-NO_ASYNC, 
            "%s: ASYNC protocol is not implemented for %s,%s errors %d.",
             TEST_NAME, device, print_pass()? tmp_msg:"", errors);

      case LOOPBACK:
	 loopback_flag = TRUE;
 
         if (simulate_error == NO_LAYER_LOOPBACK)
	    sprintf(tmpbuf, "/no/dcplayer %s lo%c", device, port);
	 else 
	    sprintf(tmpbuf, "/usr/sunlink/dcp/dcplayer %s lo%c", device, port);
         if (debug) printf("Executing %s\n", tmpbuf);
#ifdef FAKE_IT
if (simulate_error == NO_LAYER_LOOPBACK)
#endif
	 if (system(tmpbuf)) couldnt_execute(NO_LAYER_LOOPBACK);
	 break;

      default:
         throwup(-ILLEGAL_PROTOCOL, 
            "%s: Illegal protocol specified for %s,%s errors %d.",
             TEST_NAME, device, print_pass()? tmp_msg:"", errors);
   }
}

couldnt_execute(who)
int who;
{
   char file_name_buffer[30];
   char *file_name = file_name_buffer;

   switch (who) {
      case NO_LOAD: {
	 sprintf(file_name, "load' for dcp %c", board);
	 break;
         }
      case NO_ATTACH: {
	 sprintf(file_name, "attach' for %s", device);
	 break;
         }
      case NO_LAYER:
      case NO_LAYER_LOOPBACK: {
	 sprintf(file_name, "layer' for %s", device);
         }
   }

   if (atn) send_msg_to_atn (FATAL, "Couldn't successfully execute '/usr/sunlink/dcp/dcp%s, pass %d, errors %d.",
       file_name, pass, errors);
   throwup(-who, 
      "%s: Couldn't successfully execute '/usr/sunlink/dcp/dcp%s,%s errors %d.",
       TEST_NAME, file_name, print_pass()? tmp_msg:"", errors); 
} 

exercise_dcp()
{
   register        i, k, transmit_length, lp1;
   int             nbytes;
   u_char          *sbufptr, *rbufptr;
   int		   simulate_receive_timeout = FALSE;

   sbufptr = sbarray;
   rbufptr = rbarray;
   frame_count = max_frame_len - min_frame_len + 1;

   for (lp1=1; lp1 <= loop_count; lp1++) {
      prepare_buffer();
      transmit_length = min_frame_len;
      for (i = 0; i < frame_count; i++) {

  	 if (simulate_error != 0) { 	
	    if (simulate_error == COMPARE_ERROR) sbarray[1]++; 
	    if (simulate_error == WRITE_FAILED) close(fid);
	    if (simulate_error == RECEIVE_TIMEOUT) 
		simulate_receive_timeout = TRUE;
	 }

	 if (!simulate_receive_timeout) {
 	    if ((nbytes = write(fid, sbufptr, transmit_length)) == -1) {
	       perror(perror_msg);
	       errors++;
	       if (atn) send_msg_to_atn 
	          (ERROR, "Transmit failed on %s, pass %d, errors %d.",
	                   device, pass, errors);
	       throwup(-WRITE_FAILED, 
	          "ERROR: %s, transmit failed on %s,%s errors %d.",
	           TEST_NAME, device, print_pass()? tmp_msg:"", errors); 
	    }
	 } 
         if (debug) printf ("Transmit complete, buffer %d length %d\n",
			     i+1, transmit_length); 
	 if (debug) sleep(1);

	 if (debug) printf("Starting receive\n");
  	 if (simulate_error != 0) { 	
	    if (simulate_error == COMPARE_ERROR) sbarray[1]--; 
	    if (simulate_error == READ_FAILED) close(fid);
	 }

	 if (load_test || verbose) alarm(4);
	 else alarm(RECEIVE_WAIT_TIME);
#ifdef FAKE_IT
if (simulate_error == READ_FAILED || simulate_receive_timeout)
#else 
if (!ttya_test || simulate_error == READ_FAILED || simulate_receive_timeout)
#endif
	 if ((nbytes = read(fid, rbufptr, transmit_length)) == -1) {
	    alarm(0);
	    perror(perror_msg);
	    errors++;
	    if (atn) send_msg_to_atn 
	       (ERROR, "Receive failed on %s, pass %d, errors %d.",
	                device, pass, errors);
	    throwup(-READ_FAILED, 
	       "ERROR: %s, receive failed on %s,%s errors %d.",
	        TEST_NAME, device, print_pass()? tmp_msg:"", errors); 
	 }
	 alarm(0);
	 if (verbose) printf("Receive complete, length %d\n", nbytes);
#ifdef FAKE_IT
if (simulate_error == COMPARE_ERROR)
#else 
if (!ttya_test || simulate_error == COMPARE_ERROR)
#endif
	 if (bcmp(sbufptr, rbufptr, transmit_length)) {
	    errors++;
	    if (atn) send_msg_to_atn 
	       (ERROR, "Data compare error on %s, pass %d, errors %d.",
	                device, pass, errors);
	    throwup(-COMPARE_ERROR, 
	       "ERROR: %s, data compare error on %s,%s errors %d.",
	        TEST_NAME, device, print_pass()? tmp_msg:"", errors); 
         }
         ++transmit_length;
      }
   }
}

no_receive_response()
{
   if (atn) send_msg_to_atn (FATAL, 
      "%s does not respond, check loopback connector, pass %d, errors %d.",
       device, pass, errors);
   throwup(-RECEIVE_TIMEOUT, 
      "%s: %s does not respond, check loopback connector,%s errors %d.",
       TEST_NAME, device, print_pass()? tmp_msg:"", errors);
}

/* -------------------------------------------------------------------
 * prepare sending buffer
 * -------------------------------------------------------------------
 */

prepare_buffer()
{
	register int		k, j;
	register u_char		*mptr;


   mptr = sbarray;
   switch (pattn_type) {

   	case 'c':
		for (j=1; j <= MAX_LENGTH; j++)      /* filling */
           	   *mptr++ = pattern;
		break;
   	case 'i':
		k = pattern;
		for (j=1; j <= MAX_LENGTH; j++)      /* filling */
	   	   *mptr++ = (u_char)k++;
		break;
   	case 'd':
		k = pattern;
		for (j=1; j <= MAX_LENGTH; j++)      /* filling */
	   	   *mptr++ = (u_char)k--;
		break;
   	case 'r':
		for (j=1; j <= MAX_LENGTH; j++)      /* filling */
	   	   *mptr++ = (u_char)random();
		break;
   }
}

/*
 * -----------------------------------------------------------------------
 * this subroutine displays the statistic 
 * -----------------------------------------------------------------------
 */

statistics()
{
  int io_op;
  int ss;

	if (simulate_error == NO_STAT_SOCKET) sock_type = 7;
	else sock_type = SOCK_DGRAM;
	if ((ss = socket(AF_INET, sock_type, 0)) < 0) {
	   perror("dcptest: socket, perror says");
	   throwup(0, 
	      "dcptest: Couldn't open a socket for statistics, %s.", device);
 	   if (simulate_error != NO_STAT_SOCKET) return;
	}
#ifdef FAKE_IT
	if (simulate_error != NO_STAT_SOCKET) {
	   if (ss) close(ss);
	   return;
	}
#endif
	strcpy(ifr.ifr_name, device);
	if (ioctl(s, SIOCGETSYNC, &ifr)) {
	   perror("dcptest: SIOCGETSYNC, perror says");
	   fprintf(stderr, 
	      "dcptest: Couldn't get sync mode statistics for %s.\n", device);
	}
	sm = *(struct syncmode *)ifr.ifr_data;
	strcpy(ifr.ifr_name, device);
	if (ioctl(s, SIOCSSDSTATS, &ifr)) {
	   perror("dcptest: SIOCSSDSTATS, perror says");
	   fprintf(stderr, 
	      "dcptest: Couldn't get data statistics for %s.\n", device);
	}
	sd = *(struct ss_dstats *)ifr.ifr_data;
	strcpy(ifr.ifr_name, device);
	if (ioctl(s, SIOCSSESTATS, &ifr)) {
	   perror("dcptest: SIOCSSESTATS, perror says");
	   fprintf(stderr, 
	      "dcptest: Couldn't get error statistics for %s.\n", device);
	}
	if (ss) close(ss);
	se = *(struct ss_estats *)ifr.ifr_data;
	
	printf("\n%s: ", device);
	printf("baud %d", sm.sm_baudrate);
	printf("  pkts: i %d", sd.ssd_ipack);
	printf("  o %d", sd.ssd_opack);
	printf("  chars: i %d", sd.ssd_ichar);
	printf("  o %d\n", sd.ssd_ochar);
	printf("       underruns %d", se.sse_underrun);
	printf("  overruns %d", se.sse_overrun);
	printf("  aborts %d", se.sse_abort);
	printf("  crcs %d", se.sse_crc);
	printf("  error frames %d\n\n", err_frame_no);
}

	
/*
 * -------------------------------------------------------------
 *  set default parameters for the dcptest
 * -------------------------------------------------------------
 */
init_parm()
{
	int i, disp;

        pattn_type = 'r';
        pattern = 0x5a;
        protocol = SDLC_PROTOCOL;
	fid = 0;
	s = 0;
	loopback_flag = FALSE;
	baudrate = 9600;
	if (internal_loopback) clock_type = '0';
	else clock_type = '1';
	loop_count = 1;
	if (debug || ttya_test || simulate_error) {
	   min_frame_len = 1;
	   max_frame_len = 3;
	}
   	else {
#ifdef FAKE_IT
	   min_frame_len = 1;
	   max_frame_len = 3;
#else
	   if (internal_loopback) {
	      min_frame_len = 1;
	      max_frame_len = 255;
	   }
	   else {
	      min_frame_len = MAX_LENGTH;
	      max_frame_len = MAX_LENGTH;
	      if (!load_test) loop_count = 100;
	   }
#endif
	}
	disp = FALSE;
	if (load_dcp_kernal) {
		load_dcp_a = TRUE;
		load_dcp_b = TRUE;
		load_dcp_c = TRUE;
		load_dcp_d = TRUE;
		disp = TRUE;
	}
	for (i = 0; i < 4; i++) {
	   attach_a[i] = disp;
	   attach_b[i] = disp;
	   attach_c[i] = disp;
	   attach_d[i] = disp;
        }
	lb_mptr = "yes";
	nrzi_mptr = "no";
	txc_mptr = "baud";
	rxc_mptr = "baud";
	ssm->sm_txclock = TXC_IS_BAUD;
	ssm->sm_rxclock = TXC_IS_BAUD;
	ssm->sm_loopback = 1;
	ssm->sm_nrzi = 0;
	
}

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
   if (atn || verbose) {
      sprintf(tmp_msg, " pass %d,", pass);
      return TRUE;
   }              
   else return FALSE;
}                 
                  
finish()
{
   if (debug && !ttya_test) statistics();
   if (atn || verbose) {
      strcpy(device, "");
      sprintf(msg, "Stopped, pass %d, errors %d.", pass, errors);
      throwup(0, "%s: %s", TEST_NAME, msg);
      if (atn ) {
         send_msg_to_atn(INFO, msg);
         exit(0);
      }
   }
   exit(20);
}

#ifdef ATN_VERSION
#include "atnrtns.c"    /* ATN routines */
#else
send_msg_to_atn()
{
printf("This is not the ATN version!\n");
}
#endif
