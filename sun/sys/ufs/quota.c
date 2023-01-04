#ifndef lint
static	char sccsid[] = "@(#)quota.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Code pertaining to management of the in-core data structures.
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
 * Dquot cache - hash chain headers.
 */
#define	NDQHASH		64
#define	DQHASH(uid, mp) \
	(((unsigned)(mp) + (unsigned)(uid)) % NDQHASH)

struct	dqhead	{
	struct	dquot	*dqh_forw;	/* MUST be first */
	struct	dquot	*dqh_back;	/* MUST be second */
};

/*
 * Dquot in core hash chain headers
 */
struct	dqhead	dqhead[NDQHASH];

/*
 * Dquot free list.
 */
struct dquot dqfreelist;

#define dqinsheadfree(DQP) { \
	(DQP)->dq_freef = dqfreelist.dq_freef; \
	(DQP)->dq_freeb = &dqfreelist; \
	dqfreelist.dq_freef->dq_freeb = (DQP); \
	dqfreelist.dq_freef = (DQP); \
}

#define dqinstailfree(DQP) { \
	(DQP)->dq_freeb = dqfreelist.dq_freeb; \
	(DQP)->dq_freef = &dqfreelist; \
	dqfreelist.dq_freeb->dq_freef = (DQP); \
	dqfreelist.dq_freeb = (DQP); \
}

#define dqremfree(DQP) { \
	(DQP)->dq_freeb->dq_freef = (DQP)->dq_freef; \
	(DQP)->dq_freef->dq_freeb = (DQP)->dq_freeb; \
}

typedef	struct dquot *DQptr;

/*
 * Initialize quota caches.
 */
void
qtinit()
{
	register struct dqhead *dhp;
	register struct dquot *dqp;

	/*
	 * Initialize the cache between the in-core structures
	 * and the per-filesystem quota files on disk.
	 */
	for (dhp = &dqhead[0]; dhp < &dqhead[NDQHASH]; dhp++) {
		dhp->dqh_forw = dhp->dqh_back = (DQptr)dhp;
	}
	dqfreelist.dq_freef = dqfreelist.dq_freeb = (DQptr)&dqfreelist;
	for (dqp = dquot; dqp < dquotNDQUOT; dqp++) {
		dqp->dq_forw = dqp->dq_back = dqp;
		dqinsheadfree(dqp);
	}
}

/*
 * Obtain the user's on-disk quota limit for filesystem specified.
 */
struct dquot *
getdiskquota(uid, mp)
	register struct mount *mp;
{
	register struct dquot *dqp;
	register struct dqhead *dhp;
	int error;

	if (mp->m_qinod == NULL || mp->m_qflags & Q_CLOSING)
		return (NULL);
	/*
	 * Check the cache first.
	 */
	dhp = &dqhead[DQHASH(uid, mp)];
	for (dqp = dhp->dqh_forw; dqp != (DQptr)dhp; dqp = dqp->dq_forw) {
		if (dqp->dq_uid != uid || dqp->dq_mp != mp)
			continue;
		/*
		 * Cache hit with no references.
		 * Take the structure off the free list.
		 */
		if (dqp->dq_cnt == 0) {
			dqremfree(dqp);
		}
		dqp->dq_cnt++;
		return (dqp);
	}
	/*
	 * Not in cache.
	 * Get dqot at head of free list.
	 */
	if ((dqp = dqfreelist.dq_freef) == &dqfreelist) {
		tablefull("dquot");
		u.u_error = EUSERS;
		return (NULL);
	}
	/*
	 * This shouldn't happen, as we sync dquots before freeing them.
	 */
	if (dqp->dq_flags & DQ_MOD)
		panic("diskquota");
	/*
	 * Take it off the free list, and off the hash chain it was on.
	 */
	dqremfree(dqp);
	remque(dqp);
	dqp->dq_cnt = 1;
	dqp->dq_flags = 0;
	dqp->dq_uid = uid;
	dqp->dq_mp = mp;
	/*
	 * Read quota info off disk.
	 */
	error =
	    rdwri(UIO_READ, mp->m_qinod,
		(caddr_t)&dqp->dq_dqb, sizeof(struct dqblk), dqoff(uid),
		UIOSEG_KERNEL, (int *)0);
	if (error) {
		/*
		 * I/O error in reading quota file.
		 * Put dquot on a private, unfindable hash list, release
		 * it back to free list and reflect problem to caller.
		 */
		dqp->dq_forw = dqp;
		dqp->dq_back = dqp;
		dqrele(dqp);
		return (NULL);
	}
	/*
	 * Put dquot on correct has list.
	 */
	insque(dqp, dhp);
	return (dqp);
}

/*
 * Release dquot.
 */
void
dqrele(dqp)
	register struct dquot *dqp;
{

	if (dqp == NULL)
		return;
	if (dqp->dq_cnt == 0)
		panic("dqrele");
	if (dqp->dq_cnt == 1) {
		if (dqp->dq_flags & DQ_MOD) {
			dqupdate(dqp);
		}
		/*
		 * Make sure ref count is still 1 after sleeping for i/o.
		 */
		if (dqp->dq_cnt == 1)
			dqinstailfree(dqp);
	}
	dqp->dq_cnt--;
}

/*
 * Update on disk quota info.
 */
void
dqupdate(dqp)
	register struct dquot *dqp;
{

	if (dqp->dq_flags & DQ_MOD) {
		register struct inode *qip;

		dqp->dq_flags &= ~DQ_MOD;
		qip = dqp->dq_mp->m_qinod;
		if (qip == NULL)
			panic("dqupdate");
		(void) rdwri(UIO_WRITE, qip,
		    (caddr_t)&dqp->dq_dqb,
		    sizeof (struct dqblk), dqoff(dqp->dq_uid),
		    UIOSEG_KERNEL, (int *)0);
	}
}
