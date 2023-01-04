/*	@(#)ufs_inode.c 1.1 86/09/25 SMI; from UCB 4.36 83/06/11	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/buf.h"
#include "../h/kernel.h"
#include "../h/cmap.h"
#include "../ufs/mount.h"
#include "../ufs/inode.h"
#include "../ufs/fs.h"
#ifdef QUOTA
#include "../ufs/quota.h"
#endif

#define	INOHSZ	64
#define	INOHASH(dev,ino)	(((unsigned)((dev)+(ino)))%INOHSZ)

union ihead {				/* inode LRU cache, Chris Maltby */
	union  ihead *ih_head[2];
	struct inode *ih_chain[2];
} ihead[INOHSZ];

struct inode *ifreeh, **ifreet;

/*
 * Convert inode formats to vnode types
 */
enum vtype iftovt_tab[] = {
	VFIFO, VCHR, VDIR, VBLK, VREG, VLNK, VSOCK, VBAD
};

int vttoif_tab[] = {
	0, IFREG, IFDIR, IFBLK, IFCHR, IFLNK, IFSOCK, IFMT, IFIFO
};

/*
 * Initialize hash links for inodes
 * and build inode free list.
 */
ihinit()
{
	register int i;
	register struct inode *ip = inode;
	register union  ihead *ih = ihead;

	for (i = INOHSZ; --i >= 0; ih++) {
		ih->ih_head[0] = ih;
		ih->ih_head[1] = ih;
	}
	ifreeh = ip;
	ifreet = &ip->i_freef;
	ip->i_freeb = &ifreeh;
	ip->i_forw = ip;
	ip->i_back = ip;
	ip->i_vnode.v_data = (caddr_t)ip;
	ip->i_vnode.v_op = &ufs_vnodeops;
	for (i = ninode; --i > 0; ) {
		++ip;
		ip->i_forw = ip;
		ip->i_back = ip;
		*ifreet = ip;
		ip->i_freeb = ifreet;
		ifreet = &ip->i_freef;
		ip->i_vnode.v_data = (caddr_t)ip;
		ip->i_vnode.v_op = &ufs_vnodeops;
	}
	ip->i_freef = NULL;
}

#ifdef notdef
/*
 * Find an inode if it is incore.
 * This is the equivalent, for inodes,
 * of ``incore'' in bio.c or ``pfind'' in subr.c.
 */
struct inode *
ifind(dev, ino)
	dev_t dev;
	ino_t ino;
{
	register struct inode *ip;
	register union  ihead *ih;

	ih = &ihead[INOHASH(dev, ino)];
	for (ip = ih->ih_chain[0]; ip != (struct inode *)ih; ip = ip->i_forw)
		if (ino==ip->i_number && dev==ip->i_dev)
			return (ip);
	return ((struct inode *)0);
}
#endif notdef

/*
 * Look up an inode by device,inumber.
 * If it is in core (in the inode structure),
 * honor the locking protocol.
 * If it is not in core, read it in from the
 * specified device.
 * If the inode is mounted on, perform
 * the indicated indirection.
 * In all cases, a pointer to a locked
 * inode structure is returned.
 *
 * panic: no imt -- if the mounted file
 *	system is not in the mount table.
 *	"cannot happen"
 */
struct inode *
iget(dev, fs, ino)
	dev_t dev;
	register struct fs *fs;
	ino_t ino;
{
	register struct inode *ip;
	register union  ihead *ih;
	register struct buf *bp;
	register struct dinode *dp;
	register struct inode *iq;
	register struct vnode *vp;
	struct mount *mp;

	/*
	 * lookup inode in cache
	 */
loop:
	mp = getmp(dev);
	if (mp == NULL) {
		panic("iget: bad dev\n");
	}
	if (mp->m_bufp->b_un.b_fs != fs)
		panic("iget: bad fs");
	ih = &ihead[INOHASH(dev, ino)];
	for (ip = ih->ih_chain[0]; ip != (struct inode *)ih; ip = ip->i_forw)
		if (ino == ip->i_number && dev == ip->i_dev) {
			/*
			 * found it. check for locks
			 */
			if ((ip->i_flag & ILOCKED) != 0) {
				ip->i_flag |= IWANT;
				(void) sleep((caddr_t)ip, PINOD);
				goto loop;
			}
			/*
			 * If inode is on free list, remove it.
			 */
			if ((ip->i_flag & IREF) == 0) {
				if (iq = ip->i_freef)
					iq->i_freeb = ip->i_freeb;
				else
					ifreet = ip->i_freeb;
				*ip->i_freeb = iq;
				ip->i_freef = NULL;
				ip->i_freeb = NULL;
			}
			/*
			 * mark inode locked and referenced and return it.
			 */
			ip->i_flag |= ILOCKED | IREF;
			VN_HOLD(ITOV(ip));
			return(ip);
		}

	/*
	 * Inode was not in cache. Get free inode slot for new inode.
	 */
	while (ifreeh == NULL) {
		if (dnlc_purge1() == 0) {	/* XXX */
			break;
		}
	}
	if ((ip = ifreeh) == NULL) {
		tablefull("inode");
		u.u_error = ENFILE;
		return(NULL);
	}
	if (iq = ip->i_freef)
		iq->i_freeb = &ifreeh;
	ifreeh = iq;
	ip->i_freef = NULL;
	ip->i_freeb = NULL;
	/*
	 * Now to take inode off the hash chain it was on
	 * (initially, or after an iflush, it is on a "hash chain"
	 * consisting entirely of itself, and pointed to by no-one,
	 * but that doesn't matter), and put it on the chain for
	 * its new (ino, dev) pair
	 */
	remque(ip);
	insque(ip, ih);
#ifdef QUOTA
	dqrele(ip->i_dquot);
	ip->i_dquot = NULL;
#endif
	ip->i_flag = ILOCKED | IREF;
	ip->i_dev = dev;
	ip->i_devvp = mp->m_devvp;
	ip->i_number = ino;
	ip->i_diroff = 0;
	ip->i_fs = fs;
	ip->i_lastr = 0;
	bp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, itod(fs, ino)),
	    (int)fs->fs_bsize);
	/*
	 * Check I/O errors
	 */
	if ((bp->b_flags & B_ERROR) != 0) {
		brelse(bp);
		/*
		 * the inode doesn't contain anything useful, so it would
		 * be misleading to leave it on its hash chain.
		 * 'iput' will take care of putting it back on the free list.
		 */
		remque(ip);
		ip->i_forw = ip;
		ip->i_back = ip;
		/*
		 * we also loose its inumber, just in case (as iput
		 * doesn't do that any more) - but as it isn't on its
		 * hash chain, I doubt if this is really necessary .. kre
		 * (probably the two methods are interchangable)
		 */
		ip->i_number = 0;
		iunlock(ip);
		iinactive(ip);
		return(NULL);
	}
	dp = bp->b_un.b_dino;
	dp += itoo(fs, ino);
	ip->i_ic = dp->di_ic;
	vp = ITOV(ip);
	VN_INIT(vp, mp->m_vfsp, IFTOVT(ip->i_mode), ip->i_rdev);
	if (ino == (ino_t)ROOTINO) {
		vp->v_flag |= VROOT;
	}
	brelse(bp);
#ifdef QUOTA
	if (ip->i_mode != 0)
		ip->i_dquot = getinoquota(ip);
#endif
	return (ip);
}

/*
 * Unlock inode and vrele associated vnode
 */
iput(ip)
	register struct inode *ip;
{

	if ((ip->i_flag & ILOCKED) == 0)
		panic("iput");
	iunlock(ip);
	VN_RELE(ITOV(ip));
}

/*
 * Check that inode is not locked and release associated vnode.
 */
irele(ip)
	register struct inode *ip;
{
	if (ip->i_flag & ILOCKED)
		panic("irele");
	VN_RELE(ITOV(ip));
}

/*
 * Drop inode without going through the normal chain of unlocking
 * and releasing.
 */
idrop(ip)
	register struct inode *ip;
{
	register struct vnode *vp = &ip->i_vnode;

	if ((ip->i_flag & ILOCKED) == 0)
		panic("idrop");
	IUNLOCK(ip);
	if (--vp->v_count == 0) {
		ip->i_flag = 0;
		/*
		 * Put the inode back on the end of the free list.
		 */
		if (ifreeh) {
			*ifreet = ip;
			ip->i_freeb = ifreet;
		} else {
			ifreeh = ip;
			ip->i_freeb = &ifreeh;
		}
		ip->i_freef = NULL;
		ifreet = &ip->i_freef;
	}
}

/*
 * Vnode is no loger referenced, write the inode out and if necessary,
 * truncate and deallocate the file.
 */
iinactive(ip)
	register struct inode *ip;
{
	int mode;

	if (ip->i_flag & ILOCKED)
		panic("ufs_inactive");
	if (ip->i_fs->fs_ronly == 0) {
		ip->i_flag |= ILOCKED;
		if (ip->i_nlink <= 0) {
			ip->i_gen++;
			itrunc(ip, (u_long)0);
			mode = ip->i_mode;
			ip->i_mode = 0;
			ip->i_rdev = 0;
			imark(ip, IUPD|ICHG);
			ifree(ip, ip->i_number, mode);
#ifdef QUOTA
			(void)chkiq(VFSTOM(ip->i_vnode.v_vfsp),
			    ip, ip->i_uid, 0);
			dqrele(ip->i_dquot);
			ip->i_dquot = NULL;
#endif
		}
		if (ip->i_flag & (IUPD|IACC|ICHG))
			iupdat(ip, 0);
		iunlock(ip);
	}
	ip->i_flag = 0;
	/*
	 * Put the inode on the end of the free list.
	 * Possibly in some cases it would be better to
	 * put the inode at the head of the free list,
	 * (eg: where i_mode == 0 || i_number == 0)
	 * but I will think about that later .. kre
	 * (i_number is rarely 0 - only after an i/o error in iget,
	 * where i_mode == 0, the inode will probably be wanted
	 * again soon for an ialloc, so possibly we should keep it)
	 */
	if (ifreeh) {
		*ifreet = ip;
		ip->i_freeb = ifreet;
	} else {
		ifreeh = ip;
		ip->i_freeb = &ifreeh;
	}
	ip->i_freef = NULL;
	ifreet = &ip->i_freef;
}

/*
 * Check accessed and update flags on
 * an inode structure.
 * If any is on, update the inode
 * If waitfor is given, then must insure
 * i/o order so wait for write to complete.
 */
iupdat(ip, waitfor)
	register struct inode *ip;
	int waitfor;
{
	register struct buf *bp;
	struct dinode *dp;
	register struct fs *fp;

	fp = ip->i_fs;
	if ((ip->i_flag & (IUPD|IACC|ICHG)) != 0) {
		if (fp->fs_ronly)
			return;
		bp = bread(ip->i_devvp,
		    (daddr_t)fsbtodb(fp, itod(fp, ip->i_number)),
		    (int)fp->fs_bsize);
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return;
		}
		ip->i_flag &= ~(IUPD|IACC|ICHG);
		dp = bp->b_un.b_dino + itoo(fp, ip->i_number);
		dp->di_ic = ip->i_ic;
		if (waitfor)
			bwrite(bp);
		else
			bdwrite(bp);
	}
}

/*
 * Mark the accessed, updated, or changed times in an inode
 * with the current (unique) time
 */
imark(ip, flag)
	register struct inode *ip;
	register int flag;
{
	struct timeval ut;

	uniqtime(&ut);
	ip->i_flag |= flag;
	if (flag & IACC)
		ip->i_atime = ut;
	if (flag & IUPD)
		ip->i_mtime = ut;
	if (flag & ICHG) {
		ip->i_diroff = 0;
		ip->i_ctime = ut;
	}
}

#define	SINGLE	0	/* index of single indirect block */
#define	DOUBLE	1	/* index of double indirect block */
#define	TRIPLE	2	/* index of triple indirect block */
/*
 * Truncate the inode ip to at most
 * length size.  Free affected disk
 * blocks -- the blocks of the file
 * are removed in reverse order.
 *
 * NB: triple indirect blocks are untested.
 */
itrunc(oip, length)
	register struct inode *oip;
	u_long length;
{
	register daddr_t lastblock;
	daddr_t bn, lastiblock[NIADDR];
	register struct fs *fs;
	register struct inode *ip;
	struct buf *bp;
	int offset, osize, size, count, level, s;
	daddr_t lbn;
	long nblocks, blocksreleased = 0;
	register int i;
	struct vnode *devvp;
	struct inode tip;
	extern long indirtrunc();
	extern struct cmap *mfind();

	if (oip->i_size <= length) {
		imark(oip, IUPD|ICHG);
		iupdat(oip, 1);
		return;
	}
	/*
	 * Calculate index into inode's block list of
	 * last direct and indirect blocks (if any)
	 * which we want to keep.  Lastblock is -1 when
	 * the file is truncated to 0.
	 */
	fs = oip->i_fs;
	lastblock = lblkno(fs, length + fs->fs_bsize - 1) - 1;
	lastiblock[SINGLE] = lastblock - NDADDR;
	lastiblock[DOUBLE] = lastiblock[SINGLE] - NINDIR(fs);
	lastiblock[TRIPLE] = lastiblock[DOUBLE] - NINDIR(fs) * NINDIR(fs);
	nblocks = btodb(fs->fs_bsize);
	/*
	 * Update the size of the file. If the file is not being
	 * truncated to a block boundry, the contents of the
	 * partial block following the end of the file must be
	 * zero'ed in case it ever become accessable again because
	 * of subsequent file growth.
	 */
	osize = oip->i_size;
	offset = blkoff(fs, length);
	if (offset == 0) {
		oip->i_size = length;
	} else {
		lbn = lblkno(fs, length);
		bn = fsbtodb(fs, bmap(oip, lbn, B_WRITE, offset, 0));
		if (u.u_error || (long)bn < 0)
			return;
		oip->i_size = length;
		size = blksize(fs, oip, lbn);
		count = howmany(size, DEV_BSIZE);
		devvp = oip->i_devvp;
		s = splimp();
		for (i = 0; i < count; i += CLSIZE)
			if (mfind(devvp, bn + i))
				munhash(devvp, bn + i);
		(void) splx(s);
		bp = bread(devvp, bn, size);
		if (bp->b_flags & B_ERROR) {
			u.u_error = EIO;
			oip->i_size = osize;
			brelse(bp);
			return;
		}
		bzero(bp->b_un.b_addr + offset, (u_int)(size - offset));
		bdwrite(bp);
	}
	/*
	 * Update file and block pointers
	 * on disk before we start freeing blocks.
	 * If we crash before free'ing blocks below,
	 * the blocks will be returned to the free list.
	 * lastiblock values are also normalized to -1
	 * for calls to indirtrunc below.
	 */
	tip = *oip;
	tip.i_size = osize;
	for (level = TRIPLE; level >= SINGLE; level--)
		if (lastiblock[level] < 0) {
			oip->i_ib[level] = 0;
			lastiblock[level] = -1;
		}
	for (i = NDADDR - 1; i > lastblock; i--)
		oip->i_db[i] = 0;
	imark(oip, IUPD|ICHG);
	syncip(oip);

	/*
	 * Indirect blocks first.
	 */
	ip = &tip;
	for (level = TRIPLE; level >= SINGLE; level--) {
		bn = ip->i_ib[level];
		if (bn != 0) {
			blocksreleased +=
			    indirtrunc(ip, bn, lastiblock[level], level);
			if (lastiblock[level] < 0) {
				ip->i_ib[level] = 0;
				free(ip, bn, (off_t)fs->fs_bsize);
				blocksreleased += nblocks;
			}
		}
		if (lastiblock[level] >= 0)
			goto done;
	}

	/*
	 * All whole direct blocks or frags.
	 */
	for (i = NDADDR - 1; i > lastblock; i--) {
		register int bsize;

		bn = ip->i_db[i];
		if (bn == 0)
			continue;
		ip->i_db[i] = 0;
		bsize = (off_t)blksize(fs, ip, i);
		free(ip, bn, bsize);
		blocksreleased += btodb(bsize);
	}
	if (lastblock < 0)
		goto done;

	/*
	 * Finally, look for a change in size of the
	 * last direct block; release any frags.
	 */
	bn = ip->i_db[lastblock];
	if (bn != 0) {
		int oldspace, newspace;

		/*
		 * Calculate amount of space we're giving
		 * back as old block size minus new block size.
		 */
		oldspace = blksize(fs, ip, lastblock);
		ip->i_size = length;
		newspace = blksize(fs, ip, lastblock);
		if (newspace == 0)
			panic("itrunc: newspace");
		if (oldspace - newspace > 0) {
			/*
			 * Block number of space to be free'd is
			 * the old block # plus the number of frags
			 * required for the storage we're keeping.
			 */
			bn += numfrags(fs, newspace);
			free(ip, bn, oldspace - newspace);
			blocksreleased += btodb(oldspace - newspace);
		}
	}
done:
/* BEGIN PARANOIA */
	for (level = SINGLE; level <= TRIPLE; level++)
		if (ip->i_ib[level] != oip->i_ib[level])
			panic("itrunc1");
	for (i = 0; i < NDADDR; i++)
		if (ip->i_db[i] != oip->i_db[i])
			panic("itrunc2");
/* END PARANOIA */
	oip->i_blocks -= blocksreleased;
	if (oip->i_blocks < 0)			/* sanity */
		oip->i_blocks = 0;
	imark(oip, ICHG);
#ifdef QUOTA
	(void) chkdq(oip, -blocksreleased, 0);
#endif
}

/*
 * Release blocks associated with the inode ip and
 * stored in the indirect block bn.  Blocks are free'd
 * in LIFO order up to (but not including) lastbn.  If
 * level is greater than SINGLE, the block is an indirect
 * block and recursive calls to indirtrunc must be used to
 * cleanse other indirect blocks.
 *
 * NB: triple indirect blocks are untested.
 */
long
indirtrunc(ip, bn, lastbn, level)
	register struct inode *ip;
	daddr_t bn, lastbn;
	int level;
{
	register int i;
	struct buf *bp, *copy;
	register daddr_t *bap;
	register struct fs *fs = ip->i_fs;
	daddr_t nb, last;
	long factor;
	int blocksreleased = 0, nblocks;

	/*
	 * Calculate index in current block of last
	 * block to be kept.  -1 indicates the entire
	 * block so we need not calculate the index.
	 */
	factor = 1;
	for (i = SINGLE; i < level; i++)
		factor *= NINDIR(fs);
	last = lastbn;
	if (lastbn > 0)
		last /= factor;
	nblocks = btodb(fs->fs_bsize);
	/*
	 * Get buffer of block pointers, zero those 
	 * entries corresponding to blocks to be free'd,
	 * and update on disk copy first.
	 */
	copy = geteblk((int)fs->fs_bsize);
	bp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, bn), (int)fs->fs_bsize);
	if (bp->b_flags&B_ERROR) {
		brelse(copy);
		brelse(bp);
		return (0);
	}
	bap = bp->b_un.b_daddr;
	bcopy((caddr_t)bap, (caddr_t)copy->b_un.b_daddr, (u_int)fs->fs_bsize);
	bzero((caddr_t)&bap[last + 1],
	  (u_int)(NINDIR(fs) - (last + 1)) * sizeof (daddr_t));
	bwrite(bp);
	bp = copy, bap = bp->b_un.b_daddr;

	/*
	 * Recursively free totally unused blocks.
	 */
	for (i = NINDIR(fs) - 1; i > last; i--) {
		nb = bap[i];
		if (nb == 0)
			continue;
		if (level > SINGLE)
			blocksreleased +=
			    indirtrunc(ip, nb, (daddr_t)-1, level - 1);
		free(ip, nb, (int)fs->fs_bsize);
		blocksreleased += nblocks;
	}

	/*
	 * Recursively free last partial block.
	 */
	if (level > SINGLE && lastbn >= 0) {
		last = lastbn % factor;
		nb = bap[i];
		if (nb != 0)
			blocksreleased += indirtrunc(ip, nb, last, level - 1);
	}
	brelse(bp);
	return (blocksreleased);
}

/*
 * remove any inodes in the inode cache belonging to dev
 *
 * There should not be any active ones, return error if any are found
 * (nb: this is a user error, not a system err)
 *
 * Also, count the references to dev by block devices - this really
 * has nothing to do with the object of the procedure, but as we have
 * to scan the inode table here anyway, we might as well get the
 * extra benefit.
 *
 * this is called from sumount()/sys3.c when dev is being unmounted
 */
#ifdef QUOTA
iflush(dev, iq)
	dev_t dev;
	struct inode *iq;
#else
iflush(dev)
	dev_t dev;
#endif
{
	register struct inode *ip;
	register open = 0;

	for (ip = inode; ip < inodeNINODE; ip++) {
#ifdef QUOTA
		if (ip != iq && ip->i_dev == dev)
#else
		if (ip->i_dev == dev)
#endif
			if (ip->i_flag & IREF)
				return(-1);
			else {
				remque(ip);
				ip->i_forw = ip;
				ip->i_back = ip;
				/*
				 * as i_count == 0, the inode was on the free
				 * list already, just leave it there, it will
				 * fall off the bottom eventually. We could
				 * perhaps move it to the head of the free
				 * list, but as umounts are done so
				 * infrequently, we would gain very little,
				 * while making the code bigger.
				 */
#ifdef QUOTA
				dqrele(ip->i_dquot);
				ip->i_dquot = NULL;
#endif
			}
		else if ((ip->i_flag & IREF) && (ip->i_mode&IFMT)==IFBLK &&
		    ip->i_rdev == dev)
			open++;
	}
	return (open);
}

/*
 * Lock an inode. If its already locked, set the WANT bit and sleep.
 */
ilock(ip)
	register struct inode *ip;
{

	ILOCK(ip);
}

/*
 * Unlock an inode.  If WANT bit is on, wakeup.
 */
iunlock(ip)
	register struct inode *ip;
{

	if (!(ip->i_flag & ILOCKED)) {
		panic("iunlock");
	}
	IUNLOCK(ip);
}

/*
 * Check mode permission on inode.
 * Mode is READ, WRITE or EXEC.
 * In the case of WRITE, the
 * read-only status of the file
 * system is checked.
 * Also in WRITE, prototype text
 * segments cannot be written.
 * The mode is shifted to select
 * the owner/group/other fields.
 * The super user is granted all
 * permissions.
 */
iaccess(ip, m)
	register struct inode *ip;
	register int m;
{
	register int *gp;

	if (m & IWRITE) {
		register struct vnode *vp;

		vp = ITOV(ip);
		/*
		 * Disallow write attempts on read-only
		 * file systems; unless the file is a block
		 * or character device resident on the
		 * file system.
		 */
		if (ip->i_fs->fs_ronly != 0) {
			if ((ip->i_mode & IFMT) != IFCHR &&
			    (ip->i_mode & IFMT) != IFBLK) {
				u.u_error = EROFS;
				return (EROFS);
			}
		}
		/*
		 * If there's shared text associated with
		 * the inode, try to free it up once.  If
		 * we fail, we can't allow writing.
		 */
		if (vp->v_flag & VTEXT)
			xrele(vp);
		if (vp->v_flag & VTEXT) {
			u.u_error = ETXTBSY;
			return (ETXTBSY);
		}
	}
	/*
	 * If you're the super-user,
	 * you always get access.
	 */
	if (u.u_uid == 0)
		return (0);
	/*
	 * Access check is based on only
	 * one of owner, group, public.
	 * If not owner, then check group.
	 * If not a member of the group, then
	 * check public access.
	 */
	if (u.u_uid != ip->i_uid) {
		m >>= 3;
		if (u.u_gid == ip->i_gid)
			goto found;
		gp = u.u_groups;
		for (; gp < &u.u_groups[NGROUPS] && *gp != NOGROUP; gp++)
			if (ip->i_gid == *gp)
				goto found;
		m >>= 3;
	}
found:
	if ((ip->i_mode & m) == m)
		return (0);
	u.u_error = EACCES;
	return (EACCES);
}
