/*	@(#)xyxd.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Values for certain types of track headers as a 32 bit numbers
 */
#define	HDR_SPARE	0xdddddddd	/* Header for spare sector */
#define	HDR_RUNT	0xeeeeeeee	/* Header for runt sector */
#define	HDR_SLIP	0xfefefefe	/* Header for slipped sector */
#define	HDR_ZAP		0xffffffff	/* Header for a zapped sector */
