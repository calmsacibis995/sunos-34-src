#ifndef lint
static	char sccsid[] = "@(#)quota_syscalls.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Quota system calls.
 */
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/uio.h"
#include "../ufs/quota.h"
#include "../ufs/inode.h"
#include "../ufs/mount.h"
#include "../ufs/fs.h"

/*
 * Sys call to allow users to find out
 * their current position wrt quota's
 * and to allow super users to alter it.
 */
quotactl(uap)
	register struct a {
		int	cmd;
		caddr_t	fdev;
		int	uid;
		caddr_t	addr;
	} *uap;
{
	struct mount *mp;

	if (uap->uid < 0)
		uap->uid = u.u_ruid;
	if (uap->uid != u.u_ruid && !suser())
		return;
	if (uap->cmd == Q_SYNC && uap->fdev == NULL) {
		mp = 0;
	} else {
		u.u_error = fdevtomp(uap->fdev, &mp);
		if (u.u_error)
			return;
	}
	switch (uap->cmd) {

	case Q_QUOTAON:
		u.u_error = opendq(mp, uap->addr);
		break;

	case Q_QUOTAOFF:
		u.u_error = closedq(mp);
		break;

	case Q_SETQUOTA:
	case Q_SETQLIM:
		u.u_error = setquota(uap->cmd, uap->uid, mp, uap->addr);
		break;

	case Q_GETQUOTA:
		u.u_error = getquota(uap->uid, mp, uap->addr);
		break;

	case Q_SYNC:
		u.u_error = qsync(mp);
		break;

	default:
		u.u_error = EINVAL;
		break;
	}
}

/* XXX */
oldquota()
{
	printf("oldquota\n");
}

/*
 * Set the quota file up for a particular file system.
 * Called as the result of a setquota system call.
 */
int
opendq(mp, addr)
	register struct mount *mp;
	caddr_t addr;			/* quota file */
{
	struct vnode *vp;
	struct dquot *dqp;
	int error;

	if (!suser())
		return (u.u_error);
	error =
	    lookupname(addr, UIOSEG_USER, FOLLOW_LINK,
		(struct vnode **)0, &vp);
	if (error)
		return (error);
	if (VFSTOM(vp->v_vfsp) != mp || vp->v_type != VREG) {
		VN_RELE(vp);
		return (EACCES);
	}
	if (mp->m_qinod != NULL)
		(void)closedq(mp);
	mp->m_qinod = VTOI(vp);
	mp->m_qflags = 0;
	/*
	 * Timelimits for the super user set the relative time
	 * the other users can be over quota for this file system.
	 * If it is zero a default is used (see quota.h).
	 */
	dqp = getdiskquota(0, mp);
	if (dqp != NULL) {
		mp->m_btimelimit =
		    (dqp->dq_btimelimit? dqp->dq_btimelimit: DQ_BTIMELIMIT);
		mp->m_ftimelimit =
		    (dqp->dq_ftimelimit? dqp->dq_ftimelimit: DQ_FTIMELIMIT);
		dqrele (dqp);
	} else {
		mp->m_btimelimit = DQ_BTIMELIMIT;
		mp->m_ftimelimit = DQ_FTIMELIMIT;
	}
	return (0);
}

/*
 * Close off disk quotas for a file system.
 */
int
closedq(mp)
	register struct mount *mp;
{
	register struct dquot *dqp;
	register struct inode *ip;

	if (!suser())
		return (u.u_error);
	if (mp->m_qinod == NULL)
		return (0);
	/*
	 * Prevent new inodes in this filesystem from referencing dquots.
	 */
	mp->m_qflags |= Q_CLOSING;
	/*
	 * Run down the inode table and release all dquots assciated with
	 * inodes on this filesystem.
	 */
	for (ip = inode; ip < inodeNINODE; ip++) {
		dqp = ip->i_dquot;
		if (dqp != NULL && dqp->dq_mp == mp) {
			ip->i_dquot = NULL;
			dqrele(dqp);
		}
	}
	/*
	 * Run down the dquot table and check whether dquots for this
	 * filesystem are still referenced. If not take them off their
	 * hash list and put them on a private, unfindable hash list.
	 */
	for (dqp = dquot; dqp < dquotNDQUOT; dqp++) {
		if (dqp->dq_mp == mp) {
			if (dqp->dq_cnt)
				panic("closedq: stray dquot");
			remque(dqp);
			dqp->dq_forw = dqp;
			dqp->dq_back = dqp;
			dqp->dq_mp = NULL;
		}
	}
	/*
	 * Release the quota file inode.
	 */
	irele(mp->m_qinod);
	mp->m_qinod = NULL;
	return (0);
}

/*
 * Set various feilds of the dqblk according to the command.
 * Q_SETQUOTA - assign an entire dqblk structure.
 * Q_SETQLIM - assign a dqblk structure except for the usage.
 */
int
setquota(cmd, uid, mp, addr)
	int cmd;
	short uid;
	struct mount *mp;
	caddr_t addr;
{
	register struct dquot *dqp;
	struct dqblk newlim;
	int error;

	if (!suser())
		return (u.u_error);			/* XXX */
	error = copyin(addr, (caddr_t)&newlim, sizeof (struct dqblk));
	if (error)
		return (error);
	dqp = getdiskquota(uid, mp);
	if (dqp == NULL)
		return (ESRCH);
	/*
	 * Don't change disk usage on Q_SETQLIM
	 */
	if (cmd == Q_SETQLIM) {
		newlim.dqb_curblocks = dqp->dq_curblocks;
		newlim.dqb_curfiles = dqp->dq_curfiles;
	}
	if (uid == 0) {
		/*
		 * Timelimits for the super user set the relative time
		 * the other users can be over quota for this file system.
		 * If it is zero a default is used (see quota.h).
		 */
		mp->m_btimelimit =
		    newlim.dqb_btimelimit? newlim.dqb_btimelimit: DQ_BTIMELIMIT;
		mp->m_ftimelimit =
		    newlim.dqb_ftimelimit? newlim.dqb_ftimelimit: DQ_FTIMELIMIT;
	} else {
		/*
		 * If the user was under quota before, set timelimit to zero.
		 * If the (l)user is now over quota, the timelimit will start
		 * the next time he does an allocation.
		 */
		if (dqp->dq_curblocks < dqp->dq_bsoftlimit)
			newlim.dqb_btimelimit = 0;
		if (dqp->dq_curfiles < dqp->dq_fsoftlimit)
			newlim.dqb_ftimelimit = 0;
	}
	dqp->dq_dqb = newlim;
	dqp->dq_flags |= DQ_MOD;
	dqrele(dqp);
	return (0);
}

/*
 * Q_GETDLIM - return current values in a dqblk structure.
 */
int
getquota(uid, mp, addr)
	short uid;
	struct mount *mp;
	caddr_t addr;
{
	register struct dquot *dqp;
	int error;

	dqp = getdiskquota(uid, mp);
	if (dqp == NULL)
		return (ESRCH);
	error = copyout((caddr_t)&dqp->dq_dqb, addr, sizeof (struct dqblk));
	dqrele(dqp);
	return (error);
}

/*
 * Q_SYNC - sync quota files to disk.
 */
int
qsync(mp)
	register struct mount *mp;
{
	register struct dquot *dqp;

	if (!suser())
		return (u.u_error);			/* XXX */
	for (dqp = dquot; dqp < dquotNDQUOT; dqp++) {
		if ((dqp->dq_flags & DQ_MOD) == 0 || (mp && dqp->dq_mp != mp))
			continue;
		dqupdate(dqp);
	}
	return (0);
}

int
fdevtomp(fdev, mpp)
	char *fdev;
	struct mount **mpp;
{
	struct vnode *vp;
	dev_t dev;
	int error;

	error =
	    lookupname(fdev, UIOSEG_USER, FOLLOW_LINK,
		(struct vnode **)0, &vp);
	if (error)
		return (error);
	if (vp->v_type != VBLK) {
		VN_RELE(vp);
		return (ENOTBLK);
	}
	dev = vp->v_rdev;
	VN_RELE(vp);
	*mpp = getmp(dev);
	if (*mpp == NULL)
		return (ENODEV);
	else
		return (0);
}
