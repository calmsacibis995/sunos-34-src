#ifndef lint
static	char sccsid[] = "@(#)quota_ufs.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * SUN QUOTAS
 *
 * Routines used in checking limits on file system usage.
 */
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/kernel.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/buf.h"
#include "../h/uio.h"
#include "../ufs/quota.h"
#include "../ufs/inode.h"
#include "../ufs/mount.h"
#include "../ufs/fs.h"

/*
 * Find the dquot structure that should
 * be used in checking i/o on inode ip.
 */
struct dquot *
getinoquota(ip)
	register struct inode *ip;
{
	register struct dquot *dqp;

	dqp = getdiskquota(ip->i_uid, VFSTOM(ip->i_vnode.v_vfsp));
	if (dqp != NULL &&
	    dqp->dq_fhardlimit == 0 && dqp->dq_fsoftlimit == 0 &&
	    dqp->dq_bhardlimit == 0 && dqp->dq_bsoftlimit == 0) {
		dqrele(dqp);
		dqp = NULL;
	}
	return (dqp);
}

/*
 * Update disk usage, and take corrective action.
 */
int
chkdq(ip, change, force)
	struct inode *ip;
	long change;
	int force;
{
	register struct dquot *dqp;
	register u_long ncurblocks;
	int error = 0;

	if (change == 0)
		return (0);
	dqp = ip->i_dquot;
	if (dqp == NULL)
		return (0);
	dqp->dq_flags |= DQ_MOD;
	if (change < 0) {
		if ((int)dqp->dq_curblocks + change >= 0)
			dqp->dq_curblocks += change;
		else
			dqp->dq_curblocks = 0;
		dqp->dq_flags &= ~DQ_BLKS;
		return (0);
	}

	ncurblocks = dqp->dq_curblocks + change;
	/*
	 * Allocation. Check hard and soft limits.
	 * Skip checks for super user.
	 */
	if (u.u_uid == 0)
		goto out;
	/*
	 * Dissallow allocation if it would bring the current usage over
	 * the hard limit or if the user is over his soft limit and his time
	 * has run out.
	 */
	if (ncurblocks >= dqp->dq_bhardlimit && dqp->dq_bhardlimit && !force) {
		if ((dqp->dq_flags & DQ_BLKS) == 0 && ip->i_uid == u.u_ruid) {
			uprintf("\nDISK LIMIT REACHED (%s) - WRITE FAILED\n",
			   ip->i_fs->fs_fsmnt);
			dqp->dq_flags |= DQ_BLKS;
		}
		error = EDQUOT;
	}
	if (ncurblocks >= dqp->dq_bsoftlimit && dqp->dq_bsoftlimit) {
		if (dqp->dq_curblocks < dqp->dq_bsoftlimit ||
		    dqp->dq_btimelimit == 0) {
			dqp->dq_btimelimit =
			    time.tv_sec +
			    VFSTOM(ip->i_vnode.v_vfsp)->m_btimelimit;
			if (ip->i_uid == u.u_ruid)
				uprintf("\nWARNING: disk quota (%s) exceeded\n",
				   ip->i_fs->fs_fsmnt);
		} else if (time.tv_sec > dqp->dq_btimelimit && !force) {
			if ((dqp->dq_flags & DQ_BLKS) == 0 &&
			    ip->i_uid == u.u_ruid) {
				uprintf(
				"\nOVER DISK QUOTA: (%s) NO MORE DISK SPACE\n",
				ip->i_fs->fs_fsmnt);
				dqp->dq_flags |= DQ_BLKS;
			}
			error = EDQUOT;
		}
	}
out:
	if (error == 0)
		dqp->dq_curblocks = ncurblocks;
	return (error);
}

/*
 * Check the inode limit, applying corrective action.
 */
int
chkiq(mp, ip, uid, force)
	struct mount *mp;
	struct inode *ip;
	int uid;
	int force;
{
	register struct dquot *dqp;
	register u_long ncurfiles;
	int error = 0;

	if (ip != NULL) {		/* free */
		dqp = ip->i_dquot;
		if (dqp == NULL)
			return (0);
		dqp->dq_flags |= DQ_MOD;
		if (dqp->dq_curfiles)
			dqp->dq_curfiles--;
		dqp->dq_flags &= ~DQ_FILES;
		return (0);
	}

	/*
	 * Allocation. Get dquot for for uid, fs.
	 */
	dqp = getdiskquota(uid, mp);
	if (dqp == NULL) {
		return (0);
	} else if (dqp->dq_fsoftlimit == 0 && dqp->dq_fhardlimit == 0) {
		dqrele(dqp);
		return (0);
	}
	dqp->dq_flags |= DQ_MOD;
	/*
	 * Skip checks for super user.
	 */
	if (u.u_uid == 0)
		goto out;
	ncurfiles = dqp->dq_curfiles + 1;
	/*
	 * Dissallow allocation if it would bring the current usage over
	 * the hard limit or if the user is over his soft limit and his time
	 * has run out.
	 */
	if (ncurfiles >= dqp->dq_fhardlimit && dqp->dq_fhardlimit && !force) {
		if ((dqp->dq_flags & DQ_FILES) == 0 && uid == u.u_ruid) {
			uprintf("\nFILE LIMIT REACHED - CREATE FAILED (%s)\n",
			    mp->m_bufp->b_un.b_fs->fs_fsmnt);
			dqp->dq_flags |= DQ_FILES;
		}
		error = EDQUOT;
	} else if (ncurfiles >= dqp->dq_fsoftlimit && dqp->dq_fsoftlimit) {
		if (ncurfiles == dqp->dq_fsoftlimit || dqp->dq_ftimelimit==0) {
			dqp->dq_ftimelimit = time.tv_sec + mp->m_ftimelimit;
			if (uid == u.u_ruid)
				uprintf("\nWARNING - too many files (%s)\n",
				    mp->m_bufp->b_un.b_fs->fs_fsmnt);
		} else if (time.tv_sec > dqp->dq_ftimelimit && !force) {
			if ((dqp->dq_flags&DQ_FILES) == 0 && uid == u.u_ruid) {
				uprintf(
				    "\nOVER FILE QUOTA - NO MORE FILES (%s)\n",
				    mp->m_bufp->b_un.b_fs->fs_fsmnt);
				dqp->dq_flags |= DQ_FILES;
			}
			error = EDQUOT;
		}
	}
out:
	if (error == 0)
		dqp->dq_curfiles++;
	dqrele(dqp);
	return (error);
}
