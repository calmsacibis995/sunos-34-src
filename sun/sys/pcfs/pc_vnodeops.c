#ifndef lint
static	char sccsid[] = "@(#)pc_vnodeops.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/dir.h"
#include "../h/stat.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/conf.h"
#include "../h/kernel.h"
#include "../h/cmap.h"
#include "../pcfs/pc_label.h"
#include "../pcfs/pc_fs.h"
#include "../pcfs/pc_dir.h"
#include "../pcfs/pc_node.h"

extern int pcfs_open();
extern int pcfs_close();
extern int pcfs_rdwr();
extern int pcfs_getattr();
extern int pcfs_setattr();
extern int pcfs_access();
extern int pcfs_lookup();
extern int pcfs_create();
extern int pcfs_remove();
extern int pcfs_rename();
extern int pcfs_mkdir();
extern int pcfs_rmdir();
extern int pcfs_readdir();
extern int pcfs_fsync();
extern int pcfs_inactive();
extern int pcfs_bmap();
extern int pcfs_badop();
extern int pcfs_invalop();

/*
 * vnode op vectors for files, directories, and invalid files.
 */
struct vnodeops pcfs_fvnodeops = {
	pcfs_open,
	pcfs_close,
	pcfs_rdwr,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_getattr,
	pcfs_setattr,
	pcfs_access,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_fsync,
	pcfs_inactive,
	pcfs_bmap,
	pcfs_badop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
};

struct vnodeops pcfs_dvnodeops = {
	pcfs_open,
	pcfs_close,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_getattr,
	pcfs_invalop,
	pcfs_access,
	pcfs_lookup,
	pcfs_create,
	pcfs_remove,
	pcfs_invalop,
	pcfs_rename,
	pcfs_mkdir,
	pcfs_rmdir,
	pcfs_readdir,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_inactive,
	pcfs_invalop,
	pcfs_badop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop
};

struct vnodeops pcfs_ivnodeops = {
	pcfs_invalop,
	pcfs_close,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_inactive,
	pcfs_invalop,
	pcfs_badop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop
};

int
pcfs_open(vpp, flag)
	struct vnode **vpp;
	int flag;
{

PCFSDEBUG(3)
printf("pcfs_open(&0x%x, 0%o)\n", *vpp, flag);
	return (0);
}

/*
 * files are sync'ed on close to keep floppy up to date
 */
int
pcfs_close(vp, flag)
	struct vnode *vp;
	int flag;
{
	register struct pcfs *fsp;
	int error;

PCFSDEBUG(3)
printf("pcfs_close(0x%x, %d)\n", vp, flag);
	if (vp->v_type == VDIR)
		return (0);
	fsp = VFSTOPCFS(vp->v_vfsp);
	error = pc_lockfs(fsp);
	if (error)
		return(error);
	pc_nodesync(VTOPC(vp));
	pc_unlockfs(fsp);
	binvalfree(fsp->pcfs_devvp);
	return (0);
}

/*
 * read or write a vnode
 */
int
pcfs_rdwr(vp, uiop, rw, ioflag)
	struct vnode *vp;
	struct uio *uiop;
	enum uio_rw rw;
	int ioflag;
{
	register struct pcfs *fsp;
	register struct pcnode *pcp;
	int error;

	fsp = VFSTOPCFS(vp->v_vfsp);
	pcp = VTOPC(vp);
	error = pc_lockfs(fsp);
	if (error)
		return (error);
	if ((ioflag & IO_APPEND) && (rw == UIO_WRITE)) {
		/*
		 * in append mode start at end of file.
		 */
		uiop->uio_offset = pcp->pc_size;
	}
	error = rwpcp(pcp, uiop, rw, ioflag);
	pc_unlockfs(fsp);
if (error) {
PCFSDEBUG(3)
printf("pcfs_rdwr: io error = %d\n", error);
}
	return (error);
}

int
rwpcp(pcp, uio, rw, ioflag)
	register struct pcnode *pcp;
	register struct uio *uio;
	enum uio_rw rw;
	int ioflag;
{
	struct vnode *devvp;
	struct buf *bp;
	struct pcfs *fsp;
	daddr_t lbn;				/* logical block number */
	daddr_t bn;				/* phys block number */
	daddr_t rabn;
	register int n, off;
	int bsize;
	int error = 0;

	if (rw != UIO_READ && rw != UIO_WRITE)
		panic("rwpcp");
	if (uio->uio_offset < 0 || (uio->uio_offset + uio->uio_resid) < 0)
		return (EINVAL);
	if (uio->uio_resid == 0)
		return (0);
	if (rw == UIO_WRITE &&
	    uio->uio_offset + uio->uio_resid >
	      u.u_rlimit[RLIMIT_FSIZE].rlim_cur) {
		psignal(u.u_procp, SIGXFSZ);
		return (EFBIG);
	}
#ifdef notdef
	if ((ioflag & IOSYNC) && !pc_verify(pcp))
		return (EIO);
#endif
	u.u_error = 0;
	fsp = VFSTOPCFS(pcp->pc_vn.v_vfsp);
	devvp = fsp->pcfs_devvp;
	bsize = fsp->pcfs_clsize;
	do {
		lbn = uio->uio_offset / bsize;
		off = uio->uio_offset % bsize;
		n = MIN((unsigned)(bsize - off), uio->uio_resid);
		if (rw == UIO_READ) {
			int diff;

			diff = pcp->pc_size - uio->uio_offset;
			if (diff <= 0)
				return (0);
			if (diff < n)
				n = diff;
			error = pc_bmap(pcp, lbn, &bn, &rabn);
			if (error) {
				if (error == ENOENT)	/* read past EOF */
					error = 0;
				return (0);
			}
			if (pcp->pc_lastr + 1 == lbn && rabn != (daddr_t)0)
				bp = breada(devvp, bn, bsize, rabn, bsize);
			else
				bp = bread(devvp, bn, bsize);
			pcp->pc_lastr = lbn;
		} else {
			int i, count;
			extern struct cmap *mfind();

			error = pc_balloc(pcp, lbn, &bn);
			if (error) {
				if (error == ENOENT)
					error = 0;
				return (error);
			}
			if (uio->uio_offset + n > pcp->pc_size)
				pcp->pc_size = uio->uio_offset + n;
			count = howmany(bsize, DEV_BSIZE);
			for (i = 0; i < count; i += CLBYTES/DEV_BSIZE)
				if (mfind(devvp, (daddr_t)(bn + i)))
					munhash(devvp, (daddr_t)(bn + i));
			if (n == bsize)
				bp = getblk(devvp, bn, bsize);
			else
				bp = bread(devvp, bn, bsize);
		}
		n = MIN(n, bsize - bp->b_resid);
		if (bp->b_flags & (B_ERROR | B_INVAL)) {
			if (bp->b_flags & B_ERROR)
				pc_diskchanged(fsp);
			brelse(bp);
			error = EIO;
			goto bad;
		}
		u.u_error = uiomove(bp->b_un.b_addr+off, n, rw, uio);
		if (rw == UIO_READ) {
			if (n + off == bsize || uio->uio_offset == pcp->pc_size)
				bp->b_flags |= B_AGE;
			brelse(bp);
		} else {
			if (ioflag & IO_SYNC) {
				bwrite(bp);
			} else if (n + off == bsize) {
				bp->b_flags |= B_AGE;
				bawrite(bp);
			} else {
				bdwrite(bp);
			}
			pc_mark(pcp);
		}
	} while (u.u_error == 0 && uio->uio_resid > 0 && n != 0);
#ifdef notdef
	if ((ioflag & IO_SYNC) && (rw == UIO_WRITE) && (pcp->pc_flags & PC_MOD))
		pc_nodeupdate(pcp);
#endif
	if (error == 0)				/* XXX */
		error = u.u_error;		/* XXX */
bad:
	return (error);
}

int
pcfs_getattr(vp, vap)
	struct vnode *vp;
	register struct vattr *vap;
{
	register struct pcnode *pcp;
	register struct pcfs *fsp;
	int error;
	char attr;

PCFSDEBUG(3)
printf("pcfs_getattr(0x%x, 0x%x)\n", vp, vap);
	pcp = VTOPC(vp);
	fsp = VFSTOPCFS(vp->v_vfsp);
	error = pc_lockfs(fsp);
	if (error)
		return (error);
	/*
	 * Copy from pcnode.
	 */
	vap->va_type = vp->v_type;
	attr = pcp->pc_entry.pcd_attr;
	if (attr & (PCA_HIDDEN|PCA_SYSTEM))
		vap->va_mode = 0;
	else if (attr & PCA_RDONLY)
		vap->va_mode = 0555;
	else
		vap->va_mode = 0777;
	if (attr & PCA_DIR)
		vap->va_mode |= S_IFDIR;
	else
		vap->va_mode |= S_IFREG;
	vap->va_uid = u.u_uid;
	vap->va_gid = u.u_gid;
	vap->va_fsid = fsp->pcfs_devvp->v_rdev;
	vap->va_nodeid =
	     pc_makenodeid(pcp->pc_eblkno, pcp->pc_eoffset, &pcp->pc_entry);
	vap->va_nlink = 1;
	vap->va_size = pcp->pc_size;
	pc_pcttotv(&pcp->pc_entry.pcd_mtime, &vap->va_mtime);
	vap->va_ctime = vap->va_atime = vap->va_mtime;
	vap->va_rdev = -1;
	vap->va_blocks = howmany(pcp->pc_size, DEV_BSIZE);
	vap->va_blocksize = fsp->pcfs_clsize;
	pc_unlockfs(fsp);
	return (0);
}

int
pcfs_setattr(vp, vap)
	struct vnode *vp;
	register struct vattr *vap;
{
	register struct pcnode *pcp;
	int error;

PCFSDEBUG(3) {
printf("pcfs_setattr(0x%x)\n", vp);
}
	/*
	 * cannot set these attributes
	 */
	if ((vap->va_nlink != -1) || (vap->va_blocksize != -1) ||
	    (vap->va_rdev != -1) || (vap->va_blocks != -1) ||
	    (vap->va_fsid != -1) || (vap->va_nodeid != -1) ||
	    (vap->va_uid != -1) || (vap->va_gid != -1) ||
	    ((int)vap->va_type != -1)) {
printf("pcfs_setattr: invalid attributes\n");
		return (EINVAL);
	}

	pcp = VTOPC(vp);
	error = pc_lockfs(VFSTOPCFS(vp->v_vfsp));
	if (error)
		return (error);
	/*
	 * Change file access modes.
	 * If nobody has write permision, file is marked readonly.
	 * Otherwise file is writeable by anyone.
	 */
	if (vap->va_mode != (u_short)-1) {
PCFSDEBUG(5)
printf("pcfs_setattr mode=0%o\n", vap->va_mode);
		if ((vap->va_mode & 0222) == 0)
			pcp->pc_entry.pcd_attr |= PCA_RDONLY;
		else
			pcp->pc_entry.pcd_attr &= ~PCA_RDONLY;
		pcp->pc_flags |= PC_CHG;
	}
	/*
	 * Truncate file. Must have write permission and not be a directory.
	 */
	if (vap->va_size != (u_long)-1) {
PCFSDEBUG(5)
printf("pcfs_setattr size=%d\n", vap->va_size);
		if (pcp->pc_entry.pcd_attr & PCA_DIR) {
			error = EISDIR;
			goto out;
		}
		if (pcp->pc_entry.pcd_attr & PCA_RDONLY) {
			error = EACCES;
			goto out;
		}
		error = pc_truncate(pcp, (long)vap->va_size);
		if (error)
			goto out;
	}
	/*
	 * Change file modified times.
	 */
	if (vap->va_mtime.tv_sec != -1) {
PCFSDEBUG(5)
printf("pcfs_setattr modifying time\n");
		pc_tvtopct(&vap->va_mtime, &pcp->pc_entry.pcd_mtime);
		pcp->pc_flags |= PC_CHG;
	}
out:
	pc_nodesync(pcp);
	pc_unlockfs(VFSTOPCFS(vp->v_vfsp));
	return (error);
}

int
pcfs_access(vp, mode)
	struct vnode *vp;
	int mode;
{

	if ((mode & VWRITE) && (VTOPC(vp)->pc_entry.pcd_attr & PCA_RDONLY))
		return (EACCES);
	else
		return (0);
}

int
pcfs_fsync(vp)
	struct vnode *vp;
{
	int error;

PCFSDEBUG(3)
printf("pcfs_fsync(0x%x)\n",vp);
	error = pc_lockfs(VFSTOPCFS(vp->v_vfsp));
	if (error)
		return (error);
	pc_nodesync(VTOPC(vp));
	pc_unlockfs(VFSTOPCFS(vp->v_vfsp));
	return (0);
}

int
pcfs_inactive(vp)
	struct vnode *vp;
{
	register struct pcnode *pcp;

PCFSDEBUG(1)
printf("pcfs_inactive(0x%x)\n",vp);
	pcp = VTOPC(vp);
	/*
	 * Here's the scoop. V_count is 0 while we're waiting to lock the
	 * file system. This means that if any other process operates on
	 * this node while we're waiting it can rerelease it. So we
	 * mark this node as release pending.
	 */
	if (pcp->pc_flags & PC_RELE_PEND)
		return (0);
	pcp->pc_flags |= PC_RELE_PEND;
	PC_LOCKFS(VFSTOPCFS(vp->v_vfsp));
	pcp->pc_flags &= ~PC_RELE_PEND;
	/*
	 * Retest v_count because pc_lockfs can sleep.
	 */
	if (vp->v_count == 0) {
		pc_rele(pcp);
	}
	PC_UNLOCKFS(VFSTOPCFS(vp->v_vfsp));
	return (0);
}

/*
 * Unix file system operations having to do with directory manipulation.
 */

/*
 * lookup a name in a directory
 */
pcfs_lookup(dvp, nm, vpp)
	struct vnode *dvp;
	char *nm;
	struct vnode **vpp;
{
	struct pcnode *pcp;
	register int error;

PCFSDEBUG(3)
printf("pcfs_lookup(0x%x, %s)\n", dvp, nm);
	error = pc_lockfs(VFSTOPCFS(dvp->v_vfsp));
	if (error)
		return (error);
	error = pc_dirlook(VTOPC(dvp), nm, &pcp);
	if (!error) {
		*vpp = PCTOV(pcp);
	}
	pc_unlockfs(VFSTOPCFS(dvp->v_vfsp));
	return (error);
}

pcfs_create(dvp, nm, vap, exclusive, mode, vpp)
	struct vnode *dvp;
	char *nm;
	struct vattr *vap;
	enum vcexcl exclusive;
	int mode;
	struct vnode **vpp;
{
	register int error;
	struct pcnode *pcp;

PCFSDEBUG(3)
printf("pcfs_create(0x%x,%s,excl=%d,0%o)\n",dvp,nm,exclusive,mode);
	/*
	 * can't create directories. use pcfs_mkdir.
	 */
	if (vap->va_type == VDIR)
		return (EISDIR);
	pcp = (struct pcnode *) 0;
	error = pc_lockfs(VFSTOPCFS(dvp->v_vfsp));
	if (error)
		return (error);
	error = pc_direnter(VTOPC(dvp), nm, vap, &pcp);
	/*
	 * if file exists and this is a nonexclusive create,
	 * check for access permissions
	 */
	if (error == EEXIST) {
		if (exclusive == NONEXCL) {
			if (pcp->pc_vn.v_type == VDIR) {
				error = EISDIR;
			} else if (mode) {
				error = pcfs_access(PCTOV(pcp), mode);
			} else {
				error = 0;
			}
		}
		if (error) {
			if (pcp->pc_vn.v_count == 1) {
				pcp->pc_vn.v_count = 0;
				if (!(pcp->pc_flags & PC_RELE_PEND))
					pc_rele(pcp);
			} else {
				VN_RELE(PCTOV(pcp));
			}
		}
	}
	if (error) {
		pc_unlockfs(VFSTOPCFS(dvp->v_vfsp));
		return (error);
	}
	/*
	 * truncate regular files, if required
	 */
	if ((pcp->pc_vn.v_type == VREG) && (vap->va_size == 0)) {
		error = pc_truncate(pcp, 0L);
		if (error) {
			pc_unlockfs(VFSTOPCFS(dvp->v_vfsp));
			VN_RELE(PCTOV(pcp));
			return (error);
		}
		pc_nodesync(pcp);
	}
	*vpp = PCTOV(pcp);
	pc_unlockfs(VFSTOPCFS(dvp->v_vfsp));
	return (0);
}

pcfs_remove(vp, nm)
	struct vnode *vp;
	char *nm;
{
	register int error;

PCFSDEBUG(3)
printf("pcfs_remove(0x%x,%s)\n",vp,nm);
	error = pc_lockfs(VFSTOPCFS(vp->v_vfsp));
	if (error)
		return (error);
	error = pc_dirremove(VTOPC(vp), nm, VREG);
	pc_unlockfs(VFSTOPCFS(vp->v_vfsp));
	return (error);
}

/*
 * Rename a file or directory
 * This rename is restricted to only rename files within a directory.
 */
pcfs_rename(sdvp, snm, tdvp, tnm)
	struct vnode *sdvp;		/* old (source) parent vnode */
	char *snm;			/* old (source) entry name */
	struct vnode *tdvp;		/* new (target) parent vnode */
	char *tnm;			/* new (target) entry name */
{
	register struct pcnode *dp;	/* parent pcnode */
	register int error;

PCFSDEBUG(3)
printf("pcfs_rename(0x%x,%s,0x%x,%s)\n",sdvp,snm,tdvp,tnm);
	dp = VTOPC(sdvp);
	/*
	 * make sure source and target directories are the same
	 */
	if (dp->pc_scluster != VTOPC(tdvp)->pc_scluster)
		return (EXDEV);		/* XXX */
	/*
	 * make sure we can muck with this directory.
	 */
	error = pcfs_access(sdvp, VWRITE);
	if (error) {
		return (error);
	}
	error = pc_lockfs(VFSTOPCFS(dp->pc_vn.v_vfsp));
	if (error)
		return (error);
	error = pc_rename(dp, snm, tnm);
	pc_unlockfs(VFSTOPCFS(dp->pc_vn.v_vfsp));
	return (error);
}

pcfs_mkdir(dvp, nm, vap, vpp)
	struct vnode *dvp;
	char *nm;
	register struct vattr *vap;
	struct vnode **vpp;
{
	struct pcnode *pcp;
	register int error;

PCFSDEBUG(3)
printf("pcfs_mkdir(0x%x,%s)\n",dvp,nm);
	error = pc_lockfs(VFSTOPCFS(dvp->v_vfsp));
	if (error)
		return (error);
	error = pc_direnter(VTOPC(dvp), nm, vap, &pcp);
	pc_unlockfs(VFSTOPCFS(dvp->v_vfsp));
	if (!error) {
		*vpp = PCTOV(pcp);
	} else if (error == EEXIST) {
		VN_RELE(PCTOV(pcp));
	}
	return (error);
}

pcfs_rmdir(dvp, nm)
	struct vnode *dvp;
	char *nm;
{
	register int error;

PCFSDEBUG(3)
printf("pcfs_rmdir(0x%x,%s)\n", dvp, nm);
	error = pc_lockfs(VFSTOPCFS(dvp->v_vfsp));
	if (error)
		return (error);
	error = pc_dirremove(VTOPC(dvp), nm, VDIR);
	pc_unlockfs(VFSTOPCFS(dvp->v_vfsp));
	return (error);
}

/*
 * read entries in a directory.
 * we must convert pc format to unix format
 */
int
pcfs_readdir(dvp, uiop)
	struct vnode *dvp;
	register struct uio *uiop;
{
	register char *fp, *tp;
	register struct pcnode *pcp;
	struct pcdir *ep;
	struct buf *bp = NULL;
	register char c;
	register int i;
	register long offset;
	register int n;
	register int boff;
	struct direct d;
	int error;
	char *strcpy();

#define DIRENTSIZE (sizeof (d.d_fileno) + sizeof (d.d_reclen) + \
		sizeof (d.d_namlen) + \
		roundup((PCFNAMESIZE + PCFEXTSIZE + 2), sizeof (int)) )

PCFSDEBUG(5)
printf("pcfs_readdir\n");
	if ((uiop->uio_iovcnt != 1) || (uiop->uio_offset % DIRENTSIZE))
		return (EINVAL);
	pcp = VTOPC(dvp);
	error = pc_lockfs(VFSTOPCFS(dvp->v_vfsp));
	if (error)
		return (error);
	/*
	 * verify that the dp is still valid on the disk
	 */
	(void) pc_verify(pcp);
	n = uiop->uio_resid / DIRENTSIZE;
	offset = (uiop->uio_offset / DIRENTSIZE) * sizeof(struct pcdir);
	if (pcp->pc_vn.v_flag & VROOT) {
		/*
		 * kludge up entries for "." and ".." in the root.
		 */
		if (offset == 0 && n) {
			d.d_fileno = -1;
			d.d_reclen = DIRENTSIZE;
			d.d_namlen = 1;
			(void) strcpy(d.d_name, ".");
			(void) uiomove((caddr_t)&d, DIRENTSIZE, UIO_READ, uiop);
			offset = sizeof(struct pcdir);
			n--;
		}
		if (offset == sizeof(struct pcdir) && n) {
			d.d_fileno = -1;
			d.d_reclen = DIRENTSIZE;
			d.d_namlen = 2;
			(void) strcpy(d.d_name, "..");
			(void) uiomove((caddr_t)&d, DIRENTSIZE, UIO_READ, uiop);
			offset = 2 * sizeof(struct pcdir);
			n--;
		}
		offset -= 2 * sizeof(struct pcdir);
	}
	for (;n--; ep++, offset += sizeof(struct pcdir)) {
		boff = pc_blkoff(VFSTOPCFS(dvp->v_vfsp), offset);
		if (boff == 0 || bp == NULL || boff >= bp->b_bcount) {
			if (bp != NULL) {
				brelse(bp);
				bp = NULL;
			}
                        error = pc_blkatoff(pcp, offset, &bp, &ep);
			if (error) {
				/*
				 * When is an error not an error?
				 */
				if (error = ENOENT)
					error = 0;
                                break;
			}
			bp->b_flags |= B_NOCACHE;
                }
		if (ep->pcd_filename[0] == PCD_UNUSED)
			break;
		if ((ep->pcd_filename[0] == PCD_ERASED) ||
		    (ep->pcd_attr & (PCA_HIDDEN | PCA_SYSTEM)) ) {
			uiop->uio_offset += DIRENTSIZE;
			continue;
		}
		d.d_fileno = pc_makenodeid(bp->b_blkno, boff, ep);
		d.d_reclen = DIRENTSIZE;
		tp = &d.d_name[0];
		fp = &ep->pcd_filename[0];
		i = PCFNAMESIZE;
		while (i-- && ((c = *fp) != ' ')) {
			if (!(c == '.' || pc_validchar(c))) {
				tp = &d.d_name[0];
				break;
			}
			*tp++ = tolower(c);
			fp++;
		}
		fp = &ep->pcd_ext[0];
		if (tp != &d.d_name[0] && *fp != ' ') {
			*tp++ = '.';
			i = PCFEXTSIZE;
			while (i-- && ((c = *fp) != ' ')) {
				if (!pc_validchar(c)) {
					tp = &d.d_name[0];
					break;
				}
				*tp++ = tolower(c);
				fp++;
			}
		}
		*tp = 0;
		d.d_namlen = tp - &d.d_name[0];
PCFSDEBUG(6)
printf("pcfs_readdir: found %s, namelen=%d, reclen=%d, offset=%d, n=%d, ep=0x%x\n", d.d_name, d.d_namlen, d.d_reclen, offset, n, ep);
		if (d.d_namlen) {
			(void) uiomove((caddr_t)&d, DIRENTSIZE, UIO_READ, uiop);
		} else {
			uiop->uio_offset += DIRENTSIZE;
		}
	}
	if (bp)
		brelse(bp);
	pc_unlockfs(VFSTOPCFS(dvp->v_vfsp));
	return (error);
#undef DIRENTSIZE
}

#ifdef notneeded
rdwrpc(rw, pcp, base, len, offset, seg, aresid)
	enum uio_rw rw;
	struct pcnode *pcp;
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
	error = pcfs_rdwr(PCTOV(pcp), &auio, rw, 0);
	if (aresid) {
		*aresid = auio.uio_resid;
	} else if (auio.uio_resid) {
		error = EIO;
	}
	return (error);
}
#endif notneeded

int
pcfs_bmap(vp, lbn, vpp, bnp)
struct vnode *vp;
daddr_t lbn;
struct vnode **vpp;
daddr_t *bnp;
{
	register struct pcnode *pcp;
	register struct pcfs *fsp;
	int error = 0;

	pcp = VTOPC(vp);
	fsp = VFSTOPCFS(pcp->pc_vn.v_vfsp);
	error = pc_lockfs(fsp);
	if (error)
		return (error);
	if (vpp)
		*vpp = fsp->pcfs_devvp;
	if (bnp)
		error = pc_bmap(pcp, lbn, bnp, (daddr_t *)0);
	pc_unlockfs(fsp);
	return (error);
}

int
pcfs_badop()
{
	panic("pcfs_badop");
}

int
pcfs_invalop()
{
	return (EINVAL);
}
