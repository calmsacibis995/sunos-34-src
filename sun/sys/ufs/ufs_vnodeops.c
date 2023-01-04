/*	@(#)ufs_vnodeops.c 1.1 86/09/25 SMI	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/conf.h"
#include "../h/kernel.h"
#include "../h/cmap.h"
#include "../ufs/fs.h"
#include "../ufs/inode.h"
#include "../ufs/mount.h"
#include "../ufs/fsdir.h"
#ifdef QUOTA
#include "../ufs/quota.h"
#endif

#include "../specfs/fifo.h"	/* this defines PIPE_BUF for ufs_getattr() */
#include "../krpc/lockmgr.h"

#define ISVDEV(t) ((t == VCHR) || (t == VBLK) || (t == VFIFO))


extern int ufs_open();
extern int ufs_close();
extern int ufs_rdwr();
extern int ufs_ioctl();
extern int ufs_select();
extern int ufs_getattr();
extern int ufs_setattr();
extern int ufs_access();
extern int ufs_lookup();
extern int ufs_create();
extern int ufs_remove();
extern int ufs_link();
extern int ufs_rename();
extern int ufs_mkdir();
extern int ufs_rmdir();
extern int ufs_readdir();
extern int ufs_symlink();
extern int ufs_readlink();
extern int ufs_fsync();
extern int ufs_inactive();
extern int ufs_bmap();
extern int ufs_badop();
extern int ufs_bread();
extern int ufs_brelse();
extern int ufs_lockctl();
extern int ufs_fid();

struct vnodeops ufs_vnodeops = {
	ufs_open,
	ufs_close,
	ufs_rdwr,
	ufs_ioctl,
	ufs_select,
	ufs_getattr,
	ufs_setattr,
	ufs_access,
	ufs_lookup,
	ufs_create,
	ufs_remove,
	ufs_link,
	ufs_rename,
	ufs_mkdir,
	ufs_rmdir,
	ufs_readdir,
	ufs_symlink,
	ufs_readlink,
	ufs_fsync,
	ufs_inactive,
	ufs_bmap,
	ufs_badop,
	ufs_bread,
	ufs_brelse,
	ufs_lockctl,
	ufs_fid,
};

/*ARGSUSED*/
int
ufs_open(vpp, flag, cred)
	struct vnode **vpp;
	int flag;
	struct ucred *cred;
{
	return (0);
}

/*ARGSUSED*/
int
ufs_close(vp, flag, cred)
	struct vnode *vp;
	int flag;
	struct ucred *cred;
{
	return (0);
}

/*
 * read or write a vnode
 */
/*ARGSUSED*/
int
ufs_rdwr(vp, uiop, rw, ioflag, cred)
	struct vnode *vp;
	struct uio *uiop;
	enum uio_rw rw;
	int ioflag;
	struct ucred *cred;
{
	register struct inode *ip;
	int error;

	ip = VTOI(vp);
	if ((ip->i_mode&IFMT) == IFREG) {
		ILOCK(ip);
		if ((ioflag & IO_APPEND) && (rw == UIO_WRITE)) {
			/*
			 * in append mode start at end of file.
			 */
			uiop->uio_offset = ip->i_size;
		}
		error = rwip(ip, uiop, rw, ioflag);
		IUNLOCK(ip);
	} else {
		error = rwip(ip, uiop, rw, ioflag);
	}
	return (error);
}

int
rwip(ip, uio, rw, ioflag)
	register struct inode *ip;
	register struct uio *uio;
	enum uio_rw rw;
	int ioflag;
{
	struct vnode *devvp;
	struct buf *bp;
	struct fs *fs;
	daddr_t lbn, bn;
	register int n, on, type;
	int size;
	long bsize;
	extern int mem_no;
	int error = 0;

	if (rw != UIO_READ && rw != UIO_WRITE)
		panic("rwip");
	if (rw == UIO_READ && uio->uio_resid == 0)
		return (0);
	if ((uio->uio_offset < 0 || (uio->uio_offset + uio->uio_resid) < 0))
		return (EINVAL);
	if (rw == UIO_READ)
		imark(ip, IACC);
	if (uio->uio_resid == 0)
		return (0);
	type = ip->i_mode&IFMT;
	if (type == IFCHR || type == IFBLK || type == IFIFO) {
		panic("rwip dev inode");
	}
	if (rw == UIO_WRITE && type == IFREG &&
	    uio->uio_offset + uio->uio_resid >
	      u.u_rlimit[RLIMIT_FSIZE].rlim_cur) {
		psignal(u.u_procp, SIGXFSZ);
		return (EFBIG);
	}
	devvp = ip->i_devvp;
	fs = ip->i_fs;
	bsize = fs->fs_bsize;
	u.u_error = 0;
	do {
		lbn = uio->uio_offset / bsize;
		on = uio->uio_offset % bsize;
		n = MIN((unsigned)(bsize - on), uio->uio_resid);
		if (rw == UIO_READ) {
			int diff = ip->i_size - uio->uio_offset;
			if (diff <= 0)
				return (0);
			if (diff < n)
				n = diff;
		}
		bn =
		    fsbtodb(fs, bmap(ip, lbn,
			 rw == UIO_WRITE ? B_WRITE: B_READ,
			 (int)(on+n), ioflag & IO_SYNC));
		if (u.u_error || rw == UIO_WRITE && (long)bn<0)
			return (u.u_error);
		if (rw == UIO_WRITE &&
		   (uio->uio_offset + n > ip->i_size) &&
		   (type == IFDIR || type == IFREG || type == IFLNK))
			ip->i_size = uio->uio_offset + n;
		size = blksize(fs, ip, lbn);
		if (rw == UIO_READ) {
			if ((long)bn<0) {
				bp = geteblk(size);
				clrbuf(bp);
			} else if (ip->i_lastr + 1 == lbn)
				bp = breada(devvp, bn, size, rablock, rasize);
			else
				bp = bread(devvp, bn, size);
			ip->i_lastr = lbn;
		} else {
			int i, count;
			extern struct cmap *mfind();

			count = howmany(size, DEV_BSIZE);
			for (i = 0; i < count; i += CLBYTES/DEV_BSIZE)
				if (mfind(devvp, (daddr_t)(bn + i)))
					munhash(devvp, (daddr_t)(bn + i));
			if (n == bsize) 
				bp = getblk(devvp, bn, size);
			else
				bp = bread(devvp, bn, size);
		}
		n = MIN(n, bp->b_bcount - bp->b_resid);
		if (bp->b_flags & B_ERROR) {
			error = EIO;
			brelse(bp);
			goto bad;
		}
		u.u_error = uiomove(bp->b_un.b_addr+on, n, rw, uio);
		if (rw == UIO_READ) {
			if (n + on == bsize || uio->uio_offset == ip->i_size)
				bp->b_flags |= B_AGE;
			brelse(bp);
		} else {
			if ((ioflag & IO_SYNC) || (ip->i_mode&IFMT) == IFDIR)
				bwrite(bp);
			else if (n + on == bsize) {
				bp->b_flags |= B_AGE;
				bawrite(bp);
			} else
				bdwrite(bp);
			imark(ip, IUPD|ICHG);
			if (u.u_ruid != 0)
				ip->i_mode &= ~(ISUID|ISGID);
		}
	} while (u.u_error == 0 && uio->uio_resid > 0 && n != 0);
	if ((ioflag & IO_SYNC) && (rw == UIO_WRITE) &&
	    (ip->i_flag & (IUPD|ICHG))) {
		iupdat(ip, 1);
	}
	if (error == 0)				/* XXX */
		error = u.u_error;		/* XXX */
bad:
	return (error);
}

/*ARGSUSED*/
int
ufs_ioctl(vp, com, data, flag, cred)
	struct vnode *vp;
	int com;
	caddr_t data;
	int flag;
	struct ucred *cred;
{
	return (EINVAL);
}

/*ARGSUSED*/
int
ufs_select(vp, which, cred)
	struct vnode *vp;
	int which;
	struct ucred *cred;
{
	return (EINVAL);
}

/*ARGSUSED*/
int
ufs_getattr(vp, vap, cred)
	struct vnode *vp;
	register struct vattr *vap;
	struct ucred *cred;
{
	register struct inode *ip;

	ip = VTOI(vp);
	/*
	 * Copy from inode table.
	 */
	vap->va_type = IFTOVT(ip->i_mode);
	vap->va_mode = ip->i_mode;
	vap->va_uid = ip->i_uid;
	vap->va_gid = ip->i_gid;
	vap->va_fsid = ip->i_dev;
	vap->va_nodeid = ip->i_number;
	vap->va_nlink = ip->i_nlink;
	vap->va_size = ip->i_size;
	vap->va_atime = ip->i_atime;
	vap->va_mtime = ip->i_mtime;
	vap->va_ctime = ip->i_ctime;
	vap->va_rdev = ip->i_rdev;
	vap->va_blocks = ip->i_blocks;
	switch(ip->i_mode & IFMT) {

	case IFBLK:
		vap->va_blocksize = BLKDEV_IOSIZE;
		break;

	case IFCHR:
		vap->va_blocksize = MAXBSIZE;
		break;

	default:
		vap->va_blocksize = vp->v_vfsp->vfs_bsize;
		break;
	}
	return (0);
}

int
ufs_setattr(vp, vap, cred)
	register struct vnode *vp;
	register struct vattr *vap;
	struct ucred *cred;
{
	register struct inode *ip;
	int chtime = 0;
	int error = 0;

	/*
	 * cannot set these attributes
	 */
	if ((vap->va_nlink != -1) || (vap->va_blocksize != -1) ||
	    (vap->va_rdev != -1) || (vap->va_blocks != -1) ||
	    (vap->va_fsid != -1) || (vap->va_nodeid != -1) ||
	    ((int)vap->va_type != -1)) {
		return (EINVAL);
	}

	ip = VTOI(vp);
	ilock(ip);
	/*
	 * Change file access modes. Must be owner or su.
	 */
	if (vap->va_mode != (u_short)-1) {
		error = OWNER(cred, ip);
		if (error)
			goto out;
		ip->i_mode &= IFMT;
		ip->i_mode |= vap->va_mode & ~IFMT;
		if (cred->cr_uid != 0) {
			ip->i_mode &= ~ISVTX;
			if (!groupmember(ip->i_gid))
				ip->i_mode &= ~ISGID;
		}
		imark(ip, ICHG);
		if ((vp->v_flag & VTEXT) && ((ip->i_mode & ISVTX) == 0)) {
			xrele(ITOV(ip));
		}
	}
	/*
	 * To change file ownership, must be su.
	 * To change group ownership, must be su or owner and in target group.
	 * This is now enforced in chown1() below.
	 */
	if ((vap->va_uid != -1) || (vap->va_gid != -1)) {
		error = chown1(ip, vap->va_uid, vap->va_gid);
		if (error)
			goto out;
	}
	/*
	 * Truncate file. Must have write permission and not be a directory.
	 */
	if (vap->va_size != (u_long)-1) {
		if ((ip->i_mode & IFMT) == IFDIR) {
			error = EISDIR;
			goto out;
		}
		if (iaccess(ip, IWRITE)) {
			error = u.u_error;
			goto out;
		}
		itrunc(ip, vap->va_size);
	}
	/*
	 * Change file access or modified times.
	 */
	if (vap->va_atime.tv_sec != -1) {
		error = OWNER(cred, ip);
		if (error)
			goto out;
		ip->i_atime = vap->va_atime;
		chtime++;
	}
	if (vap->va_mtime.tv_sec != -1) {
		error = OWNER(cred, ip);
		if (error)
			goto out;
		ip->i_mtime = vap->va_mtime;
		chtime++;
	}
	if (chtime) {
		ip->i_flag |= IACC|IUPD|ICHG;
		ip->i_ctime = time;
	}
out:
	iupdat(ip, 1);			/* XXX should be asyn for perf */
	iunlock(ip);
	return (error);
}

/*
 * Perform chown operation on inode ip;
 * inode must be locked prior to call.
 */
chown1(ip, uid, gid)
	register struct inode *ip;
	register int uid;
	int gid;
{
#ifdef QUOTA
	register long change;
#endif

	if (uid == -1)
		uid = ip->i_uid;
	if (gid == -1)
		gid = ip->i_gid;

	if ((uid == ip->i_uid) && (gid == ip->i_gid))
		return(0);      /* no change at all */

	/* error if not super-user and:
	 *	1) trying to change owner
	 *	2) not current owner
	 *	3) new group is not a member of process group set
	 */
	if ( (u.u_uid != 0) &&
	    ((uid != ip->i_uid) || (u.u_uid != uid) || (!groupmember(gid))) ) {
		return(EPERM);
	}

#ifdef QUOTA
	if (ip->i_uid == uid)		/* this just speeds things a little */
		change = 0;
	else
		change = ip->i_blocks;
	(void) chkdq(ip, -change, 1);
	(void) chkiq(VFSTOM(ip->i_vnode.v_vfsp), ip, ip->i_uid, 1);
	dqrele(ip->i_dquot);
#endif
	ip->i_uid = uid;
	ip->i_gid = gid;
	imark(ip, ICHG);
	if (u.u_uid != 0)
		ip->i_mode &= ~(ISUID|ISGID);
#ifdef QUOTA
	ip->i_dquot = getinoquota(ip);
	(void) chkdq(ip, change, 1);
	(void) chkiq(VFSTOM(ip->i_vnode.v_vfsp), (struct inode *)NULL, uid, 1);
	return (u.u_error);		/* should == 0 ALWAYS !! */
#else
	return (0);
#endif
}

/*ARGSUSED*/
int
ufs_access(vp, mode, cred)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
{
	register struct inode *ip;
	int error;

	ip = VTOI(vp);
	ilock(ip);
	error = iaccess(ip, mode);
	iunlock(ip);
	return (error);
}

/*ARGSUSED*/
int
ufs_readlink(vp, uiop, cred)
	struct vnode *vp;
	struct uio *uiop;
	struct ucred *cred;
{
	register struct inode *ip;
	register int error;

	if (vp->v_type != VLNK)
		return (EINVAL);
	ip = VTOI(vp);
	ilock(ip);
	error = rwip(ip, uiop, UIO_READ, 0);
	iunlock(ip);
	return (error);
}

/*ARGSUSED*/
int
ufs_fsync(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
	register struct inode *ip;

	ip = VTOI(vp);
	ilock(ip);
	syncip(ip);
	iunlock(ip);
	return (0);
}

/*ARGSUSED*/
int
ufs_inactive(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{

	iinactive(VTOI(vp));
	return (0);
}

/*
 * Unix file system operations having to do with directory manipulation.
 */
/*ARGSUSED*/
ufs_lookup(dvp, nm, vpp, cred)
	struct vnode *dvp;
	char *nm;
	struct vnode **vpp;
	struct ucred *cred;
{
	struct inode *ip;
	register int error;

	error = dirlook(VTOI(dvp), nm, &ip);
	if (error == 0) {
		*vpp = ITOV(ip);
		iunlock(ip);
		/*
		 * If vnode is a device return special vnode instead
		 */
		if (ISVDEV((*vpp)->v_type)) {
			struct vnode *newvp;

			newvp = specvp(*vpp, (*vpp)->v_rdev);
			VN_RELE(*vpp);
			*vpp = newvp;
		}
	}
	return (error);
}

ufs_create(dvp, nm, vap, exclusive, mode, vpp, cred)
	struct vnode *dvp;
	char *nm;
	struct vattr *vap;
	enum vcexcl exclusive;
	int mode;
	struct vnode **vpp;
	struct ucred *cred;
{
	register int error;
	struct inode *ip;

	/*
	 * can't create directories. use ufs_mkdir.
	 */
	if (vap->va_type == VDIR)
		return (EISDIR);
	ip = (struct inode *) 0;
	error = direnter(VTOI(dvp), nm, DE_CREATE,
		(struct inode *)0, (struct inode *)0, vap, &ip);
	/*
	 * if file exists and this is a nonexclusive create,
	 * check for not directory and access permissions
	 * If create/read-only an existing directory, allow it.
	 */
	if (error == EEXIST) {
		if (exclusive == NONEXCL) {
			if (((ip->i_mode & IFMT) == IFDIR) && (mode & IWRITE)) {
				error = EISDIR;
			} else if (mode) {
				error = iaccess(ip, mode);
			} else {
				error = 0;
			}
		}
		if (error) {
			iput(ip);
		} else if (((ip->i_mode&IFMT) == IFREG) && (vap->va_size == 0)){
			/*
			 * truncate regular files, if required
			 */
			itrunc(ip, (u_long) 0);
		}
	} 
	if (error) {
		return (error);
	}
	*vpp = ITOV(ip);
	iunlock(ip);
	/*
	 * If vnode is a device return special vnode instead
	 */
	if (ISVDEV((*vpp)->v_type)) {
		struct vnode *newvp;

		newvp = specvp(*vpp, (*vpp)->v_rdev);
		VN_RELE(*vpp);
		*vpp = newvp;
	}

	if (vap != (struct vattr *)0) {
		(void) VOP_GETATTR(*vpp, vap, cred);
	}
	return (error);
}

/*ARGSUSED*/
ufs_remove(vp, nm, cred)
	struct vnode *vp;
	char *nm;
	struct ucred *cred;
{
	register int error;

	error = dirremove(VTOI(vp), nm, (struct inode *)0, 0);
	return (error);
}

/*
 * link a file or a directory
 * If source is a directory, must be superuser
 */
/*ARGSUSED*/
ufs_link(vp, tdvp, tnm, cred)
	struct vnode *vp;
	struct vnode *tdvp;
	char *tnm;
	struct ucred *cred;
{
	register struct inode *sip;
	register int error;

	sip = VTOI(vp);
	if (((sip->i_mode & IFMT) == IFDIR) && !suser()) {
		return (EPERM);
	}
	error =
	    direnter(VTOI(tdvp), tnm, DE_LINK,
		(struct inode *)0, sip, (struct vattr *)0, (struct inode **)0);
	return (error);
}

/*
 * Rename a file or directory
 * We are given the vnode and entry string of the source and the
 * vnode and entry string of the place we want to move the source to
 * (the target). The essential operation is:
 *	unlink(target);
 *	link(source, target);
 *	unlink(source);
 * but "atomically". Can't do full commit without saving state in the inode
 * on disk, which isn't feasible at this time. Best we can do is always
 * guarantee that the TARGET exists.
 */
/*ARGSUSED*/
ufs_rename(sdvp, snm, tdvp, tnm, cred)
	struct vnode *sdvp;		/* old (source) parent vnode */
	char *snm;			/* old (source) entry name */
	struct vnode *tdvp;		/* new (target) parent vnode */
	char *tnm;			/* new (target) entry name */
	struct ucred *cred;
{
	struct inode *sip;		/* source inode */
	register struct inode *sdp;	/* old (source) parent inode */
	register struct inode *tdp;	/* new (target) parent inode */
	register int error;

	sdp = VTOI(sdvp);
	tdp = VTOI(tdvp);
	/*
	 * make sure we can delete the source entry
	 */
	error = iaccess(sdp, IWRITE);
	if (error) {
		return (error);
	}
	/*
	 * look up inode of file we're supposed to rename.
	 */
	error = dirlook(sdp, snm, &sip);
	if (error) {
		return (error);
	}

	iunlock(sip);			/* unlock inode (it's held) */
	/*
	 * check for renaming '.' or '..' or alias of '.'
	 */
	if ((strcmp(snm, ".") == 0) || (strcmp(snm, "..") == 0) ||
	    (sdp == sip)) {
		error = EINVAL;
		goto out;
	}
	/*
	 * link source to the target
	 */
	error =
	    direnter(tdp, tnm, DE_RENAME,
		sdp, sip, (struct vattr *)0, (struct inode **)0);
	if (error)
		goto out;

	/*
	 * Unlink the source
	 * Remove the source entry. Dirremove checks that the entry
	 * still reflects sip, and returns an error if it doesn't.
	 * If the entry has changed just forget about it. 
	 * Release the source inode.
	 */
	error = dirremove(sdp, snm, sip, 0);
	if (error == ENOENT) {
		error = 0;
	} else if (error) {
		goto out;
	}

out:
	irele(sip);
	return (error);
}

/*ARGSUSED*/
ufs_mkdir(dvp, nm, vap, vpp, cred)
	struct vnode *dvp;
	char *nm;
	register struct vattr *vap;
	struct vnode **vpp;
	struct ucred *cred;
{
	struct inode *ip;
	register int error;

	error =
	    direnter(VTOI(dvp), nm, DE_CREATE,
		(struct inode *)0, (struct inode *)0, vap, &ip);
	if (error == 0) {
		*vpp = ITOV(ip);
		iunlock(ip);
	} else if (error == EEXIST) {
		iput(ip);
	}
	return (error);
}

/*ARGSUSED*/
ufs_rmdir(vp, nm, cred)
	struct vnode *vp;
	char *nm;
	struct ucred *cred;
{
	register int error;

	error = dirremove(VTOI(vp), nm, (struct inode *)0, 1);
	return (error);
}

/*ARGSUSED*/
ufs_readdir(vp, uiop, cred)
	struct vnode *vp;
	register struct uio *uiop;
	struct ucred *cred;
{
	register struct iovec *iovp;
	register unsigned count;

	iovp = uiop->uio_iov;
	count = iovp->iov_len;
	if ((uiop->uio_iovcnt != 1) || (count < DIRBLKSIZ) ||
	    (uiop->uio_offset & (DIRBLKSIZ -1)))
		return (EINVAL);
	count &= ~(DIRBLKSIZ - 1);
	uiop->uio_resid -= iovp->iov_len - count;
	iovp->iov_len = count;
	return (rwip(VTOI(vp), uiop, UIO_READ, 0));
}

/*ARGSUSED*/
ufs_symlink(dvp, lnm, vap, tnm, cred)
	struct vnode *dvp;
	char *lnm;
	struct vattr *vap;
	char *tnm;
	struct ucred *cred;
{
	struct inode *ip;
	register int error;

	ip = (struct inode *) 0;
	vap->va_type = VLNK;
	vap->va_rdev = 0;
	error =
	    direnter(VTOI(dvp), lnm, DE_CREATE,
		(struct inode *)0, (struct inode *)0, vap, &ip);
	if (error == 0) {
		error =
		    rdwri(UIO_WRITE, ip,
			tnm, strlen(tnm), 0, UIOSEG_KERNEL, (int *)0);
		iput(ip);
	} else if (error == EEXIST) {
		iput(ip);
	}
	return (error);
}

rdwri(rw, ip, base, len, offset, seg, aresid)
	enum uio_rw rw;
	struct inode *ip;
	caddr_t base;
	int len;
	int offset;
	int seg;
	int *aresid;
{
	struct uio auio;
	struct iovec aiov;
	register int error;

	aiov.iov_base = base;
	aiov.iov_len = len;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = offset;
	auio.uio_seg = seg;
	auio.uio_resid = len;
	error = ufs_rdwr(ITOV(ip), &auio, rw, 0, u.u_cred);
	if (aresid) {
		*aresid = auio.uio_resid;
	} else if (auio.uio_resid) {
		error = EIO;
	}
	return (error);
}

int
ufs_bmap(vp, lbn, vpp, bnp)
	struct vnode *vp;
	daddr_t lbn;
	struct vnode **vpp;
	daddr_t *bnp;
{
	register struct inode *ip;

	ip = VTOI(vp);
	if (vpp)
		*vpp = ip->i_devvp;
	if (bnp)
		*bnp = fsbtodb(ip->i_fs, bmap(ip, lbn, B_READ));
	return (0);
}

/*
 * read a logical block and return it in a buffer
 */
/*ARGSUSED*/
int
ufs_bread(vp, lbn, bpp, sizep)
	struct vnode *vp;
	daddr_t lbn;
	struct buf **bpp;
	long *sizep;
{
	register struct inode *ip;
	register struct buf *bp;
	register daddr_t bn;
	register int size;

	ip = VTOI(vp);
	size = blksize(ip->i_fs, ip, lbn);
	bn = fsbtodb(ip->i_fs, bmap(ip, lbn, B_READ));
	if ((long)bn < 0) {
		bp = geteblk(size);
		clrbuf(bp);
	} else if (ip->i_lastr + 1 == lbn) {
		bp = breada(ip->i_devvp, bn, size, rablock, rasize);
	} else {
		bp = bread(ip->i_devvp, bn, size);
	}
	ip->i_lastr = lbn;
	imark(ip, IACC);
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
		return (EIO);
	} else {
		*bpp = bp;
		return (0);
	}
}

/*
 * release a block returned by ufs_bread
 */
/*ARGSUSED*/
ufs_brelse(vp, bp)
	struct vnode *vp;
	struct buf *bp;
{
	bp->b_flags |= B_AGE;
	bp->b_resid = 0;
	brelse(bp);
}

int
ufs_badop()
{
	panic("ufs_badop");
}

/*
 * Record-locking requests are passed to the local Lock-Manager daemon.
 */
int
ufs_lockctl(vp, ld, cmd, cred)
	struct vnode *vp;
	struct flock *ld;
	int cmd;
	struct ucred *cred;
{
	lockhandle_t lh;
	struct fid *fidp;

	/* Convert vnode into lockhandle-id. This is awfully like makefh(). */
	if (VOP_FID(vp, &fidp) || fidp == NULL) {
		return (EINVAL);
	}
	bzero((caddr_t)&lh.lh_id, sizeof (lh.lh_id));	/* clear extra bytes */
	lh.lh_fsid.val[0] = vp->v_vfsp->vfs_fsid.val[0];
	lh.lh_fsid.val[1] = vp->v_vfsp->vfs_fsid.val[1];
	lh.lh_fid.fid_len = fidp->fid_len;
	bcopy(fidp->fid_data, lh.lh_fid.fid_data, fidp->fid_len);
	freefid(fidp);

	/* Add in vnode and server and call to common code */
	lh.lh_vp = vp;
	lh.lh_servername = hostname;
	return (klm_lockctl(&lh, ld, cmd, cred));
}

ufs_fid(vp, fidpp)
	struct vnode *vp;
	struct fid **fidpp;
{
	register struct ufid *ufid;

	ufid = (struct ufid *)kmem_alloc(sizeof(struct ufid));
	ufid->ufid_len = sizeof(struct ufid) - sizeof(u_short);
	ufid->ufid_ino = VTOI(vp)->i_number;
	ufid->ufid_gen = VTOI(vp)->i_gen;
	*fidpp = (struct fid *)ufid;
	return (0);
}
