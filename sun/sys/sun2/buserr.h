/*	@(#)buserr.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Definitions and structures related to bus error handling.
 */

#ifndef _BUSERR_
#define	_BUSERR_
#ifndef LOCORE
/*
 * Auxiliary information stacked by hardware on Bus Error
 * or Address Error.
 */

/*
 * 68010 "type 0" normal four word stack.
 */
#define	SF_NORMAL	0x0

/*
 * 68010 "type 8" long bus error stack frame.
 */
#define	SF_LONG8	0x8
struct	bei_long8 {
	u_int	bei_rerun : 1;	/* rerun bus cycle */
	u_int		  : 1;
	u_int	bei_ifetch: 1;	/* inst fetch (1=true) */
	u_int	bei_dfetch: 1;	/* data fetch */
	u_int	bei_rmw	  : 1;	/* read/modify/write */
	u_int	bei_hibyte: 1;	/* high byte transfer */
	u_int	bei_bytex : 1;	/* byte transfer */
	u_int	bei_rw	  : 1;	/* read=1,write=0 */
	u_int		  : 4;
	u_int	bei_fcode : 4;	/* function code */
	int	bei_accaddr;	/* access address */
	u_int		  : 16;	/* undefined */
	short	bei_dob;		/* data output buffer */
	u_int		  : 16;	/* undefined */
	short	bei_dib;		/* data input buffer */
	u_int		  : 16;	/* undefined */
	short	bei_irc;		/* inst buffer */
	short	bei_maskpc;	/* chip mask # & micropc */
	short	bei_undef[15];	/* undefined */
};
#endif

/*
 * The System Bus Error Register captures the state of the
 * system as of the last bus error.  The Bus Error Register
 * is latched until read and is addressed in FC_MAP space.
 *
 * If be_proterr is set and be_valid == 1, the protection
 * field of the page map entry caused the bus error.  If
 * be_proterr is set and be_valid == 0, the (in)validity
 * bit of the page map entry caused the bus error.
 *
 * The rest of the bits are 1 if they caused the error.
 */
#ifndef LOCORE
struct	buserrorreg {
	unsigned			:8;
	unsigned	be_valid	:1;	/* Page map Valid bit is on */
	unsigned	be_vmebuserr	:1;	/* Bus error signaled on VME */
	unsigned			:2;	/* Reserved */
	unsigned	be_proterr	:1;	/* Protection error */
	unsigned	be_timeout	:1;	/* Bus access timed out */
	unsigned	be_parerr_u	:1;	/* Parity error, upper byte */
	unsigned	be_parerr_l	:1;	/* Parity error, lower byte */
};
#endif

/*
 * Equivalent bits.
 */
#define	BE_PARERR_L	0x01		/* parity error, lower byte */
#define	BE_PARERR_U	0x02		/* parity error, upper byte */
#define	BE_TIMEOUT	0x04		/* bus access timed out */
#define	BE_PROTERR	0x08		/* protection error */
#define	BE_VMEBUSERR	0x40		/* bus error signaled on VMEbus */
#define	BE_VALID	0x80		/* page map was valid */

#define	BUSERRREG	0x0C		/* addr of buserr reg in FC_MAP space */

#define	BUSERR_BITS	"\20\10VALID\7VMEBUSERR\4PROTERR\3TIMEOUT\2UPARERR\1LPARERR"
#endif
