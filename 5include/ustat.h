/*	@(#)ustat.h 1.1 86/09/24 SMI; from S5R2 1.1	*/

struct  ustat {
	daddr_t	f_tfree;	/* total free */
	ino_t	f_tinode;	/* total inodes free */
	char	f_fname[6];	/* filsys name */
	char	f_fpack[6];	/* filsys pack name */
};
