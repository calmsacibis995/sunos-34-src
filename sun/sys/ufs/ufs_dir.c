/*	ufs_dir.c	1.6	87/01/07	*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Directory manipulation routines.
 * From outside this file, only dirlook, direnter and dirremove
 * should be called.
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/buf.h"
#include "../h/uio.h"
#include "../h/dnlc.h"
#include "../ufs/inode.h"
#include "../ufs/fs.h"
#include "../ufs/mount.h"
#include "../ufs/fsdir.h"
#ifdef QUOTA
#include "../ufs/quota.h"
#endif

/*
 * A virgin directory.
 */
struct dirtemplate mastertemplate = {
	0, 12, 1, ".",
	0, DIRBLKSIZ - 12, 2, ".."
};

#define LDIRSIZ(len) \
    ((sizeof (struct direct) - (MAXNAMLEN+1)) + ((len+1 + 3) &~ 3))

struct buf *blkatoff();

int dirchk = 0;
/*
 * Look for a certain name in a directory
 * On successful return, *ipp will point to the (locked) inode.
 */
dirlook(dp, namep, ipp)
	register struct inode *dp;
	register char *namep;		/* name */
	register struct inode **ipp;
{
	register struct buf *bp = 0;	/* a buffer of directory entries */
	register struct direct *ep;	/* the current directory entry */
	register struct inode *ip;
	struct vnode *vp, *dnlc_lookup();
	int entryoffsetinblock;		/* offset of ep in bp's buffer */
	int numdirpasses;		/* strategy for directory search */
	int endsearch;			/* offset to end directory search */
	int namlen = strlen(namep);	/* length of name */
	int offset;
	int error;
	register int i;

	/*
	 * Check accessiblity of directory.
	 */
	if ((dp->i_mode&IFMT) != IFDIR) {
		return (ENOTDIR);
	}

	if (error = iaccess(dp, IEXEC))
		return (error);

	/*
	 * Check the directory name lookup cache.
	 */
	vp = dnlc_lookup(ITOV(dp), namep, NOCRED);
	if (vp) {
		VN_HOLD(vp);
		*ipp = VTOI(vp);
		ilock(*ipp);
		return (0);
	}

	ilock(dp);
	if (dp->i_diroff > dp->i_size) {
		dp->i_diroff = 0;
	}
	if (dp->i_diroff == 0) {
		offset = 0;
		numdirpasses = 1;
	} else {
		offset = dp->i_diroff;
		entryoffsetinblock = blkoff(dp->i_fs, offset);
		if (entryoffsetinblock != 0) {
			bp = blkatoff(dp, offset, (char **)0);
			if (bp == 0) {
				error = u.u_error;
				goto bad;
			}
		}
		numdirpasses = 2;
	}
	endsearch = roundup(dp->i_size, DIRBLKSIZ);

searchloop:
	while (offset < endsearch) {
		/*
		 * If offset is on a block boundary,
		 * read the next directory block.
		 * Release previous if it exists.
		 */
		if (blkoff(dp->i_fs, offset) == 0) {
			if (bp != NULL)
				brelse(bp);
			bp = blkatoff(dp, offset, (char **)0);
			if (bp == 0) {
				error = u.u_error;	/* XXX */
				goto bad;
			}
			entryoffsetinblock = 0;
		}

		ep = (struct direct *)(bp->b_un.b_addr + entryoffsetinblock);

		/*
		 * Inline expansion of dirmangled for speed.
		 */
		i = DIRBLKSIZ - (entryoffsetinblock & (DIRBLKSIZ - 1));
		if ((ep->d_reclen & 0x3) || ep->d_reclen == 0 ||
		    ep->d_reclen > i || DIRSIZ(ep) > ep->d_reclen ||
		    dirchk &&
		    (ep->d_namlen > MAXNAMLEN ||
		    dirbadname(ep->d_name, (int)ep->d_namlen))) {
			dirbad(dp, "mangled entry", entryoffsetinblock);
			offset += i;
			entryoffsetinblock += i;
			continue;
		}

		/*
		 * Check for a name match.
		 * We must get the target inode before unlocking
		 * the directory to insure that the inode will not be removed
		 * before we get it.  We prevent deadlock by always fetching
		 * inodes from the root, moving down the directory tree. Thus
		 * when following backward pointers ".." we must unlock the
		 * parent directory before getting the requested directory.
		 * There is a potential race condition here if both the current
		 * and parent directories are removed before the `iget' for the
		 * inode associated with ".." returns.  We hope that this
		 * occurs infrequently since we can't avoid this race condition
		 * without implementing a sophisticated deadlock detection
		 * algorithm. Note also that this simple deadlock detection
		 * scheme will not work if the file system has any hard links
		 * other than ".." that point backwards in the directory
		 * structure.
		 * See comments at head of file about deadlocks.
		 */
		if (ep->d_ino && ep->d_namlen == namlen &&
		    *namep == *ep->d_name &&	/* fast chk 1st chr */
		    bcmp(namep, ep->d_name, (int)ep->d_namlen) == 0) {
			u_long ep_ino;

			/*
			 * we have to release the bp early here to avoid a
			 * deadlock situation where we have the bp and want
			 * the directory inode and someone doing a direnter
			 * has the directory inode and wants the bp.
			 */
			ep_ino = ep->d_ino;
			brelse(bp);
			bp = (struct buf *)0;
			dp->i_diroff = offset;
			if (namlen == 2 && namep[0] == '.' && namep[1] == '.') {
				iunlock(dp);	/* race to get the inode */
				ip = iget(dp->i_dev, dp->i_fs, ep_ino);
				if (ip == NULL) {
					error = u.u_error;
					goto bad2;
				}
			} else if (dp->i_number == ep_ino) {
				VN_HOLD(ITOV(dp));	/* want ourself, "." */
				ip = dp;
			} else {
				ip = iget(dp->i_dev, dp->i_fs, ep_ino);
				iunlock(dp);
				if (ip == NULL) {
					error = u.u_error;
					goto bad2;
				}
			}
			*ipp = ip;
			dnlc_enter(ITOV(dp), namep, ITOV(ip), NOCRED);
			return (0);
		}
		offset += ep->d_reclen;
		entryoffsetinblock += ep->d_reclen;
	}
	/*
	 * If we started in the middle of the directory and failed
	 * to find our target, we must check the beginning as well.
	 */
	if (numdirpasses == 2) {
		numdirpasses--;
		offset = 0;
		endsearch = dp->i_diroff;
		goto searchloop;
	}
	error = ENOENT;
bad:
	iunlock(dp);
bad2:
	if (bp)
		brelse(bp);
	return (error);
}

/*
 * If dircheckforname fails to find a name, this structure holds
 * state for direnter as to where there is space for an entry.
 * If dircheckforname succeeds then this structure holds state
 * for dirrename and dirremove as to where the entry is.
 * After dircheckforname succeeds the values are:
 *	status	offset		size		bp, ep
 *	------	------		----		------
 *	NONE	end of dir	needed		not valid
 *	COMPACT	start of area	of area		not valid
 *	FOUND	start of entry	of ent		not valid
 *	EXIST	start if entry	of prev ent	valid
 * On success, dirprepareentry makes bp and ep valid.
 */
struct slot {
	enum	{NONE, COMPACT, FOUND, EXIST} status;
	int	offset;		/* offset of area with free space */
	int	size;		/* size of area at slotoffset */
	struct buf *bp;		/* dir buf where slot is */
	struct direct *ep;	/* pointer to slot */
};

/*
 * Write a new directory entry.
 * The directory must not have been removed and must be writeable.
 * There are three operations in building the new entry: creating a file
 * or directory (DE_CREATE), renaming (DE_RENAME) or linking (DE_LINK).
 * There are five possible cases to consider:
 *	Name
 *	found	op			action
 *	----	---			-------------------------------
 *	no	DE_CREATE		create file according to vap and enter
 *	no	DE_LINK | DE_RENAME	enter the file sip
 *	yes	DE_CREATE		error EEXIST *ipp = found file
 *	yes	DE_LINK			error EEXIST
 *	yes	DE_RENAME		remove existing file, enter new file
 */
direnter(tdp, namep, op, sdp, sip, vap, ipp)
	register struct inode *tdp;	/* target directory to make entry in */
	register char *namep;		/* name of entry */
	enum de_op op;			/* entry operation */
	register struct inode *sdp;	/* source inode parent if rename */
	struct inode *sip;		/* source inode if link/rename */
	struct vattr *vap;		/* attributes if new inode needed */
	struct inode **ipp;		/* return entered inode (locked) here */
{
	struct inode *tip;		/* inode of (existing) target file */
	struct slot slot;		/* slot info to pass around */
	register int namlen;		/* length of name */
	register int error;		/* error number */

	namlen = strlen(namep);
	/*
	 * If name is "." or ".." then if this is a create look it up
	 * and return EEXIST. Rename or link TO "." or ".." is forbidden.
	 */
	if (namlen == 1 && bcmp(namep, ".", 1) == 0 ||
	    namlen == 2 && bcmp(namep, "..", 2) == 0) {
		if (op == DE_RENAME) {
			return (ENOTEMPTY);
		}
		if (ipp) {
			if (error = dirlook(tdp, namep, ipp))
				return (error);
		}
		return (EEXIST);
	}
	if (dirbadname(namep, namlen)) {
		return (EPERM);
	}
	slot.status = NONE;
	slot.bp = NULL;
	/*
	 * For link and rename lock the sorce entry and check the link count to
	 * see if it has been removed while it was unlocked. If not, we
	 * increment the link count and force the inode to disk to make sure
	 * that it is there before any directory entry that points to it.
	 */
	if (op != DE_CREATE) {
		ilock(sip);
		if (sip->i_nlink == 0) {
			iunlock(sip);
			return (ENOENT);
		}
		if(sip->i_nlink == MAXLINK) {
			iunlock(sip);
			return (EMLINK);
		}
		sip->i_nlink++;
		imark(sip, ICHG);
		iupdat(sip, 1);
		iunlock(sip);
	}
	/*
	 * lock the directory in which we are trying to make the new entry.
	 */
	ilock(tdp);
	/*
	 * Check accessiblity of directory.
	 */
	if ((tdp->i_mode&IFMT) != IFDIR) {
		error = ENOTDIR;
		goto out;
	}
	/*
	 * If target directory has not been removed, then we can consider
	 * allowing file to be created.
	 */
	if (tdp->i_nlink == 0) {
		error = ENOENT;
		goto out;
	}
	/*
	 * Execute access is required to search the directory.
	 */
	if (error = iaccess(tdp, IEXEC)) {
		goto out;
	}
	/*
	 * If this is a rename and we are doing a directory and the parent
	 * is different (".." must be changed), then the source directory must
	 * not be in the directory heirarchy above the target, as this would
	 * orphan everything below the source directory. Also the user must
	 * have write permission in the source so as to be able to change "..".
	 */
	if ((op == DE_RENAME) &&
	    ((sip->i_mode&IFMT) == IFDIR) && (sdp != tdp)) {
		if (error = iaccess(sip, IWRITE))
			goto out;
		if (error = dircheckpath(sip, tdp))
			goto out;
	}
	/*
	 * search for the entry
	 */
	error = dircheckforname(tdp, namep, namlen, &slot, &tip);
	if (error) {
		goto out;
	}

	if (tip) {
		switch (op) {

		case DE_CREATE:
			if (ipp) {
				*ipp = tip;
				error = EEXIST;
			} else {
				iput(tip);
			}
			break;

		case DE_RENAME:
			error =
			    dirrename(sdp, sip,
				tdp, namep, namlen, tip, &slot);
			iput(tip);
			break;

		case DE_LINK:
			/*
			 * Can't link to an existing file
			 */
			iput(tip);
			error = EEXIST;
			break;
		}
	} else {
		/*
		 * The entry does not exist. Check write permission in
		 * directory to see if entry can be created.
		 */
		if (error = iaccess(tdp, IWRITE)) {
			goto out;
		}
		if (op == DE_CREATE) {
			/*
			 * make a new inode and directory as required
			 */
			error = dirmakeinode(tdp, &sip, vap);
			if (error) {
				goto out;
			}
		}
		error = diraddentry(tdp, namep, namlen, &slot, sip, sdp);
		if (error) {
			if (op == DE_CREATE) {
				/*
				 * unmake the inode we just made
				 */
				if ((sip->i_mode & IFMT) == IFDIR) {
					tdp->i_nlink--;
				}
				sip->i_nlink = 0;
				sip->i_flag |= ICHG;
				irele(sip);
				sip = NULL;
			}
		} else if (ipp) {
			ilock(sip);
			*ipp = sip;
		} else if (op == DE_CREATE) {
			irele(sip);
		}
	}

out:
	if (slot.bp)
		brelse(slot.bp);
	if (error && (op != DE_CREATE)) {
		/*
		 * undo bumped link count
		 */
		sip->i_nlink--;
		imark(sip, ICHG);
	}
	iunlock(tdp);
	return (error);
}

/*
 * Check for the existence of a slot to make a directory entry.
 * On successful return *ipp points at the (locked) inode found.
 * The target directory inode (tdp) is supplied locked.
 * This may not be used on "." or "..", but aliases of "." are ok.
 */
dircheckforname(tdp, namep, namlen, slotp, tipp)
	register struct inode *tdp;	/* inode of directory being checked */
	char *namep;			/* name we're checking for */
	register int namlen;		/* length of name */
	register struct slot *slotp;	/* slot structure */
	struct inode **tipp;		/* return inode if we find one */
{
	int dirsize;			/* size of the directory */
	register struct buf *bp;	/* pointer to directory block */
	register int entryoffsetinblock;/* offset of ep in bp's buffer */
	int slotfreespace;		/* free space in block */
	register struct direct *ep;	/* directory entry */
	register int offset;		/* offset in the directory */
	register int last_offset;	/* last offset */
	int len;			/* length of mangled entry */
	int needed;
	register struct inode *ip;

	bp = NULL;
	entryoffsetinblock = 0;
	needed = LDIRSIZ(namlen);
	/*
	 * No point in using i_diroff since we must search whole directory
	 */
	dirsize = roundup(tdp->i_size, DIRBLKSIZ);
	offset = last_offset = 0;
	while (offset < dirsize) {
		/*
		 * If offset is on a block boundary,
		 * read the next directory block.
		 * Release previous if it exists.
		 */
		if (blkoff(tdp->i_fs, offset) == 0) {
			if (bp != NULL)
				brelse(bp);
			bp = blkatoff(tdp, offset, (char **)0);
			if (bp == 0) {
				return (u.u_error);
			}
			entryoffsetinblock = 0;
		}
		/*
		 * If still looking for a slot, and at a DIRBLKSIZE
		 * boundary, have to start looking for free space
		 * again.
		 */
		if (slotp->status == NONE &&
		    (entryoffsetinblock&(DIRBLKSIZ-1)) == 0) {
			slotp->offset = -1;
			slotfreespace = 0;
		}
		ep = (struct direct *)(bp->b_un.b_addr + entryoffsetinblock);
		if (len = dirmangled(tdp, ep, entryoffsetinblock)) {
			offset += len;
			entryoffsetinblock += len;
			continue;
		}
		/*
		 * If an appropriate sized slot has not yet been found,
		 * check to see if one is available. Also accumulate space
		 * in the current block so that we can determine if
		 * compaction is viable.
		 */
		if (slotp->status != FOUND) {
			int size = ep->d_reclen;

			if (ep->d_ino != 0)
				size -= DIRSIZ(ep);
			if (size > 0) {
				if (size >= needed) {
					slotp->status = FOUND;
					slotp->offset = offset;
					slotp->size = ep->d_reclen;
				} else if (slotp->status == NONE) {
					slotfreespace += size;
					if (slotp->offset == -1)
						slotp->offset = offset;
					if (slotfreespace >= needed) {
						slotp->status = COMPACT;
						slotp->size =
						    offset+ep->d_reclen -
						      slotp->offset;
					}
				}
			}
		}
		/*
		 * Check for a name match.
		 */
		if (ep->d_ino && ep->d_namlen == namlen &&
		    *namep == *ep->d_name &&	/* fast chk 1st char */
		    bcmp(namep, ep->d_name, namlen) == 0) {
			tdp->i_diroff = offset;
			if (tdp->i_number == ep->d_ino) {
				*tipp = tdp;	/* we want ourself, ie "." */
				VN_HOLD(ITOV(tdp));
			} else {
				ip = iget(tdp->i_dev, tdp->i_fs, ep->d_ino);
				if (ip == NULL) {
					brelse(bp);
					return (u.u_error);
				}
				*tipp = ip;
			}
			slotp->status = EXIST;
			slotp->offset = offset;
			slotp->size = offset - last_offset;
			slotp->bp = bp;
			slotp->ep = ep;
			return (0);
		}
		last_offset = offset;
		offset += ep->d_reclen;
		entryoffsetinblock += ep->d_reclen;
	}
	if (bp) {
		brelse(bp);
	}
	if (slotp->status == NONE) {
		slotp->offset = offset;
		slotp->size = needed;
	}
	*tipp = (struct inode *)0;
	return (0);
}

/*
 * Rename the entry in the directory tdp so that
 * it points to sip instead of tip.
 */
/*ARGSUSED*/
dirrename(sdp, sip, tdp, namep, namlen, tip, slotp)
	register struct inode *sdp;	/* parent directory of source */
	register struct inode *sip;	/* source inode */
	register struct inode *tdp;	/* parent directory of target */
	char *namep;			/* entry we are trying to change */
	int namlen;			/* length of entry string */
	struct inode *tip;		/* locked target inode */
	struct slot *slotp;		/* slot for entry */
{
	int error;
	int doingdirectory;
	int parentdifferent;

	/*
	 * Check that everything is on the same filesystem.
	 */
	if ((tip->i_vnode.v_vfsp != tdp->i_vnode.v_vfsp) ||
	    (tip->i_vnode.v_vfsp != sip->i_vnode.v_vfsp))
		return (EXDEV);		/* XXX archaic */
	/*
	 * Short circuit rename (foo, foo).
	 */
	if (sip->i_number == tip->i_number)
		return (ESAME);		/* special error code */
	/*
	 * Must have write permission to rewrite target entry.
	 */
	if (error = iaccess(tdp, IWRITE))
		return (error);
	/*
	 * If the parent directory is "sticky", then the user must
	 * own the parent directory, or the destination of the rename,
	 * otherwise the destination may not be changed (except by
	 * root). This implements append-only directories.
	 */
	if ((tdp->i_mode & ISVTX) && u.u_uid != 0 &&
	    u.u_uid != tdp->i_uid && tip->i_uid != u.u_uid)
		return (EPERM);
	/*
	 * Ensure source and target are compatible
	 * (both directories or both not directories).
	 * If target is a directory it must be empty
	 * and have no links to it.
	 */
	doingdirectory = ((sip->i_mode & IFMT) == IFDIR);
	parentdifferent = (sdp != tdp);
	if ((tip->i_mode & IFMT) == IFDIR) {
		/*
		 * Target is a dir. Source must be a dir and
		 * target must be empty.
		 */
		if (!doingdirectory) {
			error = ENOTDIR;
			goto bad;
		}
		if (!dirempty(tip) || (tip->i_nlink > 2)) {
			error = ENOTEMPTY;
			goto bad;
		}
	} else if (doingdirectory) {
		/*
		 * Source is a dir and target is not.
		 */
		error = ENOTDIR;
		goto bad;
	}
	/*
	 * Rewrite the inode pointer for target name entry
	 * from the target inode (ip) to the source inode (sip).
	 * This prevents the target entry from disappearing
	 * during a crash. Mark the directory inode to reflect the changes.
	 */
	dnlc_remove(ITOV(tdp), namep);
	slotp->ep->d_ino = sip->i_number;
	dnlc_enter(ITOV(tdp), namep, ITOV(sip), NOCRED);
	bwrite(slotp->bp);
	slotp->bp = NULL;
	imark(tdp, IUPD|ICHG);
	/*
	 * Decrement the link count of the target inode.
	 * Fix the ".." entry in sip to point to dp.
	 * This is done after the new entry is on the disk.
	 */
	tip->i_nlink--;
	if (doingdirectory) {
		/*
		 * Decrement target link count once more if it was a directory.
		 */
		if (--tip->i_nlink != 0) {
			panic("direnter: target directory link count");
		}
		itrunc(tip, (u_long)0);
		/*
		 * Renaming a directory with the parent different requires
		 * ".." to be rewritten. The window is still there for ".."
		 * to be inconsistent, but this is unavoidable, and a lot
		 * shorter than when it was done in a user process.
		 * Unconditionally decrement the link count in new parent;
		 * if parent is same, need to decrement (since original
		 * directory is going away). If new parent is different,
		 * dirfixdotdot() will bump link count back.
		 */
		tdp->i_nlink--;
		if (parentdifferent) {
			error = dirfixdotdot(sip, sdp, tdp);
			if (error) {
				goto bad;
			}
		}
	}
	imark(tip, ICHG);
bad:
	return (error);
}

/*
 * Fix the ".." entry of the child directory from the old parent to the
 * new parent directory.
 * Assumes dp is a directory and that all the inodes are on the same
 * file system.
 */
dirfixdotdot(dp, opdp, npdp)
	register struct inode *dp;	/* child directory */
	register struct inode *opdp;	/* old parent directory */
	register struct inode *npdp;	/* new parent directory */
{
	register struct buf *bp;
	struct dirtemplate *dirp;
	register int error = 0;

	/*
	 * check whether this is an ex-directory
	 */
	ilock(dp);
	if ((dp->i_nlink == 0) || (dp->i_size < sizeof(struct dirtemplate))) {
		iunlock(dp);
		return (0);
	}
	bp = blkatoff(dp, (off_t)0, (char **) &dirp);
	if (bp == NULL) {
		error = u.u_error;
		goto bad;
	}
	if (dirp->dotdot_ino == npdp->i_number) {   /* just a no-op */
		goto bad;
	}
	if (bcmp(dirp->dotdot_name, "..", 3)) {
		dirbad(dp, "mangled .. entry", 0);
		error = EINVAL;
		goto bad;
	}
	/*
	 * Increment the link count in the new parent inode and force it out.
	 */
	npdp->i_nlink++;
	imark(npdp, ICHG);
	iupdat(npdp, 1);
	/*
	 * Rewrite the child ".." entry and force it out.
	 */
	dnlc_remove(ITOV(dp), "..");
	dirp->dotdot_ino = npdp->i_number;
	dnlc_enter(ITOV(dp), "..", ITOV(npdp), NOCRED);
	bwrite(bp);
	iunlock(dp);
	/*
	 * Decrement the link count of the old parent inode and force it out.
	 * If opdp is NULL, then this is a new directory link; it has no
	 * parent, so we need not do anything.
	 */
	if (opdp != NULL) {
		ilock(opdp);
		if (opdp->i_nlink != 0) {
			opdp->i_nlink--;
			imark(opdp, ICHG);
			iupdat(opdp, 1);
		}
		iunlock(opdp);
	}
	return (0);
bad:
	if (bp)
		brelse(bp);
	iunlock(dp);
	return (error);
}

/*
 * Enter the file sip in the directory tdp with name namep.
 */
diraddentry(tdp, namep, namlen, slotp, sip, sdp)
	struct inode *tdp;
	char *namep;
	int namlen;
	struct slot *slotp;
	struct inode *sip;
	struct inode *sdp;
{
	int error;
	char *strncpy();

	/*
	 * Prepare a new entry. If the caller has not supplied an
	 * existing inode, make a new one.
	 */
	error = dirprepareentry(tdp, slotp);
	if (error) {
		return (error);
	}
	/*
	 * Check inode to be linked to see if it is in the
	 * same filesystem.
	 */
	if (tdp->i_vnode.v_vfsp != sip->i_vnode.v_vfsp) {
		error = EXDEV;
		goto bad;
	}
	if ((sip->i_mode & IFMT) == IFDIR) {
		error = dirfixdotdot(sip, sdp, tdp);
		if (error) {
			goto bad;
		}
	}
	/*
	 * Fill in entry data
	 */
	slotp->ep->d_namlen = namlen;
	(void) strncpy(slotp->ep->d_name, namep, (namlen + 4) & ~3);
	slotp->ep->d_ino = sip->i_number;
	dnlc_enter(ITOV(tdp), namep, ITOV(sip), NOCRED);
	/*
	 * Write out the directroy entry.
	 */
	bwrite(slotp->bp);
	slotp->bp = NULL;
	/*
	 * Mark the directory inode to reflect the changes.
	 */
	imark(tdp, IUPD|ICHG);
	tdp->i_diroff = 0;
	return (0);

bad:
	/*
	 * Clear out entry prepared by dirprepareent.
	 */
	slotp->ep->d_ino = 0;
	bwrite(slotp->bp);
	slotp->bp = NULL;
	return (error);
}

/*
 * Prepare a directory slot to receive an entry.
 */
dirprepareentry(dp, slotp)
	register struct inode *dp;	/* directory we are working in */
	register struct slot *slotp;	/* available slot info */
{
	register int slotfreespace;
	register int dsize;
	int entryend;
	register int loc;
	register struct direct *ep, *nep;
	char *dirbuf;

	/*
	 * If we didn't find a slot, then indicate that the
	 * new slot belongs at the end of the directory.
	 * If we found a slot, then the new entry can be
	 * put at slotp->offset.
	 */
	if (slotp->status == NONE) {
		if (slotp->offset & (DIRBLKSIZ - 1)) {
			panic("dirprepareentry: new block");
		}
		entryend = slotp->offset + slotp->size;
		/*
		 * Allocate the new block.
		 */
		if ((bmap(dp, (daddr_t)lblkno(dp->i_fs, slotp->offset), B_WRITE,
		    blkoff(dp->i_fs, entryend), 0) <= 0) || u.u_error) {
			if (u.u_error == 0)
				u.u_error = ENOSPC;
			return (u.u_error);
		}
	} else {
		entryend = slotp->offset + slotp->size;
	}
	/*
	 * adjust directory size, if needed.
	 */
	if (entryend > dp->i_size) {
		dp->i_size = entryend;
		imark(dp, IUPD|ICHG);
	}
	/*
	 * If we didn't find a slot, get the block containing the space
	 * for the new directory entry.
	 */
	slotp->bp = blkatoff(dp, slotp->offset, (char **)&slotp->ep);
	if (slotp->bp == 0) {
		return (u.u_error);
	}
	ep = slotp->ep;
	dirbuf = (char *)ep;
	switch (slotp->status) {
	case NONE:
		/*
		 * No space in the directory. slotp->offset will be on a
		 * directory block boundary and we will write the new entry
		 * into a fresh block.
		 */
		bzero((char *)ep, DIRBLKSIZ);
		ep->d_reclen = DIRBLKSIZ;
		break;

	case FOUND:
	case COMPACT:
		/*
		 * Found space for the new entry
		 * in the range slotp->offset to slotp->offset + slotp->size
		 * in the directory.  To use this space, we have to compact
		 * the entries located there, by copying them together towards
		 * the beginning of the block, leaving the free space in
		 * one usable chunk at the end.
		 */
		dsize = DIRSIZ(ep);
		slotfreespace = ep->d_reclen - dsize;
		for (loc = ep->d_reclen; loc < slotp->size; ) {
			nep = (struct direct *)(dirbuf + loc);
			if (ep->d_ino) {
				/* trim the existing slot */
				ep->d_reclen = dsize;
				ep = (struct direct *)((char *)ep + dsize);
			} else {
				/* overwrite; nothing there; header is ours */
				slotfreespace += dsize;
			}
			dsize = DIRSIZ(nep);
			slotfreespace += nep->d_reclen - dsize;
			loc += nep->d_reclen;
			bcopy((caddr_t)nep, (caddr_t)ep, (unsigned)dsize);
		}
		/*
		 * Update the pointer fields in the previous entry (if any).
		 * At this point, ep is the last entry in the range
		 * slotp->offset to slotp->offset + slotp->size.
		 * Slotfreespace is the now unallocated space after the
		 * ep entry that resulted from copying entries above.
		 */
		if (ep->d_ino == 0) {
			ep->d_reclen = slotfreespace + dsize;
		} else {
			ep->d_reclen = dsize;
			ep = (struct direct *)((char *)ep + dsize);
			ep->d_reclen = slotfreespace;
		}
		break;

	default:
		panic("dirprepareentry: invalid slot status");
	}
	slotp->ep = ep;
	return (0);
}

/*
 * Allocate and initialize a new inode that will go
 * into directory tdp.
 */
dirmakeinode(tdp, ipp, vap)
	struct inode *tdp;
	struct inode **ipp;
	register struct vattr *vap;
{
	register struct inode *ip;
	register enum vtype type;
	int imode;			/* mode and format as in inode */
	ino_t ipref;
	int error = 0;

	if (vap == (struct vattr *) 0) {
		panic("dirmakeinode: no attributes");
	}
	/*
	 * Allocate a new inode.
	 */
	type = vap->va_type;
	if (type == VDIR) {
		ipref = dirpref(tdp->i_fs);
	} else {
		ipref = tdp->i_number;
	}
	imode = MAKEIMODE(type, vap->va_mode);
	ip = ialloc(tdp, ipref, imode);
	if (ip == NULL) {
		return (u.u_error);
	}
#ifdef QUOTA
	if (ip->i_dquot != NULL)
		panic("direnter: dquot");
#endif
	imark(ip, IACC|IUPD|ICHG);
	ip->i_mode = imode;
	if (type == VBLK || type == VCHR) {
		ip->i_vnode.v_rdev = ip->i_rdev = vap->va_rdev;
	}
	ip->i_vnode.v_type = type;
	if (type == VDIR) {
		ip->i_nlink = 2;     /* anticipating a call to dirmakedirect */
	} else {
		ip->i_nlink = 1;
	}
	ip->i_uid = u.u_uid;
	ip->i_gid = tdp->i_gid;
	if ((ip->i_mode & ISGID) && !groupmember(ip->i_gid)) {
		ip->i_mode &= ~ISGID;
	}
#ifdef QUOTA
	ip->i_dquot = getinoquota(ip);
#endif
	/*
	 * Make sure inode goes to disk before directory data and entries
	 * pointing to it.
	 * Then unlock it, since nothing points to it yet.
	 */
	iupdat(ip, 1);
	if (type == VDIR) {
		error = dirmakedirect(ip, tdp);
	}
	if (error) {
		ip->i_nlink = 0;
		ip->i_flag |= ICHG;
		iput(ip);
	} else {
		iunlock(ip);
		*ipp = ip;
	}
	return (error);
}

/*
 * Make an empty directory in the inode ip.
 */
dirmakedirect(ip, dp)
	register struct inode *ip;		/* new directory */
	register struct inode *dp;		/* parent directory */
{
	register struct dirtemplate *dirp;
	struct buf *bp;
	struct fs *fs;
	daddr_t bn;

	/*
	 * Allocate space for the directory we're creating.
	 */
	fs = ip->i_fs;
	bn = fsbtodb(fs,
	    bmap(ip, (daddr_t)0, B_WRITE, sizeof(struct dirtemplate), 0));
	if (bn <= 0 || u.u_error) {
		if (u.u_error == 0)
			u.u_error = ENOSPC;
		return (u.u_error);
	}
	ip->i_size = sizeof(struct dirtemplate);
	imark(ip, IUPD|ICHG);
	/*
	 * Update the tdp link count and write out the
	 * change.  This reflects the ".." entry we'll soon write.
	 */
	dp->i_nlink++;
	imark(dp, ICHG);
	iupdat(dp, 1);
	bp = getblk(ip->i_devvp, bn, (int)fs->fs_fsize);
	dirp = (struct dirtemplate *)bp->b_un.b_addr;
	/*
	 * Now initialize the directory we're creating with the
	 * "." and ".." entries.
	 */
	*dirp = mastertemplate;
	dirp->dot_ino = ip->i_number;
	dirp->dotdot_ino = dp->i_number;
	bwrite(bp);
	return (0);
}

/*
 * Delete a directory entry
 * If oip is nonzero the entry is checked to make sure it still reflects oip.
 */
dirremove(dp, namep, oip, rmdir)
	register struct inode *dp;
	char *namep;
	struct inode *oip;
	int rmdir;
{
	register struct direct *ep;
	struct direct *pep;
	struct inode *ip;
	int namlen = strlen(namep);	/* length of name */
	struct slot slot;
	int error = 0;

	/*
	 * return error when removing . and ..
	 */
	if (namlen == 1 && bcmp(namep, ".", 1) == 0) {
		return (EINVAL);
	}
	if (namlen == 2 && bcmp(namep, "..", 2) == 0) {
		return (ENOTEMPTY);
	}

	ip = NULL;
	slot.bp = NULL;
	ilock(dp);
	/*
	 * Check accessiblity of directory.
	 */
	if ((dp->i_mode&IFMT) != IFDIR) {
		error = ENOTDIR;
		goto out;
	}

	/*
	 * Execute access is required to search the directory.
	 * Access for write is interpreted as allowing
	 * deletion of files in the directory.
	 */
	if (error = iaccess(dp, IEXEC|IWRITE)) {
		goto out;
	}

	slot.status = FOUND;	/* don't need to look for empty slot */
	error = dircheckforname(dp, namep, namlen, &slot, &ip);
	if (error) {
		goto out;
	}
	if (ip == (struct inode *) 0) {
		error = ENOENT;
		goto out;
	}
	if (oip && oip != ip) {
		error = ENOENT;
		goto out;
	}
	/*
	 * Don't remove a mounted on directory. XXX
	 */
	if (ip->i_dev != dp->i_dev) {
		error = EBUSY;
		goto out;
	}
	/*
	 * If the parent directory is "sticky", then the user must
	 * own the parent directory, or the file in it, otherwise
	 * he may not delete it (unless he's root). This implements
	 * append-only directories.
	 */
	if ((dp->i_mode & ISVTX) && u.u_uid != 0 &&
	    u.u_uid != dp->i_uid && ip->i_uid != u.u_uid) {
		error = EPERM;
		goto out;
	}
	/*
	 * If the inode being removed is a directory, we must be
	 * sure it only has entries "." and "..".
	 */
	if (rmdir && (ip->i_mode&IFMT) == IFDIR) {
		if ((ip->i_nlink != 2) || !dirempty(ip)) {
			error = ENOTEMPTY;
			goto out;
		}
	}
	/*
	 * Remove the cache'd entry, if any.
	 */
	dnlc_remove(ITOV(dp), namep);
	/*
	 * If the entry isn't the first in the directory, we must reclaim
	 * the space of the now empty record by adding the record size
	 * to the size of the previous entry.
	 */
	ep = slot.ep;
	if ((slot.offset & (DIRBLKSIZ - 1)) == 0) {
		/*
		 * First entry in block: set d_ino to zero.
		 */
		ep->d_ino = 0;
	} else {
		/*
		 * Collapse new free space into previous entry.
		 */
		pep = (struct direct *) ((char *) ep - slot.size);
		pep->d_reclen += ep->d_reclen;
	}
	bwrite(slot.bp);
	slot.bp = NULL;
	/*
	 * Now dereference the inode.
	 */
	if (ip->i_nlink > 0) {
		if (rmdir && (ip->i_mode & IFMT) == IFDIR) {
			/*
			 * Decrement by 2 because we're trashing the "."
			 * entry as well as removing the entry in dp.
			 * Clear the inode, but there may be other hard
			 * links so don't free the inode.
			 * Decrement the dp linkcount because we're
			 * trashing the ".." entry.
			 */
			ip->i_nlink -= 2;
			dp->i_nlink--;
			dnlc_remove(ITOV(ip), ".");
			dnlc_remove(ITOV(ip), "..");
			itrunc(ip, (u_long)0);
		} else {
			ip->i_nlink--;
		}
	}
	imark(dp, IUPD|ICHG);
	imark(ip, ICHG);
out:
	if (ip)
		iput(ip);
	if (slot.bp)
		brelse(slot.bp);
	iunlock(dp);
	return (error);
}

/*
 * Return buffer with contents of block "offset"
 * from the beginning of directory "ip".  If "res"
 * is non-zero, fill it in with a pointer to the
 * remaining space in the directory.
 */
struct buf *
blkatoff(ip, offset, res)
	struct inode *ip;
	off_t offset;
	char **res;
{
	register struct fs *fs;
	daddr_t lbn;
	int bsize;
	daddr_t bn;
	register struct buf *bp;

	fs = ip->i_fs;
	lbn = lblkno(fs, offset);
	bsize = blksize(fs, ip, lbn);
	bn = fsbtodb(fs, bmap(ip, lbn, B_READ));
	if (bn < 0) {
		dirbad(ip, "nonexixtent directory block", offset);
		u.u_error = ENOENT;
	} 
	if (u.u_error) {
		return (0);
	}
	bp = bread(ip->i_devvp, bn, bsize);
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
		return (0);
	}
	if (res)
		*res = bp->b_un.b_addr + blkoff(fs, offset);
	return (bp);
}

/*
 * Do consistency checking:
 *	record length must be multiple of 4
 *	record length must not be zero
 *	entry must fit in rest of this DIRBLKSIZ block
 *	record must be large enough to contain name.
 * When dirchk is set we also check:
 *	name is not longer than MAXNAMLEN
 *	name must be as long as advertised, and null terminated.
 * Checking last two conditions is done only when dirchk is
 * set, to save time.
 */
dirmangled(dp, ep, entryoffsetinblock)
	register struct inode *dp;
	register struct direct *ep;
	int entryoffsetinblock;
{
	register int i;

	i = DIRBLKSIZ - (entryoffsetinblock & (DIRBLKSIZ - 1));
	if ((ep->d_reclen & 0x3) || ep->d_reclen == 0 ||
	    ep->d_reclen > i || DIRSIZ(ep) > ep->d_reclen ||
	    dirchk &&
	    (ep->d_namlen > MAXNAMLEN ||
	    dirbadname(ep->d_name, (int)ep->d_namlen))) {
		dirbad(dp, "mangled entry", entryoffsetinblock);
		return (i);
	}
	return (0);
}

dirbad(ip, how, offset)
	struct inode *ip;
	char *how;
	int offset;
{

	printf("%s: bad dir ino %d at offset %d: %s\n",
	    ip->i_fs->fs_fsmnt, ip->i_number, offset, how);
}

dirbadname(sp, l)
	register char *sp;
	register int l;
{
	register char c;

	while (l--) {			/* check for nulls or high bit */
		c = *sp++;
		if ((c == 0) || (c & 0200)) {
			return (1);
		}
	}
	return (*sp);			/* check for terminating null */
}

/*
 * Check if a directory is empty or not.
 * Inode supplied must be locked.
 *
 * Using a struct dirtemplate here is not precisely
 * what we want, but better than using a struct direct.
 *
 * NB: does not handle corrupted directories.
 */
dirempty(ip)
	register struct inode *ip;
{
	register off_t off;
	struct dirtemplate dbuf;
	register struct direct *dp = (struct direct *)&dbuf;
	int error, count;
#define	MINDIRSIZ (sizeof (struct dirtemplate) / 2)

	for (off = 0; off < ip->i_size; off += dp->d_reclen) {
		error = rdwri(UIO_READ, ip, (caddr_t)dp, MINDIRSIZ,
		    off, 1, &count);
		/*
		 * Since we read MINDIRSIZ, residual must
		 * be 0 unless we're at end of file.
		 */
		if (error || count != 0 || dp->d_reclen == 0)
			return (0);
		/* skip empty entries */
		if (dp->d_ino == 0)
			continue;
		/* accept only "." and ".." */
		if (dp->d_namlen > 2)
			return (0);
		if (dp->d_name[0] != '.')
			return (0);
		/*
		 * At this point d_namlen must be 1 or 2.
		 * 1 implies ".", 2 implies ".." if second
		 * char is also "."
		 */
		if (dp->d_namlen == 1 || dp->d_name[1] == '.')
			continue;
		return (0);
	}
	return (1);
}

#define	RENAME_IN_PROGRESS	0x01
#define	RENAME_WAITING		0x02

/*
 * Check if source directory is in the path of the target directory.
 * Target is supplied locked, source is unlocked.
 * The target is always relocked before returning.
 */
dircheckpath(source, target)
	struct inode *source, *target;
{
	struct buf *bp;
	struct dirtemplate *dirp;
	register struct inode *ip;
	static char serialize_flag = 0;
	int error = 0;

	/*
	 * If two renames of directories were in progress at once, the partially
	 * completed work of one dircheckpath could be invalidated by the other
	 * rename.  To avoid this, all directory renames in the system are
	 * serialized.
	 */
	while (serialize_flag & RENAME_IN_PROGRESS) {
		serialize_flag |= RENAME_WAITING;
		(void) sleep((caddr_t) &serialize_flag, PINOD);
	}
	serialize_flag = RENAME_IN_PROGRESS;
	ip = target;
	if (ip->i_number == source->i_number) {
		error = EINVAL;
		goto out;
	}
	if (ip->i_number == ROOTINO) {
		goto out;
	}
	bp = 0;
	for (;;) {
		if (((ip->i_mode&IFMT) != IFDIR) ||
		    (ip->i_nlink == 0) ||
		    (ip->i_size < sizeof(struct dirtemplate))) {
			dirbad(ip, "bad size, unlinked or not dir", 0);
			error = ENOTDIR;
			break;
		}
		if (bp) {
			brelse(bp);
		}
		bp = blkatoff(ip, (off_t)0, (char **)&dirp);
		if (bp == 0) {
			error = u.u_error;
			break;
		}
		if (dirp->dotdot_namlen != 2 ||
		    bcmp(dirp->dotdot_name, "..", 3) != 0) {
			dirbad(ip, "mangled .. entry", 0);
			error = ENOTDIR;
			break;
		}
		if (dirp->dotdot_ino == source->i_number) {
			error = EINVAL;
			break;
		}
		if (dirp->dotdot_ino == ROOTINO) {
			break;
		}
		if (ip != target) {
			iput(ip);
		} else {
			iunlock(ip);
		}
		/*
		 * i_dev and i_fs are still valid after iput
		 * This is a race to get ".." just like dirlook.
		 */
		ip = iget(ip->i_dev, ip->i_fs, dirp->dotdot_ino);
		if (ip == NULL) {
			error = u.u_error;
			break;
		}
	}
	if (bp) {
		brelse(bp);
	}
out:
	if (ip) {
		if (ip != target) {
			iput(ip);
			/*
			 * Relock target and make sure it has not gone away
			 * while it was unlocked.
			 */
			ilock(target);
			if ((error == 0) && (target->i_nlink == 0)) {
				error = ENOENT;
			}
		}
	}
	/*
	 * unserialize
	 */
	if (serialize_flag & RENAME_WAITING) {
		wakeup((caddr_t) &serialize_flag);
	}
	serialize_flag = 0;
	return (error);
}
