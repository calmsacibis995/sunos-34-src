/*	@(#)vfs.c 1.1 86/09/25 SMI	*/

#include "../h/param.h"
#include "../h/user.h"
#include "../h/uio.h"
#include "../h/file.h"
#include "../h/vfs.h"
#include "../h/socket.h"
#include "../h/systm.h"
#undef NFS
#include "../h/mount.h"
#include "../h/pathname.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"

/*
 * vfs global data
 */
struct vnode *rootdir;			/* pointer to root vnode */

struct vfs *rootvfs;			/* pointer to root vfs. This is */
					/* also the head of the vfs list */
/*
 * Convert mode formats to vnode types
 */
enum vtype mftovt_tab[] = {
	VFIFO, VCHR, VDIR, VBLK, VREG, VLNK, VSOCK, VBAD
};

/*
 * System calls
 */

/*
 * mount system call
 */
mount(uap)
	register struct a {
		int	type;
		char	*dir;
		int	flags;
		caddr_t	data;
	} *uap;
{
	struct pathname pn;
	struct vnode *vp;
	struct vfs *vfsp;
	extern struct vnodeops ufs_vnodeops;

	/*
	 * Must be super user
	 */
	if (!suser())
		return;
	/*
	 * Get vnode to be covered
	 */
	u.u_error = lookupname(uap->dir, UIOSEG_USER, FOLLOW_LINK,
	    (struct vnode **)0, &vp);
	if (u.u_error)
		return;
	dnlc_purge();
	if (vp->v_count != 1) {
		VN_RELE(vp);
		u.u_error = EBUSY;
		return;
	}
	if (vp->v_type != VDIR) {
		VN_RELE(vp);
		u.u_error = ENOTDIR;
		return;
	}
	u.u_error = pn_get(uap->dir, UIOSEG_USER, &pn);
	if (u.u_error) {
		VN_RELE(vp);
		return;
	}
	if (uap->type > MOUNT_MAXTYPE ||
	    vfssw[uap->type] == (struct vfsops *)0) {
		u.u_error = ENODEV;
		VN_RELE(vp);
		pn_free(&pn);
		return;
	}
	vfsp = (struct vfs *)kmem_alloc(sizeof (struct vfs));
	VFS_INIT(vfsp, vfssw[uap->type], (caddr_t)0);

	/*
	 * Mount the filesystem.
	 * Lock covered vnode (XXX this currently only works if it is type ufs)
	 */
	if (vp->v_op == &ufs_vnodeops)
		ilock(VTOI(vp));
	u.u_error = vfs_add(vp, vfsp, uap->flags);
	if (!u.u_error) {
		u.u_error = VFS_MOUNT(vfsp, pn.pn_path, uap->data);
	}
	if (vp->v_op == &ufs_vnodeops)
		iunlock(VTOI(vp));
	pn_free(&pn);
	if (!u.u_error) {
		vfs_unlock(vfsp);
	} else {
		vfs_remove(vfsp);
		kmem_free((caddr_t)vfsp, sizeof (struct vfs));
		VN_RELE(vp);
	}
	return;
}


/*
 * Sync system call. sync each vfs
 */
sync()
{
	register struct vfs *vfsp;

	for (vfsp = rootvfs; vfsp != (struct vfs *)0; vfsp = vfsp->vfs_next) {
		VFS_SYNC(vfsp);
	}
}

/*
 * get filesystem statistics
 */
statfs(uap)
	struct a {
		char *path;
		struct statfs *buf;
	} *uap;
{
	struct vnode *vp;

	u.u_error = lookupname(uap->path, UIOSEG_USER, FOLLOW_LINK,
	    (struct vnode **)0, &vp);
	if (u.u_error)
		return;
	cstatfs(vp->v_vfsp, uap->buf);
	VN_RELE(vp);
}

fstatfs(uap)
	struct a {
		int fd;
		struct statfs *buf;
	} *uap;
{
	struct file *fp;

	u.u_error = getvnodefp(uap->fd, &fp);
	if (u.u_error == 0)
		cstatfs(((struct vnode *)fp->f_data)->v_vfsp, uap->buf);
}

cstatfs(vfsp, ubuf)
	struct vfs *vfsp;
	struct statfs *ubuf;
{
	struct statfs sb;

	u.u_error = VFS_STATFS(vfsp, &sb);
	if (u.u_error)
		return;
	u.u_error = copyout((caddr_t)&sb, (caddr_t)ubuf, sizeof(sb));
}

/*
 * Unmount system call.
 *
 * Note: unmount takes a path to the vnode mounted on as argument,
 * not special file (as before).
 */
unmount(uap)
	struct a {
		char	*pathp;
	} *uap;
{
	struct vnode *fsrootvp;
	register struct vfs *vfsp;

	if (!suser())
		return;
	/*
	 * lookup root of fs
	 */
	u.u_error = lookupname(uap->pathp, UIOSEG_USER, FOLLOW_LINK,
	    (struct vnode **)0, &fsrootvp);
	if (u.u_error)
		return;
	/*
	 * make sure this is a root
	 */
	if ((fsrootvp->v_flag & VROOT) == 0) {
		u.u_error = EINVAL;
		VN_RELE(fsrootvp);
		return;
	}
	/*
	 * get vfs
	 */
	vfsp = fsrootvp->v_vfsp;
	VN_RELE(fsrootvp);
	/*
	 * Do the unmount.
	 */
	dounmount(vfsp);
}

/*
 * XXX Subroutine so the old 4.2/S5-style "umount" call can use this code
 * as well.
 */
dounmount(vfsp)
	register struct vfs *vfsp;
{
	register struct vnode *coveredvp;

	/*
	 * Get covered vnode.
	 */
	coveredvp = vfsp->vfs_vnodecovered;
	/*
	 * lock vnode to maintain fs status quo during unmount
	 */
	u.u_error = vfs_lock(vfsp);
	if (u.u_error)
		return;

	xumount(vfsp);	/* remove unused sticky files from text table */
	dnlc_purge();	/* remove dnlc entries for this file sys */
	VFS_SYNC(vfsp);

	u.u_error = VFS_UNMOUNT(vfsp);
	if (u.u_error) {
		vfs_unlock(vfsp);
	} else {
		VN_RELE(coveredvp);
		vfs_remove(vfsp);
		kmem_free((caddr_t)vfsp, (u_int)sizeof(*vfsp));
	}
}

/*
 * External routines
 */

/*
 * vfs_mountroot is called by main (init_main.c) to
 * mount the root filesystem.
 */
void
vfs_mountroot()
{
	register int error;
	extern int (*rootfsmount)();	/* pointer to root mounting routine */
					/* set by (auto)configuration */

	/*
	 * Rootfsmount is a pointer to the routine which will mount a specific
	 * filesystem as the root. It is setup by autoconfiguration.
	 * If error panic.
	 */
	error = (*rootfsmount)();
	if (error) {
		panic("rootmount cannot mount root");
	}
	/*
	 * Get vnode for '/'.
	 * Setup rootdir, u.u_rdir and u.u_cdir to point to it.
	 * These are used by lookuppn so that it knows where
	 * to start from '/' or '.'.
	 */
	error = VFS_ROOT(rootvfs, &rootdir);
	if (error)
		panic("rootmount: cannot find root vnode");
	u.u_cdir = rootdir;
	VN_HOLD(u.u_cdir);
	u.u_rdir = NULL;
}

/*
 * vfs_add is called by a specific filesystem's mount routine to add
 * the new vfs into the vfs list and to cover the mounted on vnode.
 * The vfs is also locked so that lookuppn will not venture into the
 * covered vnodes subtree.
 * coveredvp is zero if this is the root.
 */
int
vfs_add(coveredvp, vfsp, mflag)
	register struct vnode *coveredvp;
	register struct vfs *vfsp;
	int mflag;
{
	register int error;

	error = vfs_lock(vfsp);
	if (error)
		return (error);
	if (coveredvp != (struct vnode *)0) {
		/*
		 * Return EBUSY if the covered vp is already mounted on.
		 */
		if (coveredvp->v_vfsmountedhere != (struct vfs *)0) {
			vfs_unlock(vfsp);
			return(EBUSY);
		}
		/*
		 * Put the new vfs on the vfs list after root.
		 * Point the covered vnode at the new vfs so lookuppn
		 * (vfs_lookup.c) can work its way into the new file system.
		 */
		vfsp->vfs_next = rootvfs->vfs_next;
		rootvfs->vfs_next = vfsp;
		coveredvp->v_vfsmountedhere = vfsp;
	} else {
		/*
		 * This is the root of the whole world.
		 */
		rootvfs = vfsp;
		vfsp->vfs_next = (struct vfs *)0;
	}
	vfsp->vfs_vnodecovered = coveredvp;
	if (mflag & M_RDONLY) {
		vfsp->vfs_flag |= VFS_RDONLY;
	} else {
		vfsp->vfs_flag &= ~VFS_RDONLY;
	}
	if (mflag & M_NOSUID) {
		vfsp->vfs_flag |= VFS_NOSUID;
	} else {
		vfsp->vfs_flag &= ~VFS_NOSUID;
	}
	return (0);
}

/*
 * Remove a vfs from the vfs list, and destory pointers to it.
 * Should be called by filesystem implementation after it determines
 * that an unmount is legal but before it destroys the vfs.
 */
void
vfs_remove(vfsp)
register struct vfs *vfsp;
{
	register struct vfs *tvfsp;
	register struct vnode *vp;

	/*
	 * can't unmount root. Should never happen, because fs will be busy.
	 */
	if (vfsp == rootvfs)
		panic("vfs_remove: unmounting root");
	for (tvfsp = rootvfs;
	    tvfsp != (struct vfs *)0; tvfsp = tvfsp->vfs_next) {
		if (tvfsp->vfs_next == vfsp) {
			/*
			 * remove vfs from list, unmount covered vp.
			 */
			tvfsp->vfs_next = vfsp->vfs_next;
			vp = vfsp->vfs_vnodecovered;
			vp->v_vfsmountedhere = (struct vfs *)0;
			/*
			 * release lock and wakeup anybody waiting
			 */
			vfs_unlock(vfsp);
			return;
		}
	}
	/*
	 * can't find vfs to remove
	 */
	panic("vfs_remove: vfs not found");
}

/*
 * Lock a filesystem to prevent access to it while mounting and unmounting.
 * Returns error if already locked.
 * XXX This totally inadequate for unmount right now - srk
 */
int
vfs_lock(vfsp)
	register struct vfs *vfsp;
{

	if (vfsp->vfs_flag & VFS_MLOCK)
		return (EBUSY);
	vfsp->vfs_flag |= VFS_MLOCK;
	return (0);
}

/*
 * Unlock a locked filesystem.
 * Panics if not locked
 */
void
vfs_unlock(vfsp)
	register struct vfs *vfsp;
{

	if ((vfsp->vfs_flag & VFS_MLOCK) == 0)
		panic("vfs_unlock");
	vfsp->vfs_flag &= ~VFS_MLOCK;
	/*
	 * Wake anybody waiting for the lock to clear
	 */
	if (vfsp->vfs_flag & VFS_MWAIT) {
		vfsp->vfs_flag &= ~VFS_MWAIT;
		wakeup((caddr_t)vfsp);
	}
}

struct vfs *
getvfs(fsid)
	fsid_t *fsid;
{
	register struct vfs *vfsp;

	for (vfsp = rootvfs; vfsp; vfsp = vfsp->vfs_next) {
		if (vfsp->vfs_fsid.val[0] == fsid->val[0] &&
		    vfsp->vfs_fsid.val[1] == fsid->val[1]) {
			break;
		}
	}
	return (vfsp);
}

/*
 * Take a file system ID of the sort that appears in a "struct vattr"
 * and find the VFS that has that file system ID.  This is done by
 * finding the root directory for each VFS, finding its attributes,
 * and comparing its file system ID with the given file system ID;
 * if we have a match, we've found the right VFS.  It's slow, but it
 * works, and it's only used for the System V "ustat" call which is
 * a crock anyway ("statfs" is better).
 */
int
vafsidtovfs(vafsid, vfspp)
	long vafsid;
	struct vfs **vfspp;
{
	register struct vfs *vfsp;
	struct vnode *rootvn;		/* pointer to root vnode of vfs */
	struct vattr vattr;
	register int error;

	for (vfsp = rootvfs; vfsp != (struct vfs *)0; vfsp = vfsp->vfs_next) {
		error = VFS_ROOT(vfsp, &rootvn);
		if (error)
			return(error);
		error = VOP_GETATTR(rootvn, &vattr, u.u_cred);
		if (error)
			return(error);
		if (vafsid == vattr.va_fsid) {
			*vfspp = vfsp;
			return(0);
		}
	}
	return(EINVAL);			/* not found */
}
