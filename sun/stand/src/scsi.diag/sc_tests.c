/******************************************************************************/
/*							      		      */
/*            THIS  FILE  CONTAINT  ALL  OF  TEST MODULES.                    */
/*    Follow is a list of all functions included in this file:                */
/*      _ test_icr_reg       test interface control register.                 */
/*      _ test_DMA_cntr_reg  test dma counter register.                       */
/*      _ test_DMA_addr_reg  test dma address register.                       */
/*      _ test_init_select   test device initial selection.                   */
/*      _ test_dev_ready     test device ready.                               */
/*      _ test_bus_transfer  test bus transfer.                               */
/*      _ test_DMA_and_data_integrity  test dma transfer and data integrity.  */
/*      _ test_status_and_overrun_intr test status interrupt and dma overrun. */
/*      _ test_timer         test timer.                                      */
/******************************************************************************/
  
static	char	sccsid_test[] = "@(#)sc_tests.c 1.1 86/09/25 SMI." ;


#include    <sys/types.h>
#include    <setjmp.h>
#include    <machdep.h>
#include    <s2addrs.h>
#include    <reentrant.h>
#include    "vectors.h"
#include    "m68000.h"
#include    "sc_reg.h"




extern   struct scsi_par   scsi;        
extern   jmp_buf  get_out, bus_error, int2_buf;
long 	 swap_par_addr, nsect_per_cyl;





/**************************** START OF TEST 1 ******************************/
/*                                                                         */
/* 1_ TEST INTERFACE CONTROL REGISTER:                                     */ 
/*      This test writes test patterns from  0x00  to  0x3F into ICR and   */
/*    the results are read back and compared with the expected values.     */
/*    It also checks for time out on read and write accesses to ICR.       */
/***************************************************************************/
test_icr_reg() 
{
   int  nloop, c;
   register int  act_val, exp_val ;
   struct   scsi_ha_reg   *har;

   har           =   scsi.scsi_addr;
   if ((nloop    =   get_loop_num(1)) == 0) return(0);

   for(; nloop > 0; nloop--){

      bzero((u_short *)&scsi.elog, sizeof scsi.elog);

      for(exp_val = 0; exp_val < 0x3f; exp_val++){
         if ( !_setjmp(bus_error))
            har->icr = exp_val;  /* write into icr reg. */
         else
            scsi.elog.code |= 1;
         if ( !_setjmp(bus_error))
            act_val = (har->icr & CNTRL_MASK);  /* read from icr reg. */
         else
            scsi.elog.code |= 2;

         scsi.elog.code |= ((exp_val != act_val) ? 4 : 0 );

         if (scsi.elog.code){
            scsi.elog.act_val = act_val;
            scsi.elog.exp_val = exp_val;
            return(-1);
         }
      }   
 
      if ((c = chk_quit_resp()) != 0) return(c);
 
   }
 
   printf("  passed ");
   return(0);
}



/***************************************************************************/
/*  2_ TEST DMA COUNTER REGISTER :                                         */
/*      This test writes test patterns from  0x0000 to 0xFFFF into DMA     */
/*  counter register then the results are read back and compared with the  */
/*  expected values.                                                       */
/***************************************************************************/
test_DMA_cntr_reg()
{
   int nloop, c;
   register int  act_val, exp_val ;
   struct   scsi_ha_reg  * har;

   har           =   scsi.scsi_addr;
   if ((nloop    =   get_loop_num(1)) == 0) return(0);

   for(; nloop > 0; nloop--){

      bzero((u_short *)&scsi.elog, sizeof scsi.elog);

      for(exp_val = 0; exp_val < 0xffff; exp_val++){
         if ( !_setjmp(bus_error))
            har->dma_count = exp_val;
         else
            scsi.elog.code |= 1;
         if ( !_setjmp(bus_error))
            act_val = (har->dma_count & 0xffff);
         else
            scsi.elog.code |= 2;

         scsi.elog.code |= ((exp_val != act_val) ? 4 : 0 );

         if (scsi.elog.code){
            scsi.elog.act_val = act_val;
            scsi.elog.exp_val = exp_val;
            return(-1);
         }
 
      }  
 
      if ((c = chk_quit_resp()) != 0) return(c);
 
   }

   printf("  passed ");
   return(0);
}


/***************************************************************************/
/* 3_ TEST DMA ADDRESS REGISTER : (for VME only)                           */
/*      This test writes test patterns from  0x000000 to 0xFFFFFF into DMA */
/*    address register then  the results are read back and compared with   */
/*    the expected values.                                                 */
/***************************************************************************/
test_DMA_addr_reg()
{
   int nloop, c;
   register int  act_val, exp_val ;
   struct   scsi_ha_reg  * har;

   if (!scsi.vme ){
      printf("  skipped");
      return(0);
   }
 
   har           =   scsi.scsi_addr;
   if ((nloop    =   get_loop_num(1)) == 0) return(0);

   for(; nloop > 0; nloop--){
 
      bzero((u_short *)&scsi.elog, sizeof scsi.elog);
 
      for(exp_val = 0; exp_val < 0xffff; exp_val++){
         if ( !_setjmp(bus_error))
             har->dma_addr = exp_val;
         else
             scsi.elog.code |= 1;
         if ( !_setjmp(bus_error))
            act_val = (har->dma_addr & 0xffff);
         else
            scsi.elog.code |= 2;
 
         scsi.elog.code |= ((exp_val != act_val) ? 4 : 0 );
 
         if (scsi.elog.code){
            scsi.elog.act_val = act_val;
            scsi.elog.exp_val = exp_val;
            return(-1);
         }
      }
 
      if ((c = chk_quit_resp()) != 0) return(c);
 
   }

   printf("  passed ");
   return(0);
}


/***************************************************************************/
/* 4_ TEST DEV INITIAL SELECTION :                                         */
/*      This  test verifies device initial selection sequence  by : first  */
/* setting the ID field on data register and asserts the select bit in ICR */
/* (bit 5), then check for a busy response from selected device. Afterward */
/* it reset the SCSI bus and expects that the device negates busy signal.  */
/***************************************************************************/
test_init_select()
{
   int  nloop, c;
   struct   scsi_ha_reg  * har;

   har           =   scsi.scsi_addr;
   if ((nloop    =   get_loop_num(5)) == 0) return(0);

   if (_setjmp(bus_error)){
      scsi.elog.code |= GOT_BUS_ERR;
      return(-1);
   }

   for(; nloop > 0; nloop--){
 
      bzero((u_short *)&scsi.elog, sizeof scsi.elog);

      if ((c = dev_init_sel(har)) != 0)                                     
         return(-1);
 
      cmd_reset();  /* reset scsi bus and clear ICR */
 
      if ( (scsi.elog.code |= (( har->icr & ICR_BUSY ) ? 
         (DEV_SEL_ERR | BUSY_BIT_STK) : 0)) )
         return(-1); 
 
      if ((c = chk_quit_resp()) != 0) return(c);
 
   }       

   printf("  passed ");
   return(0);
}


/***************************************************************************/
/* 5_ TEST DEVICE READY :                                                  */
/*     This test  verifies  that  bus transfer can be done, and check the  */
/*  status of disk device for further operation.  This is done  by issuing */
/*  Test Unit Ready and Request sense commands to device. The stauts and   */
/*  sense  information  are  read back  and checked  for  normal  ending   */
/*  ccondition.                                                            */
/***************************************************************************/
test_dev_ready()
{
   int      nloop, c, chk;
   struct   scsi_ha_reg  * har;

   har           =   scsi.scsi_addr;
   if ((nloop    =   get_loop_num(10)) == 0) return(0);

   if (_setjmp(bus_error)){
      scsi.elog.code |= GOT_BUS_ERR;
      return(-1);
   }

   for (;nloop > 0 ; nloop--){

      bzero((u_short *)&scsi.elog, sizeof scsi.elog);

      set_command(SC_TEST_UNIT_READY,0,0);

      if ((c = bus_transfer(har, 0, 0)) != 0)
         return(c);

      chk = scsi.scb.chk;

      if ((c = get_sense_info(har, (char *)&scsi.sense, 16)) != 0 )
         return(c);
 
      if ((scsi.elog.code |= ((chk) ? CHK_ON_STATUS : 0)))
         return(-1);
 
      if ((c = chk_quit_resp()) != 0) return(c);
 
   }

   printf("  passed ");
   return(0);
}


/***************************************************************************/
/* 6_ TEST BUS TRANSFER :                                                  */
/*     This test verifies bus transfer by writing 1 block of test pattern  */
/*  to disk then read and compared with expected values. All data transfer */
/*  are done with program I/O.                                             */
/***************************************************************************/
test_bus_transfer()
{
   int      c, len, nloop;
   struct   scsi_ha_reg  * har;

   har           =   scsi.scsi_addr;
   if ((nloop    =   get_loop_num(1)) == 0) return(0);

   if (_setjmp(bus_error)){
      scsi.elog.code |= GOT_BUS_ERR;
      return(-1);
   }

   scsi.nblk  = 1;
   len        = 512*scsi.nblk;
   init_data((u_char *)DBUF_VA, len);

   for (;nloop > 0 ; nloop--){

      bzero((u_short *)&scsi.elog, sizeof scsi.elog);

      set_command(SC_WRITE, scsi.sblk, scsi.nblk); 
 
      if ((c = bus_transfer(har, (char *)DBUF_VA, len )) != 0)
         return(c);
 
      set_command(SC_READ, scsi.sblk, scsi.nblk); 
 
      if ((c = bus_transfer(har, (char *)(DBUF_VA + len),len)) != 0)
         return(c);

      if ((c = compare_data((u_char*)DBUF_VA,(u_char*)(DBUF_VA+len),len)) != 0) 
         return(c);

      if ((scsi.elog.code |= ((scsi.scb.chk) ? CHK_ON_STATUS : 0))){
         get_sense_info(har, (char *)&scsi.sense, 16);
         return(-1);
      }  
 
      if ((c = chk_quit_resp()) != 0) return(c);
 
   }
   printf("  passed ");
   return(0);
}


/***************************************************************************/
/* 7_ TEST DMA TRANSFER   & 10_ TEST DATA INTEGRITY :                      */
/*     This test is used twice,  first for  DMA TRANSFER  and then  DATA   */
/* INTEGRITY test. The parameters are set up diferently for each test. The */
/* gobal variable "scsi.tes_num", set by the caller, is used to distinguish*/
/* between the two tests.                                                  */
/*     TEST DMA TRANSFER verifies the DMA transfer operation. This is done */
/* by using  DMA transfer to  WRITE 1 block of initialized test data to the*/
/* target device and then  READ the same block back to compare.            */
/*     TEST DATA INTEGRITY does the same thing as bove except with random  */
/* test patterns and on 20 blocks size.                                    */
/***************************************************************************/
test_DMA_and_data_integrity()
{
   int      c, len, nloop;
   struct   scsi_ha_reg  * har;

   har           =   scsi.scsi_addr;
   if ((nloop    =   get_loop_num(2)) == 0) return(0);

   if (_setjmp(bus_error)){
      scsi.elog.code |= GOT_BUS_ERR;
      return(-1);
   }

   scsi.nblk  = ((scsi.test_num == 7) ? 1 : 20);
   len        = 512*scsi.nblk;

   if (scsi.test_num == 7) 
      init_data((u_char *)DBUF_VA, len);

   for (;nloop > 0 ; nloop--){

      if (scsi.test_num == 10)
         gen_rand_data((u_char *)DBUF_VA,len);

      bzero((u_short *)&scsi.elog, sizeof scsi.elog);

      set_command(SC_WRITE, scsi.sblk, scsi.nblk);      
   
      if ((c = dma_transfer(har, (int)DBUF_PA, len)) != 0)
         return(c);
         
      set_command(SC_READ, scsi.sblk, scsi.nblk);
         
      if ((c = dma_transfer(har, (int)(DBUF_PA+len), len)) != 0)
         return(c);
         
      if ((c = compare_data((u_char*)DBUF_VA,(u_char*)(DBUF_VA+len),len)) != 0) 
         return(c);

      if ((scsi.elog.code |= ((scsi.scb.chk) ? CHK_ON_STATUS : 0))){
         get_sense_info(har, (char *)&scsi.sense, 16);
         return(-1);
      }  
 
      if ((c = chk_quit_resp()) != 0) return(c);
 
   }
   printf("  passed ");
   return(0);
}



/***************************************************************************/
/* 8_ TEST DMA STATUS INTERRUPT  &  9_ TEST DMA OVERRUN:                   */
/*     This test is used twice,  first for  DMA  STATUS INTERRUPT and then */
/* for  DMA  OVERRUN.  The parameters are set up diferently for each test. */
/* The test number in global var scsi.tes_num , set by the caller, is used */
/* to distinguish between the two tests.                                   */
/*     Test DMA STATUS INTERRUPT  has two parts. The first part does write */
/* and read test to interrupt vector register,  this part of the test  is  */
/* executed if and only if the board  under test is a VME SCSI board.  The */
/* second part of this test verifies that request for status from  target  */
/* device causes an interrupt. This is done by enabling DMA and WORD mode, */
/* and INTERRUPT bits in Interface control register and do a READ on ONE   */
/* block (512 bytes) of data using DMA transfer and verifies that interrupt*/
/* occurred on status request.                                             */
/*     Test DMA OVERRUN  verifies that an DMA Overrun causes an interrupt. */
/* This is done by DMA transfer of 2 block (512*2 bytes) of data with the  */
/* dma counter set to the length of one block. As DMA counter countes up to*/
/* 1 block, an carry out is generated which causes an overrun interrupt to */
/* occurred.                                                               */
/***************************************************************************/
test_status_and_overrun_intr()
{
   int      c, i, len, nloop, act_val, exp_val;
   u_short  expected_count;
   char     message;
   struct   scsi_ha_reg  * har;

   har           =   scsi.scsi_addr;
   if ((nloop    =   get_loop_num(10)) == 0) return(0);

   if (_setjmp(bus_error)){
      scsi.elog.code |= GOT_BUS_ERR;
      return(-1);
   }

   scsi.nblk      = ((scsi.test_num == 8) ? 1 : 2);
   len            = ((scsi.test_num == 8) ? 512*scsi.nblk : (512*scsi.nblk)/2 );
   expected_count = ((scsi.test_num == 8) ? 0xFFFF : 1);

   for (;nloop > 0 ; nloop--){

      bzero((u_short *)&scsi.elog, sizeof scsi.elog);

      if (scsi.vme && (scsi.test_num == 8)){
         for ( exp_val  = 0 ; exp_val <= 0xff; exp_val++ ){
            har->intvec = exp_val;
            act_val     = har->intvec & 0xFF;
            if (scsi.elog.code  |= ((exp_val != act_val) ? BAD_INTR_VECT : 0)){
               scsi.elog.act_val = act_val;
               scsi.elog.exp_val = exp_val;
               return(-1);
            }
         }
      }  
 
      har->intvec = 26;
      intr_init();

      set_command(SC_READ, scsi.sblk, scsi.nblk);
   
      if ((c = dev_init_sel(har)) != 0)
         return(c);

      scsi.int_en    = 1;
      har->icr       = ICR_WORD_MODE | ICR_DMA_ENABLE | ICR_INTERRUPT_ENABLE ;
      har->dma_addr  = DBUF_PA ;
      har->dma_count = ~len;

      if ( !_setjmp(int2_buf)){ 

         if ((c=put_byte(har,ICR_COMMAND,(char*)&scsi.cdb,sizeof scsi.cdb))!=0)
            return(c);

         for (i = 1; i < 25000; i++)  DELAY(100) ;
         scsi.elog.code |= DMA_NO_INTR;
         return(-1);

      }else{     /* got an status interrupt. */

         har->icr   &= ~ICR_INTERRUPT_ENABLE ;

         if ( !scsi.int_en || (scsi.int_en && (scsi.int_lv != 2))){
            scsi.elog.code |= DMA_INTR_ERR;
            return(-1);
         }

         scsi.int_en = 0; /* disable interrupt. */

         if (har->dma_count != expected_count){
            if ( har->icr & ICR_BUS_ERROR)
               scsi.elog.code |= DMA_BUS_ERR;
            else
               scsi.elog.code |= DMA_BAD_LEN;
            scsi.elog.act_val  = len - ~har->dma_count;
            scsi.elog.exp_val  = len;
            return(-1);
         }
      }
  
      if (scsi.test_num == 8) {
  
         if ((c = get_byte(har, ICR_STATUS, (char*)&scsi.scb, 16)) != 0)
            return(c);
 
         if ((c = get_byte(har, ICR_MESSAGE_IN, (char*)&message, 1)) != 0)
            return(c);
 
         if (scsi.elog.code|=((message!=SC_COMMAND_COMPLETE)?BAD_MESS_IN:0))
            return(-1);

         if ((scsi.elog.code |= ((scsi.scb.chk) ? CHK_ON_STATUS : 0))){
            get_sense_info(har, (char *)&scsi.sense, 16);
            return(-1);
         }
  
      }else cmd_reset();  
 
      if ((c = chk_quit_resp()) != 0) return(c);
 
   }

   intr_reset();
   printf("  passed ");
   return(0);
}






/***************************************************************************/
/* Not implement yet !                                                     */
/***************************************************************************/
test_timer()
{
   printf(" ~implem ");
   return(0);
}


