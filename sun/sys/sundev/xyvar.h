/*	@(#)xyvar.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */
#ifndef _XYVAR_
#define _XYVAR_

/*
 * Structure definitions for Xylogics 450 disk driver.
 */
#include "../sundev/xycreg.h"
#include "../sundev/xyreg.h"
#include "../sundev/xycom.h"

/*
 * Structure needed to execute a command.  Contains controller & iopb ptrs
 * and some error recovery information.  The link is used to identify
 * chained iopbs.
 */
struct xycmdblock {
	struct	xyctlr *c;		/* ptr to controller */
	struct	xyiopb *iopb;		/* ptr to IOPB */
	struct	xycmdblock *next;	/* next iopb in ctlr chain */
	u_char	retries;		/* retry count */
	u_char	restores;		/* restore count */
	u_char	resets;			/* reset count */
	u_char	slave;			/* current drive no. */
	u_short	cmd;			/* current command */
	u_short	flags;			/* state information */
	caddr_t	baddr;			/* physical buffer address */
	daddr_t	blkno;			/* current block */
	daddr_t altblkno;		/* alternate block (forwarding) */
	u_short	nsect;			/* sector count active */
	short	device;			/* current minor device */
};

/*
 * Data per unit
 */
struct xyunit {
	struct	dk_map *un_map;		/* logical partitions */
	char	un_attached;		/* unit has been attached */
	char	un_present;		/* unit is present */
	char	un_mode;		/* operating mode */
	u_char	un_drtype;		/* drive type */
	struct	dk_geom *un_g;		/* disk geometry */
	struct	buf *un_rtab;		/* for physio */
	int	un_ltick;		/* last time active */
	struct	mb_device *un_md;	/* generic unit */
	struct	mb_ctlr *un_mc;		/* generic controller */
	struct	dkbad *un_bad;		/* bad sector info */
	int	un_errsect;		/* sector in error */
	struct	xyerror *un_errptr;	/* ptr to last error */
	u_short	un_errcmd;		/* command in error */
	struct	xycmdblock un_cmd;	/* current command info */
};

/*
 * Data per controller
 */
struct xyctlr {
	struct	xyunit *c_units[XYUNPERC];	/* units on controller */
	struct	xydevice *c_io;			/* ptr to I/O space data */
	struct	xycmdblock *c_chain;		/* ptr to iopb chain */
	char	c_present;			/* controller is present */
	char	c_addrlen;			/* 20/24 bit addressing */
	int	c_wantint;			/* controller is busy */
	struct	xycmdblock c_cmd;		/* used for special commands */
};

#endif _XYVAR_

