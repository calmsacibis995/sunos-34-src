#include <sys/types.h>
#include <stdio.h>
#include <sys/file.h>
#include <signal.h>

#define FALSE           	0
#define TRUE            	~FALSE
#define INFO                    0
#define WARNING                 1
#define FATAL                   2
#define ERROR                   3
#define NO_SD_LOG_DIR           1
#define NO_OPEN_LOG             2
#define FP_EXCEPTION_ERROR      3
#define COMPUTATION_ERROR       4
#define TEST_NAME               "fptest"
#define LOGFILE_NAME	"log.fptest.XXXXXX"
#define TIMES   100000
#define SLEEP_MOD 1000
#define ANSWER_SB  1.6364567280
#define MARGIN      .0000000010

char device_name[15];
char *device = device_name;
int  atn = FALSE;
int  debug = FALSE;
int  pass = 0;
int errors = 0;

int failed = 0, logfd = 0;
char logfile_name[50];
char *logfile = logfile_name;
extern char *getenv();

#ifdef sun3
   extern          minitfp_();
   extern          winitfp_();
#endif

int exec_by_sysdiag = FALSE;
int verify = FALSE;
int load_test = FALSE;
int simulate_error = 0;
char pass_msg_buffer[20];
char *pass_msg = pass_msg_buffer;

static char     sccsid[] = "@(#)fptest.c 1.1 9/25/86 Copyright 1985 Sun Micro";

main(argc, argv)
int	argc;
char	*argv[];
{
   int arrcount, match;
   register float  x,y,z;
   register int    i,j;
   extern finish();
   extern float_exp();
   int fpa = FALSE;

   if (getenv("SD_LOG_DIRECTORY")) exec_by_sysdiag = TRUE;
   if (getenv("SD_LOAD_TEST")) 
	if (strcmp(getenv("SD_LOAD_TEST"), "yes") == 0) load_test = TRUE;
	
   if (argc > 1)
      for(arrcount=1; arrcount < argc; arrcount++) {
                        match = 0;
                        if (strcmp(argv[arrcount], "sd") == 0) {
                                match = TRUE;
                                exec_by_sysdiag = TRUE;
                                }
                        if (strcmp(argv[arrcount], "atn") == 0) {
                                match = TRUE;
                                atn = TRUE;
                                }
                        if (strcmp(argv[arrcount], "d") == 0) {
                                match = TRUE;
                                debug = TRUE;
                                }
                        if (strcmp(argv[arrcount], "v") == 0) {
                                match = TRUE;
                                verify = TRUE;
                                }
                        if (strcmp(argv[arrcount], "lt") == 0) {
                                match = TRUE;
                                load_test = TRUE;
                                }
                        if (argv[arrcount][0] == 'e') {
                                simulate_error = atoi(&argv[arrcount][1]);
				if (simulate_error > 0 && simulate_error < 5) {
				   match = TRUE;
				   debug = TRUE;
				   }
                                }
                        if (match == 0) {
                                printf("Usage: %s {v} {sd/atn} {lt} {d} {e1-4}\n",
					TEST_NAME);
                                exit(1);
                                }
                        }

   if (verify && debug) printf("%s: Verify mode.\n", TEST_NAME);

#ifdef sun3
	                                          /* for when fpa is ready */
   if (winitfp_()) {
      if (debug) printf("%s: An FPA is installed\n", TEST_NAME);
      if (verify) exit(21);
      else strcpy(device, "FPA");
   }
   if (minitfp_()) {
      if (debug) printf("%s: A 68881 is installed\n", TEST_NAME);
      if (verify) exit(22);
      else strcpy(device, "MC68881");
   }
   else {
      if (debug) printf("%s: No FPA or 68881 is installed\n", TEST_NAME);
      if (verify) exit(23);
      else strcpy(device, "softfp");
   }
#else
   if (debug) printf("%s: Not a sun3\n", TEST_NAME);
   if (verify) exit(0);
   else strcpy(device, "softfp");
#endif
   
   signal(SIGHUP, finish);
   signal(SIGTERM, finish);
   signal(SIGINT, finish);
   signal(SIGFPE, float_exp);


   if (atn || debug) {
       if (atn) send_msg_to_atn(INFO, "Started.");
       throwup(0, "%s: Started.", TEST_NAME);
   }
   nice(20);
   while (atn || pass == 0) {
      pass++;
      if (simulate_error == COMPUTATION_ERROR) x = 1.4568;
           else 
           x = 1.4567;
           y = 1.1234;

      if (!load_test) j = TIMES;
      else j = 1;

      for ( i = 0; i < j;  ++i ) {
           z = x * y;
          if (simulate_error == FP_EXCEPTION_ERROR) float_exp();
          if (debug) printf("Result: was (%2.10f), expected (%2.10f), Diff (%2.10e)\n", z, ANSWER_SB, z - ANSWER_SB);
          if (z < (ANSWER_SB - MARGIN) || z > (ANSWER_SB + MARGIN)) {
               errors++;
               if (atn) send_msg_to_atn(ERROR, "Result: was (%2.10f), expected (%2.10f), pass %d, errors %d", z, ANSWER_SB, pass, errors);
               throwup(-4, "ERROR: %s, Result: was (%2.10f), expected (%2.10f),%s errors %d", TEST_NAME, z, ANSWER_SB, print_pass()? pass_msg:"", errors);
          }
          if (!load_test && (i % SLEEP_MOD) == 0) sleep (3);
      }
      if (atn) printf("%s: pass %d, errors %d\n", TEST_NAME, pass, errors);
   }
   if (debug) 
       throwup(0, "%s: Stopped, pass %d, errors %d", TEST_NAME, pass, errors);
}

throwup(where, fmt, a, b, c, d, e, f, g)
int	where;
char	*fmt;
u_long	a, b, c, d, e, f, g;
{
	extern char	*mktemp();
	int		clock;

	if (!logfd){
		if (simulate_error == NO_OPEN_LOG) {
			strcpy(logfile, "not/valid/log");
		}else {
                   if (getenv("SD_LOG_DIRECTORY") 
		       && !(simulate_error == NO_SD_LOG_DIR)) {
                      strcpy(logfile,(getenv("SD_LOG_DIRECTORY")));
                      strcat(logfile,"/");
                      strcat(logfile,LOGFILE_NAME);
                   }
                   else {
                      if (atn) send_msg_to_atn
			       (FATAL, "No log file environmental variable.");
                      fprintf(stderr,"%s: No log file environmental variable.\n",
			      TEST_NAME);
                      exit(2);
                   }
		}
		if ((logfd =open(mktemp(logfile),O_WRONLY|O_CREAT|O_APPEND
			,0644)) <0){
			if (atn) send_msg_to_atn
				(FATAL, "Couldn't open logfile '%s'", logfile);
		        fprintf(stderr, 
				"%s: Couldn't open logfile '%s'\n",
				TEST_NAME, logfile);
			perror(TEST_NAME);
			fprintf(stderr,fmt, a, b, c, d, e, f, g);
			clock = time(0);
			fprintf(stderr, " %s", ctime(&clock));
			exit(where);
		} else {
			dup2(logfd, 2);		/* get stderr as logfile */
		}
	}
	fprintf(stderr, fmt, a, b, c, d, e, f, g);
	clock = time(0);
	fprintf(stderr, " %s", ctime(&clock));
	printf(fmt, a, b, c, d, e, f, g);
	printf(" %s", ctime(&clock));
	fsync(2);
	if (where < 0)
		exit(-where);
	failed = where;
}
float_exp()
{
   if (atn) send_msg_to_atn(ERROR, "Floating point exception interrupt, pass %d, errors %d", pass, errors);
   throwup(-3, "%s: Floating point exception interrupt,%s errors %d", TEST_NAME,  print_pass()? pass_msg:"", errors);
}  
print_pass()
{
   if (atn) {
      sprintf(pass_msg, " pass %d,", pass);
      return TRUE;
   }
   else return FALSE;
}
finish()
{
   if (atn || debug) {
      if (atn) 
	  send_msg_to_atn(INFO, "Stopped, pass %d, errors %d", pass, errors);
      throwup(0, "%s: Stopped, pass %d, errors %d", TEST_NAME, pass, errors);
      if (atn) exit(0);
   }
   exit(20);
}
send_msg_to_atn()
{
printf("This is not the ATN version!\n");
}

