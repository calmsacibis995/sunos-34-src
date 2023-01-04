/*	@(#)file.h 1.1 86/09/25 SMI; from UCB 6.2 83/09/23	*/

#ifndef __FILE_HEADER__
#define __FILE_HEADER__

#ifdef KERNEL
#include "../h/fcntl.h"
#else KERNEL
#include <sys/fcntl.h>
#endif KERNEL

#ifdef KERNEL
/*
 * Descriptor table entry.
 * One for each kernel object.
 */
struct	file {
	int	f_flag;		/* see below */
	short	f_type;		/* descriptor type */
	short	f_count;	/* reference count */
	short	f_msgcount;	/* references from message queue */
	struct	fileops {
		int	(*fo_rw)();
		int	(*fo_ioctl)();
		int	(*fo_select)();
		int	(*fo_close)();
	} *f_ops;
	caddr_t	f_data;		/* ptr to file specific struct (vnode/socket) */
	off_t	f_offset;
	struct	ucred *f_cred;	/* credentials of user who opened file */
};

struct	file *file, *fileNFILE;
int	nfile;
struct	file *getf();
struct	file *falloc();
#endif KERNEL

/*
 * flags - see also fcntl.h
 */
#define	FOPEN		(-1)
#define	FREAD		00001		/* descriptor read/receive'able */
#define	FWRITE		00002		/* descriptor write/send'able */
#define	FMARK		00020		/* mark during gc() */
#define	FDEFER		00040		/* defer for next gc pass */
#define	FSHLOCK		00200		/* shared lock present */
#define	FEXLOCK		00400		/* exclusive lock present */

/* bits to save after open */
#define	FMASK		00113
#define	FCNTLCANT	(FREAD|FWRITE|FMARK|FDEFER|FSHLOCK|FEXLOCK)

/*
 * User definitions.
 */

/*
 * Flock call.
 */
#define	LOCK_SH		1	/* shared lock */
#define	LOCK_EX		2	/* exclusive lock */
#define	LOCK_NB		4	/* don't block when locking */
#define	LOCK_UN		8	/* unlock */

/*
 * Lseek call.
 */
#define	L_SET		0	/* absolute offset */
#define	L_INCR		1	/* relative to current offset */
#define	L_XTND		2	/* relative to end of file */

#ifdef KERNEL
#define	GETF(fp, fd) { \
	if ((unsigned)(fd) >= NOFILE || ((fp) = u.u_ofile[fd]) == NULL) { \
		u.u_error = EBADF; \
		return; \
	} \
}

#define	DTYPE_VNODE	1	/* file */
#define	DTYPE_SOCKET	2	/* communications endpoint */

#endif

#endif !__FILE_HEADER__
