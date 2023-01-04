/*      @(#)snode.h 1.1 86/09/25 SMI      */
 
/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */


/*
 * The SNODE represents a special file in any filesystem. There is
 * one snode for each active special file. Filesystems which support
 * special files use specvp(vp, dev) to convert a normal vnode to a
 * special vnode in the ops create, mkdir, and lookup.
 */


struct snode {
	struct	snode *s_next;		/* must be first */
	struct	vnode s_vnode;		/* vnode associated with this snode */
	struct	vnode *s_realvp;	/* the vnode for the filesystem entry */
	struct	vnode *s_bdevvp;	/* blk device vnode (shared with fs) */
	u_short	s_flag;			/* flags, see below */
	dev_t	s_dev;			/* device the snode represents */
	daddr_t	s_lastr;		/* last read (read-ahead) */
	struct timeval  s_atime;	/* time of last access */
	struct timeval  s_mtime;	/* time of last modification */
	struct timeval  s_ctime;	/* time of last attributes change */
	caddr_t	s_private;		/* private data struct */
};

/* flags */
#define SLOCKED		0x01		/* snode is locked */
#define SUPD		0x02		/* update device access time */
#define SACC		0x04		/* update device modification time */
#define SWANT		0x10		/* some process waiting on lock */
#define SCHG		0x40		/* update device change time */

/*
 * Convert between vnode and snode
 */
#define	VTOS(vp)	((struct snode *)((vp)->v_data))
#define	STOV(sp)	(&(sp)->s_vnode)

/*
 * Lock and unlock snodes.
 */
#define SNLOCK(sp) { \
	while ((sp)->s_flag & SLOCKED) { \
		(sp)->s_flag |= SWANT; \
		(void) sleep((caddr_t)(sp), PINOD); \
	} \
	(sp)->s_flag |= SLOCKED; \
}

#define SNUNLOCK(sp) { \
	(sp)->s_flag &= ~SLOCKED; \
	if ((sp)->s_flag & SWANT) { \
		(sp)->s_flag &= ~SWANT; \
		wakeup((caddr_t)(sp)); \
	} \
}

/*
 * construct a spec vnode for a given device
 */
struct vnode *specvp();
