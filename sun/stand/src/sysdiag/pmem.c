#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/file.h>
#include <nlist.h>
#include <setjmp.h>
#include <signal.h>

#define FALSE                   0
#define TRUE                    ~FALSE
#define INFO                    0
#define WARNING                 1
#define FATAL                   2
#define ERROR                   3
#define NO_SD_LOG_DIR           1
#define NO_OPEN_LOG             2
#define NO_KMEM_FILE         	3
#define BAD_NAMELIST           	4
#define BAD_PHYSMEM_VALUE	5
#define NO_PHYSMEM             	6
#define WRONG_SIZE_MEM        	7
#define NO_MEM_FILE             8
#define BAD_VALLOC             	9
#define BAD_MMAP             	10
#define SIGBUS_ERROR           	11
#define SIGSEGV_ERROR          	12
#define END_ERROR               13
#define USAGE_ERROR             99

#define TEST_NAME		"pmem"	
#define LOGFILE_NAME		"log.pmem.XXXXXX"

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

static char     sccsid[] = "@(#)pmem.c 1.1 9/25/86 Copyright 1985 Sun Micro";

#define PASS_LOG_TIME           60
struct  timeval current_time;
int     log_time = 0, log_elapse_time = 0;
char file_name_buffer[30];
char *file_name = file_name_buffer;

#define	CTOB(x)			(((u_long)(x)) * pagesize)
#define PADDR(page, addr)	(CTOB(page) + ((u_long)addr & (pagesize-1)))
#define HOWOFTEN		0x10000000

struct  nlist nl[] = {
	{ "_physmem" },
	{ "" }
};

jmp_buf	error_buf;
int	physmem, size = 0;
u_char	*p;
int	signalgot;
int	code;

main(argc, argv)
int  argc;
char *argv[];
{
   int arrcount, match;
   extern finish();

   extern u_char	*valloc();
   register u_char	c, *pageaddr;
   register		i, j, pagesize = getpagesize();
   int			fd, clock;
   int			bus(), segv(), intr(), term(), hup();

   signal(SIGBUS, bus);
   signal(SIGSEGV, segv);
   signal(SIGINT, finish);
   signal(SIGTERM, finish);
   signal(SIGHUP, finish);

   if (getenv("SD_LOG_DIRECTORY")) exec_by_sysdiag = TRUE;
   if (getenv("SD_LOAD_TEST"))
        if (strcmp(getenv("SD_LOAD_TEST"), "yes") == 0) sd_load_test = TRUE;
   if (getenv("SUN_MANUFACTURING")) {
      if (strcmp(getenv("SUN_MANUFACTURING"), "yes") == 0) {
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
                        if (strncmp(argv[arrcount], "s=", 2) == 0) {
                                size = (atoi(&argv[arrcount][2]));
				match = TRUE;
                                size *= 512;
                        }
                        if (!match) {
                           printf("Usage: %s [v] [sd/atn] [s={size}m] [re] [lt] [dd] [d] [e{1-%d}]\n", TEST_NAME, END_ERROR - 1);
                           exit(USAGE_ERROR);
                        }        
      }
 
   if (simulate_error ==  NO_KMEM_FILE) strcpy(file_name, "/invalid/kmem");
   else strcpy(file_name, "/dev/kmem");

   if ((fd = open(file_name, 0)) < 0) open_error(NO_KMEM_FILE);

   nlist("/vmunix", nl);
   if (simulate_error ==  BAD_NAMELIST) nl[0].n_type = 0;
   if (nl[0].n_type == 0) {
      sprintf(msg, "Defective namelist in '/vmunix'.");
      if (atn) send_msg_to_atn(FATAL, msg);
      throwup(-BAD_NAMELIST, "%s: %s", TEST_NAME, msg);
   }
   lseek(fd, (long)nl[0].n_value, 0);

   if (simulate_error ==  BAD_PHYSMEM_VALUE) close(fd);
   if (read(fd, (char *)&physmem, sizeof (physmem)) != sizeof (physmem)) {
      perror(perror_msg);
      sprintf(msg, "Couldn't read the value of physmem in '/vmunix'.");
      if (atn) send_msg_to_atn(FATAL, msg);
      throwup(-BAD_PHYSMEM_VALUE, "%s: %s", TEST_NAME, msg);
   }
   else {
      if (simulate_error ==  NO_PHYSMEM) physmem = 0;
      if (!physmem) {
	 sprintf(msg, "Found 0 bytes of physical memory.");
	 if (atn) send_msg_to_atn(FATAL, msg); 
         throwup(-NO_PHYSMEM, "%s: %s", TEST_NAME, msg);
      }
   }
   close(fd);

   if (simulate_error ==  WRONG_SIZE_MEM) size = 256;
   if (size) if (size != physmem) {
      sprintf(msg, "The specified memory size (%1.1f Meg) is not the same as the size found (%1.1f Meg).", CTOB(size)/1048576.0, CTOB(physmem)/1048576.0); 
      if (atn) send_msg_to_atn(FATAL, msg);
      throwup((debug && simulate_error !=  WRONG_SIZE_MEM)? 
	 WRONG_SIZE_MEM : -WRONG_SIZE_MEM, "%s: %s", TEST_NAME, msg);
      physmem = size;
   }

   if (simulate_error ==  NO_MEM_FILE) strcpy(file_name, "/invalid/mem");
   else strcpy(file_name, "/dev/mem");

   if ((fd = open(file_name, O_RDONLY)) < 0) open_error(NO_MEM_FILE);

   if ((pageaddr = valloc(pagesize)) == 0 || simulate_error ==  BAD_VALLOC) {
	perror(perror_msg);
	sprintf(msg, "'valloc' results incorrect.");
	if (atn) send_msg_to_atn(FATAL, msg);
	throwup(-BAD_VALLOC, "%s: %s", TEST_NAME, msg);
   }

   if (verify) {               /* verify mode */
      printf("%s: Found 0x%x (%1.1f Meg) bytes of physical memory.\n", 
		TEST_NAME, CTOB(physmem),CTOB(physmem)/1048576.0);
      exit(0);
   }
  
   nice(20);

   sprintf(msg, "Started with 0x%x (%1.1f Meg) bytes to check.", 
		CTOB(physmem),CTOB(physmem)/1048576.0, pagesize);
   if (atn) send_msg_to_atn(INFO, msg);
   throwup(0, "%s: %s", TEST_NAME, msg);

   if (load_test) physmem = 0x200;
	
   gettimeofday(&current_time, 0);
   log_time = current_time.tv_sec;

   for(pass = 1, errors = 0; ;pass++){

      for(i = 0; i < physmem ; i++){

         switch(setjmp(error_buf)){
            case 0:
                  if (debug && display_read_data) 
	             printf("page = 0x%x/%d, addr = 0x%x\n", i, i, pageaddr);

	          if (simulate_error ==  BAD_MMAP) close(fd);
                  if (mmap(pageaddr, pagesize, PROT_READ, MAP_SHARED, 
							    fd, CTOB(i)) < 0){
                     perror(perror_msg);
	             sprintf(msg, "'mmap' incorrect results, page = 0x%x/%d, addr = 0x%x, pass %d, errors %d.", i, i, pageaddr, pass, errors);
	             if (atn) send_msg_to_atn(FATAL, msg);
	             throwup(-BAD_MMAP, "%s: %s", TEST_NAME, msg);
                  }
                  for (j = 0, p = pageaddr; j < pagesize ; j++,p++){
                     c = *p;
                     asm("nop");
                  }
                  if (simulate_error == SIGBUS_ERROR) bus();
                  if (i == 1 && simulate_error == SIGSEGV_ERROR) segv();
		  break;
            case 1:
            case 2:
                  errors++;
                  sprintf(msg, 
		     "%s at page 0x%x/%d, paddr 0x%x, pass %d, errors %d.",
		      (signalgot == SIGBUS) ? 
		      "bus error" : "segmentation violation", i, i,  
		      PADDR(i,p), pass, errors);
                  if (atn) send_msg_to_atn(ERROR, "%s", msg);

		  if (signalgot == SIGBUS) code = SIGBUS_ERROR;
		  else code = SIGSEGV_ERROR;
                  throwup(stop_on_error? -code : code,
		      "ERROR: %s, %s", TEST_NAME, msg);
                  break;

            default:
                  throwup(-END_ERROR, "Unknown return code from 'setjmp'.\n");
         }

      } /* for(i = 0; i < physmem ; i++) */

      gettimeofday(&current_time, 0);
      log_elapse_time = current_time.tv_sec - log_time;
      sprintf(msg, "%s: pass %d, errors %d.", TEST_NAME, pass, errors);
      if (getenv("LOG_PASS_MSG") && log_elapse_time >= PASS_LOG_TIME * 60) {
          throwup(0, "%s", msg);
          log_time = current_time.tv_sec;
      }
      else printf("%s\n", msg);
      if (load_test) break;

   } /* for(pass = 1, errors = 0; ;pass++) */
   throwup(0, "%s: Stopped, pass %d, errors %d.", TEST_NAME, pass, errors);
}

open_error(type)
int type;
{
   perror(perror_msg);
   sprintf(msg, "Couldn't open file '%s'.", file_name);
   if (atn) send_msg_to_atn(FATAL, msg);
   throwup(-type, "%s: %s", TEST_NAME, msg);
}

bus()
{
        signalgot = SIGBUS;
        longjmp(error_buf, 1);
}

segv()
{
	signalgot = SIGSEGV;
	longjmp(error_buf, 2);
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

finish()
{
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
