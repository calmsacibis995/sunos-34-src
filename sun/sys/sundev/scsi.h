/* @(#)scsi.h	1.9 87/04/14	Copyr 1986 Sun Micro */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

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

		/*
		 * Standard (Non Extended) SCSI Sense. Used mainly by the
  		 * Adaptec ACB 4000 which is the only controller that
		 * does not support the Extended sense format.
	         */
struct	scsi_sense {		/* scsi sense for error classes 0-6 */
	u_char	adr_val	: 1;	/* sense data is valid */
	u_char	class	: 3;	/* error class (0-6) */
	u_char	code	: 4;	/* error code */
	u_char	high_addr;	/* high byte of block addr */
	u_char	mid_addr;	/* middle byte of block addr */
	u_char	low_addr;	/* low byte of block addr */
	u_char	extra[12];	/* pad this struct so it can hold max num */
				/* of sense bytes we may receive */
};

/*
 * Value(s) of scsi_sense class field.
 */
#define SC_CLASS_EXTENDED_SENSE	0x7	/* indicates extended sense */

struct	scsi_ext_sense {	/* scsi extended sense for error class 7 */
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
	/* SCSI Commom Command Set Additional Implementation as Follows */
	u_char	optional_8;	/* CCS search and copy only */
	u_char	optional_9;	/* CCS search and copy only */
	u_char	optional_10;	/* CCS search and copy only */
	u_char	optional_11;	/* CCS search and copy only */
	u_char 		: 1;	/* reserved */
	u_char 	error_code : 7;	/* error class and code combined */
};

/*
 * Returned status particular to Adaptec 4520 only
 */
#define dev_stat_1	 optional_10;	/* device status */
#define	dev_stat_2	 optional_11;
#define	dev_uniq_1	 addit_sense;	/* device vendor unique status */
#define	dev_uniq_2	 reserved;

/*
 * Defines for Emulex MD21 SCSI/ESDI Controller. Extended sense for Format
 * command only.
 */
#define cyl_msb		info_1
#define cyl_lsb		info_2
#define head_num	info_3
#define sect_num	info_4

/* 
 * Sense key values for extended sense.
 */
#define	SC_NO_SENSE		0x0
#define	SC_RECOVERABLE_ERROR	0x1
#define	SC_NOT_READY		0x2
#define	SC_MEDIUM_ERROR		0x3
#define	SC_HARDWARE_ERROR	0x4
#define	SC_ILLEGAL_REQUEST	0x5
#define	SC_UNIT_ATTENTION	0x6
#define	SC_DATA_PROTECT		0x7
#define	SC_BLANK_CHECK		0x8 /* reserved on MD21 */
#define	SC_VENDOR_UNIQUE	0x9
#define	SC_COPY_ABORTED		0xa
#define	SC_ABORT_COMMAND	0xb
#define	SC_EQUAL		0xc
#define	SC_VOLUME_OVERFLOW	0xd /* reserved on MD21 */
#define SC_MISCOMPARE		0xe
#define SC_RESERVED		0xf /* reserved on MD21 */

struct	scsi_inquiry_data {	/* data returned as a result of CCS inquiry */
	u_char	dtype;		/* device type */
	/* byte 1 */
	u_char	rmb 	   : 1;	/* removable media */
	u_char  dtype_qual : 7;	/* device type qualifier */
	/* byte 2 */
	u_char iso 	   : 2;	/* ISO version */
	u_char ecma 	   : 3;	/* ECMA version */
	u_char ansi 	   : 3;	/* ANSI version */
	/* byte 3 */
	u_char reserv1 	   : 4;	/* reserved */
	u_char rdf 	   : 4;	/* response data format */
	u_char add_len;		/* additional length */
	u_char reserv2;		/* reserved */
	u_char reserv3;		/* reserved */
	char vid[8];		/* vendor ID */
	char pid[16];		/* product ID */
	char revision[4];	/* revision level */
};

/*
 * SCSI Operation codes. 
 */

	/*
	 * These two commands use identical command blocks for all
	 * controllers.
         */
#define	SC_TEST_UNIT_READY	0x00
#define	SC_REZERO_UNIT		0x01
#define SC_SEEK			0x0b

	/*
	 * This command uses identical command blocks for all controllers,
	 * except for the length of requested sense bytes (byte 4).
	 * Emulex MT02 sends 11 bytes.
	 * Emulex MD21 sends 9 bytes.
	 * Adaptec 4520 sends 14 bytes.
	 */
#define	SC_REQUEST_SENSE	0x03

	/* These two commands use identical command blocks for all
 	 * controllers except the Adaptec ACB 4000 which sets bit 1 of byte 1.
	 */
#define	SC_READ			0x08
#define	SC_WRITE		0x0a

#define	SC_SEEK			0x0b
#define SC_INQUIRY		0x12		/* CCS only */

/*
 * Messages that SCSI can send.
 */
#define	SC_COMMAND_COMPLETE	0x00
#define SC_SAVE_DATA_PTR	0x02
#define SC_RESTORE_PTRS		0x03
#define SC_DISCONNECT		0x04
#define SC_IDENTIFY		0x80
#define SC_DR_IDENTIFY		0xc0

#define	MORE_STATUS 0x80	/* More status flag */
#define	STATUS_LEN  3		/* Max status len for SCSI */

#define	cdbaddr(cdb, addr) 	(cdb)->high_addr = (addr) >> 16;\
				(cdb)->mid_addr = ((addr) >> 8) & 0xFF;\
				(cdb)->low_addr = (addr) & 0xFF


#define NLPART	NDKMAP		/* number of logical partitions (8) */

/*
 * SCSI unit block.
 */
struct scsi_unit {
	char	un_target;		/* scsi bus address of controller */
	char	un_lun;			/* logical unit number of device */
	char	un_present;		/* unit is present */
	u_char	un_scmd;		/* special command */
	u_short un_unit;		/* real unit number of device */
	struct	scsi_unit_subr *un_ss;	/* scsi device subroutines */
	struct	scsi_ctlr *un_c;	/* scsi ctlr */
	struct	mb_device *un_md;	/* mb device */
	struct	mb_ctlr *un_mc;		/* mb controller */
	struct	buf un_utab;		/* queue of requests */
	struct	buf un_sbuf;		/* fake buffer for special commands */
	struct	buf un_rbuf;		/* buffer for raw i/o */
	/* current transfer: */
	u_short	un_flags;		/* misc flags relating to cur xfer */
	int	un_baddr;		/* virtual buffer address */
	daddr_t	un_blkno;		/* current block */
	short	un_sec_left;		/* sector count for single sectors */
	short	un_cmd;			/* command (for cdb) */
	short	un_count;		/* num sectors to xfer (for cdb) */
	int	un_dma_addr;		/* dma address */
	u_short	un_dma_count;		/* byte count expected */
	short	un_retries;		/* retry count */
	short	un_restores;		/* restore count */
	char	un_wantint;		/* expecting interrupt */
	/* the following save current dma information in case of disconnect */
	int	un_dma_curaddr;		/* current addr to start dma to/from */
	u_short	un_dma_curcnt;		/* current dma count */
	u_short	un_dma_curdir;		/* direction of dma transfer */
	u_short un_dma_curbcr;		/* bcr, saved at head of intr routine */
	/* the following is a copy of current unit command info */
	struct un_saved_cmd_info {
		struct scsi_cdb saved_cdb; 	/* saved cdb */
		struct scsi_scb saved_scb; 	/* saved scb */
		int saved_resid;		/* saved amt untransferred */
		int saved_dma_addr;		
		u_short saved_dma_count;
	} un_saved_cmd;
	caddr_t	un_scratch_addr;	/* address of scratch buffer for MT02 */
};

/* 
 * bits in the scsi unit flags field
 */
#define SC_UNF_DVMA		0x0001	/* set if cur xfer requires dvma */
#define SC_UNF_PREEMPT		0x0002	/* cur xfer was preempted by recon */
#define SC_UNF_DMA_ACTIVE	0x0004	/* DMA active on this unit */
#define SC_UNF_RECV_DATA	0x0008	/* direction of data xfer is recieve */
#define SC_UNF_GET_SENSE	0x0010	/* run get sense cmd for failed cmd */
#define SC_UNF_DMA_INITIALIZED	0x0020	/* initialized DMA hardware */
#define SC_UNF_TAPE_APPEND_TEST	0x0040	/* run special cmd for tape append */
/* ++mjacob 4/14/87 per bug #1004516 {*/
#define	SC_UNF_NOMSGOUT         0x0080  /* don't attempt msg out (sysgen) */
/* } --mjacob                         */

struct scsi_ctlr {
	struct	scsi_ha_reg *c_har;	/* sc bus registers in i/o space */
	struct 	scsi_si_reg *c_sir;	/* si scsi ctlr logic regs */
	struct	scsi_cdb c_cdb;		/* command description block */
	struct	scsi_scb c_scb;		/* status completion block */
	struct	scsi_sense *c_sense;	/* sense info on errors */
	int	c_flags;		/* misc state flags */
	struct 	scsi_unit *c_un;	/* scsi unit using the bus */
	struct	scsi_ctlr_subr *c_ss;	/* scsi device subroutines */
	struct	udc_table c_udct;	/* scsi dma info for Sun3/50 */
	struct	buf *c_disqh;		/* ptr to head of disconnect q */
	struct	buf *c_disqt;		/* ptr to tail of disconnect q */
	struct	buf *c_flush;		/* ptr to last element to flush */
	u_int	c_recon_target;		/* target doing a reconnect */
	/* 
	 * Following is used when running a request sense command 
	 * when a command fails.
	 */
	u_int	c_num_sense_pending;	/* number of get sense cmds pending */
	struct	scsi_cdb c_save_cdb;	/* temp storage for cdb */
	struct	scsi_scb c_save_scb;	/* temp storage for scb */
	int	c_save_bcr;		/* temp storage for bcr */
	struct scsi_unit c_save_unit;	/* temp storage for scsi_unit info */
};

/* misc controller flags */
#define SCSI_PRESENT	0x0001		/* scsi bus is alive */
#define SCSI_ONBOARD	0x0002		/* scsi logic is onboard SCSI-3 */
#define SCSI_EN_RECON	0x0004		/* reconnect attempts are enabled */
#define SCSI_EN_DISCON	0x0008		/* disconnect attempts are enabled */
#define SCSI_FLUSH_DISQ	0x0010		/* flush disconnected tasks */
#define SCSI_FLUSHING	0x0020		/* flushing in progress */
#define SCSI_VME	0x0040		/* is a VME SCSI-3 */

#define IS_ONBOARD(c)	(c->c_flags & SCSI_ONBOARD)
#define IS_VME(c)	(c->c_flags & SCSI_VME)
#define IS_SCSI3(c)	(IS_ONBOARD(c) || IS_VME(c))

/* 
 * Unit specific subroutines called from controllers.
 */
struct	scsi_unit_subr {
	int	(*ss_attach)();
	int	(*ss_start)();
	int	(*ss_mkcdb)();
	int	(*ss_intr)();
	int	(*ss_unit_ptr)();
	char	*ss_devname;
};

/* 
 * Controller specific subroutines called from units.
 */
struct	scsi_ctlr_subr {
	int	(*scs_ustart)();
	int	(*scs_start)();
	int	(*scs_done)();
	int	(*scs_cmd)();
	int	(*scs_getstat)();
	int	(*scs_cmd_wait)();
	int	(*scs_off)();
	int	(*scs_reset)();
	int	(*scs_dmacount)();
	int	(*scs_go)();
};

/*
 * Defines for getting configuration parameters out of mb_device.
 */
#define	TYPE(flags)	(flags)
#define TARGET(slave)	((slave >> 3) & 07)
#define LUN(slave)	(slave & 07)

#define SCSI_DISK	0
#define SCSI_TAPE	1
#define SCSI_FLOPPY	2

#define NUNIT		8	/* max nubmer of units per controller */

/*
 * SCSI Error codes passed to device routines.
 * The codes refer to SCSI general errors, not to device
 * specific errors.  Device specific errors are discovered
 * by checking the sense data.
 * The distinction between retryable and fatal is somewhat ad hoc.
 */
#define	SE_NO_ERROR	0
#define	SE_RETRYABLE	1
#define	SE_FATAL	2

struct scsi_disk {
	struct	dk_map un_map[NLPART];	/* logical partitions */
	struct	dk_geom un_g;		/* disk geometry */
	u_char	dk_ctype;		/* controller type */
};

/* different controller types */
#define CTYPE_UNKNOWN	0
#define CTYPE_MD21	1
#define CTYPE_ACB4000	2
#define CTYPE_CCS	3

struct scsi_tape {
	int	un_mspl;		/* ptr to mode select param list */
	int	un_bufcnt;		/* number of buffers in use */
	int	un_next_block;		/* next record on tape */
	int	un_last_block;		/* last record of file, if known */
	short	un_status;		/* status from last sense */
	short	un_retry_ct;		/* retry count */
	short	un_underruns;		/* number of underruns */
	u_char	un_openf;		/* tape is open */
	u_char	un_eof;			/* eof seen but not sent to user */
	u_char	un_read_only;		/* tape is read only */
	u_char	un_lastiow;		/* last i/o was write */
	u_char	un_lastior;		/* last i/o was read */
	u_char	un_reten_rewind;	/* retension on next rewind */
	u_char	un_qic;			/* qic format */
	u_char	un_ctype;		/* controller type */
	u_char	un_reset_occurred;	/* reset occured since last command */
	u_char	un_readmode;		/* sysgen left in read mode */
	u_char	un_err_pending;		/* illegal cmd error pending */
	u_char	un_last_cmd;		/* last scsi command issued */
#ifdef sun2
	u_char	un_cipher_test;		/* test for cipher drive in progres */
#endif
};

struct scsi_floppy {
	u_char	sf_flags;
	u_char	sf_mdb;			/* media descriptor byte */
	u_char	sf_spt;			/* sectors per track */
	u_char	sf_nhead;		/* number of heads */
	int	sf_nblk;		/* number of blocks on the disk */
};
