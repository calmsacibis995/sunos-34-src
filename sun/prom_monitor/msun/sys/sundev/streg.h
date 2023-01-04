/*	@(#)streg.h 1.5 85/02/19 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Defines for SCSI tape.
 */
#define	DEV_BSIZE	512
#define	SENSE_LENGTH	16

/*
 * Operation codes.
 */
#define	SC_REWIND		SC_REZERO_UNIT
#define	SC_WRITE_FILE_MARK	0x10
#define SC_SPACE		0x11
#define SC_ERASE_CARTRIDGE	0x19
#define SC_SPACE_FILE		0x81	/* phony - for internal use only */
#define SC_SPACE_REC		0x82	/* phony - for internal use only */
#define SC_RETENSION		0x100+SC_REWIND	/* phony - for int use only */

/*
 * Sense returned by sysgen.
 */
struct	qic_sense {
	/* first byte: */
	u_char	other_bit :1;	/* some other bit set in this byte */
	u_char	no_cart   :1;	/* no cartrige, or removed */
	u_char	not_there :1;	/* drive not present */
	u_char	write_prot:1;	/* write protected */
	u_char	eot       :1;	/* end of last track */
	u_char	data_err  :1;	/* unrecoverable data error */
	u_char	no_err    :1;	/* data transmitted not in error */
	u_char	file_mark :1;	/* file mark detected */
	/* second byte: */
	u_char	other_bit2:1;	/* some other bit set in this byte */
	u_char	illegal   :1;	/* illegal command */
	u_char	no_data   :1;	/* unable to find data */
	u_char	retries   :1;	/* 8 or more retries needed */
	u_char	bot       :1;	/* beginning of tape */
	u_char		  :1;	/* reserved */
	u_char		  :1;	/* reserved */
	u_char	pwr_on    :1;	/* power on or reset since last op */
	short	retry_ct;	/* retry count */
	short	underruns;	/* number of underruns */
};

struct sc_sense {
	char	disk_sense[4];		/* sense data from disk */
	struct qic_sense qic_sense;	/* sense data from QIC II */
	char	disk_xfer[3];		/* no. blks in last disk oper */
	char	tape_xfer[3];		/* no. blks in last tape oper */
};
/*
 * Open flag codes
 */
#define	CLOSED		0
#define	OPENING		1
#define	OPEN_FAILED	2
#define	OPEN		3
#define	CLOSING		4
