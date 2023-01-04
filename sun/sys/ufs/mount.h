/*	@(#)mount.h 1.1 86/09/25 SMI; from UCB 4.4 82/07/19	*/

/*
 * Mount structure.
 * One allocated on every mount.
 * Used to find the super block.
 */
struct	mount
{
	struct vfs	*m_vfsp;	/* vfs structure for this filesystem */
	dev_t		m_dev;		/* device mounted */
	struct vnode	*m_devvp;	/* vnode for block device mounted */
	struct buf	*m_bufp;	/* pointer to superblock */
	struct inode	*m_qinod;	/* QUOTA: pointer to quota file */
	u_short		m_qflags;	/* QUOTA: filesystem flags */
	u_long		m_btimelimit;	/* QUOTA: block time limit */
	u_long		m_ftimelimit;	/* QUOTA: file time limit */
};

#ifdef KERNEL
/*
 * Convert vfs ptr to mount ptr. ONLY WORKS IF m_vfs IS FIRST.
 */
#define VFSTOM(VFSP)	((struct mount *)(VFSP->vfs_data))

/*
 * mount table
 */
extern struct mount	mounttab[NMOUNT];

/*
 * Operations
 */
struct mount *getmp();
#endif
