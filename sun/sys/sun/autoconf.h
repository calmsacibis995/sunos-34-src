/* @(#)autoconf.h 1.1 86/09/25 SMI */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */
#ifndef _AUTOCONF_
#define _AUTOCONF_

/*
 * Defines for bus types.  These are magic cookies passed between config
 * and the kernel to tell what bus each device is on.
 */
#define SP_BUSMASK	0x0000FFFF	/* mask for bus type */
#define SP_VIRTUAL	0x0001		/* virtual address */
#define SP_OBMEM	0x0002		/* on board memory */
#define SP_OBIO		0x0004		/* on board i/o */
#define SP_MBMEM	0x0010		/* multibus memory (sun2 only) */
#define SP_MBIO		0x0020		/* multibus i/o (sun2 only) */
#define SP_VME16D16	0x0100		/* vme 16/16 */
#define SP_VME24D16	0x0200		/* vme 24/16 */
#define SP_VME32D16	0x0400		/* vme 32/16 (sun3 only) */
#define SP_VME16D32	0x1000		/* vme 16/32 (sun3 only) */
#define SP_VME24D32	0x2000		/* vme 24/32 (sun3 only) */
#define SP_VME32D32	0x4000		/* vme 32/32 (sun3 only) */

/*
 * Defines for encoding the machine type in the space field of
 * each device.
 */
#define SP_MACHMASK	0xFFFF0000	/* space mask for machine type */
#define MAKE_MACH(m)	((m)<<16)
#define SP_MACH_ALL	MAKE_MACH(0)	/* 0 implies all machines */

#endif _AUTOCONF_

