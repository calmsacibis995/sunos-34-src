/*      @(#)rnode.h 1.1 86/09/25 SMI      */

#ifndef __RNODE_HEADER__
#define __RNODE_HEADER__

/*
 * Remote file information structure.
 * The rnode is the "inode" for remote files.  It contains
 * all the information necessary to handle remote file on the
 * client side.
 */
struct rnode {
	struct rnode	*r_next;	/* active rnode list */
	struct vnode	r_vnode;	/* vnode for remote file */
	fhandle_t	r_fh;		/* file handle */
	u_short		r_flags;	/* flags, see below */
	short		r_error;	/* async write error */
	daddr_t		r_lastr;	/* last block read (read-ahead) */
	u_long		r_size;		/* file size in bytes */
	struct ucred	*r_cred;	/* current credentials */
	struct ucred	*r_unlcred;	/* unlinked credentials */
	char		*r_unlname;	/* unlinked file name */
	struct vnode	*r_unldvp;	/* parent dir of unlinked file */
	struct nfsfattr	r_nfsattr;	/* cached nfs attributes */
	struct timeval	r_nfsattrtime;	/* time attributed cached */
};

/*
 * Flags
 */
#define	RLOCKED		0x01		/* rnode is in use */
#define	RWANT		0x02		/* someone wants a wakeup */
#define	RATTRVALID	0x04		/* Attributes in the rnode are valid */
#define	REOF		0x08		/* EOF encountered on read */
#define	RDIRTY		0x10		/* dirty buffers may be in buf cache */
#define RNOCACHE	0x20		/* don't cache read and write blocks */

/*
 * Convert between vnode and rnode
 */
#define	rtov(rp)	(&(rp)->r_vnode)
#define	vtor(vp)	((struct rnode *)((vp)->v_data))
#define	vtofh(vp)	(&(vtor(vp)->r_fh))
#define	rtofh(rp)	(&(rp)->r_fh)

/*
 * Lock and unlock rnodes
 */
#define rlock(rp) { \
        while ((rp)->r_flags & RLOCKED) { \
                (rp)->r_flags |= RWANT; \
                (void) sleep((caddr_t)(rp), PINOD); \
        } \
        (rp)->r_flags |= RLOCKED; \
}

#define runlock(rp) { \
        (rp)->r_flags &= ~RLOCKED; \
        if ((rp)->r_flags&RWANT) { \
                (rp)->r_flags &= ~RWANT; \
                wakeup((caddr_t)(rp)); \
        } \
}

#endif !__RNODE_HEADER__
