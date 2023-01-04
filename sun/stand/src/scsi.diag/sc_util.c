/******************************************************************************/
/*                                                                            */
/*  Follow is a list of all functions that are included in this file:         */
/*    _ chk_quit_resp   check quit or exit response from user.                */
/*    _ chk_reply       check user reply.                                     */
/*    _ cmd_exit        exit command.                                         */
/*    _ cmd_help        help command.                                         */
/*    _ cmd_info        information flag switching command.                   */
/*    _ cmd_read_cdb    read command_description_block command.               */
/*    _ cmd_read_data_reg  read data register command.                        */
/*    _ cmd_read_dma_addr  read dma address register command.                 */
/*    _ cmd_read_dma_cntr  read dma counter register command.                 */
/*    _ cmd_read_icr    read interface control register command.              */
/*    _ cmd_read_scb    read command description block command.               */
/*    _ cmd_reset       scsi's reset command.                                 */
/*    _ cmd_write_cdb   write command_description_block command.              */
/*    _ cmd_write_data_reg   write to data register command.                  */
/*    _ cmd_write_dma_addr   write to dma address register command.           */
/*    _ cmd_write_dma_cntr   write to dma counter register command.           */
/*    _ cmd_write_icr   write to interface control register command.          */
/*    _ error_report    error report function.                                */
/*    _ ex_command      executed command function.                            */
/*    _ get_loop_num    get number to loop throught the test.                 */
/*    _ pgetn           print the message and get a number from user.         */
/*    _ pgetnh          print the message and get a hexadecimal num from user.*/
/*    _ pgetr           print message and get response from user.             */
/*    _ prt_test_icr_err     print interface control register error.          */
/*    _ prt_test_cntr_err    print dma counter register error.                */
/*    _ prt_test_addr_err    print dma address register error.                */
/*    _ prt_test_4to12_err   decode error code and call correspondin error    */
/*                           print function.                                  */
/*    _ prt_data_miss_comp   print data misscompare error.                    */
/*    _ prt_bad_mess_in      print bad message in error.                      */
/*    _ prt_bad_intr_vect    print bad interrupt vector error.                */
/*    _ prt_chk_on_status    print check on status error.                     */
/*    _ prt_dev_sel_err      print device initial select error.               */
/*    _ prt_dma_bus_err      print dma bus error.                             */
/*    _ prt_dma_intr_err     print dma interrupt error.                       */
/*    _ prt_dma_no_intr      print dma no interrupt error.                    */
/*    _ prt_no_intr_err      print no interrupt error.                        */
/*    _ prt_get_put_err      print get_byte or put_byte error.                */
/*    _ prt_got_bus_err      print got bus error.                             */
/*    _ prt_rtype            print request type.                              */
/******************************************************************************/

static char     sccsid_util[] = "@(#)sc_util.c 1.1 86/09/25 SMI";


#include    <sys/types.h>
#include    <setjmp.h>
#include    <machdep.h>
#include    "sc_reg.h" 
#include    "sc_util.h"

extern jmp_buf  get_out, bus_error;
extern struct   scsi_par scsi;


int   cmd_help(), cmd_info(), cmd_reset(), cmd_read_dma_addr(), cmd_read_cdb(),
      cmd_read_dma_cntr(), cmd_read_data_reg(), cmd_read_icr(), cmd_read_scb(),
      cmd_exit(),cmd_write_dma_addr(), cmd_write_cdb(), cmd_write_dma_cntr(),
      cmd_write_data_reg(), cmd_write_icr();

long  swap_par_addr, nsect_per_cyl;

struct cmd {

     char   *cmd_name;
     int    cmd_len;
     int    (*cmd_ptr)();

}    commands[] = {
     "help",    1,    cmd_help,
     "info",    1,    cmd_info,
     "reset",   1,    cmd_reset,
     "@addr",   3,    cmd_read_dma_addr,
     "@cdb",    3,    cmd_read_cdb,
     "@cntr",   3,    cmd_read_dma_cntr,
     "@data",   3,    cmd_read_data_reg,
     "@icr",    3,    cmd_read_icr,
     "@scb",    3,    cmd_read_scb,
     "exit",    1,    cmd_exit,
     "x",       1,    cmd_exit,
     "!addr",   3,    cmd_write_dma_addr,
     "!cdb",    3,    cmd_write_cdb,
     "!cntr",   3,    cmd_write_dma_cntr,
     "!data",   3,    cmd_write_data_reg,
     "!icr",    3,    cmd_write_icr,
     "?",       1,    cmd_help,
     0,
};

char *cmd_desc[] = {
     "exit or x :  exit or get out of diagnostic.",
     "help or ? :  list all available commands and their functions.",
     "info      :  turns information flag on and off.",
     "reset     :  reset scsi bus.",
     "@addr     :  read dma address register.(vme only)",
     "@cdb      :  read command description block.",
     "@cntr     :  read dma counter register.",
     "@data     :  read data register.",
     "@icr      :  read interface control register.",
     "@scb      :  read status completion block.",
     "!addr     :  write dma address register.",
     "!cdb      :  write command description block.",
     "!cntr     :  write dma counter register.",
     "!data     :  write data register.",
     "!icr      :  write interface control register.",
     0,
};







/**************************************************************/
/* This  function  checks for any  quit or  exit diagnostics  */
/* response from user during test execution.  If  found such  */
/* responce, it return that response to calling function, else*/
/* a value of 0 is return.                                    */
/**************************************************************/
chk_quit_resp()
{
   int  c;

   c = maygetchar();
   if ((c == 'Q') || (c == 'q') || (c == 'X') || (c == 'x'))
      return(c);
   else
      return(0);
}




/**************************************************************/
/* This function checks the user reply, passed over from  the */
/* calling function. This is done by first align the buffer   */
/* pointer with the first char in string, then it chesks to   */
/* see if the char string start out with a number char. If so */
/* the char number is converted to integer and return it to   */
/* caller function.  Otherwise, it treats  the char string as */
/* command and pass it to ex_command function.                */
/**************************************************************/
chk_reply(buf,i)
   char buf[LINEBUFSZ];
   int i;
{
   char  *p;
   int   num;

   for (p = buf ; p < &buf[LINEBUFSZ]; p++)
      if (*p != ' ' && *p != '\t')
         break;  /* for loop. */

   if (*p == '\0' || p >= &buf[LINEBUFSZ] ){
      printf("\n  What ? ");
      return 0;
   }

   if (*p < '0' || *p > '9' ){
      ex_command(p);
      return (0);
   }

   num = ktoi(buf,10);
   if ( num <= 0 || num > i ){
      printf("\n  %d is not included in menu, try again.",num);
      return (0);
   }  

   return (num);
}





/**************************************************************/
/* Exit the diagnostics.                                      */
/**************************************************************/
cmd_exit()
{
   printf("\nExiting SCSI diagnostics.");
   restore_sys_buserr_handler();
   _longjmp(get_out);
}





/**************************************************************/
/* Prints help informations. List out all command and their   */
/* functions.                                                 */
/**************************************************************/
cmd_help()
{
   int i, c;
 
   printf("\n  COMMAND LIST:");
   for (i = 0; cmd_desc[i] != 0; i++)
        printf("\n    %s", cmd_desc[i]);
   printf("\n\n  hit any key to get out of help.");
   while(maygetchar() == -1);
}





/**************************************************************/
/* Switchs informational messages on which allows test info   */
/* to be printed in case of error.                            */
/**************************************************************/
cmd_info()
{
   scsi.info ^= 1;
   printf("\n  Informational messages is %s", scsi.info ? "on." : "off.");
}





/**************************************************************/
/* Reads and displays command description block.              */
/**************************************************************/
cmd_read_cdb()
{
   printf("  Command description block:");
   prt_cmd("\n  Command  =  0x%x  ");
   printf("\n  lun      =  0x%x",scsi.cdb.lun);
   printf("\n  high addr=  0x%x",scsi.cdb.high_addr);
   printf("\n  low addr =  0x%x",scsi.cdb.low_addr);
   printf("\n  count    =  0x%x",scsi.cdb.count);
   printf("\n  flag req =  %s",(scsi.cdb.fr) ? "on" : "off");
   printf("\n  link cmd =  %s",(scsi.cdb.link) ? "on" : "off");
}






/**************************************************************/
/* Reads and displays the data register.                      */
/**************************************************************/
cmd_read_data_reg()
{
   struct scsi_ha_reg  *har;

   har = scsi.scsi_addr;
   printf("  Data reg  = 0x%x",(int)har->data);
}





/**************************************************************/
/* Reads and displays DMA address register.                   */
/**************************************************************/
cmd_read_dma_addr()
{
   struct scsi_ha_reg  *har;

   har = scsi.scsi_addr;
   if (scsi.vme)
      printf("  DMA address reg  =  0x%x",(int)har->dma_addr);
   else
      printf("  Can't read DMA address reg on Multibus SCSI board.");
}





/**************************************************************/
/* Reads and displays DMA counter reigster.                   */
/**************************************************************/
cmd_read_dma_cntr()
{
   struct scsi_ha_reg  *har;

   har = scsi.scsi_addr;
   printf("  dma counter register  =  0x%x",(int)har->dma_count); 
}





/**************************************************************/
/* Reads and displays Interface Control Register.             */
/**************************************************************/
cmd_read_icr()
{
   struct scsi_ha_reg  *har;

   har = scsi.scsi_addr;
   prt_icr_bit("  Interface control reg  =  0x%x  ",(int)har->icr);
}





/**************************************************************/
/* Reads and displays Status Completion Block.                */
/**************************************************************/
cmd_read_scb()
{
   printf("  Status completion block:");
   printf("\n  int status =  %s",(scsi.scb.is) ? "on" : "off");
   printf("\n  busy       =  %s",(scsi.scb.busy) ? "on" : "off");
   printf("\n  condition  =  %s",(scsi.scb.cm) ? "met" : "not met");
   printf("\n  check      =  %s",(scsi.scb.chk) ? "on" : "off");
   if (scsi.scb.ext_st1)
      printf("\n  host err   =  %s ",(scsi.scb.ha_er) ? "on" : "off");
   if (scsi.scb.ext_st2)
      printf("\n 3rd st byte = 0x%x ",scsi.scb.byte2);
}





/**************************************************************/
/* Reset the SCSI bus.                                        */
/**************************************************************/
cmd_reset()
{
   struct scsi_ha_reg  *har;

   har = scsi.scsi_addr;
   har->icr = 0;
   har->icr = ICR_RESET;
   DELAY(100);
   har->icr = 0;
}






/**************************************************************/
/* Writes Command Description Block.                          */
/**************************************************************/
cmd_write_cdb()
{
   printf("  write command descripton block:");
   scsi.cdb.cmd  =  (0xff & pgetnh("\n  command   = 0x"));
   scsi.cdb.lun  =  (0x7 & pgetnh("\n  lun       = 0x"));
   scsi.cdb.high_addr = (0x1f & pgetnh("\n  high_addr = 0x"));
   scsi.cdb.mid_addr = (0xff & pgetnh("\n  mid_addr  = 0x"));
   scsi.cdb.low_addr = (0xff & pgetnh("\n  low_addr  = 0x"));
   scsi.cdb.count = (0xff & pgetnh("\n  count     = 0x"));
   scsi.cdb.fr = (0x1 & pgetnh("\n  flg req   = 0x"));
   scsi.cdb.link = (0x1 & pgetnh("\n  link      = 0x"));
}






/**************************************************************/
/* Writes into data register.                                 */
/**************************************************************/
cmd_write_data_reg()
{
   struct scsi_ha_reg  *har;

   har = scsi.scsi_addr;
   har->data = (0xff & pgetnh("  write data reg, data  =  0x"));
}





/**************************************************************/
/* Writes into DMA address register.                          */
/**************************************************************/
cmd_write_dma_addr()
{
   struct scsi_ha_reg  *har;

   har = scsi.scsi_addr;
   har->dma_addr = (0xffffff & 
                    pgetnh("  write dma address reg, data  =  0x"));
}





/**************************************************************/
/* Writes into DMA counter register.                          */
/**************************************************************/
cmd_write_dma_cntr()
{
   struct scsi_ha_reg  *har;

   har = scsi.scsi_addr;
   har->dma_count = (0xffff & 
                    pgetnh("  write dma counter reg, data  =  0x"));
}





/**************************************************************/
/* Writes into Interface Control register.                    */
/**************************************************************/
cmd_write_icr()
{
   struct scsi_ha_reg  *har;

   har = scsi.scsi_addr;
   har->icr = pgetnh("  write icr, data = 0x");
   prt_icr_bit("  ",(int)har->icr);
}






/**************************************************************/
/* Handles error roports. It prints out the test which failed */
/* and information on the test's functionality,  then  calls  */
/* other functions to do error displays. Afterward it calls   */
/* getnuse_debug_tools() to allows ther user to invoke debug  */
/* tools if running in manual mode.                           */
/**************************************************************/
error_report()
{
   int    i;
 
   if (scsi.info){
      for ( i = 0; test_info[scsi.test_num][i] != 0; i++)
          printf("\n\t %s", test_info[scsi.test_num][i]);
   }

   if (scsi.test_num > 3 )
      prt_test_4to12_err();
   else
      switch ( (int)scsi.test_num ){
         case 1 :  prt_test_icr_err();  break;
         case 2 :  prt_test_cntr_err(); break;
         case 3 :  prt_test_addr_err(); break;
      }

   if ( (scsi.man) && (pgetr("\n\n  Do you wish to used debug tools ?  ")))
      getnuse_debug_tools();


}






/**************************************************************/
/* This function gets the command string passed by the caller */
/* and does a command search in command's list. If found, the */
/* command will be executed and return to the caller. Other-  */
/* wise an invalid command message will be printed.           */ 
/**************************************************************/
ex_command(p)
   char  *p;
{
   struct cmd *cp;
   int  i;
 
 
   for ( cp = commands; cp->cmd_name != 0; cp++){
      i = MAX(cp->cmd_len, strlen(p));   
      if (strncmp(p, cp->cmd_name, i) == 0){ 
         (*cp->cmd_ptr)();
         return;
      }                     
   }                     
   if (cp->cmd_name == 0)                      
   printf("\n  Invalid command, type help or ? for commands list.\n");
}
 

 
 
 
/**************************************************************/
/*  Get_loop_num returns a loop counter value to the calling  */
/* function. The value return can be either the defualt value */
/* which is passed from the calling function or a value enter-*/
/* ed from user depending on the run 'all' flag.              */
/**************************************************************/
get_loop_num(num)
   int  num;
{
   if (!scsi.all)
      num = (pgetn("\n  Enter number of times to run   "));
   if (num == 0)
      printf("  skipped");    
   return(num);
}

 
 


/**************************************************************/
/* prints the char string passed by caller, gets reply from   */
/* the user, converts it into integer number, and returns it  */
/* to caller.                                                 */
/**************************************************************/
pgetn(s)
   char *s;
{
   printf(s);
   return (getn());
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






/**************************************************************/
/* This function prints error messages for test # 1 (test icr */
/* register).                                                 */
/**************************************************************/
prt_test_icr_err()
{
   if ( scsi.elog.code & 0x0003 ){
      printf("\n\t Time out during %s access to ICR register.",
            ((scsi.elog.code & 0x0003) == 1) ? "write" :
            (((scsi.elog.code & 0x0003) == 2) ? "read" : "read/write"));
      printf("\n\t Is SCSI board blugged in ?");
   }
   if (scsi.elog.code & 0x0004){
      printf("\n\t Exp and act values on write/read to ICR not equal"); 
      printf("\n\t Expected ICR = 0X%x",scsi.elog.exp_val);
      printf("\n\t Actual   ICR = 0X%x",scsi.elog.act_val);
   }
}




/**************************************************************/
/* This function prints error messages for test # 2 (test dma */
/* counter register).                                         */
/**************************************************************/
prt_test_cntr_err()
{
   if ( scsi.elog.code & 0x0003 ){
      printf("\n\t Time out during %s access to DMA counter register.",
            ((scsi.elog.code & 0x0003) == 1) ? "write" :
            (((scsi.elog.code & 0x0003) == 2) ? "read" : "read/write"));
      printf("\n\t Is SCSI board blugged in ?");
   }
   if (scsi.elog.code & 0x0004){
      printf("\n\t Exp and act values on write/read to DMA counter");
      printf("\n\t register not equal.");
      printf("\n\t Expected DMA cntr reg = 0X%x",scsi.elog.exp_val);
      printf("\n\t Actual   DMA cntr reg = 0X%x",scsi.elog.act_val);
   }
}





/**************************************************************/
/* This function prints error messages for test # 3 (test dma */
/* address register).                                         */
/**************************************************************/
prt_test_addr_err()
{
   if ( scsi.elog.code & 0x0003 ){
      printf("\n\t Time out during %s access to DMA address register.",
            ((scsi.elog.code & 0x0003) == 1) ? "write" :
            (((scsi.elog.code & 0x0003) == 2) ? "read" : "read/write"));
      printf("\n\t Is SCSI board blugged in ?");
   }
   if (scsi.elog.code & 0x0004){
      printf("\n\t Exp and act values on write/read to DMA address");
      printf("\n\t register not equal.");
      printf("\n\t Expected DMA addr reg = 0X%x",scsi.elog.exp_val);
      printf("\n\t Actual   DMA addr reg = 0X%x",scsi.elog.act_val);
   }
}







/**************************************************************/
/* This is the declaration of SCSI error codes to error print */
/* functions. Each of the predefined error code has a corre-  */
/* sponding error print function which prints out the error   */
/* messages associating with that error code.                 */
/**************************************************************/
int   prt_data_miss_comp(), prt_bad_mess_in(),  prt_bad_intr_vect(), 
      prt_chk_on_status(),  prt_dev_sel_err(),  prt_dma_bad_len(),
      prt_dma_bus_err(),    prt_dma_intr_err(), prt_dma_no_intr(),
      prt_get_put_err(),    prt_no_intr_err(),  prt_got_bus_err(),
      prt_scc_error();

struct  ecode_to_message {

      int     ecode;
      int     (*prt_err_funct)();

}     etm[] =  {

      DATA_MISS_COMP,   prt_data_miss_comp,
      BAD_MESS_IN,      prt_bad_mess_in, 
      BAD_INTR_VECT,    prt_bad_intr_vect,
      CHK_ON_STATUS,    prt_chk_on_status,
      DEV_SEL_ERR,      prt_dev_sel_err,
      DMA_BAD_LEN,      prt_dma_bad_len,
      DMA_BUS_ERR,      prt_dma_bus_err,
      DMA_INTR_ERR,     prt_dma_intr_err,
      DMA_NO_INTR,      prt_dma_no_intr,
      GET_BYTE_ERR,   	prt_get_put_err,
      GOT_BUS_ERR,      prt_got_bus_err,
      NO_INTR_REQ,      prt_no_intr_err,
      PUT_BYTE_ERR,     prt_get_put_err,
      SCC_ERROR,        prt_scc_error,
      0,
};       

/**************************************************************/
/* This function checks the scsi.elog.code  for all possible  */
/* error  by  comparing it  with all of the predefined error  */
/* codes. If a match is found, the corresponding error print  */
/* function is called to do error print.                      */
/**************************************************************/
prt_test_4to12_err()
{
   struct  ecode_to_message   *eptr; 

   if (scsi.test_num < 12){
      prt_cmd("\n\t Command =  0x%x  ");
      if (scsi.nblk > 0) printf("\n\t number of block = %d", scsi.nblk);
   }

   for (eptr = &etm[0]; eptr->ecode != 0; eptr++)
      if (scsi.elog.code & eptr->ecode)
         (*eptr->prt_err_funct)(); 

}





/**************************************************************/
/* DATA_MISS_COMP error code is set if the data writen to and */
/* read back from disk drive are miss-compared.               */
/**************************************************************/
prt_data_miss_comp()
{
   printf("\n\t Data writen to and read back from disk miss-compared!");
   printf("\n\t Data writen to disk = 0x%x",scsi.elog.exp_val);
   printf("\n\t Data read from  disk = 0x%x",scsi.elog.act_val);
}





/**************************************************************/
/*  BAD_MESS_IN error code is set if the target device returns*/
/* a value other then 0, command complete,  at the end of any */
/* SCSI operations.                                           */ 
/**************************************************************/
prt_bad_mess_in()
{
   printf("\n\t Got BAD MESSAGE in from device at completion of command");
   printf("\n\t execution. Message does not indicate COMMAND COMPLETE.");
   printf("\n\t Expected message in = 0x00");
   printf("\n\t Actual   message in = 0x%x", scsi.elog.act_val);
}





/**************************************************************/

/**************************************************************/
prt_bad_intr_vect()
{
   printf("\n\t Exp and act values on write/read to Interrupt vect");
   printf("\n\t register not equal.");
   printf("\n\t Expected Intr Vect reg = 0X%x",scsi.elog.exp_val);
   printf("\n\t Actual   Intr Vect reg = 0X%x",scsi.elog.act_val);
}





/**************************************************************/
/*  CHK_ON_STATUS error code is set if the status block return*/
/* from target device has check bit on. This bit on indicates */
/* that something went wrong during SCSI operation and sense  */
/* information is available for diagnose.                     */   
/**************************************************************/
prt_chk_on_status()
{
   printf("\n\t Status Completion Block , received from the device,");
   printf("\n\t has unexpected check bit set. A REQUEST SENSE command ");
   printf("\n\t is send to the device and following sense information");
   printf("\n\t is received from device:");
   prt_sense("");

}





/**************************************************************/
/*  DEV_SEL_ERR error code is set on the following condition: */
/*   _ the target device did not response with busy bit in ICR*/
/*   _ bus error bit in ICR is set unexpectively.             */
/*   _ busy bit stuck on after reset and clear ICR.           */
/**************************************************************/
prt_dev_sel_err()
{

   printf("\n\t Error occurred during device initial selection.");

   if (scsi.elog.code & COND_NOT_MET)
      printf("\n\t Device did not response with busy bit in ICR.");

   if (scsi.elog.code & COND_BUS_ERR){
      printf("\n\t Got BUS ERROR while waiting for device to response");
      printf("\n\t with BUSY.");
   }

   if (scsi.elog.code & BUSY_BIT_STK)
      printf("\n\t BUSY bit stuck on after reset and clear ICR.");

   prt_icr_bit("\n\t Expected ICR = 0x%x  ",scsi.elog.exp_icr);
   prt_icr_bit("\n\t Actual   ICR = 0x%x  ",scsi.elog.act_icr);

}







/**************************************************************/
/* DMA_BAD_LEN error code is set if the dma counter register  */
/* has a incorrect count at the end of dma transfer.          */
/**************************************************************/
prt_dma_bad_len()
{
   struct scsi_ha_reg  *har;

   har  =  scsi.scsi_addr;
   printf("\n\t DMA counter reg has incorrect length at completion");
   printf("\n\t of DMA transfer.");
   printf("\n\t Expected bytes transferred   = %d ",scsi.elog.exp_val);
   printf("\n\t Actual bytes transferred     = %d ",scsi.elog.act_val);
   printf("\n\t Expected DMA counter reg     = 0x%x ", 
                (scsi.test_num == 9) ? 0x1 : 0xFFFF );
   printf("\n\t Actual   DMA counter reg     = 0x%x ", har->dma_count);
}





/**************************************************************/
/* DMA_BUS_ERR error code is set under the following condition*/
/*    _ buss error bit in ICR (bit 14) is set while waiting   */
/*      waiting for an interrupt request from target device   */
/*      at completion of dma transfer.                        */
/**************************************************************/
prt_dma_bus_err()
{
   struct scsi_ha_reg  *har;

   har  =  scsi.scsi_addr;
   printf("\n\t Got unexpected ICR BUS ERROR (bit 14 of ICR) while");
   printf("\n\t waiting for device to generate an INTERRUPT REQUEST");
   printf("\n\t (bit 12 of ICR) at the end of DMA transfer.");
   printf("\n\t Expected bytes to transfer = %d ",scsi.elog.exp_val);
   printf("\n\t Actual bytes transferred   = %d ",scsi.elog.act_val);
   printf("\n\t Expected DMA counter reg   = 0x%x",
                ((scsi.test_num == 9) ? -1 : 0xFFFF) );
   printf("\n\t Actual   DMA counter reg   = 0x%x ", har->dma_count);
}






/**************************************************************/
/* DMA_INTR_ERR error code is set on the following conditions:*/
/*    _ With interrupt bit in ICR asserted, Interrupt request */
/*      or DMA overrun caussed the wrong interrupt level to   */
/*      occurred. Expecting interrupt level for SCSI is 2.    */
/*    _ Got an interrupt while interrupt is not enable.       */
/**************************************************************/
prt_dma_intr_err()
{
   if (scsi.int_en){
      printf("\n\t Received wrong interrupt level from device on status");
      printf("\n\t request, at completion of DMA transfer.");
      printf("\n\t Expected interrupt level = 2 ");
      printf("\n\t Actual   interrupt level = %d ", scsi.int_lv);
   }else{
      printf("\n\t Got unexpected interrupt from device on status");
      printf("\n\t req at completion of DMA transfer, but interrupts");
      printf("\n\t not enabled.");
   }
}






/**************************************************************/
/* DMA_NO_INTR error code is set if no interrupt level 2 from */
/* device at the completion of DMA transfer phase.            */
/**************************************************************/
prt_dma_no_intr()
{
   printf("\n\t Not getting  INTERRUPT  LEVEL 2  from target device at the ");
   printf("\n\t completion of DMA transfer phase with interrupt enable bit");
   printf("\n\t in ICR asserted).");
}





/**************************************************************/
/* NO_INTR_REQ error code is set if not getting interrupt req */
/* (bit 12 of ICR) at the end of dma transfer.                */
/**************************************************************/
prt_no_intr_err()
{
   struct scsi_ha_reg  *har;

   har  =  scsi.scsi_addr;
   if (scsi.elog.act_val != scsi.elog.exp_val){
      printf("\n\t DMA transfer on %d bytes of data not completed",
              scsi.elog.exp_val);
      printf("\n\t after 5 seconds.");
   }else{
      printf("\n\t NOT GETTING interrupt request ( bit 12 of ICR )");
      printf("\n\t from device at the completion of DMA transfer.");
   }  
   printf("\n\t Expected bytes to transfer = %d ",scsi.elog.exp_val);
   printf("\n\t Actual bytes transferred   = %d ",scsi.elog.act_val);
   printf("\n\t Expected DMA counter reg   = -1 ");
   printf("\n\t Actual   DMA counter reg   = 0x%x ", har->dma_count);
}







/**************************************************************/
/*  GET_BYTE_ERR  or  PUT_BYTE_ERR is set under the following */
/* conditions :                                               */
/*    _ buss error bit in ICR (bit 14) is set while waiting   */
/*      waiting for a request from target device.             */
/*    _ got no request from target device.                    */
/*    _ request type from traget device changed while there   */
/*      there are more information to be transfer for the     */
/*      previous request.                                     */ 
/**************************************************************/
prt_get_put_err()
{
   printf("\n\t Error occurred while in ");

   switch(scsi.elog.exp_icr & ICR_BITS){
      case 0 :
         printf("data phase. Sending data out\n\t to device.");
         break;
      case ICR_INPUT_OUTPUT :
         printf("data phase. Getting data in\n\t from device.");
         break;
      case ICR_COMMAND :
         printf("command phase. Sending command\n\t out to device.");
         break;
      case ICR_STATUS :
         printf("status phase. Getting status\n\t in from device.");
         break;
      case ICR_MESSAGE | ICR_COMMAND_DATA :
         printf("message phase. Sending message\n\t out to device.");
         break;
      case ICR_MESSAGE_IN :
         printf("message phase. Getting message\n\t in from  device.");
         break;
   }

   if ((scsi.elog.code & COND_NOT_MET) || (scsi.elog.code & COND_BUS_ERR)){
      if (scsi.elog.code & COND_NOT_MET){
         printf("\n\t Interface control register condition not met.");
         printf("\n\t Device should be REQUESTING for ");
      }else{
         printf("\n\t Got an unexpected BUS ERROR while waiting for");
         printf("\n\t device to REQUEST for ");
      }  
      prt_rtype(scsi.elog.exp_icr & ICR_BITS);
   }

   if (scsi.elog.code & XFER_LEN_ERR){
      printf("\n\t Device changed request from ");
      prt_rtype(scsi.elog.exp_icr & ICR_BITS);
      printf("\b to ");
      prt_rtype(scsi.elog.act_icr & ICR_BITS);
      printf("\b\n\t while there are still %d",
              scsi.elog.exp_val - scsi.elog.act_val);
      printf(" bytes left to transfer.");
   }

   prt_icr_bit("\n\t Expected ICR = 0x%x  ",scsi.elog.exp_icr);
   prt_icr_bit("\n\t Actual   ICR = 0x%x  ",scsi.elog.act_icr);

}




/**************************************************************/
/*  GOT_BUS_ERR  error code is set if a time out or bus error */
/* interrupt occurred during any SCSI operation.              */  
/**************************************************************/
prt_got_bus_err()
{
   char *s;

   s = scsi.berr_tp;
   printf("\n\t Got unexpected BUS ERROR : ");
   printf(s);
}





/**************************************************************/
/* Print request function decode the bits in rtype into more  */
/* meaningfull messages.                                      */
/**************************************************************/
prt_rtype(rtype)
   int    rtype;
{
   switch(rtype){
      case 0 :
         printf("data out.");
         break;
      case ICR_INPUT_OUTPUT :
         printf("data in.");
         break;
      case ICR_COMMAND :
         printf("command.");
         break;
      case ICR_STATUS :
         printf("status in.");
         break;
      case ICR_MESSAGE | ICR_COMMAND_DATA :
         printf("message out.");
         break;
      case ICR_MESSAGE_IN :
         printf("message in.");
         break;
      default :
         printf("unknown request!");
   }
}

