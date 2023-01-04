/*	@(#)sc_reg.h 1.1 86/09/25 SMI;*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/* 
 * SCSI Sun host adapter control registers.
 */

struct	scsi_ha_reg {		/* host adapter (I/O space) registers */
	u_char	data;		/* data register */
	u_char	unused;
	u_char	cmd_stat;	/* command/status register */
	u_char	unused2;
	u_short	icr;		/* interface control register */
	u_short	unused3;
	u_long	dma_addr;	/* dma base address */
	u_short	dma_count;	/* dma count register */
        u_short intvec;         /* interrupt vector register.(VME only) */
};

/*
 * bits in the interface control register 
 */
#define	ICR_PARITY_ERROR	0x8000
#define	ICR_BUS_ERROR		0x4000
#define	ICR_ODD_LENGTH		0x2000
#define	ICR_INTERRUPT_REQUEST	0x1000
#define	ICR_REQUEST		0x0800
#define	ICR_MESSAGE		0x0400
#define	ICR_COMMAND_DATA	0x0200	/* command=1, data=0 */
#define	ICR_INPUT_OUTPUT	0x0100	/* input=1, output=0 */
#define	ICR_PARITY		0x0080
#define	ICR_BUSY		0x0040
/* Only the following bits may usefully be set by the CPU */
#define	ICR_SELECT		0x0020
#define	ICR_RESET		0x0010
#define	ICR_PARITY_ENABLE	0x0008
#define	ICR_WORD_MODE		0x0004
#define	ICR_DMA_ENABLE		0x0002
#define	ICR_INTERRUPT_ENABLE	0x0001

/*
 * Compound conditions of icr bits message, command/data and input/output.
 */
#define	ICR_COMMAND	(ICR_COMMAND_DATA)
#define	ICR_STATUS	(ICR_COMMAND_DATA | ICR_INPUT_OUTPUT)
#define	ICR_MESSAGE_IN	(ICR_MESSAGE | ICR_COMMAND_DATA | ICR_INPUT_OUTPUT)
#define	ICR_BITS	(ICR_MESSAGE | ICR_COMMAND_DATA | ICR_INPUT_OUTPUT)

/*
 * Messages that SCSI can send.
 *
 * For now, there is just one.
 */
#define	SC_COMMAND_COMPLETE	0x00

/*
 * Standard SCSI control blocks.
 * These go in or out over the SCSI bus.
 */
struct	scsi_cdb {		/* scsi command description block */
	u_char	cmd;		/* command code */
	u_char	lun	  : 3;	/* logical unit number */
	u_char	high_addr : 5;	/* high part of address */
	u_char	mid_addr;	/* middle part of address */
	u_char	low_addr;	/* low part of address */
	u_char	count;		/* block count */
	u_char	vu_57	  : 1;	/* vendor unique (byte 5 bit 7) */
	u_char	vu_56	  : 1;	/* vendor unique (byte 5 bit 6) */
	u_char		  : 4;	/* reserved */
	u_char	fr	  : 1;	/* flag request (interrupt at completion) */
	u_char	link	  : 1;	/* link (another command follows) */
};

/*
 * defines for SCSI tape cdb.
 */
#define	t_code		high_addr
#define	high_count	mid_addr
#define	mid_count	low_addr
#define low_count	count

struct	scsi_scb {		/* scsi status completion block */
	/* byte 0 */
	u_char	ext_st1	: 1;	/* extended status (next byte valid) */
	u_char	vu_06	: 1;	/* vendor unique */
	u_char	vu_05	: 1;	/* vendor unique */
	u_char	is	: 1;	/* intermediate status sent */
	u_char	busy	: 1;	/* device busy or reserved */
	u_char	cm	: 1;	/* condition met */
	u_char	chk	: 1;	/* check condition: sense data available */
	u_char	vu_00	: 1;	/* vendor unique */
	/* byte 1 */
	u_char	ext_st2	: 1;	/* extended status (next byte valid) */
	u_char	reserved: 6;	/* reserved */
	u_char	ha_er	: 1;	/* host adapter detected error */
	/* byte 2 */
	u_char	byte2;		/* third byte */
};

struct	scsi_sense {		/* scsi sense for error classes 0-6 */
	u_char	adr_val	: 1;	/* sense data is valid */
	u_char	class	: 3;	/* error class (0-6) */
	u_char	code	: 4;	/* error code */
	u_char	high_addr;	/* high byte of block addr */
	u_char	mid_addr;	/* middle byte of block addr */
	u_char	low_addr;	/* low byte of block addr */
	u_char	extra[4];	/* for compatibility with sense7 */
};

struct  scsi_mode_sense {
        u_char  resev0;      /* reserved */
        u_char  resev1;      /* reserved */
        u_char  resev2;      /* reserved */
        u_char  len;            /* length of the extended description list */
        u_char  dens_code;      /* density code */
        u_char  resev3;      /* reserved */  
        u_char  resev4;      /* reserved */  
        u_char  resev5;      /* reserved */  
        u_char  blksz_b0;       /* block size byte #0 */
        u_char  blksz_b1;       /* block size byte #1 */
        u_char  blksz_b2;       /* block size byte #2 */
        u_char  blksz_b3;       /* block size byte #3 */
};

struct	scsi_sense7 {		/* scsi sense for error class 7 */
	/* byte 0 */
	u_char	adr_val	: 1;	/* sense data is valid */
	u_char		: 7;	/* fixed at binary 1110000 */
	u_char	reserved;	/* reserved */
	/* byte 2 */
	u_char	fil_mk	: 1;	/* file mark on device */
	u_char	eom	: 1;	/* end of media */
	u_char		: 2;	/* reserved */
	u_char	key	: 4;	/* sense key */
	u_char	info_1;		/* information byte 1 */
	u_char	info_2;		/* information byte 2 */
	u_char	info_3;		/* information byte 3 */
	u_char	info_4;		/* information byte 4 */
	u_char	add_len;	/* number of additional bytes */
	/* additional bytes follow, if any */
};

/* 
 * Sense 7 key values (some day we may use these).
 */
#define	NO_SENSE	0
#define	RECOVERABLE	1
#define	NOT_READY	2
#define	MEDIA_ERR	3
#define	HARDWARE_ERR	4
#define	ILLEGAL_REQ	5
#define	MEDIA_CHANGE	6
#define	WRITE_PROT	7
#define	DIAGNOSTIC	8
#define	VENDOR		9
#define	POWER_UP_FAIL	10
#define	ABORT		11
#define	EQUAL		12
#define	OVERFLOW	13
/* codes 14 and 15 are reserved */

/*
 * SCSI Operation codes. 
 */
#define	SC_TEST_UNIT_READY	0x00
#define	SC_REZERO_UNIT		0x01
#define	SC_REQUEST_SENSE	0x03
#define	SC_MODE_SENSE   	0x1a
#define	SC_READ			0x08
#define	SC_WRITE		0x0a
#define	SC_SEEK			0x0b

#define	MORE_STATUS 0x80	/* More status flag */
#define	STATUS_LEN  3		/* Max status len for SCSI */

#define	cdbaddr(cdb, addr) 	(cdb)->high_addr = (addr) >> 16;\
				(cdb)->mid_addr = ((addr) >> 8) & 0xFF;\
				(cdb)->low_addr = (addr) & 0xFF


#define     LINEBUFSZ         81 
#define     DELAY(N)          { register int n = N>>1; while (n-- > 0);}
#define     CNTRL_MASK        0x3f
#define     SCSI_HOST_ADDR    0x80
#define     MAX(a,b)          ((a) > (b) ? (a) : (b))



#define VME_SCSI_PHYS_ADRS   ((char *) 0x280000)

/* Define for SCSI buffer for DMA */

#define     DBUF_PA		0x0
#define     DBUF_VA		(u_char *)(MBMEM_BASE+DBUF_PA)



struct scsi_log {
     int       code;      /* general error code, always set on error.    */
     u_short   act_icr;   /* actual condition got back from controller.  */
     u_short   exp_icr;   /* expected condition waiting for.             */
     int       act_val;
     int       exp_val;
};



struct scsi_par {                    
     /* SCSI'S REGISTIERS ADDRESS.   */
     struct scsi_ha_reg         *scsi_addr; /* pointer to scsi's registers. */

     /* GLOBAL FLAGS USED BY TEST.   */
     u_char  			vme    : 1 ;/* indicates multibus or VME.   */
     u_char                     info   : 1 ;/* informational messages flag. */
     u_char                     int_en : 1 ;/* interrupt enable flag.       */
     u_char                     all    : 1 ;/* run all flag.                */
     u_char                     man    : 1 ;/* automatic flag.              */
   
     /* TEST'S PARAMETERS.           */
     int                 	target,     /* target ID.                   */
         			unit,       /* unit ID.                     */
				pass_num,   /* number of passes.            */
                                test_num,   /* test number (current test).  */
                                int_lv,     /* interrupt level.             */
				sblk,       /* starting logical blosk addr. */
				nblk;	    /* number of blocks to transfer.*/

     /* ERROR LOG STRUCTURES.        */
     char                       *berr_tp;   /* pointer to bus_err string.   */
     struct scsi_log            elog;       /* SCSI error log.              */

     /* BUS ERROR HANDLERS ROUTINES. */
     int                        (*sys_buserr_handler)();
     int			(*pro_buserr_handler)();

     /* COMMAND, STANTUS, & SENSE    */
     struct scsi_cdb            cdb;        /* reserve storage for cdb.     */
     struct scsi_scb            scb;        /* reserve storage for scb.     */
     struct scsi_sense          sense;      /* reserve storage for sense.   */
     int                        ext_sense1; /* reserve for ext sense info   */ 
     int                        ext_sense2; /* reserve for ext sense info   */ 
};





/* SCSI error code definition. */

#define     COND_NOT_MET      0x00000001
#define     COND_BUS_ERR      0x00000002

#define     DEV_SEL_ERR       0x00000004
#define     BUSY_BIT_STK      0x00000008

#define     PUT_BYTE_ERR      0x00000010
#define     GET_BYTE_ERR      0x00000020
#define     XFER_LEN_ERR      0x00000040

#define     CHK_ON_STATUS     0x00000100
#define     CHK_RQ_SENSE      0x00000200
#define     DATA_MISS_COMP    0x00000400

#define     NO_INTR_REQ       0x00001000
#define     BAD_MESS_IN       0x00002000
#define     BAD_INTR_VECT     0x00004000

#define     DMA_BUS_ERR       0x00010000
#define     DMA_BAD_LEN       0x00020000
#define     DMA_INTR_ERR      0x00040000
#define     DMA_NO_INTR       0x00080000

#define     SCC_ERROR         0x00100000

#define     GOT_BUS_ERR       0x10000000
