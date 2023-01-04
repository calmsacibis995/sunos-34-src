/*	@(#) saio.h 1.1 86/09/25 SMI	*/
/*	@(#)saio.h 1.4 85/05/30 Copyright Sun Microsystems	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * header file for standalone I/O package
 */

#include <sys/param.h>
#include "./mon/bootparam.h"

/*
 * io block: the structure passed to or from the device drivers.
 * 
 * Includes pointers to the device
 * in use, a pointer to device-specific data (iopb's or device
 * state information, typically), cells for the use of seek, etc.
 * NOTE: expand at end to preserve compatibility with PROMs
 */
struct saioreq {
	char	si_flgs;
	struct boottab *si_boottab;	/* Points to boottab entry if any */
	char	*si_devdata;		/* Device-specific data pointer */
	int	si_ctlr;		/* Controller number or address */
	int	si_unit;		/* Unit number within controller */
	daddr_t	si_boff;		/* Partition number within unit */
	daddr_t	si_cyloff;
	off_t	si_offset;
	daddr_t	si_bn;			/* Block number to R/W */
	char	*si_ma;			/* Memory address to R/W */
	int	si_cc;			/* Character count to R/W */
	struct	saif *si_sif;		/* interface pointer */
};


#define F_READ	01
#define F_WRITE	02
#define F_ALLOC	04
#define F_FILE	010

/*
 * request codes. Must be the same as F_XXX above
 */
#define	READ	F_READ
#define	WRITE	F_WRITE

/*
 * How many buffers to make, and how many files can be open at once.
 */
#define	NBUFS	4
#define NFILES	8

/*
 * Ethernet interface descriptor
 */
struct saif {
	int	(*sif_xmit)();		/* transmit packet */
	int	(*sif_poll)();		/* check for and receive packet */
	int	(*sif_reset)();		/* reset interface */
};
