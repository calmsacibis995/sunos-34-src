/*	@(#)idprom.h 1.3 84/04/12 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Structure declaration for ID prom in CPU and Ethernet boards
 *
 * Note that in CPU boards the PROM is not addressed this way, tho
 * this is the order of the bytes burned into the PROM.  See getidprom.s
 * and s2map.s for further info.
 */
struct idprom {
	unsigned char	id_format;	/* format identifier */
	/* The following fields are valid only in format IDFORM_1. */
	unsigned char	id_machine;	/* machine type */
	unsigned char	id_ether[6];	/* ethernet address */
	long		id_date;	/* date of manufacture */
	unsigned	id_serial:24;	/* serial number */
	unsigned char	id_xsum;	/* xor checksum */
	unsigned char	id_undef[16];	/* undefined */
};

#define IDFORM_1	1	/* Format number for first ID proms */

#define	IDM_CPU_MULTI	1	/* Machine type for Multibus CPU board */
#define	IDM_CPU_VME	2	/* Machine type for VME CPU board */
