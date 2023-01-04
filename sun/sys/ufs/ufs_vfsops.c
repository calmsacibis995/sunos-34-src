/*	@(#)ufs_vfsops.c 1.1 86/09/25 SMI; from UCB 4.1 83/05/27	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/pathname.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/conf.h"
#include "../ufs/fs.h"
#include "../ufs/mount.h"
#include "../ufs/inode.h"
#undef NFS
#include "../h/mount.h"

/*
 * ufs vfs operations.
 */
int ufs_mount();
int ufs_unmount();
int ufs_root();
int ufs_statfs();
int ufs_sync();
int ufs_vget();

struct vfsops ufs_vfsops = {
	ufs_mount,
	ufs_unmount,
	ufs_root,
	ufs_statfs,
	ufs_sync,
	ufs_vget,
};

#ifndef NFSROOT
/*
 * this is the default filesystem type.
 * this should be setup by the configurator
 */
extern int ufs_mountroot();
int (*rootfsmount)() = ufs_mountroot;
#endif

/*
 * Default device to mount on.
 */
extern struct vnode *rootvp;

/*
 * Mount table.
 */
struct mount	mounttab[NMOUNT];

/*
 * ufs_mount system call
 */
ufs_mount(vfsp, path, data)
	struct vfs *vfsp;
	char *path;
	caddr_t data;
{
	int error;
	dev_t dev;
	struct vnode *devvp;
	struct ufs_args args;

	/*
	 * Get arguments
	 */
	error = copyin(data, (caddr_t)&args, sizeof (struct ufs_args));
	if (error) {
		return (error);
	}
	if ((error = getmdev(args.fspec, &dev)) != 0)
		return (error);
	/*
	 * make a special (device) vnode for the filesystem
	 */
	devvp = bdevvp(dev);
	/*
	 * Mount the filesystem.
	 */
	error = mountfs(&devvp, path, vfsp);
	if (error) {
		VN_RELE(devvp);
	}
	return (error);
}

/*
 * Called by vfs_mountroot when ufs is going to be mounted as root
 */
ufs_mountroot()
{
	struct vfs *vfsp;
	register struct fs *fsp;
	register int error;

	vfsp = (struct vfs *)kmem_alloc(sizeof (struct vfs));
	VFS_INIT(vfsp, &ufs_vfsops, (caddr_t)0);
	error = mountfs(&rootvp, "/", vfsp);
	if (error) {
		kmem_free((caddr_t)vfsp, sizeof (struct vfs));
		return (error);
	}
	error = vfs_add((struct vnode *)0, vfsp, 0);
	if (error) {
		(void) unmount1(vfsp, 0);
		kmem_free((caddr_t)vfsp, sizeof (struct vfs));
		return (error);
	}
	vfs_unlock(vfsp);
	fsp = ((struct mount *)(vfsp->vfs_data))->m_bufp->b_un.b_fs;
	inittodr(fsp->fs_time);
	return (0);
}

int
mountfs(devvpp, path, vfsp)
	struct vnode **devvpp;
	char *path;
	struct vfs *vfsp;
{
	register struct fs *fsp;
	register struct mount *mp = 0;
	register struct buf *bp = 0;
	struct buf *tp = 0;
	int error;
	int blks;
	caddr_t space;
	int i;
	int size;
	u_int len;

	/*
	 * Open block device mounted on.
	 * When bio is fixed for vnodes this can all be vnode operations
	 */
	error = VOP_OPEN(devvpp,
	    (vfsp->vfs_flag & VFS_RDONLY) ? FREAD : FREAD|FWRITE, u.u_cred);
	if (error)
		return (error);
	/*
	 * read in superblock
	 */
	tp = bread(*devvpp, SBLOCK, SBSIZE);
	if (tp->b_flags & B_ERROR)
		goto out;
	/*
	 * check for dev already mounted on
	 */
	for (mp = &mounttab[0]; mp < &mounttab[NMOUNT]; mp++) {
		if (mp->m_bufp != 0 && (*devvpp)->v_rdev == mp->m_dev) {
			mp = 0;
			goto out;
		}
	}
	/*
	 * find empty mount table entry
	 */
	for (mp = &mounttab[0]; mp < &mounttab[NMOUNT]; mp++) {
		if (mp->m_bufp == 0)
			goto found;
	}
	mp = 0;
	goto out;
found:
	vfsp->vfs_data = (caddr_t)mp;
	mp->m_vfsp = vfsp;
	mp->m_bufp = tp;	/* just to reserve this slot */
	mp->m_dev = NODEV;
	mp->m_devvp = *devvpp;
	fsp = tp->b_un.b_fs;
	if (fsp->fs_magic != FS_MAGIC || fsp->fs_bsize > MAXBSIZE)
		goto out;
	/*
	 * Copy the super block into a buffer in it's native size.
	 */
	bp = geteblk((int)fsp->fs_sbsize);
	mp->m_bufp = bp;
	bcopy((caddr_t)tp->b_un.b_addr, (caddr_t)bp->b_un.b_addr,
	   (u_int)fsp->fs_sbsize);
	brelse(tp);
	tp = 0;
	fsp = bp->b_un.b_fs;
	if (vfsp->vfs_flag & VFS_RDONLY) {
		fsp->fs_ronly = 1;
	} else {
		fsp->fs_fmod = 1;
		fsp->fs_ronly = 0;
	}
	vfsp->vfs_bsize = fsp->fs_bsize;
	/*
	 * Read in cyl group info
	 */
	blks = howmany(fsp->fs_cssize, fsp->fs_fsize);
	space = wmemall(vmemall, (int)fsp->fs_cssize);
	if (space == 0)
		goto out;
	for (i = 0; i < blks; i += fsp->fs_frag) {
		size = fsp->fs_bsize;
		if (i + fsp->fs_frag > blks)
			size = (blks - i) * fsp->fs_fsize;
		tp = bread(mp->m_devvp, (daddr_t)fsbtodb(fsp, fsp->fs_csaddr+i),
		    size);
		if (tp->b_flags&B_ERROR) {
			wmemfree(space, (int)fsp->fs_cssize);
			goto out;
		}
		bcopy((caddr_t)tp->b_un.b_addr, space, (u_int)size);
		fsp->fs_csp[i / fsp->fs_frag] = (struct csum *)space;
		space += size;
		brelse(tp);
		tp = 0;
	}
	mp->m_dev = mp->m_devvp->v_rdev;
	vfsp->vfs_fsid.val[0] = (long)mp->m_dev;
	vfsp->vfs_fsid.val[1] = MOUNT_UFS;
	(void) copystr(path, fsp->fs_fsmnt, sizeof(fsp->fs_fsmnt)-1, &len);
	bzero(fsp->fs_fsmnt + len, sizeof (fsp->fs_fsmnt) - len);
	return (0);
out:
	if (mp)
		mp->m_bufp = 0;
	if (bp)
		brelse(bp);
	if (tp)
		brelse(tp);
	(void) VOP_CLOSE(*devvpp,
	    (vfsp->vfs_flag & VFS_RDONLY) ? FREAD : FREAD|FWRITE, u.u_cred);
	return (EBUSY);
}

/*
 * The System V Interface Definition requires a "umount" operation
 * which takes a device pathname as an argument.  This requires this
 * to be a system call.
 */

umount(uap)
	struct a {
		char	*fspec;
	} *uap;
{
	register struct mount *mp;
	dev_t dev;

	if (!suser())
		return;

	if ((u.u_error = getmdev(uap->fspec, &dev)) != 0)
		return;

	if ((mp = getmp(dev)) == NULL) {
		u.u_error = EINVAL;
		return;
	}

	dounmount(mp->m_vfsp);
}

/*
 * vfs operations
 */

ufs_unmount(vfsp)
	struct vfs *vfsp;
{

	return (unmount1(vfsp, 0));
}

unmount1(vfsp, forcibly)
	register struct vfs *vfsp;
	int forcibly;
{
	dev_t dev;
	register struct mount *mp;
	register struct fs *fs;
	register int stillopen;
	int flag;

	mp = (struct mount *)vfsp->vfs_data;
	dev = mp->m_dev;
#ifdef QUOTA
	if ((stillopen = iflush(dev, mp->m_qinod)) < 0 && !forcibly)
#else
	if ((stillopen = iflush(dev)) < 0 && !forcibly)
#endif
		return (EBUSY);
	if (stillopen < 0)
		return (EBUSY);			/* XXX */
#ifdef QUOTA
	(void)closedq(mp);
	/*
	 * Here we have to iflush again to get rid of the quota inode.
	 * A drag, but it would be ugly to cheat, & this doesn't happen often
	 */
	(void)iflush(dev, (struct inode *)NULL);
#endif
	fs = mp->m_bufp->b_un.b_fs;
	wmemfree((caddr_t)fs->fs_csp[0], (int)fs->fs_cssize);
	flag = !fs->fs_ronly;
	brelse(mp->m_bufp);
	mp->m_bufp = 0;
	mp->m_dev = 0;
	if (!stillopen) {
		(void) VOP_CLOSE(mp->m_devvp, flag, u.u_cred);
		binval(mp->m_devvp);
		VN_RELE(mp->m_devvp);
		mp->m_devvp = (struct vnode *)0;
	}
	return (0);
}

/*
 * find root of ufs
 */
int
ufs_root(vfsp, vpp)
	struct vfs *vfsp;
	struct vnode **vpp;
{
	register struct mount *mp;
	register struct inode *ip;

	mp = (struct mount *)vfsp->vfs_data;
	ip = iget(mp->m_dev, mp->m_bufp->b_un.b_fs, (ino_t)ROOTINO);
	if (ip == (struct inode *)0) {
		return (u.u_error);
	}
	iunlock(ip);
	*vpp = ITOV(ip);
	return (0);
}

/*
 * Get file system statistics.
 */
int
ufs_statfs(vfsp, sbp)
register struct vfs *vfsp;
struct statfs *sbp;
{
	register struct fs *fsp;

	fsp = ((struct mount *)vfsp->vfs_data)->m_bufp->b_un.b_fs;
	if (fsp->fs_magic != FS_MAGIC)
		panic("ufs_statfs");
	sbp->f_bsize = fsp->fs_fsize;
	sbp->f_blocks = fsp->fs_dsize;
	sbp->f_bfree =
	    fsp->fs_cstotal.cs_nbfree * fsp->fs_frag +
		fsp->fs_cstotal.cs_nffree;
	/*
	 * avail = MAX(max_avail - used, 0)
	 */
	sbp->f_bavail =
	    (fsp->fs_dsize * (100 - fsp->fs_minfree) / 100) -
		 (fsp->fs_dsize - sbp->f_bfree);
	/*
	 * inodes
	 */
	sbp->f_files =  fsp->fs_ncg * fsp->fs_ipg;
	sbp->f_ffree = fsp->fs_cstotal.cs_nifree;
	bcopy((caddr_t)fsp->fs_id, (caddr_t)&sbp->f_fsid, sizeof (fsid_t));
	return (0);
}

/*
 * Flush any pending I/O.
 */
int
ufs_sync()
{
	update();
	return (0);
}

sbupdate(mp)
	struct mount *mp;
{
	register struct fs *fs = mp->m_bufp->b_un.b_fs;
	register struct buf *bp;
	int blks;
	caddr_t space;
	int i, size;

	bp = getblk(mp->m_devvp, SBLOCK, (int)fs->fs_sbsize);
	bcopy((caddr_t)fs, bp->b_un.b_addr, (u_int)fs->fs_sbsize);
	bwrite(bp);
	blks = howmany(fs->fs_cssize, fs->fs_fsize);
	space = (caddr_t)fs->fs_csp[0];
	for (i = 0; i < blks; i += fs->fs_frag) {
		size = fs->fs_bsize;
		if (i + fs->fs_frag > blks)
			size = (blks - i) * fs->fs_fsize;
		bp = getblk(mp->m_devvp, (daddr_t)fsbtodb(fs, fs->fs_csaddr+i),
		    size);
		bcopy(space, bp->b_un.b_addr, (u_int)size);
		space += size;
		bwrite(bp);
	}
}

/*
 * Common code for mount and umount.
 * Check that the user's argument is a reasonable
 * thing on which to mount, and return the device number if so.
 */
static int
getmdev(fspec, pdev)
	char *fspec;
	dev_t *pdev;
{
	register int error;
	struct vnode *vp;

	/*
	 * Get the device to be mounted
	 */
	error =
	    lookupname(fspec, UIOSEG_USER, FOLLOW_LINK,
		(struct vnode **)0, &vp);
	if (error)
		return (error);
	if (vp->v_type != VBLK) {
		VN_RELE(vp);
		return (ENOTBLK);
	}
	*pdev = vp->v_rdev;
	VN_RELE(vp);
	if (major(*pdev) >= nblkdev)
		return (ENXIO);
	return (0);
}

ufs_vget(vfsp, vpp, fidp)
	struct vfs *vfsp;
	struct vnode **vpp;
	struct fid *fidp;
{
	register struct ufid *ufid;
	register struct inode *ip;
	register struct mount *mp;

	mp = (struct mount *)vfsp->vfs_data;
	ufid = (struct ufid *)fidp;
	ip = iget(mp->m_dev, mp->m_bufp->b_un.b_fs, ufid->ufid_ino);
	if (ip == NULL) {
		*vpp = NULL;
		return (0);
	}
	if (ip->i_gen != ufid->ufid_gen) {
		idrop(ip);
		*vpp = NULL;
		return (0);
	}
	iunlock(ip);
	*vpp = ITOV(ip);
	return (0);
}
