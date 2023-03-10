/*	@(#)pc_dir.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#define PCFNAMESIZE	8
#define PCFEXTSIZE	3

struct pctime {
	short pct_time;			/* hh:mm:ss (little endian) */
	short pct_date;			/* yy:mm:dd (little endian) */
};

/*
 * Shifts and masks for time and date fields, in host byte order.
 */
#define SECSHIFT	0
#define SECMASK		0x1F
#define MINSHIFT	5
#define MINMASK		0x3F
#define HOURSHIFT	11
#define HOURMASK	0x1F

#define DAYSHIFT	0
#define DAYMASK		0x1F
#define MONSHIFT	5
#define MONMASK		0x0F
#define YEARSHIFT	9
#define YEARMASK	0x7F

struct pcdir {
	char pcd_filename[PCFNAMESIZE];	/* file name */
	char pcd_ext[PCFEXTSIZE];	/* file extension */
	char pcd_attr;
	char pcd_unused[10];
	struct pctime pcd_mtime;	/* create/modify time */
	short pcd_scluster;		/* starting cluster (little endian) */
	long pcd_size;			/* file size (little endian) */
};

/*
 * The first char of the file name has special meaning as follows:
 */
#define PCD_UNUSED	0x00	/* directory entry has never been used */
#define PCD_ERASED	0xE5	/* directroy entry was erased */

/*
 * File attributes.
 */
#define	PCA_RDONLY	0x01	/* file is read only */
#define PCA_HIDDEN	0x02	/* file is hidden */
#define PCA_SYSTEM	0x04	/* system file */
#define PCA_LABEL	0x08	/* entry contains the volume label */
#define PCA_DIR		0x10	/* subdirectory */
#define PCA_ARCH	0x20	/* file has been modified since last backup */

/*
 * macros for converting to/from upper or lower case.
 * users may give and get names in lower case, but they are stored on the
 * disk in upper case to be PCDOS compatable
 */
#define toupper(C)	(((C) >= 'a' && (C) <= 'z')? (C) - 'a' + 'A': (C))
#define tolower(C)	(((C) >= 'A' && (C) <= 'Z')? (C) - 'A' + 'a': (C))

#ifdef KERNEL
extern void pc_tvtopct();		/* convert timeval to pctime */
extern void pc_pcttotv();		/* convert pctime to timeval */
extern int pc_validchar();		/* check character valid in filename */

enum de_op	{ DE_CREATE, DE_LINK, DE_RENAME };	/* pc_direnter ops */
#endif
