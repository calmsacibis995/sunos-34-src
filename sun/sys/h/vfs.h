/*	@(#)vfs.h 1.1 86/09/25 SMI	*/

/*
 * File system identifier. Should be unique (at least per machine).
 */
typedef struct {
	long val[2];			/* file system id type */
} fsid_t;

/*
 * File identifier. Should be unique per filesystem on a single machine.
 */
#define	MAXFIDSZ	16
#define	freefid(fidp) \
    kmem_free((caddr_t)(fidp), sizeof (struct fid) - MAXFIDSZ + (fidp)->fid_len)

struct fid {
	u_short		fid_len;		/* length of data in bytes */
	char		fid_data[MAXFIDSZ];	/* data (variable length) */
};

/*
 * Structure per mounted file system.
 * Each mounted file system has an array of
 * operations and an instance record.
 * The file systems are put on a singly linked list.
 */
struct vfs {
	struct vfs	*vfs_next;		/* next vfs in vfs list */
	struct vfsops	*vfs_op;		/* operations on vfs */
	struct vnode	*vfs_vnodecovered;	/* vnode we mounted on */
	int		vfs_flag;		/* flags */
	int		vfs_bsize;		/* native block size */
	fsid_t		vfs_fsid;		/* file system id */
	u_short		vfs_exroot;		/* exported fs uid 0 mapping */
	short		vfs_exflags;		/* exported fs flags */
	caddr_t		vfs_data;		/* private data */
};

/*
 * vfs flags.
 * VFS_MLOCK lock the vfs so that name lookup cannot proceed past the vfs.
 * This keeps the subtree stable during mounts and unmounts.
 */
#define VFS_RDONLY	0x01		/* read only vfs */
#define VFS_MLOCK	0x02		/* lock vfs so that subtree is stable */
#define VFS_MWAIT	0x04		/* someone is waiting for lock */
#define VFS_NOSUID	0x08		/* someone is waiting for lock */
#define	VFS_EXPORTED	0x10		/* file system is exported (NFS) */

/*
 * exported vfs flags.
 */
#define	EX_RDONLY	0x01		/* exported read only */

/*
 * Operations supported on virtual file system.
 */
struct vfsops {
	int	(*vfs_mount)();		/* mount file system */
	int	(*vfs_unmount)();	/* unmount file system */
	int	(*vfs_root)();		/* get root vnode */
	int	(*vfs_statfs)();	/* get fs statistics */
	int	(*vfs_sync)();		/* flush fs buffers */
	int	(*vfs_vget)();		/* get vnode from fid */
};

#define VFS_MOUNT(VFSP, PATH, DATA) \
				 (*(VFSP)->vfs_op->vfs_mount)(VFSP, PATH, DATA)
#define VFS_UNMOUNT(VFSP)	 (*(VFSP)->vfs_op->vfs_unmount)(VFSP)
#define VFS_ROOT(VFSP, VPP)	 (*(VFSP)->vfs_op->vfs_root)(VFSP,VPP)
#define VFS_STATFS(VFSP, SBP)	 (*(VFSP)->vfs_op->vfs_statfs)(VFSP,SBP)
#define VFS_SYNC(VFSP)		 (*(VFSP)->vfs_op->vfs_sync)(VFSP)
#define VFS_VGET(VFSP, VPP, FIDP) (*(VFSP)->vfs_op->vfs_vget)(VFSP, VPP, FIDP)

/*
 * file system statistics
 */
struct statfs {
	long f_type;			/* type of info, zero for now */
	long f_bsize;			/* fundamental file system block size */
	long f_blocks;			/* total blocks in file system */
	long f_bfree;			/* free block in fs */
	long f_bavail;			/* free blocks avail to non-superuser */
	long f_files;			/* total file nodes in file system */
	long f_ffree;			/* free file nodes in fs */
	fsid_t f_fsid;			/* file system id */
	long f_spare[7];		/* spare for later */
};

#ifdef KERNEL
/*
 * public operations
 */
extern void	vfs_mountroot();	/* mount the root */
extern int	vfs_add();		/* add a new vfs to mounted vfs list */
extern void	vfs_remove();		/* remove a vfs from mounted vfs list */
extern int	vfs_lock();		/* lock a vfs */
extern void	vfs_unlock();		/* unlock a vfs */
extern struct vfs *getvfs();		/* return vfs given fsid */

#define VFS_INIT(VFSP, OP, DATA)	{ \
	(VFSP)->vfs_next = (struct vfs *)0; \
	(VFSP)->vfs_op = (OP); \
	(VFSP)->vfs_flag = 0; \
	(VFSP)->vfs_exflags = 0; \
	(VFSP)->vfs_data = (DATA); \
}

/*
 * globals
 */
extern struct vfs *rootvfs;		/* ptr to root vfs structure */

#endif
