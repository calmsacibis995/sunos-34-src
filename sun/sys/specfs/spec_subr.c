#ifndef lint
static  char sccsid[] = "@(#)spec_subr.c 1.1 86/09/25 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */


#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../specfs/snode.h"

extern struct vnodeops spec_vnodeops;
struct snode *sfind();

/*
 * Return a shadow special vnode for the given dev.
 * If no snode exists for this dev create one and put it
 * in a table hashed by dev,realvp.  If the snode for
 * this dev is already in the table return it (ref count is
 * incremented by sfind.  The snode will be flushed from the
 * table when spec_inactive calls sunsave.
 */
struct vnode *
specvp(vp, dev)
	struct vnode *vp;
	dev_t dev;
{
	register struct snode *sp;
	extern struct snode *fifosp();
	struct vattr va;

	if ((sp = sfind(dev, vp)) == NULL) {
		if (vp->v_type == VFIFO) {
			sp = fifosp(vp);
		} else {
			sp = (struct snode *)kmem_alloc((u_int)sizeof (*sp));
			bzero((caddr_t)sp, sizeof (*sp));
			STOV(sp)->v_op = &spec_vnodeops;

			/* init the times in the snode to those in the vnode */
			if (!VOP_GETATTR(vp, &va, u.u_cred)) {
				sp->s_atime = va.va_atime;
				sp->s_mtime = va.va_mtime;
				sp->s_ctime = va.va_ctime;
			}
		}
		VN_HOLD(vp);
		STOV(sp)->v_type = vp->v_type;
		if (vp->v_type == VBLK) {
			sp->s_bdevvp = bdevvp(dev);
		}
		sp->s_realvp = vp;
		sp->s_dev = dev;
		STOV(sp)->v_rdev = dev;
		STOV(sp)->v_count = 1;
		STOV(sp)->v_data = (caddr_t)sp;
		STOV(sp)->v_vfsp = vp->v_vfsp;
		ssave(sp);
	}
	return (STOV(sp));
}

/*
 * Snode lookup stuff.
 * These routines maintain a table of snodes hashed by dev so
 * that the snode for an dev can be found if it already exists.
 * NOTE: STABLESIZE must be a power of 2 for stablehash to work!
 */

#define	STABLESIZE	16
#define	stablehash(dev)	((major(dev) + minor(dev)) & (STABLESIZE-1))

static
struct snode *stable[STABLESIZE];

/*
 * Put a snode in the table
 */
static
ssave(sp)
	struct snode *sp;
{

	sp->s_next = stable[stablehash(sp->s_dev)];
	stable[stablehash(sp->s_dev)] = sp;
}

/*
 * Remove a snode from the table
 */
sunsave(sp)
	struct snode *sp;
{
	struct snode *st;
	struct snode *stprev = NULL;
	 
	st = stable[stablehash(sp->s_dev)]; 
	while (st != NULL) { 
		if (st == sp) { 
			if (stprev == NULL) {
				stable[stablehash(sp->s_dev)] = st->s_next;
			} else {
				stprev->s_next = st->s_next;
			}
			break;
		}	
		stprev = st;
		st = st->s_next;
	}	
	if (sp->s_realvp) {
		VN_RELE(sp->s_realvp);
		if (sp->s_bdevvp) {
			VN_RELE(sp->s_bdevvp);
		}
		sp->s_realvp = NULL;
	}
}

/*
 * Lookup a snode by dev,vp
 */
static
struct snode *
sfind(dev, vp)
	dev_t dev;
	struct vnode *vp;
{
	register struct snode *st;
	 
	st = stable[stablehash(dev)]; 
	while (st != NULL) { 
		if (st->s_dev == dev && st->s_realvp == vp) { 
			VN_HOLD(STOV(st));
			return (st); 
		}	
		st = st->s_next;
	}	
	return (NULL);
}

/*
 * Mark the accessed, updated, or changed times in an snode
 * with the current (unique) time
 */
smark(sp, flag)
	register struct snode *sp;
	register int flag;
{
	struct timeval ut;

	uniqtime(&ut);
	sp->s_flag |= flag;
	if (flag & SACC)
		sp->s_atime = ut;
	if (flag & SUPD)
		sp->s_mtime = ut;
	if (flag & SCHG) {
		sp->s_ctime = ut;
	}
}
