#ifndef lint
static	char sccsid[] = "@(#)pc_node.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "../h/param.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/buf.h"
#include "../h/systm.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../pcfs/pc_label.h"
#include "../pcfs/pc_fs.h"
#include "../pcfs/pc_dir.h"
#include "../pcfs/pc_node.h"

#define NPCHASH	1

#if NPCHASH == 1
#define PCFHASH(FSP, BN, O)	0
#define PCDHASH(FSP, SC)	0
#else
#define PCFHASH(FSP, BN, O)	(((unsigned)FSP + BN + O) % NPCHASH)
#define PCDHASH(FSP, SC)	(((unsigned)FSP + SC) % NPCHASH)
#endif

struct pchead {
	struct pcnode *pch_forw;
	struct pcnode *pch_back;
};

struct pchead pcfhead[NPCHASH];
struct pchead pcdhead[NPCHASH];

/*
 * fake entry for root directory, since this does not have a parent
 * pointing to it.
 */
static struct pcdir rootentry = {
	"",
	"",
	PCA_DIR,
	"",
	{0, 0},
	0,
	0
};

static int pc_getentryblock();

void
pc_init()
{
	register struct pchead *hdp, *hfp;
	register int i;

PCFSDEBUG(2)
printf("pc_init\n");
	hdp = &pcdhead[0];
	hfp = &pcfhead[0];
	for (i = 0; i < NPCHASH; i++) {
		hdp->pch_forw =  (struct pcnode *)hdp;
		hdp->pch_back =  (struct pcnode *)hdp;
		hfp->pch_forw =  (struct pcnode *)hfp;
		hfp->pch_back =  (struct pcnode *)hfp;
	}
}

struct pcnode *
pc_getnode(fsp, blkno, offset, ep)
	register struct pcfs *fsp;	/* filsystem for node */
	register daddr_t blkno;		/* phys block no of dir entry */
	register int offset;		/* offset of dir entry in block */
	register struct pcdir *ep;	/* node dir entry */
{
	register struct pcnode *pcp;
	register struct pchead *hp;
	register struct vnode *vp;
	register short scluster;

	if (!(fsp->pcfs_flags & PCFS_LOCKED))
		panic("pc_getnode");
	if (ep == (struct pcdir *)0) {
		ep = &rootentry;
		scluster = 0;
	} else {
		scluster = ltohs(ep->pcd_scluster);
	}
	/*
	 * First look for active nodes.
	 * File nodes are identified by the location (blkno, offset) of
	 * its directory entry.
	 * Directory nodes are identified by the starting cluster number
	 * for the entries.
	 */
	if (ep->pcd_attr & PCA_DIR) {
PCFSDEBUG(6)
printf("pc_getnode: looking for directory node, scluster=%d\n", scluster);
		hp = &pcdhead[PCDHASH(fsp, scluster)];
		for (pcp = hp->pch_forw;
		     pcp != (struct pcnode *)hp; pcp = pcp->pc_forw) {
			if ((fsp == VFSTOPCFS(pcp->pc_vn.v_vfsp)) &&
			    (scluster == pcp->pc_scluster)) {
PCFSDEBUG(6)
printf("pc_getnode: found old directory node, pcp=0x%x\n", pcp);
				VN_HOLD(PCTOV(pcp));
				return (pcp);
			}
		}
	} else {
PCFSDEBUG(6)
printf("pc_getnode: looking for file node, blkno=%d, off=%d\n", blkno, offset);
		hp = &pcfhead[PCFHASH(fsp, blkno, offset)];
		for (pcp = hp->pch_forw;
		    pcp != (struct pcnode *)hp; pcp = pcp->pc_forw) {
			if ((fsp == VFSTOPCFS(pcp->pc_vn.v_vfsp)) &&
			    ((pcp->pc_flags & PC_INVAL) == 0) &&
			    (blkno == pcp->pc_eblkno) && 
			    (offset == pcp->pc_eoffset)) {
PCFSDEBUG(6)
printf("pc_getnode: found old file node, pcp=0x%x\n", pcp);
				VN_HOLD(PCTOV(pcp));
				return (pcp);
			}
		}
	}
	/*
	 * Cannot find node in active list. Allocate memory for a new node
	 * initialize it, and put it on the active list.
	 */
	pcp =
	    (struct pcnode *)kmem_alloc((u_int)sizeof(struct pcnode));
	bzero((caddr_t)pcp, sizeof(struct pcnode));
PCFSDEBUG(6)
printf("pc_getnode: making new node pcp=0x%x\n", pcp);
	vp = PCTOV(pcp);
	vp->v_count = 1;
	if (ep->pcd_attr & PCA_DIR) {
		vp->v_op = &pcfs_dvnodeops;
		vp->v_type = VDIR;
		if (scluster == 0) {
			vp->v_flag = VROOT;
			blkno = offset = 0;
		}
	} else {
		vp->v_op = &pcfs_fvnodeops;
		vp->v_type = VREG;
		fsp->pcfs_frefs++;
	}
	fsp->pcfs_nrefs++;
	vp->v_data = (caddr_t)pcp;
	vp->v_vfsp = PCFSTOVFS(fsp);
	pcp->pc_entry = *ep;
	pcp->pc_eblkno = blkno;
	pcp->pc_eoffset = offset;
	pcp->pc_scluster = ltohs(ep->pcd_scluster);
	pcp->pc_size = ltohl(ep->pcd_size);
	pcp->pc_flags = 0;
	insque(pcp, hp);
	return (pcp);
}

void
pc_rele(pcp)
	register struct pcnode *pcp;
{
	register struct pcfs *fsp;

	fsp = VFSTOPCFS(pcp->pc_vn.v_vfsp);
	if (!(fsp->pcfs_flags & PCFS_LOCKED) || pcp->pc_vn.v_count != 0 ||
	    pcp->pc_flags & PC_RELE_PEND)
		panic("pc_rele");
	if (!(pcp->pc_vn.v_flag & VROOT || pcp->pc_flags & PC_INVAL)) {
		/*
		 * If the file was removed while active it may be safely
		 * truncated now.
		 */
		if (pcp->pc_entry.pcd_filename[0] == PCD_ERASED) {
			int error;

			error = pc_getfat(fsp);
			if (error == 0) {
				(void) pc_truncate(pcp, 0L);
				pc_syncfat(fsp);
			}
		}
	}
	remque(pcp);
	if ((pcp->pc_vn.v_type == VREG) && !(pcp->pc_flags & PC_INVAL))
		fsp->pcfs_frefs--;
	fsp->pcfs_nrefs--;
	if (fsp->pcfs_nrefs < 0 || fsp->pcfs_frefs < 0)
		panic("pc_rele: ref count");
	kmem_free((caddr_t)pcp, (u_int)sizeof(struct pcnode));
}

/*
 * Mark a pcnode as modified with the current time.
 */
void
pc_mark(pcp)
	register struct pcnode *pcp;
{

PCFSDEBUG(4)
printf("pc_mark pcp=0x%x\n", pcp);
	if (pcp->pc_vn.v_type == VREG) {
		pc_tvtopct(&time, &pcp->pc_entry.pcd_mtime);
		pcp->pc_entry.pcd_scluster = htols(pcp->pc_scluster);
		pcp->pc_entry.pcd_size = htoll(pcp->pc_size);
		pcp->pc_flags |= PC_MOD | PC_CHG;
	}
}

/*
 * Truncate a file to a length.
 * Node must be locked.
 */
int
pc_truncate(pcp, length)
	register struct pcnode *pcp;
	long length;
{
	register struct pcfs *fsp;
	int error = 0;

PCFSDEBUG(4)
printf("pc_truncate pcp=0x%x\n", pcp, length);
	if (pcp->pc_flags & PC_INVAL)
		return (EIO);
	fsp = VFSTOPCFS(pcp->pc_vn.v_vfsp);
	/*
	 * directories are always truncated to zero and are not marked
	 */
	if (pcp->pc_vn.v_type == VDIR) {
		error = pc_bfree(pcp, (short)0);
		return (error);
	}
	/*
	 * If length is the same as the current size
	 * just mark the pcnode and return.
	 */
	if (length > pcp->pc_size) {
		daddr_t bno;

		/*
		 * We are extending a file.
		 * Extend it with pc_balloc (no holes).
		 */
		error = pc_balloc(pcp, pc_lblkno(fsp, length), &bno);
	} else if (length < pcp->pc_size) {
		/*
		 * We are shrinking a file.
		 * Free blocks after the block that length points to.
		 */
		error = pc_bfree(pcp, (short)howmany(length, fsp->pcfs_clsize));
	}
	pcp->pc_size = length;
	pc_mark(pcp);
	return (error);
}

/*
 * Sync all data associated with a file.
 * Flush all the blocks in the buffer cache out to disk, sync the FAT and
 * update the directory entry.
 */
void
pc_nodesync(pcp)
	register struct pcnode *pcp;
{
	register struct pcfs *fsp;
	register int flags;

PCFSDEBUG(4)
printf("pc_nodesync pcp=0x%x\n", pcp);
	fsp = VFSTOPCFS(pcp->pc_vn.v_vfsp);
	flags = pcp->pc_flags;
	pcp->pc_flags &= ~(PC_MOD | PC_CHG);
	if ((flags & (PC_MOD | PC_CHG)) && (pcp->pc_vn.v_type == VDIR)) {
		panic("pc_nodesync");
	}
	if (flags & PC_MOD) {
		/*
		 * Flush all data blocks from buffer cache and
		 * update the fat which points to the data.
		 */
		(void) pc_bflush(pcp);
		pc_syncfat(fsp);
	}
	if (flags & PC_CHG) {
		/*
		 * update the directory entry
		 */
		pc_nodeupdate(pcp);
	}
}

/*
 * Update the node's directory entry.
 */
void
pc_nodeupdate(pcp)
	register struct pcnode *pcp;
{
	struct buf *bp;

	if (pcp->pc_vn.v_flag & VROOT)
		panic("pc_nodeupdate");
	if (pcp->pc_flags & PC_INVAL)
		return;
PCFSDEBUG(4)
printf("pc_nodeupdate pcp=0x%x, bn=%d, off=%d\n", pcp, pcp->pc_eblkno, pcp->pc_eoffset);
	if (pc_getentryblock(pcp, &bp))
		return;
	pcp->pc_entry.pcd_scluster = htols(pcp->pc_scluster);
	pcp->pc_entry.pcd_size = htoll(pcp->pc_size);
	*((struct pcdir *)(bp->b_un.b_addr + pcp->pc_eoffset)) = pcp->pc_entry;
	bwrite(bp);
}

/*
 * Verify that the disk in the drive is the same one that we
 * got the pcnode from.
 * MUST be called with node unlocked.
 */
int
pc_verify(pcp)
	register struct pcnode *pcp;
{
	struct buf *bp;
	register struct pcdir *ep;
	register struct pcfs *fsp;
	int error;

PCFSDEBUG(3)
printf("pc_verify pcp=0x%x, bn=%d\n", pcp, pcp->pc_eblkno);
	if (pcp->pc_flags & PC_INVAL)
		return (EIO);
	if (pcp->pc_vn.v_flag & VROOT)
		return (0);
	fsp = VFSTOPCFS(pcp->pc_vn.v_vfsp);
	/*
	 * If we have verified the disk recently, don't bother.
	 * Otherwise, setup the next verify interval and reset the fat
	 * timer to this because we don't have to time out the fat since
	 * the disk has been verified.
	 */
	if (timercmp(&time, &fsp->pcfs_verifytime, <))
		return (0);
	fsp->pcfs_verifytime = time;
	fsp->pcfs_verifytime.tv_sec += PCFS_DISKTIMEOUT;
	fsp->pcfs_fattime = fsp->pcfs_verifytime;
	error = pc_getentryblock(pcp, &bp);
	if (error)
		return (error);
	ep = (struct pcdir *)(bp->b_un.b_addr + pcp->pc_eoffset);
	/*
	 * Check that the name extension and attributes are the same.
	 */
	if (bcmp(ep->pcd_filename, pcp->pc_entry.pcd_filename, PCFNAMESIZE) ||
	    bcmp(ep->pcd_ext, pcp->pc_entry.pcd_ext, PCFEXTSIZE) ||
	    ((ep->pcd_attr & PCA_DIR) != (pcp->pc_entry.pcd_attr & PCA_DIR)) ||
	    ((ep->pcd_attr & PCA_DIR) &&
	     (ep->pcd_scluster != pcp->pc_entry.pcd_scluster)) ) {
		error = EIO;
	}
	brelse(bp);
	if (error)
		pc_diskchanged(fsp);
PCFSDEBUG(5)
printf("pc_verify pcp=0x%x: pcp OK\n", pcp);
	return (error);
}

/*
 * Get block for entry.
 */
static int
pc_getentryblock(pcp, bpp)
	register struct pcnode *pcp;
	struct buf **bpp;
{
	register struct pcfs *fsp;

	fsp = VFSTOPCFS(pcp->pc_vn.v_vfsp);
	if (pcp->pc_eblkno >= fsp->pcfs_datastart ||
	    (pcp->pc_eblkno - fsp->pcfs_rdirstart) < 
	      (fsp->pcfs_rdirsec & ~(fsp->pcfs_spcl - 1)) ) {
		*bpp =
		    bread(fsp->pcfs_devvp, pcp->pc_eblkno, fsp->pcfs_clsize);
	} else {
		*bpp =
		   bread(fsp->pcfs_devvp,
			pcp->pc_eblkno,
			(int) (fsp->pcfs_datastart-pcp->pc_eblkno)*PC_SECSIZE);
	}
	if ((*bpp)->b_flags & (B_ERROR | B_INVAL)) {
		if ((*bpp)->b_flags & B_ERROR)
			pc_diskchanged(fsp);
		brelse(*bpp);
		return (EIO);
	}
	(*bpp)->b_flags |= B_NOCACHE;		/* don't cache */
	return (0);
}

/*
 * The disk has been changed!
 */
void
pc_diskchanged(fsp)
	register struct pcfs *fsp;
{
	register struct pcnode *pcp;
	register struct pchead *hp;

	/*
	 * Turn all non-root directory pcnodes into root.
	 * Mark all file pcnodes as invalid.
	 * Invalidate the in core fat.
	 * Invalidate cached data blocks and blocks waiting for I/O.
	 */
PCFSDEBUG(1)
printf("pc_diskchanged fsp=0x%x\n", fsp);
	if (fsp->pcfs_frefs)
		uprintf(
		    "I/O error or floppy disk change: possible file damage\n");
	for (hp = pcdhead; hp < &pcdhead[NPCHASH]; hp++) {
		for (pcp = hp->pch_forw;
		    pcp != (struct pcnode *)hp; pcp = pcp->pc_forw) {
			if (VFSTOPCFS(pcp->pc_vn.v_vfsp) == fsp &&
			    pcp->pc_scluster != 0) {
				pcp->pc_vn.v_flag |= VROOT;
				pcp->pc_scluster = 0;
				pcp->pc_eblkno = 0;
				pcp->pc_eoffset = 0;
				pcp->pc_entry = rootentry;
			}
		}
	}
	for (hp = pcfhead; fsp->pcfs_frefs && hp < &pcfhead[NPCHASH]; hp++) {
		for (pcp = hp->pch_forw; fsp->pcfs_frefs &&
		    pcp != (struct pcnode *)hp; pcp = pcp->pc_forw) {
			if (VFSTOPCFS(pcp->pc_vn.v_vfsp) == fsp) {
				pcp->pc_flags |= PC_INVAL;
				pcp->pc_vn.v_op = &pcfs_ivnodeops;
				fsp->pcfs_frefs--;
			}
		}
	}
	if (fsp->pcfs_frefs)
		panic("pc_diskchanged");
	if (fsp->pcfs_fatp != (u_char *)0)
		pc_invalfat(fsp);
	binval(fsp->pcfs_devvp);
}

#ifdef notdef
void
pc_allnodesync(fsp)
	register struct pcfs *fsp;
{
	register struct pchead *hp;
	register struct pcnode *pcp;

PCFSDEBUG(4)
printf("pc_allnodesync fsp=0x%x\n", fsp);
	for (hp = pcfhead; hp < &pcfhead[NPCHASH]; hp++) {
		for (pcp = hp->pch_forw;
		     pcp != (struct pcnode *)hp; pcp = pcp->pc_forw) {
			if (fsp == VFSTOPCFS(pcp->pc_vn.v_vfsp)) {
				pc_nodesync(pcp);
			}
		}
	}
}
#endif
