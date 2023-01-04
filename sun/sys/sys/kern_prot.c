/*	@(#)kern_prot.c 1.1 86/09/25 SMI; from UCB 5.17 83/05/27	*/

/*
 * System calls related to processes and protection
 */

#include "../machine/reg.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/proc.h"
#include "../h/timeb.h"
#include "../h/times.h"
#include "../h/reboot.h"
#include "../h/buf.h"
#include "../h/acct.h"

getpid()
{

	u.u_r.r_val1 = u.u_procp->p_pid;
	u.u_r.r_val2 = u.u_procp->p_ppid;
}

getpgrp()
{
	register struct a {
		int	pid;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p;

	if (uap->pid == 0)
		uap->pid = u.u_procp->p_pid;
	p = pfind(uap->pid);
	if (p == 0) {
		u.u_error = ESRCH;
		return;
	}
	u.u_r.r_val1 = p->p_pgrp;
}

getuid()
{

	u.u_r.r_val1 = u.u_ruid;
	u.u_r.r_val2 = u.u_uid;
}

getgid()
{

	u.u_r.r_val1 = u.u_rgid;
	u.u_r.r_val2 = u.u_gid;
}

getgroups()
{
	register struct	a {
		u_int	gidsetsize;
		int	*gidset;
	} *uap = (struct a *)u.u_ap;
	register int *gp;

	for (gp = &u.u_groups[NGROUPS]; gp > u.u_groups; gp--)
		if (gp[-1] >= 0)
			break;
	if (uap->gidsetsize < gp - u.u_groups) {
		u.u_error = EINVAL;
		return;
	}
	uap->gidsetsize = gp - u.u_groups;
	u.u_error = copyout((caddr_t)u.u_groups, (caddr_t)uap->gidset,
	    uap->gidsetsize * sizeof (u.u_groups[0]));
	if (u.u_error)
		return;
	u.u_r.r_val1 = uap->gidsetsize;
}

setpgrp()
{
	register struct proc *p;
	register struct a {
		int	pid;
		int	pgrp;
	} *uap = (struct a *)u.u_ap;

	if (uap->pid == 0)
		uap->pid = u.u_procp->p_pid;
	p = pfind(uap->pid);
	if (p == 0) {
		u.u_error = ESRCH;
		return;
	}
/* need better control mechanisms for process groups */
	if (p->p_uid != u.u_uid && u.u_uid && !inferior(p)) {	/* XXX - should be p_suid */
		u.u_error = EPERM;
		return;
	}
	p->p_pgrp = uap->pgrp;
}

setreuid()
{
	struct a {
		int	ruid;
		int	euid;
	} *uap;
	register int ruid, euid;

	uap = (struct a *)u.u_ap;
	ruid = uap->ruid;
	if (ruid != -1) {
		if (u.u_ruid != ruid && u.u_uid != ruid && !suser())
			return;
	}
	euid = uap->euid;
	if (euid != -1) {
		if (u.u_ruid != euid && u.u_uid != euid
		    && u.u_procp->p_suid != euid && !suser())
			return;
	}
	/*
	 * Everything's okay, do it.
	 */
	u.u_cred = crcopy(u.u_cred);
	if (ruid != -1) {
		u.u_procp->p_suid = ruid;
		u.u_ruid = ruid;
	}
	if (euid != -1) {
		u.u_procp->p_uid = euid;	/* XXX - should be ruid */
		u.u_uid = euid;
	}
}

setregid()
{
	register struct a {
		int	rgid;
		int	egid;
	} *uap;
	register int rgid, egid;

	uap = (struct a *)u.u_ap;
	rgid = uap->rgid;
	if (rgid == -1)
		rgid = u.u_rgid;
	if (u.u_rgid != rgid && u.u_gid != rgid && !suser())
		return;
	egid = uap->egid;
	if (egid == -1)
		egid = u.u_gid;
	if (u.u_rgid != egid && u.u_gid != egid && !suser())
		return;
	u.u_cred = crcopy(u.u_cred);
	if (u.u_rgid != rgid) {
		leavegroup(u.u_rgid);
		(void) entergroup(rgid);
		u.u_rgid = rgid;
	}
	u.u_gid = egid;
}

setgroups()
{
	register struct	a {
		u_int	gidsetsize;
		int	*gidset;
	} *uap = (struct a *)u.u_ap;
	register int *gp;
	struct ucred *newcr, *tmpcr;

	if (!suser())
		return;
	if (uap->gidsetsize > sizeof (u.u_groups) / sizeof (u.u_groups[0])) {
		u.u_error = EINVAL;
		return;
	}
	newcr = crdup(u.u_cred);
	u.u_error = copyin((caddr_t)uap->gidset, (caddr_t)newcr->cr_groups,
	    uap->gidsetsize * sizeof (newcr->cr_groups[0]));
	if (u.u_error) {
		crfree(newcr);
		return;
	}
	tmpcr = u.u_cred;
	u.u_cred = newcr;
	crfree(tmpcr);
	for (gp = &u.u_groups[uap->gidsetsize]; gp < &u.u_groups[NGROUPS]; gp++)
		*gp = NOGROUP;
}

/*
 * Group utility functions.
 */

/*
 * Delete gid from the group set.
 */
leavegroup(gid)
	int gid;
{
	register int *gp;

	for (gp = u.u_groups; gp < &u.u_groups[NGROUPS]; gp++)
		if (*gp == gid)
			goto found;
	return;
found:
	for (; gp < &u.u_groups[NGROUPS-1]; gp++)
		*gp = *(gp+1);
	*gp = NOGROUP;
}

/*
 * Add gid to the group set.
 */
entergroup(gid)
	int gid;
{
	register int *gp;

	for (gp = u.u_groups; gp < &u.u_groups[NGROUPS]; gp++)
		if (*gp == gid)
			return (0);
	for (gp = u.u_groups; gp < &u.u_groups[NGROUPS]; gp++)
		if (*gp < 0) {
			*gp = gid;
			return (0);
		}
	return (-1);
}

/*
 * Check if gid is a member of the group set.
 */
groupmember(gid)
	int gid;
{
	register int *gp;

	if (u.u_gid == gid)
		return (1);
	for (gp = u.u_groups; gp < &u.u_groups[NGROUPS] && *gp != NOGROUP; gp++)
		if (*gp == gid)
			return (1);
	return (0);
}

/*
 * Test if the current user is the super user.
 */
suser()
{
	if (u.u_uid == 0) {
		u.u_acflag |= ASU;
		return (1);
	}
	u.u_error = EPERM;
	return (0);
}

/*
 * Routines to allocate and free credentials structures
 */

int cractive = 0;

struct credlist {
	union {
		struct ucred cru_cred;
		struct credlist *cru_next;
	} cl_U;
#define	cl_cred	cl_U.cru_cred
#define	cl_next	cl_U.cru_next
};

struct credlist *crfreelist = NULL;

/*
 * Allocate a zeroed cred structure and crhold it.
 */
struct ucred *
crget()
{
	register struct ucred *cr;

	if (crfreelist) {
		cr = &crfreelist->cl_cred;
		crfreelist = ((struct credlist *)cr)->cl_next;
	} else {
		cr = (struct ucred *)kmem_alloc((u_int)sizeof(*cr));
	}
	bzero((caddr_t)cr, sizeof(*cr));
	crhold(cr);
	cractive++;
	return(cr);
}

/*
 * Free a cred structure.
 * Throws away space when ref count gets to 0.
 */
crfree(cr)
	struct ucred *cr;
{
	int	s = spl6();

	if (--cr->cr_ref != 0) {
		(void) splx(s);
		return;
	}
	((struct credlist *)cr)->cl_next = crfreelist;
	crfreelist = (struct credlist *)cr;
	cractive--;
	(void) splx(s);
}

/*
 * Copy cred structure to a new one and free the old one.
 */
struct ucred *
crcopy(cr)
	struct ucred *cr;
{
	struct ucred *newcr;

	newcr = crget();
	*newcr = *cr;
	crfree(cr);
	newcr->cr_ref = 1;
	return(newcr);
}

/*
 * Dup cred struct to a new held one.
 */
struct ucred *
crdup(cr)
	struct ucred *cr;
{
	struct ucred *newcr;

	newcr = crget();
	*newcr = *cr;
	newcr->cr_ref = 1;
	return(newcr);
}
