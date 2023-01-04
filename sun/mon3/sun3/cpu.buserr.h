
/*	@(#)cpu.buserr.h 1.1 86/09/27 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * cpu.buserr.h
 *
 * The Sun-3 Bus Error Register captures the state of the system as of the
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
	unsigned	be_invalid	:1;	/* Page map Valid bit is off */
	unsigned	be_proterr      :1;     /* Protection error */
	unsigned    	be_timeout	:1;     /* Bus access timed out */
	unsigned	be_vmeberr	:1;     /* Bus error signaled on VME */
	unsigned	be_fpaberr	:1;	/* FPA bus error response */
	unsigned	be_fpaenerr	:1;	/* FPA enable error */
	unsigned			:1;	/* Unused */
	unsigned	be_watchdog	:1;	/* Watchdog or User reset */
};

