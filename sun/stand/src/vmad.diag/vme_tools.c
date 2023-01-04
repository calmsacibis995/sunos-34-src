
/*********************************************************************/
/* Probe tools for VME_Multibus Adapter Diagnostic		     */
/*********************************************************************/
static char     toolsccsid[] = "@(#)vme_tools.c 1.1 9/25/86 Copyright Sun Micro";

#include    <sys/types.h>
#include    <setjmp.h>
#include    <machdep.h>
#include    <s2addrs.h>
#include    "vme_adap.h"



jmp_buf  get_out,  bus_err, intr_buf;
struct   vme_adap_par vap;
int    	 test_flag, debug_flag;






/***************************************************************************/
/* probe_tool  						     		   */
/*   This debug tool will loop on accessing the data register. The routine */
/* will catch bus errors and will not time out. User can abort this debug  */
/* debug loop by entering the conrol key.				   */
/***************************************************************************/
probe_tool()
{

   struct   prog_dma_reg  *ptpdr;
   int          c, odd, cnt = 0;

   ptpdr         =   vap.pdr;
   odd = 0;
  
   cup(17, 40);
   while(1){
	_setjmp(bus_err);
	if(cnt > 20){
		odd ^= 1;
		if(odd)
			printf("\b+");
		else
			printf("\b-");

		cnt = 0;
		if((c = chk_usr_req()) != 0)
       			return(c);
	}
	if(cnt < 25) 
                cnt++;
       	ptpdr->data = 0xFFFF;
   }
}




/**********************************************************************/
/* reset_tool						              */
/*   This debug tool will loop on reseting the board. Bus errors will */
/* not cause the program to abort abnormally. User can stop the loop  */
/* by entering the conrol key.					      */
/**********************************************************************/
reset_tool()
{

   struct   prog_dma_reg  *ptpdr;
   int          c, odd, cnt = 0;

   ptpdr         =   vap.pdr; 
   odd = 0;
  
   cup(17, 40);
   while(1){
	_setjmp(bus_err);
	if(cnt > 20){
		odd ^= 1; 
                if(odd) 
                        printf("\b+");
                else
                        printf("\b-");
		cnt = 0;
		if((c = chk_usr_req()) != 0)
                 	return(c);
	}
	if(cnt < 25)
		cnt++;
	reset_board();
   }
}




/***************************************************************************/
/* interrupt_tool							   */
/*    This debug loop will continuosly tries to interrupt the board. The   */
/* interrupt service routine will do nothing but returning to the caller.  */
/* User can stop the loop by entering the control key.			   */
/***************************************************************************/
interrupt_tool()
{

   struct   prog_dma_reg  *ptpdr;
   int		c, odd, cnt = 0;
 
   ptpdr         =   vap.pdr;  
   odd = 0;
   tool_intr_init();

   cup(17, 40);
   while(1){ 
	_setjmp(intr_buf);
	if(cnt > 20){
		odd ^= 1; 
                if(odd) 
                        printf("\b+");
                else
                        printf("\b-");
		cnt = 0;
		if((c = chk_usr_req()) != 0){
			ptpdr->pcr = PD_RESET;
                 	return(c);
		}
	}
	if(cnt < 25)
		cnt++;
	asm ("movw      #0x2100, sr");
        ptpdr->pcr = 0x01;
   }
}




/************************************************************************/
/* Byte/Word DMA Tool							*/
/*   This debug loop will continuosly transfers the user given amount of*/
/* bytes/words until user stops the loop by a control key. Each time it */
/* will dma for the number of bytes/words and start over and over. The  */
/* has the oppotinuty of checking the read/write dma strobes and the    */
/* the data/address lines as well.                                      */
/************************************************************************/
dma_tool()
{
   u_char        *pt;
   int   cnt, odd, c, mode, direct, len;
   u_short	pattern;
   struct   prog_dma_reg  * ptpdr;

   ptpdr           =   vap.pdr;
   odd = 0;

   clear_below(21, 1);
   len = pmgn("enter number of bytes to transfer: ");
   mode = PD_WORD_MODE;

   if(vap.test_num == 3){
	direct = 0x00;                  /* write to cup memory  */ 
	printf("\n");
   	pattern = pgetnh("pattern to write in hex? ");
	ptpdr->data = pattern & 0xffff;
    }else
	direct = 0x08;			/* read from cup memory */

    cup(17, 40);
    while(1){
        _setjmp(intr_buf);
        if(cnt > 20){
		odd ^= 1; 
                if(odd) 
                        printf("\b+");
                else
                        printf("\b-");
                cnt = 0;
                if((c = chk_usr_req()) != 0)
                        return(c);
        }
        if(cnt < 25)
                cnt++;

       /* 
        * begin dma 
        */ 
        dma_transfer(0, mode, direct, len); 
    }
}





/************************************************************************/
/* verify_reg_tool							*/
/*   This routine is used to verify data/count registers by writing a   */
/* user defined pattern to the reg and then reading it back for         */
/* comparing for a number of times given by user. User can terminate    */
/* the loop by a control key.						*/
/************************************************************************/
verify_reg_tool()
{

struct   prog_dma_reg  *ptpdr;
u_long	 pattern, loop, val;
int      i, c, line = 0;

ptpdr         =   vap.pdr;
cup(21, 1);
loop          =   pmgn("enter number of times to verify:   ");
printf("\n");
pattern	      =   pgetnh("pattern to write in hex?  ");
clear_screen();

	switch(vap.test_num){
	   case 5:
	       for(i = 1; i < loop; i++){
		   if(!_setjmp(bus_err)){
		      ptpdr->data = pattern & 0xffff;
		      val = ptpdr->data & 0xffff;
		      printf("W=0X%x, R=0X%x  ", pattern & 0xffff
				, val & 0xffff);
		   }else{
		      printf("\n** got bus error reading data register");
		      printf("\n** stop looping");
		      break;
		   }
	           if((c = chk_usr_req()) != 0){
			redraw_screen();
               	        return(c);
		   }
	        } 
		break;

	    case 6:
                for(i = 1; i < loop; i++){
                    if(!_setjmp(bus_err)){
	              ptpdr->dma_count = pattern & 0xffff;
		      val = ptpdr->dma_count & 0xffff;
		      printf("W=0X%x, R=0X%x  ", pattern & 0xffff
				, val & 0xffff);
	            }else{
		      printf("** got bus error reading data register");
		      printf("\n** stop looping");
                      break;
                    }
                    if((c = chk_usr_req()) != 0){
			redraw_screen();
                        return(c);
		    }
                }
		break;

	}
	printf("\n<ENTER A KEY TO CONTINUE>");
	getchar();
	redraw_screen();
	return(0);
}



