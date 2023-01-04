/*	@(#)disktab.h 1.1 86/09/24 SMI; from UCB 4.2 03/06/83	*/

/*
 * Disk description table, see disktab(5)
 */
#define	DISKTAB		"/usr/etc/disktab"

struct	disktab {
	char	*d_name;		/* drive name */
	char	*d_type;		/* drive type */
	int	d_secsize;		/* sector size in bytes */
	int	d_ntracks;		/* # tracks/cylinder */
	int	d_nsectors;		/* # sectors/track */
	int	d_ncylinders;		/* # cylinders */
	int	d_rpm;			/* revolutions/minute */
	struct	partition {
		int	p_size;		/* #sectors in partition */
		short	p_bsize;	/* block size in bytes */
		short	p_fsize;	/* frag size in bytes */
	} d_partitions[8];
	u_short d_apc;			/* alternates per cylinder */
};

struct	disktab *getdiskbyname();
