/*	@(#)s2misc.h 1.5 83/09/16 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Miscellaneous information about the Sun-2.
 *
 * This file is used for both standalone code (ROM Monitor, 
 * Diagnostics, boot programs, etc) and for the Unix kernel.  IF YOU HAVE
 * TO CHANGE IT to fit your application, MOVE THE CHANGE BACK TO THE PUBLIC
 * COPY, and make sure the change is upward-compatible.  The last thing we 
 * need is seventeen different copies of this file, like we have with the
 * Sun-1 header files.
 */


/*
 * The on-board ZSCC (Serial Communications Controller) chips are fed with
 * a PCLK (processor clock) value of 4.9152MHz or about 204ns.  (This must
 * be known when calculating time constants for the baud rate generator.)
 */
#define	ZSCC_PCLK	4915200


/*
 * The on-board AM9513 System Timing Controller chip is fed with an FIN
 * clock of 4.9152MHz, or about 204ns.  (This must be known when calculating
 * timer values.)
 */
#define	CLK_BASIC	4915200


/*
 * The Sun-2 board currently uses 68010's which bring out 24 address lines.
 * We could #define ADDR_LEN log16(ADRSPC_SIZE) but that somehow doesn't work.
 */
#define	ADDR_LEN	6	/* Hex characters for printing addresses */
