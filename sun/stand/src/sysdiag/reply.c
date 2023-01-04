/* 
 *
 *	attempts to see if machine(s) are alive by ICMP echoing it for
 *      timeout seconds (default is 20)
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <netinet/in_systm.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <signal.h>
#include <sys/wait.h>
#include <ctype.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <net/if.h>


#define NET_FILES_INCLUDED	0
#define FALSE			0
#define TRUE			~FALSE
#define INFO			0
#define WARNING			1
#define FATAL			2
#define ERROR			3
#define NO_SD_LOG_DIR		1
#define NO_OPEN_LOG		2
#define NO_HOST_NAME		3
#define NO_SOCKET		4
#define NO_TRANSMIT_BC		5
#define NO_TRANSMIT		6
#define TRANSMIT_BC_TIMEOUT	7
#define TRANSMIT_TIMEOUT	8
#define NO_BC_HOST		9
#define RECEIVE_BC_TIMEOUT	10
#define RECEIVE_TIMEOUT		11
#define RECVFROM_ERROR		12
#define PACKETSIZE_ERROR	13
#define ICMP_TYPE_ERROR		14
#define COMPARE_ERROR		15
#define NO_RECEIVED_HOST_NAME	16
#define NO_BC_RESPONSE		17
#define NO_AUTO_ADR		18
#define NO_XMIT_SOCKET		19
#define NO_NETWORKS		21
#define NO_SELECTED_NETWORK	22
#define END_ERROR               23
#define USAGE_ERROR             99
#define TEST_NAME		"reply"
#define LOGFILE_NAME		"log.reply.XXXXXX"

#define PACKETSIZE		16	/* old kernel bug needs this */
#define MAX_BROADCAST_SIZE 1400

#define T_BC_WAIT_TIME		30	/* broadcast xmit timeout wait time */
#define T_WAIT_TIME		30	/* transmit timeout wait time */
#define R_BC_WAIT_TIME		30	/* broadcast receive time out value */
#define R_WAIT_TIME		60	/* receive time out value */
#define SYSDIAG_PASSES		4	/* # of passed for one sysdiag pass */

#define SOFT_ERROR_THRESHOLD	50	/* flag hard error if exceeded */

char device_name[15];
char *device = device_name;
int  atn = FALSE;
int sending_atn_msg = FALSE;
int  debug = FALSE;
int  pass = 0;
int errors = 0;
int verbose = TRUE;
int  stop_on_error = TRUE;

int  logfd = 0, failed = 0;
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

static char     sccsid[] = "@(#)reply.c 1.1 9/25/86 Copyright 1985 Sun Micro";

	/* normal execution variables */
FILE *fp;
int continous_soft_errors = 0;
int soft_errors = 0;
char **host, **host_argv;
int argc_t, argc_tmp;
char auto_buf[257][20];	/* assumming a host name no longer that 20 char */
char tmp_buf[20];
char *tmp_bf = tmp_buf;
char hname[20];
int hnameln = 20;
int err_cnt[257];
int exe_cnt[257];
char reply_buf[PACKETSIZE];
char indx = 0, tmp_adr;
int tmp;
int prcnt = 100;
int packetsize = PACKETSIZE;
char auto_bcast = TRUE; 
int noanswer();
int continue_after_timeout;
int replies_received;
int transmit_routine_timeout();
int testing_net = 0;
int atn_starting_board = 0;
int atn_boards_to_test = 0;
int atn_use_0 = FALSE;
int sock_type;
int sysdiag_passes;

caddr_t adrtostr();
caddr_t inet_ntoa();
u_long inet_network();
struct in_addr adr_in, inet_makeaddr(), addrs[20];
int s, fromlen, size, adr;
struct hostent *hp, *hpb;
struct icmp *icp = (struct icmp *)reply_buf;
struct sockaddr_in to, from;
struct timeval time1, time2, b_time;
char inbuf[MAX_BROADCAST_SIZE];

/*
 *	Parse command line to set up mode and execution flags
 */

main(argc,argv)
int     argc;
char    *argv[];

{						
   int arrcount, match, nets;
   extern   finish();

   signal(SIGHUP, finish);
   signal(SIGTERM, finish);
   signal(SIGINT, finish);

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
   strcpy(device, "enet0");

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
                        if (argv[arrcount][0] == 'e' && 
						argv[arrcount][1] != 'n') {
                                simulate_error = atoi(&argv[arrcount][1]);
                                if (simulate_error > 0 &&
						simulate_error < END_ERROR) {
                                   match = TRUE;
                                }
                        }
                        if (strncmp(argv[arrcount], "enet", 4) == 0) {
                                testing_net = atoi(&argv[arrcount][4]);
                                if (testing_net >= 0 && testing_net < 4) {
                                   match = TRUE;
				   strcpy(device, argv[arrcount]);
				}
				if (testing_net == 10) {
				   match = TRUE;
				   atn_use_0 = TRUE;
				   testing_net = 1;
				   strcpy(device, "enet1");
				}
                        }
                        if (!match) {
                           printf("Usage: %s [v] [sd/atn] [enet{0-3}] [re] [lt] [dd] [d] [e{1-%d}]\n", TEST_NAME, END_ERROR - 1);
                           exit(USAGE_ERROR);
                        }
      }

   if (atn) {
      strcpy(device, "");
      atn_boards_to_test = testing_net;
      if (atn_boards_to_test > 0)
	 if (!atn_use_0) atn_starting_board = 1;
      if (debug) printf("Testing %d board(s).\n",
		         atn_boards_to_test? atn_boards_to_test:1);
   }
   else if (debug) printf("Testing network %d.\n", testing_net);

   if (simulate_error == NO_SOCKET) sock_type = 7;
   else sock_type = SOCK_RAW;
   if ((s = socket(AF_INET, sock_type, 0)) < 0) {
      perror("reply: socket, perror says");
      if (!verify) {
         if (atn) send_msg_to_atn(FATAL, "Couldn't open a socket.");
         throwup(-NO_SOCKET, "%s: Couldn't open a socket.", TEST_NAME);
      }
      printf("%s: Couldn't open a socket.\n", TEST_NAME);
      exit(0);
   }
   nets = getbroadcastnets(addrs, s, inbuf);
   close(s);

   if (verify) { 				            /* verify mode */
      if (verbose) 
	 printf("%s: Located %d ethernet board(s).\n", TEST_NAME, nets);
      exit(nets);
   }
   if (simulate_error == NO_NETWORKS) nets = 0;
   if (nets < 1) {
     if (atn) send_msg_to_atn(FATAL, "No networks located.");
     throwup(-NO_NETWORKS, "%s: No networks located.", TEST_NAME);
   }
   if (simulate_error == NO_SELECTED_NETWORK) nets = 0;
   if (testing_net > nets - 1) {
     if (atn) {
	send_msg_to_atn (FATAL, "%d board(s) selected to test, %d located.",
	       	    atn_boards_to_test? atn_boards_to_test:1, nets? nets -1:0);
        throwup(-NO_SELECTED_NETWORK, 
           "%s: %d board(s) selected to test, %d located.", TEST_NAME,
	    atn_boards_to_test? atn_boards_to_test:1, nets? nets -1:0);
     }
     throwup(-NO_SELECTED_NETWORK, 
             "%s: Selected network '%s' not located.", TEST_NAME, device);
   }
   host = host_argv = argv;	/* com line arg manipulation spares */
   argc_t = argc_tmp = argc;

   if (verbose && testing_net == 0) {
      if (gethostname(hname, hnameln) < 0 || simulate_error == NO_HOST_NAME) {
        perror("reply: gethostname failed, perror says");
        if (atn) send_msg_to_atn
           (FATAL, "No hostname available, must be set by UNIX.");
        throwup(-NO_HOST_NAME, 
		"%s: No hostname available, must be set by UNIX.", TEST_NAME);
      }
   }

   main_loop();
}

/*
 *	Sign on and enter the main execution loop for the kind of test being
 *	executed. i.e. auto-configure.
 */
			
main_loop()
{
   register char i, asmpl;
   int no_auto_adr = 0;
   extern   get_auto_adr_timeout();

   auto_buf[0][0] = '\0';
	
   if (atn || verbose) {           
       if (atn) {
          send_msg_to_atn(INFO, 
		    "Started, %d board(s) to test.",
		     atn_boards_to_test? atn_boards_to_test:1);
          throwup(0, "%s: Started, %d board(s) to test.",
		      TEST_NAME, atn_boards_to_test? atn_boards_to_test:1);
       }
       else throwup(0, "%s: Started on %s.", TEST_NAME, device);
   }
   if (debug) sysdiag_passes = 1;
   else sysdiag_passes = SYSDIAG_PASSES;

   while (atn || pass < sysdiag_passes) {
      if (atn && no_auto_adr == 0) {
	 auto_buf[0][0] = '\0';
	 indx = 0;
	 if (pass == 0) testing_net = atn_starting_board;
	 else {
	    testing_net++;
	    if (testing_net > atn_boards_to_test) 
	       testing_net = atn_starting_board;
	 }
	 sprintf(device, "enet%d", testing_net);
         if (debug) printf("Started on %s.\n", device);
	 if (testing_net == atn_starting_board) pass++;
      }
      else if (no_auto_adr == 0) pass++;
      auto_bcast = TRUE; /* do bcast and log response */
      get_bcast_adr();
      netwrk();  
      auto_bcast = FALSE; /* test the respondees */
      if (!verbose && !load_test) sleep(5);
      for (asmpl = 0; auto_buf[asmpl][0] != '\0'; ++asmpl);
      for (i = 0; i < asmpl; ++i) { 
         signal(SIGALRM, get_auto_adr_timeout);
         if (simulate_error == NO_AUTO_ADR) {
            alarm(1);
            while(1) { }
         }
         alarm(300);
         if (!get_auto_adr()) {
	    no_auto_adr++; 
            alarm(0);
	 } else {
            alarm(0);
	    no_auto_adr = 0;
            netwrk();  
            if (load_test) break;	
            ++indx;
         }
      }
      if (load_test) break;	
      if (no_auto_adr == 0) {
         if (atn || verbose) printf("%s: %s, pass %d, errors %d.\n", 
				    TEST_NAME, device, pass, errors);
         if (!verbose) sleep(30);
      } else {
         if (no_auto_adr > 10) {
            errors++;
            if (atn) send_msg_to_atn (ERROR, 
              "No replies to broadcast echo packet on %s, pass %d, errors %d.", 
               device, pass, errors);
            throwup(-NO_BC_RESPONSE, "ERROR: %s, no replies to broadcast echo packet on %s,%s errors %d.", TEST_NAME, device, print_pass()? tmp_msg:"", errors);
         }
      }

   }      /* end of while (atn || pass < 5) */   

   if (atn || verbose) throwup(0, "%s: Stopped %s, pass %d, errors %d.", 
				   TEST_NAME, device, pass, errors);
   exit(0);
} /* end of main_loop() */

/*
 *	Set up the information (data structures, packets)
 *	needed for inter-process communication
 */

netwrk()
{

   tmp_adr = inet_lnaof(adr); /* cnvrt test host # to 1 to 255 array offset
					for exe and error tabulation  */
   exe_cnt[tmp_adr]++;
				/* create the transmit socket */
   if (simulate_error == NO_XMIT_SOCKET) sock_type = 7;
   else sock_type = SOCK_RAW;
   if ((s = socket(AF_INET, sock_type, 0)) < 0) {
     perror("reply: socket, perror says");
     if (atn) send_msg_to_atn(FATAL, 
	     "Couldn't create a transmit socket for %s, pass %d, errors %d.",
              device, pass, errors);
     throwup(-NO_XMIT_SOCKET, 
	      "%s: Couldn't create a transmit socket for %s,%s errors %d.", 
               TEST_NAME, device, print_pass()? tmp_msg:"", errors);
   }
   to.sin_family = AF_INET;
   to.sin_port = 0;
   to.sin_addr.s_addr = adr;
   gettimeofday(&time2, 0);
   b_time = time2;
   if (auto_bcast) bcopy(&time2, &b_time, sizeof(struct timeval));
				   /* create the transmit packet (raw) */
   bcopy(&time2, reply_buf + 8, sizeof(struct timeval));
   icp->icmp_type = ICMP_ECHO;
   icp->icmp_code = 0;
   icp->icmp_cksum = 0;
   icp->icmp_id = 1;
   icp->icmp_seq = 1;
   icp->icmp_cksum = in_cksum(icp, packetsize);

   do_it();

   close(s);
}

/*
 *	transmit and receive echo messages
 */

do_it()
{
   int simulate_transmit_timeout = FALSE;
   int simulate_receive_timeout = FALSE;

   	
   if (debug) {
      if (auto_bcast) printf("\nStarting BC xmit %s *****\n", device);
      else printf("\nStarting xmit %s\n", device);
   }
   if (simulate_error != 0) {
      if (auto_bcast && simulate_error == NO_TRANSMIT_BC) s = 0;
      if (!auto_bcast && simulate_error == NO_TRANSMIT) s = 0;
      if (auto_bcast && simulate_error == TRANSMIT_BC_TIMEOUT) 
	 simulate_transmit_timeout = TRUE;
      if (!auto_bcast && simulate_error == TRANSMIT_TIMEOUT) 
	 simulate_transmit_timeout = TRUE;
      if (auto_bcast && simulate_error == RECEIVE_BC_TIMEOUT) 
	 simulate_receive_timeout = TRUE;
      if (!auto_bcast && simulate_error == RECEIVE_TIMEOUT) 
	 simulate_receive_timeout = TRUE;
   }

   signal(SIGALRM, transmit_routine_timeout);
   if (simulate_transmit_timeout) {
      alarm(1);
      while(1) { }
   }
   alarm(auto_bcast == TRUE ? T_BC_WAIT_TIME : T_WAIT_TIME);

   if (!simulate_receive_timeout) {
      if (sendto(s, icp, packetsize, 0, &to, sizeof(to)) != packetsize) {
           alarm(0);
	   errors++;
     	   perror("reply: sendto, perror says");
     	   if (atn) send_msg_to_atn (ERROR, 
                 "Transmit failed on %s, %secho packet, pass %d, errors %d.", 
                 device, auto_bcast == TRUE ? "broadcast " : "", pass, errors);
     	   throwup(auto_bcast == TRUE ? -NO_TRANSMIT_BC : -NO_TRANSMIT, 
               "ERROR: %s, transmit failed on %s, %secho packet,%s errors %d.", 
                TEST_NAME, device, auto_bcast == TRUE ? "broadcast " : "", 
                print_pass()? tmp_msg:"", errors);
      }
   }
   alarm(0);
   if (debug) printf("Exiting xmit %s\n", device);

   receive_echo_reply();

}

/*
 *	receive echo messages
 */

receive_echo_reply()

{
   continue_after_timeout = FALSE;
   replies_received = 0;

   signal(SIGALRM, noanswer);

   while(1) {

/* verify that the packet received ok and matches transmit packet */

      while (1) {
         fromlen = sizeof(from);
         if (debug) printf("Starting receive %s\n", device);
	 if (simulate_error == RECVFROM_ERROR) s = 0;
	 alarm(auto_bcast == TRUE ? R_BC_WAIT_TIME : R_WAIT_TIME);
         if ((size = recvfrom(s, reply_buf, sizeof(reply_buf), 0, &from, &fromlen)) < 0) {
	    alarm(0);
	    if (continue_after_timeout) return;
	    soft_fail("recvfrom");
	    continue;
         }
         alarm(0);
         if (debug) printf("Exiting receive %s\n", device);
         if (size != packetsize || simulate_error == PACKETSIZE_ERROR) {
	    soft_fail("packetsize");
	    continue;
         }
         if (icp->icmp_type != ICMP_ECHOREPLY || 
			       simulate_error == ICMP_TYPE_ERROR) {
            soft_fail("icmp-type");
	    continue;
         }
         if (bcmp(&time2, reply_buf + 8, sizeof(struct timeval)) != 0 || 
					    simulate_error == COMPARE_ERROR) {
	    soft_fail("compare");
	    continue;
         }
	
	 replies_received++; 
	 continous_soft_errors = 0;
				   /* save the responding hosts name */
         if (auto_bcast == TRUE) {
            strcpy(tmp_bf, adrtostr(from.sin_addr));
	    insert();
	    if (load_test) return;
            break;
         }	
         else {  /* echo responding hosts name and percentage pass count */
            tmp =((exe_cnt[tmp_adr]-(err_cnt[tmp_adr]))*100)/exe_cnt[tmp_adr];
            if (verbose) {
	       printf("%s has replied on %s, passes = %d%% %s", 
                         adrtostr(from.sin_addr), device, tmp,
			 (prcnt > tmp) ? "<============\n" : "\n");
            }
            return;
         }
      }  /* end of second while (1) */
   }  /* end of first while (1) */
}

/*
 *	soft error routine for receive packets
 */

soft_fail(error_type)
char *error_type;
{   
   struct timeval reply_data;

   if (debug) {
	soft_errors++;
	if (strcmp(error_type, "recvfrom") == 0) 
	   perror("reply: recvfrom, perror says");
	throwup(0, "%s: Soft error - (%s) on %s, from '%s', pass %d, errors %d, soft errors %d.", TEST_NAME, error_type, device, adrtostr(from.sin_addr), pass, errors, soft_errors);
	if (strcmp(error_type, "compare") == 0) {
	   bcopy(reply_buf + 8, &reply_data, sizeof(struct timeval));
	   fprintf(stderr, "Data was =       '%x%x', %s", 
		reply_data.tv_sec, reply_data.tv_usec, ctime(&reply_data));
	   fprintf(stderr, "Expected =       '%x%x', %s", 
			time2.tv_sec, time2.tv_usec, ctime(&time2));
	   fprintf(stderr, "Broadcast time = '%x%x', %s", 
		b_time.tv_sec, b_time.tv_usec, ctime(&b_time));
	}
    }

    if (!auto_bcast && strcmp(error_type, "compare") == 0 && 
	  bcmp(&b_time, reply_buf + 8, sizeof(struct timeval)) == 0) return; 
    continous_soft_errors++;
    if (continous_soft_errors >= SOFT_ERROR_THRESHOLD) {
       if (strcmp(error_type, "recvfrom") == 0 && !debug) 
	  perror("reply: recvfrom, perror says");
       if (atn) send_msg_to_atn (ERROR, "%s soft error threshold exceeded (%s), pass %d, errors %d, soft errors %d.", device, error_type, pass, errors, continous_soft_errors);
       throwup(-39, "ERROR: %s, %s soft error threshold exceeded (%s),%s errors %d, soft errors %d.", TEST_NAME, device, error_type, print_pass()? tmp_msg:"", errors, continous_soft_errors);
    }
}


/*
 *	Insert the host name into the auto-configure queue.
 *	Do the insert into the first empty slot.
 */

insert()
{
register char i, a;

   i = 0;
   for (;;) {	/* is host already in queue? */
	for (; tmp_buf[0] == auto_buf[i][0]; ++i) {
	   for (a=1; tmp_buf[a] == auto_buf[i][a]; ++a) {
		if ((tmp_buf[a] == '\0') && auto_buf[i][a] == '\0') 
		return;
           }
	}
	if (auto_buf[i][0] == '\0') { 
	   strcpy(auto_buf[i], tmp_buf);
	   auto_buf[i+1][0] = '\0';
           if (debug) printf("%s broadcast response was from '%s', i = %d.\n",
			      device, auto_buf[i], i);
	   return;
	}
   ++i;
   }
} 

/*
 *	Convert the host name (or number) from the auto-configure queue to a
 *	network useable destination address.
 */

get_auto_adr()
{
int response_is_me = 0, in_adr, me;
struct in_addr addr_in;

beg:
   if (auto_buf[indx][0] == '\0') indx = 0; /* recirculate if at queues end */
  
   me = FALSE;
   if (isdigit(auto_buf[indx][0])) {
      in_adr = htonl(strtoi(auto_buf[indx]));
      addr_in = addrs[testing_net];

/* printf("in_adr = 0x%x, addr_in.s_addr = 0x%x\n", in_adr, addr_in.s_addr); */

      if (in_adr == addr_in.s_addr || simulate_error == NO_BC_RESPONSE) 
	 me = TRUE;
   }
   else if (strcmp(hname, auto_buf[indx]) == 0 || 
			      simulate_error == NO_BC_RESPONSE) me = TRUE;

   if (me) {
      response_is_me++;	
      if (response_is_me > 5) {
         if (debug) printf ("%s the only response was from '%s', indx = %d.\n",
	                     device, auto_buf[indx], indx);
         return FALSE;
      }
      ++indx;
      goto beg;
   } 

   if (isdigit(auto_buf[indx][0])) adr = htonl(strtoi(auto_buf[indx]));
   else {
      if ((hp = gethostbyname(auto_buf[indx])) == NULL || 
				     simulate_error == NO_RECEIVED_HOST_NAME) {
	 if (atn) send_msg_to_atn (INFO, 
	     "Can't find host '%s' in '/etc/hosts', %s, pass %d, errors %d.",
	      auto_buf[indx], device, pass, errors);
	 throwup(0, 
	    "%s: Can't find host '%s' in '/etc/hosts', %s,%s errors %d.",
	     TEST_NAME, auto_buf[indx], device, print_pass()? tmp_msg:"", 
	     errors);
      }
      else adr = *((int *)hp->h_addr);
   }
   return TRUE;	
}

/*
 *	Using the current hosts network, make a useable network
 *	broadcast address.
 */
 
get_bcast_adr()
{
/*
   if ((hpb = gethostbyname(hname)) == NULL || simulate_error == NO_BC_HOST) {
      if (atn) send_msg_to_atn (FATAL, 
	   "Can't find host '%s' in '/etc/hosts', %s, pass %d, errors %d.", 
            hname, device, pass, errors);
      throwup(-NO_BC_HOST, 
	      "%s: Can't find host '%s' in '/etc/hosts', %s,%s errors %d.", 
              TEST_NAME, hname, device, print_pass()? tmp_msg:"", errors);
   }
   adr_in = inet_makeaddr((inet_netof(*((int *)hpb->h_addr))), INADDR_ANY);
*/
   adr_in = inet_makeaddr((inet_netof(addrs[testing_net])), INADDR_ANY);
   adr = adr_in.s_addr;
}

/*
 *	Convert the network address into a host name
 */

char *
adrtostr(adr)
int adr;
{
	struct hostent *hp;
	char buf[400];		/* hope this is long enough */
	if (testing_net == 0 && verbose) 
            hp = gethostbyaddr(&adr, sizeof(adr), AF_INET);
	else hp = NULL;
	if (hp == NULL) {
	    	sprintf(buf, "0x%x", adr);
		return buf;
	}
	else
		return hp->h_name;
}

/*
 *	Generate a checksum for the transmit packet
 */

in_cksum(addr, len)
	u_short *addr;
	int len;
{
	register u_short *ptr;
	register int sum;
	u_short *lastptr;

	sum = 0;
	ptr = (u_short *)addr;
	lastptr = ptr + (len/2);
	for (; ptr < lastptr; ptr++) {
		sum += *ptr;
		if (sum & 0x10000) {
			sum &= 0xffff;
			sum++;
		}
	}
	return (~sum & 0xffff);
}

/*
 *	Entry point for signal that is invoked when a packet has not been
 *	transmitted in 'timeout' seconds.
 */

transmit_routine_timeout()
{
   errors++;
   if (atn) send_msg_to_atn 
	    (ERROR, "%sransmit timeout on %s, pass %d, errors %d.", 
             auto_bcast == TRUE ? "Broadcast t" : "T", device, pass, errors);
   throwup(auto_bcast == TRUE ? -TRANSMIT_BC_TIMEOUT : -TRANSMIT_TIMEOUT, 
                  "ERROR: %s, %sransmit timeout on %s,%s errors %d.", 
                  TEST_NAME, auto_bcast == TRUE ? "broadcast t" : "t",
                  device, print_pass()? tmp_msg:"", errors);
}
/*
 *	Entry point for signal that is invoked when a packet has not been
 *	received in 'timeout' seconds.
 */

noanswer()
{
   int tmp;
   char **arg;
   char *who;

   if (replies_received > 0) {
      continue_after_timeout = TRUE;
      if (debug) 
	 printf("Receive timeout on %s, %d replies to %secho.\n",
             device, replies_received, auto_bcast == TRUE ? "broadcast " : "");
   } else {
      if (auto_bcast == FALSE) {
         continue_after_timeout = TRUE;
         who = auto_buf[indx];
                                    /* calculate pass percentage count */

         tmp = ((exe_cnt[tmp_adr] - (err_cnt[tmp_adr] + 1)) * 100) / 
                                                             exe_cnt[tmp_adr];
         if (verbose) throwup(0, "%s: Receive timeout on %s, no reply from '%s', replies = %d%%, pass %d, error %d.", TEST_NAME, device, who, tmp, pass, errors);

      } else {
         errors++;
         if (atn) send_msg_to_atn (ERROR, "Receive timeout on %s, no replies to %secho, pass %d, errors %d.", device, auto_bcast == TRUE ? "broadcast " : "", pass, errors);
         throwup(-RECEIVE_BC_TIMEOUT, "ERROR: %s, receive timeout on %s, no replies to %secho,%s errors %d.", TEST_NAME, device, auto_bcast == TRUE ? "broadcast " : "", print_pass()? tmp_msg:"", errors);
      }
   }
}

/*
 *	timeout for get_auto_adr routine
 */

get_auto_adr_timeout()
{
   errors++;
   if (atn) send_msg_to_atn 
	     (ERROR, "Get address timeout on %s, pass %d, errors %d.", 
              device, pass, errors);
   throwup(-NO_AUTO_ADR, "ERROR: %s, get address timeout on %s,%s errors %d.", 
                  TEST_NAME, device, print_pass()? tmp_msg:"", errors);
}

/*
 *	Convert network address number format
 */

strtoi(str)
char *str;
{
	int ans;
	
	if (str[0] == '0' && (040|str[1]) == 'x')
		sscanf(str+2, "%x", &ans);
	else if (str[0] == '0')
		sscanf(str, "%o", &ans);
	else
		sscanf(str, "%d", &ans);
	return ans;
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
 
/*
 * This routine was borrowed from bcast.c
 */
/*
 * The following is kludged-up support for simple rpc broadcasts.
 * Someday a large, complicated system will replace these trivial
 * routines which only support udp/ip .
 */

static int
getbroadcastnets(addrs, sock, buf)
        struct in_addr *addrs;
        int sock;  /* any valid socket will do */
        char *buf;  /* why allocxate more when we can use existing... */
{
        struct ifconf ifc;
        struct ifreq ifreq, *ifr;
        struct sockaddr_in *sin;
        int n, i;
 
        ifc.ifc_len = MAX_BROADCAST_SIZE;
        ifc.ifc_buf = buf;
        if (ioctl(sock, SIOCGIFCONF, (char *)&ifc) < 0) {
          perror("reply: ioctl (get interface configuration), perror says");
          return (0);
        }
        ifr = ifc.ifc_req;
        for (i = 0, n = ifc.ifc_len/sizeof (struct ifreq); n > 0; n--, ifr++) {
                ifreq = *ifr;
                if (ioctl(sock, SIOCGIFFLAGS, (char *)&ifreq) < 0) {
                  perror("reply: ioctl (get interface flags), perror says");
                  continue;
                }
                if ((ifreq.ifr_flags & IFF_BROADCAST) &&
                    (ifreq.ifr_flags & IFF_UP) &&
                    ifr->ifr_addr.sa_family == AF_INET) {
                        sin = (struct sockaddr_in *)&ifr->ifr_addr;
                        addrs[i++] = sin->sin_addr;
                }
        }
        return (i);
}

/*
 *	End it!
 */

finish()
{
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
