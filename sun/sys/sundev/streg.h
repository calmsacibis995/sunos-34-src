/* @(#)streg.h	1.4	12/23/86 Copyright 1986 Sun Microsystems, Inc. */
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Defines for SCSI tape.
 */
#define	DEV_BSIZE	512
#define	SENSE_LENGTH	16

/*
 * Open flag codes
 */
#define	CLOSED		0
#define	OPENING		1
#define	OPEN_FAILED	2
#define	OPEN		3
#define	CLOSING		4

/*
 * Operation codes.
 */
#define	SC_REWIND		SC_REZERO_UNIT
#define SC_QIC02		0x0d
#define	SC_WRITE_FILE_MARK	0x10
#define SC_SPACE		0x11
#define SC_MODE_SELECT		0x15
#define SC_ERASE_CARTRIDGE	0x19
#define SC_LOAD			0x1b
#define SC_SPACE_FILE		0x81	/* phony - for internal use only */
#define SC_SPACE_REC		0x82	/* phony - for internal use only */
#define SC_RETENSION		0x100+SC_REWIND	/* phony - for int use only */
#define SC_QIC11		0x83	/* phony - for int use only */
#define SC_QIC24		0x84	/* phony - for int use only */
#define SC_READ_XSTATUS_CIPHER	0xe0	/* read extended status, Cipher only */

#define SC_IS_WRITE_COMMAND(cmd)	((cmd) == SC_WRITE_FILE_MARK || \
					 (cmd) == SC_ERASE_CARTRIDGE || \
					 (cmd) == SC_WRITE)
#define ST_TYPE_INVALID		0x00

/* 
 * Misc defines specific to sysgen controllers
 */
#define ST_SYSGEN_QIC11		0x26
#define ST_SYSGEN_QIC24		0x27
#define ST_SYSGEN_SENSE_LEN	16
#define ST_TYPE_SYSGEN		1
#define IS_SYSGEN(dsi)	(dsi->un_ctype == ST_TYPE_SYSGEN ? 1 : 0)

/* 
 * Misc defines specific to emulex controllers 
 */
#define ST_EMULEX_QIC11		0x84
#define ST_EMULEX_QIC24		0x05
#define ST_EMULEX_SENSE_LEN	11
#define ST_TYPE_EMULEX		2
#define IS_EMULEX(dsi)	(dsi->un_ctype == ST_TYPE_EMULEX ? 1 : 0)

/*
 * Parameter list for the MODE_SELECT command.
 * The parameter list contains a header, followed by zero or more
 * block descriptors, followed by vendor unique parameters, if any.
 */
struct st_ms_hdr {
	u_char	reserved1;	/* reserved */
	u_char	reserved2;	/* reserved */
	u_char		  :1;	/* reserved */
	u_char	bufm	  :3;	/* buffered mode */
	u_char	speed	  :4;	/* speed */
	u_char	bd_len;		/* length in bytes of all block descs */
};

struct st_ms_bd {
	u_char	density;	/* density code */
	u_char	high_nb;	/* num of logical blocks on the medium that */
	u_char	mid_nb;		/* are to be formatted with the density code */
	u_char	low_nb;		/* and block length in block desc */
	u_char	reserved;	/* reserved */
	u_char	high_bl;	/* block length */
	u_char	mid_bl;		/* block length */
	u_char	low_bl;		/* block length */
};

/*
 * Mode Select Parameter List expected by emulex controllers.
 */
struct st_emulex_mspl {
	struct st_ms_hdr hdr;	/* mode select header */
	struct st_ms_bd  bd;	/* block descriptor */
	u_char		  :5;	/* unused */
	u_char	dea	  :1;	/* disable erase ahead */
	u_char	aui	  :1;	/* auto-load inhibit */
	u_char	sec	  :1;	/* soft error count */
};
#define EM_MS_PL_LEN	13	/* length of mode select param list */
#define EM_MS_BD_LEN	8	/* length of block descriptors */

/*
 * Sense info returned by sysgen controllers.
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
	u_char	bot	  :1;	/* beginning of tape */
	u_char		  :1;	/* reserved */
	u_char		  :1;	/* reserved */
	u_char	pwr_on    :1;	/* power on or reset since last op */
	short	retry_ct;	/* retry count */
	short	underruns;	/* number of underruns */
};

struct st_sysgen_sense {
	char	disk_sense[4];		/* sense data from disk */
	struct qic_sense qic_sense;	/* sense data from QIC II */
	char	disk_xfer[3];		/* no. blks in last disk oper */
	char	tape_xfer[3];		/* no. blks in last tape oper */
};

/*
 * First part of MT02 sense.
 */
struct st_emulex_sense1 {
	/* byte 0 */
	u_char	adr_val	: 1;	/* sense data is valid */
	u_char		: 7;	/* fixed at binary 1110000 */
	/* byte 1 */
	u_char	seg_num;	/* segment number, applies to copy cmd only */
	/* byte 2 */
	u_char	fil_mk	: 1;	/* file mark on device */
	u_char	eom	: 1;	/* end of media */
	u_char	ili	: 1;	/* incorrect length indicator */
	u_char		: 1;	/* reserved */
	u_char	key	: 4;	/* sense key, see below */
	/* bytes 3 through 7 */
	u_char	info_1;		/* information byte 1 */
	u_char	info_2;		/* information byte 2 */
	u_char	info_3;		/* information byte 3 */
	u_char	info_4;		/* information byte 4 */
	u_char	add_len;	/* number of additional bytes */
};
/*
 * Sense info returned by emulex controllers.
 */
struct st_emulex_sense {
	struct st_emulex_sense1 ext_sense;
	u_char	error;			/* error class and code, see below */
	u_char	retries_msb;		/* retry count, most signif byte */
	u_char	retries_lsb;		/* retry count, least signif byte */
};

/* number of emulex sense bytes in addition to generic extended sense */
#define EM_ES_ADD_LEN			3

/* error fields values that we currently use */
#define EM_NO_SENSE			0x00
#define EM_DRIVE_NOT_READY		0x04
#define EM_MEDIA_NOT_LOADED		0x09
#define EM_INSUFFICIENT_CAPACITY	0x0a
#define EM_DRIVE_TIMEOUT		0x0b
#define EM_UNCORRECTABLE_DATA_ERR	0x11
#define EM_BLOCK_NOT_FOUND		0x14
#define EM_DMA_TIMEOUT			0x16
#define EM_WRITE_PROTECTED		0x17
#define EM_CORRECTABLE_DATA_CHECK	0x18
#define EM_BAD_BLOCK			0x19
#define EM_FILE_MARK_DETECTED		0x1c
#define EM_COMPARE_ERR			0x1d
#define EM_INVALID_COMMAND		0x20
#define EM_UNIT_ATTENTION		0x30
#define EM_COMMAND_TIMEOUT		0x31
#define EM_APPEND_ERR			0x33
#define EM_READ_END_OF_MEDIA		0x34

/*
 * Macros for getting information from the sense data returned
 * by the tape controller.
 */
#define ST_FILE_MARK(dsi, sense) \
	( ((IS_EMULEX(dsi) && \
	(((struct st_emulex_sense *)sense)->error == EM_FILE_MARK_DETECTED)) \
	|| \
	(IS_SYSGEN(dsi) && \
	((struct st_sysgen_sense *)sense)->qic_sense.file_mark)) ? 1 : 0)

#define ST_WRITE_PROT(dsi, sense) \
	( ((IS_EMULEX(dsi) && \
	(((struct st_emulex_sense *)sense)->error == EM_WRITE_PROTECTED)) \
	|| \
	(IS_SYSGEN(dsi) && \
	((struct st_sysgen_sense *)sense)->qic_sense.write_prot)) ? 1 : 0)

#define ST_EOT(dsi, sense) \
	( ((IS_EMULEX(dsi) && \
	(((struct st_emulex_sense *)sense)->error == \
					EM_INSUFFICIENT_CAPACITY) || \
	(((struct st_emulex_sense *)sense)->error == \
					EM_READ_END_OF_MEDIA)) || \
	(IS_SYSGEN(dsi) && \
	((struct st_sysgen_sense *)sense)->qic_sense.eot)) ? 1 : 0)

#define ST_ILLEGAL(dsi, sense) \
	( ((IS_EMULEX(dsi) && \
	(((struct st_emulex_sense *)sense)->error == EM_INVALID_COMMAND)) \
	|| \
	(IS_SYSGEN(dsi) && \
	((struct st_sysgen_sense *)sense)->qic_sense.illegal)) ? 1 : 0)

#define ST_NO_CART(dsi, sense) \
	( ((IS_EMULEX(dsi) && \
	(((struct st_emulex_sense *)sense)->error == EM_MEDIA_NOT_LOADED)) \
	|| \
	(IS_SYSGEN(dsi) && \
	((struct st_sysgen_sense *)sense)->qic_sense.no_cart)) ? 1 : 0)

#define ST_RESET(dsi, sense) \
	( ((IS_EMULEX(dsi) && \
	(((struct st_emulex_sense *)sense)->error == EM_UNIT_ATTENTION)) \
	|| \
	(IS_SYSGEN(dsi) && \
	((struct st_sysgen_sense *)sense)->qic_sense.pwr_on)) ? 1 : 0)

#define ST_CORRECTABLE(dsi, sense) \
	(IS_EMULEX(dsi) && \
	(((struct st_emulex_sense *)sense)->error == EM_CORRECTABLE_DATA_CHECK))
