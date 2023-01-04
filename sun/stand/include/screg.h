/*	@(#) screg.h 1.1 9/25/86 SMI	*/
/*	from scsi_reg.h 4.3 83/08/24 SMI	*/

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
#define	SC_READ			0x08
#define	SC_WRITE		0x0a
#define	SC_SEEK			0x0b
#define SC_INT_DRIVE		0x0c
#define SC_INTER_DIAG           0xe4
#define SC_RAM_DIAG		0xe0
#define SC_DRIVE_DIAG		0xe3

#define	MORE_STATUS 0x80	/* More status flag */
#define	STATUS_LEN  3		/* Max status len for SCSI */

#define	cdbaddr(cdb, addr) 	(cdb)->high_addr = (addr) >> 16;\
				(cdb)->mid_addr = ((addr) >> 8) & 0xFF;\
				(cdb)->low_addr = (addr) & 0xFF
