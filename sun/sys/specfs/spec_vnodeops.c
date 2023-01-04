#ifndef lint
static  char sccsid[] = "@(#)spec_vnodeops.c 1.3 86/12/15 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */


#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/kernel.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/uio.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../h/cmap.h"
#include "../specfs/snode.h"

#include "../krpc/lockmgr.h"

int spec_open();
int spec_close();
int spec_rdwr();
int spec_ioctl();
int spec_select();
int spec_getattr();
int spec_setattr();
int spec_access();
int spec_link();
int spec_inactive();
int spec_badop();
int spec_noop();
int spec_lockctl();
int spec_fsync();
int spec_fid();

struct vnodeops spec_vnodeops = {
	spec_open,
	spec_close,
	spec_rdwr,
	spec_ioctl,
	spec_select,
	spec_getattr,
	spec_setattr,
	spec_access,
	spec_noop,	/* lookup */
	spec_noop,	/* create */
	spec_noop,	/* remove */
	spec_link,
	spec_noop,	/* rename */
	spec_noop,	/* mkdir */
	spec_noop,	/* rmdir */
	spec_noop,	/* readdir */
	spec_noop,	/* symlink */
	spec_noop,	/* readlink */
	spec_fsync,
	spec_inactive,
	spec_badop,	/* bmap */
	spec_badop,	/* badop */
	spec_badop,	/* bread */
	spec_badop,	/* brelse */
	spec_lockctl,
	spec_fid,
};

/*
 * open a special file (device)
 * some weird stuff here having to do with clone and indirect devices:
 * When a file open operation happens (e.g. ufs_open) and the vnode has
 * type VDEV the open routine makes a spec vnode and calls us. When we
 * do the device open routine there are two possible strange results:
 * 1) an indirect device will return an error on open and return a new
 *    dev number. we have to make that into a spec vnode and call open
 *    on it again.
 * 2) a clone device will return a new dev number on open but no error.
 *    in this case we just make a new spec vnode out of the new dev number
 *    and return that.
 */
/*ARGSUSED*/
int
spec_open(vpp, flag, cred)
	struct vnode **vpp;
	int flag;
	struct ucred *cred;
{
	register struct snode *sp;
	dev_t dev;
	dev_t newdev;
	dev_t olddev;
	int error;

	/*
	 * Setjmp in case open is interrupted.
	 * If it is, close and return error.
	 */
	if (setjmp(&u.u_qsave)) {
		error = EINTR;
		(void) spec_close(*vpp, flag & FMASK, cred);
		return (error);
	}
	sp = VTOS(*vpp);

	/*
	 * Do open protocol for special type.
	 */
	olddev = dev = sp->s_dev;

	switch ((*vpp)->v_type) {

	case VCHR:

		newdev = dev;
		error = 0;
		do {
			dev = newdev;
			if ((u_int)major(dev) >= nchrdev)
				return (ENXIO);

			error = (*cdevsw[major(dev)].d_open)(dev,flag, &newdev);

			/*
			 * If this is an indirect device we need to do the
			 * open again.
			 */
		} while (newdev != dev && error == EAGAIN);

		if (olddev != newdev && error == 0) {
			register struct vnode *nvp;

			/*
			 * Allocate new snode with new minor device. Release
			 * old snode. Set vpp to point to new one.  This snode
			 * will go away when the last reference to it goes away.
			 * Warning: if you stat this, and try to match it with
			 * a name in the filesystem you will fail, unless you
			 * had previously put names in that match.
			 */
			nvp = specvp(*vpp, newdev);
			VN_RELE(*vpp);
			*vpp = nvp;
		}
		break;

	case VBLK:
		if ((u_int)major(dev) >= nblkdev)
			return (ENXIO);
		error = (*bdevsw[major(dev)].d_open)(dev, flag);
		break;

	case VFIFO:
		printf("spec_open: got a VFIFO???\n");

	case VSOCK:
		error = EOPNOTSUPP;
		break;
	
	default:
		error = 0;
		break;
	}
	return (error);
}

/*ARGSUSED*/
int
spec_close(vp, flag, cred)
	struct vnode *vp;
	int flag;
	struct ucred *cred;
{
	register struct snode *sp;
	int (*cfunc)();
	dev_t dev;

	/*
	 * setjmp in case close is interrupted
	 */
	if (setjmp(&u.u_qsave)) {
		return (EINTR);
	}
	sp = VTOS(vp);
	dev = sp->s_dev;
	switch(vp->v_type) {

	case VCHR:
		cfunc = cdevsw[major(dev)].d_close;
		break;

	case VBLK:
		cfunc = bdevsw[major(dev)].d_close;
		break;

	case VFIFO:
		printf("spec_close: got a VFIFO???\n");

	default:
		return (0);
	}
	if (vp->v_type == VBLK) {
		/*
		 * On last close of a block device (that isn't mounted)
		 * we must invalidate any in core blocks, so that
		 * we can, for instance, change floppy disks.
		 */
		bflush(vp);
		binval(vp);
	}
	/*
	 * Close the device.
	 */
	(*cfunc)(dev, flag);
	return (0);
}

/*
 * read or write a vnode
 */
/*ARGSUSED*/
int
spec_rdwr(vp, uiop, rw, ioflag, cred)
	struct vnode *vp;
	struct uio *uiop;
	enum uio_rw rw;
	int ioflag;
	struct ucred *cred;
{
	register struct snode *sp;
	struct vnode *blkvp;
	dev_t dev;
	struct buf *bp;
	daddr_t lbn, bn;
	register int n, on;
	int size;
	long bsize;
	extern int mem_no;
	int error = 0;

	sp = VTOS(vp);
	dev = (dev_t)sp->s_dev;
	if (rw != UIO_READ && rw != UIO_WRITE)
		panic("rwsp");
	if (rw == UIO_READ && uiop->uio_resid == 0)
		return (0);
	if ((uiop->uio_offset < 0 || (uiop->uio_offset + uiop->uio_resid) < 0)
	    && !(vp->v_type == VCHR && mem_no == major(dev))) {
		return (EINVAL);
	}
	if (rw == UIO_READ) {
		smark(VTOS(vp), SACC);
	}
	if (vp->v_type == VCHR) {
		if (rw == UIO_READ) {
			error = (*cdevsw[major(dev)].d_read)(dev, uiop);
		} else {
			smark(VTOS(vp), SUPD|SCHG);
			error = (*cdevsw[major(dev)].d_write)(dev, uiop);
		}
		return (error);
	} else if (vp->v_type != VBLK) {
		return (EOPNOTSUPP);
	}
	if (uiop->uio_resid == 0)
		return (0);
	bsize = BLKDEV_IOSIZE;
	u.u_error = 0;
	blkvp = VTOS(vp)->s_bdevvp;
	do {
		lbn = uiop->uio_offset / bsize;
		on = uiop->uio_offset % bsize;
		n = MIN((unsigned)(bsize - on), uiop->uio_resid);
		bn = lbn * (BLKDEV_IOSIZE/DEV_BSIZE);
		rablock = bn + (BLKDEV_IOSIZE/DEV_BSIZE);
		rasize = size = bsize;
		if (rw == UIO_READ) {
			if ((long)bn<0) {
				bp = geteblk(size);
				clrbuf(bp);
			} else if (sp->s_lastr + 1 == lbn)
				bp = breada(blkvp, bn, size, rablock,
					rasize);
			else
				bp = bread(blkvp, bn, size);
			sp->s_lastr = lbn;
		} else {
			int i, count;
			extern struct cmap *mfind();

			count = howmany(size, DEV_BSIZE);
			for (i = 0; i < count; i += CLBYTES/DEV_BSIZE)
				if (mfind(blkvp, (daddr_t)(bn + i)))
					munhash(blkvp, (daddr_t)(bn + i));
			if (n == bsize) 
				bp = getblk(blkvp, bn, size);
			else
				bp = bread(blkvp, bn, size);
		}
		n = MIN(n, bp->b_bcount - bp->b_resid);
		if (bp->b_flags & B_ERROR) {
			error = EIO;
			brelse(bp);
			goto bad;
		}
		u.u_error = uiomove(bp->b_un.b_addr+on, n, rw, uiop);
		if (rw == UIO_READ) {
			if (n + on == bsize)
				bp->b_flags |= B_AGE;
			brelse(bp);
		} else {
			if (ioflag & IO_SYNC)
				bwrite(bp);
			else if (n + on == bsize) {
				bp->b_flags |= B_AGE;
				bawrite(bp);
			} else
				bdwrite(bp);
			smark(VTOS(vp), SUPD|SCHG);
		}
	} while (u.u_error == 0 && uiop->uio_resid > 0 && n != 0);
	if (error == 0)				/* XXX */
		error = u.u_error;		/* XXX */
bad:
	return (error);
}

/*ARGSUSED*/
int
spec_ioctl(vp, com, data, flag, cred)
	struct vnode *vp;
	int com;
	caddr_t data;
	int flag;
	struct ucred *cred;
{
	register struct snode *sp;

	sp = VTOS(vp);
	if (vp->v_type != VCHR)
		panic("spec_ioctl");
	return ((*cdevsw[major(sp->s_dev)].d_ioctl)
			(sp->s_dev, com, data, flag));
}

/*ARGSUSED*/
int
spec_select(vp, which, cred)
	struct vnode *vp;
	int which;
	struct ucred *cred;
{
	register struct snode *sp;

	sp = VTOS(vp);
	if (vp->v_type != VCHR)
		panic("spec_select");
	return ((*cdevsw[major(sp->s_dev)].d_select)(sp->s_dev, which));
}

/*ARGSUSED*/
int
spec_inactive(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
	int olderror;

	/* XXX
	 * spec_fsync does a xxx_setattr which may set u.u_error. Blech.
	 */
	olderror = u.u_error;
	(void) spec_fsync(vp, cred);
	u.u_error = olderror;
	sunsave(VTOS(vp));
	kmem_free((caddr_t)VTOS(vp), (u_int)sizeof (struct snode));
	return (0);
}

int
spec_getattr(vp, vap, cred)
	struct vnode *vp;
	register struct vattr *vap;
	struct ucred *cred;
{
	int error;
	register struct snode *sp;

	sp = VTOS(vp);
	error = VOP_GETATTR(sp->s_realvp, vap, cred);
	if (!error) {
		/* set current times from snode, even if older than vnode */
		vap->va_atime = sp->s_atime;
		vap->va_mtime = sp->s_mtime;
		vap->va_ctime = sp->s_ctime;

		/* set device-dependent blocksizes */
		switch (vap->va_type) {
		case VBLK:
			vap->va_blocksize = BLKDEV_IOSIZE;
			break;

		case VCHR:
			vap->va_blocksize = MAXBSIZE;
			break;
		}
	}
	return (error);
}

int
spec_setattr(vp, vap, cred)
	struct vnode *vp;
	register struct vattr *vap;
	struct ucred *cred;
{
	struct snode *sp;
	int error;
	register int chtime = 0;

	sp = VTOS(vp);
	error = VOP_SETATTR(sp->s_realvp, vap, cred);
	if (!error) {
		/* if times were changed, update snode */
		if (vap->va_atime.tv_sec != -1) {
			sp->s_atime = vap->va_atime;
			chtime++;
		}
		if (vap->va_mtime.tv_sec != -1) {
			sp->s_mtime = vap->va_mtime;
			chtime++;
		}
		if (chtime)
			sp->s_ctime = time;
	}
	return (error);
}

int
spec_access(vp, mode, cred)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
{

	return (VOP_ACCESS(VTOS(vp)->s_realvp, mode, cred));
}

spec_link(vp, tdvp, tnm, cred)
	struct vnode *vp;
	struct vnode *tdvp;
	char *tnm;
	struct ucred *cred;
{

	return (VOP_LINK(VTOS(vp)->s_realvp, tdvp, tnm, cred));
}

/*
 * In order to sync out the snode times without multi-client problems,
 * make sure the times written out are never earlier than the times
 * already set in the vnode.
 */
int
spec_fsync(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
	register int error;
	register struct snode *sp;
	struct vattr *vap;
	struct vattr *vatmp;

	sp = VTOS(vp);
	/* if times didn't change, don't flush anything */
	if ((sp->s_flag & (SACC|SUPD|SCHG)) == 0)
		return (0);

	vatmp = (struct vattr *)kmem_alloc((u_int)sizeof (*vatmp));
	error = VOP_GETATTR(sp->s_realvp, vatmp, cred);
	if (!error) {
		vap = (struct vattr *)kmem_alloc((u_int)sizeof (*vap));
		vattr_null(vap);
		vap->va_atime = timercmp(&vatmp->va_atime, &sp->s_atime, >) ?
		    vatmp->va_atime : sp->s_atime;
		vap->va_mtime = timercmp(&vatmp->va_mtime, &sp->s_mtime, >) ?
		    vatmp->va_mtime : sp->s_mtime;
		VOP_SETATTR(sp->s_realvp, vap, cred);
		kmem_free((caddr_t)vap, (u_int)sizeof (*vap));
	}
	kmem_free((caddr_t)vatmp, (u_int)sizeof (*vatmp));
	(void) VOP_FSYNC(sp->s_realvp, cred);
	return (0);
}

int
spec_badop()
{
	panic("spec_badop");
}

int
spec_noop()
{

	return (EINVAL);
}

/*
 * Record-locking requests are passed back to the real vnode handler.
 */
int
spec_lockctl(vp, ld, cmd, cred)
	struct vnode *vp;
	struct flock *ld;
	int cmd;
	struct ucred *cred;
{
	return (VOP_LOCKCTL(VTOS(vp)->s_realvp, ld, cmd, cred));
}

spec_fid(vp, fidpp)
	struct vnode *vp;
	struct fid **fidpp;
{
	return (VOP_FID(VTOS(vp)->s_realvp, fidpp));
}
