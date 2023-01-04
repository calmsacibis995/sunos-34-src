/*	@(#)mount.h 1.1 86/09/25 SMI	*/

/*
 * mount options
 */
#define M_RDONLY	0x01		/* mount fs read only */
#define M_NOSUID	0x02		/* mount fs with setuid not allowed */

/*
 * File system types, these corespond to entries in fsconf
 */
#define	MOUNT_UFS	0
#define	MOUNT_NFS	1
#define	MOUNT_PC	2
#define	MOUNT_MAXTYPE	2

struct ufs_args {
	char	*fspec;
};

#ifdef NFS

struct nfs_args {
	struct sockaddr_in	*addr;		/* file server address */
	fhandle_t		*fh;		/* File handle to be mounted */
	int			flags;		/* flags */
	int			wsize;		/* write size in bytes */
	int			rsize;		/* read size in bytes */
	int			timeo;		/* initial timeout in .1 secs */
	int			retrans;	/* times to retry send */
	char			*hostname;	/* server's name */
};

/*
 * NFS mount option flags
 */
#define	NFSMNT_SOFT	0x001	/* soft mount (hard is default) */
#define	NFSMNT_WSIZE	0x002	/* set write size */
#define	NFSMNT_RSIZE	0x004	/* set read size */
#define	NFSMNT_TIMEO	0x008	/* set initial timeout */
#define	NFSMNT_RETRANS	0x010	/* set number of request retrys */
#define	NFSMNT_HOSTNAME	0x020	/* set hostname for error printf */
#define	NFSMNT_INT	0x040	/* allow interrupts on hard mount */
#endif NFS

#ifdef PCFS
struct pc_args {
	char	*fspec;
};
#endif PCFS

#ifdef KERNEL
/*
 * mount filesystem type switch table
 */
extern struct vfsops *vfssw[];
#endif
