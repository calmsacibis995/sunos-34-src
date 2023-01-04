#ifndef lint
static	char sccsid[] = "@(#)win_syscall.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * SunWindows kernel equivalents of selected user process level calls,
 * i.e, open and read.
 */

#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/uio.h"
#include "../h/file.h"
#include "../h/vnode.h"

/*
 * The kernel_internal_read_workaround flag gets around a problem with kernel
 * internal reads.  The problem is that the read path assumes a user process
 * read and therefore accesses the u structure (e.g., rwip.ufs_vnodeops.c).
 * This messes up the return values from real user level reads so that
 * errno and read(2)'s return value was not set reliably.  The workaround is
 * to save and restore u's read related fields.
 */
int	kernel_internal_read_workaround = 1;

/*
 * Open device for kernel use that user has already opened.
 */
int
kern_openfd(fd, fpp, flag)
	int	fd;
	struct	file **fpp;
	int	flag;
{
	register struct file *fp;
	register int	err = 0;

	*fpp = NULL;
	fp = getf(fd);
	if (fp == 0) {
		printf("getf fp == 0\n");
		return (EINVAL);
	}
	err = VOP_OPEN((struct vnode **)&fp->f_data, flag, u.u_cred);
	if (err) {
		printf("VOP_OPEN err %d\n", err);
		return (err);
	}
	*fpp = fp;
	fp->f_count++;
	return (0);
}

int
kern_read(fp, buf, len_ptr)
	struct	file *fp;
	caddr_t	buf;
	int	*len_ptr;
{
	struct uio auio;
	struct iovec aiov;
	int error;

	aiov.iov_base = buf;
	aiov.iov_len = *len_ptr;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_seg = UIOSEG_KERNEL;
	auio.uio_resid = *len_ptr;
	auio.uio_offset = fp->f_offset;
	if (kernel_internal_read_workaround) {
		int u_error_save, u_val1_save, u_val2_save;

		u_error_save = u.u_error;
		u_val1_save = u.u_r.r_val1;
		u_val2_save = u.u_r.r_val2;
		error = (*fp->f_ops->fo_rw)(fp, UIO_READ, &auio);
		u.u_error = u_error_save;
		u.u_r.r_val1 = u_val1_save;
		u.u_r.r_val2 = u_val2_save;
	} else
		error = (*fp->f_ops->fo_rw)(fp, UIO_READ, &auio);
	*len_ptr -= auio.uio_resid;
	return (error);
}


