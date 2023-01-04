/*	@(#)dkio.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */
#ifndef _DKIO_
#define _DKIO_

#ifdef KERNEL
#include "../h/ioctl.h"
#include "../sun/dklabel.h"
#else
#include <sys/ioctl.h>
#include <sun/dklabel.h>
#endif

/*
 * Structures and definitions for disk io control commands
 */

/*
 * Structures used as data by ioctl calls.
 */

/*
 * Used for controller info
 */
struct dk_info {
	int	dki_ctlr;		/* controller address */
	short	dki_unit;		/* unit (slave) address */
	short	dki_ctype;		/* controller type */
	short	dki_flags;		/* flags */
};

#define	DK_DEVLEN	8		/* device name max length */
/*
 * Used for configuration info
 */
struct dk_conf {
	char	dkc_cname[DK_DEVLEN];	/* controller name */
	u_short	dkc_ctype;		/* controller type */
	u_short	dkc_flags;		/* flags */
	short	dkc_cnum;		/* controller number */
	caddr_t	dkc_addr;		/* controller address */
	u_int	dkc_space;		/* controller bus type */
	int	dkc_prio;		/* interrupt priority */
	int	dkc_vec;		/* interrupt vector */
	char	dkc_dname[DK_DEVLEN];	/* drive name */
	short	dkc_unit;		/* unit number */
	short	dkc_slave;		/* slave number */
};

/*
 * Controller types
 */
#define	DKC_UNKNOWN	0
#define	DKC_SMD2180	1
#define	DKC_WDC2880	2
#define	DKC_SMD2181	3
#define	DKC_DSD5215	5
#define	DKC_XY450	6
#define	DKC_ACB4000	7
#define DKC_MD21	8
#define	DKC_XB1401	10

/*
 * Flags
 */
#define	DKI_BAD144	0x01	/* use DEC std 144 bad sector fwding */
#define	DKI_MAPTRK	0x02	/* controller does track mapping */
#define	DKI_FMTTRK	0x04	/* formats only full track at a time */
#define	DKI_FMTVOL	0x08	/* formats only full volume at a time */

/*
 * Used for drive info
 */
struct dk_type {
	u_short dkt_hsect;		/* hard sector count (read only) */
	u_short dkt_promrev;		/* prom revision (read only) */
	u_char	dkt_drtype;		/* drive type (ctlr specific) */
	u_char	dkt_drstat;		/* drive status (ctlr specific, ro) */
};

/*
 * Used for all partitions
 */
struct dk_allmap {
	struct dk_map	dka_map[NDKMAP];
};

/*
 * Used for bad sector map
 */
struct dk_badmap {
	caddr_t dkb_bufaddr;		/* address of user's map buffer */
};

/*
 * Definition of a disk's geometry
 */
struct dk_geom {
	unsigned short	dkg_ncyl;	/* # of data cylinders */
	unsigned short	dkg_acyl;	/* # of alternate cylinders */
	unsigned short	dkg_bcyl;	/* cyl offset (for fixed head area) */
	unsigned short	dkg_nhead;	/* # of heads */
	unsigned short	dkg_bhead;	/* head offset (for Larks, etc.) */
	unsigned short	dkg_nsect;	/* # of sectors per track */
	unsigned short	dkg_intrlv;	/* interleave factor */
	unsigned short	dkg_gap1;	/* gap 1 size */
	unsigned short	dkg_gap2;	/* gap 2 size */
	unsigned short	dkg_apc;	/* alternates per cyl (SCSI only) */
	unsigned short	dkg_extra[9];	/* for compatible expansion */
};

/*
 * Used for generic commands
 */
struct dk_cmd {
	u_short	dkc_cmd;		/* command to be executed */
	daddr_t	dkc_blkno;		/* disk address for command */
	int	dkc_secnt;		/* sector count for command */
	caddr_t	dkc_bufaddr;		/* user's buffer address */
	u_int	dkc_buflen;		/* size of user's buffer */
};

/*
 * Used for disk diagnostics
 */
struct dk_diag {
	char	dkd_mode;		/* unit's mode of operation */
	u_char	dkd_errno;		/* most recent error */
	u_char	dkd_errlevel;		/* most recent error level */
	u_char	dkd_errtype;		/* most recent error type */
	int	dkd_errsect;		/* most recent sector in error */
	u_short	dkd_errcmd;		/* command in error */
};

/*
 * Operating modes
 */
#define	DK_NORMAL	0		/* normal operation */
#define	DK_DIAG		1		/* diagnostic operation */

/*
 * Error types
 */
#define	DK_NONMEDIA	0		/* not caused by a media defect */
#define	DK_ISMEDIA	1		/* caused by a media defect */

/*
 * Disk io control commands
 */
#define	DKIOCGGEOM	_IOR(d, 2, struct dk_geom)	/* Get geometry */
#define	DKIOCSGEOM	_IOW(d, 3, struct dk_geom)	/* Set geometry */
#define	DKIOCGPART	_IOR(d, 4, struct dk_map)	/* Get partition info */
#define	DKIOCSPART	_IOW(d, 5, struct dk_map)	/* Set partition info */
#define	DKIOCINFO	_IOR(d, 8, struct dk_info)	/* Get info */
#define	DKIOCGCONF	_IOR(d, 126, struct dk_conf)	/* Get conf info */
#define DKIOCSTYPE	_IOW(d, 125, struct dk_type)	/* Set drive info */
#define DKIOCGTYPE	_IOR(d, 124, struct dk_type)	/* Get drive info */
#define DKIOCSAPART	_IOW(d, 123, struct dk_allmap)	/* Set all partitions */
#define DKIOCGAPART	_IOR(d, 122, struct dk_allmap)	/* Get all partitions */
#define DKIOCSBAD	_IOW(d, 121, struct dk_badmap)	/* Set bad sector map */
#define DKIOCGBAD	_IOW(d, 120, struct dk_badmap)	/* Get bad sector map */
#define	DKIOCSCMD	_IOW(d, 119, struct dk_cmd)	/* Set generic cmd */
#define	DKIOCGCMD	_IOW(d, 118, struct dk_cmd)	/* Get generic cmd */
#define	DKIOCSDIAG	_IOW(d, 117, struct dk_diag)	/* Set diagnostics */
#define	DKIOCGDIAG	_IOR(d, 116, struct dk_diag)	/* Get diagnostics */

#endif !_DKIO_

