/*	@(#)xycom.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */
#ifndef _XYCOM_
#define _XYCOM_

/*
 * Common definitions for Xylogics disk drivers.  Names are prefixed
 * with 'xy', but these definitions also apply to the xd driver.
 */

/*
 * States for the command block flags
 */
#define XY_FBSY		0x0001		/* cmdblock in use */
#define XY_WANTED	0x0002		/* process waiting for iopb */
#define XY_WAIT		0x0004		/* process waiting for completion */
#define XY_FRDY		0x0008		/* cmdblock ready for execution */
#define XY_DONE		0x0010		/* operation completed */
#define XY_INRST	0x0020		/* in a restore */
#define XY_INFRD	0x0040		/* in bad block forwarding */
#define XY_NOMSG	0x0080		/* suppress error messages */
#define	XY_FAILED	0x0100		/* command failed */

/*
 * Modes to execute a command
 */
#define XY_SYNCH	0		/* synchronous */
#define XY_ASYNCH	1		/* interrupt, no wait on iopb */
#define XY_ASYNCHWAIT	2		/* interrupt, wait on iopb */

/*
 * Error message control -- if a given bit is set, those errors are
 * printed. All others are suppressed.
 */
#define EL_FORWARD	0x0001		/* block forwarding message */
#define EL_FIXED	0x0002		/* fixed error message */
#define EL_RETRY	0x0004		/* retry message */
#define EL_RESTR	0x0008		/* restore message */
#define EL_RESET	0x0010		/* reset message */
#define EL_FAIL		0x0020		/* failure message */

/*
 * The classes of errors.
 */
#define	XY_SUCC		0x00		/* success -- no error */
#define XY_ERCOR	0x01		/* corrected error */
#define XY_ERTRY	0x02		/* retryable error */
#define XY_ERBSY	0x03		/* drive busy error */
#define XY_ERFLT	0x04		/* drive faulted error */
#define XY_ERHNG	0x05		/* controller hung error */
#define XY_ERFTL	0x06		/* fatal error */

/*
 * Struct for defining actions to be taken for each class of error.
 */
struct xyerract {
	u_char	retry;		/* number of retries */
	u_char	restore;	/* number of drive resets */
	u_char	reset;		/* number of controller resets */
};

/*
 * Struct for defining each recognizable error.
 * Because of the sparse population, an array is not used.
 */
struct xyerror {
	u_char	errno;		/* error number */
	u_char	errlevel;	/* error level (corrected, fatal, etc) */
	u_char	errtype;	/* error type (media vs nonmedia) */
	char	*errmsg;	/* error message */
};

/*
 * Miscellaneous defines.
 */
#define b_cylin b_resid			/* used for disksort */
#define XYNUNIT		32		/* max # of units on system */
#define	XYNLPART	NDKMAP		/* # of logical partitions (8) */
#define	UNIT(dev)	((dev>>3) % XYNUNIT)
#define	LPART(dev)	(dev % XYNLPART)
#define NOLPART		(-1)		/* used for 'non-partition commands */
#define	SECSIZE		512
#define XYWATCHTIMO	20		/* seconds till disk check */
#define XYLOSTINTTIMO	4		/* seconds till lost interrupt */
#define	XYDEFTRKLEN	24		/* length of manufacturer's defect */

#endif _XYCOM_
