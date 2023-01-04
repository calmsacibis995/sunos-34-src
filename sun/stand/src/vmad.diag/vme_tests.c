
/*****************************************************************/
/* Test routines for VME_Multibus Adapter Diagnostic		 */
/*****************************************************************/
static char     testsccsid[] = "@(#)vme_tests.c 1.1 9/25/86 Copyright Sun Micro";

#include    <sys/types.h>
#include    <setjmp.h>
#include    <machdep.h>
#include    <s2addrs.h>
#include    "vme_adap.h"


jmp_buf  get_out,  bus_err, intr_buf;
struct   vme_adap_par  vap;



u_short tst_pat[] = {
        0x0000,
        0xFFFF,
        0x3333,
        0xCCCC,
        0x5555,
        0xAAAA,
        0xF0F0,
        0x0F0F,
        0xC3C3,
        0x3C3C,
        0xA5A5,
        0x5A5A,
        0xDEAD,
};


/******************************************************************/
/* 0_ TEST TIME OUT:                                              */
/* This test checks for address lines by writing/reading the      */
/* data reg. The data is not checked for reliability.             */
/* Also, it addresses an address that is not in the range of      */
/* the board to check if time out being happened. Otherwise       */
/* the error will be reported.			                  */
/******************************************************************/
test_time_out()
{

   int nloop, c, val = 0x5A5A ;
   struct   prog_dma_reg  *ptpdr;


   ptpdr         =   vap.pdr;
   if ((nloop    =   get_loop_num()) == 0)
	 return(0);

   for(; nloop > 0; nloop--){
 
      bzero((u_short *)&vap.elog, sizeof vap.elog);
 
      if ( !_setjmp(bus_err))
         ptpdr->data = (val & 0xFFFF);
      else{
         vap.elog.code |= BUS_ERROR;
	 return(-1);
      }

      if ( !_setjmp(bus_err))
         val = (ptpdr->data & 0xffff);
      else{
         vap.elog.code |= BUS_ERROR ;
	 return(-1);
      }
      
/*
      if ( !_setjmp(bus_err)){
         val = ptpdr->ivr;
printf("%x", val);
getchar();
	 delay(100);
         vap.elog.code |= NO_TIME_OUT_ERR;
      }

*/
      if(vap.elog.code)
	  return(-1);

      if((c = chk_usr_req()) != 0)
	 return(c);

   }

   if(!vap.all)
   	prt_passed_msg();
   return(0);
}




/******************************************************************/
/* 1_ TEST DATA PATH:                                             */
/* This test is done by writing bit patterns to the data and      */
/* counter reg and reading them back for comparing. All 16 bits   */ 
/* are tested for the correctness.                                */
/******************************************************************/
test_data_path()
{
   int nloop, c, i;
   register int  cnt_val, data_val;
   struct   prog_dma_reg  *ptpdr;
 
   ptpdr           =   vap.pdr;
   if ((nloop    =   get_loop_num()) == 0)
	  return(0);

   for(; nloop > 0; nloop--){
 
      bzero((u_short *)&vap.elog, sizeof vap.elog);
 
      for(i = 0; tst_pat[i+1] != 0xDEAD; i++){

       /*
	* write the patterns to data and dma_count regs.
	*/
         if ( !_setjmp(bus_err)){
            ptpdr->dma_count = tst_pat[i];
	    ptpdr->data      = tst_pat[i];
         }else{
            vap.elog.code |=  BUS_ERROR;
	    vap.elog.wr_flag = 0;
	    return(-1);
	 }

       /*
	* read the written patterns from data and dma_count regs.
	*/
         if ( !_setjmp(bus_err)){
            cnt_val  = (ptpdr->dma_count & 0xffff);
	    data_val = (ptpdr->data & 0xffff);
         }else{
            vap.elog.code |=  BUS_ERROR;
	    vap.elog.wr_flag = 1;
	    return(-1);
	 }

       /*
	* compare the read and written patterns, if no match
	* set the error flag.
	*/
	if(tst_pat[i] != cnt_val){
		vap.elog.code |= DATA_MIS_COMPARE;
		vap.elog.reg_in_err = 2;
	}
	if(tst_pat[i] != data_val){
		vap.elog.code |= DATA_MIS_COMPARE;
		vap.elog.reg_in_err = 0;
	}

         if (vap.elog.code){
            vap.elog.act_val = (tst_pat[i] != cnt_val) ? 
				cnt_val : data_val;
            vap.elog.exp_val = tst_pat[i];
            return(-1);
         }

      }   

      if((c = chk_usr_req()) != 0)
	 return(c);

   }
   if(!vap.all)
   	prt_passed_msg();
   return(0);
}




/*******************************************************************/
/* 2_ TEST CONTROL REGISTER :                                      */
/* This test writes bits in control reg and reads back to compare  */
/* to see if right thing happens. Also resets the board and checks */
/* it to see if it reads as 0x1100 or not.                         */
/*******************************************************************/

#define	exp_mode	PD_WORD_MODE | PD_DIRECTION | PD_COUNT_MODE

test_control_reg()
{
   int 	nloop, c, i;
   u_short	act_val, sav_mode;
   struct   prog_dma_reg  *ptpdr;

   ptpdr           =   vap.pdr;
   if ((nloop    =   get_loop_num()) == 0)
	 return(0);


   for(; nloop > 0; nloop--){
 
      	bzero((u_short *)&vap.elog, sizeof vap.elog);

	if( !_setjmp(bus_err)){
		sav_mode = ptpdr->pcr & 0x402f;   /* cares only b0-b5, b14 */
		ptpdr->pcr |= exp_mode & 0x402f;
	} else{
		vap.elog.code |= BUS_ERROR;
		return(-1);
	}

	if( !_setjmp(bus_err))
		act_val = ptpdr->pcr & 0x402f;
	else{
		vap.elog.code |= BUS_ERROR;
		return(-1);
	}

	if((act_val & 0x402f) != ((exp_mode | sav_mode) & 0x402f))
		vap.elog.code |= DATA_MIS_COMPARE;

	if(vap.elog.code){
		vap.elog.act_val = act_val;
		vap.elog.exp_val = exp_mode;
		return(-1);
	}

	/*
	 * "reset" function returns 0, if reset was successful.
	 */
	if(reset_board() != 0){
		vap.elog.code |= NO_RESET_ERR;
		vap.elog.act_val = ptpdr->pcr & 0xFFFF;
		vap.elog.exp_val = 0x1100;
		return(-1);
	}

   if ((c = chk_usr_req()) != 0)
	 return(c);

   }
   if(!vap.all)
   	prt_passed_msg();
   return(0);
}




/******************************************************************/
/* 3_ TEST INTERRUPT:                                             */
/* This test checks if the interrupt mechanisim works or not and  */
/* whether the correct interrupt vector is addressed or not. In   */
/* correct case the interrupt vector # 72 must be addressed.      */
/******************************************************************/
test_interrupt()

{
   int 	nloop, c, i;
   register int  act_val, exp_val ;
   struct   prog_dma_reg  *ptpdr;


   ptpdr         =   vap.pdr;

  /*
   * Initialize the intrrupt vector table to the user defined
   * intr service routine.
   */
   intr_init();

   if ((nloop    =   get_loop_num()) == 0)
	 return(0);
   reset_board();

   for(; nloop > 0; nloop--){
 
      	bzero((u_short *)&vap.elog, sizeof vap.elog);

	if(!_setjmp(intr_buf)){
		asm ("movw      #0x2100, sr");	/* lower priority of cpu */
		ptpdr->pcr = 0x01;
		delay(100);
		vap.elog.code |= NO_INTRUP_ERR;
		vap.elog.act_val = ptpdr->pcr;
		return(-1);
	}else 
	     	if(vap.intr_lev != 72){
			vap.elog.code |= BAD_INTRUP_ERR;
			vap.elog.act_val = vap.intr_lev;
			vap.elog.exp_val = 72;
			return(-1);
		}	

	if ((c = chk_usr_req()) != 0){
		ptpdr->pcr &= 0xfffe;
         	return(c);
	}
   }

   if(!vap.all)
   	prt_passed_msg();
   ptpdr->pcr &= 0xfffe;
   return(0);
}




/******************************************************************/
/* 4_ TEST BYTE DMA:                                              */
/* This routine was designed to check dma circuitry. It does dma  */
/* 11  bytes of all 0's  one byte at a time which already set     */
/* to all 0xff's. It will check for validity of transfered and    */
/* also checks for correct byte addressing by examining the bytes */
/* just before and after the transaction byte. Then if the test   */
/* was successful it transfers 256 bytes of all 1's all by one    */
/* dma transfer. In case of error, you can try different patterns */
/* by using the tool program.					  */
/******************************************************************/
test_byte_mode()
{
   u_char	 *pt;
   int	 nloop, i, c, e, mode, direct, len;
   struct   prog_dma_reg  *ptpdr;


   ptpdr           =   vap.pdr;
   if ((nloop    =   get_loop_num()) == 0) 
	return(0);
   
   for(; nloop > 0; nloop--){
 
      	bzero((u_short *)&vap.elog, sizeof vap.elog);

 	for(i = 0, pt = DBUF_VA; i < 11; i++, pt++)
		*pt = 0xff;
	
	len = 1; 
	direct = 0x00;  	/* write to cup memory */
	mode   = 0x00;		/* byte mode	       */
	ptpdr->data = (0 << 8);
	pt = DBUF_VA;

	for(i = 0, pt = DBUF_VA; i < 11; i++, pt++){
	   if(e = dma_transfer(i, mode, direct, len) != 0)
 		return(-1);

	/*
 	 * Check if the byte transfered has the right value in mem.
	 * Check the byte before and the byte after the hero byte
	 * to not affected by the transfered byte or not?
	 */
	   if(i > 0 && i < 10){
	        if(*pt != 0x00 || *(pt - 1) != 0xff || *(pt + 1) != 0xff){
			vap.elog.code = DMA_BYTE_ADDR_ERR;
			vap.elog.exp_val = (*pt) ? 0x00 : 0xff;
			vap.elog.act_val = (*pt) ? *pt :
					 ( *(pt -1) ? *(pt + 1) : *(pt -1) );
			return(-1);
	        }else
			*pt = 0xff;
	    }else
		*pt = 0xff;
	if(e != 0)
		return(-1);
	}
	
	/*
	 * Dma 256 bytes this time.
	 */
	direct = PD_DIRECTION;		/* read from cpu memory */
	len = 256;

	/*
        * begin dma
        */
        if(e = dma_transfer(0, mode, direct, len) != 0)
                return(-1);

   	if(c = chk_usr_req() != 0)
		return(c);
   }
   if(!vap.all)
   	prt_passed_msg();
   return(0);
}





/******************************************************************/
/* 5_ TEST WORD DMA:                                              */
/* this test will check the DMA circuitry in word mode by writing */
/* 256 word of all 0's to the area of memory that has been set to */
/* all 1's. All 256 word will be transferred at the same time.    */
/* Then the it will check for the value other than 0's. If the    */
/* test fails, the tool program will try different patterns other */
/* than all 0's or all 1's.                                       */
/******************************************************************/
test_word_mode()
{
   u_char	 *pt;
   int	 nloop, i, e, c, mode, direct, len;
   struct   prog_dma_reg  * ptpdr;


   ptpdr           =   vap.pdr;
   if ((nloop    =   get_loop_num()) == 0) 
	return(0);


   for(; nloop > 0; nloop--){
 
      	bzero((u_short *)&vap.elog, sizeof vap.elog);

	for(i = 0, pt = DBUF_VA; i < 512; i++, pt++)
                *pt = 0xff;

	len = 512; 		/* 256 words to transfer */
	direct = 0x00;
	mode = PD_WORD_MODE;
	ptpdr->data = 0x00 & 0xffff;
	
       /*
	* begin dma
	*/
	if(e = dma_transfer(0, mode, direct, len) != 0)
 		return(-1);

       /*
	* check the buffer for accurate data, if any miss match
	* set the error code.
	*/
	for(i = 0, pt = DBUF_VA; i < 512; i += 2, pt += 2){
		if(*pt != 0x00 && *(pt + 1) != 0x00){
			vap.elog.code = DMA_WORD_ADDR_ERR;
			vap.elog.exp_val = 0x00;
			vap.elog.act_val = (*pt << 8) | *(pt + 1);
			return(-1);
	        }
	    }

	if(e != 0)
		return(-1);
	
   	if(c = chk_usr_req() != 0)
		return(c);
   }

   if(!vap.all)
   	prt_passed_msg();
   return(0);
}

