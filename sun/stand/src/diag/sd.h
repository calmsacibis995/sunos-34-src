/*	@(#)sd.h 1.1 86/09/25 SMI */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#define ST506_NDEFECT	127     /* must fit in 1K controller buffer... */
#define ESDI_NDEFECT	NDEFCT	/* so it fits in the new defect list */

#define RDEF_ALL	0	/* read all defects */
#define RDEF_MANUF	1	/* read manufacturer's defects */
#define RDEF_CKLEN	2	/* check length of manufacturer's list */

struct format_parms {		/* physical BFI format */
	u_short	reserved;
	u_short	list_len;
	struct defect {
		unsigned cyl  : 24;
		unsigned head : 8;
		long	bytes_from_index;
	} defect[ESDI_NDEFECT];
} format_parms;

struct format_parms grown_defs;	/* logical block format */
#define grown_sect	bytes_from_index
	
struct reassign_parms {
	u_short	reserved;
	u_short	list_len;	/* length in bytes of defects only */
	struct new_defect {
		unsigned lba;	/* logical block address */
	} new_defect[2];
} reassign_parms;

struct code1_cdb {
	u_char	cmd;		/* command code */
	u_char	lun	: 3;	/* logical unit number */
	u_char	reserved: 3;	
	u_char	bytc	: 1;
	u_char	rela	: 1;	
	u_int	lba;		/* logical block address */
	u_char	reserved1;	
	u_char	reserved2;	
	u_char	num_blks;
	u_char	ecc	  : 1;
	u_char	erty	  : 1;
	u_char	av	  : 1;
	u_char	res	  : 3;	/* reserved */
	u_char	fr	  : 1;	/* flag request */
	u_char	link	  : 1;	/* link (another command follows) */
} code1_cdb;

struct ccs_modesel_head {
	u_char	reserve1;
	u_char	medium;			/* medium type */
	u_char 	reserve2;
	u_char	block_desc_length;	/* length of block descriptor */
	u_int	density		: 8;	/* density code */
	u_int	number_blocks	:24;
	u_int	reserve3	: 8;
	u_int	block_length	:24;
}; 

/*
 * Page Header
 */
struct page_header {
	u_char	reserved_1	: 1;
	u_char	reserved_2	: 1;
	u_char	page_code	: 6;	/* define page function */
	u_char	page_length;		/* length of current page */
};

/*
 * Page One - Error Recovery Parameters 
 */
struct ccs_err_recovery {
	struct	page_header page_header;
	u_char	awre		: 1;	/* auto write realloc enabled */
	u_char	arre		: 1;	/* auto read realloc enabled */
	u_char	tb		: 1;	/* transfer block */
	u_char 	rc		: 1;	/* read continuous */
	u_char	eec		: 1;	/* enable early correction */
	u_char	per		: 1;	/* post error */
	u_char	dte		: 1;	/* disable transfer on error */
	u_char	dcr		: 1;	/* disable correction */
	u_char	retry_count;
	u_char	correction_span;
	u_char	head_offset_count;
	u_char	strobe_offset_count;
	u_char	recovery_time_limit;
};

/*
 * Page Two - Disconnect / Reconnect Control Parameters
 */
struct ccs_disco_reco {
	struct	page_header page_header;
	u_char	buffer_full_ratio;	/* write, how full before reconnect? */
	u_char	buffer_empty_ratio;	/* read, how full before reconnect? */

	u_short	bus_inactivity_limit;	/* how much bus time for busy */
	u_short	disconnect_time_limit;	/* min to remain disconnected */
	u_short	connect_time_limit;	/* min to remain connected */
	u_short	reserved;
};

/*
 * Page Three - Direct Access Device Format Parameters
 */
struct ccs_format {
	struct	page_header page_header;
	u_short	tracks_per_zone;	/*  Handling of Defects Fields */
	u_short	alt_sect_zone;
	u_short alt_tracks_zone;
	u_short	alt_tracks_vol;
	u_short	sect_track;		/* Track Format Field */
	u_short data_sect;		/* Sector Format Fields */
	u_short	interleave;
	u_short	track_skew_factor;
	u_short	cyl_skew_factor;
	u_char	ssec		: 1;	/* Drive Type Field */
	u_char	hsec		: 1;
	u_char	rmb		: 1;
	u_char	surf		: 1;
	u_char	ins		: 1;
	u_char	reserved_1	: 3;
	u_char	reserved_2;
	u_char	reserved_3;
	u_char	reserved_4;
};

/*
 * Page Four - Rigid Disk Drive Geometry Parameters 
 */
struct ccs_geometry {
	struct	page_header page_header;
	u_char	cyl_ub;			/* number of cylinders */
	u_char	cyl_mb;
	u_char	cyl_lb;
	u_char	heads;			/* number of heads */
	u_char	precomp_cyl_ub;		/* cylinder to start precomp */
	u_char	precomp_cyl_mb;
	u_char	precomp_cyl_lb;
	u_char	current_cyl_ub;		/* cyl to start reduced current */
	u_char	current_cyl_mb;
	u_char	current_cyl_lb;
	u_short	step_rate;		/* drive step rate */
	u_char	landing_cyl_ub;		/* landing zone cylinder */
	u_char	landing_cyl_mb;
	u_char	landing_cyl_lb;
	u_char	reserved_1;
	u_char	reserved_2;
	u_char	reserved_3;
};
	
/*
 * Inquiry returns controller specific info
 */
struct inquiry_data {	/* as per CCS spec */
	u_char dev_type;	/* device type */
	u_char rmd	: 1; 	/* removable media semaphore */
	u_char dev_qual	: 7;	/* device type qualifier */
	u_char version;		/* version compatibility info */
	u_char 		: 4;	/* reserved */
	u_char rdf	: 4;	/* response data format */
	u_char length;		/* additional length */
	u_char vndr1_uniq[3];	/* vendor unique */ 
	u_char vndr_id[8];	/* vendor identification - ascii */
	u_char product_id[16];	/* product identification - ascii */
	u_char vndr2_uniq[32];	/* vendor unique */	
} inquiry_data;

char *adap_00_errors[] = {
	"No Additional Sense Information.",
	"No Index / Sector Signal.",
	"No Seek Complete.",
	"Write Fault.",
	"Drive Not Ready.",
	"Drive Not Selected.",
	"No Track Zero Found.",
	"Multiple Drives Selected.",
	"No Address Acknowledged.",
	"Media Not Loaded.",
	"Insufficient Capacity.",
	"Drive Timeout.",
};

char *ccs_00_errors[] = {
	"No Additional Sense Information.",
	"No Index / Sector Signal.",
	"No Seek Complete.",
	"Write Fault.",
	"Drive Not Ready.",
	"Drive Not Selected.",
	"No Track Zero Found.",
	"Multiple Drives Selected.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
};

char *adap_01_errors[] = {
	"I.D. CRC Error.",
	"Unrecoverable Data Error.",
	"I.D. Address Mark Not Found.",
	"Data Address Mark Not Found.",
	"Record Not Found.",
	"Seek Error.",
	"DMA Timeout Error.",
	"Write Protected.",
	"Correctable Data Check.",
	"Bad Block Found.",
	"Interleave Error.",
	"Data Transfer Incomplete.",
	"Unformatted or Bad format on drive.",
	"Self Test Failed.",
	"Defective Track (Media Errors).",
};

char *ccs_01_errors[] = {
	"I.D. CRC or ECC Error.",
	"Unrecoverable Data Error.",
	"I.D. Address Mark Not Found.",
	"Data Address Mark Not Found.",
	"Record Not Found.",
	"Seek Error.",
	"Recovered Data Address Mark.",
	"Recovered Read Error With Read Retries.",
	"Recovered Read Error With ECC Correction.",
	"Defect List Error.",
	"Parameter Overrun.",
	"Synchronous Transfer Error.",
	"Primary Defect List Not Found.",
	"Compare Error.",
	"Recovered ID With ECC Correction.",
	"Reserved Error Reported.",
};

char *adap_02_errors[] = {
	"Invalid command.",
	"Illegal block address.",
	"Aborted.",
	"Volume overflow.",
};

char *ccs_02_errors[] = {
	"Invalid Command Operation Code.",
	"Illegal Block Address.",
	"Illegal Function For Device Type.",
	"Reserved Error Reported.",
	"Illegal Field In CDB.",
	"Invalid LUN.",
	"Invalid Field In Parameter List.",
	"Write Protected.",
	"Medium Changed.",
	"Power On, Reset, Or Bus Device Reset.",
	"Mode Select Parameters Changed.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
};

char *ccs_03_errors[] = {
	"Incompatible Cartridge.",
	"Format Failed.",
	"No Defect Spare Location Available.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
};

char *ccs_04_errors[] = {
	"RAM Failure.",
	"ECC Diagnostic Failure.",
	"Power On Failure.",
	"Message Reject Error.",
	"SCSI Hardware / Firmaware Error.",
	"Select / Reselect Failed.",
	"Unsuccessful Soft Reset.",
	"Parity Error.",
	"Initiator Detected Error.",
	"Inappropriate / Illegal Message.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
	"Reserved Error Reported.",
};

char **adap_SC_errors[] = {
	adap_00_errors,
	adap_01_errors,
	adap_02_errors,
	0, 0, 0,
};

char **ccs_SC_errors[] = {
	ccs_00_errors,
	ccs_01_errors,
	ccs_02_errors,
	ccs_03_errors,
	ccs_04_errors,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

int adap_SC_errct[] = {
	sizeof adap_00_errors / sizeof adap_00_errors[0],
	sizeof adap_01_errors / sizeof adap_01_errors[0],
	sizeof adap_02_errors / sizeof adap_02_errors[0],
	0, 0, 0,
};

int ccs_SC_errct[] = {
	sizeof ccs_00_errors / sizeof ccs_00_errors[0],
	sizeof ccs_01_errors / sizeof ccs_01_errors[0],
	sizeof ccs_02_errors / sizeof ccs_02_errors[0],
	sizeof ccs_03_errors / sizeof ccs_03_errors[0],
	sizeof ccs_04_errors / sizeof ccs_04_errors[0],
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

char *SC_sense7_keys [] = {
	"No Sense.",
	"Recoverable Error.",
	"Not Ready.",
	"Media Error.",
	"Hardware Error.",
	"Illegal Request.",
	"Unit Attention.",
	"Data Protect.",
	"Blank Check.",
	"Vendor Unique.",
	"Copy Aborted.",
	"Aborted Command.",
	"Equal.",
	"Volume Overflow.",
	"Miscompare.",
};

char *SC_cmdlist [] = {
	"Test unit ready",	/* 00 */
	"Rezero unit",		/* 01 */
	"?",
	"Request sense",	/* 03 */
	"Format unit",		/* 04 */
	"?", "?",
	"Reassign Block",	/* 07 */
	"Read",			/* 08 */
	"?",
	"Write",		/* 0A */
	"Seek",			/* 0B */
	"?", "?", "?",
	"Translate",		/* 0F */
	"?", "?",
	"Inquiry",		/* 12 */
	"?", "?",
	"Mode select",		/* 15 */
	"?", "?", "?", "?",
	"Mode Sense",		/* 1A */
	"?",
	"Receive Diagnostic",	/* 1C */
        "Send Diagnostic",      /* 1D */
	"?", "?", "?", "?", "?", "?", "?",
	"Read Capacity",	/* 25 */
	"?", "?", "?", "?", "?", "?", "?", "?", "?",
	"Verify", 		/* 2F */
	"?", "?", "?", "?", "?", "?", "?",
	"Read Defect data ",	/* 37 */
};

