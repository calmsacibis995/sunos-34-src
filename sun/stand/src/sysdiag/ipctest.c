#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <sys/file.h>
#include <signal.h>

#define FALSE			0
#define TRUE			~FALSE
#define INFO			0
#define WARNING			1
#define FATAL			2
#define ERROR			3
#define NO_SD_LOG_DIR		1
#define NO_OPEN_LOG		2
#define NO_START		3
#define IPC_TIMEOUT		4
#define NO_IPC_MSG_FILE         5
#define END_ERROR               6
#define IPC_INFO                10
#define IPC_WARNING             11
#define IPC_FATAL               12
#define IPC_ERROR               13
#define IPC_UNKNOWN             14
#define IPC_HANG             	15
#define IPC_END_ERROR          	16
#define USAGE_ERROR		99

#define TEST_NAME		"ipctest"
#define LOGFILE_NAME		"log.ipctest.XXXXXX"

char device_name[15];
char *device = device_name;
int  atn = FALSE;
int sending_atn_msg = FALSE;
int  debug = FALSE;
int  pass = 1;
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

static char     sccsid[] = "@(#)ipctest.c 1.1 9/25/86 Copyright 1985 Sun Micro";

#define PASS_LOG_TIME           60
struct  timeval current_time;
int     log_time = 0, log_elapse_time = 0;
int  sd_load_test = FALSE;

#define WAIT_FOR_MSG 		150
#define IPC_TIME_LIMIT 		1200    /* 1200 = 20 minutes */
#define WAIT_ON_LAST_MSG 	120     /* 120 = 4 minutes */
#define MAX_IPCS		4

FILE *af, *afo;
int aafo = 0;
char    testing_devices[40] = {""};
#define MAX_LENGTH      250
char ipc_buf[MAX_LENGTH];
char *ipc_msg = ipc_buf;
int atn_msg_type;
int return_code = 0;
char ipc_file_name[50];
char *ipc_msg_file = ipc_file_name;
char tmpbuf[100];
char sysdiag_directory[50];
char *sd_log_directory = sysdiag_directory;
char sd_hostname[20];
char *sysdiag_host = sd_hostname;
int ipc;
int ipc_elapse_time = 0;
int ipc_time_limit = IPC_TIME_LIMIT;
char pc_device[4];
int  test_args = FALSE;
int  use_ipc_script = FALSE;
int  ipc_simulate_error = FALSE;
int  user_terminated = FALSE;
int  wait_on_last_msg = WAIT_ON_LAST_MSG;
int  ipc_started = FALSE;

struct dev {
  char  device[5];
  int   selected;
  char  ipc_msg_to_sd[50];
  char  sd_msg_to_ipc[50];
  int   now;
  int	start_time;
  int	begin_msg_received;
  int   complete_pass;
  int   bump_pass_cntr;
  int   pass;
  int   errors;
  int   disk;
  int   parallel_port;
} testing[MAX_IPCS] = {
                          {
                          "ipc0",
                          FALSE,
			  "ipc0-sd",
			  "sd-ipc0",
                          FALSE,
			  0,
			  FALSE,
			  1,
                          TRUE,
			  0,
			  0,
			  FALSE,
			  FALSE
                          },
                          { 
                          "ipc1",
                          FALSE,
			  "ipc1-sd",
			  "sd-ipc1",
                          FALSE,
			  0,
			  FALSE,
			  1,
                          TRUE,
			  0,
			  0,
			  FALSE,
			  FALSE
                          },
                          { 
                          "ipc2",
                          FALSE,
			  "ipc2-sd",
			  "sd-ipc2",
                          FALSE,
			  0,
			  FALSE,
			  1,
                          TRUE,
			  0,
			  0,
			  FALSE,
			  FALSE
                          },
                          { 
                          "ipc3",
                          FALSE,
			  "ipc3-sd",
			  "sd-ipc3",
                          FALSE,
			  0,
			  FALSE,
			  1,
                          TRUE,
			  0,
			  0,
			  FALSE,
			  FALSE
                          }
                      };

main(argc, argv)
int	argc;
char	*argv[];
{
   int arrcount, match, i, j;
   extern interrupt();

   signal(SIGHUP, interrupt);
   signal(SIGTERM, interrupt);
   signal(SIGINT, interrupt);

   if (getenv("SYSDIAG_HOST"))
	  strcpy(sysdiag_host, (getenv("SYSDIAG_HOST")));
   else strcpy(sysdiag_host, "no host name");
   if (getenv("SD_LOAD_TEST"))
          if (strcmp(getenv("SD_LOAD_TEST"), "yes") == 0) {
             sd_load_test = TRUE;
	     test_args = TRUE;
	  }
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

   strcpy(testing_devices, "");
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
				test_args = TRUE;
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
				sd_load_test = TRUE;
				test_args = TRUE;
                        }
                        if (strcmp(argv[arrcount], "re") == 0) {
                                match = TRUE;
                                stop_on_error = FALSE;
                        }
                        if (argv[arrcount][0] == 'e') {
                                simulate_error = atoi(&argv[arrcount][1]);
				if (simulate_error > 0 && 
				                simulate_error < END_ERROR) { 
				   match = TRUE;
				}
				else if ((simulate_error >= IPC_WARNING &&
					    simulate_error < IPC_END_ERROR) ||
					    (simulate_error >= 30 &&
					    simulate_error <= 99)) {
				   match = TRUE;
				   ipc_simulate_error = simulate_error;
				   test_args = TRUE;
				}
                        }
                        if (strcmp(argv[arrcount], "us") == 0) {
                                match = TRUE;
                                use_ipc_script = TRUE;
                        }
                        if (strncmp(argv[arrcount], "ipc", 3) == 0) {
                                if (argv[arrcount][3] >= '0' && 
						argv[arrcount][3] < '4') {
                                   match = TRUE;
				   ipc = atoi(&argv[arrcount][3]);
				   strcat(testing_devices, " ");
				   strcat(testing_devices, argv[arrcount]);
				   testing[ipc].selected = TRUE;
                                }
                        }
			if (strncmp(argv[arrcount], "d", 1) == 0) {
                                if (argv[arrcount][1] >= '0' && 
						argv[arrcount][1] < '4') {
                                   match = TRUE;
				   ipc = atoi(&argv[arrcount][1]);
				   testing[ipc].disk = TRUE;
				   test_args = TRUE;
                                }
			}
			if (strncmp(argv[arrcount], "pp", 2) == 0) {
                                if (argv[arrcount][2] >= '0' && 
						argv[arrcount][2] < '4') {
                                   match = TRUE;
				   ipc = atoi(&argv[arrcount][2]);
				   testing[ipc].parallel_port = TRUE;
				   test_args = TRUE;
                                }
			}

                        if (!match) {
                           printf("Usage: %s [v] [sd/atn] [ipc{0-3}] [d{0-3}] [pp{0-3}] [re] [lt] [dd] [d] [us] [e{1-%d/%d-%d/30-99}]\n", TEST_NAME, END_ERROR - 1, IPC_WARNING, IPC_END_ERROR -1);
                           exit(USAGE_ERROR);
                        }
      }

   if (strcmp(testing_devices, "") == 0) {
      strcpy(testing_devices, " ipc0");
      testing[0].selected = TRUE;
   }


   if (verify) {               /* verify mode */
     if (verbose)
        printf("%s: Verify mode, devices are%s.\n", TEST_NAME, testing_devices);
     exit(0);
   }
   if (simulate_error == IPC_TIMEOUT) {
      ipc_time_limit = 5;
      wait_on_last_msg = 5;
   }
   else if (simulate_error == IPC_HANG) {
      if (strlen(testing_devices)  < 8) ipc_time_limit = 20;
      else if (strlen(testing_devices) < 13) ipc_time_limit = 30;
      else if (strlen(testing_devices) < 19) ipc_time_limit = 40;
      else ipc_time_limit = 50;
      wait_on_last_msg = 5;
   }

   sprintf(msg, "Started on%s.", testing_devices);
   if (atn) send_msg_to_atn(INFO, msg);
   throwup(0, "%s: %s", TEST_NAME, msg);

   for (ipc = 0; ipc < MAX_IPCS; ipc++) {
      strcpy(ipc_msg_file, sd_log_directory);
      strcat(ipc_msg_file, testing[ipc].ipc_msg_to_sd);
      strcpy(testing[ipc].ipc_msg_to_sd, ipc_msg_file);
      unlink(testing[ipc].ipc_msg_to_sd);
/*      printf("Remove ipc file %s\n", testing[ipc].ipc_msg_to_sd); */
      strcpy(ipc_msg_file, sd_log_directory);
      strcat(ipc_msg_file, testing[ipc].sd_msg_to_ipc);
      strcpy(testing[ipc].sd_msg_to_ipc, ipc_msg_file);
      unlink(testing[ipc].sd_msg_to_ipc);
/*      printf("Remove sd  file %s\n", testing[ipc].sd_msg_to_ipc); */
   }

   if (simulate_error == NO_IPC_MSG_FILE) 
      strcpy(testing[0].ipc_msg_to_sd, "no/ipc/msg.file");

   for (ipc = 0; ipc < MAX_IPCS; ipc++) {
      if (testing[ipc].selected) testing[ipc].complete_pass = 0;
   }

   gettimeofday(&current_time, 0);
   log_time = current_time.tv_sec;

   while (1) {

      for (ipc = 0; ipc < MAX_IPCS; ipc++) {

	 if (testing[ipc].bump_pass_cntr) {
	    testing[ipc].pass++;
	    testing[ipc].bump_pass_cntr = FALSE;
	 }

         if (testing[ipc].selected) {
	    strcpy(device, testing[ipc].device);
	    if (!testing[ipc].now) {
	       if (user_terminated) finish();
	       if (debug) printf("Starting %s test.\n", device);
	       sprintf(tmp_msg, "e%d ", ipc_simulate_error);
	       sprintf(msg, "%s%s%s%s%s%s%s%s%s",
	       		     atn? "atn ":"", 
	       		     exec_by_sysdiag? "sd ":"", 
	       		     testing[ipc].disk? "disk ":"",
	       		     testing[ipc].parallel_port? "pp ":"",
	       		     sd_load_test? "lt ":"",
	       		     debug? "d ":"", 
	       		     display_read_data? "dd ":"", 
	       		     stop_on_error? "":"re ", 
	       		     ipc_simulate_error? tmp_msg:"");
	       if (strcmp(msg, "") != 0) {
	          afo = fopen(testing[ipc].sd_msg_to_ipc, "w");
	          fprintf(afo, "%s", msg);
	          fclose(afo);
		  chmod(testing[ipc].sd_msg_to_ipc, 00777);
	       }
	       if (simulate_error != NO_START) {
	          if (use_ipc_script) {
	             sprintf(tmpbuf, "ipc %s &", device);
	          }
	          else {
		     strncpy(pc_device, device + 1, 3);
	             sprintf(tmpbuf, "pctool -d /dev/%s -c \"pc %d %s\" &", 
				      pc_device, ipc, sysdiag_host);
	          }
	       }
	       else sprintf(tmpbuf, "invalid.test %s", device);
	       if (debug) printf("Executing %s\n", tmpbuf);
	       if (user_terminated) finish();
	       if (simulate_error != IPC_TIMEOUT) {
                  if (system(tmpbuf)) {
	             sleep(1);
	             if (user_terminated) finish();
		     sprintf(msg, "%s, couldn't start device test.", device);
		     if (atn) send_msg_to_atn(FATAL, msg);
		     throwup(stop_on_error? -NO_START:NO_START, 
			     "%s: %s", TEST_NAME, msg);
		     ipc_started = FALSE;
                  }
		  else ipc_started = TRUE;
	       }
	       if (ipc_started || simulate_error == IPC_TIMEOUT) {
	          if (debug) 
		     printf("Returned after starting %s test.\n", device);
	          testing[ipc].now = TRUE;
	          gettimeofday(&current_time, 0);
	          testing[ipc].start_time = current_time.tv_sec;
	          testing[ipc].begin_msg_received = FALSE;
		  if (user_terminated) finish();
	       }
	    }
	    check_for_ipc_msg(ipc);

	    if (testing[ipc].now) {
	       gettimeofday(&current_time, 0);
	       ipc_elapse_time = current_time.tv_sec - testing[ipc].start_time;
	       if (ipc_elapse_time > ipc_time_limit) {
	          testing[ipc].now = FALSE;
	          sprintf(msg, "%s, the time limit for the test has been exceeded, the test start message was %sreceived, pass %d, errors %d.", device, testing[ipc].begin_msg_received? "" : "not ", testing[ipc].pass, testing[ipc].errors);
                  if (atn) send_msg_to_atn(FATAL, msg);
	          throwup(stop_on_error? -IPC_TIMEOUT:IPC_TIMEOUT, 
		          "%s: %s", TEST_NAME, msg);
	       }
	    }

	    if (user_terminated) finish();

	    if (testing[0].complete_pass >= 1 && testing[1].complete_pass >= 1
		  			&& testing[2].complete_pass >= 1 
		  			&& testing[3].complete_pass >= 1) {
	       for (i = 0; i < MAX_IPCS; i++) {
		  if (testing[i].selected) {
                     testing[i].complete_pass--;
                  }
               }

               gettimeofday(&current_time, 0);
               log_elapse_time = current_time.tv_sec - log_time;
               if (getenv("LOG_PASS_MSG") && 
		                       log_elapse_time >= PASS_LOG_TIME * 60){
	          strcpy(ipc_msg, "");
	          for (i = 0; i < MAX_IPCS; i++) {
		     if (testing[i].selected) {
	                sprintf(tmp_msg, " %s: pass %d, errors %d.", 
		                testing[i].device, testing[i].pass, 
				testing[i].errors);
	                strcat(ipc_msg, tmp_msg);
                     }
                  }
                  throwup(0, "%s: pass %d, errors %d.%s", 
			      TEST_NAME, pass, errors, ipc_msg);
                  log_time = current_time.tv_sec;
               }  
               else printf("%s: pass %d, errors %d.\n", 
			    TEST_NAME, pass, errors);
	       pass++;
	    }
	    if (load_test) finish();
            if (!verbose) sleep (5);
         }
      }
   }
}
check_for_ipc_msg(ipc)
int ipc;
{	
   char *ipc_msg = ipc_buf;
   int null_file_cntr = 0;

/*   if (debug) printf("Entering check_for_ipc_msg for %d.\n", ipc); */
   if ((af = fopen(testing[ipc].ipc_msg_to_sd, "r")) != NULL) {
/*    if (debug) printf("Found ipc msg for %d, file %s, af = %d.\n", ipc, testing[ipc].ipc_msg_to_sd, af); */
      sleep(3); 
      *ipc_msg = '\0';
      while (*ipc_msg == '\0' && null_file_cntr < WAIT_FOR_MSG) {
         fgets(ipc_msg, MAX_LENGTH, af);
	 if (*ipc_msg == '\0') {
	    rewind(af);
	    null_file_cntr++;
	    if (debug) printf("%s: Null message file, pass %d.\n",
			       device, null_file_cntr);
 	    sleep(2); 
	 }
      }
/*
   if (debug) printf("After fgets for %d, af = %d.\n", ipc, af);
   if (debug) printf("ipc msg was: %s\n", ipc_msg);
*/
      fclose(af);
/*
   if (debug) printf("After fclose for %d.\n", ipc);
*/
      unlink(testing[ipc].ipc_msg_to_sd);
/*
   if (debug) printf("After unlink for %d.\n", ipc);
*/
      *(ipc_msg + (strlen(ipc_msg) -1)) = '\0';
      if (debug) printf("%s, ipc msg was: %s\n", device, ipc_msg);

      if (strncmp(ipc_msg, "INFO:", 5) == 0) {
         atn_msg_type = INFO;
         ipc_msg += 6;
         return_code = IPC_INFO;
         if (strncmp(ipc_msg, "Beginning", 9) == 0) {
	    testing[ipc].begin_msg_received = TRUE;
         }
         else if (strncmp(ipc_msg, "Completed", 9) == 0) {
            testing[ipc].now = FALSE;
	    testing[ipc].complete_pass++;
	    testing[ipc].bump_pass_cntr = TRUE;
	    printf("%s: pass %d, errors %d.\n",
			device, testing[ipc].pass, testing[ipc].errors);
         }
         else if (strncmp(ipc_msg, "Terminated", 10) == 0) {
	    testing[ipc].now = FALSE;
	    finish();
         }
      }                     
      else if (strncmp(ipc_msg, "WARNING:", 8) == 0) {
          atn_msg_type = WARNING;
          ipc_msg += 9;  
          return_code = IPC_WARNING;
      }                
      else if (strncmp(ipc_msg, "FATAL:", 6) == 0) {
          testing[ipc].now = FALSE;
          atn_msg_type = FATAL;
          ipc_msg += 7;  
          return_code = IPC_FATAL;
      }                
      else if (strncmp(ipc_msg, "ERROR:", 6) == 0) {
          testing[ipc].now = FALSE;
          atn_msg_type = ERROR;
	  testing[ipc].errors++;
	  errors++;
          ipc_msg += 7;  
          return_code = atoi(ipc_msg);
          ipc_msg += 3;  
      }
      else {                
	 sprintf(msg, "%s, unknown message type -> %s <- pass %d, errors %d.",
		      device, ipc_msg, testing[ipc].pass, testing[ipc].errors);
         if (atn) send_msg_to_atn(FATAL, msg);
	 throwup(-IPC_UNKNOWN, "%s: %s", TEST_NAME, msg);
      }
 
      if (return_code == IPC_WARNING || return_code == IPC_FATAL || 
						   return_code == IPC_ERROR) {
	 sprintf(msg, "%s, %s pass %d, errors %d.",
		      device, ipc_msg, testing[ipc].pass, testing[ipc].errors);
         if (atn) send_msg_to_atn(atn_msg_type, msg);
 
         if (return_code != IPC_ERROR) 
	    throwup(return_code == IPC_WARNING? return_code:-return_code, 
		    "%s: %s", TEST_NAME, msg);
	 else throwup(stop_on_error? -return_code:return_code,
		    "ERROR: %s, %s", TEST_NAME, msg);
      }
      else if (atn || debug) printf("ipctest: %s, %s\n", device, ipc_msg);
   }
/*   if (debug) printf("Exiting check_for_ipc_msg for %d.\n", ipc); */
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
 
  if (!logfd){   
     if (simulate_error == NO_OPEN_LOG) {
        strcpy(logfile, "not/valid/log");
     }
     else {
        if (getenv("SD_LOG_DIRECTORY") && simulate_error != NO_SD_LOG_DIR) {
           strcpy(logfile, (getenv("SD_LOG_DIRECTORY")));
           strcat(logfile, "/");
           strcpy(sd_log_directory, logfile);
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

/*** not standard ***/
  if (where < 0) {
     clean_up();
     print_end_msg();
     exit(-where);
  }
/*** not standard ***/

  failed = where;
}

clean_up()
{
   char *ipc_msg = ipc_buf;
   int null_file_cntr = 0;
   int wait_count = 0;

   for (ipc = 0; ipc < MAX_IPCS; ipc++) {
      if (testing[ipc].selected) {
         if (testing[ipc].now) {
            if ((afo = fopen(testing[ipc].sd_msg_to_ipc, "w")) == NULL) {
	       strcpy(device, "");
	       sprintf(msg, "Couldn't open file '%s', pass %d, errors %d.",
			     testing[ipc].sd_msg_to_ipc, pass, errors);
	       if (atn) send_msg_to_atn(FATAL, msg);
	       throwup(NO_IPC_MSG_FILE, "%s: %s", TEST_NAME, msg);
            }
            fprintf(afo, "Stop.");
            fclose(afo); 
	    chmod(testing[ipc].sd_msg_to_ipc, 00777);
	    if (debug) printf ("Sent 'stop' to %s.\n",testing[ipc].device);
         }
      }
   }
   sleep(2);
   while (testing[0].now || testing[1].now || testing[2].now || testing[3].now){
      for (ipc = 0; ipc < MAX_IPCS; ipc++) {
	 if (testing[ipc].selected) {
	    strcpy(device, testing[ipc].device);
	    if (testing[ipc].now) {
/*	       if (debug) printf ("Waiting for last message from %s.\n",device);
*/
               if ((af = fopen(testing[ipc].ipc_msg_to_sd, "r")) != NULL) {
	          sleep(1); 
	          ipc_msg = ipc_buf;
	          *ipc_msg = '\0';
		  null_file_cntr = 0;
                  while (*ipc_msg == '\0' && null_file_cntr < WAIT_FOR_MSG) {
                     fgets(ipc_msg, MAX_LENGTH, af);
	             if (*ipc_msg == '\0') {
	                rewind(af);
	                null_file_cntr++;
	                if (debug) printf("%s: Null message file, pass %d.\n",
			                   device, null_file_cntr);
 	                sleep(2); 
	             }
                  }
                  fclose(af);
                  unlink(testing[ipc].ipc_msg_to_sd);
                  *(ipc_msg + (strlen(ipc_msg) -1)) = '\0';
                  if (debug) printf("%s, ipc msg was: %s\n", device, ipc_msg);

                  if (strncmp(ipc_msg, "INFO:", 5) == 0) {
                    atn_msg_type = INFO;
                    ipc_msg += 6;
                    return_code = IPC_INFO;
                    if (strncmp(ipc_msg, "Completed", 9) == 0) {
                       testing[ipc].now = FALSE;
                    }  
                    if (strncmp(ipc_msg, "Terminated", 10) == 0) {
                       testing[ipc].now = FALSE;
                    }  
                  }  
                  else if (strncmp(ipc_msg, "WARNING:", 8) == 0) {
                      atn_msg_type = WARNING;
                      ipc_msg += 9;
                      return_code = IPC_WARNING;
                  }
                  else if (strncmp(ipc_msg, "FATAL:", 6) == 0) {
                      testing[ipc].now = FALSE;
                      atn_msg_type = FATAL;
                      ipc_msg += 7;
                      return_code = IPC_FATAL;
                  }                
                  else if (strncmp(ipc_msg, "ERROR:", 6) == 0) {
                      testing[ipc].now = FALSE;
                      atn_msg_type = ERROR;
                      testing[ipc].errors++;
                      errors++;
                      ipc_msg += 7;
                      return_code = atoi(ipc_msg);
                      ipc_msg += 3;  
                  }
                  else {
	             sprintf(msg, 
		       "%s, unknown message type -> %s <- pass %d, errors %d.",
		       device, ipc_msg, testing[ipc].pass, testing[ipc].errors);
                     if (atn) send_msg_to_atn(FATAL, msg);
	             throwup(-IPC_UNKNOWN, "%s: %s", TEST_NAME, msg);
                  }
                  if (return_code == IPC_WARNING || return_code == IPC_FATAL ||
						    return_code == IPC_ERROR) {
	             sprintf(msg, "%s, %s pass %d, errors %d.", device,
			     ipc_msg, testing[ipc].pass, testing[ipc].errors);
                     if (atn) send_msg_to_atn(atn_msg_type, msg);
 
                     if (return_code != IPC_ERROR) 
	                throwup(return_code, "%s: %s", TEST_NAME, msg);
	             else throwup(return_code, "ERROR: %s, %s", TEST_NAME, msg);
                  }    
                  else if (atn || debug) 
                           printf("ipctest: %s, %s\n", device, ipc_msg);
	       }
	    }
         }
      }
      sleep(2);
      if (wait_count++ >= wait_on_last_msg) break;
   }
}

print_end_msg()

{
   strcpy(ipc_msg, "");
   for (ipc = 0; ipc < MAX_IPCS; ipc++) {
      if (testing[ipc].selected) {
         sprintf(tmp_msg, " %s: pass %d, errors %d.", testing[ipc].device, testing[ipc].pass, testing[ipc].errors);
         strcat(ipc_msg, tmp_msg);
      }
   }

   strcpy(device, "");
   sprintf(msg, "Stopped, pass %d, errors %d.%s", pass, errors, ipc_msg);
   if (atn) send_msg_to_atn(INFO, msg);
   throwup(0, "%s: %s", TEST_NAME, msg);
}

finish()
{
   clean_up();
   print_end_msg();
   if (atn) exit(0);
   exit(20);
}

interrupt()
{
   extern fast_halt();

   if (!user_terminated) {
      user_terminated = TRUE;
      if (debug) printf("Interrupt occurred.\n");
   }
   else {
      signal(SIGHUP, fast_halt);
      signal(SIGTERM, fast_halt);
      signal(SIGINT, fast_halt);
      if (debug) printf("Second interrupt occurred.\n");
   }
}

fast_halt()
{
   if (debug) printf("Exited via fast halt.\n");
   if (atn) exit(0);
   exit(20);
}

#ifdef ATN_VERSION
#include "atnrtns.c"	/* ATN routines */
#else  
send_msg_to_atn()
{
printf("This is not the ATN version!\n");
}
#endif
