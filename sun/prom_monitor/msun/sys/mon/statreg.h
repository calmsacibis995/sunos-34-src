/*	@(#)statreg.h 2.3 83/09/16 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * statreg.h
 *
 * Bit definitions for MC68000 processor status register (SR)
 *
 * Taken from MC68000 Microprocessor User's Manual, page 1-4
 */

#define	SR_CARRY	0x1	/* C (carry) bit */
#define	SR_C		0x1	/* C (carry) bit */
#define	SR_OVERFLOW	0x2	/* V (overflow) bit */
#define	SR_V		0x2	/* V (overflow) bit */
#define	SR_ZERO		0x4	/* Z (zero) bit */
#define	SR_Z		0x4	/* Z (zero) bit */
#define	SR_NEGATIVE	0x8	/* N (negative) bit */
#define	SR_N		0x8	/* N (negative) bit */
#define	SR_EXTEND	0x10	/* X (extend) bit */
#define	SR_X		0x10	/* X (extend) bit */

#define	SR_CCODES	(SR_C|SR_V|SR_Z|SR_N|SR_X)	/* condition codes */

#define	SR_INTMASK	0x700	/* interrupt level */

#define	SR_SUPERVISOR	0x2000	/* supervisor state bit */

#define	SR_TRACE	0x8000	/* trace trap bit */
