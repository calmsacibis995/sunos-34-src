static char     fpasccsid[] = "@(#)fpatest.c 1.1 9/25/86 Copyright Sun Microsystems";


#include <sys/types.h>
#include <stdio.h>
#include <sys/file.h>
#include <signal.h>
#include <math.h>
#include <errno.h>
#include <sys/mman.h>
#include <nlist.h>
#include <setjmp.h>
#include "fpa.h"
#include <sys/time.h>
#include <sys/wait.h>

#define pi			3.141592654

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
#define SPMATH_ERROR		5
#define SYSTEST_ERROR		6
#define PROBE_EEPROM_ERROR	7
#define DPMATH_ERROR		10
#define SKY_ERROR		12
#define NO_COMPARE_FILES	69
#define ERROR_USAGE		99
#define TIMES   		10000
#define SLEEP_MOD 		1000
#define ANSWER_SB  		1.6364567280
#define MARGIN      		.0000000010
#define SPMARGIN		.0000001
#define DPMARGIN		.000000000000001
#define fp_absent		0
#define fp_enabled		1

#define	CTOB(x)			(((u_long)(x)) * pagesize)
#define BTOC(x)			(((u_long)(x)) / pagesize)
#define PADDR(page, addr)	(CTOB(page) + ((u_long)addr & (pagesize-1)))

#ifdef sun3
#ifdef FPA
#define Device			"FPA"
#define LOGFILE_NAME		"log.fpatest.XXXXXX"
#define TEST_NAME		"fpatest"
#endif

#ifdef MC68881
#define Device			"MC68881"
#define LOGFILE_NAME		"log.mc68881.XXXXXX"
#define TEST_NAME		"mc68881"
#endif

#ifdef SOFT
#define Device			"softfp"
#define LOGFILE_NAME		"log.softfp.XXXXXX"
#define TEST_NAME		"softfp"
#endif

#else
#define Device                  "softfp"
#define LOGFILE_NAME            "log.softfp.XXXXXX"
#define TEST_NAME               "softfp"
#endif

char device_name[15];
char *device = device_name;
int  atn = FALSE;
int sending_atn_msg = FALSE;
int  debug = FALSE;
int  pass = 0;
int errors = 0;
int verbose = TRUE;

int failed = 0, logfd = 0;
char logfile_name[50];
char *logfile = logfile_name;
extern char *getenv();
char sysdiag_directory[50];
char *SD = sysdiag_directory;


extern int errno;

#ifdef sun3
   extern          minitfp_();
   extern          mywinitfp_();
#endif
 
int exec_by_sysdiag = FALSE;
int verify = FALSE;
int load_test = FALSE;
int simulate_error = 0;
int go_on_881error = 0; /*default is disable */
char perror_msg_buffer[30];
char *perror_msg = perror_msg_buffer;
char tmp_msg_buffer[30];
char *tmp_msg = tmp_msg_buffer;


#ifdef FPA
extern int fpa_systest();
extern int	sigsegv_handler();
#endif


main(argc, argv)
int	argc;
char	*argv[];
{
   int arrcount, match;
   register float  x,y,z;
   register int    i,j;
   extern finish();
   int fpa = FALSE;
   int	spmath(), dpmath();
   u_long *fpa_pointer;
   int	type;
   char *fp;
   int	probe881(), probe;


   if (getenv("SD_LOG_DIRECTORY")) exec_by_sysdiag = TRUE;
   if (getenv("SD_LOAD_TEST")) 
	if (strcmp(getenv("SD_LOAD_TEST"), "yes") == 0) load_test = TRUE;

   sprintf(perror_msg, " perror says");
	
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
                        if (strcmp(argv[arrcount], "v") == 0) {
                                match = TRUE;
                                verify = TRUE;
                                }
                        if (strcmp(argv[arrcount], "lt") == 0) {
                                match = TRUE;
                                load_test = TRUE;
                                }
			if (strcmp(argv[arrcount], "c") == 0) {
				match = TRUE;
				go_on_881error = TRUE;
				}
                        if (argv[arrcount][0] == 'e') {
                                simulate_error = atoi(&argv[arrcount][1]);
				if (simulate_error > 0 && simulate_error < 8) {
				   match = TRUE;
				   }
                                }
                        if (match == 0) {
                           printf("Usage: %s [v] [sd/atn] [lt] [d] [e{1-4,6,7}]\n",TEST_NAME);
                           exit(ERROR_USAGE);
                           }
                        }
#ifdef sun3
#ifdef ATN_VERSION

#ifdef SOFT
   if (minitfp_())
   {
	if (simulate_error == PROBE_EEPROM_ERROR) probe = 0;
	else
		probe = probe881();
	if (probe == -1)
	{
		send_msg_to_atn(FATAL,"Could not find configuration for MC68881 in EEPROM.");
		throwup(-PROBE_EEPROM_ERROR,"Could not find configuration for MC68881 in EEPROM.");
	}
	if (probe == 0)
	{
		printf("System Configuration does not have an MC68881 configured for this system. \n");
		if (verify) exit(0);
		if (go_on_881error == 0)
		{
			printf("Please remove the MC68881 now.\n");
			send_msg_to_atn(FATAL,"System configuration DOES NOT account for an MC68881 to be installed.");
			throwup(-PROBE_EEPROM_ERROR,"System has an MC68881, but system configuration does not have it configured for this system.");
		}
	}
   }
   if (minitfp_() == 0)
   {
	if (simulate_error == PROBE_EEPROM_ERROR) probe = 1;
	else
		probe = probe881();
	if (probe == -1) 
	{
		send_msg_to_atn(FATAL,"Could not find configuration for MC68881 in EEPROM."); 
		throwup(-PROBE_EEPROM_ERROR,"Could not find configuration for MC68881 in EEPROM."); 
	}
	if (probe == 1)
	{
		printf("System Configuration requires an MC68881 to be installed.\n ");
		if (verify) exit(0);
		if (go_on_881error == 0)
		{
			printf("Please install an MC68881 now.\n");
			send_msg_to_atn(FATAL,"System configuration REQUIRES an MC68881 to be installed.");
			throwup(-PROBE_EEPROM_ERROR,"System does not have an MC68881, but system configuration has it configured for this system.");
		}
   	}
   }
#endif /*endif for def of SOFT */

#ifdef FPA
   if (minitfp_() == 0)
   {
	if (simulate_error == PROBE_EEPROM_ERROR) probe = 1;
	else
		probe = probe881();
	if (probe == -1)
	{
		send_msg_to_atn(FATAL,"Could not find configuration for MC68881 in EEPROM.");
		throwup(-PROBE_EEPROM_ERROR,"Could not find configuration for MC68881 in EEPROM.");
	}
	if (probe == 1)
	{
		printf("System Configuration requires an MC68881 to be installed. \n");
		if (verify) exit(0);
		if (go_on_881error == 0)
		{
			printf("Please install an MC68881 now.\n");
			send_msg_to_atn(FATAL,"System configuration REQUIRES an MC68881 to be installed.");
			throwup(-PROBE_EEPROM_ERROR,"System does not have an MC68881, but system configuration has it configured for this system.");
		}
   	}
   }
#endif /*endif for def of FPA */

#ifdef MC68881
   if (minitfp_() == 0)
   {
	if (simulate_error == PROBE_EEPROM_ERROR) probe = 1;
	else
		probe = probe881();
	if (probe == -1)
	{
		send_msg_to_atn(FATAL,"Could not find configuration for MC68881 in EEPROM.");
		throwup(-PROBE_EEPROM_ERROR,"Could not find configuration for MC68881 in EEPROM.");
	}
	if (probe == 1)
	{
		printf("System Configuration requires an MC68881 to be installed. \n");
		if (verify) exit(0);
		if (go_on_881error == 0)
		{
			printf("Please install an MC68881 now.\n");
			send_msg_to_atn(FATAL,"System configuration REQUIRES an MC68881 to be installed.");
			throwup(-PROBE_EEPROM_ERROR,"System does not have an MC68881, but system configuration has it configured for this system.");
		}
   	}
   }
#endif /*endif for def of mc68881 */

#endif /*endif for def of ATN_VERSION */
#endif /*endif for def of sun3 */

#ifdef sun3
#ifndef ATN_VERSION
   if (getenv("SUN_MANUFACTURING"))
   {
	if (strcmp(getenv("SUN_MANUFACTURING"), "yes") == 0)
	{
   		if (minitfp_())
   		{
			if (simulate_error == PROBE_EEPROM_ERROR) probe = 0;
			else
				probe = probe881();
			if (probe == -1)
			{
				throwup(-PROBE_EEPROM_ERROR,"Could not find configuration for MC68881 in EEPROM.");
			}
			if (probe == 0)
			{
				printf("System Configuration does not have a MC68881 configured for this system. \n");
				if (verify) exit(0);
				if (go_on_881error == 0)
				{
					printf("Please remove the MC68881 now.\n");
					throwup(-PROBE_EEPROM_ERROR,"System has an MC68881, but system configuration does not have it configured for this system."); 
				}
			}
   		}
   		if (minitfp_() == 0)
   		{
			if (simulate_error == PROBE_EEPROM_ERROR) probe = 1;
			else
				probe = probe881();
			if (probe == -1)
			{
				throwup(-PROBE_EEPROM_ERROR,"Could not find configuration for MC68881 in EEPROM.");
			}
			if (probe == 1)
			{
				printf("System Configuration requires an MC68881 to be installed. \n");
				if (verify) exit(0);
				if (go_on_881error == 0)
				{
					printf("Please install an MC68881 now.\n");
					throwup(-PROBE_EEPROM_ERROR,"System does not have an MC68881, but system configuration has it configured for this system.");
				}
			}
   		}
	}
   }
#endif /*end of ifndef ATN_VERSION */
#endif /* end of ifdef sun3 */
		

   if (verify && verbose) printf("%s: Verify mode.\n", Device);
#ifdef sun3
	                                          /* for when fpa is ready */
   if (mywinitfp_()) {
      if (verbose) printf(" An FPA is installed\n");
      if (verify) exit(21);
      else strcpy(device, "FPA");
   }
   if (minitfp_()) {
      if (verbose) printf(" An MC68881 is installed\n");
      if (verify) exit(22);
      else strcpy(device, "MC68881");
   }
   else {
      if (verbose) printf("No FPA or 68881 is installed\n");
      if (verify) exit(23);
      else strcpy(device, "softfp");
   }
#else
   if (verbose) printf("Not a sun3\n");
   if (verify) exit (0);
   else strcpy(device, "softfp");
#endif
   
   signal(SIGHUP, finish);
   signal(SIGTERM, finish);
   signal(SIGINT, finish);

#ifdef FPA
   signal(SIGFPE,sigsegv_handler);
   signal(SIGSEGV,sigsegv_handler);
#endif

   
   if (atn || verbose) {
       if (atn) send_msg_to_atn(INFO, "Started.");
       throwup(0, "%s: Started.", Device);
   }
   nice(20);

   while (atn || pass == 0) {
      pass++;
#ifdef FPA
   if (fpa_systest() == (-1))
   {
   	errors++;
 	if (simulate_error == SYSTEST_ERROR)
	{
		if (atn) send_msg_to_atn(FATAL,"Failed Systest for FPA.");
		throwup(-SYSTEST_ERROR, "Failed systest for FPA.");
	}
	if (atn) send_msg_to_atn(FATAL,"Failed Systest for FPA");
	throwup(-SYSTEST_ERROR,"Failed Systest for FPA");
   }
   if (simulate_error == FP_EXCEPTION_ERROR)
   {
	printf("Can not test fp exception using this test.\n");
	throwup(-FP_EXCEPTION_ERROR,"%s: errors %d.",Device,errors);
   }
   if (atn || verbose) printf("%s: Passed Systest for FPA.\n",Device);
   if (atn || verbose) 
       throwup(0, "%s: Stopped, pass %d, errors %d.", Device, pass, errors);

#endif

#ifdef MCNSOFT

      if (simulate_error == SYSTEST_ERROR)
   {
	printf("Can not test this error for this test.\n");
	throwup(-simulate_error,"%s: pass %d, errors %d.",Device,pass,errors);
   }
      if (spmath())
      {
	if (atn) send_msg_to_atn(ERROR,"Single Precision %s Math Error",Device);
	throwup(-SPMATH_ERROR,"Failed single precision FPA math test.");
      }
      if (atn || verbose) printf("Passed single precision FPA math test using the %s. \n",Device);
      if (dpmath())
      {
	if (atn) send_msg_to_atn(ERROR,"Double Precision %s Math Error",Device);
	throwup(-DPMATH_ERROR,"Failed double precision FPA math test.");
      }
      if (atn || verbose) printf("Passed double precision FPA math test using the %s.\n",Device);
      if (simulate_error == COMPUTATION_ERROR)  x = 1.4568; 
           else 
           x = 1.4567;
           y = 1.1234;

      if (load_test || verbose) j = 1;
      else j = TIMES;

      if (debug) printf("Starting multiple multiplications!\n");
      for ( i = 0; i < j;  ++i ) {
          z = x * y;
	  if (simulate_error == FP_EXCEPTION_ERROR) float_exp();
          if (debug) printf("Result: was (%2.10f), expected (%2.10f), Diff (%2.10e)\n", z, ANSWER_SB, z - ANSWER_SB);
          if (z < (ANSWER_SB - MARGIN) || z > (ANSWER_SB + MARGIN)) {
               errors++;
               if (atn) send_msg_to_atn(ERROR, "Result: was (%2.10f), expected (%2.10f), pass %d, errors %d.", z, ANSWER_SB, pass, errors);
               throwup(-COMPUTATION_ERROR, "%s: Result: was (%2.10f), expected (%2.10f),%s errors %d.", Device, z, ANSWER_SB, print_pass()? tmp_msg:"", errors);
          }
	  if (debug && (i % SLEEP_MOD) == 0) printf("i = %d\n",i);
          if (!debug && !load_test && (i % SLEEP_MOD) == 0) sleep (3);
      }
      if (load_test) break;
      if (atn || verbose) 
         printf("%s: pass %d, errors %d.\n", Device, pass, errors);
/*
   if (simulate_error > 0 && simulate_error <  6) throwup(-simulate_error, "%s: Stopped, pass %d, errors %d.", Device, pass, errors);
*/
   if (debug) printf("simulate_error = %d\n",simulate_error);
       if (atn || verbose)
       		throwup(0, "%s: Stopped, pass %d, errors %d.", Device, pass, errors);
#endif /* endif for def of MC68881 */

   } /* end of while */
} /*end of main */


spmath()
{
	float 	a,b,ans;
	int	spfpa;

	a = 1.2345;
	b = 0.9876;

	/* Basic tests of the following arithmetic operations: +, -, *, and /   */
	if (verbose) printf("%s: Starting SPMATH routine\n",Device);
	for (spfpa = 1; spfpa < 100; ++spfpa)
	{
	if (debug && (spfpa % 10) == 0) printf("%s: spfpa= %d\n",Device,spfpa);
	ans = a + b;
	ans = a + b;
	if (ans != 2.2221000)
	{
		if (ans < (2.2221000 - SPMARGIN) || ans > (2.2221000 + SPMARGIN))
		{
			printf("Error:  a + b\n");
			printf("Expected: 2.2221000    	Actual: %1.7f\n",ans);
			return(1);
		}
	}
	ans = (a - b);
	if (ans != 0.2469000)
	{
		if (ans < (0.2469000 - SPMARGIN) || ans > (0.2469000 + SPMARGIN))
		{
			printf("Error   a - b\n");
			printf("Expected: 0.2469000    	Actual: %1.7f\n",ans);
			return(1);
		}
	}
	ans = a * b;
	if (ans != 1.2191923)
	{
		if (ans < (1.2191923 - SPMARGIN) || ans > (1.2191923 + SPMARGIN))
		{
			printf("Error   a * b\n");
			printf("Expected: 1.2191922    	Actual: %1.7f\n",ans);
			return(1);
		}
	}
	ans = a / b;
	if (ans != 1.2500000)
	{
		if (ans < (1.2500000 - SPMARGIN) || ans > (1.2500000 + SPMARGIN))
		{
			printf("Error   a / b\n");
			printf("Expected: 1.2500000	Actual: %1.7f\n",ans);
			return(1);
		}
	}
	ans = a + (a - b);
	if (ans != 1.4814000)
	{
		if (ans < (1.4814000 - SPMARGIN) || ans > (1.4814000 + SPMARGIN))
		{
			printf("Error:  a + (a + b)\n");
			printf("Expected: 1.4814000 	Actual: %1.7f\n",ans);
			return(1);
		}
	}
	ans = a - (a + b);
	if (ans != -(0.9876000))
	{
		if (ans < (-(0.9876000) - SPMARGIN) || ans > (-(0.9876000) + SPMARGIN))
		{
			printf("Error:  a - (a + b)\n");
			printf("Expected: -0.9876000 	Actual: %1.7f\n",ans);
			return(1);
		}
	}
	ans = a + (a * b);
	if (ans != 2.4536924)
	{
		if (ans < (2.4536924 - SPMARGIN) || ans > (2.4536924 + SPMARGIN))
		{
			printf("Error:  a + (a * b)\n");
			printf("Expected: 2.4536924  	Actual: %1.7f\n",ans);
			return(1);
		}
	}
	ans = a - (a * b);
	if (ans != 0.0153078)
	{
		if (ans < (0.0153078 - SPMARGIN) || ans > (0.0153078 + SPMARGIN))
		{
			printf("Error:  a - (a * b)\n");
			printf("Expected: 0.0153078    	Actual: %1.7f\n",ans);
			return(1);
		}
	}
	ans = a + (a / b);
	if (ans != 2.4845002)
	{
		if (ans < (2.4845002 - SPMARGIN) || ans > (2.4845002 + SPMARGIN))
		{
			printf("a + (a / b)\n");
			printf("Expected: 2.4845002   	 Actual: %1.7f\n",ans);
			return(1);
		}
	}
	ans = a - (a / b);
	if (ans != -(0.0155000))
	{
		if (ans < (-(0.0155000) - SPMARGIN) || ans > (-(0.0155000) + SPMARGIN))
		{
			printf("Error:  a - (a / b)\n");
			printf("Expected: -0.0155000   	 Actual: %1.7f\n",ans);
			return(1);
		}
	}
	ans = a * (a + b);
	if (ans != 2.7431827)
	{
		if (ans < (2.7431827 - SPMARGIN) || ans > (2.7431827 + SPMARGIN))
		{
			printf("Error:  a * (a + b)\n");
			printf("Expected: 2.7431825   	 Actual: %1.7f\n",ans);
			return(1);
		}
	}
	ans = a * (a - b);
	if (ans != 0.3047981)
	{
		if (ans < (0.3047981 - SPMARGIN) || ans > (0.3047981 + SPMARGIN))
		{
			printf("Error:  a * ( a - b)\n");
			printf("Expected: 0.3047980   	 Actual: %1.7f\n",ans);
			return(1);
		}
	}
	ans = a / (a + b);
	if (ans != 0.5555556)
	{
		if (ans < (0.5555556 - SPMARGIN) || ans > (0.5555556 + SPMARGIN))
		{
			printf("Error:  a / ( a - b)\n");
			printf("Expected: 0.5555550      Actual: %1.7f\n",ans);
			return(1);
		}
	}
	ans = a / (a - b);
	if (ans != 4.9999995)
	{
		if (ans < (4.9999995 - SPMARGIN) || ans > (4.9999995 + SPMARGIN))
		{
			printf("Error:  a / ( a - b)\n");
			printf("Expected: 5.0000000   	 Actual: %1.7f\n",ans);
			return(1);
		}
	}
	ans = a * (a / b);
	if (ans != 1.5431250)
	{
		if (ans < (1.5431250 - SPMARGIN) || ans > (1.5431250 + SPMARGIN))
		{
			printf("Error:  a * ( a / b)\n");
			printf("Expected: 1.5431250   	 Actual: %1.7f\n)",ans);
			return(1);
		}
	}
	ans = a / (a * b);
	if (ans != 1.0125557)
	{
		if (ans < (1.0125557 - SPMARGIN) || ans > (1.0125557 + SPMARGIN))
		{
			printf("Error:  a / ( a * b)\n");
			printf("Expected: 1.0125557	 Actual: %1.7f\n)",ans);
			return(1);
		}
	}
	
	if (load_test) break;
	if (!load_test && (spfpa % 25) ==0) sleep(1);
	} /* end of for loop */
	return(0);

}

dpmath()
{
	double		x,result;
	long float 	a,b,ans;
	int		dpfpa;

	a = 1.2345;
	b = 0.9876;
	/* Basic tests of the following arithmetic operations: +, -, *, and /   */
	if (verbose) printf("%s: Starting DPMATH routine\n",Device);
	for (dpfpa = 1; dpfpa < 100; ++dpfpa)
	{

	if (debug && (dpfpa % 10) == 0) printf("%s: dpfpa= %d\n",Device,dpfpa);
	ans = (a + b );
	if (ans != 2.222100000000000)
	{
		if (ans < (2.222100000000000 - DPMARGIN) || ans > (2.222100000000000 + DPMARGIN))
		{
			printf("Error:  a + b\n");
			printf("Expected: 2.222100000000000    	Actual: %1.15f\n",ans);
			return(1);
		}
	}
	ans = (a - b);
	if (ans != 0.246899999999999)
	{
		if (ans < (0.246899999999999 - DPMARGIN) || ans > (0.246899999999999 + DPMARGIN))
		{
			printf("Error:  a - b\n");
			printf("Expected: 0.246899999999999    	Actual: %1.15f\n",ans);
			return(1);
		}
	}
	ans = a * b;
	if (ans != 1.219192199999999)
	{
		if (ans < (1.219192199999999 - DPMARGIN) || ans > (1.219192199999999 + DPMARGIN))
		{
			printf("Error:  a * b\n");
			printf("Expected: 1.219192199999999    	Actual: %1.15f\n",ans);
			return(1);
		}
	}
	ans = a / b;
	if (ans != 1.249999999999999)
	{
		if (ans < (1.249999999999999 - DPMARGIN) || ans > (1.249999999999999 + DPMARGIN))
		{
			printf("Error:  a / b\n");
			printf("Expected: 1.249999999999999	Actual: %1.15f\n",ans);
			return(1);
		}
	}
	ans = a + (a - b);
	if (ans != 1.481399999999999)
	{
		if (ans < (1.481399999999999 - DPMARGIN) || ans > (1.481399999999999 + DPMARGIN))
		{
			printf("Error:  a + (a - b)\n");
			printf("Expected: 1.481399999999999 	Actual: %1.15f\n",ans);
			return(1);
		}
	}
	ans = a - (a + b);
	if (ans != -(0.987600000000000))
	{
		if (ans < (-(0.987600000000000) - DPMARGIN) || ans > (-(0.987600000000000) + DPMARGIN))
		{
			printf("Error:  a - (a + b)\n");
			printf("Expected: -0.987600000000000 	Actual: %1.15f\n",ans);
			return(1);
		}
	}
	ans = a + (a * b);
	if (ans != 2.453692199999999)
	{
		if (ans < (2.453692199999999 - DPMARGIN) || ans > (2.453692199999999 + DPMARGIN))
		{
			printf("Error:  a + (a * b)\n");
			printf("Expected: 2.453692199999999  	Actual: %1.15f\n",ans);
			return(1);
		}
	}
	ans = a - (a * b);
	if (ans != 0.015307800000000)
	{
		if (ans < (0.015307800000000 - DPMARGIN) || ans > (0.015307800000000 + DPMARGIN))
		{
			printf("Error:  a - (a * b)\n");
			printf("Expected: 0.015307800000000    	Actual: %1.15f\n",ans);
			return(1);
		}
	}
	ans = a + (a / b);
	if (ans != 2.484499999999999)
	{
		if (ans < (2.484499999999999 - DPMARGIN) || ans > (2.484499999999999 + DPMARGIN))
		{
			printf("Error:  a + (a / b)\n");
			printf("Expected: 2.484499999999999   	 Actual: %1.15f\n",ans);
			return(1);
		}
	}
	ans = a - (a / b);
	if (ans != -(0.015499999999999))
	{
		if (ans < (-(0.015499999999999) - DPMARGIN) || ans > (-(0.015499999999999) + DPMARGIN))
		{
			printf("Error:  a - (a / b)\n");
			printf("Expected: -0.015499999999999   	 Actual: %1.15f\n",ans);
			return(1);
		}
	}
	ans = a * (a + b);
	if (ans != 2.743182449999999)
	{
		if (ans < (2.743182449999999 - DPMARGIN) || ans > (2.743182449999999 + DPMARGIN))
		{
			printf("Error:  a * (a + b)\n");
			printf("Expected: 2.743182449999999   	 Actual: %1.15f\n",ans);
			return(1);
		}
	}
	ans = a * (a - b);
	if (ans != 0.304798049999999)
	{
		if (ans < (0.304798049999999 - DPMARGIN) || ans > (0.304798049999999 + DPMARGIN))
		{
			printf("Error:  a * (a - b)\n");
			printf("Expected: 0.304798049999999   	 Actual: %1.15f\n",ans);
			return(1);
		}
	}
	ans = a / (a + b);
	if (ans != 0.555555555555555)
	{
		if (ans < (0.555555555555555 - DPMARGIN) || ans > (0.555555555555555 + DPMARGIN))
		{
			printf("Error:  a / (a + b)\n");
			printf("Expected: 0.555555555555555      Actual: %1.15f\n",ans);
			return(1);
		}
	}
	ans = a / (a - b);
	if (ans != 5.000000000000001)
	{
		if (ans < (5.000000000000001 - DPMARGIN) || ans > (5.000000000000001 + DPMARGIN))
		{
			printf("Error:  a / (a - b)\n");
			printf("Expected: 5.000000000000001   	 Actual: %1.15f\n",ans);
			return(1);
		}
	}
	ans = a * (a / b);
	if (ans != 1.543124999999999)
	{
		if (ans < (1.543124999999999 - DPMARGIN) || ans > (1.543124999999999 + DPMARGIN))
		{
			printf("Error:  a * (a / b)\n");
			printf("Expected: 1.543124999999999   	 Actual: %1.15f\n)",ans);
			return(1);
		}
	}
	ans = a / (a * b);
	if (ans != 1.012555690562980)
	{
		if (ans < (1.012555690562980 - DPMARGIN) || ans > (1.012555690562980 + DPMARGIN))
		{
			printf("Error:   a / (a * b)\n");
			printf("Expected: 1.0125555690562980	 Actual: %1.15f\n)",ans);
			return(1);
		}
	}
/* Start Double Precision test of trg functions */

	/* sin of values in the range of -2pi to +2pi   */
	result = sin(-(pi * 2));
	if (result != -(0.000000000820413))
	{
		if (result < (-(0.000000000820413) - DPMARGIN) || result > (-(0.000000000820413) + DPMARGIN))
		{
			printf("Error:  sin(-2pi)\n");
			printf("Expected: -0.000000000820413 Actual: %1.15f\n",result);
			return(1);
 		}
	}
	result = sin((pi * (-3)) / 2);
	if (result != 1.0000000000000000)
	{
		if (result < (1.0000000000000000 - DPMARGIN) || result > (-0.000000000000000 + DPMARGIN))
		{
			printf("Error: sin(-3pi/2)\n");
			printf("Expected: 1.0000000000000000 Actual: %1.15f\n",result);
			return(1);
 		}
	}
	result = sin(-(pi));
	if (result != 0.000000000410206)
	{
		if (result < (0.000000000410206 - DPMARGIN) || result > (0.00000000410206 + DPMARGIN))
		{
			printf("Error: sin(-pi)\n");
			printf("Expected: 0.000000000410206 Actual: %1.15f\n",result);
			return(1);
 		}
	}
	result = sin(-(pi / 2));
	if (result != -(1.0000000000000000))
	{
		if (result < (-(1.0000000000000000) - DPMARGIN) || result > (-(1.0000000000000000) + DPMARGIN))
		{
			printf("Error: sin(-pi/2)\n");
			printf("Expected: -1.0000000000000000 Actual: %1.15f\n",result);
			return(1);
 		}
	}
	result = sin(0);
	if (result != (0.0000000000000000))
	{
		if (result < (0.0000000000000000 - DPMARGIN) || result > (0.000000000000000 + DPMARGIN))
		{
			printf("Error: sin(0)\n");
			printf("Expected: 0.0000000000000000       Actual: %1.15f\n",result);
			return(1);
 		}
	}
	result = sin(pi / 2);
	if (result !=  1.0000000000000000)
	{
		if (result < (1.0000000000000000 - DPMARGIN) || result > (1.0000000000000000 + DPMARGIN))
		{
			printf("Error: sin(pi/2)\n");
			printf("Expected: 1.0000000000000000       Actual: %1.15f\n",result);
			return(1);
 		}
	}
	result = sin(pi);
	if (result != -(0.000000000410206))
	{
		if (result < (-(0.000000000410206) - DPMARGIN) || result > (-(0.000000000410206) + DPMARGIN))
		{
			printf("Error: sin(pi)\n");
			printf("Expected: -0.000000000410206       Actual: %1.15f\n",result);
			return(1);
 		}
	}
	result = sin((pi * 3) / 2);
	if (result != -(1.0000000000000000))
	{
		if (result < (-(1.0000000000000000) - DPMARGIN) || result > (-(1.0000000000000000) + DPMARGIN))
		{
			printf("Error: sin(3pi/2)\n");
			printf("Expected: -1.0000000000000000 Actual: %1.15f\n",result);
			return(1);
 		}
	}
	result = sin(pi * 2);
	if (result != 0.000000000820143)
	{
		if (result < (0.000000000820143 - DPMARGIN) || result > (0.00000000820143 + DPMARGIN))
		{
			printf("Error: sin(2pi)\n");
			printf("Expected: 0.000000000820143 Actual: %1.15f\n",result);
			return(1);
 		}
	}

	/* cos of values in the range of -2pi to +2pi   */
	result = cos(pi * (-2));
	if (result !=  1.0000000000000000)
	{
		if (result < (1.0000000000000000 - DPMARGIN) || result > (1.0000000000000000 + DPMARGIN))
		{
			printf("Error: cos(-2pi)\n");
			printf("Expected: 1.0000000000000000       Actual: %1.15f\n",result);
			return(1);
 		}
	}
	result = cos((pi * (-3)) / 2);
	if (result != 0.000000000615310)
	{
		if (result < (0.000000000615310 - DPMARGIN) || result > (0.00000000615310 + DPMARGIN))
		{
			printf("Error: cos(-3pi/2)\n");
			printf("Expected: 0.000000000615310 Actual: %1.15f\n",result);
			return(1);
 		}
	}
	result = cos(-pi);
	if (result != -(1.0000000000000000))
	{
		if (result < (-(1.0000000000000000) - DPMARGIN) || result > (-(1.0000000000000000) + DPMARGIN))
		{
			printf("Error: cos(-pi)\n");
			printf("Expected: -1.0000000000000000 Actual: %1.15f\n",result);
			return(1);
 		}
	}
	result = cos(-(pi / 2));
	if (result != -(0.000000000205103))
	{
		if (result < (-(0.000000000205103) - DPMARGIN) || result > (-(0.000000000205103) + DPMARGIN))
		{
			printf("Error: cos(-pi/2)\n");
			printf("Expected: -0.000000000205103 Actual: %1.15f\n",result);
			return(1);
 		}
	}
	result = cos(0);
	if (result !=  1.0000000000000000)
	{
		if (result < (1.0000000000000000 - DPMARGIN) || result > (1.0000000000000000 + DPMARGIN))
		{
			printf("Error: cos(0)\n");
			printf("Expected: 1.0000000000000000       Actual: %1.15f\n",result);
			return(1);
 		}
	}
	result = cos(pi / 2);
	if (result != (-0.000000000205103))
	{
		if (result < (-(0.000000000205103) - DPMARGIN) || result > (-(0.000000000205103) + DPMARGIN))
		{
			printf("Error: cos(pi/2)\n");
			printf("Expected: -0.000000000205103 Actual: %1.15f\n",result);
			return(1);
 		}
	}
	result = cos(pi);
	if (result != (-1.0000000000000000))
	{
		if (result < (-(1.0000000000000000) - DPMARGIN) || result > (-(1.0000000000000000) + DPMARGIN))
		{
			printf("Error: cos(pi)\n");
			printf("Expected: -1.0000000000000000 Actual: %1.15f\n",result);
			return(1);
 		}
	}
	result = cos((pi * 3) / 2);
	if (result != (0.000000000615310))
	{
		if (result < (0.000000000615310 - DPMARGIN) || result > (0.00000000615310 + DPMARGIN))
		{
			printf("Error: cos(3pi/2)\n");
			printf("Expected: 0.000000000615310 Actual: %1.15f\n",result);
			return(1);
 		}
	}
	result = cos(pi * 2);
	if (result !=  1.0000000000000000)
	{
		if (result < (1.0000000000000000 - DPMARGIN) || result > (1.0000000000000000 + DPMARGIN))
		{
			printf("Error: cos(pi/2)\n");
			printf("Expected: 1.0000000000000000       Actual: %1.15f\n",result);
			return(1);
 		}
	}
	
	 
	/* sin and cos of: pi/4, 3pi/4, 5pi/4 and 7pi/4  */
	result = sin(pi / 4);
	if (result != (0.707106781259062))
	{
		if (result < (0.707106781259062 - DPMARGIN) || result > (0.707106781259062 + DPMARGIN))
		{
			printf("Error: sin(pi/4)\n");
			printf("Expected: 0.707106781259062 Actual: %1.15f\n",result);
			return(1);
 		}
	}
	result = sin((pi * 3) / 4);
	if (result != 0.707106780969002)
	{
		if (result < (0.707106780969002 - DPMARGIN) || result > (0.707106780969002 + DPMARGIN))
		{
			printf("Error: sin(3pi/4)\n");
			printf("Expected: 0.707106780969002 Actual: %1.15f\n",result);
			return(1);
 		}
	}
	result = sin((pi * 5) / 4);
	if (result != -(0.707106781549122))
	{
		if (result < (-(0.707106781549122) - DPMARGIN) || result > (-(0.707106781549122) + DPMARGIN))
		{
			printf("Error: sin(5pi/4)\n");
			printf("Expected: -0.707106781549122 Actual: %1.15f\n",result);
			return(1);
 		}
	}
	result = sin((pi * 7) / 4);
	if (result != -(0.707106780678942))
	{
		if (result < (-(0.707106780678942) - DPMARGIN) || result > (-(0.707106780678942) + DPMARGIN))
		{
			printf("Error: sin(7pi/4)\n");
			printf("Expected: -0.707106780678942 Actual: %1.15f\n",result);
			return(1);
 		}
	}
	result = cos(pi / 4);
	if (result != 0.707106781114032)
	{
		if (result < (0.707106781114032 - DPMARGIN) || result > (0.707106781114032 + DPMARGIN))
		{
			printf("Error: cos(pi/4)\n");
			printf(" Expected: 0.707106781114032 Actual: %1.15f\n",result);
			return(1);
 		}
	}
	result = cos((pi * 3) / 4);
	if (result != -(0.707106781404092))
	{
		if (result < (-(0.707106781404092) - DPMARGIN) || result > (-(0.707106781404092) + DPMARGIN))
		{
			printf("Error: cos(3pi/4)\n");
			printf(" Expected: -0.707106781404092 Actual: %1.15f\n",result);
			return(1);
 		}
	}
	result = cos((pi * 5) / 4);
	if (result != -(0.707106780823972))
	{
		if (result < (-(0.707106780823972) - DPMARGIN) || result > (-(0.707106780823972) + DPMARGIN))
		{
			printf("Error: cos(5pi/4)\n");
			printf(" Expected: -0.707106780823972 Actual: %1.15f\n",result);
			return(1);
 		}
	}
	result = cos((pi * 7) / 4);
	if (result != (0.707106781694152))
	{
		if (result < (0.707106781694152 - DPMARGIN) || result > (0.707106781694152 + DPMARGIN))
		{
			printf("Error: cos(7pi/4)\n");
			printf(" Expected: 0.707106781694152 Actual: %1.15f\n",result);
			return(1);
 		}
	}

	/* exponential		*/
	x = exp(0.0);
	if (x != 1.0000000000000000)
	{
		if (x < (1.0000000000000000 - DPMARGIN) || x > (1.0000000000000000 + DPMARGIN))
		{
			printf("Error: exp(0)\n");
			printf(" Expected: 1.0000000000000000 Actual: %1.15f\n",x);
			return(1);
 		}
	}
	x = exp(1.0);
	if (x != 2.718281828459045)
	{
		if (x < (2.718281828459045 - DPMARGIN) || x > (2.718281828459045 + DPMARGIN))
		{
			printf("Error: exp(1)\n");
			printf(" Expected: 2.718281828459045 Actual: %1.15f\n",x);
			return(1);
 		}
	}
	x = exp(2.0);
	if (x != 7.389056098930650)
	{
		if (x < (7.389056098930650 - DPMARGIN) || x > (7.389056098930650 + DPMARGIN))
		{
			printf("Error: exp(2)\n");
			printf(" Expected: 7.389056098930650 Actual: %1.15f\n",x);
			return(1);
 		}
	}
	x = exp(5.0);
	if (x != 148.413159102576600)
	{
		if (x < (148.413159102576600 - DPMARGIN) || x > (148.413159102576600 + DPMARGIN))
		{
			printf("Error: exp(5)\n");
			printf(" Expected: 148.413159102576600 Actual: %1.15f\n",x);
			return(1);
 		}
	}
	x = exp(10.0);
	if (x != 22026.465794806718000)
	{
		if (x < (22026.465794806718000 - DPMARGIN) || x > (22026.465794806718000 + DPMARGIN))
		{
			printf("Error: exp(10)\n");
			printf(" Expected: 22026.465794806718000 Actual: %1.15f\n",x);
			return(1);
 		}
	}
	x = exp(-1.0);
	if (x != 0.367879441171442)
	{
		if (x < (0.367879441171442 - DPMARGIN) || x > (0.367879441171442 + DPMARGIN))
		{
			printf("Error: exp(-1)\n");
			printf(" Expected: 0.367879441171442 Actual: %1.15f\n",x);
			return(1);
 		}
	}
	x = exp(-2.0);
	if (x != 0.135335283236612)
	{
		if (x < (0.135335283236612 - DPMARGIN) || x > (0.135335283236612 + DPMARGIN))
		{
			printf("Error: exp(-2)\n");
			printf("Expected: 0.135335283236612 Actual: %1.15f\n",x);
			return(1);
 		}
	}
	x = exp(-5.0);
	if (x != 0.006737946999085)
	{
		if (x < (0.006737946999085 - DPMARGIN) || x > (0.006737946999085 + DPMARGIN))
		{
			printf("Error: exp(-5)\n");
			printf("Expected: 0.006737946999085 Actual: %1.15f\n",x);
			return(1);
 		}
	}
	x = exp(-10.0);
	if (x != 0.000045399929762)
	{
		if (x < (0.000045399929762 - DPMARGIN) || x > (0.000045399929762 + DPMARGIN))
		{
			printf("Error: exp(-10)\n");
			printf("Expected: 0.000045399929762 Actual: %1.15f\n",x);
			return(1);
 		}
	}
	x = exp(log(1.0));
	if (x != 1.0000000000000000)
	{
		if (x < (1.0000000000000000 - DPMARGIN) || x > (1.0000000000000000 + DPMARGIN))
		{
			printf("Error: exp(log(1)\n");
			printf("Expected: 1.0000000000000000 Actual: %1.15f\n",x);
			return(1);
 		}
	}
	x = exp(log(10.0));
	if (x != 10.000000000000002)
	{
		if (x < (10.000000000000002 - DPMARGIN) || x > (10.000000000000002 + DPMARGIN))
		{
			printf("Error: exp(log(10)\n");
			printf("Expected 10.000000000000002 Actual: %1.15f\n",x);
			return(1);
 		}
	}
	/* logarithms            */
	x = log(1.0);
	if (x != 0.0000000000000000)
	{
		if (x < (0.0000000000000000 - DPMARGIN) || x > (0.0000000000000000 + DPMARGIN))
		{
			printf("Error: log(1)\n");
			printf("Expected: 0.0000000000000000 Actual: %1.15f\n",x);
			return(1);
 		}
	}
	x = log(2.0);
	if (x != 0.693147180559945)
	{
		if (x < (0.693147180559945 - DPMARGIN) || x > (0.693147180559945 + DPMARGIN))
		{
			printf("Error: log(2)\n");
			printf("Expected: 0.693147180559945 Actual: %1.15f\n",x);
			return(1);
 		}
	}
	x = log(10.0);
	if (x != 2.302585092994045)
	{
		if (x < (2.302585092994045 - DPMARGIN) || x > (2.302585092994045 + DPMARGIN))
		{
			printf("Error: log(10)\n");
			printf("Expected: 2.302585092994045 Actual: %1.15f\n",x);
			return(1);
 		}
	}
	x = log(100.0);
	if (x != 4.605170185988091)
	{
		if (x < (4.605170185988091 - DPMARGIN) || x > (4.605170185988091 + DPMARGIN))
		{
			printf("Error: log(100)\n");
			printf("Expected: 4.605170185988091 Actual: %1.15f\n",x);
			return(1);
 		}
	}
	x = log(exp(0.0));
	if (x != 0.0000000000000000)
	{
		if (x < (0.0000000000000000 - DPMARGIN) || x > (0.0000000000000000 + DPMARGIN))
		{
			printf("Error: log(exp(0))\n");
			printf("Expected: 0.0000000000000000 Actual: %1.15f\n",x);
			return(1);
 		}
	}
	x = log(exp(1.0));
	if (x != 1.0000000000000000)
	{
		if (x < (1.0000000000000000 - DPMARGIN) || x > (1.0000000000000000 + DPMARGIN))
		{
			printf("Error: log(exp(1))\n");
			printf("Expected: 1.0000000000000000 Actual: %1.15f\n",x);
			return(1);
 		}
	}
	x = log(exp(10.0));
	if (x != 10.0000000000000000)
	{
		if (x < (10.0000000000000000 - DPMARGIN) || x > (10.0000000000000000 + DPMARGIN))
		{
			printf("Error: log(exp(10))\n");
			printf("Expected: 10.0000000000000000 Actual: %1.15f\n",x);
			return(1);
 		}
	}
	/* These functions are supported by the 68881 but not the FPA  */

	x = tan(-(2 * pi));
	if (x != -(0.000000000820414));
	{
		if (x < (-(0.000000000820414) - DPMARGIN) || x > (-(0.000000000820414) + DPMARGIN))
		{
			printf("Error: tan(-2pi)\n");
			printf("Expected: -0.000000000820414  Actual: %1.15f\n",x);
			return(1);
		}
	}
	x = tan(-(7 * pi) / 4);
	if (x != 0.999999998564275);
	{
		if (x < (0.999999998564275 - DPMARGIN) || x > (0.999999998564275 + DPMARGIN))
		{
			printf("Error: tan(-7pi/4)\n");
			printf("Expected: 0.999999998564275  Actual: %1.15f\n",x);
			return(1);
		}
	}
	x = tan(-(5 * pi) / 4);
	if (x != -(1.000000001025517));
	{
		if (x < (-(1.000000001025517) - DPMARGIN) || x > (-(1.000000001025517) + DPMARGIN))
		{
			printf("Error: tan(-5pi/4)\n");
			printf("Expected: -1.000000001025517  Actual: %1.15f\n",x);
			return(1);
		}
	}
	x = tan(-(pi));
	if (x != -(0.000000000410207));
	{
		if (x < (-(0.000000000410207) - DPMARGIN) || x > (-(0.000000000410207) + DPMARGIN))
		{
			printf("Error: tan(-pi\n");
			printf("Expected: 0.000000000410207  Actual: %1.15f\n",x);
			return(1);
		}
	}
	x = tan(-(3 *pi) / 4);
	if (x != 0.999999999384690);
	{
		if (x < (0.999999999384690 - DPMARGIN) || x > (0.999999999384690 + DPMARGIN))
		{
			printf("Error: tan(-3pi/4)\n");
			printf("Expected: 0.999999999384690  Actual: %1.15f\n",x);
			return(1);
		}
	}
	x = tan(-(pi) / 4);
	if (x != -(1.000000000205103));
	{
		if (x < (-(1.000000000205103) - DPMARGIN) || x > (-(1.000000000205103) + DPMARGIN))
		{
			printf("Error: tan(-pi/4)\n");
			printf("Expected: -1.000000000205103  Actual: %1.15f\n",x);
			return(1);
		}
	}
	x = tan(0.0);
	if (x != 0.000000000000000);
	{
		if (x < (0.000000000000000 - DPMARGIN) || x > (0.000000000000000 + DPMARGIN))
		{
			printf("Error: tan(0.0)\n");
			printf("Expected: 0.000000000000000  Actual: %1.15f\n",x);
			return(1);
		}
	}
	x = tan(pi / 4);
	if (x != 1.000000000205103);
	{
		if (x < (1.000000000205103 - DPMARGIN) || x > (1.000000000205103 + DPMARGIN))
		{
			printf("Error: tan(pi / 4)\n");
			printf("Expected: 1.000000000205103  Actual: %1.15f\n",x);
			return(1);
		}
	}
	x = tan((3 * pi) / 4);
	if (x != -0.999999999384690);
	{
		if (x < (-(0.999999999384690) - DPMARGIN) || x > (-(0.999999999384690) + DPMARGIN))
		{
			printf("Error: tan(3pi/4)\n");
			printf("Expected: -0.999999999384690  Actual: %1.15f\n",x);
			return(1);
		}
	}
	x = tan(pi);
	if (x != 0.000000000410207);
	{
		if (x < (0.000000000410207 - DPMARGIN) || x > (0.000000000410207 + DPMARGIN))
		{
			printf("Error: tan(pi)\n");
			printf("Expected: 0.000000000410207  Actual: %1.15f\n",x);
			return(1);
		}
	}
	x = tan((5 * pi) / 4);
	if (x != 1.000000001025517);
	{
		if (x < (1.000000001025517 - DPMARGIN) || x > (1.000000001025517 + DPMARGIN))
		{
			printf("Error: tan(5pi/4)\n");
			printf("Expected: 1.000000001025517  Actual: %1.15f\n",x);
			return(1);
		}
	}
	x = tan((7 * pi) / 4);
	if (x != -0.999999998564275);
	{
		if (x < (-(0.999999998564275) - DPMARGIN) || x > (-(0.999999998564275) + DPMARGIN))
		{
			printf("Error: tan(7pi/4)\n");
			printf("Expected: -0.999999998564275  Actual: %1.15f\n",x);
			return(1);
		}
	}
	x = tan((2 * pi));
	if (x != 0.000000000820414);
	{
		if (x < (0.000000000820414 - DPMARGIN) || x > (0.000000000820414 + DPMARGIN))
		{
			printf("Error: tan(2pi)\n");
			printf("Expected: 0.000000000820414  Actual: %1.15f\n",x);
			return(1);
		}
	}
	x = sqrt(0.0);
	if (x != 0.000000000000000)
	{
		if (x < (0.000000000000000 - DPMARGIN) || x > (0.000000000000000 + DPMARGIN))
		{
			printf("Error: sqrt(0)\n");
			printf("Expected: 0.000000000000000  Actual: %1.15f\n",x);
			return(1);
		}
	}
	x = sqrt(1.0);
	if (x != 1.000000000000000)
	{
		if (x < (1.000000000000000 - DPMARGIN) || x > (1.000000000000000 + DPMARGIN))
		{
			printf("Error: sqrt(1)\n");
			printf("Expected: 1.000000000000000  Actual: %1.15f\n",x);
			return(1);
		}
	}
	x = sqrt(4.0);
	if (x != 2.000000000000000)
	{
		if (x < (2.000000000000000 - DPMARGIN) || x > (2.000000000000000 + DPMARGIN))
		{
			printf("Error: sqrt(4)\n");
			printf("Expected: 2.000000000000000  Actual: %1.15f\n",x);
			return(1);
		}
	}
	x = sqrt(9.0);
	if (x != 3.000000000000000)
	{
		if (x < (3.000000000000000 - DPMARGIN) || x > (3.000000000000000 + DPMARGIN))
		{
			printf("Error: sqrt(9)\n");
			printf("Expected: 3.000000000000000  Actual: %1.15f\n",x);
			return(1);
		}
	}
	x = sqrt(16.0);
	if (x != 4.000000000000000)
	{
		if (x < (4.000000000000000 - DPMARGIN) || x > (4.000000000000000 + DPMARGIN))
		{
			printf("Error: sqrt(16)\n");
			printf("Expected: 4.000000000000000  Actual: %1.15f\n",x);
			return(1);
		}
	}
	x = sqrt(25.0);
	if (x != 5.000000000000000)
	{
		if (x < (5.000000000000000 - DPMARGIN) || x > (5.000000000000000 + DPMARGIN))
		{
			printf("Error: sqrt(25)\n");
			printf("Expected: 5.000000000000000  Actual: %1.15f\n",x);
			return(1);
		}
	}
	x = sqrt(36.0);
	if (x != 6.000000000000000)
	{
		if (x < (6.000000000000000 - DPMARGIN) || x > (6.000000000000000 + DPMARGIN))
		{
			printf("Error: sqrt(36)\n");
			printf("Expected: 6.000000000000000  Actual: %1.15f\n",x);
			return(1);
		}
	}
	x = sqrt(49.0);
	if (x != 7.000000000000000)
	{
		if (x < (7.000000000000000 - DPMARGIN) || x > (7.000000000000000 + DPMARGIN))
		{
			printf("Error: sqrt(49)\n");
			printf("Expected: 7.000000000000000  Actual: %1.15f\n",x);
			return(1);
		}
	}
	x = sqrt(64.0);
	if (x != 8.000000000000000)
	{
		if (x < (8.000000000000000 - DPMARGIN) || x > (8.000000000000000 + DPMARGIN))
		{
			printf("Error: sqrt(64)\n");
			printf("Expected: 8.000000000000000  Actual: %1.15f\n",x);
			return(1);
		}
	}
	x = sqrt(81.0);
	if (x != 9.000000000000000)
	{
		if (x < (9.000000000000000 - DPMARGIN) || x > (9.000000000000000 + DPMARGIN))
		{
			printf("Error: sqrt(81)\n");
			printf("Expected: 9.000000000000000  Actual: %1.15f\n",x);
			return(1);
		}
	}
	x = sqrt(100.0);
	if (x != 10.000000000000000)
	{
		if (x < (10.000000000000000 - DPMARGIN) || x > (10.000000000000000 + DPMARGIN))
		{
			printf("Error: sqrt(100)\n");
			printf("Expected: 10.000000000000000  Actual: %1.15f\n",x);
			return(1);
		}
	}

	if (load_test) break;
	if (!load_test && (dpfpa % 25) ==0) sleep(1);
	} /* end of for loop */

	return(0);
}

float_exp()
{
   if (atn) send_msg_to_atn (3,"Floating point exception interrupt, pass %d, errors %d.",pass,errors);
   throwup(-FP_EXCEPTION_ERROR,"%s: Floating point exception interrupt,%s errors %d.",Device,print_pass()? tmp_msg:"",errors);
}
finish()
{
   if (atn || verbose) {
      if (atn) 
	  send_msg_to_atn(INFO, "Stopped, pass %d, errors %d.", pass, errors);
      throwup(0, "%s: Stopped, pass %d, errors %d.", Device, pass, errors);
      if (atn) exit(0);
   }
   exit(20);
}

#ifdef sun3
int mywinitfp_()
/*
 *	Procedure to determine if a physical FPA and 68881 are present and
 *	set fp_state_sunfpa and fp_state_mc68881 accordingly.
	Also returns 1 if both present, 0 otherwise.
 */

{
int openfpa, mode81 ;
long fp_state_sunfpa;

	if (debug) printf("Looking for the fpa\n");
	if (minitfp_() != 1) fp_state_sunfpa = fp_absent ;
	openfpa = open("/dev/fpa", O_RDWR);
	if ((openfpa < 0) && (errno != EEXIST))
		{ /* openfpa < 0 */
		if (errno == EBUSY) printf("No FPA Context Available \n");
		fp_state_sunfpa = fp_absent ;
		if (debug) printf("Can not find FPA\n");
		} /* openfpa < 0 */
	else
		{ /* openfpa >= 0 */
		if (errno == ENXIO) printf("FPA already open \n");
		fp_state_sunfpa = fp_enabled;
		if (debug) printf("Found FPA\n");
		} /* openfpa >= 0 */
	return((fp_state_sunfpa == fp_enabled) ? 1 : 0) ;
}
#endif

#ifdef sun3
probe881()
{
	int 	val;
	int 	eeopen;
	int 	pointer;
	int 	seek_ok;
	int 	nbytes;
	long 	offset;
	unsigned char	ee_buffer[8];
	char	buf;
	int 	i;

	offset = 0x0000BC;
	pointer = L_SET;
	eeopen = open("/dev/eeprom",O_RDONLY);
	if (debug)
		printf("eeopen = %d\n",eeopen);
	if (eeopen < 0)
	{
		printf("Cannot open /dev/eeprom \n");
		exit(0);
	}
	if (debug) printf("passed open\n");
	for (i = 0; i < 14; i++)
	{
		seek_ok = lseek(eeopen,offset,pointer);
		if (debug) 
			printf("seek_ok = %x\n",seek_ok);
		if (seek_ok == -1)
		{
			printf("lseek failed\n");
			exit(0);
		}
		if (debug) printf("passed lseek\n");
		nbytes = 8;
		val = read(eeopen,ee_buffer,nbytes);
		if (debug)
			printf("val = %d\n",val);
		if (val == -1)
		{
			printf("read failed\n");
			exit(0);
		}
		if (debug)
		{
			printf("ee_buffer[0] = %x\n",ee_buffer[0]);
			printf("ee_buffer[1] = %x\n",ee_buffer[1]);
			printf("ee_buffer[2] = %x\n",ee_buffer[2]);
			printf("ee_buffer[3] = %x\n",ee_buffer[3]);
			printf("ee_buffer[4] = %x\n",ee_buffer[4]);
			printf("ee_buffer[5] = %x\n",ee_buffer[5]);
			printf("ee_buffer[6] = %x\n",ee_buffer[6]);
			printf("ee_buffer[7] = %x\n",ee_buffer[7]);
			printf("passed read\n");
		}
		if (ee_buffer[0] == 0xFF)
		{
			printf("Could not find configuration for MC68881 in EEPROM\n");
			return(-1);
		}
		if (ee_buffer[0] == 0x01)
		{
			buf = ee_buffer[2];
			if (debug) printf("buf = %x\n",buf);
			buf = buf & 0x01;
			if (debug) printf("buf = %x\n",buf);
			if (buf == 1)
				return (1);
			return(0);
		}
		offset = 0;
		pointer = L_INCR;
		if (i == 13)
		{
			printf(" System configuration of EEPROM is wrong.\n");
			printf(" Expected to find FF which represents the end of the configuration.\n");
			return(-1);
		}
	}
}
#endif


#ifdef ATN_VERSION
#include "atnrtns.c"    /* ATN routines */
#else
send_msg_to_atn()
{
printf("This is not the ATN version!\n");
}
#endif

throwup(where, fmt, a, b, c, d, e, f, g, h, i)
int     where;
char    *fmt;
u_long  a, b, c, d, e, f, g, h, i;
{
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
           if (atn)
              send_msg_to_atn (FATAL, "No log file environmental variable.");
           fprintf(stderr,
              "%s: No log file environmental variable.\n", TEST_NAME);
           fprintf(stderr,
              "%s: Was attempting to log the following message:\n", TEST_NAME);
           fprintf(stderr, "%s %s", fmt_msg, ctime(&clock));
           exit(NO_SD_LOG_DIR);
        }
     }
     if ((logfd = open(mktemp(logfile),O_WRONLY|O_CREAT|O_APPEND ,0644)) <0){
        perror(perror_msg);
        if (atn)
           send_msg_to_atn (FATAL, "Couldn't open logfile '%s'.", logfile);
        fprintf(stderr, "%s: Couldn't open logfile '%s'.\n",TEST_NAME,logfile);
        fprintf(stderr, "%s: Was attempting to log the following message:\n",
           TEST_NAME);
        fprintf(stderr, "%s %s", fmt_msg, ctime(&clock));
        exit(NO_OPEN_LOG);
     }
     else dup2(logfd, 2);               /* set logfile as stderr */
  }
  fprintf(stderr, "%s %s", fmt_msg, ctime(&clock));
  printf("%s %s", fmt_msg, ctime(&clock));

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

