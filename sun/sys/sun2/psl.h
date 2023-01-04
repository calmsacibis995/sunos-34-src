/*	@(#)psl.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 *  Definition of bits in the 68010 status register (SR)
 */
#define	SR_TRACE	0x8000		/* trace mode */
#define	SR_SMODE	0x2000		/* system mode */
#define	SR_INTPRI	0x0700		/* interrupt priority bits */
#define	SR_CC		0x001F		/* all condition code bits */

/* Handy values for SR */
#define	SR_HIGH		0x2700		/* supervisor, high prio */
#define	SR_LOW		0x2000		/* supervisor, low prio */
#define	SR_USER		0x0000		/* user, low prio */
#define	SR_USERCLR	0xFF00		/* system bits */

#define	PSL_USERSET	SR_USER		/* must set for user */
#define	PSL_USERCLR	SR_USERCLR	/* must clear for user */
#define	PSL_ALLCC	SR_CC		/* condition code bits */
#define	PSL_T		SR_TRACE	/* trace bit */
