/*	@(#)mtio.h 1.1 86/09/25 SMI; from UCB 4.10 83/01/17	*/

/*
 * Structures and definitions for mag tape io control commands
 */

/* structure for MTIOCTOP - mag tape op command */
struct	mtop	{
	short	mt_op;		/* operations defined below */
	daddr_t	mt_count;	/* how many of them */
};

/* operations */
#define MTWEOF	0	/* write an end-of-file record */
#define MTFSF	1	/* forward space file */
#define MTBSF	2	/* backward space file */
#define MTFSR	3	/* forward space record */
#define MTBSR	4	/* backward space record */
#define MTREW	5	/* rewind */
#define MTOFFL	6	/* rewind and put the drive offline */
#define MTNOP	7	/* no operation, sets status only */
#define MTRETEN	8	/* retension the tape */
#define MTERASE	9	/* erase the entire tape */

/* structure for MTIOCGET - mag tape get status command */

struct	mtget	{
	short	mt_type;	/* type of magtape device */
/* the following two registers are grossly device dependent */
	short	mt_dsreg;	/* ``drive status'' register */
	short	mt_erreg;	/* ``error'' register */
/* end device-dependent registers */
	short	mt_resid;	/* residual count */
/* the following two are not yet implemented */
	daddr_t	mt_fileno;	/* file number of current position */
	daddr_t	mt_blkno;	/* block number of current position */
/* end not yet implemented */
};

/*
 * Constants for mt_type byte
 */
#define	MT_ISTS		0x01		/* vax: unibus ts-11 */
#define	MT_ISHT		0x02		/* vax: massbus tu77, etc */
#define	MT_ISTM		0x03		/* vax: unibus tm-11 */
#define	MT_ISMT		0x04		/* vax: massbus tu78 */
#define	MT_ISUT		0x05		/* vax: unibus gcr */
#define	MT_ISCPC	0x06		/* sun: multibus cpc */
#define	MT_ISAR		0x07		/* sun: multibus archive */
#define	MT_ISSC		0x08		/* sun: SCSI archive */
#define	MT_ISXY		0x09		/* sun: Xylogics 472 */

/* mag tape io control commands */
#define	MTIOCTOP	_IOW(m, 1, struct mtop)		/* do a mag tape op */
#define	MTIOCGET	_IOR(m, 2, struct mtget)	/* get tape status */

#ifndef KERNEL
#define	DEFTAPE	"/dev/rmt12"
#endif
