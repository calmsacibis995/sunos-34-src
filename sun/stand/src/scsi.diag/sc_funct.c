/******************************************************************************/
/*                                                                            */
/*  THIS  FILE  CONTAINT  SCSI  RELATED  FUNCTIONS  USED  BY  TEST  MODULES.  */
/*                                                                            */
/*  Follow is the list of all functions include in this file:                 */
/*    _ bus_transfer     bus transfer function.                               */
/*    _ compare_data     compare expected and actual data in dma memory space.*/
/*    _ dma_transfer     dma transfer function.                               */
/*    _ dev_init_sel     device initial select function.                      */
/*    _ exchg_buserr_handler  exchange system with program's bus error handler*/
/*    _ get_byte         get a byte from target device.                       */
/*    _ get_sense_info   get sense information from target device.            */
/*    _ gen_rand_data    generate random data.                                */
/*    _ init_data        initialize test pattern.                             */
/*    _ intr_init        initialize interrupt vector for scsi.                */
/*    _ intr_reset       restore original interrupt vetor.                    */
/*    _ reentrant        interrupt handler routine.                           */
/*    _ put_byte         give a byte to target device.                        */
/*    _ prt_cmd          print comand in command description block.           */
/*    _ prt_icr_bit      print meaning of bits in interface control register. */
/*    _ prt_sense        print sense information.                             */
/*    _ restore_sys_buserr_handler  restore system bus error handler routine. */
/*    _ set_command      set command in command description block.            */
/*    _ time_out         time out error handler routine.                      */
/*    _ wait_cond        wait on a condition to occurre in ICR.               */
/******************************************************************************/
static  char    sccsid_funct[] = "@(#)sc_funct.c 1.1 86/09/25 SMI Copyright Sun Microsystem." ;


#include    <sys/types.h>
#include    <setjmp.h>
#include    <s2addrs.h>
#include    <machdep.h>
#include    <reentrant.h>
#include    "vectors.h"
#include    "m68000.h"
#include    "sc_reg.h"


char *class_00_errors[] = {
        "No sense.",
        "No index signal.",
        "No seek complete.",
        "Write fault.",
        "Drive not ready.",
        "Drive not selected.",
        "No track 00.",
        "Multiple drives selected.",
        "No address acknowledged.",
        "Media not loaded.",
        "Insufficient capacity.",
};

char *class_01_errors[] = {
        "I.D. CRC error.",
        "Unrecoverable data error.",
        "I.D. address mark not found.",
        "Data address mark not found.",
        "Record not found.",
        "Seek error.",
        "DMA timeout error.",
        "Write protected.", 
        "Correctable data check.", 
        "Bad block found.", 
        "Interleave error.", 
        "Data transfer incomplete.", 
        "Unformatted or bad format on drive.", 
        "Self test failed.", 
        "Defective track (media errors).", 
}; 

char *class_02_errors[] = { 
        "Invalid command.", 
        "Illegal block address.", 
        "Aborted.", 
        "Volume overflow.", 
}; 
 
char **SC_errors[] = { 
        class_00_errors,
        class_01_errors,
        class_02_errors,
        0, 0, 0, 0,
};

int SC_errct[] = {
        sizeof class_00_errors / sizeof class_00_errors[0],
        sizeof class_01_errors / sizeof class_01_errors[0],
        sizeof class_02_errors / sizeof class_02_errors[0],
        0, 0, 0, 0,
};

 
char *SC_sense7_keys [] = {
        "No sense.",
        "Recoverable error.",
        "Not ready.",
        "Media error.",
        "Hardware error.",
        "Illegal request.",
        "Media change.",
        "Write protect.",
        "Diagnostic unique.",
        "Vendor unique.",
        "Power up failed.",
        "Aborted command.",
        "Equal.",
        "Volume overflow.",
};


char *bus_err_type [] = {
        "page invalid.",
        "p1 bus master.",
        "protection error.",
        "timeout error.",
        "upper byte parity.",
        "lower byte parity.",
};



extern jmp_buf  get_out, bus_error;
extern struct   scsi_par scsi;

jmp_buf int2_buf;

static long svect1, svect2, svect3, svect4, svect5, svect6, svect7;





/******************************************************************/
/* Bus_transfer function attempt to do data transfer using program*/
/* I/O technique. This is done by first select the target device, */
/* set up and send command block to device, do the data transfer, */ 
/* read status back from device,  and at last, read the message   */
/* back from device and expect it to be command complete message. */
/*  If anything go wrong along the way, it immediately terminate  */
/* the transfer process, set up error code in scsi.elog structure */
/* and return a value of -1 to the calling function.              */
/*  Parameters list :                                             */
/*      har        pointer contain address to scsi host adp reg.  */
/*      dest_addr  destination address, where to transfer data    */
/*                 to and from.                                   */
/*      len        number of bytes to transfre.                   */ 
/******************************************************************/
bus_transfer(har ,dest_addr, len)
   struct scsi_ha_reg  *har;
   int    len;
   char   *dest_addr;
{
   char message;
   int  c;
 
   if ((c = dev_init_sel(har)) != 0)
      return(c);

   har->icr &= ~ICR_SELECT;
 
   if ((c = put_byte(har, ICR_COMMAND, (char*)&scsi.cdb, sizeof scsi.cdb)) != 0)
      return(c);
 
  
   if ((scsi.cdb.cmd == SC_REQUEST_SENSE) || (scsi.cdb.cmd == SC_READ)){
      if ((c = get_byte(har, ICR_INPUT_OUTPUT, dest_addr, len)) != 0)
         return(c);
   }
 
   if (scsi.cdb.cmd == SC_WRITE){
      if ((c = put_byte(har, 0, dest_addr, len)) != 0)
         return(c);
   }
 
   if ((c = get_byte(har, ICR_STATUS, (char *)&scsi.scb, sizeof scsi.scb)) != 0)
      return(c);
 
   if ((c = get_byte(har, ICR_MESSAGE_IN,(char *)&message, 1)) != 0)
      return(c);
 
   if ((scsi.elog.code |= ((message!=SC_COMMAND_COMPLETE) ? BAD_MESS_IN : 0))){
      scsi.elog.act_val = (int)message & 0xFF;
      return(-1);
   }
   
   return(0);

}  






/******************************************************************/
/* compare_data function compares the data writen to and read back*/
/* from target device with the specified length.                  */
/*   Parameter list:                                              */
/*    _ ptr_wr_data  pointer to expected data area.               */
/*    _ ptr_rd_data  pointer to actual data area.                 */
/*    _ len          length of data.                              */
/******************************************************************/
compare_data(ptr_wr_data, ptr_rd_data, len)
   u_char *ptr_wr_data, *ptr_rd_data;
   int    len;
{
   int  i, *exp_val, *act_val;

   for (i = 0; i < len ; ptr_wr_data++, ptr_rd_data++, i++){
       if ((*ptr_wr_data & 0xFF) != (*ptr_rd_data & 0xFF)){
          scsi.elog.code |= DATA_MISS_COMP;
          exp_val           = (int*)((int)ptr_wr_data & 0xFFFFFC);
          act_val           = (int*)((int)ptr_rd_data & 0xFFFFFC);
          scsi.elog.exp_val = *exp_val ;
          scsi.elog.act_val = *act_val ;
          return(-1);
       }
   }
   return(0);
}






/******************************************************************/
/* dma_transfer function does data transfer using dma circuitry.  */
/* This is done by first select the target device, set up dma addr*/
/* and counter register, assert dma enable bit in ICR, and send   */
/* command block to target device. At completion of command xfer  */
/* the dma circuitry does bus request to cpu and inturn becomes   */
/* te bus master. Dma transfer is done throught dma address, dma  */
/* counter, bus abitration, and interrupt circuitries. At the end */
/* of dma transfer, the target device will generate an interrupt  */
/* request to indicate the dma transfer is completed and status   */
/* informations is available. Status and message in are read in   */
/* using program I/O protocall.                                   */
/*  If anything go wrong along the way, it immediately terminate  */
/* the transfer process, set up error code in scsi.elog structure */
/* and return a value of -1 to the calling function.              */
/*  Parameters list :                                             */
/*      har        pointer contain address to scsi host adp reg.  */
/*      dest_addr  destination address, where to transfer data    */
/*                 to and from.                                   */
/*      len        number of bytes to transfre.                   */
/******************************************************************/
dma_transfer(har, dest_addr, len)
   struct scsi_ha_reg  *har;
   int    dest_addr, len;
{
   char   message;
   int    c;
 
   if ((c = dev_init_sel(har)) != 0)
      return(c);

   har->dma_addr  = ((scsi.vme) ? dest_addr & 0xFFFFFF : dest_addr & 0xFFFFF);
   har->dma_count = ~len;
   har->icr       = ICR_WORD_MODE | ICR_DMA_ENABLE;
 
   if ((c = put_byte(har,ICR_COMMAND,(char *)&scsi.cdb,sizeof scsi.cdb)) != 0)
      return(c);

   if ((c = wait_cond(har,ICR_INTERRUPT_REQUEST)) != 0){
      scsi.elog.code |= ((scsi.elog.code&COND_BUS_ERR)?DMA_BUS_ERR:NO_INTR_REQ);
      scsi.elog.act_val = len - ~har->dma_count;
      scsi.elog.exp_val = len;
      return(c);
   }

   if ((har->dma_count != -1) && !(scsi.cdb.cmd == SC_REQUEST_SENSE &&
      (len - ~har->dma_count >= 3)) ){
      scsi.elog.act_val = len - ~har->dma_count;
      scsi.elog.exp_val = len;
      scsi.elog.code |= DMA_BAD_LEN;
      return(-1);
   }

   if ((c = get_byte(har, ICR_STATUS, (char*)&scsi.scb, sizeof scsi.scb)) != 0)
      return(c);
 
   if (((c = get_byte(har, ICR_MESSAGE_IN, (char*)&message, 1)) & 0xFF) != 0)
      return(c);
 
   if ((scsi.elog.code |= ((message!=SC_COMMAND_COMPLETE) ? BAD_MESS_IN : 0))){
      scsi.elog.act_val = (int)message & 0xFF;
      return(-1);
   }
   
   return(0);
}








/******************************************************************/
/*  This function does  device  selction sequence.  It is done by */
/* first set the host and target unit addresses in data register, */
/* assert the select bit in Interface control register, Then wait */
/* for the selected device to response with  busy  in Interface   */
/* control register.                                              */ 
/******************************************************************/
dev_init_sel(har)
   struct scsi_ha_reg  *har;
{
   int    c;
   
   har->icr  = ICR_RESET;           /* reset scsi control */
   DELAY(100);                      /* allow it catch its breath */
   har->icr  = 0;                   /* start off in known state */
   
   har->data = (1 << scsi.target) | SCSI_HOST_ADDR;
   har->icr  = ICR_SELECT;
   
   if ((c = wait_cond(har,ICR_BUSY)) == -1){ 
      scsi.elog.code    |= DEV_SEL_ERR;
      scsi.elog.exp_icr |= ICR_BUSY | ICR_SELECT;
      return(-1); 
   }
   
   return(0);
}





/******************************************************************/
/* This function saves the address of bus error interrupt handler */
/* routine and set up new one to call time_out function in case   */
/* of bus error.                                                  */
/******************************************************************/
exchg_buserr_handler()
{
   extern int time_out();

   scsi.sys_buserr_handler = ex_vector->e_buserr;
   scsi.pro_buserr_handler = time_out;
   ex_vector->e_buserr     = time_out;

}






/******************************************************************/
/* Get byte function is a program protocall to get data, status,  */
/* or message in information from target device.                  */
/* Parameter list :                                               */
/*   har          pointer to scsi internal registers.             */
/*   rep_type     indicates what type of request is being served. */
/*                Data in, Status in, or Message in.              */
/*   location     where the informations are to be store.         */
/*   len          the number of byte to get from device.          */ 
/******************************************************************/
get_byte(har, req_type, location, len)
   struct scsi_ha_reg  *har;
   char *location;
   register int req_type, len;
{
   int  i, c, icr;
   
   for ( i = 0; i < len; i++, location++){
   
      if (scsi.elog.code |= ((wait_cond(har,ICR_REQUEST)==-1)?GET_BYTE_ERR:0))
         break;  /* break the for loop. */
   
      icr = har->icr;
   
      if ( (icr & ICR_BITS) == req_type ){
   
         *location = ((req_type == ICR_INPUT_OUTPUT) ? har->data:har->cmd_stat);
   
      }else{
   
         if ((scsi.cdb.cmd == SC_REQUEST_SENSE)&&(req_type == ICR_INPUT_OUTPUT)
               &&((icr & ICR_BITS) == ICR_STATUS))
            break;  /* break the for loop. */
   
         if ((req_type == ICR_STATUS) && ((icr & ICR_BITS) == ICR_MESSAGE_IN))
            break;  /* break the for loop. */
   
         scsi.elog.code |= (GET_BYTE_ERR | XFER_LEN_ERR);
         break;  /* break the for loop. */
      }  
   }  
   
   if (scsi.elog.code & GET_BYTE_ERR){
      scsi.elog.exp_icr |= req_type | ICR_BUSY;
      scsi.elog.act_icr |= har->icr & 0xFFFF;
      scsi.elog.exp_val  = len;
      scsi.elog.act_val  = i;
      return(-1); /* return a -1 indicates error has occurred. */
   
   }
   
   return(0);
}







/******************************************************************/
/* Get_sense_info function does a request sense information from  */
/* target device.                                                 */
/*   Parameter list :                                             */
/*   har          pointer to scsi internal registers.             */
/******************************************************************/
get_sense_info(har, dest_ptr, len)
   struct scsi_ha_reg  *har;
   char   *dest_ptr;
   int    len;
{
   int    c;

   set_command(SC_REQUEST_SENSE, 0, 0);
   c = bus_transfer(har, dest_ptr, len);
   return(c);
}






/******************************************************************/
/* gen_rand_data functtion generates a block of random data in a  */
/* given lenght.                                                  */
/*   Parameter list :                                             */
/*      dest        location in memory where data are to be read  */
/*                  from.                                         */
/*      len         the length of the data field to be initialize.*/
/******************************************************************/
gen_rand_data(dest,len)
   register  u_char  *dest;
   register  int      len;
{
   int i;

   for (i = 0; i < len; i++, dest++)
      *dest = (char)(random() & 0xFF);

}






/******************************************************************/
/*  init_data initializes data patterns in given location and len */
/* of memory space.  These data patterns in memory are used to    */
/* transfer to target device and read back to compare.            */
/*   Parameter list :                                             */
/*      dest        location in memory where data are to be read  */
/*                  from.                                         */
/*      len         the length of the data field to be initialize.*/
/******************************************************************/
init_data(dest, len)
   register  u_char  *dest;
   register  int      len;
{
   int       i, width;

   width  =  len / 8 ;

   for (i = 0; i < width; i++, dest++)
      *dest = 0x00;

   for (; i < 2*width; i++, dest++)
      *dest = 0xFF;

   for (; i < 3*width; i++, dest++)
      *dest = 0x33;

   for (; i < 4*width; i++, dest++)
      *dest = 0xCC;

   for (; i < 5*width; i++, dest++)
      *dest = 0x55;

   for (; i < 6*width; i++, dest++)
      *dest = 0xAA;

   for (; i < 7*width; i++, dest++)
      *dest = i & 0xFF;

   for (; i < 8*width; i++, dest++)
      *dest = i & 0xFF;

}





/******************************************************************/
/* Initialize level interrupt to  take program's reentrance in an */
/* even of interrupt.                                             */
/******************************************************************/
intr_init(){

    int scsi_intr1(),scsi_intr2(),scsi_intr3(),scsi_intr4(),
        scsi_intr5(),scsi_intr6(),scsi_intr7();

    svect1 = IRQ1Vect;
    IRQ1Vect = (long)scsi_intr1;
    svect2 = IRQ2Vect;
    IRQ2Vect = (long)scsi_intr2;
    svect3 = IRQ3Vect;
    IRQ3Vect = (long)scsi_intr3;
    svect4 = IRQ4Vect;
    IRQ4Vect = (long)scsi_intr4;
    svect5 = IRQ5Vect;
    IRQ5Vect = (long)scsi_intr5;
    svect6 = IRQ6Vect;
    IRQ6Vect = (long)scsi_intr6;
    intlevel(0);
}






/******************************************************************/
/* Restore the original interrupt vector values.                  */
/******************************************************************/
intr_reset(){
    IRQ1Vect = svect1;
    IRQ2Vect = svect2;
    IRQ3Vect = svect3;
    IRQ4Vect = svect4;
    IRQ5Vect = svect5;
    IRQ6Vect = svect6;
    intlevel(7);
}
 




/******************************************************************/
/* In an even of interrupt, the interrupt vectors will pont to one*/
/* of the below reentrant function. While in reentrant function,  */
/* the interrupt level that caussed the interrupt in recorded in  */
/* global variable scsi.int_lv.                                   */
/******************************************************************/
reentrant(scsi_intr2){
   scsi.int_lv = 2;
   _longjmp(int2_buf);
}
reentrant(scsi_intr1){
   scsi.int_lv = 1;
   _longjmp(int2_buf);
}
reentrant(scsi_intr3){
   scsi.int_lv = 3;
   _longjmp(int2_buf);
}
reentrant(scsi_intr4){
   scsi.int_lv = 4;
   _longjmp(int2_buf);
}
reentrant(scsi_intr5){
   scsi.int_lv = 5;
   _longjmp(int2_buf);
}
reentrant(scsi_intr6){
   scsi.int_lv = 6;
   _longjmp(int2_buf);
}






/******************************************************************/
/*  Put_byte function is a program I/O protocall to send data out,*/
/* command, or massage out to target device.                      */
/* Parameter list :                                               */
/*   har          pointer to scsi internal registers.             */
/*   rep_type     indicates what type of request is being served. */
/*                Data out, command out, or Message out.          */
/*   location     where the informations are to be send to device.*/
/*   len          the number of byte to send to device.           */ 
/******************************************************************/
put_byte(har, req_type, location, len)
   struct scsi_ha_reg  *har;
   char *location;
   register int req_type, len;
{
   int i, c, icr;

   for ( i = 0; i < len; i++, location ++){
   
      if (scsi.elog.code |= ((wait_cond(har,ICR_REQUEST)==-1)?PUT_BYTE_ERR:0))
         break;  /* break the for loop. */
        
      icr = har->icr;
        
      if ( (icr & ICR_BITS) == req_type ){
        
         if (req_type == 0)
            har->data = *location;
         else
            har->cmd_stat = *location;
        
      }else{
        
         scsi.elog.code |= (PUT_BYTE_ERR | XFER_LEN_ERR);
         break;  /* break the for loop. */
        
      }  
   }   
        
   if (scsi.elog.code & PUT_BYTE_ERR){
      scsi.elog.exp_icr |= req_type | ICR_BUSY;
      scsi.elog.act_icr |= har->icr & 0xFFFF;
      scsi.elog.exp_val  = len;
      scsi.elog.act_val  = i;
      return(-1); /* return a -1 indicates error has occurred. */
   }
        
   return(0);
}







/******************************************************************/
/* This function decodes the command portion of the command des-  */
/* scription block into meaningfull messages.                     */
/******************************************************************/
prt_cmd(s)
   char *s;
{
   printf(s,scsi.cdb.cmd);
   switch (scsi.cdb.cmd){
      case 0x00 : printf("Test Unit Ready");
                  break;
      case 0x01 : printf("Rezero Unit");
                  break;
      case 0x03 : printf("Request Sense");
                  break;
      case 0x04 : printf("Format Unit");
                  break;
      case 0x05 : printf("Read Capacity");
                  break;
      case 0x08 : printf("Read");
                  break;
      case 0x0a : printf("Write");
                  break;
      case 0x0b : printf("Seek");
                  break;
      case 0x0e : printf("Write and Verify");
                  break;
      case 0x0f : printf("Verify");
                  break;
      case 0x10 : printf("Search Data High");
                  break;
      case 0x11 : printf("Search Data Equal");
                  break;
      case 0x12 : printf("Search Data Low");
                  break;
      case 0x16 : printf("Reverse Unit");
                  break;
      case 0x17 : printf("Release Unit");
                  break;
      case 0x1c : printf("Read Diagnostic");
                  break;
      case 0x1d : printf("Write Diagnostic");
                  break;
   }
}



/******************************************************************/
/* This fuction decodes bits in ICR into meaningfull message.     */
/******************************************************************/
prt_icr_bit(s,i)
   char *s;
   int  i;
{
   printf(s,i);
   if (i & ICR_PARITY_ERROR)
      printf("Par_err ");
   if (i & ICR_BUS_ERROR)
      printf("Bus_err ");
   if (i & ICR_ODD_LENGTH)
      printf("Odd_len ");
   if (i & ICR_INTERRUPT_REQUEST)
      printf("Int_req ");
   if (i & ICR_REQUEST)
      printf("Req ");
   if (i & ICR_MESSAGE)
      printf("Msg ");
   if (i & ICR_COMMAND_DATA)
      printf("c/d ");
   if (i & ICR_INPUT_OUTPUT)
      printf("i/o ");
   if (i & ICR_PARITY)
      printf("Parity ");
   if (i & ICR_BUSY)
      printf("Busy ");
   if (i & ICR_SELECT)
      printf("Sel ");
   if (i & ICR_RESET)
      printf("Reset ");
   if (i & ICR_PARITY_ENABLE)
      printf("Par_ena ");
   if (i & ICR_WORD_MODE)
      printf("Word_mode ");
   if (i & ICR_DMA_ENABLE)
      printf("Dma_ena ");
   if (i & ICR_INTERRUPT_ENABLE)
      printf("Int_ena ");
}
 




/**************************************************************/
/* Print sense function takes the information stored in sense */
/* struct (scsi.sense) and decodes it into meaningfull        */
/* messages.                                                  */
/**************************************************************/
prt_sense(s)
   char  *s;
{
   struct scsi_sense7 *sense7;

   printf(s);
   printf("\n\t address val = %d %s", scsi.sense.adr_val,
         scsi.sense.adr_val ? "" : "probable bad format.");
   if (scsi.sense.class <= 6){
      printf("\n\t error class = %d", scsi.sense.class);
      printf("\n\t error code  = %d", scsi.sense.code);
      printf("\n\t log blk adr = %d", (scsi.sense.high_addr << 16) |
            (scsi.sense.mid_addr << 8) | scsi.sense.low_addr );
      if (scsi.sense.code < SC_errct[scsi.sense.class])
         printf("\n\t %s",SC_errors[scsi.sense.class][scsi.sense.code]);
   }else{
      if (scsi.sense.class == 7){
         sense7 = (struct scsi_sense7 *)&scsi.sense;
         printf("\n\t error class   = 7 ");  
         printf("\n\t file mark     = %d", sense7->fil_mk);
         printf("\n\t end of medium = %d", sense7->eom);
         printf("\n\t sense key     = %x ", sense7->key);
         if (sense7->key < sizeof SC_sense7_keys / sizeof SC_sense7_keys[0])
            printf("%s", SC_sense7_keys[sense7->key]);
         else
            printf("invalid key.");
         printf("\n\t log blk addr  = %d", (sense7->info_1 << 24) |
               (sense7->info_2 << 16) | (sense7->info_3 << 8) |
               sense7->info_4);   
      }else{
         printf("\n\t Invalid error class in sense info :");
         printf("\n\t error class   = %d", scsi.sense.class);
      }  
   }  
}







/******************************************************************/
/* This function restores the original bus error interrupt handler*/
/******************************************************************/
restore_sys_buserr_handler()
{
   ex_vector->e_buserr = scsi.sys_buserr_handler;
}





/******************************************************************/
/*  Set command  function  set up  the command description block  */
/* using parameters passed  over from caller function  before     */
/* sending it to the device. Follow is the parameters list and    */
/* their functions :                                              */
/*     _ cmd       the command.                                   */
/*     _ blk_addr  the starting block address.                    */
/*     _ blk_count the block count.                               */
/******************************************************************/
set_command(cmd, blk_addr, blk_count)
   int      cmd, blk_addr, blk_count;
{
   bzero((char *)&scsi.cdb, sizeof scsi.cdb);  /* zero out command block */
   scsi.cdb.cmd = cmd;
   scsi.cdb.lun = scsi.unit;
   scsi.cdb.low_addr  = ( blk_addr & 0xFF);
   scsi.cdb.mid_addr  = ((blk_addr & 0xFF00) >> 8);
   scsi.cdb.high_addr = ((blk_addr & 0xFF0000) >> 16);
   scsi.cdb.count     = blk_count;
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
   if(!breg.berr_field.berr_pagevalid) scsi.berr_tp = bus_err_type[0];
   if(breg.berr_field.berr_busmaster)  scsi.berr_tp = bus_err_type[1];
   if(breg.berr_field.berr_proterr)    scsi.berr_tp = bus_err_type[2];
   if(breg.berr_field.berr_timeout)    scsi.berr_tp = bus_err_type[3];
   if(breg.berr_field.berr_parerru)    scsi.berr_tp = bus_err_type[4];
   if(breg.berr_field.berr_parerrl)    scsi.berr_tp = bus_err_type[5];
   _longjmp(bus_error);
}






/******************************************************************/
/*  Wait for a condition to be meet on scsi bus. This is done by  */
/*  constantly reading the Interface control register and compare */
/*  the result with the condition passed by caller. The condition */
/*  can be one of the follow :  busy, request, or interrupt req.  */
/******************************************************************/
wait_cond(har,cond)
   struct scsi_ha_reg  *har;
   int    cond;
{
   int    i, icr = 0;

   for (i = 0; i <= 20000; i++){

      DELAY(100);
      icr = har->icr;

      if (icr & ICR_BUS_ERROR)
         break;  /* break the for loop. */

      if (cond == (icr & cond))
         return(0) ;  /* condition has met. */

   }

   scsi.elog.code   |= ((icr & ICR_BUS_ERROR) ? COND_BUS_ERR  :  COND_NOT_MET);
   scsi.elog.exp_icr = cond;
   scsi.elog.act_icr = icr & 0xFFFF;
   return(-1);
}



/*****************************************************************/
/* reset the scsi bus                                            */
/*****************************************************************/
clear_bus(har)
   struct scsi_ha_reg  *har;
{

   har->icr  = ICR_RESET;           /* reset scsi control	 */
   DELAY(100);                      /* allow it catch its breath */
   har->icr  = 0;                   /* start off in known state  */

}
