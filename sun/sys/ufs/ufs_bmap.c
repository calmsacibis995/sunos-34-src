/*	@(#)ufs_bmap.c 1.1 86/09/25 SMI; from UCB 5.4 83/03/15	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/vnode.h"
#include "../h/buf.h"
#include "../h/proc.h"
#include "../ufs/inode.h"
#include "../ufs/fs.h"

/*
 * Bmap defines the structure of file system storage
 * by returning the physical block number on a device given the
 * inode and the logical block number in a file.
 * When convenient, it also leaves the physical
 * block number of the next block of the file in rablock
 * for use in read-ahead.
 */
/*VARARGS3*/
daddr_t
bmap(ip, bn, rwflg, size, sync)
	register struct inode *ip;
	daddr_t bn;
	int rwflg;
	int size;	/* supplied only when rwflg == B_WRITE */
	int sync;	/* supplied only when rwflg == B_WRITE */
{
	register int i;
	int osize, nsize;
	struct buf *bp, *nbp;
	struct fs *fs;
	int j, sh;
	daddr_t nb, ob, lbn, *bap, pref, blkpref();

	if (bn < 0) {
		u.u_error = EFBIG;
		return ((daddr_t)0);
	}
	fs = ip->i_fs;
	rablock = 0;
	rasize = 0;		/* conservative */

	/*
	 * If the next write will extend the file into a new block,
	 * and the file is currently composed of a fragment
	 * this fragment has to be extended to be a full block.
	 */
	lbn = lblkno(fs, ip->i_size);
	if (rwflg == B_WRITE && lbn < NDADDR && lbn < bn) {
		osize = blksize(fs, ip, lbn);
		if (osize < fs->fs_bsize && osize > 0) {
			ob = ip->i_db[lbn];
			bp = realloccg(ip, ob,
				blkpref(ip, lbn, (int)lbn, &ip->i_db[0]),
				osize, (int)fs->fs_bsize);
			if (bp == NULL)
				return ((daddr_t)-1);
			ip->i_size = (lbn + 1) * fs->fs_bsize;
			nb = dbtofsb(fs, bp->b_blkno);
			ip->i_db[lbn] = nb;
			imark(ip, IUPD|ICHG);
			/*
			 * if syncronous operation is specified, then
			 * write out the new block synchronously, then
			 * update the inode to make sure it points to it
			 */
			if (sync) {
				bwrite(bp);
				iupdat(ip, 1);
			} else {
				bdwrite(bp);
			}
			if (nb != ob) {
				free(ip, ob, (off_t)osize);
			}
		}
	}
	/*
	 * The first NDADDR blocks are direct blocks
	 */
	if (bn < NDADDR) {
		nb = ip->i_db[bn];
		if (rwflg == B_READ) {
			if (nb == 0)
				return ((daddr_t)-1);
			goto gotit;
		}
		if (nb == 0 || ip->i_size < (bn + 1) * fs->fs_bsize) {
			if (nb != 0) {
				/* consider need to reallocate a frag */
				osize = fragroundup(fs, blkoff(fs, ip->i_size));
				nsize = fragroundup(fs, size);
				if (nsize <= osize)
					goto gotit;
				ob = nb;
				bp = realloccg(ip, ob,
					blkpref(ip, bn, (int)bn, &ip->i_db[0]),
					osize, nsize);
				if (bp == NULL)
					return ((daddr_t)-1);
				nb = dbtofsb(fs, bp->b_blkno);
				ip->i_db[bn] = nb;
				imark(ip, IUPD|ICHG);
				if ((ip->i_mode&IFMT) == IFDIR)
					/*
					 * Write directory blocks synchronously
					 * so they never appear with garbage in
					 * them on the disk.
					 */
					bwrite(bp);
				else
					bdwrite(bp);
				if (nb != ob) {
					free(ip, ob, (off_t)osize);
				}
			} else {
				if (ip->i_size < (bn + 1) * fs->fs_bsize)
					nsize = fragroundup(fs, size);
				else
					nsize = fs->fs_bsize;
				bp = alloc(ip,
					blkpref(ip, bn, (int)bn, &ip->i_db[0]),
					nsize);
				if (bp == NULL)
					return ((daddr_t)-1);
				nb = dbtofsb(fs, bp->b_blkno);
				ip->i_db[bn] = nb;
				imark(ip, IUPD|ICHG);
				if ((ip->i_mode&IFMT) == IFDIR)
					/*
					 * Write directory blocks synchronously
					 * so they never appear with garbage in
					 * them on the disk.
					 */
					bwrite(bp);
				else
					bdwrite(bp);
			}
		}
gotit:
		if (bn < NDADDR - 1) {
			rablock = fsbtodb(fs, ip->i_db[bn + 1]);
			rasize = blksize(fs, ip, bn + 1);
		}
		return (nb);
	}

	/*
	 * Determine how many levels of indirection.
	 */
	pref = 0;
	sh = 1;
	lbn = bn;
	bn -= NDADDR;
	for (j = NIADDR; j>0; j--) {
		sh *= NINDIR(fs);
		if (bn < sh)
			break;
		bn -= sh;
	}
	if (j == 0) {
		u.u_error = EFBIG;
		return ((daddr_t)0);
	}

	/*
	 * fetch the first indirect block
	 */
	nb = ip->i_ib[NIADDR - j];
	if (nb == 0) {
		if (rwflg == B_READ)
			return ((daddr_t)-1);
		pref = blkpref(ip, lbn, 0, (daddr_t *)0);
	        bp = alloc(ip, pref, (int)fs->fs_bsize);
		if (bp == NULL)
			return ((daddr_t)-1);
		nb = dbtofsb(fs, bp->b_blkno);
		/*
		 * Write synchronously so that indirect blocks
		 * never point at garbage.
		 */
		bwrite(bp);
		ip->i_ib[NIADDR - j] = nb;
		imark(ip, IUPD|ICHG);
	}

	/*
	 * fetch through the indirect blocks
	 */
	for (; j <= NIADDR; j++) {
		bp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, nb),
		    (int)fs->fs_bsize);
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return ((daddr_t)0);
		}
		bap = bp->b_un.b_daddr;
		sh /= NINDIR(fs);
		i = (bn / sh) % NINDIR(fs);
		nb = bap[i];
		if (nb == 0) {
			if (rwflg==B_READ) {
				brelse(bp);
				return ((daddr_t)-1);
			}
			if (pref == 0)
				if (j < NIADDR)
					pref = blkpref(ip, lbn, 0,
						(daddr_t *)0);
				else
					pref = blkpref(ip, lbn, i, &bap[0]);
		        nbp = alloc(ip, pref, (int)fs->fs_bsize);
			if (nbp == NULL) {
				brelse(bp);
				return ((daddr_t)-1);
			}
			nb = dbtofsb(fs, nbp->b_blkno);
			if (j < NIADDR || (ip->i_mode&IFMT) == IFDIR || sync) {
				/*
				 * Write synchronously so indirect blocks
				 * never point at garbage and blocks
				 * in directories never contain garbage.
				 */
				bwrite(nbp);
			} else {
				bdwrite(nbp);
			}
			bap[i] = nb;
			if (sync) {
				bwrite(bp);
			} else {
				bdwrite(bp);
			}
		} else {
			brelse(bp);
		}
	}

	/*
	 * calculate read-ahead.
	 */
	if (i < NINDIR(fs) - 1) {
		rablock = fsbtodb(fs, bap[i+1]);
		rasize = fs->fs_bsize;
	}
	return (nb);
}
