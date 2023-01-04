#ifndef lint
static	char sccsid[] = "@(#)pc_vfsops.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/conf.h"
#undef NFS
#include "../h/mount.h"
#include "../pcfs/pc_label.h"
#include "../pcfs/pc_fs.h"
#include "../pcfs/pc_dir.h"
#include "../pcfs/pc_node.h"

/*
 * pcfs vfs operations.
 */
extern int pcfs_mount();
extern int pcfs_unmount();
extern int pcfs_root();
extern int pcfs_statfs();
extern int pcfs_sync();
extern int pcfs_invalop();

struct vfsops pcfs_vfsops = {
	pcfs_mount,
	pcfs_unmount,
	pcfs_root,
	pcfs_statfs,
	pcfs_sync,
	pcfs_invalop,	/* vfs_getvp */
};

/*
 * Default device to mount on.
 */
/* extern dev_t pcfs_rootdev; */

int pcfsdebuglevel = 0;


/*
 * pc_mount system call
 */
pcfs_mount(vfsp, path, data)
	struct vfs *vfsp;
	char *path;
	caddr_t data;
{
	int error;
	dev_t dev;
	struct vnode *vp;
	static int init = 0;
	struct pc_args args;

PCFSDEBUG(2)
printf("pcfs_mount\n");
        /*
         * Get arguments
         */
        error = copyin(data, (caddr_t)&args, sizeof (struct pc_args));
        if (error) {
                return (error);
        }

	if (!init) {
		init++;
		pc_init();
	}
	/*
	 * Get the device to be mounted on
	 */
	error = lookupname(args.fspec, UIOSEG_USER, FOLLOW_LINK,
	    (struct vnode **)0, &vp);
	if (error)
		return (error);
	if (vp->v_type != VBLK) {
		VN_RELE(vp);
		return (ENOTBLK);
	}
	dev = vp->v_rdev;
	VN_RELE(vp);
	if (major(dev) >= nblkdev) {
		return (ENXIO);
	}
	/*
	 * Mount the filesystem
	 */
	error = pcfs_mountfs(dev, path, vfsp);
	return (error);
}

#ifdef notneeded
/*
 * Called by vfs_mountroot when pcfs is going to be mounted as root
 */
pcfs_mountroot()
{

	panic("pcfs_mountroot");
}
#endif

/*ARGSUSED*/
int
pcfs_mountfs(dev, path, vfsp)
	dev_t dev;
        char *path;
	struct vfs *vfsp;
{
	register struct pcfs *fsp;

PCFSDEBUG(2)
printf("pcfs_mountfs\n");
	fsp = (struct pcfs *)kmem_alloc((u_int)sizeof(struct pcfs));
	bzero((caddr_t)fsp, sizeof(struct pcfs));
	fsp->pcfs_devvp = bdevvp(dev);
	vfsp->vfs_bsize = PC_SECSIZE;
	fsp->pcfs_vfs = vfsp;
	vfsp->vfs_data = (caddr_t)fsp;
	return (0);
}

/*
 * vfs operations
 */

pcfs_unmount(vfsp)
	register struct vfs *vfsp;
{
	register struct pcfs *fsp;

PCFSDEBUG(2)
printf("pc_unmount\n");
	fsp = VFSTOPCFS(vfsp);
	if (fsp->pcfs_nrefs)
		return (EBUSY);
	binval(fsp->pcfs_devvp);
	if (fsp->pcfs_fatp != (u_char *)0)
		pc_invalfat(fsp);
	kmem_free((caddr_t)fsp, (u_int)sizeof(struct pcfs));
	return (0);
}

/*
 * find root of pcfs
 */
int
pcfs_root(vfsp, vpp)
	struct vfs *vfsp;
	struct vnode **vpp;
{
	register struct pcfs *fsp;
	struct pcnode *pcp;

	fsp = VFSTOPCFS(vfsp);
	PC_LOCKFS(fsp);
	pcp = pc_getnode(fsp, (daddr_t)0, 0, (struct pcdir *)0);
PCFSDEBUG(3)
printf("pcfs_root(0x%x) pcp= 0x%x\n", vfsp, pcp);
	PC_UNLOCKFS(fsp);
	*vpp = PCTOV(pcp);
	return (0);
}

/*
 * Get file system statistics.
 */
int
pcfs_statfs(vfsp, sbp)
	register struct vfs *vfsp;
	struct statfs *sbp;
{
	register struct pcfs *fsp;
	int error;

	fsp = VFSTOPCFS(vfsp);
	error = pc_getfat(fsp);
	if (error)
		return (error);
	sbp->f_bsize = fsp->pcfs_clsize;
	sbp->f_blocks = fsp->pcfs_ncluster;
	sbp->f_bavail = sbp->f_bfree = pc_freeclusters(fsp);
	bzero((caddr_t)&sbp->f_fsid, sizeof(fsid_t));
	sbp->f_fsid.val[0] = (long)fsp->pcfs_devvp->v_rdev;
	return (0);
}

/*
 * Flush any pending I/O.
 */
int
pcfs_sync(vfsp)
	register struct vfs *vfsp;
{

PCFSDEBUG(3)
printf("pcfs_sync\n");
	bflush((struct vnode *) 0);
	pc_syncfat(VFSTOPCFS(vfsp));
	return (0);
}

int
pc_lockfs(fsp)
	register struct pcfs *fsp;
{
	int error;

	PC_LOCKFS(fsp);
PCFSDEBUG(6)
printf("pcfs_lockfs(0x%x) locked\n", fsp);
	error = pc_getfat(fsp);
	if (error) {
PCFSDEBUG(6)
printf("pcfs_lockfs(0x%x) getfat error\n", fsp);
		pc_unlockfs(fsp);
	}
	return (error);
}

void
pc_unlockfs(fsp)
	register struct pcfs *fsp;
{

	if (!(fsp->pcfs_flags & PCFS_LOCKED))
		panic("pc_unlockfs");
PCFSDEBUG(6)
printf("pcfs_unlockfs(0x%x)\n", fsp);
	PC_UNLOCKFS(fsp);
}

/*
 * Get the file allocation table.
 * If there is an old one, invalidate it.
 */
int
pc_getfat(fsp)
	register struct pcfs *fsp;
{
	register struct buf *bp = 0;
	register struct buf *tp = 0;
	register u_char *fatp;
	dev_t dev;
	int error;

PCFSDEBUG(2)
printf("pc_getfat\n");
	if (fsp->pcfs_fatp) {
		/*
		 * There is a fat in core.
		 * If there are open file pcnodes or we have modified it or
		 * it hasn't timed out yet use the in core fat.
		 * Otherwise invalidate it and get a new one
		 */
		if (fsp->pcfs_frefs ||
		    (fsp->pcfs_flags & PCFS_FATMOD) ||
		    timercmp(&time, &fsp->pcfs_fattime, <)) {
			return  (0);
		} else {
			pc_invalfat(fsp);
		}
	}
	dev = fsp->pcfs_devvp->v_rdev;
	/*
	 * Open block device mounted on.
	 */
	error =
	    (*bdevsw[major(dev)].d_open)(dev,
		(PCFSTOVFS(fsp)->vfs_flag & VFS_RDONLY) ?
		    FREAD : FREAD|FWRITE);
	if (error)
		/* return (error); */
		return (EIO);
	/*
	 * Get fat and check it for validity
	 */
	tp = bread(fsp->pcfs_devvp, (daddr_t)PC_FATBLOCK,
		PC_MAXFATSEC * PC_SECSIZE);
	if (tp->b_flags & (B_ERROR | B_INVAL)) {
		if (tp->b_flags & B_ERROR)
			pc_diskchanged(fsp);
		brelse(tp);
		return (EIO);
	}
	tp->b_flags |= B_INVAL;
	fatp = (u_char *)tp->b_un.b_addr;
	if (fatp[1] != 0xFF || fatp[2] != 0xFF) {
		error = EINVAL;
		goto out;
	}
	switch (fatp[0]) {

	case SS8SPT:
		fsp->pcfs_spcl = 1;
		fsp->pcfs_fatsec = 1;
		fsp->pcfs_spt = 8;
		fsp->pcfs_rdirsec = 4;
		fsp->pcfs_ncluster = 313;
		break;

	case DS8SPT:
		fsp->pcfs_spcl = 2;
		fsp->pcfs_fatsec = 1;
		fsp->pcfs_spt = 8;
		fsp->pcfs_rdirsec = 7;
		fsp->pcfs_ncluster = 315;
		break;

	case SS9SPT:
		fsp->pcfs_spcl = 1;
		fsp->pcfs_fatsec = 2;
		fsp->pcfs_spt = 9;
		fsp->pcfs_rdirsec = 4;
		fsp->pcfs_ncluster = 351;
		break;

	case DS9SPT:
		fsp->pcfs_spcl = 2;
		fsp->pcfs_fatsec = 2;
		fsp->pcfs_spt = 9;
		fsp->pcfs_rdirsec = 7;
		fsp->pcfs_ncluster = 354;
		break;

	default:
		error = EINVAL;
		goto out;
	}
	fsp->pcfs_clsize = fsp->pcfs_spcl * PC_SECSIZE;
	fsp->pcfs_rdirstart = PC_FATBLOCK + (2 * fsp->pcfs_fatsec);
	fsp->pcfs_datastart = fsp->pcfs_rdirstart + fsp->pcfs_rdirsec;
	/*
	 * Copy the fat into a buffer in its native size.
	 */
	if (fsp->pcfs_fatsec != PC_MAXFATSEC) {
		bp = geteblk(fsp->pcfs_fatsec * PC_SECSIZE);
		bcopy((caddr_t)tp->b_un.b_addr, (caddr_t)bp->b_un.b_addr,
		   (u_int)(fsp->pcfs_fatsec * PC_SECSIZE));
		brelse(tp);
	} else {
		bp = tp;
	}
	tp = 0;
	fsp->pcfs_fatbp = bp;
	fsp->pcfs_fatp = (u_char *)bp->b_un.b_addr;
	fsp->pcfs_fattime = time;
	fsp->pcfs_fattime.tv_sec += PCFS_DISKTIMEOUT;
PCFSDEBUG(2)
printf("read in fat\n");
	return (0);
out:
	if (bp)
		brelse(bp);
	if (tp)
		brelse(tp);
	return (error);
}

void
pc_syncfat(fsp)
	register struct pcfs *fsp;
{
	register struct buf *bp;
	register int fatsize;

PCFSDEBUG(2)
printf("pcfs_syncfat\n");
	if ((fsp->pcfs_fatp == (u_char *)0) || !(fsp->pcfs_flags & PCFS_FATMOD))
		return;
	/*
	 * write out fat
	 */
	fsp->pcfs_flags &= ~PCFS_FATMOD;
	fsp->pcfs_fattime = time;
	fsp->pcfs_fattime.tv_sec += PCFS_DISKTIMEOUT;
	fatsize = fsp->pcfs_fatbp->b_bcount;
	bp = getblk(fsp->pcfs_devvp, (daddr_t)PC_FATBLOCK, fatsize);
	bp->b_flags |= B_NOCACHE;		/* don't cache */
	bcopy((caddr_t)fsp->pcfs_fatp, bp->b_un.b_addr, (u_int)fatsize);
	bwrite(bp);
	/*
	 * write out alternate fat
	 */
	bp = getblk(fsp->pcfs_devvp,
		(daddr_t)(PC_FATBLOCK + fsp->pcfs_fatsec), fatsize);
	bp->b_flags |= B_NOCACHE;		/* don't cache */
	bcopy((caddr_t)fsp->pcfs_fatp, bp->b_un.b_addr, (u_int)fatsize);
	bawrite(bp);
PCFSDEBUG(2)
printf("wrote out fat\n");
}

void
pc_invalfat(fsp)
	register struct pcfs *fsp;
{
	dev_t dev;

PCFSDEBUG(2)
printf("pc_invalfat\n");
	if (fsp->pcfs_fatp == (u_char *)0 || fsp->pcfs_frefs)
		panic("pc_invalfat");
	/*
	 * Release old fat, invalidate all the blocks associated
	 * with the device.
	 */
	brelse(fsp->pcfs_fatbp);
	fsp->pcfs_fatp = (u_char *)0;
	dev = fsp->pcfs_devvp->v_rdev;
	/*
	 * close mounted device
	 */
	(void) (*bdevsw[major(dev)].d_close)(dev,
	    (PCFSTOVFS(fsp)->vfs_flag & VFS_RDONLY) ?
		 FREAD : FREAD|FWRITE);
	binval(fsp->pcfs_devvp);
}

pc_badfs(fsp)
	struct pcfs *fsp;
{

	uprintf("corrupted PC file system on dev 0x%x\n",
		fsp->pcfs_devvp->v_rdev);
}
