
/*	@(#)diagmenus.h 1.1 86/09/27 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * diagmenus.h: Miscellaneous defines for diagmenus.c.
 */

#define	SCP_ADDR	0xFFFFFF6
#define	TEST_MEM	0x120000	/* SCSI TEST Memory base */
#define	VIDEO_BASE	0x100000	/* M25 Video Memory basr */
#ifdef SIRIUS
#define VIDEOMEM_SIZE   0x40000         /* Sirius Video Memory size */
#else
#define VIDEOMEM_SIZE	0x20000		/* Carrera & M25 Video Memory size */
#endif SIRIUS
#define ESC_ASCII	0x1b
#define EOF	-1
#define MAXTIME_AMD_TX 10000
#define LONGSIZE(x)	(sizeof(x) & ~(sizeof(long) - 1))
#define MAXTIME_AMD 100
#define MAXTIME_AMD_RX 10000
#define M25_CPUDELAY	3
#define SC_UDC_WAIT	1
#define AMD_UDC_PTRMAX 0x7E
#define ERROR_NO_FIFO_EMPTY     1
#define DEBUG FALSE
#define DEBUG_FIFO FALSE
#define DEBUG_STATUS FALSE
#define DEBUG_SCSI_BYTE_COUNTER FALSE
#define ERROR_FIFO_EMPTY        2
#define PASS	FALSE	/* FALSE (NO) error */
#define FAIL	TRUE	/* TRUE error       */
#define ON	TRUE
#define OFF	FALSE

/**************************************************************************
 * Defines for NCR 5380 SCSI chip
 **************************************************************************/

#define NCR_SCSI_ASSERT_RST 	( 1<<7 )
#define NCR_SCSI_AIP		( 1<<6 )	/* arbitration in progress */
#define NCR_SCSI_LA		( 1<<5 )	/* Lost arbitration	   */
#define NCR_SCSI_ASSERT_ACK 	( 1<<4 )
#define NCR_SCSI_ASSERT_BSY 	( 1<<3 )
#define NCR_SCSI_ASSERT_SEL 	( 1<<2 )
#define NCR_SCSI_ASSERT_ATN 	( 1<<1 )
#define NCR_SCSI_ASSERT_DATA_BUS	( 1<<0 )

#define NCR_SCSI_ASSERT_REQ 	( 1<<3 )
#define NCR_SCSI_ASSERT_MSG 	( 1<<2 )
#define NCR_SCSI_ASSERT_CD 	( 1<<1 )
#define NCR_SCSI_ASSERT_IO	( 1<<0 )

#define NCR_SCSI_TEST_MODE	( 1<<6 )
#define NCR_SCSI_DIFF_ENB	( 1<<5 )

#define NCR_SCSI_DMA_MODE 	( 1<<1 )
#define NCR_SCSI_TARGET_MODE	( 1<<6 )
 
#define NCR_DB7			( 1<<7 )
#define NCR_DB6			( 1<<6 )
#define NCR_DB5			( 1<<5 )
#define NCR_DB4			( 1<<4 )
#define NCR_DB3			( 1<<3 )
#define NCR_DB2			( 1<<2 )
#define NCR_DB1			( 1<<1 )
#define	NCR_DB0			( 1<<0 )
/**************************************************************************
 * END of Defines for NCR 5380 SCSI chip
 **************************************************************************/

/**************************************************************************
 * Defines for AMD 9516 Universal Data Controller chip
 **************************************************************************/
#define AMD_UDC_OFFSET	0x8		/* AMD is 8 words above */ 
					/* base of SCSI         */
					/* Base is provided by call to      */
					/* where_is_scsi.  These    */
					/* provide the offset information   */
#define	AMD_UDC_DATA	0		/* AMD Register DATA port           */
#define	AMD_UDC_PTR	AMD_UDC_DATA+1	/* AMD Register address select port */
					/* The +1 is +1 short int (16 bits) */
					/* size controlled by how the       */
					/* address port is referenced from  */
					/* the data port		    */
					/* Example			    */

#define	AMD_UDC_CAR_LAF_1	2	/* Current Address Register * 
				        /* Lower Address Field channel-1   */
#define	AMD_UDC_CAR_LAF_2	0	/* Current Address Register */
				        /* Lower Address Field channel-2   */

/* addresses of the udc registers accessed directly by driver */

#define AMD_UDC_ADR_MODE	0x38	/* master mode register */
#define AMD_UDC_ADR_COMMAND	0x2e	/* command register (write only) */
#define AMD_UDC_ADR_STATUS	0x2e	/* status register (read only) */
#define AMD_UDC_ADR_CAR_HIGH	0x26	/* chain addr reg, high word */
#define AMD_UDC_ADR_CAR_LOW	0x22	/* chain addr reg, low word */
#define AMD_UDC_ADR_CARA_HIGH	0x1a	/* cur addr reg A, high word */
#define AMD_UDC_ADR_CARA_LOW	0x0a	/* cur addr reg A, low word */
#define AMD_UDC_ADR_CARB_HIGH	0x12	/* cur addr reg B, high word */
#define AMD_UDC_ADR_CARB_LOW	0x02	/* cur addr reg B, low word */
#define AMD_UDC_ADR_CMR_HIGH	0x56	/* channel mode reg, high word */
#define AMD_UDC_ADR_CMR_LOW	0x52	/* channel mode reg, low word */
#define AMD_UDC_ADR_COUNT	0x32	/* number of words to transfer */

/* 
 * For a dma transfer, the appropriate udc registers are loaded from a 
 * table in memory pointed to by the chain address register.
 */
struct amd_udc_table {
	u_short			rsel;	/* tells udc which regs to load */
	u_short			haddr;	/* high word of main mem dma address */
	u_short			laddr;	/* low word of main mem dma address */
	u_short			count;	/* num words to transfer */
	u_short			hcmr;	/* high word of channel mode reg */
	u_short			lcmr;	/* low word of channel mode reg */
};

/* indicates which udc registers are to be set based on info in above table */
#define AMD_UDC_RSEL_RECV		0x0182
#define AMD_UDC_RSEL_SEND		0x0282

/* setting of chain mode reg: selects how the dma op is to be executed */
#define AMD_UDC_CMR_HIGH	0x0040	/* high word of channel mode reg */
#define AMD_UDC_CMR_LSEND	0x00c2	/* low word of cmr when send */
#define AMD_UDC_CMR_LRECV	0x00d2	/* low word of cmr when receiving */

/* setting for the master mode register */
#define AMD_UDC_MODE		0xd	/* enables udc chip */

/* setting for the low byte in the high word of an address */
#define AMD_UDC_ADDR_INFO	0x40	/* inc addr after each word is dma'd */

/* udc commands */
#define AMD_UDC_CMD_STRT_CHN	0xa0	/* start chaining */
#define AMD_UDC_CMD_SET_CIE	0x32	/* channel 1 interrupt enable */
#define AMD_UDC_CMD_CLR_CIE	0x30	/* channel 1 interrupt disable */

/* bits in the udc status register */
#define AMD_UDC_SR_CIE		0x8000	/* channel interrupt enable */
#define AMD_UDC_SR_IP		0x2000	/* interrupt pending */
#define AMD_UDC_SR_CA		0x1000	/* channel abort */
#define AMD_UDC_SR_NAC		0x0800	/* no auto reload or chaining*/
#define AMD_UDC_SR_WFB		0x0400	/* waiting for bus */
#define AMD_UDC_SR_SIP		0x0200	/* second interrupt pending */
#define AMD_UDC_SR_HM		0x0040	/* hardware mask */
#define AMD_UDC_SR_HRQ		0x0020	/* hardware request */
#define AMD_UDC_SR_MCH		0x0010	/* match on upper comparator byte */
#define AMD_UDC_SR_MCL		0x0008	/* match on lower comparator byte */
#define AMD_UDC_SR_MC		0x0004	/* match condition ended dma */
#define AMD_UDC_SR_EOP		0x0002	/* eop condition ended dma */
#define AMD_UDC_SR_TC		0x0001	/* termination of count ended dma */

 



/**************************************************************************
 * END of Defines for AMD Universal Data Controller chip
 **************************************************************************/

/**************************************************************************
 * Defines for M25 specific SCSI control registers
 **************************************************************************/
#define M25_SCSI_CONTROL_OFFSET	0x14	 /* stat reg is 18 above */ 
					 /* base of SCSI         */


#define M25_SCSI_SR_XFR_ACTIVE 	( 1<<15 )  /*(r) NCR Transfer active     */
#define M25_SCSI_SR_B_CONFLICT 	( 1<<14 )  /*(r) SCSI bus conflict       */
#define M25_SCSI_SR_B_ERR 	( 1<<13 )  /*(r) scsi Bus error		 */
#define M25_SCSI_SR_2ND_BYTE 	( 1<<12 )  /*(r) used by fifo control logic */
#define M25_SCSI_SR_FIFO_FULL 	( 1<<11 )  /*(r) fifo full (~4k)         */
#define M25_SCSI_SR_FIFO_EMPTY 	( 1<<10 )  /*(r) fifo empty              */
#define M25_SCSI_SR_NCR_IP 	( 1<<9 )   /*(r) NCR scsi interrupt pending */
#define M25_SCSI_SR_AMD_IP 	( 1<<8 )   /*(r) AMD udc  interrupt pending */

#define M25_SCSI_SEND	 	( 1<<3 ) /*(r/w) dma direction 1=to fifo */
#define M25_SCSI_INT_EN 	( 1<<2 ) /*(r/w) NCR Interrrupt enable   */
#define M25_FIFO_RUN	 	( 1<<1 ) /*(r/w) opposite of reset       */
#define M25_UDC_RUN		( 1<<0 ) /*(r/w) SCSI and UDC on same line  */
#define M25_SCSI_RUN		( 1<<0 ) /*(r/w) opposite of reset       */


/**************************************************************************
 * END of Defines for M25 specific SCSI control registers
 **************************************************************************/
/**************************************************************************
 * Assorted definitions
 **************************************************************************/

#define INT_ON	ON
#define INT_OFF OFF

/**************************************************************************
 * Definitions of structures used by these tests
 **************************************************************************/

struct ncr_scsi_rd		/* structure to NCR SCSI chip (5380)	*/
				/* in read mode				*/
	{
	u_char  current_data;		/* NCR 5380 read data */
				        /* on SCSI bus        */
	u_char  initiator_cmd;		/* read/write */
	u_char  mode;			/* read/write */
	u_char  target_command;		/* read/write */
	u_char  current_bus_status;	/* read */
	u_char  bus_and_status;		/* read */
	u_char  input_data;		/* read */
	u_char  reset_parity_ints;	/* read (causes reset of parity and */
					/*       interrupt                  */


	};
struct ncr_scsi_wr		/* structure to NCR SCSI chip (5380)	*/
				/* in write mode			*/
	{
	u_char  output_data;		/* write */
	u_char  initiator_cmd;		/* read/write */
	u_char  mode;			/* read/write */
	u_char  target_command;		/* read/write */
	u_char  select_enable;		/* write */
	u_char  start_dma_send;		/* write */
	u_char  start_dma_target_rcv;	/* write */
	u_char  start_dma_initiator_rcv;	/* write */

	};


struct amd_udc			/* AMD Universal Dma Controller */
	{
	u_short	data;		/* data input/output register     */
	u_short	address;	/* address pointer for internal register */
	};



struct m25_scsi_control
	{
	u_short	firstbyte;	/* register containing the odd byte */
				/* after a dma xfer of odd length   */
	u_short	byte_cntr;	/* counter of bytes transferred		*/
	u_short	status;		/* status/control byte for m25 specials */
	};



