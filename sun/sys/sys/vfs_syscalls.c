/*	@(#)vfs_syscalls.c 1.1 86/09/25 SMI	*/

#include "../h/param.h"
#include "../h/user.h"
#include "../h/file.h"
#include "../h/stat.h"
#include "../h/uio.h"
#include "../h/tty.h"
#include "../h/vfs.h"
#include "../h/pathname.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"

extern	struct fileops vnodefops;

/*
 * System call routines for operations on files other
 * than read, write and ioctl.  These calls manipulate
 * the per-process file table which references the
 * networkable version of normal UNIX inodes, called vnodes.
 *
 * Many operations take a pathname, which is read
 * into a kernel buffer by pn_get (see vfs_pathname.c).
 * After preparing arguments for an operation, a simple
 * operation proceeds:
 *
 *	error = lookupname(pname, seg, followlink, &dvp, &vp, &vattr)
 *
 * where pname is the pathname operated on, seg is the segment that the
 * pathname is in (UIOSEG_USER or UIOSEG_KERNEL), followlink specifies
 * whether to follow symbolic links, dvp is a pointer to the vnode that
 * represents the parent directory of vp, the pointer to the vnode
 * referenced by the pathname. vattr is a vattr structure which hold the
 * attributes of the final component. The lookupname routine fetches the
 * pathname string into an internal buffer using pn_get (vfs_pathname.c),
 * and iteratively running down each component of the path until the
 * the final vnode and/or it's parent are found. If either of the addresses
 * for dvp or vp are NULL, then it assumes that the caller is not interested
 * in that vnode. If the pointer to the vattr structure is NULL then attributes
 * are not returned. Once the vnode or its parent is found, then a vnode
 * operation (e.g. VOP_OPEN) may be applied to it.
 *
 * One important point is that the operations on vnode's are atomic, so that
 * vnode's are never locked at this level.  Vnode locking occurs
 * at lower levels either on this or a remote machine. Also permission
 * checking is generally done by the specific filesystem. The only
 * checks done by the vnode layer is checks involving file types
 * (e.g. VREG, VDIR etc.), since this is static over the life of the vnode.
 *
 */

/*
 * Change current working directory (".").
 */
chdir(uap)
	register struct a {
		char *dirnamep;
	} *uap;
{
	struct vnode *vp;

	u.u_error = chdirec(uap->dirnamep, &vp);
	if (u.u_error == 0) {
		VN_RELE(u.u_cdir);
		u.u_cdir = vp;
	}
}

/*
 * Change notion of root ("/") directory.
 */
chroot(uap)
	register struct a {
		char *dirnamep;
	} *uap;
{
	struct vnode *vp;

	if (!suser())
		return;

	u.u_error = chdirec(uap->dirnamep, &vp);
	if (u.u_error == 0) {
		if (u.u_rdir != (struct vnode *)0)
			VN_RELE(u.u_rdir);
		u.u_rdir = vp;
	}
}

/*
 * Common code for chdir and chroot.
 * Translate the pathname and insist that it
 * is a directory to which we have execute access.
 * If it is replace u.u_[cr]dir with new vnode.
 */
chdirec(dirnamep, vpp)
	char *dirnamep;
	struct vnode **vpp;
{
	struct vnode *vp;		/* new directory vnode */
	register int error;

	error =
	    lookupname(dirnamep, UIOSEG_USER, FOLLOW_LINK,
		(struct vnode **)0, &vp);
	if (error)
		return (error);
	if (vp->v_type != VDIR) {
		error = ENOTDIR;
	} else {
		error = VOP_ACCESS(vp, VEXEC, u.u_cred);
	}
	if (error) {
		VN_RELE(vp);
	} else {
		*vpp = vp;
	}
	return (error);
}

/*
 * Open system call.
 */
open(uap)
	register struct a {
		char *fnamep;
		int fmode;
		int cmode;
	} *uap;
{

	u.u_error = copen(uap->fnamep, uap->fmode - FOPEN, uap->cmode);
}

/*
 * Creat system call.
 */
creat(uap)
	register struct a {
		char *fnamep;
		int cmode;
	} *uap;
{

	u.u_error = copen(uap->fnamep, FWRITE|FCREAT|FTRUNC, uap->cmode);
}

/*
 * Common code for open, creat.
 */
copen(pnamep, filemode, createmode)
	char *pnamep;
	int filemode;
	int createmode;
{
	register struct file *fp;
	struct vnode *vp;
	register int error;
	register int i;

	/*
	 * allocate a user file descriptor and file table entry.
	 */
	fp = falloc();
	if (fp == NULL)
		return(u.u_error);
	i = u.u_r.r_val1;		/* this is bullshit */
	/*
	 * open the vnode.
	 */
	error =
	    vn_open(pnamep, UIOSEG_USER,
		filemode, ((createmode & 07777) & ~u.u_cmask), &vp);

	/*
	 * If there was an error, deallocate the file descriptor.
	 * Otherwise fill in the file table entry to point to the vnode.
	 */
	if (error) {
		u.u_ofile[i] = NULL;
		crfree(fp->f_cred);
		fp->f_count = 0;
	} else {
		fp->f_flag = filemode & FMASK;
		fp->f_type = DTYPE_VNODE;
		fp->f_data = (caddr_t)vp;
		fp->f_ops = &vnodefops;

		/*
		 * For named-pipes, the FNDELAY flag must propagate to
		 * the rdwr layer.  Also, FAPPEND must always be set so
		 * that fp->f_offset is correctly maintained.
		 */
		if (vp->v_type == VFIFO) {
			fp->f_offset = 0;
			fp->f_flag |= FAPPEND | (filemode & FNDELAY);
		}
	}
	return(error);
}

/*
 * Create a special (or regular) file.
 */
mknod(uap)
	register struct a {
		char		*pnamep;
		int		fmode;
		int		dev;
	} *uap;
{
	struct vnode *vp;
	struct vattr vattr;

	/* map 0 type into regular file, as other versions of UNIX do */
	if ((uap->fmode & IFMT) == 0)
		uap->fmode |= IFREG;

	/* Must be super-user unless making a FIFO node */
	if (((uap->fmode & IFMT) != IFIFO) && !suser())
		return;

	/*
	 * Setup desired attributes and vn_create the file.
	 */
	vattr_null(&vattr);
	vattr.va_type = MFTOVT(uap->fmode);
	vattr.va_mode = (uap->fmode & 07777) & ~u.u_cmask;

	switch (vattr.va_type) {
	case VDIR:
		u.u_error = EISDIR;	/* Can't mknod directories: use mkdir */
		return;

	case VBAD:
	case VCHR:
	case VBLK:
		vattr.va_rdev = uap->dev;
		break;

	case VNON:
		u.u_error = EINVAL;
		return;

	default:
		break;
	}

	u.u_error = vn_create(uap->pnamep, UIOSEG_USER, &vattr, EXCL, 0, &vp);
	if (u.u_error == 0)
		VN_RELE(vp);
}

/*
 * Make a directory.
 */
mkdir(uap)
	struct a {
		char	*dirnamep;
		int	dmode;
	} *uap;
{
	struct vnode *vp;
	struct vattr vattr;

	vattr_null(&vattr);
	vattr.va_type = VDIR;
	vattr.va_mode = (uap->dmode & 0777) & ~u.u_cmask;

	u.u_error = vn_create(uap->dirnamep, UIOSEG_USER, &vattr, EXCL, 0, &vp);
	if (u.u_error == 0)
		VN_RELE(vp);
}

/*
 * make a hard link
 */
link(uap)
	register struct a {
		char	*from;
		char	*to;
	} *uap;
{

	u.u_error = vn_link(uap->from, uap->to, UIOSEG_USER);
}

/*
 * rename or move an existing file
 */
rename(uap)
	register struct a {
		char	*from;
		char	*to;
	} *uap;
{

	u.u_error = vn_rename(uap->from, uap->to, UIOSEG_USER);
}

/*
 * Create a symbolic link.
 * Similar to link or rename except target
 * name is passed as string argument, not
 * converted to vnode reference.
 */
symlink(uap)
	register struct a {
		char	*target;
		char	*linkname;
	} *uap;
{
	struct vnode *dvp;
	struct vattr vattr;
	struct pathname tpn;
	struct pathname lpn;

	u.u_error = pn_get(uap->linkname, UIOSEG_USER, &lpn);
	if (u.u_error)
		return;
	u.u_error = lookuppn(&lpn, NO_FOLLOW, &dvp, (struct vnode **)0);
	if (u.u_error) {
		pn_free(&lpn);
		return;
	}
	if (dvp->v_vfsp->vfs_flag & VFS_RDONLY) {
		u.u_error = EROFS;
		goto out;
	}
	u.u_error = pn_get(uap->target, UIOSEG_USER, &tpn);
	vattr_null(&vattr);
	vattr.va_mode = 0777;
	if (u.u_error == 0) {
		u.u_error =
		   VOP_SYMLINK(dvp, lpn.pn_path, &vattr, tpn.pn_path, u.u_cred);
		pn_free(&tpn);
	}
out:
	pn_free(&lpn);
	VN_RELE(dvp);
}

/*
 * Unlink (i.e. delete) a file.
 */
unlink(uap)
	struct a {
		char	*pnamep;
	} *uap;
{

	u.u_error = vn_remove(uap->pnamep, UIOSEG_USER, FILE);
}

/*
 * Remove a directory.
 */
rmdir(uap)
	struct a {
		char	*dnamep;
	} *uap;
{

	u.u_error = vn_remove(uap->dnamep, UIOSEG_USER, DIRECTORY);
}

/*
 * get directory entries in a file system independent format
 */
getdirentries(uap)
	register struct a {
		int	fd;
		char	*buf;
		unsigned count;
		long	*basep;
	} *uap;
{
	struct file *fp;
	struct uio auio;
	struct iovec aiov;

	u.u_error = getvnodefp(uap->fd, &fp);
	if (u.u_error)
		return;
	if ((fp->f_flag & FREAD) == 0) {
		u.u_error = EBADF;
		return;
	}
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = fp->f_offset;
	auio.uio_seg = UIOSEG_USER;
	auio.uio_resid = uap->count;
	u.u_error = VOP_READDIR((struct vnode *)fp->f_data, &auio, fp->f_cred);
	if (u.u_error)
		return;
	u.u_error =
	    copyout((caddr_t)&fp->f_offset, (caddr_t)uap->basep, sizeof(long));
	u.u_r.r_val1 = uap->count - auio.uio_resid;
	fp->f_offset = auio.uio_offset;
}

/*
 * Seek on file.  Only hard operation
 * is seek relative to end which must
 * apply to vnode for current file size.
 * 
 * Note: lseek(0, 0, L_XTND) costs much more than it did before.
 */
lseek(uap)
	register struct a {
		int	fd;
		off_t	off;
		int	sbase;
	} *uap;
{
	struct file *fp;

	u.u_error = getvnodefp(uap->fd, &fp);
	if (u.u_error) {
		if (u.u_error == EINVAL)
			u.u_error = ESPIPE;	/* be compatible */
		return;
	}

	if (((struct vnode *)fp->f_data)->v_type == VFIFO) {
		u.u_error = ESPIPE;
		return;
	}

	switch (uap->sbase) {

	case L_INCR:
		fp->f_offset += uap->off;
		break;

	case L_XTND: {
		struct vattr vattr;

		u.u_error =
		    VOP_GETATTR((struct vnode *)fp->f_data, &vattr, u.u_cred);
		if (u.u_error)
			return;
		fp->f_offset = uap->off + vattr.va_size;
		break;
	}

	case L_SET:
		fp->f_offset = uap->off;
		break;

	default:
		u.u_error = EINVAL;
	}
	u.u_r.r_off = fp->f_offset;
}

/*
 * Determine accessibility of file, by
 * reading its attributes and then checking
 * against our protection policy.
 */
access(uap)
	register struct a {
		char	*fname;
		int	fmode;
	} *uap;
{
	struct vnode *vp;
	register u_short mode;
	register int svuid;
	register int svgid;

	/*
	 * Lookup file
	 */
	u.u_error =
	    lookupname(uap->fname, UIOSEG_USER, FOLLOW_LINK,
		(struct vnode **)0, &vp);
	if (u.u_error)
		return;

	/*
	 * Use the real uid and gid and check access
	 */
	svuid = u.u_uid;
	svgid = u.u_gid;
	u.u_uid = u.u_ruid;
	u.u_gid = u.u_rgid;

	mode = 0;
	/*
	 * fmode == 0 means only check for exist
	 */
	if (uap->fmode) {
		if (uap->fmode & R_OK)
			mode |= VREAD;
		if (uap->fmode & W_OK) {
			if(vp->v_vfsp->vfs_flag & VFS_RDONLY) {
				u.u_error = EROFS;
				goto out;
			}
			mode |= VWRITE;
		}
		if (uap->fmode & X_OK)
			mode |= VEXEC;
		u.u_error = VOP_ACCESS(vp, mode, u.u_cred);
	}

	/*
	 * release the vnode and restore the uid and gid
	 */
out:
	VN_RELE(vp);
	u.u_uid = svuid;
	u.u_gid = svgid;
}

/*
 * Get attributes from file or file descriptor.
 * Argument says whether to follow links, and is
 * passed through in flags.
 */
stat(uap)
	caddr_t uap;
{

	u.u_error = stat1(uap, FOLLOW_LINK);
}

lstat(uap)
	caddr_t uap;
{

	u.u_error = stat1(uap, NO_FOLLOW);
}

stat1(uap0, follow)
	caddr_t uap0;
	enum symfollow follow;
{
	struct vnode *vp;
	struct stat sb;
	register int error;
	register struct a {
		char	*fname;
		struct	stat *ub;
	} *uap = (struct a *)uap0;

	error =
	    lookupname(uap->fname, UIOSEG_USER, follow,
		(struct vnode **)0, &vp);
	if (error)
		return (error);
	error = vno_stat(vp, &sb);
	VN_RELE(vp);
	if (error)
		return (error);
	return (copyout((caddr_t)&sb, (caddr_t)uap->ub, sizeof (sb)));
}

/*
 * Read contents of symbolic link.
 */
readlink(uap)
	register struct a {
		char	*name;
		char	*buf;
		int	count;
	} *uap;
{
	struct vnode *vp;
	struct iovec aiov;
	struct uio auio;

	u.u_error =
	    lookupname(uap->name, UIOSEG_USER, NO_FOLLOW,
		(struct vnode **)0, &vp);
	if (u.u_error)
		return;
	if (vp->v_type != VLNK) {
		u.u_error = EINVAL;
		goto out;
	}
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = 0;
	auio.uio_seg = UIOSEG_USER;
	auio.uio_resid = uap->count;
	u.u_error = VOP_READLINK(vp, &auio, u.u_cred);
out:
	VN_RELE(vp);
	u.u_r.r_val1 = uap->count - auio.uio_resid;
}

/*
 * Change mode of file given path name.
 */
chmod(uap)
	register struct a {
		char	*fname;
		int	fmode;
	} *uap;
{
	struct vattr vattr;

	vattr_null(&vattr);
	vattr.va_mode = uap->fmode & 07777;
	u.u_error = namesetattr(uap->fname, FOLLOW_LINK, &vattr);
}

/*
 * Change mode of file given file descriptor.
 */
fchmod(uap)
	register struct a {
		int	fd;
		int	fmode;
	} *uap;
{
	struct vattr vattr;

	vattr_null(&vattr);
	vattr.va_mode = uap->fmode & 07777;
	u.u_error = fdsetattr(uap->fd, &vattr);
}

/*
 * Change ownership of file given file name.
 */
chown(uap)
	register struct a {
		char	*fname;
		int	uid;
		int	gid;
	} *uap;
{
	struct vattr vattr;

	vattr_null(&vattr);
	vattr.va_uid = uap->uid;
	vattr.va_gid = uap->gid;
	u.u_error = namesetattr(uap->fname, NO_FOLLOW,  &vattr);
}

/*
 * Change ownership of file given file descriptor.
 */
fchown(uap)
	register struct a {
		int	fd;
		int	uid;
		int	gid;
	} *uap;
{
	struct vattr vattr;

	vattr_null(&vattr);
	vattr.va_uid = uap->uid;
	vattr.va_gid = uap->gid;
	u.u_error = fdsetattr(uap->fd, &vattr);
}

/*
 * Set access/modify times on named file.
 */
utimes(uap)
	register struct a {
		char	*fname;
		struct	timeval *tptr;
	} *uap;
{
	struct timeval tv[2];
	struct vattr vattr;

	u.u_error = copyin((caddr_t)uap->tptr, (caddr_t)tv, sizeof (tv));
	if (u.u_error)
		return;
	vattr_null(&vattr);
	vattr.va_atime = tv[0];
	vattr.va_mtime = tv[1];
	u.u_error = namesetattr(uap->fname, FOLLOW_LINK, &vattr);
}

/*
 * Truncate a file given its path name.
 */
truncate(uap)
	register struct a {
		char	*fname;
		int	length;
	} *uap;
{
	struct vattr vattr;

	vattr_null(&vattr);
	vattr.va_size = uap->length;
	u.u_error = namesetattr(uap->fname, FOLLOW_LINK, &vattr);
}

/*
 * Truncate a file given a file descriptor.
 */
ftruncate(uap)
	register struct a {
		int	fd;
		int	length;
	} *uap;
{
	register struct vnode *vp;
	struct file *fp;

	u.u_error = getvnodefp(uap->fd, &fp);
	if (u.u_error)
		return;
	vp = (struct vnode *)fp->f_data;
	if ((fp->f_flag & FWRITE) == 0) {
		u.u_error = EINVAL;
	} else if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
		u.u_error = EROFS;
	} else {
		struct vattr vattr;

		vattr_null(&vattr);
		vattr.va_size = uap->length;
		u.u_error = VOP_SETATTR(vp, &vattr, fp->f_cred);
	}
}

/*
 * Common routine for modifying attributes
 * of named files.
 */
namesetattr(fnamep, followlink, vap)
	char *fnamep;
	enum symfollow followlink;
	struct vattr *vap;
{
	struct vnode *vp;
	register int error;

	error =
	    lookupname(fnamep, UIOSEG_USER, followlink,
		 (struct vnode **)0, &vp);
	if (error)
		return(error);	
	if(vp->v_vfsp->vfs_flag & VFS_RDONLY)
		error = EROFS;
	else
		error = VOP_SETATTR(vp, vap, u.u_cred);
	VN_RELE(vp);
	return(error);
}

/*
 * Common routine for modifying attributes
 * of file referenced by descriptor.
 */
fdsetattr(fd, vap)
	int fd;
	struct vattr *vap;
{
	struct file *fp;
	register struct vnode *vp;
	register int error;

	error = getvnodefp(fd, &fp);
	if (error == 0) {
		vp = (struct vnode *)fp->f_data;
		if(vp->v_vfsp->vfs_flag & VFS_RDONLY)
			return(EROFS);
		error = VOP_SETATTR(vp, vap, fp->f_cred);
	}
	return(error);
}

/*
 * Flush output pending for file.
 */
fsync(uap)
	struct a {
		int	fd;
	} *uap;
{
	struct file *fp;

	u.u_error = getvnodefp(uap->fd, &fp);
	if (u.u_error == 0)
		u.u_error = VOP_FSYNC((struct vnode *)fp->f_data, fp->f_cred);
}

/*
 * Set file creation mask.
 */
umask(uap)
	register struct a {
		int mask;
	} *uap;
{
	u.u_r.r_val1 = u.u_cmask;
	u.u_cmask = uap->mask & 07777;
}

/*
 * Revoke access the current tty by all processes.
 * Used only by the super-user in init
 * to give ``clean'' terminals at login.
 */
vhangup()
{

	if (!suser())
		return;
	if (u.u_ttyp == NULL)
		return;
	forceclose(u.u_ttyd);
	if ((u.u_ttyp->t_state) & TS_ISOPEN)
		gsignal(u.u_ttyp->t_pgrp, SIGHUP);
}

forceclose(dev)
	dev_t dev;
{
	register struct file *fp;
	register struct vnode *vp;

	for (fp = file; fp < fileNFILE; fp++) {
		if (fp->f_count == 0)
			continue;
		if (fp->f_type != DTYPE_VNODE)
			continue;
		vp = (struct vnode *)fp->f_data;
		if (vp == 0)
			continue;
		if (vp->v_type != VCHR)
			continue;
		if (vp->v_rdev != dev)
			continue;
		fp->f_flag &= ~(FREAD|FWRITE);
	}
}

/*
 * Get the file structure entry for the file descrpitor, but make sure
 * its a vnode.
 */
int
getvnodefp(fd, fpp)
	int fd;
	struct file **fpp;
{
	register struct file *fp;

	fp = getf(fd);
	if (fp == (struct file *)0)
		return(EBADF);
	if (fp->f_type != DTYPE_VNODE)
		return(EINVAL);
	*fpp = fp;
	return(0);
}
