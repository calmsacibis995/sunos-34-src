
/*
 *      static char     fpasccsid[] = "@(#)fpa.systest.c 1.1 9/25/86 Copyright Sun Microsystems";
 *
 *      Copyright (c) 1985 by Sun Microsystems, Inc 
 *
 *
 *      This is the main file.
 *
 *      Author : Chad B. Rao
 *
 *      Date   : 1/16/86       ....   Revision A
 *
 */
#include <sys/types.h>
#include "fpa.h"
#include <stdio.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include </usr/include/sundev/fpareg.h>  
		/* for FPA_FAIL */ 

extern int ierr_test(), imask_test(), ldptr_test(), map_ram();
extern int ustore_ram(), reg_uh_ram(), sim_ins_test(), register_ram();
extern int reg_ram(), shadow_ram(), test_mode_reg(), test_wstatus_reg();
extern int w_op_test(), fpa_ws(), timing(), w_jump_cond_test();
extern int fpa_wd(), open_new_context(), close_new_context(), other_contexts();
extern int lock_test(), ptr_incdec_test(), pointer_test(), wlwf_test();
extern int winitfp_(), lin_pack_test(), nack2_test(); 
extern restore_signals();

extern int  dev_no, open_fpa;
extern int	errno;

char *err_msg[] = {
       
       "Ierr",
       "Imask",
       "Ldptr",
       "Mapping Ram",
       "Micro Store Ram",
       "Register Ram Upper Half",
       "Simple Instruction",
       "Register Ram Lower Half",
       "Shadow Ram",
       "Pointer",
       "Pointer Incr. Decr.",
       "Lock",
       "F+",
       "Mode Register",
       "Wstatus Register",
       "Weitek Data Path",
       "Weitek Operation",
       "Weitek Status",
       "Jump Conditions",
       "Timing",
       "Linpack"

}; 


char send_err_msg [120];
char broad_msg[120];
char kernal_msg[120];
char success_msg[120];
char    temp_str[180];
int	sig_err_flag;
int	seg_sig_flag;
u_long  contexts; /* to get the context in case it fails */
int	verbose_mode = 0, no_times = 1;

main(argc, argv) 
	int argc;
	char *argv[];
{
	int	value, i, val_rotate, j;
	long	io_value;
	u_char    val;
	u_long  *st_reg_ptr, pass_count;
	char  *s;
	char	null_str[2];
	
	null_str[0] = '\0';
	send_err_msg[0] = '\0';
	broad_msg[0] = '\0';
	kernal_msg[0] = '\0';
	success_msg[0] = '\0';
	temp_str[0] = '\0';
	sig_err_flag, seg_sig_flag = 0;

	for (i = 1; i < argc ; i++){
		if (*argv[i] == '-'){
			switch(argv[i][1]){
				case 'v':
					verbose_mode = 1;
					break;
				case 'p':
					if (!(no_times = atoi(&argv[i][2]))) no_times = 0x7fffffff;
					break;
				default:
					printf("Unknown option %c\n", argv[i][1]);
					exit(1);
					break;
			}
		} else {
			printf("Usage: %s [-v] [-p<pass_count>]\n", *argv);
			exit(1);
		}
	}

	if (verbose_mode)			
		printf("FPA System (Reliability) Test : ");
	st_reg_ptr = (u_long *)FPA_STATE_PTR;
	if (!(winitfp_()))
	{
		if (verbose_mode)
			printf("Could not open FPA.\n");
		exit(0);
	}
	dev_no = open_fpa;
	close_new_context();
	pass_count = 1;
	do {
		contexts = 0x0; /* initialize the context number */

		value = open_new_context();
		if (value == 0) {
			val_rotate = *st_reg_ptr & 0x1F;
			contexts = (1 << val_rotate);
			if (verbose_mode)
				printf("All Tests - Context Number = %d\n", val_rotate);
			if (verbose_mode) 
                                printf("	Ierr Test.\n");
			if (ierr_test()) {
				fail_close(0); 
				exit(1);
			}
			if (verbose_mode) 
                                printf("	Imask Test.\n");
			if (imask_test()) {
				fail_close(1);  
				exit(1);
			}
			if (verbose_mode) 
                                printf("	Load Pointer Test.\n");
			if (ldptr_test()) {
				fail_close(2);  
				exit(1);
			}
		/*	turn the load enable bit to check the rams */
			
			if ((io_value = ioctl(dev_no,FPA_LOAD_ON,(char *)null_str)) >= 0)
			{	/* do the ram tests only if the user is super user*/
				if (verbose_mode) 
       		                         printf("	Mapping Ram Test.\n");
				if (map_ram()) {
					ioctl(dev_no,FPA_LOAD_OFF,(char *)null_str);
					fail_close(3);  
					exit(1);
				}
	 			if (verbose_mode)  
               		                 printf("        Micro Store Ram Test.\n");
				if (ustore_ram()) {
                                        ioctl(dev_no,FPA_LOAD_OFF,(char *)null_str);
					fail_close(4);  
					exit(1);
				}
				ioctl(dev_no,FPA_LOAD_OFF,(char *)null_str);
                        }
			if (verbose_mode)  
                       	         printf("        Register Ram Upper Half Test.\n");	
			if (reg_uh_ram()) {
				fail_close(5);  
       		       	        exit(1); 
       		 	} 
			if (verbose_mode)
                    	        printf("        Register Ram Lower Half Test.\n");
                        if (reg_ram()) {
                           	fail_close(7);
                                exit(1);
                        }
			if (verbose_mode)  
                                printf("        Simple Instruction Test.\n");
			if (sim_ins_test()) {
				fail_close(6); 
              			exit(1); 
        		} 
			if (verbose_mode)  
                                printf("        Shadow Ram Test.\n");
			if (shadow_ram()) {
				fail_close(8);  
               		        exit(1); 
        		} 
			if (verbose_mode)  
                                printf("        Pointer Test.\n");
                        if (pointer_test()){
                                fail_close(9); 
                                exit(1); 
                        } 
			if (verbose_mode)  
                                printf("        Pointer Increment and Decrement Test.\n");
                        if (ptr_incdec_test()){
                                fail_close(10); 
                                exit(1); 
                        }
			sig_err_flag = 0xff; /* set the flag for error handler */ 
			if (verbose_mode)  
                                printf("        Lock Test.\n");
                        if (lock_test()){
                                fail_close(11); 
                                exit(1);  
                        } 
			sig_err_flag = 0; /* reset the flag for error handler */
			seg_sig_flag = 0xff; /* set segment violation flag */
			if (verbose_mode)
				printf("	Nack Test.\n");
			nack2_test(); /* if any thing goes wrong the program will not come back */
			seg_sig_flag = 0x0; /* reset the segment violation flag */
			if (verbose_mode)  
                                printf("        F+ Test.\n");
			if (wlwf_test()) {  
                                fail_close(12);  
                                exit(1); 
                        }  
			if (verbose_mode)  
                                printf("        Mode Register Test.\n");
			if (test_mode_reg()) {
				fail_close(13);  
               		         exit(1); 
	        	}	 
			if (verbose_mode)  
                                printf("        Wstatus Register Test.\n");
			if (test_wstatus_reg()) {
				fail_close(14);  
       		         	exit(1); 
        		} 
			if (verbose_mode)  
                                printf("        Weitek Data Path Test.\n");
			if (fpa_wd()) {
				fail_close(15);  
               		 	exit(1);
			}
			if (verbose_mode)  
                                printf("        Weitek Operation Test.\n");
			if (w_op_test()) {
				fail_close(16); 
       		         	exit(1);  
        		}  
			if (verbose_mode)  
                                printf("        Weitek Status Test.\n");
			if (fpa_ws()) {
				fail_close(17);  
               		 	exit(1);  
        		}  
			if (verbose_mode)  
                                printf("        Jump Conditions Test.\n");
			if (w_jump_cond_test()) {
       		         	fail_close(18);
				exit(1);  
        		}  
			if (verbose_mode)  
                                printf("        Timing Test.\n");
			if (timing()) {
				fail_close(19);
				exit(1);  
        		}
			if (verbose_mode)  
                                printf("        Linpack Test.\n");
			if (lin_pack_test()) { 
				fail_close(20);
				exit(1);
			}
			close_new_context();
		} 
		else {
			fpa_open_fail(value);
			restore_signals();
			exit(0);
		}
	
		if (val = other_contexts()) 
		{
			if (val != 0xff) { /* test is ok. something wrong */
				      /*   with opening fpa */
				fpa_open_fail(val);
				restore_signals();
				exit(0);
			}
			else {
				fail_close(7); /* lower ram failed */
				exit(1);
			}
		}

		if (verbose_mode)
                                printf("PASS COUNT = %d\n",pass_count);
		pass_count++;
		no_times--; /* reduce the count */
	} while (no_times != 0); 	

	time_of_day();
	make_diaglog_msg(contexts,0,0);
	restore_signals();
	exit(0);
}

fpa_open_fail(fail_code)
	int  fail_code;
{	
	char  *ptr1;
	FILE    *input_file, *fopen();

	time_of_day();

	if ((input_file = fopen("/usr/adm/diaglog","a")) == NULL)
	{
	  printf("FPA Reliability Test Error.\n");
	  printf("    Could not access /usr/adm/diaglog file, can be accessed only under root\n");
	  return;
	}
	ptr1 = send_err_msg;

	if (fail_code == ENETDOWN) {
			strcat(ptr1," : FPA was disabled by kernal while doing ");
			strcat(ptr1,"reliability test, may be due to h/w problem.");
	}
	else if (fail_code == EBUSY) {
		strcat(ptr1," : No FPA context is Available.");
	}
	else if (fail_code == EEXIST) {
		strcat(ptr1," : Duplicate open on FPA context.");
	}	
	
	strcat(ptr1," FPA reliability test is terminated.\n");
	fputs(ptr1,input_file);
	fclose(input_file);
}
fail_close(val)
	int val;
{
	time_of_day();
	
	if (make_diaglog_msg(contexts,1,val))
			return;
	make_broadcast_msg();
	make_unix_msg();
}

/*
 * the following routine gets the time of the day and puts in an array
 *
 */
time_of_day()
{
        char    *tempptr, *temp;
        long     temptime, i;
	char    *ptr1, *ptr2, *ptr3, *ptr4;
 
        time(&temptime); 
        tempptr = ctime(&temptime);

	ptr1 = send_err_msg;    /* for error, writing into the diaglog file */
	ptr2 = broad_msg;       /* for error, sending it to broadcast */
	ptr3 = kernal_msg;	/* for error, sending it to kernal */
	ptr4 = success_msg;	/* for successful message */
	strcat(ptr1,"\n");
	strcat(ptr2,"\n");
	strcat(ptr3,"\n");
	strcat(ptr4,"\n");
	strncat(ptr1, tempptr, 24);
	strncat(ptr2, tempptr, 24);
	strncat(ptr3, tempptr, 24);
	strncat(ptr4, tempptr, 24);
	
}

make_unix_msg()
{
	char	*ptr1, *ptr2, *ptr3;
	int	i, j, val1;
	char    total_str[180];
	int	temp;

	total_str[0] = '\0';
	ptr1 = send_err_msg;
	ptr2 = temp_str;
	ptr3 = total_str;

        strcat(ptr3,ptr1);

	ptr1 = total_str;

	if ((temp = ioctl(dev_no,FPA_FAIL,(char *)total_str)) < 0)
	{
		if (errno == EPERM)
			printf("Not a Super User :Fparel unable to disable FPA. \n");
		else
		
			perror("fparel");
	}
}
make_diaglog_msg(conts, suc_err, val)
	u_long conts;
	int	val; /* for the string */
	int	suc_err; /* should print success = 0; error = 1 */
{
	FILE    *input_file, *fopen();
	char	*ptr1, *ptr2, *ptr3, *ptr4;
	char	i, flag_comma, val1;
	char    number_str[4];
	u_long  val_rotate;

	number_str[0] = '\0';
	ptr1 = send_err_msg;
	ptr2 = temp_str;
	ptr3 = number_str;
	ptr4 = err_msg[val];

	
	if ((input_file = fopen("/usr/adm/diaglog","a")) == NULL)
	{
	 printf("FPA Reliability Test Error.\n");
	 printf("    Could not access /usr/adm/diaglog file, can be accessed only under root permission.\n");
		return(-1);
	}
	if (suc_err == 1) 
		strcat(ptr1," : Sun FPA Reliability Test Failed, Sun FPA DISABLED -  Service Required.\n");
	else if (suc_err == 0)
		strcat(ptr1," : Sun FPA Reliability Test Passed.\n");

	if (suc_err == 1) {
		strcat(ptr2, " Sun FPA : ");
		strcat(ptr2, ptr4);
		strcat(ptr2," Test Failed, Context Number = ");
	}
	else if (suc_err == 0)
		strcat(ptr2," Sun FPA : Contexts tested are : ");

	if (suc_err == 1) 
	{
		val1 = 0;
		for (i = 0; i < 32; i++)
		{
			val_rotate = (conts >> i);
			if (val_rotate & 0x1)
				val1 = i;
		}
		if (val1 < 9){
			number_str[1] = '\0';
			number_str[0]  = val1 + '0';
		}
		else
		{
			 if (val1 <= 19)
			{
				number_str[0] = '1';
				number_str[1] = (val1 - 10) + '0';
			}
			else if (val1 <= 29)
			{
				number_str[0] = '2';
				number_str[1] = (val1 - 20) + '0';
			}
			else 
			{
				number_str[0] = '3';
				number_str[1] = (val1 - 30) + '0';
			}
			number_str[2] = '\0';
		}
		ptr3 = number_str;
                strcat(ptr2,ptr3);
		strcat(ptr2,".\n");

	}
	else {
		flag_comma = 0x0;
			
		for (i = 0; i < 32; i++)  {
			val_rotate = (conts >> i);
			 
			if (val_rotate & 0x1) {
				if (flag_comma != 0x0) 
					strcat(ptr2,", ");
				
				flag_comma = 0x1;
				if (i <= 9){
					number_str[0] = i + '0';
					number_str[1] = '\0';
				}
				else {
					if (i <= 19)
					{
						number_str[0] = '1';
						number_str[1] = (i - 10) + '0';
					}
					else if	 (i <= 29)
					{
						number_str[0] = '2';
						number_str[1] = (i - 20) + '0';
					}
					else
					{
						number_str[0] = '3';
						number_str[1] = (i - 30) + '0';
					}
					number_str[2] = '\0';  /* put the null at the end */
				}
				ptr3 = number_str;
				strcat(ptr2,ptr3);

			}
		}
		strcat(ptr2,".\n");
	}
	fputs(ptr1,input_file);
	fputs(ptr2,input_file);
	fclose(input_file);		
	return(0);			
}
make_broadcast_msg()
{
	FILE	*input_file, *fopen();
	char	val;
	char    *ptr1, *ptr2, *ptr3, *ptr4, *ptr5;
	char	file_name[20];	
	char    file_full_name[40];
	char	wall_str[40];
	char    rm_str[40];

	file_name[0] = '\0';
	wall_str[0] = '\0';
	rm_str[0] = '\0';
	file_full_name[0] = '\0';
	
	ptr1 = broad_msg;

	strcat(ptr1," : Sun FPA Reliability Test Failed, Sun FPA DISABLED - Service Required.\n");

	ptr2 = file_name;
	ptr3 = wall_str;
	ptr4 = rm_str;
	ptr5 = file_full_name;
	strcat(ptr5,"/tmp/");
	tmpnam(ptr2);	/* get the temporary name */
	strcat(ptr5,ptr2);	
	input_file = fopen(ptr5,"w");	
	fputs(ptr1, input_file);
	fclose(input_file);
	strcpy(ptr3,"wall ");
	strcat(ptr3,ptr5);
	strcat(ptr4,"rm -f ");
	strcat(ptr4,ptr5);
	system(ptr3);
	system(ptr4); 
}

 
