/******************************************************************************/
/*   THIS FILE CONTAINS UART'S TESTS AND TOOLS FOR SCSI BOARD.                */
/*   RESPONSIBLE ENGINERR :  HAI THE NGO                                      */
/*   Follow is a list of all functions include in this file:                  */
/*     _ test_SCC      test SCC ports.                                        */
/*     _ set_scc_par   set up scc's parameters.                               */
/*     _ scc_auto_test      run scc test in automatis mode( go, no go).       */
/*     _ scc_manual_test    run scc test in manual mode (menu driven).        */
/*     _ test_time_const    test time constant register WR12 and WR13.        */
/*     _ scc_data_xfer      data transfer using external loop back.           */
/*     _ scc_putbyte   transfer a byte of data.                               */
/*     _ scc_getbyte   receive a byte of data.                                */
/*     _ scc_init      initialize scc's port for external loop back.          */
/*     _ prt_scc_error      print scc's error.                                */
/*     _ prt_scc_dt_ovrr    print data overrun error.                         */
/*     _ prt_scc_rx_perr    print receive parity error.                       */
/*     _ prt_scc_rx_tout    print receive time out error.                     */
/*     _ prt_scc_tc_err     print time constant register error.               */
/*     _ prt_scc_tx_rx_err  print transmite and receive error.                */
/*     _ prt_scc_tx_tout    print transmite time out error.                   */
/*     _ prt_scc_wr_err     print scc's interrupt vector register.            */
/*     _ tool_scc      debug tool for scc ports.                              */
/*     _ set_scc_port  set scc's port.                                        */
/*     _ loop_rw_WR    loop on read/write to scc's register.                  */
/*     _ loop_scc_xfer loop on scc's external data transfer.                  */
/******************************************************************************/


static  char    sccsid_scc[] = "@(#)sc_scc.c 1.1 86/09/25 SMI." ;

#include    <sys/types.h>
#include    <machdep.h>
#include    <setjmp.h>
#include    <zsreg.h>
#include    <s2addrs.h>
#include    "sc_reg.h"

 
#define TCl(baud)       (((4915200/(2*baud*16)) - 2) & 0xff)
#define TCu(baud)       (((4915200/(2*baud*16)) - 2) >> 8 & 0xff)
 

/* SCC error code definition. */
#define     SCC_WR_ERR        0x0001
#define     SCC_TC_ERR        0x0002
#define     SCC_TX_TOUT       0x0004
#define     SCC_RX_TOUT       0x0008
#define     SCC_RX_PERR       0x0010
#define     SCC_DT_OVRR       0x0020
#define     SCC_FR_ERR        0x0040
#define     SCC_TX_RX_ERR     0x0080




extern jmp_buf  get_out, bus_error;
extern struct scsi_par scsi;            /* SCSI parameters.                   */
extern end;






/**************************************************************/
/* UART register addressing:  It would be nice if they used 4 */
/*address pins to address 15  registers, but they only used 1.*/
/*So you have to write to the control port then read or write */
/*it;  the 2nd cycle is done to whatever register number  you */
/*wrote in the first  cycle.  The  data register  can also be */
/*accessed as Read/Write register 8.                          */
/**************************************************************/
struct scc_port {
        unsigned char   cntrl;
        unsigned char   :8;
        unsigned char   data;
        unsigned char   :8;
};




/**************************************************************/
/* scc_par structure contains the port1 address and  ID,  and */
/* port2 addrss and  ID.                                      */ 
/**************************************************************/
struct scc {

     struct    scc_port  *p1addr;
     u_char    p1;
     struct    scc_port  *p2addr;
     u_char    p2;

}    scc_par[4] ;






/**************************************************************/
/* scc_elog is the error log structure which contains  scc's  */
/* error code, name of the failing ports, name of failing reg,*/
/* actual and expected values, and the baud rate under which  */
/* the failure occurred.                                      */ 
/**************************************************************/
struct scc_err_log {

     u_short   code;        
     u_char    port1;   
     u_char    port2;  
     u_char    reg;   
     u_char    act_val;
     u_char    exp_val;
     int       baud;     

}    scc_elog ;   




/**************************************************************/
/* This is the declaration for SCC test menu.                 */
/**************************************************************/
char  *scc_test_menu[] = {
     "Test Uart1 port A.",   
     "Test Uart1 port B.",  
     "Test Uart0 port C.", 
     "Test Uart0 port D.",  
     "Test all SCC ports.",  
     "go to top menu.",  
     0,
};





/**************************************************************/
/*  menu_scc_tool is an debug menu for SCC.                   */
/**************************************************************/
char *menu_scc_tool[] = {
     "change test pattern.",
     "change baud rate.",
     "select SCC reg.",
     "select SCC port 1.",
     "select SCC port 2.",
     "loop on w/r to SCC reg (port1).",
     "loop on SCC xfer (port1 -> port2).",
     "go to top menu.",
     0,
};



/**************************************************************/
/* scc_baud  contains  all the  buad rates  that are used for */
/* SCC's  testing.                                            */
/**************************************************************/
int  scc_baud[] = {
     300,
     600,
     1200,
     2400,
     4800,
     9600,
     19200,
     38400,
     0
};
     

/**************************************************************/
/* The following predefined SCC initial sequence is used to   */
/* initialize the SCC ports under test for external loop back.*/ 
/**************************************************************/
u_char  UAInSeq[] = {
     
        /* Set up all the elements on the chip: */
        9,      ZSWR9_RESET_WORLD,      /* Reset the world first.           */
        4,      ZSWR4_PARITY_EVEN|      /* Async mode, etc, etc, etc.       */
                ZSWR4_1_STOP|
                ZSWR4_X16_CLK,
        3,      ZSWR3_RX_8,             /* 8-bit chars, no auto CD/CTS shake*/
        5,      ZSWR5_RTS|              /* Set RTS/DTR, xmit 8-bit chars.   */
                ZSWR5_TX_8|
                ZSWR5_DTR,
        9,      ZSWR9_NO_VECTOR,        /* Don't try to respond to intacks. */
        11,     ZSWR11_TRXC_XMIT|       /* Output xmitter clock.            */
                ZSWR11_TRXC_OUT_ENA|
                ZSWR11_TXCLK_BAUD|
                ZSWR11_RXCLK_BAUD,
        12,     TCl(9600),              /* Default baud rate.               */
        13,     TCu(9600),              /* Ditto.                           */
        14,     ZSWR14_BAUD_FROM_PCLK,  /* Baud Rate Gen source = CPU clock.*/
     
        /* Now enable the various set-up elements: */
        3,      ZSWR3_RX_8|             /* Enable receiver.                 */
                ZSWR3_RX_ENABLE,
        5,      ZSWR5_RTS|              /* Enable transmitter.              */
                ZSWR5_TX_ENABLE|
                ZSWR5_TX_8|
                ZSWR5_DTR,
        14,     ZSWR14_BAUD_ENA|        /* Enable baud rate generator.      */
                ZSWR14_BAUD_FROM_PCLK,
     
        /* Now clear out assorted garbage */
        0,      ZSWR0_RESET_STATUS |    /* Reset status latches.            */
                ZSWR0_RESET_ERRORS, 
        0,      ZSWR0_RESET_STATUS |    /* Reset status latches again.      */ 
                ZSWR0_RESET_ERRORS, 
};


/**************************************************************/
/* 12_ TEST SCSI SCC :                                        */
/*   This test is run only if the board under test is a Multi-*/
/* bus SCSI board. Before and testing can be done, set_scc_par*/
/* function is call to set up MMU mapping to the SCC chips on */
/* this board. Afterward, SCC testing is done by writing and  */
/* reading test patterns in to the time constant register, and*/
/* do data transfer from port1 to port2 of the SCC chip using */
/* external loop back. An special made cable is required.     */
/* This test does not check the full functionality of the SCC */
/* chip! but at least it ensures that the interfase signals to*/
/* and from the the chip are okay...  ( another  version of   */
/* this scc test, which includes scc functionality tests, may */
/* be needed later down the road).                            */
/**************************************************************/

test_SCC()
{
   int      c;

   if (scsi.vme){  
      printf("  skipped"); 
      return(0);
   }

   if (_setjmp(bus_error)){ 
      scsi.elog.code |= GOT_BUS_ERR; 
      return(-1); 
   }

   bzero((u_short *)&scsi.elog, sizeof scsi.elog);

   set_scc_par();

   if (scsi.all)
      c = scc_auto_test();
   else
      c = scc_manual_test();

   scsi.elog.code |= ((c != 0) ? SCC_ERROR : 0);

   return(c);
}



 
/**************************************************************/
/* This function set up all parameters in scc_par which are   */
/* going to be used by the SCC test.                          */ 
/**************************************************************/
set_scc_par()
{
   int  scc_addr;

   scc_addr = PTOB(BTOP(&end + 0x1000 + PAGEMASK));
   map(scc_addr, 4*PAGESIZE, 0x80800, PM_BUSMEM);

   scc_par[0].p1addr = (struct scc_port *)(scc_addr + PAGESIZE);
   scc_par[0].p2addr = (struct scc_port *)(scc_addr + PAGESIZE + 4);
   scc_par[1].p1addr = (struct scc_port *)(scc_addr + PAGESIZE + 4);
   scc_par[1].p2addr = (struct scc_port *)(scc_addr + PAGESIZE);
   scc_par[2].p1addr = (struct scc_port *)(scc_addr);
   scc_par[2].p2addr = (struct scc_port *)(scc_addr + 4);
   scc_par[3].p1addr = (struct scc_port *)(scc_addr + 4);
   scc_par[3].p2addr = (struct scc_port *)(scc_addr);
   scc_par[0].p1     = 0x0A;
   scc_par[0].p2     = 0x0B;
   scc_par[1].p1     = 0x0B;
   scc_par[1].p2     = 0x0A;
   scc_par[2].p1     = 0x0C;
   scc_par[2].p2     = 0x0D;
   scc_par[3].p1     = 0x0D;
   scc_par[3].p2     = 0x0C;

} 
 
 
 



/**************************************************************/
/*  scc_auto_test function runs scc'c tests on all the ports. */
/* If one of the serial port has problem, scc testing will be */
/* terminate  and a value of  -1  is return  to the  calling  */
/* function, otherwise  0  will be return to indicates that   */
/* all scc tests have run succesfully.                        */
/**************************************************************/
scc_auto_test()
{
   struct  scc  *scc;
   int     c, i, j;

 
   for ( i = 0; i <= 3; i++){
      scc = &scc_par[i]; 
      if ((c = test_time_const(scc->p1addr)) != 0){
         scc_elog.port1 = scc->p1;
         return(c);
      }
   }

   for ( i = 0; scc_baud[i] != 0; i++){

      for ( j = 0; j <= 3; j++){
         scc = &scc_par[j]; 

         if ( (scsi.all && (scsi.pass_num == 1)) || !scsi.all )
            printf("\r  %d Testing SCC transfer,(%d)p%x -> p%x.",
                  ((scsi.all) ? 12 : j+1), scc_baud[i], scc->p1, scc->p2);
         
         if ((c = scc_data_xfer(scc->p1addr, scc->p2addr, scc_baud[i])) != 0){
            scc_elog.port1 = scc->p1;
            scc_elog.port2 = scc->p2;
            scc_elog.baud  = scc_baud[i];
            break;
         } 
      }

   }

   if (scsi.all && (c == 0))  printf("  passed ");
   return(c);
}





/**************************************************************/
/*  This function prints SCC's tests menu and ask the user to */
/* select which scc port to test. If test scc ports option is */
/* selected, it calls scc_auto_test function which increment  */
/* throught all of the scc ports, otherwise the selected port */
/* is called to execute individually.                         */ 
/**************************************************************/
scc_manual_test()
{
   struct  scc  *scc;
   int     c, i, num;
   char    buf[LINEBUFSZ];

   while(1){

      num = prt_get_option(scc_test_menu, "\n\nSCC TEST MENU", " ");

      if (num == 6)  return(0);

      if (num == 5){ 
         c = scc_auto_test();
      }else{
   
         scc = &scc_par[--num];
         num++;
         if ((c = test_time_const(scc->p1addr)) != 0){
            scc_elog.port1 = scc->p1;
            return(c);
         }

         for ( i = 0; scc_baud[i] != 0; i++){

            if ( (scsi.all && (scsi.pass_num == 1)) || !scsi.all )
               printf("\r  %d Testing SCC transfer,(%d)p%x -> p%x.",
                     ((scsi.all) ? 12 : num), scc_baud[i], scc->p1, scc->p2);
         
            if ((c = scc_data_xfer(scc->p1addr, scc->p2addr, scc_baud[i]))!= 0){
               scc_elog.port1 = scc->p1;
               scc_elog.port2 = scc->p2;
               scc_elog.baud  = scc_baud[i];
               break;
            }
         } 
      }
    
      if (c != 0) return(c); else printf("  passed ");
   }
} 






/**************************************************************/
/* test_time_const function does the read / write to the high */
/* and low time constant registers.                           */
/**************************************************************/
test_time_const(port)
   struct scc_port *port;
{
   register char exp_val, act_val;
   u_char   reg;

   for (reg = 12; reg < 14; reg++){
      for(exp_val = 0x00; exp_val < 0xFF; exp_val++) {
         act_val      =  port->cntrl; /* ensure pointing to WR0   */
         port->cntrl  =  reg;         /* select register          */
         port->cntrl  =  exp_val;     /* write valid data to reg  */
         act_val      =  port->cntrl; /* ensure pointing to WR0   */
         port->cntrl  =  reg;         /* select register          */
         act_val      =  port->cntrl; /* read valid data from reg */
         if(act_val  != exp_val) {
            scc_elog.code   |= SCC_TC_ERR; 
            scc_elog.reg     = reg;
            scc_elog.exp_val = exp_val & 0xFF;
            scc_elog.act_val = act_val & 0xFF;
            return(-1);
         }
      }
   }
   return(0);
}   
 
 



/**************************************************************/
/* scc_data_xfer function does data transfer from port 1 to   */
/* port 2 using external loop back.                           */
/**************************************************************/
scc_data_xfer(port1, port2, baud)
   struct scc_port *port1, *port2;
   u_long baud;
{
   u_char exp_val, act_val;
   int  c;

   scc_init(port1, port2, baud);

   for (exp_val = 0; exp_val < 0xFF; exp_val++){
   
      if ((c = scc_putbyte(port1, exp_val)) != 0) break; 
 
      if ((c = scc_getbyte(port2, &act_val)) != 0) break; 

      if (c = (((exp_val & 0xFF) != (act_val & 0xFF)) ? -1 : 0)){
         scc_elog.code   |= SCC_TX_RX_ERR;
         break;
      } 
   }
 
   if ( c == -1 ){
      scc_elog.exp_val = exp_val & 0xFF;
      scc_elog.act_val = act_val & 0xFF;
   }
 
   return(c);
}





/**************************************************************/
/* scc_putbyte checks for transfer ready bit in register 0 and*/
/* writes the data into data register of the given port.  An  */
/* error is acknowledge if transfer ready signal is not active*/
/* after some delay.                                          */  
/**************************************************************/
scc_putbyte(port, val)
   struct scc_port *port;
   u_char val;
{
   register int    time;

   for (time = 0; (port->cntrl & ZSRR0_TX_READY) == 0; time++){
      if (time >= 4096){
         scc_elog.code   |= SCC_TX_TOUT;
         return(-1);
      }
   }

   port->data = val;
   return(0);
}




/**************************************************************/
/* scc_getbyte reads a byte from SCC data register. Error is  */
/* detected if not getting receive ready after some delay or  */
/* if control register has parity or data overrun bit on.     */
/**************************************************************/
scc_getbyte(port, val) 
   struct scc_port *port;
   u_char *val;
{ 
   register int    time;
   u_char   status;
 
   port->cntrl = 1;
   status      = port->cntrl;  /* read status back. */
  
   if (scc_elog.code |= ((status & ZSRR1_PE) ? SCC_RX_PERR :
      ((status & ZSRR1_DO) ? SCC_DT_OVRR : 0x00)))
      return(-1);
  
   port->cntrl = 0; 
   for (time = 0; (port->cntrl & ZSRR0_RX_READY) == 0 ; time++){
      if (time >= 4096){
         scc_elog.code |= SCC_RX_TOUT;
         return(-1);
      }
   }  
 
   *val = port->data;
   
   port->cntrl = 0;
   port->cntrl = ZSWR0_RESET_ERRORS;
   return(0);
}
    




/**************************************************************/
/* scc_init function initializes serial ports 1 and 2 for data*/
/* transfer.                                                  */
/**************************************************************/
scc_init(p1, p2, baud)
   register struct scc_port *p1, *p2;
   register u_long baud;
{
   register u_char *initp;
   u_char   temp;        
 
   temp      = p1->cntrl;       /* ensure poiting to WR0 */ 
   p1->cntrl = 9;
   p1->cntrl = ZSWR9_RESET_WORLD;  /* Reset Uart. */
  
   /* initialize port 1. */ 
   for (initp = UAInSeq + 2; initp < &UAInSeq[sizeof(UAInSeq)]; ){
      if ( *initp == 12){
         p1->cntrl = 12;        /*get to WR12, lower byte of time constant.*/
         p1->cntrl = TCl(baud); /*store lower byte of baud rate into WR12. */
         p1->cntrl = 13;        /*get to WR13, upper byte of time constant.*/
         p1->cntrl = TCu(baud); /*store upper byte of baud rate into WR12. */
         initp += 4; 
      }else{
         p1->cntrl = *initp++;
         p1->cntrl = *initp++;
      }
   }
   
   /* initialize port 2. */ 
   temp      = p2->cntrl; /* ensure poiting to WR0 */ 
   for (initp = UAInSeq + 2; initp < &UAInSeq[sizeof(UAInSeq)]; ){
      if ( *initp == 12){
         p2->cntrl = 12;        /*get to WR12, lower byte of time constant.*/
         p2->cntrl = TCl(baud); /*store lower byte of baud rate into WR12. */
         p2->cntrl = 13;        /*get to WR13, upper byte of time constant.*/
         p2->cntrl = TCu(baud); /*store upper byte of baud rate into WR12. */
         initp += 4; 
      }else{ 
         p2->cntrl = *initp++;
         p2->cntrl = *initp++;
      }
   }        
}





/**************************************************************/
/*   This is the declaration of  SCC's error codes to error   */ 
/* print functions. Each of the predefined error code has a   */
/* corresponding  error print function which prints out the   */
/* error messages associating with that error code.           */ 
/**************************************************************/
int   prt_scc_dt_ovrr(),  prt_scc_fr_err(),  prt_scc_rx_perr(),
      prt_scc_rx_tout(),  prt_scc_tc_err(),  prt_scc_tx_rx_err(),
      prt_scc_tx_tout(),  prt_scc_wr_err();

struct  scc_ecode_to_message {
 
      int     ecode;
      int     (*prt_err_funct)();
 
}     scc_etm[] = {

      SCC_DT_OVRR,      prt_scc_dt_ovrr,
      SCC_RX_PERR,      prt_scc_rx_perr,
      SCC_RX_TOUT,      prt_scc_rx_tout,
      SCC_TC_ERR,       prt_scc_tc_err,
      SCC_TX_RX_ERR,    prt_scc_tx_rx_err,
      SCC_TX_TOUT,      prt_scc_tx_tout,
      SCC_WR_ERR,  	prt_scc_wr_err,
      0, 
};

/**************************************************************/
/* This function checks the scc_elog.code  for  all possible  */
/* errors  by comparing it  with all of the predefined scc's  */
/* error codes. If a match is found, the corresponding error  */
/* print function is called to do error print.                */ 
/**************************************************************/
prt_scc_error()
{
   struct  scc_ecode_to_message   *eptr;
 
   for (eptr = &scc_etm[0]; eptr->ecode != 0; eptr++)
      if (scc_elog.code & eptr->ecode)
         (*eptr->prt_err_funct)();
 
}






/**************************************************************/
/*   SCC_DT_OVRR  error code  is set if  there is an overrun  */
/*  condition detected during serial data transfer.           */ 
/**************************************************************/
prt_scc_dt_ovrr()
{
   printf("\n\t SCC data overrun condition detected.");
} 






/**************************************************************/
/*  SCC_RX_PERR   error code  is set if an  parity error is   */
/* detected  during data transfer.                            */
/**************************************************************/
prt_scc_rx_perr()
{
   printf("\n\t Received an parity in dication in  XXXX register");
   printf("\n\t during data transfer.                           ");
}




/**************************************************************/
/* SCC_RX_TOUT  error code is set if the receiving port does  */
/* not have ZSRR0_RX_READY (receive ready) in the control reg */
/* during data transfer.                                      */ 
/**************************************************************/
prt_scc_rx_tout()
{
   printf("\n\t SCC PORT =  %x ",scc_elog.port2);
   printf("\n\t TIME OUT on waiting for RECEIVE READY signal.");
}




/**************************************************************/
/*  SCC_TC_ERR code is set if the values writen to and read   */
/* back from the high or low constant register miss-compared. */
/**************************************************************/
prt_scc_tc_err()
{
   printf("\n\t SCC PORT =  %x ",scc_elog.port1);
   printf("\n\t SCC REG  =  WR%d ",scc_elog.reg);
   printf("\n\t Expected and actual values on write/read to the %s byte",
             ((scc_elog.reg == 12) ? "low" : "high") );
   printf("\n\t of TIME CONSTANT reg (WR%d) not equal.", scc_elog.reg);
   printf("\n\t Expect value  =  0x%x ",scc_elog.exp_val & 0xFF);
   printf("\n\t Actual value  =  0x%x ",scc_elog.act_val & 0xFF);
}





/**************************************************************/
/*  SCC_TX_RX_ERR  error code is set if the data transmitted  */
/* and received throuht external loop back miss-compared.     */
/**************************************************************/
prt_scc_tx_rx_err()
{
   printf("\n\t SCC PORT1 =  %x ",scc_elog.port1);
   printf("\n\t SCC PORT2 =  %x ",scc_elog.port2);
   printf("\n\t Data transmitted from port %x and received by",scc_elog.port1);
   printf("\n\t port %x not equal.", scc_elog.port2);
   printf("\n\t Expect value  =  0x%x ",scc_elog.exp_val);
   printf("\n\t Actual value  =  0x%x ",scc_elog.act_val);
}





/**************************************************************/
/* SCC_TX_TOUT error code is set if the transmitting port does*/
/* not have ZSRR0_TX_READY (transmit ready) in the control reg*/
/* during data transfer.                                      */ 
/**************************************************************/
prt_scc_tx_tout()
{
   printf("\n\t SCC PORT =  %x ",scc_elog.port1);
   printf("\n\t TIME OUT on waiting for TRANSMIT READY signal.");
}





/**************************************************************/
/*  SCC_WR_ERR code is set if the values writen to and read   */
/* back from the interrupt register  miss-compared.           */
/**************************************************************/
prt_scc_wr_err()
{
   printf("\n\t SCC PORT =  %x ",scc_elog.port1);
   printf("\n\t SCC REG  =  WR%d ",scc_elog.reg);
   printf("\n\t Expected and actual values on write/read to SCC's");
   printf("\n\t interrupt vector register not equal.");
   printf("\n\t Expect value  =  0x%x ",scc_elog.exp_val & 0xFF);
   printf("\n\t Actual value  =  0x%x ",scc_elog.act_val & 0xFF);
}






/**************************************************************/
/* tool_scc is a debug tools for SCC problem. Follow is the   */
/* of all the options available to the user :                 */
/*      _ change failing test pattern,                        */
/*      _ change the baud rate,                               */
/*      _ select which register to work on (low or high TC),  */
/*      _ select which SCC port t work on,                    */
/*      _ loop on read/write to the selected register, and    */
/*      _ loop on data transfer from port 1 to port 2.        */
/**************************************************************/
tool_scc()
{
   register  u_char  test_val;
   u_char    p1, p2, reg;
   int       baud_rate, num, time;
   struct    scc_port   *port1, *port2;

   baud_rate= scc_elog.baud; 
   test_val = scc_elog.exp_val & 0xFF;
   reg      = scc_elog.reg &0xFF;
   p1       = scc_elog.port1;
   p2       = scc_elog.port2;
 
   set_scc_port(&port1, p1);
   set_scc_port(&port2, p2);

   printf("\n port1 addr = 0x%x  port2 addr = 0x%x", port1,port2); /*DEGUB*/

   if ((scc_elog.code == SCC_WR_ERR)||(scc_elog.code == SCC_TC_ERR)){
      printf("\n  SCC port1 is default to %x ", scc_elog.port1);
      printf("\n  SCC reg   is default to %x ", reg & 0xFF);
      printf("\n  Read/write to reg WR%d of port1.", reg);
   }else{
      printf("\n  SCC port1 is default to = %x ", scc_elog.port1);
      printf("\n  SCC port2 is default to = %x ", scc_elog.port2);
      printf("\n  Baud rate is default to = %d ", scc_elog.baud);
      printf("\n  SCC data transfer from port1 to port2.");
   }
   printf("\n  Failing  test pattern =  0x%x ",test_val);
   printf("\n  Test patterns are mask with 0xFF.");

 
   while (1){
 
      num = prt_get_option(menu_scc_tool, "\n\nSCC DEBUG MENU", " ");
 
      switch (num) {
         case 1 :  test_val = (pgetnh("\n  Enter test pattern in hex = 0x"));
                   test_val &= 0xFF;
                   printf("\n  test pattern is set to 0x%x", test_val);
                   break;

         case 2 :  baud_rate = (pgetn("\n  Enter baud rate  "));
                   printf("\n  Baud rate is set to %d", baud_rate);
                   break;

         case 3 :  reg = (pgetn("\n  Enter scc register  :  WR") & 0xFF);
                   printf("\n  selected register  =  WR%d", reg);
                   break;

         case 4 :  p1  = (pgetnh("\n  Enter selected scc port1  "));
                   set_scc_port(&port1, p1);
                   printf("\n  SCC port1 is set to  %x ", p1 & 0xFF);
                   break;

         case 5 :  p2  = (pgetnh("\n  Enter selected scc port2  "));
                   set_scc_port(&port2, p2);
                   printf("\n  SCC port2 is set to  %x ", p2 & 0xFF);
                   break;

         case 6 :  printf("\n  Looping on w/r 0x%x to WR%d of port_%x ", 
                               test_val, reg, p1);
                   loop_rw_WR(port1, reg, test_val);
                   break;

         case 7 :  printf("\n  Looping on SCC xfer 0x%x from port%x to port%x",
                               test_val, p1, p2);
                   printf("\n  Baud rate is set to %d", baud_rate);
                   loop_scc_xfer(port1, port2, baud_rate, test_val);
                   break;
 
         case 8 :  return;
      }  
   }   
}





/**************************************************************/
/* this function is used by scc_tool to get the port address  */
/* given that it know the port id.                            */
/**************************************************************/
set_scc_port(port, port_id)
   struct scc_port  **port;
   u_char port_id;
{
   struct scc  *scc;
   int    i;

   for (i = 0, scc = &scc_par[0]; i < 4; i++, scc++){ 
      if (scc->p1 == port_id)
         *port = scc->p1addr;
   }
}





/**************************************************************/
/* this function loops on read/write to time constant register*/
/**************************************************************/
loop_rw_WR(port, reg, test_val)
   struct scc_port  *port;
   u_char    reg;
   register u_char test_val;
{
   register u_char act_val;

   printf("\n  Hit control c to get out of loop.");
 
   while(maygetchar() != ('c' & 037)){
      act_val      =  port->cntrl; /* ensure pointing to WR0  */
      port->cntrl  =  reg;         /* select register         */
      port->cntrl  =  test_val;    /* write valid data to reg */
      act_val      =  port->cntrl; /* ensure pointing to WR0  */
      port->cntrl  =  reg;         /* select register         */
      act_val      =  port->cntrl; /* read valid data from reg*/
   }
}
 
 
 


/**************************************************************/
/* this function loops on serial data transfer from port1 to  */
/* port 2 with a given baud rate.                             */
/**************************************************************/
loop_scc_xfer(port1, port2, baud, exp_val)
   struct scc_port  *port1, *port2;
   u_char exp_val;
   int    baud;
{
   u_char act_val;
   int   c;
 
   scc_init(port1, port2, baud);
   printf("\n  Hit control c to get out of loop.");
 
   while(1){
      top:
 
      if (maygetchar() == ('c' & 037)) break; 
      
      if ((c = scc_putbyte(port1, exp_val)) != 0)
         goto top;

      scc_getbyte(port2, &act_val);
   }
}
