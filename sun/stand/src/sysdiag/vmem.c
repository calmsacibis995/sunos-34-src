#include <sys/types.h>
#include <stdio.h>
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
#define NO_MEMORY_ALLOCATED     3
#define INSUFFICIENT_MEMORY     4
#define LOST_MEMORY     	5
#define COMPARE_ERROR           6
#define RECOMPARE_ERROR         7
#define SIGBUS_ERROR            8
#define SIGSEGV_ERROR           9
#define END_ERROR               10
#define USAGE_ERROR             99

#define TEST_NAME               "vmem"
#define LOGFILE_NAME            "log.vmem.XXXXXX"

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
int sd_load_test = FALSE;
int simulate_error = 0;
char perror_msg_buffer[30];
char *perror_msg = perror_msg_buffer;
char tmp_msg_buffer[30];
char *tmp_msg = tmp_msg_buffer;
char msg_buffer[200];
char *msg = msg_buffer;
int  retry_cmp_error = FALSE;
int  display_read_data = FALSE;
int shoebox_test = FALSE;
 
char pass_msg_buffer[20];
char *pass_msg = pass_msg_buffer;

static char     sccsid[] = "@(#)vmem.c 1.1 9/25/86 Copyright 1985 Sun Micro";

#ifdef sun3
#define SUN_MARGIN      0x100000
#define SHOEBOX_MARGIN  0x0
#else
#define SUN_MARGIN      0x80000
#define SHOEBOX_MARGIN  0x80000
#endif
#define TTY_MARGIN      0xa0000
#define COLOR_MARGIN    0x1a0000

#define MEMSIZE		0x800000

extern u_long obs_value;

int  return_status = 0;

char		*start = 0;
int		index, size = 0, search, margin;

main(argc, argv)
int	argc;
char	*argv[];
{
   int arrcount, match;
   extern       finish();

   extern char 	*malloc(), *realloc();
   int          bus(), segv();

   signal(SIGHUP, finish);
   signal(SIGTERM, finish);
   signal(SIGINT, finish);
   signal(SIGBUS, bus);
   signal(SIGSEGV, segv);

   if (getenv("SD_LOG_DIRECTORY")) exec_by_sysdiag = TRUE;
   if (getenv("SD_LOAD_TEST"))
      if (strcmp(getenv("SD_LOAD_TEST"), "yes") == 0) sd_load_test = TRUE;
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
   sprintf(perror_msg, "%s: perror says", TEST_NAME);
   strcpy(device, "\0");

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
                        if (strncmp(argv[arrcount], "s=", 2) == 0) {
                                match = TRUE;
				size = (atoi(&argv[arrcount][2]) * 1000);
                        }
                        if (!match) {
                           printf("Usage: %s [v] [sd/atn] [s={size}k] [re] [lt] [dd] [d] [e{1-%d}]\n", TEST_NAME, END_ERROR - 1);
                           exit(USAGE_ERROR);
                        }
      }  

   if (size == 0) {
      if (debug) printf("size\n");

      for(size = MEMSIZE ; size > sizeof(long); size >>=1){
         if(start = malloc(size)){
            free(start);
            if (debug) printf("0x%x y\n", size);
            break;
         } else {
            if (debug) printf("0x%x n\n", size);
         }
      }

      if (!start || simulate_error == NO_MEMORY_ALLOCATED){
         if (verbose) throwup(-NO_MEMORY_ALLOCATED, 
                     "%s: Could not allocate any memory.", TEST_NAME);
         else exit(NO_MEMORY_ALLOCATED);
      }

      if (debug) printf("search\n");

      for (search = size/2 ; search > sizeof(long); search >>= 1){
         if (start = malloc(size + search)){
            free(start);
            if (debug) printf("0x%x y\n", search);
            size += search;
         } else {
            if (debug) printf("0x%x n\n", search);
         }
      }
   }
   search = size;

   if (verify) {             /* verify mode */

        printf("%s: Found 0x%x (%1.1f Meg) bytes of virtual memory.\n",
		TEST_NAME, search, search/1048576.0);
   }
   else {

        if (strcmp(getenv("TERM"), "sun") == 0) margin = SUN_MARGIN;
        else margin = TTY_MARGIN;
        if (!atn) if (getenv("COLOR_MONITOR")) margin += COLOR_MARGIN; 
	if (shoebox_test) margin += SHOEBOX_MARGIN;
        size -= margin;
        if (debug) printf("Margin is 0x%x.\n", margin);

        if (load_test || debug) size /= 10;

	if (simulate_error == INSUFFICIENT_MEMORY) size = 2;
        if ((size < sizeof(long))) {
           if (atn || verbose) {
              sprintf(msg, "Insufficient memory available.");
              if (atn) send_msg_to_atn(FATAL, msg);
              throwup(-INSUFFICIENT_MEMORY, "%s: %s", TEST_NAME, msg);
           }
           else exit(INSUFFICIENT_MEMORY);
        }
        if (simulate_error == LOST_MEMORY) size = 0x1700000;
        if (!(start = malloc(size))) {
           if (atn || verbose) {
              sprintf(msg, "Lost memory at 0x%x.", size);
              if (atn) send_msg_to_atn(FATAL, msg);
              throwup(-LOST_MEMORY, "%s: %s", TEST_NAME, msg);
           }
           else exit(LOST_MEMORY);
        }
	if (sd_load_test) size /= 10;
	if (atn || verbose) {
	    sprintf(msg, "Started, found 0x%x testing 0x%x bytes.",search,size);
            if (atn) send_msg_to_atn(INFO, msg);
            throwup(0, "%s: %s", TEST_NAME, msg);
	}
        else printf("%s: Found 0x%x testing 0x%x bytes.\n", 
		     TEST_NAME, search,size);
	
	nice(15);
	while (atn || pass == 0) {
	   pass++;
	   lfill(start, size, 0x91348742);

           if (simulate_error == SIGBUS_ERROR) bus();
           if (simulate_error == SIGSEGV_ERROR) segv();
	   if (simulate_error == COMPARE_ERROR) lfill(start + 4, 4, 0x91348752);
	   if (simulate_error == RECOMPARE_ERROR) 
		lfill(start + 8, 4, 0x92348742);

	   printf("%s: Written\n", TEST_NAME);

	   if (!(load_test || verbose)) sleep (5);

	   if (index = o_lcheck(start, size, 0x91348742)) {
	      errors++;
	      display_error_msg();
	      if (!atn || stop_on_error) exit(return_status);
	   } 
	   printf("%s: Read\n", TEST_NAME);

	   if (!(load_test || verbose)) sleep (5);
	   if (atn)  {
	      printf("%s: pass %d, errors %d.\n", TEST_NAME, pass, errors);
	      if (load_test) break;
	   }
        }
	if (atn || verbose) throwup(0, 
		   "%s: Stopped, pass %d, errors %d.", TEST_NAME, pass, errors);
	exit(return_status);
   }
}

display_error_msg()
{
   sprintf(msg, 
    "ompare error at 0x%x, expected 0x%x, actual 0x%x, start address was 0x%x,%s errors %d.", ((long *)start) + size/sizeof(long) - index, 0x91348742, obs_value, start, print_pass()? pass_msg:"", errors);
   if (atn) send_msg_to_atn(ERROR, "C%s", msg);
   throwup(retry_cmp_error? COMPARE_ERROR : -COMPARE_ERROR, 
      "ERROR: %s, c%s", TEST_NAME, msg);

   sprintf(msg, "The data reread at error location 0x%x was 0x%x.",
      ((long *)start) + size/sizeof(long) - index, 
      ((long *)start)[size/sizeof(long)-index]);
   if (atn) send_msg_to_atn(INFO, msg);
   printf("%s: %s\n", TEST_NAME, msg);
   fprintf(stderr, "%s: %s\n", TEST_NAME, msg);

   if (simulate_error == COMPARE_ERROR) lfill(start + 4, 4, 0x91348742);
   if (index = o_lcheck(start, size, 0x91348742)) {

      sprintf(msg, "Error on recompare at 0x%x, expected 0x%x, actual 0x%x.",
         ((long *)start) + size/sizeof(long) - index, 0x91348742, obs_value);
      if (atn) send_msg_to_atn(INFO, msg);
      printf("%s: %s\n", TEST_NAME, msg);
      fprintf(stderr, "%s: %s\n", TEST_NAME, msg);

      sprintf(msg, "The data reread at recompare error location 0x%x was 0x%x.",
	 ((long *)start) + size/sizeof(long) - index, 
	 ((long *)start)[size/sizeof(long)-index]);
      if (atn) send_msg_to_atn(INFO, msg);
      printf("%s: %s\n", TEST_NAME, msg);
      fprintf(stderr, "%s: %s\n", TEST_NAME, msg);
      return_status = RECOMPARE_ERROR;
   }
   else {
      sprintf(msg, "A recompare of all the virtual memory was successful.");
      if (atn) send_msg_to_atn(INFO, msg);
      printf("%s: %s\n", TEST_NAME, msg);
      fprintf(stderr, "%s: %s\n", TEST_NAME, msg);
      return_status = COMPARE_ERROR;
   }
   fflush(stderr);
}

print_pass()
{
   if (atn) {
      sprintf(pass_msg, " pass %d,", pass);
      return TRUE;
   }
   else return FALSE;
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

bus()
{
   errors++;
   sprintf(msg, "us error,%s errors %d.", print_pass()? pass_msg:"", errors);
   if (atn) send_msg_to_atn(ERROR, "B%s", msg);
   throwup(-SIGBUS_ERROR, "ERROR: %s, b%s", TEST_NAME, msg);
}

segv()
{
   errors++;
   sprintf(msg, "egmentation violation,%s errors %d.", 
                 print_pass()? pass_msg:"", errors);
   if (atn) send_msg_to_atn(ERROR, "S%s", msg);
   throwup(-SIGSEGV_ERROR, "ERROR: %s, s%s", TEST_NAME, msg);
}

finish()
{
   if (atn || verbose) {
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
