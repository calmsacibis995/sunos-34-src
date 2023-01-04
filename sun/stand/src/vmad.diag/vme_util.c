#include    <sys/types.h>
#include    <setjmp.h>
#include    <machdep.h>
#include    "vme_adap.h"
#include    "vme_util.h"

static char     utilsccsid[] = "@(#)vme_util.c 1.1 9/25/86 Copyright Sun Micro";

jmp_buf 	 get_out, bus_err, intr_buf;
struct   	 vme_adap_par vap;

char 	*test_name[] = {
     	"Time Out", 
     	"Data Path",        
     	"Bus Arbit",          
     	"Interrupt",          
     	"Byte Mode",          
     	"Word Mode",          
     	"All"
}; 




int test_flag, debug_flag;


/**************************************************************/
/* This  function  checks for any  quit or  exit diagnostics  */
/* or stop loop/resume loop request  from user during test    */
/* execution.  In case of ^X -- exit diagnostic, ^C -- exit   */
/* active test, ^S -- suspend loop, ^Q -- resume loop, or     */
/* will return 0 to the caller.                               */
/**************************************************************/
chk_usr_req()
{

   int   c, a;
   c = maygetchar();
   switch(c){
	case 0x13:			/* control S */
		for(;;){
		  if(debug_flag && vap.test_num > 4)
			;
		  else{
			clear_below(21, 1);
			cup(20, 1);
		  }
		  printf("\n waiting for you to enter CNTRL Q to continue or,");
		  printf("\n one of the special keys ");
		  a = getchar();
		  if( a != 0x11 && a != 0x03 && a != 0x18 && a != 0x06)
			if(test_flag)
				clear_below(21, 1);
		  if(a == 0x11){
			if(test_flag)
				clear_below(21, 1);
			if(debug_flag && vap.test_num < 5){
				clear_below(21, 1);
				cup(17, 40);
			}
		  	return(0);
		  }
		  if(a == 0x03 || a == 0x18){
			if(test_flag)
				clear_below(21, 1);
			break;
		  }
		  if(test_flag)
		  	cup(22, 1);
		  printf("\n What? ");
		}
	case 0x03:                      /* control C  		  */
                  return(c);
        case 0x18:                      /* control X   		  */
		  _longjmp(get_out);	/* exit diagnostic	  */
	case 0x06:
		  toggle_info_msg();    /* turn on or off info bit*/
		  return(0);
	default:
		  return(0);
   }
}
      
 
 



/**************************************************************/
/* Print the information message from the test_info table.    */
/*							      */
/**************************************************************/
prt_info_msg()
{
int 	i, j = 0;
char	c;
	
 vap.info = 0; 				/* turn off info flag */
 clear_screen();
 if(test_flag){
   for ( i = 0; test_info[vap.test_num][i] != 0; i++, j++){
          printf("    %s\n", test_info[vap.test_num][i]);

	  if(j > 23){	
		j = 0;
		printf("\n<< PRESS ANY KEY TO CONTINUE or,");
	  	printf(" ENTER q, TO QUIT INFO >>");	

	  	if(c = getchar() == 'q' || c == 'Q'){
			redraw_screen();
			return(-1);	/* don't print the rest */
		}
		printf("\n");
	   }
   }
 }else{
   for ( i = 0; debug_info[vap.test_num][i] != 0; i++, j++){
          printf("    %s\n", debug_info[vap.test_num][i]);

          if(j > 23){
                j = 0;
                printf("\n<< PRESS ANY KEY TO CONTINUE or,");
                printf(" ENTER q, TO QUIT INFO >>");

                if(c = getchar() == 'q' || c == 'Q'){
                        redraw_screen();
                        return(-1);     /* don't print the rest */
                }
		printf("\n");
           }   
   }
 }
   printf("<PRESS ANY KEY TO CONTINUE>");
   getchar();
   redraw_screen();
}




/**************************************************************/
/* toggles informational messages on which allows test info   */
/* to be printed before a test or a tool begins.              */ 
/**************************************************************/
toggle_info_msg()
{
   vap.info ^= 1;
   cup(19, 70);
   printf("%s", vap.info? "on " : "off");
}




/**************************************************************/
/* This is the declaration of VME adapter error codes to err  */
/* print functions. Each of the predefined error code has a   */
/* corresponding error print function which prints the error  */
/* messages associating with that error code.                 */ 
/**************************************************************/
int	prt_bus_err(), prt_dma_time_out_err(), prt_no_time_out_err(),
	prt_no_reset_err(), prt_intrup_errs(), prt_data_mis_cmpare_err(),
	prt_data_addr_errs();

struct  errcode_to_msg {

      int     err_code;
      int     (*prt_err_funct)();

}etm[] =  {

	BUS_ERROR,		prt_bus_err,
	DATA_MIS_COMPARE,       prt_data_mis_cmpare_err,
	NO_TIME_OUT_ERR,	prt_no_time_out_err,
	NO_RESET_ERR,		prt_no_reset_err,
	NO_INTRUP_ERR,		prt_intrup_errs,
	BAD_INTRUP_ERR,		prt_intrup_errs,
	DMA_TIME_OUT_ERR,       prt_dma_time_out_err,
	DMA_BYTE_ADDR_ERR,	prt_data_addr_errs,
	DMA_WORD_ADDR_ERR,	prt_data_addr_errs,
	0,
};

/**************************************************************/
/* Handles error roports. It prints out the test which failed */
/* and information on the test's functionality,  then  calls  */
/* other functions to do error displays. Afterward it calls   */
/* find_debug_tool() to allows their user to invoke debug     */
/* tools if running in manual mode.                           */ 
/**************************************************************/
error_report()
{
   struct  errcode_to_msg   *eptr;
   int     i;

     /*
      * Clear the screen for error report.
      */
      clear_below(22, 1);
      cup(22, 1);
      for (eptr = &etm[0]; eptr->err_code != 0; eptr++)
         if (vap.elog.code & eptr->err_code)
            (*eptr->prt_err_funct)();
}





/**************************************************************/
/*  Get_loop_num returns a loop counter value to the calling  */
/* function. The value return can be either the defualt value */
/* which is passed from the calling function or a value enter-*/
/* ed from user depending on the run 'all' flag.              */ 
/**************************************************************/
get_loop_num()
{
   int num = 1;
   if (!vap.all){
      cup(21, 1);
      num = (pmgn(" Enter number of times to run: "));
   }
   return(num);
}





/**************************************************************/
/* prints the char string passed by caller, gets reply from   */
/* the user, converts it into hex number, and returns it to   */
/* caller.                                                    */
/**************************************************************/
pgetnh(s)
   char *s;
{
   printf(s);
   return (getnh());
}




/**************************************************************/
/* This function compares the response from the user and the  */
/* character 'y' or 'Y' and returns the result to te caller.  */
/**************************************************************/
pgetr(s)
   char  *s;
{
   char  buf[LINEBUFSZ], *p;

   printf(s);

   gets(buf);

   for (p = buf; p < &buf[LINEBUFSZ]; p++)
      if (*p != ' ' && *p != '\t')
	      break;  /* for loop. */
 
   return (*p == 'y' || *p == 'Y');
}




char 	*fault_reg[] = {
	"data reg",
	"control reg",
	"count reg",
	"dma addr reg",
	0
};

/**************************************************************/
/* prt_bus_err                                                */
/* prints the type bus error occured                          */
/*                                                            */
/**************************************************************/
prt_bus_err()
{
struct 	prog_dma_reg	*ptpdr = vap.pdr;
	
	printf(" * bus err of type %s occured while testing %s\n",
		vap.berr_pt, test_name[vap.test_num]);
	
}
/**************************************************************/
/* prt_dma_time_out_err                                       */
/*                                                            */
/**************************************************************/
prt_dma_time_out_err()
{
struct  prog_dma_reg    *ptpdr = vap.pdr;
         
	printf(" * dma time out occured while testing %s dma\n", 
		test_name[vap.test_num]); 
	printf(" the DMA count reg read %x 	should read %x\n",
		vap.elog.act_val, vap.elog.exp_val);
	printf(" the DMA address reg is %x", ptpdr->dma_addr);
}
/**************************************************************/
/* prt_no_time_out_err                                        */
/*                                                            */
/**************************************************************/
prt_no_time_out_err()
{
struct  prog_dma_reg    *ptpdr = vap.pdr;

	printf(" * time out did not happen\n");
	printf(" the board must have been timed out because a hardware\n");
	printf(" that does not exist on the board, was accessed");
}
/**************************************************************/
/* prt_no_reset_err                                           */
/*                                                            */
/**************************************************************/
prt_no_reset_err()
{
struct  prog_dma_reg    *ptpdr = vap.pdr;

	printf(" * reset board failed\n");
	printf(" the control reg reads %x        should read %x",
		vap.elog.act_val, vap.elog.exp_val);
	
}
/**************************************************************/
/* prt_intrup_errs                                            */
/*                                                            */
/**************************************************************/
prt_intrup_errs()
{
struct  prog_dma_reg    *ptpdr = vap.pdr;

	if(vap.elog.code & NO_INTRUP_ERR){
		printf(" * intr signal is not passed to adapter board\n");
		printf(" the control reg reads %x", vap.elog.act_val);
	}else{
		printf(" * wrong intr vector is used\n");
		printf(" the interrupt vector %x must be used, check DIP 12",
			vap.elog.exp_val);
	}
}
/**************************************************************/
/* prt_data_mis_cmpare_err                                    */
/*                                                            */
/**************************************************************/
prt_data_mis_cmpare_err()
{
struct  prog_dma_reg    *ptpdr = vap.pdr;

	printf(" * data miss compare \n");
	printf(" wrote %x to %s		read back %x",
		vap.elog.exp_val, fault_reg[vap.elog.reg_in_err],
		vap.elog.act_val);
		
}
/**************************************************************/
/* prt_data_addr_errs                                         */
/*                                                            */
/**************************************************************/
prt_data_addr_errs()
{
struct  prog_dma_reg    *ptpdr = vap.pdr;

	printf(" * byte packing error occured during dma transfer\n");
	printf(" the incorrect byte address was passed by the adapter");
}



/****************************************************************/
/* prt_passed_msg()                                             */
/*                                                              */
/****************************************************************/
 
prt_passed_msg()
{
        clear_below(21, 1);
        cup(21, 1);
        printf(" TEST PASSED ");
}




/****************************************************************/
/* redraw_screen						*/
/*								*/
/****************************************************************/
redraw_screen()
{

	prt_headings();
	draw_line();
	prt_menu(0);                         /* special kesys         */
	prt_menu(1);
	if(debug_flag)
		prt_menu(3);
	else
		prt_menu(2);
}





/****************************************************************/
/* prt_headings							*/
/*								*/
/****************************************************************/
prt_headings()
{
   clear_screen();
   cup(1, 1);
   printf("           ");
   printf(" SUN's VME-MULTIBUS ADAPTER Board Diagnostic  Rev 1.1 9/25/86 ");
}
