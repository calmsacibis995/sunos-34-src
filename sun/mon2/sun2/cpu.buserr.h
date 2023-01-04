/*	@(#)cpu.buserr.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * cpu.buserr.h
 *
 * The Sun-2 Bus Error Register captures the state of the system as of the
 * last bus error.  (If multiple bus errors occur, only the last one is kept.)
 * Enough information is provided to uniquely determine the cause
 * of the bus error.
 *
 * If be_proterr is set and be_valid == 1, the protection field of the
 * page map entry caused the bus error.  If be_proterr is set and be_valid
 * == 0, the (in)validity bit of the page map entry caused the bus error.
 *
 * The rest of the bits are 1 if they caused the error.
 */
struct buserrorreg {
	unsigned	be_valid	:1;	/* Page map Valid bit is on */
	unsigned	be_vmeberr	:1;	/* Bus error signaled on VME */
	unsigned			:2;	/* Reserved */
	unsigned	be_proterr	:1;	/* Protection error */
	unsigned	be_timeout	:1;	/* Bus access timed out */
	unsigned	be_parerr_u	:1;	/* Parity error, upper byte */
	unsigned	be_parerr_l	:1;	/* Parity error, lower byte */
};

#define	BUSERR_BITS "\20\10VALID\7VMEBUSERR\4PROTERR\3TIMEOUT\2UPARERR\1LPARERR"
