#include <sys/types.h>
#include <stdio.h>
#include <sys/file.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/mtio.h>
#include <sys/ioctl.h>

#define FALSE                   0
#define TRUE                    ~FALSE
#define INFO                    0
#define WARNING                 1
#define FATAL                   2
#define ERROR                   3
#define NO_SD_LOG_DIR           1
#define NO_OPEN_LOG             2
#define ATN_SECTOR_ERROR        3
#define WRITE_NOT_ALLOWED       4
#define RET_OPEN_ERROR       	5
#define ERASE_ERROR       	6
#define RETENSION_ERROR       	7
#define OPEN_ERROR       	8
#define BIG_WRITE_ERROR       	9
#define LITTLE_WRITE_ERROR     	10
#define RE_OPEN_ERROR       	11
#define BIG_READ_ERROR       	12
#define BIG_REREAD_ERROR       	13
#define LITTLE_READ_ERROR      	14
#define LITTLE_REREAD_ERROR    	15
#define BIG_COMPARE_ERROR       16
#define BIG_RECOMPARE_ERROR     17
#define LITTLE_COMPARE_ERROR    18
#define LITTLE_RECOMPARE_ERROR  19
#define END_ERROR               20
#define USAGE_ERROR             99

#define TEST_NAME               "devtest"
#define LOGFILE_NAME            "log.devtest.XXXXXX"

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
char tmp_msg_buffer[100];
char *tmp_msg = tmp_msg_buffer;
char msg_buffer[200];
char *msg = msg_buffer;
int  retry_cmp_error = FALSE;
int  display_read_data = FALSE;
int shoebox_test = FALSE;

static char     sccsid[] = "@(#)devtest.c 1.1 9/25/86 Copyright 1985 Sun Micro";

int  return_status = 0;
char pass_msg_buffer[30];
char *pass_msg = pass_msg_buffer;
#define PASS_LOG_TIME           60
struct  timeval current_time;
int     log_time = 0, log_elapse_time = 0;

#define NBLOCKS		(126)
#define BLOCKSIZE	(512)
#define PASS_SLEEP_MOD  5
#define ERROR_THRESHOLD 2
                       /* 1/4 inch tape head cleaning time = 7.5 hrs (27000)*/
#define HEAD_CLEAN_TIME 27000

char other_d_name[15];
char *other_device = other_d_name;
char saved_device_name[15];
char *save_device = saved_device_name;
int do_tape_sleep = TRUE;
int after_op_sleep = 1;
int head_clean_time = HEAD_CLEAN_TIME;
int atn_head_clean_time = 600;
int testing_cartridge_tape = FALSE;
int write_end_test = FALSE;
int switch_8_and_0 = FALSE;
int scsi_tape = FALSE;
int read_compare = FALSE;
int more_tape_time = FALSE;
int type = 0;
char time_msg_buffer[30];
char *test_time_msg = time_msg_buffer;

struct commands {
        char *c_name;
        int c_code;
        int c_ronly;
        int c_usecnt;
} com[] = {
        { "retension",  MTRETEN, 1, 0 },
        { "erase",      MTERASE, 0, 0 },
        { 0 }
}; 
struct mtop mt_com;

struct timeval current_time;
u_long head_start_time, head_use_time, tape_sleep_start, total_test_time = 0;
u_long sleep_time, slept_already, elapse_time = 0, current_test_time = 0;
char            ch[20];

u_char	dev_buf[NBLOCKS*BLOCKSIZE];
union data_word {	
	int i; 
        u_char b[4];
}; 
union data_word data_sb;
int 	error_offset;
int	fd = 0, writing = FALSE, looping = FALSE;
int  mt_sts;
char date_time_buffer[30];
char *date_time = date_time_buffer;
int atn_sector_error = FALSE;
int using_disk_file = TRUE;
int stop_to_clean_hds = TRUE;
int reread_sector = FALSE;
int reread_file = FALSE;
int continue_reread = FALSE;
int check_threshold = TRUE;
int capacity = 0;
int reread_errors = 0;
int n, start_block, error_byte;
struct commands *comp;
char *cp;

main(argc,argv)
int	argc;
char    *argv[];
{
   int arrcount, match;
   extern finish();

   register u_char	c, *p;
   register i, s, b, e, nblks = 127;

   signal(SIGHUP, finish);
   signal(SIGTERM, finish);
   signal(SIGINT, finish);

   if (getenv("SD_LOAD_TEST"))
        if (strcmp(getenv("SD_LOAD_TEST"), "yes") == 0) load_test = TRUE;
   if (getenv("SUN_MANUFACTURING")) {
      if (strcmp(getenv("SUN_MANUFACTURING"), "yes") == 0) {
         display_read_data = TRUE;
         retry_cmp_error = TRUE;
	 if (getenv("SHOEBOX_TEST")) shoebox_test = TRUE;
      }
      if (strcmp(getenv("RUN_ON_ERROR"), "enabled") == 0) {
         stop_on_error = FALSE;
      }
   }
   if (getenv("NO_CLEAN_HEADS")) stop_to_clean_hds = FALSE;
   if (getenv("NO_TAPE_SLEEP")) do_tape_sleep = FALSE;

   sprintf(perror_msg, "%s: perror says", TEST_NAME);
   strcpy(device, "test");

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
				looping = TRUE;
                                if (argv[arrcount][3] != 't')
                                    sending_atn_msg = TRUE;
                                verbose = FALSE;
                        }
                        if (strcmp(argv[arrcount], "sb") == 0) {
                                match = TRUE;
                                shoebox_test = TRUE;
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
                        if (strncmp(argv[arrcount], "/dev/", 5) == 0) {
                                match = TRUE;
			        strcpy(device, argv[arrcount]);
				using_disk_file = FALSE;
                        }
                        if (strcmp(argv[arrcount], "test") == 0 ||
			    strcmp(argv[arrcount], "tape.test") == 0) {
                                match = TRUE;
			        strcpy(device, argv[arrcount]);
                        }
                        if (strcmp(argv[arrcount], "ns") == 0) {
                                match = TRUE;
                                do_tape_sleep = FALSE;
                        }
                        if (strcmp(argv[arrcount], "nc") == 0) {
                                match = TRUE;
                                stop_to_clean_hds = FALSE;
                        }
                        if (strcmp(argv[arrcount], "we") == 0) {
                                match = TRUE;
                                write_end_test = TRUE;
                        }
                        if (strncmp(argv[arrcount], "b=", 2) == 0) {
                                match = TRUE;
                                nblks = (atoi(&argv[arrcount][2]));
                        }
                        if (strncmp(argv[arrcount], "c=", 2) == 0) {
                                match = TRUE;
				capacity = (atoi(&argv[arrcount][2]));
			}
                        if (strncmp(argv[arrcount], "r=", 2) == 0) {
                                match = TRUE;
                                nblks = (atoi(&argv[arrcount][2]));
				read_compare = TRUE;
				display_read_data = TRUE;
                        }
                        if (strncmp(argv[arrcount], "s=", 2) == 0) {
                                match = TRUE;
				atn_sector_error = TRUE;
				nblks = (atoi(&argv[arrcount][2]));
			}
                        if (strncmp(argv[arrcount], "w=", 2) == 0) {
                                match = TRUE;
				writing = TRUE;
                                nblks = (atoi(&argv[arrcount][2]));
                        }
                        if (!match) {
                           printf("Usage: %s /dev/name [w/r/b/s]=count [v] [sd/atn] [sb] [c=capacity] [re] [nc] [ns] [we] [lt] [dd] [d] [e{1-%d}]\n", TEST_NAME, END_ERROR - 1);
                           exit(USAGE_ERROR);
                        }        
      }
 
   if (verify) {               /* verify mode */
        if (verbose) printf("%s: Device is '%s'.\n", TEST_NAME, device);
        exit(0);
   }
   if (atn_sector_error) {
      sprintf(msg, "Incorrect number of sectors for %d meg byte disk '%s', sectors found were %d.", capacity, device, nblks);
      if (atn) send_msg_to_atn(FATAL, msg);
      throwup(-ATN_SECTOR_ERROR, "%s: %s", TEST_NAME, msg);
   }
   if (writing && (strncmp(device, "/dev/rsd", 8) == 0 || 
                                    strncmp(device, "/dev/rxd", 8) == 0 ||
                                    strncmp(device, "/dev/rxy", 8) == 0)) {
      sprintf(msg, "Writing is not allowed on '%s'.", device);
      if (atn) send_msg_to_atn(FATAL, msg);
      throwup(-WRITE_NOT_ALLOWED, "%s: %s", TEST_NAME, msg);
   }
   if ((strncmp(device, "/dev/rst", 8) == 0) || 
                                    (strncmp(device, "/dev/rar", 8) == 0)) {
      testing_cartridge_tape = TRUE;
      if (!shoebox_test) looping = TRUE;
      if ((strcmp(device, "/dev/rst8") == 0)) {
         switch_8_and_0 = TRUE;
         strcpy(other_device, "/dev/rst0");
      }
      if ((strcmp(getenv("TERM"), "sun") != 0 && !atn) || 
	 !stop_to_clean_hds) head_clean_time = 8000000;
   }

   if (looping || verbose) {
      sprintf(msg, "Started testing '%s'%s, %s with %d blocks specified.",
	 device, switch_8_and_0? " and '/dev/rst0'": "", 
	 writing? "write/read":"read only", nblks);
      if (atn) send_msg_to_atn(INFO, msg);
      throwup(0, "%s: %s", TEST_NAME, msg);
      gettimeofday(&current_time, 0);
      log_time = current_time.tv_sec;
   }

   if (testing_cartridge_tape) {
      if (!verbose) sleep(60);
      else printf("Sharing for 60 seconds after start msg.\n");
      if (nblks > 1500 && !verbose) after_op_sleep = 120;
   }
   if (load_test && nblks > 500) nblks = 382;

   while (looping || pass == 0) {
      pass++;
      if (looping) {
	 if (pass != 1 && switch_8_and_0) {
	    if (debug) printf("Switching from '%s' to '%s'.\n", 
				 device, other_device);
	    strcpy(save_device, device);
	    strcpy(device, other_device);
	    strcpy(other_device, save_device);
         }
         gettimeofday(&current_time, 0);
         head_start_time = current_time.tv_sec;
         more_tape_time = TRUE;
      }
      if (strncmp(device, "/dev/rst", 8) == 0) {
         scsi_tape = TRUE;
         if (pass == 1 && !(read_compare)) {
            cp = "erase";
            type = ERASE_ERROR;
         }
         else {
            cp = "retension";
            type = RETENSION_ERROR;
         }
         erase_retension(type);
         cp = "retension";
         type = RETENSION_ERROR;
         erase_retension(type);
      }
      if (simulate_error == OPEN_ERROR) strcpy(device, "/invalid/file");

      if (writing) {

	 if (debug) printf("Opening '%s' for write.\n", device);
         if ((fd = open(device, O_WRONLY)) < 0) open_error(OPEN_ERROR);

         e = 0;
         for (i = 0; (i + NBLOCKS) <= nblks; i += NBLOCKS){
            lfill(dev_buf, sizeof(dev_buf), i);
            if (simulate_error == BIG_COMPARE_ERROR || 
		                  simulate_error == BIG_RECOMPARE_ERROR) {
               dev_buf[e] = 1;
               e += BLOCKSIZE + 1 ;
            }
            lseek(fd, i*BLOCKSIZE,0);

            if (simulate_error == BIG_WRITE_ERROR) close(fd);
            if ((n = write(fd, dev_buf,sizeof(dev_buf))) != sizeof(dev_buf)){
               transfer_error(BIG_WRITE_ERROR, i/NBLOCKS + 1, i);
            } 
	    if (!(verbose || load_test)) {
               if (nblks <= 1500) sleep(2);
               else if ((i % PASS_SLEEP_MOD) == 0) sleep(2);
	    }
         }
	 if (debug) {
	    if (i != 0) printf("Completed %d big write(s)", i/NBLOCKS);
	    else printf("No big writes");
	    printf(", sharing for %d second(s).\n", after_op_sleep);
	 }
         if(!(verbose || load_test)) sleep(after_op_sleep);
         e = 0;
         start_block = i;
         for (i = 0; i < nblks % NBLOCKS ; i++){
            lfill(dev_buf, BLOCKSIZE, i);
            if (simulate_error == LITTLE_COMPARE_ERROR ||
		                  simulate_error == LITTLE_RECOMPARE_ERROR) {
               dev_buf[e] = 255;
               e++;
            }
            if (simulate_error == LITTLE_WRITE_ERROR) close(fd);
            if ((n = write(fd, dev_buf,BLOCKSIZE)) != BLOCKSIZE){
               transfer_error(LITTLE_WRITE_ERROR, i + 1, start_block + i);
            }
	    if (!(verbose || load_test)) {
               if ((i % PASS_SLEEP_MOD) == 0) sleep(1);
	    }
         }
	 if (debug) {
	    if (i != 0) printf("Completed %d little write(s)", i);
	    else printf("No little writes");
	    printf(", sharing for %d second(s).\n", after_op_sleep);
	 }
         if (scsi_tape) get_tape_status(); 
         if (write_end_test) {
            printf("Tape at end of long write.\n");
            sleep(2000000);
         }
	 if (debug) printf("Closing '%s' after write.\n", device);
         close(fd);
         if(!(verbose || load_test)) sleep(after_op_sleep);
      }
                 				/*  read routine */

      if (simulate_error == RE_OPEN_ERROR) strcpy(device, "/invalid/file");
      if (debug) printf("Opening '%s' for read.\n", device);

      read_file(nblks);

      if (reread_file) {
	 if (debug) printf("Closing '%s' after bad read.\n", device);
         close(fd);
	 if (debug) printf("Opening '%s' for bad read verify.\n", device);
         sprintf(msg, "Starting automatic reread of '%s',%s errors %d%s", 
		 device, print_pass()? pass_msg:"", 
		 errors, print_test_time()? test_time_msg:".");
         if (atn) send_msg_to_atn(INFO, msg);
         throwup(0, "%s: %s", TEST_NAME, msg);
	 if (!(simulate_error == BIG_COMPARE_ERROR || 
	     simulate_error == LITTLE_COMPARE_ERROR)) read_file(nblks);
	 reread_file = FALSE;
         sprintf(msg, "The reread of '%s' %s,%s reread errors %d%s", 
		 device, reread_errors? "is complete":"was successful", 
		 print_pass()? pass_msg:"", 
		 reread_errors, print_test_time()? test_time_msg:".");
         if (atn) send_msg_to_atn(INFO, msg);
         throwup(0, "%s: %s", TEST_NAME, msg);
      }

      if (load_test && !testing_cartridge_tape) break;
      if (!verbose) sleep (4);
      if (looping) {
         gettimeofday(&current_time, 0);
         log_elapse_time = current_time.tv_sec - log_time;
         sprintf(msg, "%s: '%s', pass %d, errors %d%s", TEST_NAME, 
               device, pass, errors, print_test_time()? test_time_msg:".");
         if (getenv("LOG_PASS_MSG") && log_elapse_time >= PASS_LOG_TIME * 60){
            throwup(0, "%s", msg);
            log_time = current_time.tv_sec;
         }  
         else printf("%s\n", msg);

         if (scsi_tape) get_tape_status();
	 if (debug) printf("Closing '%s' after read.\n", device);
         close(fd);

         if (testing_cartridge_tape) check_clean_time();

        } /* end of if (looping) */
   }
   if (atn || verbose)
       throwup(0, "%s: Stopped, pass %d, errors %d.", TEST_NAME, pass, errors);
   exit(return_status);
}

read_file(nblks)
int nblks;
{
   register i;

      if ((fd = open(device,  O_RDONLY)) < 0) open_error(RE_OPEN_ERROR);
      for (i = 0; (i + NBLOCKS) <= nblks; i += NBLOCKS){
         lseek(fd, i*BLOCKSIZE,0);
         if ((simulate_error == BIG_READ_ERROR && !reread_file) ||
	      simulate_error == BIG_REREAD_ERROR) close(fd);
         if ((n = read(fd, dev_buf, sizeof(dev_buf))) != sizeof(dev_buf)){
            transfer_error(reread_file? BIG_REREAD_ERROR:BIG_READ_ERROR,
			   i/NBLOCKS + 1, i);
	    if (reread_file) return;
	 }
	 else if (writing || read_compare) {
            if ( n = lcheck(dev_buf, sizeof(dev_buf), i)){
               error_offset = sizeof(dev_buf) - n * sizeof(long);
               compare_error(reread_file? 
			     BIG_RECOMPARE_ERROR:BIG_COMPARE_ERROR, i);
	       if (reread_sector) {
	          i -= NBLOCKS;
	          reread_sector = FALSE;
	       }
	       if (reread_file && !continue_reread) return;
	       continue_reread = FALSE;
            }
	    if (!(verbose || load_test)) {
               if (nblks <= 1500) sleep(2);
               else if ((i % PASS_SLEEP_MOD) == 0) sleep(2);
	    }
	 }
      }
      if (debug) {
         if (i != 0) printf("Completed %d big read(s)", i/NBLOCKS);
         else printf("No big reads");
	 printf(", sharing for %d second(s).\n", after_op_sleep);
      }
      if(!(verbose || load_test)) sleep(after_op_sleep);
      start_block = i;
      for (i = 0; i < nblks % NBLOCKS ; i++){
         if ((simulate_error == LITTLE_READ_ERROR && !reread_file) ||
	      simulate_error == LITTLE_REREAD_ERROR) close(fd);
         if ((n = read(fd, dev_buf, BLOCKSIZE)) != BLOCKSIZE){
            transfer_error(reread_file? LITTLE_REREAD_ERROR:LITTLE_READ_ERROR,
			   i + 1, start_block + i);
	    if (reread_file) return;
         }
	 else if (writing || read_compare) {
            if ( n = lcheck(dev_buf, BLOCKSIZE, i)){
               error_offset = BLOCKSIZE - n * sizeof(long);
               compare_error(reread_file? 
			     LITTLE_RECOMPARE_ERROR:LITTLE_COMPARE_ERROR, i);
	       if (reread_sector) {
                  lseek(fd, -(BLOCKSIZE), 1);
                  i--;
	          reread_sector = FALSE;
	       }
	       if (reread_file && !continue_reread) return;
	       continue_reread = FALSE;
            }
	    if (!(verbose || load_test)) {
               if ((i % PASS_SLEEP_MOD) == 0) sleep(1);
	    }
	 }
      }
      if (debug) {
         if (i != 0) printf("Completed %d little read(s)", i);
         else printf("No little reads");
	 printf(", sharing for %d second(s).\n", 4);
      }
}

erase_retension(type)
int type;
{
   if (simulate_error == RET_OPEN_ERROR) strcpy(device, "/invalid/file");

   for (comp = com; comp->c_name != NULL; comp++)
      if (strncmp(cp, comp->c_name, strlen(cp)) == 0) break;
   if (debug) printf("Opening '%s' for erase/retension.\n", device);
   if ((fd = open(device, comp->c_ronly ? 0 : 2)) < 0) 
      open_error(RET_OPEN_ERROR);
   mt_com.mt_op = comp->c_code;
   mt_com.mt_count = 1;
   if (!debug || (debug && 
      (simulate_error == ERASE_ERROR || simulate_error == RETENSION_ERROR))) {
      if (type == ERASE_ERROR && simulate_error == ERASE_ERROR) close(fd);
      if (type == RETENSION_ERROR && simulate_error == RETENSION_ERROR) 
	 close(fd);

      if ((mt_sts = ioctl(fd, MTIOCTOP, &mt_com) < 0)) {
         perror(perror_msg);
         sprintf(msg, "%s failed on '%s',%s errors %d%s",
                     comp->c_name, device, print_pass()? pass_msg:"",
                     errors, print_test_time()? test_time_msg:".");
         if (atn) send_msg_to_atn(FATAL, msg);
         throwup(-type, "%s: %s", TEST_NAME, msg);
      }
   }
   if (debug) printf("Closing '%s' after erase/retension.\n", device);
   close(fd);
   if (debug) printf("'%s' %s op = %d count = %d.\n", device, comp->c_name,
                      mt_com.mt_op, mt_com.mt_count);
   if(!verbose) sleep(60);
}

open_error(type)
int type;
{
   perror(perror_msg);
   sprintf(msg, "Couldn't open file '%s',%s errors %d%s",
	      device, print_pass()? pass_msg:"", errors,
	      print_test_time()? test_time_msg:".");
   if (atn) send_msg_to_atn(FATAL, msg);
   throwup(-type, "%s: %s", TEST_NAME, msg);
}

transfer_error(type, which_one, block)
int type;
int which_one;
int block;
{
   int stop;

   stop = TRUE;
   if (reread_file) reread_errors++;
   else errors++;

   perror(perror_msg);
   switch (type) {
      case BIG_WRITE_ERROR:
	 sprintf(tmp_msg, "Big write");
	 break;
      case LITTLE_WRITE_ERROR:
	 sprintf(tmp_msg, "Little write");
	 break;
      case BIG_READ_ERROR:
      case BIG_REREAD_ERROR:
	 sprintf(tmp_msg, "Big %sread", reread_file? "re":"");
	 if (!stop_on_error) stop = FALSE;
	 break;
      case LITTLE_READ_ERROR:
      case LITTLE_REREAD_ERROR:
	 sprintf(tmp_msg, "Little %sread", reread_file? "re":"");
	 if (!stop_on_error) stop = FALSE;
   }
   sprintf(msg, "%s %d failed on '%s', block %d,%s %serrors %d%s", 
		 tmp_msg, which_one, device, block, print_pass()? pass_msg:"", 
		 reread_file? "reread ":"",
		 reread_file? reread_errors:errors, 
		 print_test_time()? test_time_msg:".");
   if (atn) send_msg_to_atn(ERROR, msg);
   throwup(stop? -type : type, "ERROR: %s, %s", TEST_NAME, msg);
   return_status = failed;
   error_threshold();
}

compare_error(type, i)
int type;
int i;
{
   if (reread_file) reread_errors++;
   else errors++;

   data_sb.i = i;
   for (error_byte = 0; (data_sb.b[error_byte] == dev_buf[error_offset + error_byte]) && error_byte < 4; error_byte++); 

   switch (type) {
      case BIG_COMPARE_ERROR:
      case BIG_RECOMPARE_ERROR:
         sprintf(tmp_msg, "big %sread %d, byte %d (the read started at block %d, the error was in block %d)", reread_file? "re":"", i/NBLOCKS + 1, error_offset + error_byte,  i, i + (error_offset/BLOCKSIZE));
         break;
      case LITTLE_COMPARE_ERROR:
      case LITTLE_RECOMPARE_ERROR:
         sprintf(tmp_msg, "little %sread %d, block %d byte %d", reread_file? "re":"", i + 1, start_block + i, error_offset + error_byte);
   }

   sprintf(msg, "Compare error on '%s', %s,%s %serrors %d%s", device, tmp_msg, print_pass()? pass_msg:"", reread_file? "reread ":"", reread_file? reread_errors:errors, print_test_time()? test_time_msg:".");
   if (atn) send_msg_to_atn(ERROR, msg);
   throwup(type, "ERROR: %s, %s", TEST_NAME, msg);
   return_status = failed;

   if (display_read_data) {
      display_data();
      if (debug && !reread_file) {
	 if (stop_on_error && !testing_cartridge_tape) {
            printf("Same blocks? y/n ");
            scanf("%s", ch);
            if (ch[0] == 'y') {
	       reread_sector = TRUE;
            }
	 }
      }
   }
   if (check_threshold) error_threshold();
}

display_data()
{
   int b;

   sprintf(msg, "Data should be (in hex): \n%x %x %x %x %x %x %x %x %x %x %x %x ... (repeating pattern)\n",data_sb.b[0], data_sb.b[1], data_sb.b[2], data_sb.b[3], data_sb.b[0], data_sb.b[1], data_sb.b[2], data_sb.b[3], data_sb.b[0], data_sb.b[1], data_sb.b[2], data_sb.b[3]);
   printf(msg);
   fprintf(stderr, msg); 

   sprintf(msg, "Data starting at the word in error was (in hex):\n");
   printf(msg); 
   fprintf(stderr, msg);  

   for (b = 0; b < 20; b++) {
      printf("%x ", dev_buf[error_offset + b]);
      fprintf(stderr, "%x ", dev_buf[error_offset + b]);
   }
   printf("\n");
   fprintf(stderr, "\n\n");
   fflush(stderr);
   if (stop_on_error) {
      if (testing_cartridge_tape) {
         gettimeofday(&current_time, 0);
         elapse_time = current_time.tv_sec - head_start_time;
         head_use_time += elapse_time;
         total_test_time += elapse_time;
         more_tape_time = FALSE;
      }
      printf("Continue read check? y/n ");
      scanf("%s", ch);
      if (ch[0] == 'n') {
	 if (reread_file) exit(return_status);
         printf("Reread the entire file? y/n ");
         scanf("%s", ch);
         if (ch[0] == 'n') {
            exit(return_status);
         }
         reread_file = TRUE;
      }
      else if (reread_file) continue_reread = TRUE;
      check_threshold = FALSE;
      gettimeofday(&current_time, 0);
      head_start_time = current_time.tv_sec;
      more_tape_time = TRUE;
   }
}

error_threshold()
{
  if (stop_on_error) exit (return_status);
  if (errors >= ERROR_THRESHOLD) {
     if ((testing_cartridge_tape || using_disk_file) && !reread_file) {
       reread_file = TRUE;
     }
     else {
	if (reread_file) sprintf(tmp_msg, ", reread errors %d", reread_errors);
        sprintf(msg, "Stopped, error limit exceeded on '%s',%s errors %d%s%s",
 	    device, print_pass()? pass_msg:"", errors, 
	    reread_file? tmp_msg:"",
	    print_test_time()? test_time_msg:".");
        if (atn) send_msg_to_atn(FATAL, msg);
        throwup(-return_status, "%s: %s", TEST_NAME, msg);
     }
  } 
}

check_clean_time()
{
   gettimeofday(&current_time, 0);
   elapse_time = current_time.tv_sec - head_start_time;
   head_use_time += elapse_time;
   total_test_time += elapse_time;
   more_tape_time = FALSE;
   sleep_time = elapse_time;
   slept_already = 0;
   if (debug) {
      printf("'%s', tape pass time = %d, head time = %d, total time = %d.\n",
	      device, elapse_time, head_use_time, total_test_time);
      sleep_time = 10;
   }
   if (head_use_time >= head_clean_time) {
      gettimeofday(&current_time, 0);
      tape_sleep_start = current_time.tv_sec;
      if (atn) {
         sprintf(msg, "'%s', time to clean the 1/4 inch tape drive head%s",
		       device, print_test_time()? test_time_msg:".");
         send_msg_to_atn(WARNING, msg);
         printf("%s: %s %s", TEST_NAME, msg, ctime(&current_time));
      }
      if (debug) fprintf(stderr, "%s %s", msg, ctime(&current_time));
      bzero(date_time_buffer, sizeof(date_time_buffer));
      strncpy(date_time, (ctime(&current_time)),24);
      if (atn) {
	 if (!debug) sleep(atn_head_clean_time);
	 else printf("Sleeping %d seconds for atn head cleaning.\n", 
		      atn_head_clean_time);
      }
      else {
         ch[0] = ' ';
         while (ch[0] != 'c') {
            printf("\n%s: Clean the 1/4 inch tape drive head and enter c to continue / t to terminate (%s): ", TEST_NAME, date_time);
            scanf("%s", ch);
            if (ch[0] == 't') finish();
         }
      }
      head_use_time = 0;
      gettimeofday(&current_time, 0);
      if (debug) 
	 fprintf(stderr, "The 1/4 inch tape drive head was cleaned%s %s", 
                   print_test_time()? test_time_msg:".", ctime(&current_time));
      slept_already = current_time.tv_sec - tape_sleep_start;
   }
   sleep_time = sleep_time - slept_already;
   if (do_tape_sleep && sleep_time > 1 && sleep_time < 1000000) {
      printf("%s: '%s' sleeping for %d minutes. %s", 
              TEST_NAME, device, sleep_time/60, ctime(&current_time));
      if (!debug) sleep(sleep_time);
   } 
   else printf("%s: '%s' no sleep.\n", TEST_NAME, device);
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

print_test_time()
{
   if (looping && testing_cartridge_tape) {
      if (more_tape_time) {
         gettimeofday(&current_time, 0);
         current_test_time = total_test_time + 
	    (current_time.tv_sec - head_start_time);
      }
      else current_test_time = total_test_time;
      sprintf(test_time_msg, ", test time %1.1f hours.", 
 	                        (current_test_time/60)/60.0);
      return TRUE;
   } 
   else return FALSE;
}

/* NOTE: this routine is currently unused, until futher information is
         known about its worthwhileness such as when is the status reset,
         are the counters really retries and underruns etc.
*/

get_tape_status()

{
/*
   if (ioctl(fd, MTIOCGET, (char *)&mt_status) < 0) {
      perror(perror_msg);
      if (atn) send_msg_to_atn(ERROR, "Tape status operation on '%s' failed%s",
                    device, print_test_time()? test_time_msg:".");
      throwup(-12, "%s: Tape status operation on '%s' failed%s",
                    TEST_NAME, device, print_test_time()? test_time_msg:".");
   }
   printf("%s: retries=%d underruns=%d.\n",
           TEST_NAME, mt_status.mt_fileno, mt_status.mt_blkno);
*/
}

print_pass()
{
   if (looping) {
      sprintf(pass_msg, " pass %d,", pass);
      return TRUE;
   }
   else return FALSE;
}

finish()
{
   if (looping || verbose) {
      sprintf(msg, "Stopped testing '%s', pass %d, errors %d%s",
                    device, pass, errors, print_test_time()? test_time_msg:".");
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
