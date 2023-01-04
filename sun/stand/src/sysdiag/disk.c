/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 *
 *	this progam writes two 1/2 MB files with random
 *	data, and then compares them.  If any errors occur
 *	in the process, we log errors and stop.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <signal.h>

#define FALSE                   0
#define TRUE                    ~FALSE
#define INFO                    0
#define WARNING                 1
#define FATAL                   2
#define ERROR                   3
#define NO_SD_LOG_DIR           1
#define NO_OPEN_LOG             2
#define FILE1_OPEN_ERROR        3
#define FILE2_OPEN_ERROR        4
#define FILE1_WRITE_ERROR       5
#define FILE2_WRITE_ERROR       6
#define FILE1_CLOSE_ERROR       7
#define FILE2_CLOSE_ERROR       8
#define FILE1_REOPEN_ERROR      9
#define FILE2_REOPEN_ERROR      10
#define FILE1_READ_ERROR        11
#define FILE2_READ_ERROR        12
#define FILE1_BAD        	13
#define FILE1_CMP_ERROR        	14
#define FILE2_CMP_ERROR        	15
#define FILE1_RECLOSE_ERROR     16
#define FILE2_RECLOSE_ERROR     17
#define FILE1_UNLINK_ERROR      18
#define FILE2_UNLINK_ERROR      19
#define NO_SD_DRIVES            21
#define TOO_MANY_DRIVES         22
#define END_ERROR        	TOO_MANY_DRIVES + 1
#define LEAVE_FILES         	30	
#define USAGE_ERROR             99

#define TEST_NAME               "disk"
#define LOGFILE_NAME            "log.disk.XXXXXX"

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
char tmp_msg_buffer[80];
char *tmp_msg = tmp_msg_buffer;
char msg_buffer[200];
char *msg = msg_buffer;
int  retry_cmp_error = FALSE;
int  display_read_data = FALSE;

static char     sccsid[] = "@(#)disk.c 1.1 9/25/86 Copyright 1985 Sun Micro";

#define PASS_LOG_TIME           60
struct  timeval current_time;
int	log_time = 0, log_elapse_time = 0;
int sd_load_test = FALSE;

#define	OMODE1		( O_WRONLY | O_CREAT | O_TRUNC)
#define OMODE2		( O_RDONLY)
#define NAME1		"tmpdisk1.XXXXXX"
#define NAME2		"tmpdisk2.XXXXXX"
#define NO_FILE1	"/invalid/file1.XXXXXX"
#define NO_FILE2	"/invalid/file2.XXXXXX"
#define BSIZE		512
#define MAXBLOCK	1024
#define MAX_DRIVES	8
#define DATA_SEQUENTAL	0
#define DATA_ZERO	1
#define DATA_ONES	2
#define DATA_A		3
#define DATA_5		4
#define DATA_RANDOM	5

struct dev {
  char  device[5];
  char  directory[75];
  int	space;
  int	blocks;
  int	fd1;
  char	name1[100];
  int	file1_created;
  int	fd2;
  char	name2[100];
  int	file2_created;
  int	blocks_written;
  int	blocks_read;
} testing[MAX_DRIVES] = {
			  {
			  "sd0",
			  "/usr/tmp",
			  700,
			  512,
			  0,
			  "/usr/tmp/tmpdisk1.XXXXXX",
			  FALSE,
			  0,
			  "/usr/tmp/tmpdisk2.XXXXXX",
			  FALSE,
			  0,
			  0
			  },
			  {
			  "sd1",
			  "/usr2",
			  700,
			  100,
			  0,
			  "/usr2/tmpdisk1.XXXXXX",
			  FALSE,
			  0,
			  "/usr2/tmpdisk2.XXXXXX",
			  FALSE,
			  0,
			  0
			  },
			  {
			  "sd2",
			  "/tmp",
			  700,
			  1,
			  0,
			  "/tmp/tmpdisk1.XXXXXX",
			  FALSE,
			  0,
			  "/tmp/tmpdisk2.XXXXXX",
			  FALSE,
			  0,
			  0
			  },
			  {
			  ""
			  }
			};

int 	max_blocks = MAXBLOCK;
int  	drive = 0;
int	files_open = 0;
int	fail = 0;
int	cmp_error = FILE1_CMP_ERROR, index;
char	save_name[100], *name1, *name2;
char	*no_file1 = NO_FILE1;
char	*no_file2 = NO_FILE2;
char 	testing_devices[MAX_DRIVES * 5] = {""};
int	file1_bad = FALSE;
int	end_test_on_error = TRUE;
int	reread_block = FALSE;
int	recompare = FALSE;
int	already_recompared = FALSE;
int	reread = FALSE;
int	read_retry = FALSE;
int	cmp_simulate_transient = FALSE;
int	read_simulate_transient = FALSE;
int	data_pattern = 100;
int	usr_selected_pattern = FALSE;
u_long	pattern;

u_long	block_patterns[MAXBLOCK]; 

u_long	d_buf[3][BSIZE/sizeof(u_long)]; 
int u_long	*b1 = d_buf[0], *b2 = d_buf[1], *b3 = d_buf[2];

main(argc,argv)
int	argc;
char	*argv[];
{
   int arrcount, match;
   extern finish();

   extern char	*mktemp();

   signal(SIGINT, finish);
   signal(SIGHUP, finish);
   signal(SIGTERM, finish);

   if (getenv("SD_LOG_DIRECTORY")) exec_by_sysdiag = TRUE;
   if (getenv("SD_LOAD_TEST"))
	if (strcmp(getenv("SD_LOAD_TEST"), "yes") == 0) sd_load_test = TRUE;
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
   strcpy(device, testing[0].device);

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
				max_blocks = 2;
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
                        }
                        if (strncmp(argv[arrcount], "p=", 2) == 0) {
				usr_selected_pattern = TRUE;
				if (argv[arrcount][2] == 's') {
				   data_pattern = DATA_SEQUENTAL;
                                   match = TRUE;
				}
				if (argv[arrcount][2] == '0') {
				   data_pattern = DATA_ZERO;
                                   match = TRUE;
				}
				if (argv[arrcount][2] == '1') {
				   data_pattern = DATA_ONES;
                                   match = TRUE;
				}
				if (argv[arrcount][2] == 'a') {
				   data_pattern = DATA_A;
                                   match = TRUE;
				}
				if (argv[arrcount][2] == '5') {
				   data_pattern = DATA_5;
                                   match = TRUE;
				}
				if (argv[arrcount][2] == 'r') {
				   data_pattern = DATA_RANDOM;
                                   match = TRUE;
				}
                        }
                        if (!match) {
                           printf ("Usage: %s [v] [sd/atn] [p={s/0/1/a/5/r}] [re] [lt] [dd] [d] [e{1-%d}]\n", TEST_NAME, END_ERROR - 1);
                           exit(USAGE_ERROR);
                        }        
      }
 
   get_devices();

   if (verify) {               /* verify mode */
      if (verbose) 
         printf("%s: Verify mode, devices =%s.\n", TEST_NAME, testing_devices);
      exit(0);
   }

   sprintf(msg, "Started on%s.", testing_devices);
   if (atn) send_msg_to_atn(INFO, msg);
   throwup(0, "%s: %s", TEST_NAME, msg);

   gettimeofday(&current_time, 0);
   log_time = current_time.tv_sec;

   for (pass = 1 ;; pass++) {
      drive = 0;
      files_open = 0;
      while (strcmp(testing[drive].device, "") != 0 && drive < MAX_DRIVES) {
	 strcpy(device, testing[drive].device);

	 if (!testing[drive].file1_created) {
            name1 = testing[drive].name1;
            name2 = testing[drive].name2;
            if (simulate_error == FILE1_OPEN_ERROR) strcpy(name1, no_file1);
            name1 = mktemp(name1);
            if (simulate_error == FILE2_OPEN_ERROR) strcpy(name2, no_file2);
            name2 = mktemp(name2);
	 }

         if (verbose) {
            printf("Opening %s, file1  %s, file2  %s.\n", 
	      device, testing[drive].name1, 
	      testing[drive].name2);
         }

         if ((testing[drive].fd1=open(testing[drive].name1,OMODE1,0660))==EOF){
	    flag_error("open", testing[drive].name1, FILE1_OPEN_ERROR);
         } 
         testing[drive].file1_created = TRUE;
	 files_open++;

         if ((testing[drive].fd2=open(testing[drive].name2,OMODE1,0660))==EOF){
	    flag_error("open", testing[drive].name2, FILE2_OPEN_ERROR);
         }
         testing[drive].file2_created = TRUE;
	 testing[drive].blocks_written = 0;
	 files_open++;
         drive++;
      }

      if (!usr_selected_pattern) {
         if (++data_pattern > DATA_RANDOM) data_pattern = DATA_SEQUENTAL;
         pattern = -1;
      }

      while (files_open) {
        drive = 0;
	switch (data_pattern) {
		case DATA_SEQUENTAL:
			   pattern++;
			   break;
		case DATA_ZERO:
			   pattern = 0;
			   break;
		case DATA_ONES:
			   pattern = 0xffffffff;
			   break;
		case DATA_A:
			   pattern = 0xaaaaaaaa;
			   break;
		case DATA_5:
			   pattern = 0x55555555;
			   break;
		case DATA_RANDOM:
			   pattern = random();
	}
	if (simulate_error == FILE1_BAD) lfill(b1, BSIZE, 0x11111111);
        else lfill(b1, BSIZE, pattern);

	while (strcmp(testing[drive].device, "") != 0 && drive < MAX_DRIVES) {
	   if (testing[drive].fd1) {
              strcpy(device, testing[drive].device);
              if (simulate_error == FILE1_WRITE_ERROR) 
		 close(testing[drive].fd1);
	      if (simulate_error == FILE1_CMP_ERROR) 
		 d_buf[0][testing[drive].blocks_written] = 
		 d_buf[0][testing[drive].blocks_written] + 1;

	      if (block_patterns[testing[drive].blocks_written] != pattern) {
/*
	         if (debug) printf
	            ("Block %d written with repeating hex pattern of '%08x'.\n",
	              testing[drive].blocks_written, pattern);
*/
	         block_patterns[testing[drive].blocks_written] = pattern;
	      }

              if (write(testing[drive].fd1, b1, BSIZE) != BSIZE) {
	         transfer_error ("Write", testing[drive].name1, 
		 FILE1_WRITE_ERROR, testing[drive].blocks_written);
              }

              if (simulate_error == FILE2_WRITE_ERROR) 
		 close(testing[drive].fd2);
	      if (simulate_error == FILE1_CMP_ERROR)
	         d_buf[0][testing[drive].blocks_written] = 
		 d_buf[0][testing[drive].blocks_written] - 1;
	      if (simulate_error == FILE2_CMP_ERROR && drive == 0 &&
		 testing[drive].blocks_written == 1)  {
	         d_buf[0][25] = d_buf[0][25] - 1;
	         cmp_error = FILE2_CMP_ERROR;
	      }

	      if (write(testing[drive].fd2, b1, BSIZE) != BSIZE){
	         transfer_error("Write", testing[drive].name2, 
		 FILE2_WRITE_ERROR, testing[drive].blocks_written);
              }
	      if (simulate_error == FILE2_CMP_ERROR && drive == 0 &&
		 testing[drive].blocks_written == 1)  {
	         d_buf[0][25] = d_buf[0][25] + 1;
	      }

	      if (++testing[drive].blocks_written >= testing[drive].blocks) {
	         if (verbose) printf("Closing %s, blocks written = %d.\n", 
		    testing[drive].device, testing[drive].blocks_written);
                 if (simulate_error == FILE1_CLOSE_ERROR) 
		    close(testing[drive].fd1);
                 if (testing[drive].fd1 = close(testing[drive].fd1)){
                    flag_error("close", testing[drive].name1, 
		    FILE1_CLOSE_ERROR);
                 }
		 files_open--;
                 if (simulate_error == FILE2_CLOSE_ERROR) 
		    close(testing[drive].fd2);
                 if (testing[drive].fd2 = close(testing[drive].fd2)){
                    flag_error("close", testing[drive].name2, 
		    FILE2_CLOSE_ERROR);
                 }
	         files_open--;
	      }
           }
	   drive++;
        }
      }
      drive = 0;
      while (strcmp(testing[drive].device, "") != 0 && drive < MAX_DRIVES) {
         
	 strcpy(device, testing[drive].device);

         if (simulate_error == FILE1_REOPEN_ERROR) {
	    name1 = testing[drive].name1;
	    strcpy(save_name, name1);
	    strcpy(name1, no_file1);
	    name1 = mktemp(name1);
         }
         if (simulate_error == FILE2_REOPEN_ERROR) {
	    name2 = testing[drive].name2;
	    strcpy(save_name, name2);
	    strcpy(name2, no_file2);
	    name2 = mktemp(name2);
         }

         if ((testing[drive].fd1 = open(testing[drive].name1,OMODE2)) == EOF){
            flag_error("reopen", testing[drive].name1, FILE1_REOPEN_ERROR);
         }
	 files_open++;
         if ((testing[drive].fd2 = open(testing[drive].name2,OMODE2)) == EOF){
            flag_error("reopen", testing[drive].name2, FILE2_REOPEN_ERROR);
         }
	 testing[drive].blocks_read = 0;
	 files_open++;
         drive++;
      }

      while (files_open) {
         drive = 0;

	 while (strcmp(testing[drive].device, "") != 0 && drive < MAX_DRIVES) {
	    if (testing[drive].fd1) {
               strcpy(device, testing[drive].device);
	       if (simulate_error == FILE1_READ_ERROR) 
	          close(testing[drive].fd1);

               lfill(b3, BSIZE, block_patterns[testing[drive].blocks_read]);

	       if (read_retry) {
   	          sprintf(msg,"\nRereading and recomparing block %d on %s.\n\n",
				    testing[drive].blocks_read, device);
   		  printf("%s", msg);
   		  fprintf(stderr, "%s", msg);
		  fflush(stderr);
	       }

               if (read(testing[drive].fd1, b1, BSIZE) != BSIZE){
	          transfer_error("Read", testing[drive].name1, 
	          FILE1_READ_ERROR, testing[drive].blocks_read);
               }
	       if (simulate_error == FILE2_READ_ERROR) 
	          close(testing[drive].fd2);

	       if (simulate_error != FILE1_CMP_ERROR)
		  file1_bad = lcmp(b1,b3,BSIZE);

	       if (read(testing[drive].fd2, b2, BSIZE) != BSIZE){
	          transfer_error("Read", testing[drive].name2, 
	          FILE2_READ_ERROR, testing[drive].blocks_read);
               }
	       if (((index = lcmp(b1,b2,BSIZE)) || file1_bad) && 
						!read_simulate_transient) {
		  compare_error();
		  if (display_read_data) display_data();
		  while (recompare) {
		     recompare = FALSE;
   		     sprintf(msg, "\nRecomparing block %d on %s.\n\n",
				    testing[drive].blocks_read, device);
   		     printf("%s", msg);
   		     fprintf(stderr, "%s", msg);
		     fflush(stderr);
		     if (simulate_error != FILE1_CMP_ERROR)
			file1_bad = lcmp(b1,b3,BSIZE);
		     if (((index = lcmp(b1,b2,BSIZE)) || file1_bad) &&
			                          !cmp_simulate_transient) {
			compare_error();
			display_data();
		     }
		     else {
   		        sprintf(msg, 
			"The recompare of block %d on %s was successful.\n\n",
			 testing[drive].blocks_read, device);
   		        printf("%s", msg);
   		        fprintf(stderr, "%s", msg);
			fflush(stderr);
		     }
		  }
               }
	       else {
	          if (read_retry) {
		     read_retry = FALSE;
   		     sprintf(msg, "The compare after a reread of block %d on %s was successful.\n\n", testing[drive].blocks_read, device);
   		     printf("%s", msg);
   		     fprintf(stderr, "%s", msg);
		     fflush(stderr);
		  }
	       }
	       if (++testing[drive].blocks_read >= testing[drive].blocks) {
	          if (verbose) printf("Closing %s, blocks read = %d.\n",
	             testing[drive].device, testing[drive].blocks_read);
                  if (simulate_error == FILE1_RECLOSE_ERROR) 
                     close(testing[drive].fd1);
                  if (testing[drive].fd1 = close(testing[drive].fd1)){
                     flag_error("reclose", testing[drive].name1, 
                     FILE1_RECLOSE_ERROR);
                  }
                  files_open--;
                  if (simulate_error == FILE2_RECLOSE_ERROR) 
                     close(testing[drive].fd2);
                  if (testing[drive].fd2 = close(testing[drive].fd2)){
                     flag_error("reclose", testing[drive].name2, 
                     FILE2_RECLOSE_ERROR);
                  }
                  files_open--;
               }
            }
	    if (reread_block) reread_block = FALSE;
	    else drive++;
         }
      }
      
      if (load_test) {
         clean_up();
         break;
      }
      if (!verbose) sleep (5);
      gettimeofday(&current_time, 0);
      log_elapse_time = current_time.tv_sec - log_time;
      sprintf(msg, "%s: pass %d, errors %d.", TEST_NAME, pass, errors);
      if (getenv("LOG_PASS_MSG") && log_elapse_time >= PASS_LOG_TIME * 60) {
         throwup(0, "%s", msg);
         log_time = current_time.tv_sec;
      }
      else printf("%s\n", msg);
   }
   throwup(0, "%s: Stopped, pass %d, errors %d.", TEST_NAME, pass, errors);
}

get_devices()
{
#define DRIVE_TYPES 13

   int checking = FALSE;
   int i;
   char device_dir[10];
   char device_space[10];
   static char device_list[DRIVE_TYPES][5] = {
					      "sd0",
					      "sd1",
					      "sd2",
					      "sd3",
					      "xd0",
					      "xd1",
					      "xd2",
					      "xd3",
					      "xy0",
					      "xy1",
					      "xy2",
					      "xy3",
					      "nd0"
					    };
   drive = 0;
   if (checking) printf("entering get_devices.\n");
   if (exec_by_sysdiag) {
      for (i = 0; i < DRIVE_TYPES; i++) {
	 strcpy(device_dir, "DIR_");
	 strcat(device_dir, device_list[i]);
	 strcpy(device_space, "SPACE_");
	 strcat(device_space, device_list[i]);
         if (checking) printf("device_dir = %s, device_space = %s, i = %d.\n", 
	    device_dir, device_space, i);
	 if (getenv(device_dir)) {
            if (checking) 
	       printf("found device_dir, i = %d, drive = %d.\n", i, drive);
            if (drive >= MAX_DRIVES) {
	       sprintf(msg, "Too many disk drives selected.");
	       if (atn) send_msg_to_atn(FATAL, msg);
	       throwup(-TOO_MANY_DRIVES, "%s: %s", TEST_NAME, msg);
	    }
	    strcpy(testing[drive].device, device_list[i]); 
	    strcpy(testing[drive].directory, (getenv(device_dir)));
	    testing[drive].space = atoi(getenv(device_space));
	    if (testing[drive].space > MAXBLOCK) 
	       testing[drive].blocks = MAXBLOCK;
 	    else testing[drive].blocks = testing[drive].space;
	    testing[drive].fd1 = 0;
	    strcpy(testing[drive].name1, testing[drive].directory);
	    strcat(testing[drive].name1, "/");
	    strcat(testing[drive].name1, NAME1);
	    testing[drive].file1_created = FALSE;
	    testing[drive].fd2 = 0;
	    strcpy(testing[drive].name2, testing[drive].directory);
	    strcat(testing[drive].name2, "/");
	    strcat(testing[drive].name2, NAME2);
	    testing[drive].file2_created = FALSE;
	    testing[drive].blocks_written = 0;
	    testing[drive].blocks_read = 0;
	    drive++;
	 }
      }
      if (checking) printf("exiting search.\n");
      if (drive == 0) {
	if (!verbose) {
	  sprintf(msg, "No disk drives selected.");
	  if (atn) send_msg_to_atn(FATAL, msg);
	  throwup(-NO_SD_DRIVES, "%s: %s", TEST_NAME, msg);
	}
      }
      else if (drive < MAX_DRIVES) strcpy(testing[drive].device, "");
   } 
   drive = 0;
   while (strcmp(testing[drive].device, "") != 0 && drive < MAX_DRIVES) {
      strcat(testing_devices, " ");
      strcat(testing_devices, testing[drive].device);
      if (load_test || sd_load_test) testing[drive].blocks = 2;
      if (debug) printf ("\nTesting %s, dir = %s, space = %d, blocks = %d, fd1 = %d, name1 = %s, file1_created = %d, fd2 = %d, name2 = %s, file2_created = %d, blks written = %d, blks read = %d.\n", testing[drive].device, testing[drive].directory, testing[drive].space, testing[drive].blocks, testing[drive].fd1, testing[drive].name1, testing[drive].file1_created, testing[drive].fd2, testing[drive].name2, testing[drive].file2_created, testing[drive].blocks_written, testing[drive].blocks_read); 
      drive++;
   }
   if (debug) printf ("\n");
}

flag_error(type, filename, code)
char *type;
char *filename;
int  code;
{
   perror(perror_msg);
   sprintf(msg, "Couldn't %s file '%s' on %s, pass %d, errors %d.",
	 type, filename, device, pass, errors);
   if (atn) send_msg_to_atn(FATAL, msg);
   throwup(-code, "%s: %s", TEST_NAME, msg);
}

transfer_error(type, filename, code, rec)
char *type;
char *filename;
int  code, rec;
{
   perror(perror_msg);
   errors++;
   sprintf(msg, "%s failed on %s '%s', blk %d, pass %d, errors %d.",
       type, device, filename, rec, pass, errors);
   if (atn) send_msg_to_atn(ERROR, msg);
   throwup(-code, "ERROR: %s, %s", TEST_NAME, msg);
}

compare_error()
{
   if (file1_bad) {
      index = file1_bad;
      file1_bad = FALSE;
      cmp_error = FILE1_BAD;
      sprintf(tmp_msg, "'%s'", testing[drive].name1);
   }
   else sprintf(tmp_msg, "between '%s' and '%s'", 
      testing[drive].name1, testing[drive].name2);
   errors++;
   sprintf(msg, "ompare error on %s %s, blk %d, offset %d, pass %d, errors %d.", device, tmp_msg, testing[drive].blocks_read, BSIZE - sizeof(u_long)*index, pass, errors);
   if (atn) send_msg_to_atn(ERROR, "C%s", msg);
   if (display_read_data || !stop_on_error) end_test_on_error = FALSE;
   throwup(end_test_on_error? -cmp_error : cmp_error, 
	   "ERROR: %s, c%s", TEST_NAME, msg);
}

display_data()
{
   int b, i;
   char ch[20];
   u_long print_pattern;
   
   print_pattern = block_patterns[testing[drive].blocks_read];

   sprintf(msg, "\nBlock %d on %s was written with a repeating hex pattern of '%08x'\n", testing[drive].blocks_read, device, print_pattern);
   printf("%s", msg);
   fprintf(stderr, "%s", msg);

   sprintf(msg, "Data from %s '%s' starting at the word in error was:\n", device, testing[drive].name1);
   printf("%s", msg);
   fprintf(stderr, "%s", msg);

   for (i = 0; i < 8; i++) {
     sprintf(msg, "%08x ",
	     d_buf[0][i + ((BSIZE - sizeof(u_long)*index)/sizeof(u_long))]);
     printf("%s", msg);
     fprintf(stderr, "%s", msg);
   }
   sprintf(msg, "\n\nData from %s '%s' starting at the word in error was:\n", device, testing[drive].name2);
   printf("%s", msg);
   fprintf(stderr, "%s", msg);

   for (i = 0; i < 8; i++) {
     sprintf(msg, "%08x ",
	d_buf[1][i + ((BSIZE - sizeof(u_long)*index)/sizeof(u_long))]);
     printf("%s", msg);
     fprintf(stderr, "%s", msg);
   }
   printf("\n");
   fprintf(stderr, "\n\n");
   fflush(stderr);

   if (stop_on_error) {
      printf("\nContinue check? y/n: ");
      scanf("%s", ch);
      if (ch[0] == 'n') {
	printf("Remove test files? y/n: ");
	scanf("%s", ch);
	if (ch[0] == 'y') {
	   clean_up();
           exit(cmp_error);
        }
	else exit(LEAVE_FILES);
      }
      printf("Redo the compare? y/n ");
      scanf("%s", ch);
      if (ch[0] == 'y') {
         recompare = TRUE;
         if (simulate_error == FILE1_BAD && errors > 3) 
	    cmp_simulate_transient = TRUE;
      }
      else {
         printf("Reread the same blocks? y/n ");
         scanf("%s", ch);
         if (ch[0] == 'y') {
            lseek(testing[drive].fd1, -(BSIZE), 1);
            lseek(testing[drive].fd2, -(BSIZE), 1);
            testing[drive].blocks_read--;
            reread_block = TRUE;
            read_retry = TRUE;
            if (simulate_error == FILE1_CMP_ERROR && errors > 3) 
	       read_simulate_transient = TRUE;
         }
         else read_retry = FALSE;
      }
   }
   else {
      if (!already_recompared) {
	 already_recompared = TRUE;
         recompare = TRUE;
         if (simulate_error == FILE1_BAD && errors > 3) 
	    cmp_simulate_transient = TRUE;
      }
      else {
         if (!reread) {
	    reread = TRUE;
            lseek(testing[drive].fd1, -(BSIZE), 1);
            lseek(testing[drive].fd2, -(BSIZE), 1);
            testing[drive].blocks_read--;
            reread_block = TRUE;
            read_retry = TRUE;
            if (simulate_error == FILE1_CMP_ERROR && errors > 3) 
	       read_simulate_transient = TRUE;
	 }
	 else {
	    read_retry = FALSE;
	    already_recompared = FALSE;
	    reread = FALSE;
            sprintf(msg, "\nContinuing with the next block on %s.\n\n", device);
            printf("%s", msg);
            fprintf(stderr, "%s", msg);
	 }
      }
   }
}

/*
		Note:   Not a standard "throwup" routine.
*/

throwup(where, fmt, a, b, c, d, e, f, g, h, i)
int	where;
char	*fmt;
u_long	a, b, c, d, e, f, g, h, i;
{
  char *attempt_log_msg = "Was attempting to log the following message:\n";
  extern char	*mktemp();
  int		clock;
  char 		fmt_msg_buffer[200];
  char 		*fmt_msg = fmt_msg_buffer;

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
     else dup2(logfd, 2);		/* set logfile as stderr */
  }
  fprintf(stderr, "%s %s", fmt_msg, ctime(&clock));
  printf("%s %s", fmt_msg, ctime(&clock));
  
  fflush(stderr);
  fsync(2);

/*** not standard ***/
  if (where < 0) {
     clean_up();
     exit(-where);
  }
/*** not standard ***/

  failed = where;
}

clean_up()
{
  drive = 0;
  name1 = testing[drive].name1;
  name2 = testing[drive].name2;
  if (simulate_error == FILE1_REOPEN_ERROR) strcpy(name1, save_name);
  if (simulate_error == FILE2_REOPEN_ERROR) strcpy(name2, save_name);
  while (strcmp(testing[drive].device, "") != 0 && drive < MAX_DRIVES) {
     strcpy(device, testing[drive].device);
     if (testing[drive].file1_created) {
        if (testing[drive].fd1 > 0) testing[drive].fd1 = close(testing[drive].fd1);
        testing[drive].file1_created = FALSE;
        if (simulate_error == FILE1_UNLINK_ERROR) unlink(testing[drive].name1);
        if(unlink(testing[drive].name1)) {
           flag_error("unlink", testing[drive].name1, -FILE1_UNLINK_ERROR);
        }
     }
     if (testing[drive].file2_created) {
        if (testing[drive].fd2 > 0) testing[drive].fd2 = close(testing[drive].fd2);
	testing[drive].file2_created = FALSE;
	if (simulate_error == FILE2_UNLINK_ERROR) unlink(testing[drive].name2);
        if(unlink(testing[drive].name2)) {
	   flag_error("unlink", testing[drive].name2, -FILE2_UNLINK_ERROR);
        }
     }
     drive++;
  }
  if (failed > 0) exit(failed);
}

finish()
{
   clean_up();
   sprintf(msg, "Stopped, pass %d, errors %d.", pass, errors);
   throwup(0, "%s: %s", TEST_NAME, msg);
   if (atn) {
      send_msg_to_atn(INFO, msg);
      exit(0);
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
