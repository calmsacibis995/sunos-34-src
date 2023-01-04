/*	@(#)vm_sw.c 1.1 86/09/25 SMI; from UCB 4.18 83/05/18	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/user.h"
#include "../h/vnode.h"
#include "../h/map.h"
#include "../h/uio.h"
#include "../h/file.h"

struct	buf rswbuf;
/*
 * Indirect driver for multi-controller paging.
 */
swstrategy(bp)
	register struct buf *bp;
{
	int sz, off, seg;
	dev_t dev;
	struct vnode *vp;

#ifdef GENERIC
	/*
	 * A mini-root gets copied into the front of the swap
	 * and we run over top of the swap area just long
	 * enough for us to do a mkfs and restor of the real
	 * root (sure beats rewriting standalone restor).
	 *
	 * Why stop there?  Setup is now going to run window  
	 * system or curses programs from the mini-root.
	 */
#define	MINIROOTSIZE	9216	/* 4.5 Meg */
	if (rootdev == dumpdev)
		bp->b_blkno += MINIROOTSIZE;
#endif
	sz = howmany(bp->b_bcount, DEV_BSIZE);
	if (bp->b_blkno+sz > nswap) {
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	if (nswdev > 1) {
		off = bp->b_blkno % dmmax;
		if (off+sz > dmmax) {
			bp->b_flags |= B_ERROR;
			iodone(bp);
			return;
		}
		seg = bp->b_blkno / dmmax;
		dev = swdevt[seg % nswdev].sw_dev;
		vp = swdevt[seg % nswdev].sw_vp;
		seg /= nswdev;
		bp->b_blkno = seg*dmmax + off;
	} else {
		dev = swdevt[0].sw_dev;
		vp = swdevt[0].sw_vp;
	}
	bp->b_dev = dev;
	bsetvp(bp, vp);
	if (dev == 0)
		panic("swstrategy");
	VOP_STRATEGY(bp);
}

swread(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return (physio(swstrategy, &rswbuf, dev, B_READ, minphys, uio));
}

swwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return (physio(swstrategy, &rswbuf, dev, B_WRITE, minphys, uio));
}

/*
 * System call swapon(name) enables swapping on device name,
 * which must be in the swdevsw.  Return EBUSY
 * if already swapping on this device.
 */
swapon()
{
	register struct a {
		char	*name;
	} *uap;
	struct vnode *vp;
	dev_t dev;
	register struct swdevt *sp;

	if (!suser())
		return;
	uap = (struct a *)u.u_ap;
	u.u_error =
	    lookupname(uap->name, UIOSEG_USER, FOLLOW_LINK,
		(struct vnode **)0, &vp);
	if (u.u_error)
		return;
	if (vp->v_type != VBLK) {
		u.u_error = ENOTBLK;
		VN_RELE(vp);
		return;
	}
	dev = (dev_t)vp->v_rdev;
	VN_RELE(vp);
	if (major(dev) >= nblkdev) {
		u.u_error = ENXIO;
		return;
	}
	/*
	 * Search starting at second table entry,
	 * since first (primary swap area) is freed at boot.
	 */
	for (sp = &swdevt[1]; sp->sw_dev; sp++)
		if (sp->sw_dev == dev) {
			if (sp->sw_freed) {
				u.u_error = EBUSY;
				return;
			}
			swfree(sp - swdevt);
			return;
		}
	u.u_error = ENODEV;
}

/*
 * Swfree(index) frees the index'th portion of the swap map.
 * Each of the nswdev devices provides 1/nswdev'th of the swap
 * space, which is laid out with blocks of dmmax pages circularly
 * among the devices.
 */
swfree(index)
	int index;
{
	register swblk_t vsbase;
	register long blk;
	struct vnode *vp;
	register swblk_t dvbase;
	register int nblks;

	vp = swdevt[index].sw_vp;
	if (vp == NULL) {
		panic("swfree: null sw_vp");
	}
	VOP_OPEN(&vp, FREAD|FWRITE, u.u_cred);
	swdevt[index].sw_freed = 1;
	nblks = swdevt[index].sw_nblks;
	for (dvbase = 0; dvbase < nblks; dvbase += dmmax) {
		blk = nblks - dvbase;
		if ((vsbase = index*dmmax + dvbase*nswdev) >= nswap)
			panic("swfree");
		if (blk > dmmax)
			blk = dmmax;
		if (vsbase == 0) {
			/*
			 * Can't free a block starting at 0 in the swapmap
			 * but need some space for argmap so use 1/2 this
			 * hunk which needs special treatment anyways.
			 */
			argdev = swdevt[0].sw_dev;
			VN_RELE(argdev_vp);
			argdev_vp = bdevvp(argdev);
			rminit(argmap, (long)(blk/2-ctod(CLSIZE)),
			    (long)ctod(CLSIZE), "argmap", ARGMAPSIZE);
			/*
			 * First of all chunks... initialize the swapmap
			 * the second half of the hunk.
			 */
			rminit(swapmap, (long)(blk/2), (long)(blk/2),
			    "swap", nswapmap);
		} else
			rmfree(swapmap, blk, vsbase);
	}
}
