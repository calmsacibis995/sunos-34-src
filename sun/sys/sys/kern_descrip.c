/*	@(#)kern_descrip.c 1.1 86/09/25 SMI; from UCB 6.3 83/11/18	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/stat.h"

#include "../h/ioctl.h"

/*
 * Descriptor management.
 */

/*
 * TODO:
 *	increase NOFILE
 *	eliminate u.u_error side effects
 */

/*
 * System calls on descriptors.
 */
getdtablesize()
{

	u.u_r.r_val1 = NOFILE;
}

getdopt()
{

}

setdopt()
{

}

dup()
{
	register struct a {
		int	i;
	} *uap = (struct a *) u.u_ap;
	struct file *fp;
	int j;

	if (uap->i &~ 077) { uap->i &= 077; dup2(); return; }	/* XXX */

	fp = getf(uap->i);
	if (fp == 0)
		return;
	j = ufalloc(0);
	if (j < 0)
		return;
	dupit(j, fp, u.u_pofile[uap->i]);
}

dup2()
{
	register struct a {
		int	i, j;
	} *uap = (struct a *) u.u_ap;
	register struct file *fp;

	fp = getf(uap->i);
	if (fp == 0)
		return;
	if (uap->j < 0 || uap->j >= NOFILE) {
		u.u_error = EBADF;
		return;
	}
	u.u_r.r_val1 = uap->j;
	if (uap->i == uap->j)
		return;
	if (u.u_ofile[uap->j]) {
		/* Release all System-V style record locks, if any */
		(void) vno_lockrelease(u.u_ofile[uap->j]);	/* errors? */

		if (u.u_pofile[uap->j] & UF_MAPPED)
			munmapfd(uap->j);
		closef(u.u_ofile[uap->j]);
		/*
		 * Even if an error occurred when calling the close routine
		 * for the vnode or the device, the file table entry has
		 * had its reference count decremented anyway.  As such,
		 * the descriptor is closed, so there's not much point
		 * in worrying about errors; we might as well pretend
		 * the "close" succeeded.
		 */
		u.u_error = 0;
	}
	dupit(uap->j, fp, u.u_pofile[uap->i]);
}

dupit(fd, fp, flags)
	int fd;
	register struct file *fp;
	register int flags;
{

	u.u_ofile[fd] = fp;
	u.u_pofile[fd] = flags &~ UF_EXCLOSE;
	fp->f_count++;
}

/*
 * The file control system call.
 */
fcntl()
{
	register struct file *fp;
	register struct a {
		int	fdes;
		int	cmd;
		int	arg;
	} *uap;
	register i;
	register char *pop;
	struct flock ld;
	register int oldwhence;

	uap = (struct a *)u.u_ap;
	fp = getf(uap->fdes);
	if (fp == NULL)
		return;
	pop = &u.u_pofile[uap->fdes];
	switch(uap->cmd) {
	case F_DUPFD:
		i = uap->arg;
		if (i < 0 || i >= NOFILE) {
			u.u_error = EINVAL;
			return;
		}
		if ((i = ufalloc(i)) < 0)
			return;
		dupit(i, fp, *pop);
		break;

	case F_GETFD:
		u.u_r.r_val1 = *pop & 1;
		break;

	case F_SETFD:
		*pop = (*pop &~ 1) | (uap->arg & 1);
		break;

	case F_GETFL:
		u.u_r.r_val1 = fp->f_flag+FOPEN;
		break;

	case F_SETFL:
		fp->f_flag &= FCNTLCANT;
		fp->f_flag |= (uap->arg-FOPEN) &~ FCNTLCANT;
		u.u_error = fset(fp, FNDELAY, fp->f_flag & FNDELAY);
		if (u.u_error)
			break;
		u.u_error = fset(fp, FASYNC, fp->f_flag & FASYNC);
		if (u.u_error)
			(void) fset(fp, FNDELAY, 0);
		break;

	case F_GETOWN:
		u.u_error = fgetown(fp, &u.u_r.r_val1);
		break;

	case F_SETOWN:
		u.u_error = fsetown(fp, uap->arg);
		break;

		/* System-V Record-locking (lockf() maps to fcntl()) */
	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:
		/* First off, allow only vnodes here */
		if (fp->f_type != DTYPE_VNODE) {
			u.u_error = EBADF;
			return;
		}
		/* get flock structure from user-land */
		if (copyin((caddr_t) uap->arg, (caddr_t) &ld, sizeof (ld))) {
			return;
		}

		/* *** NOTE ***
		 * The SVID does not say what to return on file access errors!
		 * Here, EBADF is returned, which is compatible with S5R3
		 * and is less confusing than EACCES
		 */
		/* check access permissions */
		if (uap->cmd != F_GETLK) {
			switch (ld.l_type) {
			case F_RDLCK:
				if (!(fp->f_flag & FREAD)) {
					u.u_error = EBADF;
					return;
				}
				break;

			case F_WRLCK:
				if (!(fp->f_flag & FWRITE)) {
					u.u_error = EBADF;
					return;
				}
				break;

			case F_UNLCK:
				break;

			default:
				u.u_error = EINVAL;
				return;
			}
		}
		
		/* convert offset to start of file */
		oldwhence = ld.l_whence;	/* save to renormalize later */
		if (u.u_error = rewhence(&ld, fp, 0))
			return;

		/* convert negative lengths to positive */
		if (ld.l_len < 0) {
			ld.l_start += ld.l_len;		/* adjust start point */
			ld.l_len = -(ld.l_len);		/* absolute value */
		}
	 
		/* check for validity */
		if (ld.l_start < 0) {
			u.u_error = EINVAL;
			return;
		}

		if ((uap->cmd != F_GETLK) && (ld.l_type != F_UNLCK)) {
			/* If any locking is attempted, mark file locked
			 * to force unlock on close.
			 * Also, since the SVID specifies that the FIRST
			 * close releases all locks, mark process to
			 * reduce the search overhead in vno_lockrelease().
			 */
			*pop |= UF_FDLOCK;
			u.u_procp->p_flag |= SLKDONE;
		}

		/*
		 * Dispatch out to vnode layer to do the actual locking.
		 * Then, translate error codes for SVID compatibility
		 */
		switch (u.u_error = VOP_LOCKCTL((struct vnode *)fp->f_data,
		    &ld, uap->cmd, fp->f_cred)) {
		case 0:
			break;		/* continue, if successful */
		case EWOULDBLOCK:
			u.u_error = EACCES;	/* EAGAIN ??? */
			return;
		default:
			return;		/* some other error code */
		}

		/* if F_GETLK, return flock structure to user-land */
		if (uap->cmd == F_GETLK) {
			/* per SVID, change only 'l_type' field if unlocked */
			if (ld.l_type == F_UNLCK) {
				if (copyout((caddr_t) &ld.l_type,
				    (caddr_t)&((struct flock*)uap->arg)->l_type,
				    sizeof (ld.l_type))) {
					return;
				}
			} else {
				if (u.u_error = rewhence(&ld, fp, oldwhence))
					return;
				if (copyout((caddr_t) &ld, (caddr_t) uap->arg,
				    sizeof (ld))) {
					return;
				}
			}
		}
		break;

	default:
		u.u_error = EINVAL;
	}
}

fset(fp, bit, value)
	struct file *fp;
	int bit, value;
{

	if (value)
		fp->f_flag |= bit;
	else
		fp->f_flag &= ~bit;
	return (fioctl(fp, (int)(bit == FNDELAY ? FIONBIO : FIOASYNC),
	    (caddr_t)&value));
}

fgetown(fp, valuep)
	struct file *fp;
	int *valuep;
{
	int error;

	switch (fp->f_type) {

	case DTYPE_SOCKET:
		*valuep = ((struct socket *)fp->f_data)->so_pgrp;
		return (0);

	default:
		error = fioctl(fp, (int)TIOCGPGRP, (caddr_t)valuep);
		*valuep = -*valuep;
		return (error);
	}
}

fsetown(fp, value)
	struct file *fp;
	int value;
{

	if (fp->f_type == DTYPE_SOCKET) {
		((struct socket *)fp->f_data)->so_pgrp = value;
		return (0);
	}
	if (value > 0) {
		struct proc *p = pfind(value);
		if (p == 0)
			return (EINVAL);
		value = p->p_pgrp;
	} else
		value = -value;
	return (fioctl(fp, (int)TIOCSPGRP, (caddr_t)&value));
}

fioctl(fp, cmd, value)
	struct file *fp;
	int cmd;
	caddr_t value;
{

	return ((*fp->f_ops->fo_ioctl)(fp, cmd, value));
}

close()
{
	register struct a {
		int	i;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;
	register u_char *pf;

	fp = getf(uap->i);
	if (fp == 0)
		return;

	/* Release all System-V style record locks, if any */
	(void) vno_lockrelease(fp);	/* WHAT IF error returned? */

	pf = (u_char *)&u.u_pofile[uap->i];
	if (*pf & UF_MAPPED)
		munmapfd(uap->i);
	u.u_ofile[uap->i] = NULL;
	*pf = 0;
	closef(fp);
	/* WHAT IF u.u_error ? */
}

fstat()
{
	register struct file *fp;
	register struct a {
		int	fdes;
		struct	stat *sb;
	} *uap;
	struct stat ub;

	uap = (struct a *)u.u_ap;
	fp = getf(uap->fdes);
	if (fp == 0)
		return;
	switch (fp->f_type) {

	case DTYPE_VNODE:
		u.u_error = vno_stat((struct vnode *)fp->f_data, &ub);
		break;

	case DTYPE_SOCKET:
		u.u_error = soo_stat((struct socket *)fp->f_data, &ub);
		break;

	default:
		panic("fstat");
		/*NOTREACHED*/
	}
	if (u.u_error == 0)
		u.u_error = copyout((caddr_t)&ub, (caddr_t)uap->sb,
		    sizeof (ub));
}

/*
 * Allocate a user file descriptor.
 */
ufalloc(i)
	register int i;
{

	for (; i < NOFILE; i++)
		if (u.u_ofile[i] == NULL) {
			u.u_r.r_val1 = i;
			u.u_pofile[i] = 0;
			return (i);
		}
	u.u_error = EMFILE;
	return (-1);
}

ufavail()
{
	register int i, avail = 0;

	for (i = 0; i < NOFILE; i++)
		if (u.u_ofile[i] == NULL)
			avail++;
	return (avail);
}

struct	file *lastf;
/*
 * Allocate a user file descriptor
 * and a file structure.
 * Initialize the descriptor
 * to point at the file structure.
 */
struct file *
falloc()
{
	register struct file *fp;
	register i;

	i = ufalloc(0);
	if (i < 0)
		return (NULL);
	if (lastf == 0)
		lastf = file;
	for (fp = lastf; fp < fileNFILE; fp++)
		if (fp->f_count == 0)
			goto slot;
	for (fp = file; fp < lastf; fp++)
		if (fp->f_count == 0)
			goto slot;
	tablefull("file");
	u.u_error = ENFILE;
	return (NULL);
slot:
	u.u_ofile[i] = fp;
	fp->f_count = 1;
	fp->f_data = 0;
	fp->f_offset = 0;
	crhold(u.u_cred);
	fp->f_cred = u.u_cred;
	lastf = fp + 1;
	return (fp);
}

/*
 * Convert a user supplied file descriptor into a pointer
 * to a file structure.  Only task is to check range of the descriptor.
 * Critical paths should use the GETF macro.
 */
struct file *
getf(f)
	register int f;
{
	register struct file *fp;

	if ((unsigned)f >= NOFILE || (fp = u.u_ofile[f]) == NULL) {
		u.u_error = EBADF;
		return (NULL);
	}
	return (fp);
}

/*
 * Internal form of close.
 * Decrement reference count on file structure.
 * If last reference not going away, but no more
 * references except in message queues, run a
 * garbage collect.  This would better be done by
 * forcing a gc() to happen sometime soon, rather
 * than running one each time.
 */
closef(fp)
	register struct file *fp;
{

	if (fp == NULL)
		return;
#ifdef STREAMS
	if (fp->f_type == DTYPE_VNODE) {
		register struct vnode *vp;

		vp = (struct vnode *)fp->f_data;
		/*
		 * If it's a stream, must clean up on EVERY close.
		 */
		if (vp->v_type == VCHR && vp->v_stream)
			strclean(vp);
	}
#endif
	if (fp->f_count > 1) {
		fp->f_count--;
		if (fp->f_count == fp->f_msgcount)
			unp_gc();
		return;
	}
	if (fp->f_count < 1)
		panic("closef: count < 1");

	(*fp->f_ops->fo_close)(fp);
	crfree(fp->f_cred);
	fp->f_count = 0;
}

/*
 * Normalize SystemV-style record locks
 */
rewhence(ld, fp, newwhence)
	struct flock *ld;
	struct file *fp;
	int newwhence;
{
	struct vattr va;
	register int error;

	/* if reference to end-of-file, must get current attributes */
	if ((ld->l_whence == 2) || (newwhence == 2)) {
		if (error = VOP_GETATTR((struct vnode *)fp->f_data, &va,
		    u.u_cred))
			return(error);
	}

	/* normalize to start of file */
	switch (ld->l_whence) {
	case 0:
		break;
	case 1:
		ld->l_start += fp->f_offset;
		break;
	case 2:
		ld->l_start += va.va_size;
		break;
	default:
		return(EINVAL);
	}

	/* renormalize to given start point */
	switch (ld->l_whence = newwhence) {
	case 1:
		ld->l_start -= fp->f_offset;
		break;
	case 2:
		ld->l_start -= va.va_size;
		break;
	}
	return(0);
}

/*
 * Apply an advisory lock on a file descriptor.
 */
flock()
{
	register struct a {
		int	fd;
		int	how;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;

	fp = getf(uap->fd);
	if (fp == NULL)
		return;
	if (fp->f_type != DTYPE_VNODE) {
		u.u_error = EOPNOTSUPP;
		return;
	}

	if (uap->how & LOCK_UN) {
		vno_bsd_unlock(fp, FSHLOCK|FEXLOCK);
		return;
	}
	/* check for valid lock type */
	if (uap->how & LOCK_EX)
		uap->how &= ~LOCK_SH;		/* can't have both types */
	else if (!(uap->how & LOCK_SH)) {
		u.u_error = EINVAL;		/* but must have one */
		return;
	}
	u.u_error = vno_bsd_lock(fp, uap->how);
}
