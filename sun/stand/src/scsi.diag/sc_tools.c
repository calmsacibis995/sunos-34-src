/******************************************************************************/
/*  Follow is a list of all functions include is this file :                  */
/*    _ getnuse_debug_tools   get and use debug tool.                         */
/*    _ prt_set_cmd      print command menu, get reply, and set command.      */
/*    _ prt_get_option   print debug menu and get option.                     */
/*    _ reset_prt_berr   reset scsi bus and print bus error.                  */
/*    _ step_prompt      prompt the user of stepping procedure.               */
/*    _ t_dev_sel        device select function for debug tool.               */
/*    _ t_cmd_xfer       command transfer function for debug tool.            */
/*    _ t_data_xfer      data transfer function for debug tool.               */
/*    _ t_status_xfer    status transfer function for debug tool.             */
/*    _ t_mess_xfer      message transfer function for debug tool.            */
/*    _ t_wait_intr      wait on interrupt request function for debug tool.   */
/*    _ tool_rw_screg    debug tool for read/write to scsi's register.        */
/*    _ tool_dev_sel     debug tool for device initial selection sequence.    */
/*    _ tool_bus_transfer     debug tool for bus transfer.                    */
/*    _ tool_dma_transfer     debug tool for dma transfer.                    */
/*    _ tool_dma_intr    debug tool for dma interrupt.                        */
/*    _ tool_scsi_timer  debug tool for scsi's timer.                         */
/*    _ wr_sc_reg        write/read to scsi's register for rd/wrt debug tool. */
/******************************************************************************/
static  char    sccsid_tl[] = "@(#)sc_tools.c 1.1 86/09/25 SMI." ;

#include    <sys/types.h>
#include    <setjmp.h>
#include    <s2addrs.h>	
#include    <machdep.h>	
#include    "sc_reg.h"


extern jmp_buf get_out, bus_error, int2_buf;

extern struct  scsi_par scsi;

int   tool_rw_screg(), tool_dev_sel(), tool_bus_transfer(), tool_dma_transfer(),
      tool_dma_intr(), tool_scsi_timer(), tool_scc(); 
long  swap_par_addr, nsect_per_cyl;

struct scsi_tools {

     char   *name;
     int    (*tool_ptr)();

}    tools[] = {
     "INTERFACE CONTROL REGISTER",   tool_rw_screg,
     "DMA COUNTER REGISTER",         tool_rw_screg,
     "DMA ADDRESS REGISTER",         tool_rw_screg,
     "INITIAL SELECTION SEQUENCE",   tool_dev_sel,
     "DEVICE READY",                 tool_bus_transfer,
     "BUS TRANSFER",                 tool_bus_transfer,
     "DMA TRANSFER",                 tool_dma_transfer,
     "DMA STATUS INTERRUPT",         tool_dma_intr,
     "DMA OVERRUN",                  tool_dma_intr,
     "DATA INTEGRITY",               tool_dma_transfer,
     "SCSI TIMER",                   tool_scsi_timer,
     "SCC",                          tool_scc,
};


char *menu_opt1[] = {
     " alter test pattern.",
     " write once.",
     " read once.",
     " loop on write.",
     " loop on read.",
     " loop on write/read.",
     " pop up one menu.",
     0,
};

char *menu_opt2[] = {
     " single stepping.",
     " run one pass.",
     " looping.",
     " pop up one menu.",
     0,
};
 
 
char *menu_opt3[] = {
     " change command.",
     " single stepping.",
     " run one pass.",
     " looping.",
     " pop up one menu.",
     0,
};
 

char *menu_opt4[] = {
     " change command.",
     " single stepping.",
     " run one pass.",
     " looping.",
     " pop up one menu.",
     " get intr vect debug option.",
     0,  
};

 
char *menu_cmd[] = {
     " Test Unit Ready.",
     " Request Sense.",
     " Write.",
     " Read.",
     " Pop up one menu.",
     0,
};

 
 
 
 
/******************************************************************/
/* getnuse_debug_tools function uses the test_num, stored in scsi */
/* struct, to calculates the pointer to the correspoding tool for */
/* the failing test.                                              */ 
/******************************************************************/
getnuse_debug_tools()
{
   struct  scsi_tools *tp;

   tp = &tools[(int)(scsi.test_num - 1)];
   printf("\n\n  DEBUG TOOL FOR %s.", tp->name);
   (*tp->tool_ptr)();
}






/******************************************************************/
/* prt_set_cmd function prints the command menu and sets command  */
/* description block to whatever to user choose. following is the */
/* list of all command options available :                        */
/*      _ test unit ready.                                        */
/*      _ request sense.                                          */
/*      _ write.                                                  */
/*      _ read.                                                   */
/******************************************************************/
prt_set_cmd()
{
   int i, num;
   char  buf[LINEBUFSZ];

   for (num = 0; num <= 0; ){
      printf("\n\nCOMMAND MENU :");
      for(i=0; menu_cmd[i] != 0; ++i)
         printf("\n  %d _ %s", i+1, menu_cmd[i]);
      printf("\n  Select one  ");
      gets(buf);
      num = (chk_reply(buf,i));
   }

   if (num == 5) return;

   scsi.nblk = ((num <= 2) ? 0 :
                (pgetn("\n Enter the number of block to transfer   ")));

   switch(num){
      case 1 : set_command(SC_TEST_UNIT_READY, 0, 0);
               break;

      case 2 : set_command(SC_REQUEST_SENSE, 0, 0);
               break;

      case 3 : set_command(SC_WRITE, scsi.sblk, scsi.nblk);
               break;
      case 4 : set_command(SC_READ, scsi.sblk, scsi.nblk);
               break;
   }
   prt_cmd("\n  Command is set to 0x%x  ");
   printf("\n  # of blocks =  %d ", scsi.nblk);
   printf("\n  # of bytes  =  %d ", 512*scsi.nblk);
}







/******************************************************************/
/* This function prints tool menu then gets and checks the reply  */
/* from the user. If the reply is a number with the range of the  */
/* menu, it will be return to the calling functon.                */
/* Following is the arguements list :                             */
/*     _m_ptr    =  pointer to the menu.                          */
/*     _s        =  pointer to the menu's header string.          */
/*     _num      =  number enter by user.                         */
/******************************************************************/
prt_get_option(m_ptr, s1, s2)
   char **m_ptr, *s1, *s2;
{
   int i, num;
   char  buf[LINEBUFSZ];

   for (num = 0; num <= 0 ; ){
      printf("\n\n%s", s1); 
      printf("%s:", s2);
      for(i=0; m_ptr[i] != 0; i++)
         printf("\n  %d _ %s", i+1, m_ptr[i]);
      printf("\n  Select one  ");
      gets(buf);
      num = (chk_reply(buf,i));
   }
   return(num);
}





/******************************************************************/
/* reset_prt_berr function is called when a bus error or timeout  */
/* happened. It resets the scsi bus and prints error message if   */
/* error print flag in on.                                        */
/******************************************************************/
reset_prt_berr(har, pflg)
   int     pflg;
   struct  scsi_ha_reg   *har;
{
 
   har->icr = ICR_RESET;
   har->icr = 0;
 
   if (pflg) prt_got_bus_err();
}





/******************************************************************/
/* step_prompt function prompts the user that single stepping is  */
/* done by hitting any key to increment to next step if promp     */
/* flag is on, then stays in a loop waiting for user to strike a  */
/* key.                                                           */
/******************************************************************/
step_prompt(prompt_flg)
   int  prompt_flg;
{

   if (prompt_flg){
      printf("\n  STEPPING THROUGH %s.", tools[(int)(scsi.test_num - 1)].name );
      printf("\n  Hit any key to increment the steps.");
   }
   while ( maygetchar() == -1 )
               ;
}





/******************************************************************/
/* t_dev_sel function does device selection and returns a value of*/
/* 0 or -1 depending on if the select operation is successfull or */
/* not, respectively.                                             */
/*   Parameter list :                                             */
/*     _ har      pointer to scsi's registers.                    */
/*     _ step     single stepping mode. Display progress and wait */
/*                for user to stricks a key.                      */
/*     _ run1     run once mode. display error messages upon error*/
/*                detection.                                      */ 
/******************************************************************/
t_dev_sel(har, step, run1)
   struct  scsi_ha_reg   *har;
   int     step, run1;
{
   int     c;

   if ((c = dev_init_sel(har)) == 0){
      if (step){
         printf("\n\n  Device select is completed."); 
         step_prompt(0);
      }	
      return(0);
   }

   if (step || run1) prt_dev_sel_err();
   return(-1);
}






/******************************************************************/
/* t_cmd_xfer function  does the command transfer to target device*/
/* and returns a value of 0 or -1 depending on if the command xfer*/
/* operation is successfull or not, respectively.                 */
/*   Parameter list :                                             */
/*     _ har      pointer to scsi's registers.                    */
/*     _ step     single stepping mode. Display progress and wait */
/*                for user to stricks a key.                      */
/*     _ run1     run once mode. display error messages upon error*/
/*                detection.                                      */ 
/******************************************************************/
t_cmd_xfer(har, step, run1)
   struct  scsi_ha_reg   *har;
   int     step, run1;
{
   int     c;

   if ((c = put_byte(har, ICR_COMMAND, (char*)&scsi.cdb, sizeof scsi.cdb))==0){
      if (step){
         printf("\n\n  Command transfer to device is completed .");
         step_prompt(0);
      }  
      return(0);
   }

   if (step || run1) prt_get_put_err();
   return(-1);
}




/******************************************************************/
/* t_data_xfer does data transfer to or from device  and returns  */
/* an 0 or -1 depending on if the data transfer operation is suc- */
/* cessfull or not, respectively.                                 */
/*   Parameter list :                                             */
/*     _ har      pointer to scsi's registers.                    */
/*     _ step     single stepping mode. Display progress and wait */
/*                for user to stricks a key.                      */
/*     _ run1     run once mode. display error messages upon error*/
/*                detection.                                      */
/******************************************************************/
t_data_xfer(har, step, run1)
   struct  scsi_ha_reg   *har;
   int     step, run1;
{
   int     c;

   if (scsi.cdb.cmd == SC_WRITE)
      c = put_byte(har, 0,(char*)DBUF_VA,512*scsi.nblk);

   if (scsi.cdb.cmd == SC_READ)
      c = get_byte(har, ICR_INPUT_OUTPUT,
                  (char*)(DBUF_VA+(512*scsi.nblk)),512*scsi.nblk);

   if (scsi.cdb.cmd == SC_REQUEST_SENSE)
      c = put_byte(har, ICR_INPUT_OUTPUT,(char*)&scsi.sense, 16);
 
   if ( c == 0 ){
     if (step){
         printf("\n\n  %s transfer is completed.", 
               (scsi.cdb.cmd == SC_REQUEST_SENSE) ? "Sense" : "Data");
         step_prompt(0);
      }  
      return(0);
   }

   if (step || run1) prt_get_put_err();
   return(-1);
}





/******************************************************************/
/*  t_status_xfer does status transfer and returns a value of 0 or*/
/* -1 depending on if the status tranfer operation is successfully*/
/* completed or not, respectively.                                */ 
/*   Parameter list :                                             */
/*     _ har      pointer to scsi's registers.                    */
/*     _ step     single stepping mode. Display progress and wait */
/*                for user to stricks a key.                      */
/*     _ run1     run once mode. display error messages upon error*/
/*                detection.                                      */ 
/******************************************************************/
t_status_xfer(har, step, run1)
   struct  scsi_ha_reg   *har;
   int     step, run1;
{
   int     c;

   if ((c = get_byte(har, ICR_STATUS, (char *)&scsi.scb, sizeof scsi.scb))==0){
      if (step){
         printf("\n\n  Status information is read back.");
         if (scsi.scb.chk)
            printf(" Check bit is on.");
         step_prompt(0);
      }  
      return(0);
   }

   if (step || run1) prt_get_put_err();
   return(-1);
}





/******************************************************************/
/*  t_mess_xfer does message transfer and returns a value of 0 or */
/* -1 depending on if the message tranfer operation is success-   */
/* fully completed or not, respectively.                          */
/*   Parameter list :                                             */
/*     _ har      pointer to scsi's registers.                    */
/*     _ step     single stepping mode. Display progress and wait */
/*                for user to stricks a key.                      */
/*     _ run1     run once mode. display error messages upon error*/
/*                detection.                                      */ 
/******************************************************************/
t_mess_xfer(har, step, run1)
   struct  scsi_ha_reg   *har;
   int     step, run1;
{
   int     c, message;

   if ((c = get_byte(har, ICR_MESSAGE_IN, (char*)&message, 1)) == 0){
      if (step){
         printf("\n\n  Message read back  = 0x%x",(message & 0xFF));
         step_prompt(0);
      }  
      return(0);
   }

   if (step || run1) prt_get_put_err();
   return(-1);
}






/******************************************************************/
/* t_wait_intr function waits for interrupt request from target   */
/* at completion of dma transfer. It return a value of 0 or -1 to */
/* the caller depending on if receives interrupt request or if the*/ 
/* dma counter has incorrect value at the end of dma transfer.    */
/*   Parameter list :                                             */
/*     _ har      pointer to scsi's registers.                    */
/*     _ step     single stepping mode. Display progress and wait */
/*                for user to stricks a key.                      */
/*     _ run1     run once mode. display error messages upon error*/
/*                detection.                                      */ 
/*     _ len      byte count.                                     */
/******************************************************************/
t_wait_intr(har, step, run1,len) 
   struct  scsi_ha_reg   *har;
   int     step, run1, len;
{
   int  c;

   if ((c = wait_cond(har, ICR_INTERRUPT_REQUEST)) == 0){
      if (step){
         printf("\n\n  Got interrupt request from device at");
         printf(" commpletion \n  of DMA transfer.");
         step_prompt(0);
      }
      return(0);
   }

   if (step || run1){
      scsi.elog.act_val = len - ~har->dma_count;
      scsi.elog.exp_val = len;
      if (scsi.elog.code == COND_NOT_MET) prt_no_intr_err();
      if (scsi.elog.code == COND_BUS_ERR) prt_dma_bus_err();
   }  
   return(-1);
}








/******************************************************************/
/* Tool_rw_screg is a debug tool for reading and writing to the   */
/* the following scsi's register : ICR, DMA counter, DMA address, */
/* and Interrupt vector register. Which register and it mask value*/
/* is determine using test_number.                                */
/* The DEBUG TOOL MENU is first print out which allow the user to */
/* select his/her option. If a selected option is other then one  */
/* or seven (which allows the user to alter test pattern or exit  */
/* debug tool), it will call wr_sc_reg function which does the    */
/* read and write to the register. The set of parameters passes   */
/* to wr_sc_reg function is determine using the option choose.    */
/******************************************************************/
tool_rw_screg()
{
   int  act_val, exp_val, num, mask, reg;
   char *menu_title, *reg_name;
   struct    scsi_ha_reg  *har;


   har    =  scsi.scsi_addr;
   reg   =  scsi.test_num;
   mask  =  ((reg == 1) ? 0x3F : 
            ((reg == 2) ? 0xFFFF : 
            ((reg == 8) ? 0xFF :
            (((reg == 3) && scsi.vme) ? 0xFFFFFF : 0xFFFFF ))));

   menu_title = ((reg == 1) ? "INTERFACE CONTROL REG READ/WRITE OPTION" :
                ((reg == 2) ? "DMA COUNTER REG READ/WRITE OPTION"       :
                ((reg == 3) ? "DMA ADDRESS REG READ/WRITE OPTION"       :
                              "INTERRUPT VECTOR REG READ/WRITE OPTION")));

   reg_name   = ((reg == 1) ? "Interface Control Register"  :
                ((reg == 2) ? "DMA Counter Register"        :
                ((reg == 3) ? "DMA Address Register"        :
                              "Interrupt Vector Regiser")));

   printf("\n  Test patterns are mask with hex value 0x%x.", mask);
   exp_val = (int)(scsi.elog.exp_val & mask);
   printf("\n  Failing test pattern = 0x%x", exp_val);

   while (1) {

      num = prt_get_option(menu_opt1, menu_title, " ");
        
      switch (num){
         case 1 : exp_val = ((pgetnh("  Enter test pattern in hex = 0x"))&mask);
                  printf("\n  expect data is set to 0x%x", exp_val);
                  break;
         case 2 : wr_sc_reg(har, reg, mask, exp_val, 0, 1, reg_name);
                  break;
         case 3 : wr_sc_reg(har, reg, mask, exp_val, 0, 2, reg_name);
                  break;
         case 4 : wr_sc_reg(har, reg, mask, exp_val, 1, 1, reg_name);
                  break;
         case 5 : wr_sc_reg(har, reg, mask, exp_val, 1, 2, reg_name);  
                  break;
         case 6 : wr_sc_reg(har, reg, mask, exp_val, 1, 3, reg_name);
                  break;
         case 7 : return;
      }  
   } 
}




/******************************************************************/
/*  device select tools provides three option to the user.  The   */
/* first option is single stepping. This is done by allowing the  */
/* user to go throught device select sequence step by step and    */
/* watch its progress. The second option is run once which go thru*/
/* the whole process once. When in single step or run once mode,  */
/* error messages are printed upon error detection. The third     */
/* option is loop which allows the user to continuously loop thru */
/* device select process until a control 'c' is hit.              */
/******************************************************************/
tool_dev_sel()
{
   int     i, icr, id, num, step, run1, loop;
   struct  scsi_ha_reg   *har;
 
   har    = scsi.scsi_addr;
   id     = (1 << scsi.target) | SCSI_HOST_ADDR ; 
   while (1){

      num = prt_get_option(menu_opt2, "DEVICE SELECT DEBUG OPTION", " "); 

      if (num == 4)
         return;

      step = (num == 1) ?  1 : 0;
      run1 = (num == 2) ?  1 : 0;
      loop = (num == 3) ?  1 : 0;

      if (loop)
         printf("\n  Hit control 'c' to get out of loop");

      for (;;){
         start :          
 
         if (_setjmp(bus_error))
            reset_prt_berr(har, step || run1);

         if ( loop && (maygetchar() == ('c' & 037)))  break; /* for loop */
 
         if (step) step_prompt(1);
 
         har->data = id; 
         if (step){
            printf("\n\n  Data reg is set to 0x%x", id);
            step_prompt(0);
         }

         har->icr = ICR_SELECT;

         if (step){
            printf("\n\n  Select bit ( bit 5 of ICR ) is asserted.");
            step_prompt(0);
         }

         for (i = 1; i < 20000; i ++){
            DELAY(100);
            if ((icr = har->icr) & ICR_BUSY) break;
         }
        
         if ((icr & ICR_BUSY) && step){
            printf("\n\n  Got busy bit ( bit 6 of ICR ) set by device.");
            step_prompt(0);
         }
        
         if (!(icr & ICR_BUSY)){
            if (!loop){
               prt_icr_bit("\n\n  No BUSY from device. ICR = 0x%x  ",icr);
               break; /* break the for loop. */
            }else
               goto start;
         }

         har->icr = ICR_RESET;
         har->icr = 0;
         icr = har->icr;
         if (step){
            prt_icr_bit("\n\n  Just reset and clear ICR.  ICR = 0x%x", icr);
            step_prompt(0);
         }

         if ((!loop) && (icr & ICR_BUSY))
            printf("\n\n  BUSY bit is stuck.  ICR  =  0x%x", icr);

         if (!loop){
            printf("\n  DONE.");
            break; /* break the for loop. */
         }
      }         
   }
}





/******************************************************************/
/* tool_bus_transfer is a debug tool for bus transfer operation.  */
/* The default command is what ever in the command description    */
/* when the error occurred. Upon entering, a DEBUG TOOL MENU is   */
/* display to allows the user to choose his/her option. Follow is */
/* the list of all available options and how they're being use:   */
/*   _ change command   allows the user to change the command in  */
/*                      command description block and execute it  */
/*                      if he/she does not want to use the default*/
/*                      command.                                  */
/*   _ single stepping  this allows the user to single stepping   */
/*                      throught bus transfer operation and watchs*/
/*                      its progress. Error messages will be dis- */
/*                      played upon error dectection.             */
/*   _ run once         allows the user to go throught the whole  */
/*                      bus transfer operation once without having*/
/*                      to display its process. Error messages    */
/*                      will be displayed upon error dectection.  */
/*   _ loop             allows the user to loop throught bus xfer */
/*                      operation indefinitely until a loop term- */
/*                      inator is entered ( control 'c'). No error*/
/*                      message will be displayed reguardless of  */
/*                      error detection or not.                   */
/******************************************************************/
tool_bus_transfer()
{
   int     c, i, num, step, run1, loop; 
   struct  scsi_ha_reg   *har;
 
   har    = scsi.scsi_addr;
   i      = (int)(scsi.test_num - 1);
   prt_cmd("\n  Default command is set to  0x%x ");

   while (1){
      for (num = 0; num <= 1; ){
         num = prt_get_option(menu_opt3, tools[i].name, " DEBUG OPTION ");
         if (num == 1)  prt_set_cmd();
      }  

      if (num == 5)
         return;

      step    = ((num == 2) ?  1 : 0);
      run1    = ((num == 3) ?  1 : 0);
      loop    = ((num == 4) ?  1 : 0);


      if (loop)
         printf("\n  Hit control 'c' to get out of loop");

      for (;;){

         start :
            
         if ( _setjmp(bus_error))
            reset_prt_berr(har, step || run1); 
 
         if ( loop && (maygetchar() == ('c' & 037))) break; /* for loop*/
 
         bzero((u_short *)&scsi.elog, sizeof scsi.elog); 

         if (step)  step_prompt(1);

         if ((c = t_dev_sel(har, step, run1)) != 0){
            if (!loop) break; else goto start;  
         }

         har->icr &= ~ICR_SELECT;

         if ((c = t_cmd_xfer(har, step, run1)) != 0){
            if (!loop) break; else goto start;  
         }

         if ((c = t_data_xfer(har, step, run1)) != 0){
            if (!loop) break; else goto start;
         }

         if ((c = t_status_xfer(har, step, run1)) != 0){
            if (!loop) break; else goto start;  
         }

         if ((c = t_mess_xfer(har, step, run1)) != 0){
            if (!loop) break; else goto start;  
         }

         if (!loop){
            printf("\n  DONE.");
            break; /* break the for loop. */
         }
      }   
   }
}





/******************************************************************/
/* tool_dma_transfer is a debug tool for dma transfer operation.  */
/* The default command is what ever in the command description    */
/* when the error occurred.  Follow is the list of all available  */
/* options :                                                      */
/*   _ change command   allows the user to change the command.    */
/*   _ single stepping  this allows the user to single stepping   */
/*                      throught dma transfer operation.          */
/*   _ run once         allows the user to go throught the whole  */
/*                      dma transfer operation once without having*/
/*                      to display its process.                   */
/*   _ loop             allows the user to loop throught dma xfer */
/*                      operation indefinitely until a loop term- */
/*                      inator is entered ( control 'c'). No error*/
/*                      message will be displayed reguardless of  */
/*                      error detection or not.                   */
/******************************************************************/
tool_dma_transfer()
{
   int     c, i, num, len, step, run1, loop; 
   struct  scsi_ha_reg   *har;
 
   har    = scsi.scsi_addr;
   i      = (int)(scsi.test_num - 1);
   prt_cmd("\n  Default command is set to  0x%x ");

   while (1){
      for (num = 0; num <= 1; ){
         num = prt_get_option(menu_opt3, tools[i].name, " DEBUG OPTION ");
         if (num == 1)  prt_set_cmd();
      }  

      if (num == 5) return;

      step    = ((num == 2) ?  1 : 0);
      run1    = ((num == 3) ?  1 : 0);
      loop    = ((num == 4) ?  1 : 0);
      len     = 512*scsi.nblk;

      if (loop) printf("\n  Hit control 'c' to get out of loop");

      for (;;){

         start :
            
         if ( _setjmp(bus_error))
            reset_prt_berr(har, step || run1); 
 
         if ( loop && (maygetchar() == ('c' & 037))) break; /* for loop*/
 
         bzero((u_short *)&scsi.elog, sizeof scsi.elog); 

         if (step)  step_prompt(1);

         if ((c = t_dev_sel(har, step, run1)) != 0){
            if (!loop) break; else goto start;  
         }

         har->dma_addr  = DBUF_PA ;
         har->dma_count = ~len & 0xFFFF;
         har->icr       = ICR_WORD_MODE | ICR_DMA_ENABLE;

         if ((c = t_cmd_xfer(har, step, run1)) != 0){
            if (!loop) break; else goto start;  
         }

         if ((c = t_wait_intr(har, step, run1,len)) != 0){ 
            if (!loop) break; else goto start;  
         }


         if ((c = t_status_xfer(har, step, run1)) != 0){
            if (!loop) break; else goto start;  
         }

         if ((c = t_mess_xfer(har, step, run1)) != 0){
            if (!loop) break; else goto start;  
         }

         if (!loop){
            printf("\n  DONE.");
            break; /* break the for loop. */
         }
      }  
   }
}






/******************************************************************/
/* tool_dma_intr is a debug tool for dma transfer operation with  */
/* interrupt enable. The default command is what ever in the CDB  */
/* when the error occurred.  Follow is the list of all available  */
/* options :                                                      */
/*   _ change command   allows the user to change the command.    */
/*   _ single stepping  this allows the user to single stepping   */
/*                      throught dma transfer operation.          */
/*   _ run once         allows the user to go throught the whole  */
/*                      dma transfer operation once.              */
/*   _ loop             allows the user to loop.                  */
/******************************************************************/
tool_dma_intr()
{
   int  c, count, i, num, len, step, run1, loop;
   struct  scsi_ha_reg   *har;
 
   har   = scsi.scsi_addr;
   i     = (int)(scsi.test_num);
   prt_cmd("\n  Default command is set to  0x%x ");
 
   while (1){
 
      for (num = 0; num <= 1; ){
 
         if ((i == 8) && scsi.vme ){
            num = prt_get_option(menu_opt4, tools[i-1].name, " DEBUG OPTION ");
            if (num == 6) tool_rw_screg(); 
         }else{
            num = prt_get_option(menu_opt3, tools[i-1].name, " DEBUG OPTION ");
         }

         if (num == 1) prt_set_cmd();         
 
      }   
 
      if (num == 5) return;
      step    = ((num == 2) ?  1 : 0);
      run1    = ((num == 3) ?  1 : 0);
      loop    = ((num == 4) ?  1 : 0);
      len     = (( i  == 8) ?  512*scsi.nblk : (512*scsi.nblk)/2 );

      if (loop) printf("\n  Hit control 'c' to get out of loop");

      for (;;){

         start :

         if ( _setjmp(bus_error))
            reset_prt_berr(har, step || run1);
 
         if ( loop && (maygetchar() == ('c' & 037))) break;

         bzero((u_short *)&scsi.elog, sizeof scsi.elog); 
         scsi.int_en  = 1;

         if (step) step_prompt(1);
        
         if ((c = t_dev_sel(har, step, run1)) != 0){
            if (!loop) break; else goto start;
         }
 
         har->dma_addr  = DBUF_PA ;
         har->dma_count = ~len & 0xFFFF;
         har->icr       = ICR_WORD_MODE | ICR_DMA_ENABLE | ICR_INTERRUPT_ENABLE;

         if ( !_setjmp(int2_buf)){

            if ((c = t_cmd_xfer(har, step, run1)) != 0){
               if (!loop) break; else goto start;
            }

            for (i = 0; i < 5000 ; i++)
               DELAY(100);

            if (loop){
               goto start;
            }else{
               prt_dma_no_intr();
               break;  /* break the for loop. */
            }

         }else{
            
            har->icr &= ~ICR_INTERRUPT_ENABLE ;

            if (!scsi.int_en || (scsi.int_en && (scsi.int_lv != 2))){
               if (!loop) prt_dma_intr_err(); else goto start;
            }

            count = har->dma_count;
            if (( count != -1) || (count != 1)){
               if (!loop){
                  scsi.elog.act_val  = len - ~har->dma_count;
                  scsi.elog.exp_val  = len;
                  if (har->icr & ICR_BUS_ERROR)
                     prt_dma_bus_err();
                  else
                     prt_dma_bad_len();
               }else goto start;
            }
             
            scsi.int_en = 0;
         } 
      }   
   } 
}





 
/******************************************************************/
/******************************************************************/
tool_scsi_timer() 
{ 
   int  c, num, step, run1, loop;
   struct  scsi_ha_reg   *har;
 
   har    = scsi.scsi_addr;

   printf("\n  Not implemented yet...");
} 





/******************************************************************/
/* wr_sc_reg function write expected value and read and actual to */
/* and from the register designated by reg. Follow is the list of */
/* all parameters use by wr_sc_reg and their meaning :            */
/*  _har    : pointer to scsi's registers.                        */
/*  _reg    : designates the register, 1 for ICR, 2 for DMA_count,*/
/*            3 for DMA_address, and 8 for Interrupt Vector Reg.  */
/*  _mask   : mask for read and write values.                     */
/*  _exp_val: the value to be written to the register.            */
/*  _loop   : loop flag.                                          */
/*  _rw     : determines read or write.                           */
/*                1 for write                                     */
/*                2 for read                                      */
/*            and 3 for write/read.                               */ 
/*  _reg_name : pointer to register's name string.                */
/******************************************************************/
wr_sc_reg(har, reg, mask, exp_val, loop, rw, reg_name)
   struct    scsi_ha_reg  *har;
   int  reg, mask, loop, rw;
   register  int   exp_val;
   char      *reg_name;
{
   register  int   act_val;

   if (loop)
      printf("\n  Hit control 'c' to get out of loop.");

   while ( maygetchar() != ('c' & 037)){

      if ( _setjmp(bus_error) ){
         if ((!loop) || (maygetchar() == ('c' & 037)) ){
            printf("  Got BUS ERROR : %s", (char*)scsi.berr_tp);
            break;  /* break the while loop. */
         }
      } 

      if ((rw == 1) || (rw == 3)){
         switch(reg){
            case 1 : har->icr       = exp_val;    break;   
            case 2 : har->dma_count = exp_val;    break;   
            case 3 : har->dma_addr  = exp_val;    break;   
            case 8 : har->intvec    = exp_val;    break;   
         }
         if (!loop)
            printf("  just wrote 0x%x to %s.", exp_val, reg_name);
      }

      if ((rw == 2) || (rw == 3)){
         switch(reg){
            case 1 : act_val = (har->icr       & mask) ;    break;   
            case 2 : act_val = (har->dma_count & mask) ;    break;   
            case 3 : act_val = (har->dma_addr  & mask) ;    break;   
            case 8 : act_val = (har->intvec    & mask) ;    break;   
         }
         if (!loop)
            printf("  just read 0x%x from %s.", act_val, reg_name);
      }

      if (!loop) break;  /* break the while loop. */
   }
}

