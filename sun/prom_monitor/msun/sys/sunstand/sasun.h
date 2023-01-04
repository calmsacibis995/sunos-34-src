/*	@(#)sasun.h 1.1 84/12/21 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Manifest constants for the Sun standalone environment
 *
 * Get MBMEM_BASE and MBIO_BASE from s2addrs.
 */
#include "../mon/s2addrs.h"

/*
 * Buffer offset in Multibus memory space
 * must be above all IOPBs to prevent clobbering
 */
#define	DMADDR		0x2000	
