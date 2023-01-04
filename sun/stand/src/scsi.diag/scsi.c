/******************************************************************************/
/******************************  TEST HEADER  *********************************/
/*                                                                            */
/*  1. PROGRAM TITLE :  SCSI Diagnostic.                                      */
/*  2. DATE :                                                                 */
/*  3. RESPONSIBLE ENGINEER : HAI THE NGO                                     */
/*			                 				      */
/*  4. OPTION USED :                                                          */
/*  5. FUNCTIONAL SPECIFICATION :                                             */
/*   		Performs functionality test on SCSI board. And provides       */
/*  	  board level debug feature, such that it will be usefull for         */
/*        component level debug and repair.                                   */
/*                                                                            */
/*  6. CHANGES :                                                              */
/*     REV     ID      SUMMARY OF CHANGES                                     */
/*     ------------------------------------------------------------------     */
/*     2.1     A        On 08/09/84, the changes were made to convert         */
/*                     test to menu driven, improve error reporting, add      */
/*                     in options and commands, and changed DMA transfer      */
/*                     to work with i/o device, likely disc, on system.       */
/*                      Comments were also added in to make test more         */
/*                     maintainable in furture.                               */
/*                                                                            */
/*                                                                            */
/*    Follow is the list of all functions that are defined in this file :     */
/*      _ main             main program.                                      */
/*      _ prompt_set_par   prompt and set scsi parameters.                    */
/*      _ execute_diag     execute SCSI diagnostic.                           */
/*      _ auto_diag        auto diagnostic ( go, no go).                      */
/*      _ manual_diag      manual diagnostic ( menu driven).                  */
/*      _ run_all_tests    executes all scsi's tests.                         */
/*      _ prt_menu_get_rep print test menu and get reply.                     */
/*      _ chk_ret_val      check return value.                                */
/*      _ clear_message    clear pass messages.                               */
/******************************************************************************/
/******************************************************************************/

  
static  char    sccsid[] = "@(#)scsi.c 1.1 86/09/25 SMI." ;


#include    <sys/types.h>
#include    <setjmp.h>
#include    <machdep.h>
#include    <s2addrs.h>
#include    <sun/dklabel.h>
#include    "sc_reg.h"
 
/*
 * swap_par_addr will hold the start address of the swap partition
 * on the disk. This is done by extracting the label info on the
 * disk. If by any reason, the disk did not have a label(even backup label)
 * the swap_par_addr is set to Micropolis 1304 swap partiton address
 * (cyl 156). This assignment of the swap partition for testing WRITE commands
 * will guaranty the nondestructiveness of the SCSI test.
 * Also nsect_per_cyl and drive_id will be copied from label data.
 */
extern	long swap_par_addr, nsect_per_cyl;
char	*drive_id;

int    test_icr_reg(), test_DMA_cntr_reg(), test_DMA_addr_reg(),
       test_init_select(), test_dev_ready(), test_bus_transfer(),
       test_DMA_and_data_integrity(), test_status_and_overrun_intr(), 
       test_timer(), test_SCC();
 
struct menu {
    
     int    test_num;
     char   *name;
     int    (*sc_test_ptr)();

}    top_menu[] = {
     01, " Interface Control Register",     test_icr_reg,
     02, " DMA Counter Register",           test_DMA_cntr_reg,
     03, " DMA Address Register (VME only)",test_DMA_addr_reg,
     04, " Initial Selection Sequence",     test_init_select,
     05, " Device Ready",                   test_dev_ready,
     06, " Bus Transfer",                   test_bus_transfer,
     07, " DMA Transfer",                   test_DMA_and_data_integrity,
     08, " Status Interrupt",               test_status_and_overrun_intr,
     09, " DMA Overrun",                    test_status_and_overrun_intr,
     10, " Data integrity",                 test_DMA_and_data_integrity,
     11, " Timer (VME only)",               test_timer,
     12, " SCC (Multibus only)",            test_SCC,
     13, " all.",                           0,
     0,  0,
};

 
jmp_buf  get_out, bus_error;          	/* declare none local jump.           */

struct  scsi_par  scsi;                 /* SCSI parameters.                   */




/****************************** BEGIN OF MAIN *********************************/
main()
{
   prompt_set_par();

   exchg_buserr_handler();  /* set up to catch bus error */ 

   if ( !_setjmp(get_out))
      execute_diag();
   
   restore_sys_buserr_handler();                   
   printf("\n  EXIT...!!!\n");
}

/**************************  END OF MAIN  *************************************/





/******************************************************************/
/* This function set up all parameters that are going to be used  */
/* by the test.                                                   */
/******************************************************************/
prompt_set_par()
{
int label_found;

   bzero((u_short *)&scsi, sizeof scsi);

   scsi.scsi_addr = SCSI_BASE;
   scsi.target    = 0;
   scsi.unit      = 0;
   scsi.int_en    = 0;
   scsi.vme       = (( GETPHYS(SCSI_BASE) == 0x200000 ) ? 1 : 0);

   /*
    * call label_verify routine to find out the disk type and set
    * the disk parameters. If no label, assume SCSI disk.
    */
   label_found = label_verify();

   if(!label_found){
   	nsect_per_cyl = 6 * 17;
	swap_par_addr = 160;
   }

  /*
   * Set starting block address for write to disk to swap partion
   * address. 
   */
   scsi.sblk      = nsect_per_cyl * swap_par_addr;
 
   printf("\n\nSUN %s SCSI Board Diagnostic  Rev. 1.1  9/25/86 ",
          (scsi.vme) ? "VME" : "Multibus");

   if(label_found)
	printf("\nRunning on %s", drive_id);
   else 
	printf("\nRunning on unknown disk type");
}



/******************************************************************/
/* This function ask ther user to select Automatic or manual mode */
/* testing, and calls the selected test mode.                     */
/******************************************************************/
execute_diag()
{
   char    buf[LINEBUFSZ], *p;


   while (1){

      printf("\n\nEnter A for automatic or M for manual    ");
      gets(buf);
 
      for (p = buf; p < &buf[LINEBUFSZ]; p++)  /* align the entry. */
         if ( *p != ' ' && *p != '\t')
            break;  /* break the for loop. */
      
      if (*p == 'a' || *p == 'A' || *p == 'm' || *p == 'M')
         break;  /* break the while loop. */
   
      printf("What  ? ");
      
   }

   if (*p == 'a' || *p == 'A')
      auto_diag();
   else
      manual_diag();
}



/******************************************************************/
/*  auto_diag function sets n_run to one and calls run_all_tests  */
/* function.  If run_all_tests  returns a value of  0, indicates  */
/* that all tests have run successfully, it will ask the user if  */
/* he/she want to rerun and the number of times. If  a  return    */
/* value is other then 0, indicates that something has gone wrong,*/
/* it will terminates the test and return to calling function.    */
/******************************************************************/
auto_diag()
{
   int     c = 0, n_run = 1;

   printf("\n  SCSI AUTOMATIC TEST.\n");

   while ((c == 0) && (n_run > 0)){

      if ((c = run_all_tests(n_run)) == 0 ){
      
         printf("\n\n  SCSI DIAGNOSTIC COMPLETED...NO ERROR");
         if (pgetr("\n  Do you wish to run Diagnostic again ?  "))
            n_run = pgetn("\n  Enter number of times to run   ");
         else
            break;  /* break the while loop */

      }
   }
}






/******************************************************************/
/* This function prints tests menu and ask the user to select     */
/* which test to run. If test all option is selected, it calls    */
/* run_all_test function which increment throught all of the tests*/
/* otherwise a individual test is called to execute.              */
/******************************************************************/
manual_diag()
{
   int     c, t_entries = 0, num = 0, n_run;
   struct  menu  *mp;

   scsi.man = 1;
   for ( mp = &top_menu[0]; (mp+1)->name != 0 ; mp++) ;
   t_entries= mp->test_num;

   while (1){

      prt_menu_get_rep(&num);

      if ( t_entries == num){
       
         printf("\nRUN ALL TESTS.");
         n_run = (pgetn("\n Enter number of times to run   "));
         c = run_all_tests(n_run);

      }else{

         scsi.all = 0;
         mp = ( &top_menu[0] + ( --num));      /* pointer = selected test.*/
         printf("\n  %d%sTesting%s.",mp->test_num,
               ((mp->test_num < 10) ? "  " : " "), mp->name);
         scsi.test_num = mp->test_num;
         if ((c = (*mp->sc_test_ptr)()) != 0)  
            chk_ret_val(c);

      }
   }
}


/******************************************************************/
/*  run_all_tests function calls and executes all of the tests    */
/* listed on menu. If any of the calling test return with a value */
/* of -1 , indicates that an error has occurres, an error handler */
/* routine is called to print out error messages, and eventually  */
/* return to calling function.                                    */  
/******************************************************************/
run_all_tests(n_run)
   int     n_run;
{
   int     c;
   struct  menu  *mp;

   scsi.all = 1;

   for ( scsi.pass_num = 1; scsi.pass_num <= n_run  ; scsi.pass_num++){

      /* increment menu pointer and call corresponding test */
      for ( mp = &top_menu[0]; (mp+1)->name != 0 ; mp++){

         if ( scsi.pass_num == 1 )
            printf("\n  %d%sTesting%s.",mp->test_num,
                  ((mp->test_num < 10) ? "  " : " "), mp->name);

         scsi.test_num = mp->test_num;
         if ((c = (*mp->sc_test_ptr)()) != 0){
            chk_ret_val(c);
            return(c);
         } 

         if (scsi.pass_num != 1) clear_message();

      }

      if (scsi.pass_num != 1)
         printf("\n  SCSI Diagnostic...pass # %d", scsi.pass_num);

   }

   return(0);
}




/******************************************************************/
/* This function prints out the top menu, checks the reply from   */
/* user, and sets the total number of entries in top menu and the */
/* selection number in the t_entr and num variables respectively. */
/******************************************************************/
prt_menu_get_rep(num)
   int    *num;
{
   int     i;
   char    buf[LINEBUFSZ];
   struct  menu   *mp;


   for ( *num = 0; *num == 0;){

      printf("\n\nTESTS MENU:");
      for (i = 0,mp = &top_menu[0]; mp->name != 0; i++, mp++)
         printf("\n  %d%sTest%s.", mp->test_num,
               ((mp->test_num < 10) ? "_ " : "_"), mp->name);
      printf("\n  Select One  ");

      gets(buf);

      mp--; /* align the pointer to the last test in menu */
      *num    = (chk_reply(buf,mp->test_num));
   }
}





/******************************************************************/
/* This function cheks the return value to determinde if and error*/
/* has occurred. If so it calls error_report function, otherwise  */
/* quit or exit the diagnostics  depending  on the return  value  */
/* passed by the calling function.                                */
/******************************************************************/
chk_ret_val(c)
   int  c;
{
   struct  menu   *mp;

   switch (c){
      case -1 :
         mp = ( &top_menu[0] + (scsi.test_num - 1));
         printf("\n\n**Error: test %s failed.", mp->name);
         error_report();
         break;

      case 'p' :
         break;

      case 'Q' :
         break;

      case 'x' :
         _longjmp(get_out);

      case 'X' :
         _longjmp(get_out);

   }
} 





/******************************************************/
/* Clear passes and skipped messages.                 */
/******************************************************/
clear_message()
{
   printf("\b\b\b\b\b\b\b\b\b");
   printf("         ");
   printf("\b\b\b\b\b\b\b\b\b");
}


/**********************************************************/
/* label_verify  copied from label.c with alot of changes */
/**********************************************************/
label_verify()
{
	struct scsi_ha_reg *har;
        struct dk_label *l = (struct dk_label *)DBUF_VA;
        int found, i;

	har           =   scsi.scsi_addr;

        l->dkl_asciilabel[0] = 0;
        l->dkl_magic = 0;
        set_command(SC_READ, 0, 1);
	if(dma_transfer(har, DBUF_PA, 512) != 0)
		printf("can not talk to disk, correct the problem");

        if ((l->dkl_magic != DKL_MAGIC) ) 
                 return (0);

        found = 0;
        for (i = 0; i < NDKMAP; i++) {
                if (l->dkl_map[i].dkl_nblk != 0) 
                        found = 1;

        }
        if (!found) 
                return(0);

        drive_id      = l->dkl_asciilabel;
    	swap_par_addr = l->dkl_map[1].dkl_cylno + 5; /* add 5 to be sure */
	nsect_per_cyl = l->dkl_nhead * l->dkl_nsect;

        return(1);
}



