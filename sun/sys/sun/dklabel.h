/*	@(#)dklabel.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */
#ifndef _DKLABEL_
#define _DKLABEL_

/*
 * Miscellaneous defines
 */
#define	DKL_MAGIC	0xDABE	/* magic number */
#define	NDKMAP	8		/* # of logical partitions */

/*
 * Format of a Sun SMD disk label.
 * Resides in cylinder 0, sector 0 on each head which is the first
 * head of a physical partition (e.g., heads 0 and 2 for a CDC Lark).
 * dkl_ppart gives the physical partition number (currently only 0 or 1).
 * dkl_bhead must match the head on which the label is found;
 * otherwise the label was probably overwritten by another.
 *
 * sizeof(struct dk_label) should be 512 (sector size)
 */
struct dk_label {
	char	dkl_asciilabel[128];	/* for compatibility */
	char	dkl_pad[512-(128+8*8+12*2)];
	unsigned short	dkl_apc;	/* alternates per cylinder */
	unsigned short	dkl_gap1;	/* size of gap 1 */
	unsigned short	dkl_gap2;	/* size of gap 2 */
	unsigned short	dkl_intrlv;	/* interleave factor */
	unsigned short	dkl_ncyl;	/* # of data cylinders */
	unsigned short	dkl_acyl;	/* # of alternate cylinders */
	unsigned short	dkl_nhead;	/* # of heads in this partition */
	unsigned short	dkl_nsect;	/* # of 512 byte sectors per track */
	unsigned short	dkl_bhead;	/* identifies proper label location */
	unsigned short	dkl_ppart;	/* physical partition # */
	/* */
	struct dk_map {			/* logical partitions */
		daddr_t	dkl_cylno;	/* starting cylinder */
		daddr_t dkl_nblk;	/* number of blocks */
	} dkl_map[NDKMAP];
	unsigned short	dkl_magic;	/* identifies this label format */
	unsigned short	dkl_cksum;	/* xor checksum of sector */
};


#endif !_DKLABEL_
