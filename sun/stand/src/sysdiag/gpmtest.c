/*
	gpmtest.c: executes a series of microcode test on the Graphics
			Processor and Graphics Buffer boards

	Each test starts by the host writing data to shared memory.  While
	the host reads this data, the GP passes the data through its
	hardware and finally back into shared memory.  The host then
	compares this data with the written data and reports errors.

	A series of tests exists, each touching different parts of the
	GP and GB.  These tests run in sequence;  the last test touches
	nearly all of the GP and GB boards.

	Jan 1985
		rev 28 Mar 1985 - made pp prom test an option
				  and added exit(6) and exit(7)
				  and added creation date output message
	John Fetter

	May 1985
		rev 10 May 1985 - Modified to run under sysdiag
				  as gpmtest.
	Frank Jones

	June 1985
		rev 3 June 1985 - Removed .o off the micro-code files.
	Frank Jones

	Possible improvements:
		add microstore write/read tests
		on error exit, reset gp
*/

#include "gp1.h"
#include <sys/types.h>
#include <stdio.h>
#include <signal.h>
#include <sys/file.h>
#include <strings.h>
#include <sys/mman.h>

#define FALSE                   0
#define TRUE                    ~FALSE
#define INFO                    0
#define WARNING                 1
#define FATAL                   2
#define ERROR                   3
#define NO_SD_LOG_DIR           1
#define NO_OPEN_LOG             2
#define VME24_NOT_OPEN          3
#define NO_MALLOC          	4
#define NO_MAP          	5
#define UFILE_NOT_OPEN          6
#define TEST1_VERIFY_ERROR      7
#define TEST1_PATH_ERROR      	8
#define TEST2_VERIFY_ERROR      9
#define TEST2_PATH_ERROR      	10
#define TEST3_VERIFY_ERROR      11
#define TEST3_PATH_ERROR      	12
#define TEST4_VERIFY_ERROR      13
#define TEST4_HUNG_ERROR     	14
#define TEST4_PATH_ERROR      	15
#define CHECKSUM_ERROR      	16
#define STATUS_ERROR      	17
#define TEST5_VERIFY_ERROR      18
#define TEST5_HUNG_ERROR     	19
#define TEST5_PATH_ERROR      	21
#define TEST6_VERIFY_ERROR      22
#define TEST6_HUNG_ERROR     	23
#define TEST6_PATH_ERROR      	24
#define INT1_ERROR      	25
#define TEST7_VERIFY_ERROR      26
#define TEST7_HUNG_ERROR     	27
#define INT2_ERROR      	28
#define TEST7_PATH_ERROR      	29
#define END_ERROR               30
#define USAGE_ERROR             99

#define TEST_NAME               "gpmtest"
#define LOGFILE_NAME            "log.gpmtest.XXXXXX"

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

static char     sccsid[] = "@(#)gpmtest.c 1.1 9/25/86 Copyright 1985 Sun Micro";

char file_name_buffer[30];
char *file_name = file_name_buffer;

#define BLOCK_SIZE_1	8*1024	/* half of shared memory in 16-bit words*/
#define BLOCK_SIZE_2	4*1024	/* quarter of shared memory in 16-bit words*/
#define GP_TESTS	20
#define GB_TESTS	6
#define PASS_COUNT	10
#define PROM_SIZE	16*1024
#define	HUNG_STATE	10
#define PPPROM_TEST	25

short *gp1_base;
short *gp1_shmem;

static int gp1_fd;
static caddr_t allocp;

char sysdiag_directory[50];
char *SD = sysdiag_directory; 
int test;

main(argc, argv)
int argc;
char **argv;
{
   int arrcount, match;
   extern finish();

   short c;
   int ppprom_flag;
   int passes;
   int start_test, num_of_tests;
   char state[256];

   start_test = 1;
   num_of_tests = GP_TESTS;
   ppprom_flag = 0;

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
                        if (strcmp(argv[arrcount], "gb") == 0) {
                                match = TRUE;
                                num_of_tests += GB_TESTS;
                        }
                        if (strcmp(argv[arrcount], "ppprom") == 0) {
                                match = TRUE;
                                ppprom_flag = TRUE;
                        }
                        if (!match) {
                           printf("Usage: %s [v] [sd/atn] [gb] [re] [lt] [dd] [d] [e{1-%d}]\n", TEST_NAME, END_ERROR - 1);
                           exit(USAGE_ERROR);
                        }
      }  

   if (verify) {               /* verify mode */
        if (verbose) printf("%s: Verify mode.\n", TEST_NAME);
        exit(0);
   }
   if (atn || verbose) {
       if (atn) send_msg_to_atn(INFO, "Started testing graphics processor%s.",
				num_of_tests == GP_TESTS? "":" and buffer");
       throwup(0, "%s: Started testing graphics processor%s.",
		   TEST_NAME, num_of_tests == GP_TESTS? "":" and buffer");
   }

   gp1_open();

   initstate(1000, state, 256);
   if (debug || load_test) passes = 1;
   else passes = PASS_COUNT;

   while (atn || pass == 0) {
      pass++;

      for(test = start_test; test <= num_of_tests; test++) {
         if(test != PPPROM_TEST | ppprom_flag == 1)
	    runtest(test, passes);
         else {
            if (!(exec_by_sysdiag)) { 
               printf("TEST %2d: ", PPPROM_TEST);
               printf("you did not enable the pp prom test");
               printf("\n\n");
            }
         }
      }
      if (load_test) break;
      if (!verbose) sleep (5);
      if (atn || verbose)
         printf("%s: pass %d, errors %d.\n", TEST_NAME, pass, errors);
   }
   if (atn || verbose)
       throwup(0, "%s: Stopped, pass %d, errors %d.", TEST_NAME, pass, errors);

   gp1_reset();
}

/* function to load selected microcode and run a test*/

runtest(test,passes)
int test, passes;
	{
	int errcount;
	gp1_reset();

   switch (test) {            /* Viewing Processor only tests */

	case 1:
		gp1_load("gpmtest.shmem.2p");
		if (verbose) printf("TEST %2d: shmem->shmem (byte)\n",test);
		break;
	case 2: 
		gp1_load("gpmtest.shmem.2p");
		if (verbose) printf("TEST %2d: shmem->shmem (word)\n",test);
		break;
	case 3:
		gp1_load("gpmtest.shmem.2p");
		if (verbose) printf("TEST %2d: shmem->shmem (integer)\n",test);
		break;
	case 4:
		gp1_load("gpmtest.vp_29116.2p");
		if (verbose) printf("TEST %2d: shmem->VP_29116->shmem\n",test);
		break;
	case 5:
		gp1_load("gpmtest.fprega.2p");
		if (verbose) printf("TEST %2d: shmem->fpreg A->shmem\n",test);
		break;
	case 6:
		gp1_load("gpmtest.fpregb.2p");
		if (verbose) printf("TEST %2d: shmem->fpreg B->shmem\n",test);
		break;
	case 7:
		gp1_load("gpmtest.fpalu.2p");
		if (verbose) printf
		   ("TEST %2d: shmem->fpreg->Weitek ALU->fpreg->shmem\n",test);
		break;
	case 8:
		gp1_load("gpmtest.fpmult.2p");
		if (verbose) printf
		  ("TEST %2d: shmem->fpreg->Weitek ALU & MULT->fpreg->shmem\n",
		    test);
		break;
	case 9:
		gp1_load("gpmtest.vpprom.2p");
		if (verbose) printf("TEST %2d: vpprom->shmem\n",test);
		break;
/* Viewing Processor and Painting Processor tests */
	case 10: 
		gp1_load("gpmtest.fifo_vme.2p");
		if (verbose)
		   printf("TEST %2d: shmem->fifo->VME (word)->shmem\n",test);
		break;
	case 11: 
		gp1_load("gpmtest.fifo_vme_dec.2p");
		if (verbose) 
		 printf("TEST %2d: shmem->fifo->VME (dec count)->shmem\n",test);
		break;
	case 12: 
		gp1_load("gpmtest.vme_byte.2p");
		if (verbose) 
		   printf("TEST %2d: shmem->fifo->VME (byte)->shmem\n",test);
		break;
	case 13: 
		gp1_load("gpmtest.int_flag.2p");
		if (verbose) printf
		  ("TEST %2d: shmem->fifo->VME->shmem (test int flag)\n",test);
		break;
	case 14: 
		gp1_load("gpmtest.vme_read.2p");
		if (verbose) 
		   printf("TEST %2d: shmem->VME (word)->fifo->shmem\n",test);
		break;
	case 15: 
		gp1_load("gpmtest.vme_read_byte.2p");
		if (verbose) 
		   printf("TEST %2d: shmem->VME (byte)->fifo->shmem\n",test);
		break;
	case 16: 
		gp1_load("gpmtest.pp_29116.2p");
		if (verbose) 
		   printf("TEST %2d: shmem->fifo->PP_29116->VME->shmem\n",test);
		break;
	case 17: 
		gp1_load("gpmtest.scrpad.2p");
		if (verbose)
		   printf("TEST %2d: shmem->fifo->scrpad->VME->shmem\n",test);
		break;
	case 18: 
		gp1_load("gpmtest.ppfifo.2p");
		if (verbose)
		   printf("TEST %2d: shmem->fifo->scrpad->fifo->shmem\n",test);
		break;
	case 19: 
		gp1_load("gpmtest.fifo_vme.2p");
		if (verbose) printf("TEST %2d: status flags\n",test);
		break;
	case 20: 
		gp1_load("gpmtest.allbutgb.2p");
		if (verbose) printf("TEST %2d: shmem->VP_29116->fpreg->fifo->PP_29116->scrpad->VME->shmem\n",test);
		break;
/* Graphics Buffer tests */
	case 21: 
		gp1_load("gpmtest.xoperand.2p");
		if (verbose) printf
		   ("TEST %2d: shmem->fifo->X_operand->VME->shmem\n",test);
		break;
	case 22: 
		gp1_load("gpmtest.yoperand.2p");
		if (verbose) printf
		   ("TEST %2d: shmem->fifo->Y_operand->VME->shmem\n",test);
		break;
	case 23: 
		gp1_load("gpmtest.gbnorm.2p");
		if (verbose) printf
		("TEST %2d: shmem->fifo->g_buffer (normal)->VME->shmem\n",test);
		break;
	case 24: 
		gp1_load("gpmtest.gbrmw.2p");
		if (verbose) printf
		   ("TEST %2d: shmem->fifo->g_buffer (rmw)->VME->shmem\n",test);
		break;
	case 25: 
		gp1_load("gpmtest.ppprom.2p");
		if (verbose) 
 		   printf("TEST %2d: shmem->pprom->VME->shmem\n",test);
		break;
	case 26: 
		gp1_load("gpmtest.all.2p");
		if (verbose) printf("TEST %2d: shmem->VP_29116->fpreg->fifo->PP_29116->scrpad->int_mult->gb->VME->shmem\n",test);
		break;
	}
	gp1_vp_start(0);
	gp1_pp_start(0);
	switch (test) {
/* Viewing Processor only tests */
		case 1:	errcount = shmem_test16k_byte(passes);
			break;
		case 2:	errcount = shmem_test16k_word(passes);
			break;
		case 3:	errcount = shmem_test16k_int(passes);
			break;
		case 4: errcount = shmem_test8k(passes);
			break;
		case 5: errcount = shmem_test8k(passes);
			break;
		case 6: errcount = shmem_test8k(passes);
			break;
		case 7: errcount = shmem_test8k(passes);
			break;
		case 8: errcount = shmem_test8k(passes);
			break;
		case 9: errcount = test_prom();
			break;
/* Viewing Processor and Painting Processor tests */
		case 10: errcount = shmem_test8k(passes);
			break;
		case 11: errcount = shmem_test8k(passes);
			break;
		case 12: errcount = shmem_test8k(passes);
			break;
		case 13: errcount = shmem_iflag(passes);
			break;
		case 14: errcount = shmem_test8k(passes);
			break;
		case 15: errcount = shmem_test8k(passes);
			break;
		case 16: errcount = shmem_test8k(passes);
			break;
		case 17: errcount = shmem_test8k(passes);
			break;
		case 18: errcount = shmem_test8k(passes);
			break;
		case 19: errcount = test_sflag();
			break;
		case 20: errcount = shmem_test8k(passes);
			break;
/* Graphics Buffer tests */
		case 21: errcount = shmem_test8k(passes);
			break;
		case 22: errcount = shmem_test8k(passes);
			break;
		case 23: errcount = shmem_testgb();
			break;
		case 24: errcount = shmem_testgb();
			break;
		case 25: errcount = test_prom();
			break;
		case 26: errcount = shmem_test8k(passes);
			break;
		}
	return(errcount);
	}

/* function to test shared memory with byte accesses */
shmem_test16k_byte(passes)
int passes;
{
   int pass, error1 = 0, error2 = 0, i;
   unsigned char readback, data[2*BLOCK_SIZE_1];
   unsigned char *ptr;

   for(pass=1; pass<=passes; pass++) {
					/* Write data to shared memory */
      ptr = (unsigned char *) &gp1_shmem[0x0];
      for(i = 0; i < 2*BLOCK_SIZE_1; i++) {
         data[i] = (char) random();
         *ptr++ = data[i];
      }
      if (simulate_error == TEST1_VERIFY_ERROR || 
	  simulate_error == TEST1_PATH_ERROR) {
          ptr = (unsigned char *) &gp1_shmem[0x10];
	  readback = *ptr;
	  *ptr = readback + 1;
      }
					/* Readback the just-written data */
      ptr = (unsigned char *) &gp1_shmem[0x0];
      for(i = 0; i < 2*BLOCK_SIZE_1; i++) {
         readback = *ptr++;
         if (readback != data[i] && simulate_error != TEST1_PATH_ERROR) {
            error1++;
            errors++;
            if (atn) send_msg_to_atn (ERROR, "Data compare error on memory verify, byte address = 0x%x, exp = 0x%x, actual = 0x%x, test %d, pass %d, errors %d.", i, data[i], readback, test, pass, errors);
            throwup(-TEST1_VERIFY_ERROR, "ERROR: %s, data compare error on memory verify, byte address = 0x%x, exp = 0x%x, actual = 0x%x, test %d,%s errors %d.", TEST_NAME, i, data[i], readback, test, print_pass()? tmp_msg:"", errors);
         }
      }
				/* Readback the data passed through the GP */
      ptr = (unsigned char *) &gp1_shmem[BLOCK_SIZE_1];
      for(i = 0; i < 2*BLOCK_SIZE_1; i++) {
         readback = *ptr++;
         if (readback != data[i]) {
	    error2++;
            errors++;
            if (atn) send_msg_to_atn (ERROR, "Data compare error, byte address = 0x%x, exp = 0x%x, actual = 0x%x, test %d, pass %d, errors %d.", i+2*BLOCK_SIZE_1, data[i], readback, test, pass, errors);
            throwup(-TEST1_PATH_ERROR, "ERROR: %s, data compare error, byte address = 0x%x, exp = 0x%x, actual = 0x%x, test %d,%s errors %d.", TEST_NAME, i+2*BLOCK_SIZE_1, data[i], readback, test, print_pass()? tmp_msg:"", errors);
         }
      }
      if (verbose) printf("         Pass %2d complete\n",pass);
   }
   return(error1+error2);
}

/* function to test shared memory with word (16-bit) accesses */
shmem_test16k_word(passes)
int passes;
{
   int pass, error1 = 0, error2 = 0, i;
   unsigned short readback, data[BLOCK_SIZE_1];
   unsigned short *ptr;

   for(pass=1; pass<=passes; pass++) {
					/* Write data to shared memory */
      ptr = (unsigned short *) &gp1_shmem[0x0];
      for(i = 0; i < BLOCK_SIZE_1; i++) {
         data[i] = (short) random();
         *ptr++ = data[i];
      }
      if (simulate_error == TEST2_VERIFY_ERROR ||
          simulate_error == TEST2_PATH_ERROR) {
          ptr = (unsigned short *) &gp1_shmem[0x10];
          readback = *ptr;
          *ptr = readback + 1;
      }
					/* Readback the just-written data */
      ptr = (unsigned short *) &gp1_shmem[0x0];
      for(i = 0; i < BLOCK_SIZE_1; i++) {
         readback = *ptr++;
         if (readback != data[i] && simulate_error != TEST2_PATH_ERROR) {
            error1++;
            errors++;
            if (atn) send_msg_to_atn (ERROR, "Data compare error on memory verify, word address = 0x%x, exp = 0x%x, actual = 0x%x, test %d, pass %d, errors %d.", i, data[i], readback, test, pass, errors);
            throwup(-TEST2_VERIFY_ERROR, "ERROR: %s, data compare error on memory verify, word address = 0x%x, exp = 0x%x, actual = 0x%x, test %d,%s errors %d.", TEST_NAME, i, data[i], readback, test, print_pass()? tmp_msg:"", errors);
         }
      }
				/* Readback the data passed through the GP */
      ptr = (unsigned short *) &gp1_shmem[BLOCK_SIZE_1];
      for(i = 0; i < BLOCK_SIZE_1; i++) {
         readback = *ptr++;
         if (readback != data[i]) {
            error2++;                    
            errors++;                    
            if (atn) send_msg_to_atn (ERROR, "Data compare error, word address = 0x%x, exp = 0x%x, actual = 0x%x, test %d, pass %d, errors %d.", i+BLOCK_SIZE_1, data[i], readback, test, pass, errors);
            throwup(-TEST2_PATH_ERROR, "ERROR: %s, data compare error, word address = 0x%x, exp = 0x%x, actual = 0x%x, test %d,%s errors %d.", TEST_NAME, i+BLOCK_SIZE_1, data[i], readback, test, print_pass()? tmp_msg:"", errors);
         } 
      }
      if (verbose) printf("         Pass %2d complete\n",pass);
   }
   return(error1+error2);
}

/* function to test shared memory with integer (32-bit) accesses */
shmem_test16k_int(passes)
int passes;
{
   int pass, error1 = 0, error2 = 0, i;
   unsigned int readback, data[2*BLOCK_SIZE_1];
   unsigned int *ptr;

   for(pass=1; pass<=passes; pass++) {
					/* Write data to shared memory */
      ptr = (unsigned int *) &gp1_shmem[0x0];
      for(i = 0; i < (BLOCK_SIZE_1 / 2); i++) {
         data[i] = (int) random();
         *ptr++ = data[i];
      }
      if (simulate_error == TEST3_VERIFY_ERROR ||
          simulate_error == TEST3_PATH_ERROR) {
          ptr = (unsigned int *) &gp1_shmem[0x10];
          readback = *ptr;
          *ptr = readback + 1;
      }
					/* Readback the just-written data */
      ptr = (unsigned int *) &gp1_shmem[0x0];
      for(i = 0; i < (BLOCK_SIZE_1 / 2); i++) {
         readback = *ptr++;
         if (readback != data[i] && simulate_error != TEST3_PATH_ERROR) {
            error1++;
            errors++;
            if (atn) send_msg_to_atn (ERROR, "Data compare error on memory verify, integer address = 0x%x, exp = 0x%x, actual = 0x%x, test %d, pass %d, errors %d.", i, data[i], readback, test, pass, errors);
            throwup(-TEST3_VERIFY_ERROR, "ERROR: %s, data compare error on memory verify, integer address = 0x%x, exp = 0x%x, actual = 0x%x, test %d,%s errors %d.", TEST_NAME, i, data[i], readback, test, print_pass()? tmp_msg:"", errors);
         }
      }
				/* Readback the data passed through the GP */
      ptr = (unsigned int *) &gp1_shmem[BLOCK_SIZE_1];
      for(i = 0; i < (BLOCK_SIZE_1 / 2); i++) {
         readback = *ptr++;
         if (readback != data[i]) {
            error2++; 
            errors++; 
            if (atn) send_msg_to_atn (ERROR, "Data compare error, integer address = 0x%x, exp = 0x%x, actual = 0x%x, test %d, pass %d, errors %d.", i+(BLOCK_SIZE_1 / 2), data[i], readback, test, pass, errors);
            throwup(-TEST3_PATH_ERROR, "ERROR: %s, data compare error, integer address = 0x%x, exp = 0x%x, actual = 0x%x, test %d,%s errors %d.", TEST_NAME, i+(BLOCK_SIZE_1 / 2), data[i], readback, test, print_pass()? tmp_msg:"", errors);
         }
      }
      if (verbose) printf("         Pass %2d complete\n",pass);
   }
   return(error1+error2);
}

/* function to test shmem -> ... -> shmem microcode */
shmem_test8k(passes)
int passes;
{
   int pass, error1 = 0, error2 = 0, i, hung_count = 0;
   unsigned short readback, data[BLOCK_SIZE_2];
   short *ptr;

   for(pass=1; pass<=passes; pass++) {
					/* write data to shared memory */
      ptr = (short *) &gp1_shmem[0x1000];
      for(i=0; i<BLOCK_SIZE_2; i++) {
         data[i] = (short) random();
         *ptr++ = data[i];
      }
      if (simulate_error == TEST4_VERIFY_ERROR ||
          simulate_error == TEST4_PATH_ERROR) {
          ptr = (short *) &gp1_shmem[0x1010];
          readback = *ptr;
          *ptr = readback + 1;
      }
      gp1_shmem[0] = 0x800;		/* set semaphore */
      hung_count = 0;
      do {				 /* readback the just-written data*/
         ptr = (short *) &gp1_shmem[0x1000];
         for(i=0; i<BLOCK_SIZE_2; i++) {
            readback = *ptr++;
            if (readback != data[i] && simulate_error != TEST4_PATH_ERROR) {
               error1++;
               errors++;
               if (atn) send_msg_to_atn (ERROR, "Data compare error on memory verify, word address = 0x%x, exp = 0x%x, actual = 0x%x, test %d, pass %d, errors %d.", i+0x1000, data[i], readback, test, pass, errors);
               throwup(-TEST4_VERIFY_ERROR, "ERROR: %s, data compare error on memory verify, word address = 0x%x, exp = 0x%x, actual = 0x%x, test %d,%s errors %d.", TEST_NAME, i+0x1000, data[i], readback, test, print_pass()? tmp_msg:"", errors);
            }
         }
         if (++hung_count == HUNG_STATE) {
            errors++;
            if (atn) send_msg_to_atn (ERROR, "The graphics processor hung, test %d, pass %d, errors %d.", test, pass, errors);
            throwup(-TEST4_HUNG_ERROR, "ERROR: %s, the graphics processor hung, test %d,%s errors %d.", TEST_NAME, test, print_pass()? tmp_msg:"", errors);
	 }
      }						 /* semaphore reset ?? */
      while (gp1_shmem[0] != 0 || simulate_error == TEST4_HUNG_ERROR);
				/* readback the data passed through the GP*/
      ptr = (short *) &gp1_shmem[0x2000];
      for(i=0; i<BLOCK_SIZE_2; i++) {
         readback = *ptr++;
         if (readback != data[i]) {
            error2++;        
            errors++;        
            if (atn) send_msg_to_atn (ERROR, "Data compare error, word address = 0x%x, exp = 0x%x, actual = 0x%x, test %d, pass %d, errors %d.", i+0x2000, data[i], readback, test, pass, errors);
            throwup(-TEST4_PATH_ERROR, "ERROR: %s, data compare error, word address = 0x%x, exp = 0x%x, actual = 0x%x, test %d,%s errors %d.", TEST_NAME, i+0x2000, data[i], readback, test, print_pass()? tmp_msg:"", errors);
         }
      }
      if (verbose) printf("         Pass %2d complete\n",pass);
   }
   return(error1+error2);
}

/* function to read shmem contents (prom -> shmem) and test checksum */
test_prom()
{
   int i,checksum = 0;
   unsigned short readback;
   int error = 0;
		/* ensure time for GP to move prom contents to shared memory */
   sleep(2);
				/* compute and check checksum */
   for(i=0; i<PROM_SIZE-1; i++) {
      checksum += gp1_shmem[i];
      checksum &= 0xffff;
   }
   readback = gp1_shmem[PROM_SIZE-1];
   if (simulate_error == CHECKSUM_ERROR) readback++;
   if (((unsigned short) checksum) == readback) {
      if (verbose) printf("         CHECKSUM MATCH; value= %x\n",checksum);
   }
   else {
      error++;
      errors++;
      if (atn) send_msg_to_atn (ERROR, "Checksum incorrect, checksum exp = 0x%x, checksum actual = 0x%x, test %d, pass %d, errors %d.", checksum, readback, test, pass, errors);
      throwup(-CHECKSUM_ERROR, "ERROR: %s, checksum incorrect, checksum exp = 0x%x, checksum actual = 0x%x, test %d,%s errors %d.", TEST_NAME, checksum, readback, test, print_pass()? tmp_msg:"", errors);
   }
   return(error);
}

/* function to test vme-readable status flags */
test_sflag()
{
   int pass, passes = 32, error1 = 0, error2 = 0, error3 = 0;
   int i, hung_count = 0;
   unsigned short readback, data[BLOCK_SIZE_2];
   short *ptr;
   unsigned short sflag4, sflag8;

   if (debug || load_test) passes = 1;

   for(pass=1; pass<=passes; pass++) {

/* Mask off the last four bits of pass number and (ones) complement them.
   This should equal the state of the Viewing Processor and Painting
   Processor status flags */

      sflag4 = 0xf - (pass & 0xf);
      sflag8 = (sflag4<<4) + sflag4;
      readback = gp1_base[GP1_STATUS_REG] & 0xff;	
      if (simulate_error == STATUS_ERROR) readback++;
      if(sflag8 != readback) {
         error3++;
         errors++;
         if (atn) send_msg_to_atn (ERROR, "Status incorrect, exp = 0x%x, actual = 0x%x, test %d, pass %d, errors %d.", sflag8, readback, test, pass, errors);
         throwup(-STATUS_ERROR, "ERROR: %s, status incorrect, exp = 0x%x, actual = 0x%x, test %d,%s errors %d.", TEST_NAME, sflag8, readback, test, print_pass()? tmp_msg:"", errors);
      }
					/* write data to shared memory */
      ptr = (short *) &gp1_shmem[0x1000];
      for(i=0; i<BLOCK_SIZE_2; i++) {
         data[i] = (short) random();
         *ptr++ = data[i];
      }
      if (simulate_error == TEST5_VERIFY_ERROR ||
          simulate_error == TEST5_PATH_ERROR) {
          ptr = (short *) &gp1_shmem[0x1010];
          readback = *ptr;
          *ptr = readback + 1;
      }
      gp1_shmem[0] = 0x800;		/* set semaphore */
      hung_count = 0;

     /* readback the just-written data until the GP is done moving the data */
      do {
         ptr = (short *) &gp1_shmem[0x1000];
         for(i=0; i<BLOCK_SIZE_2; i++) {
            readback = *ptr++;
            if (readback != data[i] && simulate_error != TEST5_PATH_ERROR) {
               error1++;
               errors++;
               if (atn) send_msg_to_atn (ERROR, "Data compare error on memory verify, word address = 0x%x, exp = 0x%x, actual = 0x%x, test %d, pass %d, errors %d.", i+0x1000, data[i], readback, test, pass, errors);
               throwup(-TEST5_VERIFY_ERROR, "ERROR: %s, data compare error on memory verify, word address = 0x%x, exp = 0x%x, actual = 0x%x, test %d,%s errors %d.", TEST_NAME, i+0x1000, data[i], readback, test, print_pass()? tmp_msg:"", errors);
            }
         }
         if (++hung_count == HUNG_STATE) {
            errors++;
            if (atn) send_msg_to_atn (ERROR, "The graphics processor hung, test %d, pass %d, errors %d.", test, pass, errors);
            throwup(-TEST5_HUNG_ERROR, "ERROR: %s, the graphics processor hung, test %d,%s errors %d.", TEST_NAME, test, print_pass()? tmp_msg:"", errors);
         }
      }						/* semaphore reset ?? */
      while (gp1_shmem[0] != 0 || simulate_error == TEST5_HUNG_ERROR);
		/* readback the data passed through the GP by the microcode*/
      ptr = (short *) &gp1_shmem[0x2000];
      for(i=0; i<BLOCK_SIZE_2; i++) {
         readback = *ptr++;
         if (readback != data[i]) {
            error2++;
            errors++;
            if (atn) send_msg_to_atn (ERROR, "Data compare error, word address = 0x%x, exp = 0x%x, actual = 0x%x, test %d, pass %d, errors %d.", i+0x2000, data[i], readback, test, pass, errors);
            throwup(-TEST5_PATH_ERROR, "ERROR: %s, data compare error, word address = 0x%x, exp = 0x%x, actual = 0x%x, test %d,%s errors %d.", TEST_NAME, i+0x2000, data[i], readback, test, print_pass()? tmp_msg:"", errors);
         }
      }
      if (verbose) if ((pass % 16) == 0 || debug || load_test) 
		printf("         Pass %2d complete\n",pass);
   }
   return(error1+error2+error3);
}

/* function to test shmem -> ... -> shmem with graphics buffer microcode */
shmem_testgb()
{
   int pass, passes = 256, error1 = 0, error2 = 0, i, hung_count = 0;
   int gbaddr;
   unsigned short readback, data[BLOCK_SIZE_2];
   short *ptr;
   int *ptr_gbaddr;

   if (debug || load_test) passes = 1;

   ptr_gbaddr = (int *) &gp1_shmem[2];
   for(pass=1; pass<=passes; pass++) {
				/* 256 = 1 Megaword divided by 4k words */
				/* write data to shared memory */
      ptr = (short *) &gp1_shmem[0x1000];
      for(i=0; i<BLOCK_SIZE_2; i++) {
         data[i] = (short) random();
         *ptr++ = data[i];
      }
      gbaddr = 0x1000 * (pass-1);
      *ptr_gbaddr = gbaddr;

      if (simulate_error == TEST6_VERIFY_ERROR ||
          simulate_error == TEST6_PATH_ERROR) {
          ptr = (short *) &gp1_shmem[0x1010];
          readback = *ptr;   
          *ptr = readback + 1;
      }
      gp1_shmem[0] = 0x800;		/* set semaphore */
      hung_count = 0;

      /* readback the just-written data until the GP is done moving the data */
      do {
         ptr = (short *) &gp1_shmem[0x1000];
         for(i=0; i<BLOCK_SIZE_2; i++) {
            readback = *ptr++;
            if (readback != data[i] && simulate_error != TEST6_PATH_ERROR) {
               error1++;
               errors++;
               if (atn) send_msg_to_atn (ERROR, "Data compare error on memory verify, word address = 0x%x, exp = 0x%x, actual = 0x%x, test %d, pass %d, errors %d.", i+0x1000, data[i], readback, test, pass, errors);
               throwup(-TEST6_VERIFY_ERROR, "ERROR: %s, data compare error on memory verify, word address = 0x%x, exp = 0x%x, actual = 0x%x, test %d,%s errors %d.", TEST_NAME, i+0x1000, data[i], readback, test, print_pass()? tmp_msg:"", errors);
            }
         }
         if (++hung_count == HUNG_STATE) {
            errors++;
            if (atn) send_msg_to_atn (ERROR, "The graphics processor hung, test %d, pass %d, errors %d.", test, pass, errors);
            throwup(-TEST6_HUNG_ERROR, "ERROR: %s, the graphics processor hung, test %d,%s errors %d.", TEST_NAME, test, print_pass()? tmp_msg:"", errors);
         }
      }						/* semaphore reset ?? */
      while (gp1_shmem[0] != 0 || simulate_error == TEST6_HUNG_ERROR);
		/* readback the data passed through the GP by the microcode*/
      ptr = (short *) &gp1_shmem[0x2000];
      for(i=0; i<BLOCK_SIZE_2; i++) {
         readback = *ptr++;
         if (readback != data[i]) {
            error2++;
            errors++;
            if (atn) send_msg_to_atn (ERROR, "Data compare error, word address = 0x%x, gb address = 0x%x, exp = 0x%x, actual = 0x%x, test %d, pass %d, errors %d.", i+0x2000, i+gbaddr, data[i], readback, test, pass, errors);
            throwup(-TEST6_PATH_ERROR, "ERROR: %s, data compare error, word address = 0x%x, gb address = 0x%x, exp = 0x%x, actual = 0x%x, test %d,%s errors %d.", TEST_NAME, i+0x2000, i+gbaddr, data[i], readback, test, print_pass()? tmp_msg:"", errors);
         }
      }
      if (verbose) if ((pass % 32) == 0 || debug || load_test) 
            printf("         Pass %2d complete\n",pass);
   }
   return(error1+error2);
}

/* function to test shmem -> ... -> shmem microcode using interrupt flag
   as reset semaphore instead of shared memory location 0 */
shmem_iflag(passes)
int passes;
{
   int pass, error1 = 0, error2 = 0, error3 = 0, i, hung_count = 0;
   unsigned short readback, data[BLOCK_SIZE_2];
   short *ptr;
				/* ensure interrupt enable is turned off */
   if (simulate_error == INT1_ERROR) gp1_base[GP1_CONTROL_REG] = 0x0100;
   else gp1_base[GP1_CONTROL_REG] = 0x0200;
   if ((gp1_base[GP1_STATUS_REG] & 0x4000) != 0) {
      errors++;
      if (atn) send_msg_to_atn (ERROR, "Interrupt cannot be disabled, test %d, pass %d, errors %d.", test, pass, errors);
      throwup(-INT1_ERROR, "ERROR: %s, interrupt cannot be disabled, test %d,%s errors %d.", TEST_NAME, test, print_pass()? tmp_msg:"", errors);
   }
   for(pass=1; pass<=passes; pass++) {
				/* write data to shared memory */
      ptr = (short *) &gp1_shmem[0x1000];
      for(i=0; i<BLOCK_SIZE_2; i++) {
         data[i] = (short) random();
         *ptr++ = data[i];
      }
      if (simulate_error == TEST7_VERIFY_ERROR ||
          simulate_error == TEST7_PATH_ERROR) {
          ptr = (short *) &gp1_shmem[0x1010];
          readback = *ptr;
          *ptr = readback + 1;
      }
      gp1_shmem[0] = 0;		/* reset semaphore for VP */
      gp1_shmem[0] = 0x800;		/* set semaphore */
      hung_count = 0;

     /* readback the just-written data until the GP is done moving the data */
      do {
         ptr = (short *) &gp1_shmem[0x1000];
         for(i=0; i<BLOCK_SIZE_2; i++) {
            readback = *ptr++;
            if (readback != data[i] && simulate_error != TEST7_PATH_ERROR) {
               error1++;
               errors++;
               if (atn) send_msg_to_atn (ERROR, "Data compare error on memory verify, word address = 0x%x, exp = 0x%x, actual = 0x%x, test %d, pass %d, errors %d.", i+0x1000, data[i], readback, test, pass, errors);
               throwup(-TEST7_VERIFY_ERROR, "ERROR: %s, data compare error on memory verify, word address = 0x%x, exp = 0x%x, actual = 0x%x, test %d,%s errors %d.", TEST_NAME, i+0x1000, data[i], readback, test, print_pass()? tmp_msg:"", errors);
            }
         }
         if (++hung_count == HUNG_STATE) {
            errors++;
            if (atn) send_msg_to_atn (ERROR, "The graphics processor hung, test %d, pass %d, errors %d.", test, pass, errors);
            throwup(-TEST7_HUNG_ERROR, "ERROR: %s, the graphics processor hung, test %d,%s errors %d.", TEST_NAME, test, print_pass()? tmp_msg:"", errors);
         }
      }
					/* semaphore (int flag) reset? ? */
      while ((gp1_base[GP1_STATUS_REG] & 0x8000) == 0  ||
				simulate_error == TEST7_HUNG_ERROR);
				/* reset interrupt flag (the PP semaphore) */
      if (simulate_error == INT2_ERROR) gp1_base[GP1_CONTROL_REG] = 0x0000;
      else gp1_base[GP1_CONTROL_REG] = 0x8000;
					/* did the interrupt flag reset */
      if ((gp1_base[GP1_STATUS_REG] & 0x8000) != 0) {
         error3++;
         errors++;
         if (atn) send_msg_to_atn (ERROR, "Interrupt flag did not reset, test %d, pass %d, errors %d.", test, pass, errors);
         throwup(-INT2_ERROR, "ERROR: %s, interrupt flag did not reset, test %d,%s errors %d.", TEST_NAME, test, print_pass()? tmp_msg:"", errors);
      }
		/* readback the data passed through the GP by the microcode*/
      ptr = (short *) &gp1_shmem[0x2000];
      for(i=0; i<BLOCK_SIZE_2; i++) {
         readback = *ptr++;
         if (readback != data[i]) {
            error2++;
            errors++;
            if (atn) send_msg_to_atn (ERROR, "Data compare error, word address = 0x%x, exp = 0x%x, actual = 0x%x, test %d, pass %d, errors %d.", i+0x2000, data[i], readback, test, pass, errors);
            throwup(-TEST7_PATH_ERROR, "ERROR: %s, data compare error, word address = 0x%x, exp = 0x%x, actual = 0x%x, test %d,%s errors %d.", TEST_NAME, i+0x2000, data[i], readback, test, print_pass()? tmp_msg:"", errors);
         }
      }
      if (verbose) printf("         Pass %2d complete\n",pass);
   }
   return(error1+error2+error3);
}
 
gp1_open()
{
	int p;
	int align;
	caddr_t gpm;

	if (simulate_error == VME24_NOT_OPEN) 
	   strcpy(file_name, "/dev/vme24.invalid");
	else strcpy(file_name, "/dev/vme24");

	if ((gp1_fd = open(file_name, O_RDWR)) < 0) {
	   perror(perror_msg);
      	   if (atn) send_msg_to_atn(FATAL,
       		"Couldn't open file '%s'.", file_name);
      	   throwup(-VME24_NOT_OPEN,
       		"%s: Couldn't open file '%s'.", TEST_NAME, file_name);
	}
	align = getpagesize();
	if ((allocp = (caddr_t) malloc(VME_GP1SIZE+align)) == 0 ||
		simulate_error == NO_MALLOC) {
	   perror(perror_msg);
      	   if (atn) send_msg_to_atn(FATAL,
       		"Couldn't allocate address space.");
      	   throwup(-NO_MALLOC,
       		"%s: Couldn't allocate address space.", TEST_NAME);
	}
	p = ((int) allocp + align -1) & ~(align-1);
	if (mmap(p, VME_GP1SIZE, PROT_READ|PROT_WRITE, MAP_SHARED,
		gp1_fd, VME_GP1BASE) || simulate_error == NO_MAP) {
	   perror(perror_msg);
      	   if (atn) send_msg_to_atn(FATAL,
       		"Couldn't map the graphics processor.");
      	   throwup(-NO_MAP,
       		"%s: Couldn't map the graphics processor.", TEST_NAME);
	}
	gp1_base = (short *) p;
	gpm = (caddr_t) (p + GP1_SHMEM_OFFSET);
	gp1_shmem = (short *) gpm;
}

gp1_close()
{
	if (gp1_base)
		{
		close(gp1_fd);
		free(allocp);
		gp1_base=0;
		}
}

gp1_reset()
{
	gp1_hwreset();
	gp1_swreset();
}

gp1_hwreset()
{
	gp1_base[GP1_CONTROL_REG] = GP1_CR_CLRIF | GP1_CR_INT_DISABLE |
					GP1_CR_RESET;
	gp1_base[GP1_CONTROL_REG] = 0;
}

gp1_swreset()
{
	register int *shmem = (int *) gp1_shmem;
	register short i;

	i = 133;
	while (--i)
		*shmem++ = 0;
	*((int *) &gp1_shmem[10]) = 0x800000FF;
}

gp1_load(filename)
char *filename;
{
   FILE *fp;
   u_short tadd, nlines;
   u_short ucode[4096 * 4];
   int nwords;
   register u_short *ptr;
   register short *gp1_ucode;

   if(exec_by_sysdiag) {
     strcpy(SD, getenv("SYSDIAG_DIRECTORY"));
     strcat(SD,"/");
     if (simulate_error == UFILE_NOT_OPEN) strcat(SD,"no.ucode");
     else strcat(SD,filename);
   }

   if((fp=fopen(SD,"r"))==NULL) {
      if (atn) send_msg_to_atn(FATAL,
          "Couldn't open microcode file '%s'.", SD);
      throwup(-UFILE_NOT_OPEN,
          "%s: Couldn't open microcode file '%s'.", TEST_NAME, SD);
   }
   gp1_ucode = &gp1_base[GP1_UCODE_DATA_REG];
   while(fread(&tadd, sizeof(tadd), 1, fp) == 1) {
						/* starting microcode address */
	gp1_base[GP1_UCODE_ADDR_REG] = tadd;
	fread(&nlines, sizeof(nlines), 1, fp);
						/* number of microcode lines  */
	while(nlines > 0) {
		nwords = (nlines > 4096) ? 4096 : nlines;
		nlines -= nwords;
		fread(ucode, sizeof(u_short),  4 * nwords, fp);	
		for (ptr = ucode; nwords > 0; nwords--) {
			*gp1_ucode = *ptr++;
			*gp1_ucode = *ptr++;
			*gp1_ucode = *ptr++;
			*gp1_ucode = *ptr++;
		}
	}
   }

fclose(fp);
}

gp1_vp_start(cont_flag)
int cont_flag;
{
	register short *gp1_cntrl = &gp1_base[GP1_CONTROL_REG];

	*gp1_cntrl = 0;
	if (cont_flag)
		*gp1_cntrl = GP1_CR_VP_CONT;
	else
		*gp1_cntrl = GP1_CR_VP_STRT0 | GP1_CR_VP_CONT;
}

gp1_vp_halt()
{
	register short *gp1_cntrl = &gp1_base[GP1_CONTROL_REG];

	*gp1_cntrl = 0;
	*gp1_cntrl = GP1_CR_VP_HLT;
}

gp1_pp_start(cont_flag)
int cont_flag;
{
	register short *gp1_cntrl = &gp1_base[GP1_CONTROL_REG];

	*gp1_cntrl = 0;
	if (cont_flag)
		*gp1_cntrl = GP1_CR_PP_CONT;
	else
		*gp1_cntrl = GP1_CR_PP_STRT0 | GP1_CR_PP_CONT;
}

gp1_pp_halt()
{
	register short *gp1_cntrl = &gp1_base[GP1_CONTROL_REG];

	*gp1_cntrl = 0;
	*gp1_cntrl = GP1_CR_PP_HLT;
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
   if (atn) {
      sprintf(tmp_msg, " pass %d,", pass);
      return TRUE;
   }
   else return FALSE;
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
