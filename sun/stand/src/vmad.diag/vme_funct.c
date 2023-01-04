/*****************************************************************/
/* Functions for VME_Multibus Adapter Diagnostic		 */
/*****************************************************************/
static char     func_sccsid[] = "@(#)vme_funct.c 1.1 9/25/86 Copyright Sun Micro";

#include    <sys/types.h>
#include    <setjmp.h>
#include    <s2addrs.h>
#include    <machdep.h>
#include    <reentrant.h>
#include    "vectors.h"
#include    "m68000.h"
#include    "vme_adap.h"

jmp_buf  get_out,  intr_buf,  bus_err;
struct   vme_adap_par vap;

int	svect48, svect1_7;
int	test_flag, debug_flag;
extern int time_out(), tool_time_out();




char *bus_err_type [] = {
        "page invalid.",
        "p1 bus master.",
        "protection error.",
        "timeout error.",
        "upper byte parity.",
        "lower byte parity.",
};

static int        sys_stack_p;     /* holder of system stack pointer */


/******************************************************************/
/* This function saves the address of bus error interrupt handler */
/* routine.							  */
/******************************************************************/
save_sys_buserr_addr()
{

   vap.sys_buserr_handler = ex_vector->e_buserr;
}



/******************************************************************/
/* This function exchanges the sys_bus error routine with the     */
/* one defined in this diagnostic to catch bus erreors.           */
/******************************************************************/
exchg_buserr_handler()
{

   if(test_flag){
   	vap.pro_buserr_handler  = time_out;
   	ex_vector->e_buserr     = time_out;
   }else{
 	vap.pro_buserr_handler  = tool_time_out;
     	ex_vector->e_buserr     = tool_time_out;
   }

}





/******************************************************************/
/* This function restores the original bus error interrupt handler*/
/******************************************************************/
restore_sys_buserr_handler()
{
   ex_vector->e_buserr = vap.sys_buserr_handler;
}



/******************************************************************/
/*  In the event of a bus error, Time_out function is called to   */
/*  evaluate and save the bus error type in error log structure   */
/*  for further use by other function.                            */
/******************************************************************/
time_out()
{
   struct berr_reg breg;

   breg.berr_whole = (berr_size)getberrreg();
   if(!breg.berr_field.berr_pagevalid) vap.berr_pt = bus_err_type[0];
   if(breg.berr_field.berr_busmaster)  vap.berr_pt = bus_err_type[1];
   if(breg.berr_field.berr_proterr)    vap.berr_pt = bus_err_type[2];
   if(breg.berr_field.berr_timeout)    vap.berr_pt = bus_err_type[3];
   if(breg.berr_field.berr_parerru)    vap.berr_pt = bus_err_type[4];
   if(breg.berr_field.berr_parerrl)    vap.berr_pt = bus_err_type[5];
   _longjmp(bus_err);
}



/******************************************************************/
/* This routine will be used when debug mode is active. In fact   */
/* it does nothing but jumping back to the debug tool program to  */
/* try the action again.                                          */
/******************************************************************/
tool_time_out()
{
	reset_board();
	_longjmp(bus_err);
}



  extern   vme_intr1_7(), vme_intr48_63(), vme_intr64_255(),
	   vme_intr72(), vme_intrdummy();


/******************************************************************/
/* Initialize interrupt vector service routines and replace with  */
/* those of system's.                                             */
/******************************************************************/
intr_init()
{
int i;
 
	asm ("movw	#0x2100, sr");		/* lower priority of cpu */

	for(i = 0; i < 16; i++)
		ex_vector->e_res2[i] = vme_intr48_63;
	for(i = 0; i < 192; i++)
		ex_vector->e_user[i] = vme_intr64_255;
	
	/*
	 * In correct environment interrupt vector
	 * number 72 (0x48) must be addressed by the board.
	 */
	ex_vector->e_user[8] = vme_intr72;

}

 
 
 
/******************************************************************/
/* Initialize a do nothing interrupt routinne in order to not     */
/* cause the program jump out. This is only used in debug mode.   */
/******************************************************************/
tool_intr_init()
{
int	i;

        for(i = 0; i < 16; i++)
                ex_vector->e_res2[i] = vme_intrdummy;
        for(i = 0; i < 192; i++)
                ex_vector->e_user[i] = vme_intrdummy;

        /*
         * In correct environment interrupt vector
         * number 72 (0x48) must be addressed by the board.
         */
        ex_vector->e_user[8] = vme_intrdummy;
 
}




/******************************************************************/
/* In the event of interrupt, the interrupt vectors will point to */
/* one of the reentrant functions. While in reentrant function,   */
/* the interrupt level that caussed the interrupt is recorded in  */
/* structure variable vap.intr_lev                                */
/******************************************************************/
reentrant(vme_intr48_63){

     vap.intr_lev = 48;
     find_vec_num();
     _longjmp(intr_buf);
}

reentrant(vme_intr64_255){

     vap.intr_lev = 64;
     find_vec_num();
     _longjmp(intr_buf);
}

reentrant(vme_intr72){

    /*
     * The correct vector is addressed.
     */
     vap.intr_lev = 72;
     _longjmp(intr_buf);
}


reentrant(vme_intrdummy){
    
    delay(20);
    _longjmp(intr_buf);
}


/***********************************************************************/
/* find_vec_num -- returns the vector number accessed by the service   */
/* routine.                                                            */
/***********************************************************************/
find_vec_num()	
{
int     number;  	/* vector number */
u_short *stac_ptr;      /* system stack pointer */

	asm (" movl  (sp), _sys_stack_p ");   /* addr in SSP + 4 bytes */

       /*
 	* This function is called by intr service routine.
	* It runs when the cpu discovers a user-defined interrupt.
 	*
 	* Give a message saying what the vector number (also called the gp
 	* interrupt id) is.
 	*/
 

        /*
         * The sys stack pointer points to the
         * stack frame (see M68000 Programmers Ref Man).
         */
 
        stac_ptr = (u_short *)sys_stack_p;
 
        /*
         * Point to 4th word of stack frame, which holds the vector offset.
         * The vector offset is the vector pointer times 4.  The word is
         * actually like this:
         *
         *      15     11                       0
         *       -------------------------------
         *      |Format|        Vector Offset   |
         *       -------------------------------
         *
         * Format is supposed to be all zeros, but don't take a chance.
         * So first, shift left to erase the Format field, then shift
         * right to convert the Vector Offset to the Vector Number.
         */
        stac_ptr += 3;  /* point to 4th word */
          
        number = (*stac_ptr & 0x0fff) >> 2;  /* eliminate format field */
 
 
}





/******************************************************************/
/* This routine will transfer the LEN number of bytes/words to    */
/* and from memory. If the transfer done successfully it will     */
/* return 0 to caller, or -1 if something went wrong.             */
/******************************************************************/
dma_transfer(cnt, mode, direct, len)
int	cnt, mode, direct, len;
{

struct prog_dma_reg *ptpdr;
   int		done, i;
   u_char	cnt_val;

   ptpdr =	 vap.pdr;

   /*
    * Reset the board, if not successful try a couple of times.
    */
   for(i = 0; i < 5; i++){
	if(reset_board() == 0)
		break;
   }

   ptpdr->dma_count = ~len;
   ptpdr->dma_addr  = (DBUF_PA + cnt) & 0xffffff;
   ptpdr->pcr 	    = mode | direct;

  /*
   * Enable DMA
   */
   ptpdr->pcr	   |= PD_DMA_ENABLE;
   
   /*
    * the DMA count reg must be set to cnt_val at the end of
    * transfer. cnt_val will have 0, if byte mode or
    * 1, if word mode is active.
    */
   cnt_val 	    = (mode & PD_WORD_MODE) ? 0x01 : 0x00;

   /*
    * loop until the transfer to be done.
    * The control reg is checked for the value of 0x4000.
    */
   for(i = 0; i < 10000; i++){
	if(done = (ptpdr->pcr & PD_BUS_ERROR) ? 1 : 0 )
		break;
	delay(100);
   }
   if(done && (ptpdr->dma_count == cnt_val))
	return(0);				/* DMA successfull */
   
   vap.elog.code |= DMA_TIME_OUT_ERR;
   vap.elog.exp_val    = cnt_val;
   vap.elog.act_val    = ptpdr->dma_count & 0xffff;
   return(-1);
}



/********************************************************************/
/* reset_board							    */
/********************************************************************/

reset_board()
{

struct   prog_dma_reg  *ptpdr;

	ptpdr         =   vap.pdr;

	ptpdr->pcr = PD_RESET;
	delay(10);
	ptpdr->pcr = 0x00;
	if((ptpdr->pcr & 0xFFFF) != 0x1100)
		return(-1);
	return(0);
}
