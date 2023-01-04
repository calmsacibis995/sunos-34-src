
static char     sccsid_h[] = "@(#)vme_adap.h 1.1 9/25/86 Copyright Sun Micro";

#define VME_ADAP_BASE	SCSI_BASE
#define VME_PHYS_ADRS   ((char *) 0x280000)
#define LINEBUFSZ       81

#define     MAX(a,b)            ((a) > (b) ? (a) : (b))

#define     DBUF_PA             0x0
#define     DBUF_VA             (u_char *)(MBMEM_BASE+DBUF_PA)



/*
 * Modified SCSI Programable DMA.
 * Sun host adapter control registers.
 */

struct  prog_dma_reg {          /* host adapter (I/O space) registers */
        u_short  data;           /* data register */
        u_short  unused0;
        u_short  pcr;            /* programmable dma control register */
        u_short  unused1;
        u_long   dma_addr;       /* dma base address */
        u_short  dma_count;      /* dma count register */
	u_long   unused2;
        u_short  ivr;            /* non-existant imaginary intr vec reg */
};


 
struct menu {
     int    test_num;
     char   *name;
     int    (*pd_test_ptr)();
};


/*
 * bits in the programmable dma control register
 */

#define  PD_INTERRUPT_ENABLE	0x0001
#define  PD_DMA_ENABLE          0x0002 
#define  PD_WORD_MODE           0x0004
#define  PD_DIRECTION  	        0x0008	/* 0-- write to mem, 1--read fr mem */
#define  PD_RESET               0x0010
#define  PD_COUNT_MODE      	0x0020  /* if set, cnt reg is src to wr to mem*/
#define  PD_BUS_ERROR 		0x4000



/*
 * the REG_VAL structure will hold the actual regiter values and
 * the expected values if needed.
 */
struct reg_val {
	u_short		a_pcr;
	u_short         e_pcr;
	u_short         a_data;
	u_short         e_data;
	u_short         a_count;
	u_short         e_count;
	u_long		a_dma;
	u_long          e_dma;
};



/*
 * PD_ERR_LOG structure will save the following data when
 * error happens
 */
struct pd_err_log {
       int       code;                  /* errlog structure      */
       int       exp_val;
       int       act_val;
       int       wr_flag;               /* 0, write 1, read err  */
       int	 reg_in_err;		
					/* register in error:
					 * 0, data reg.
					 * 1, control reg.
					 * 2, count reg.
					 * 3, dma addr reg.
					 */
};


struct  vme_adap_par {
 				 	    /* PDMA'S REGISTIERS ADDRESS.   */
     struct prog_dma_reg     *pdr;          /* pointer to PDMA's registers. */

     u_char           info   : 1 ;          /* informational messages flag. */
     u_char           int_en : 1 ;          /* interrupt enable flag.       */
     u_char           all    : 1 ;          /* run all flag.                */
     u_char           man    : 1 ;          /* manual flag.                 */
  
     int                 pass_num,          /* number of passes             */
			 loop_num,	    /* number of times requested    */
                         test_num,          /* test number (current test).  */
                         intr_lev;          /* interrupt level.             */

     char                *berr_pt;          /* pointer to bus_err string.   */

     struct pd_err_log       elog;          /* PDMA error log.              */
       
     int           (*sys_buserr_handler)(); /* sys bus error handler routine*/
     int           (*pro_buserr_handler)(); /* my bus error handler routine */
 
}; 


/*
 *  error codes .
 */
#define    BUS_ERROR             0x00000001 
#define    DATA_MIS_COMPARE      0x00000002
#define    NO_TIME_OUT_ERR       0x00000004
#define	   NO_RESET_ERR		 0x00000008
#define	   NO_INTRUP_ERR	 0x00000010
#define    BAD_INTRUP_ERR	 0x00000020
#define	   DMA_TIME_OUT_ERR	 0x00000040
#define	   DMA_BYTE_ADDR_ERR	 0x00000080
#define	   DMA_WORD_ADDR_ERR     0x00000100
