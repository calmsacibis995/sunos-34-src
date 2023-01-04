/*	@(#)kern_fork.c 1.1 86/09/25 SMI; from UCB 4.3 83/06/14	*/
#include "../machine/reg.h"
#include "../machine/pte.h"
#include "../machine/psl.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/vnode.h"
#include "../h/seg.h"
#include "../h/vm.h"
#include "../h/text.h"
#include "../h/file.h"
#include "../h/acct.h"

#ifdef sun
#include "fpa.h"
#if NFPA > 0
#include "../sundev/fpareg.h"
#endif NFPA > 0
#endif sun

/*
 * fork system call.
 */
fork()
{
	struct dmap cdmap, csmap;	/* temporaries */

	cdmap = zdmap;
	csmap = zdmap;
	if (swpexpand(u.u_dsize, u.u_ssize, &cdmap, &csmap) == 0) {
		u.u_r.r_val2 = 0;
		return;
	}
	u.u_cdmap = &cdmap;
	u.u_csmap = &csmap;
	fork1(0);
	u.u_cdmap = (struct dmap *)0;
	u.u_csmap = (struct dmap *)0;
}

vfork()
{

	fork1(1);
}

fork1(isvfork)
	int isvfork;
{
	register struct proc *p1, *p2;
	register a;
	extern int maxuprc;
#ifdef sun
#if NFPA > 0
	int new_context; /* FPA context of child process */
	struct file *nfp; /* to be assigned to u.u_ofile[] of child */
#endif NFPA > 0
#endif sun

	a = 0;
	p2 = NULL;
	for (p1 = proc; p1 < procNPROC; p1++) {
		if (p1->p_stat==NULL && p2==NULL)
			p2 = p1;
		else {
			if (p1->p_uid==u.u_uid && p1->p_stat!=NULL)
				a++;	/* XXX - should be u.u_ruid, S5-style */
		}
	}
	/*
	 * Disallow if
	 *  No processes at all;
	 *  not su and too many procs owned; or
	 *  not su and would take last slot.
	 */
	if (p2==NULL)
		tablefull("proc");
	if (p2==NULL
	    || (u.u_uid!=0 && (p2==procNPROC-1 || a>maxuprc))) {	/* XXX - should allow zero u.u_ruid also */
		u.u_error = EAGAIN;
		if (!isvfork) {
			(void) vsexpand(0, u.u_cdmap, 1);
			(void) vsexpand(0, u.u_csmap, 1);
		}
		goto out;
	}
#ifdef sun
#if NFPA > 0
	if (u.u_fpa_flags)
		if (fpa_fork_context(p2, &nfp, &new_context) < 0) {
			/* no FPA context, inode, or file , error quit */
			u.u_error = EAGAIN;
			goto out;
		}
#endif NFPA > 0
#endif sun
	p1 = u.u_procp;
	if (newproc(isvfork)) {
#ifdef sun
#if NFPA > 0
#define u_fpas_state u_fpa_status.fpas_state
		if (u.u_fpa_flags) {
			/*
			 * Set up u.u_ofile[] to the opened FPA file
			 * and u.u_fpa_state to the new FPA context
			 * number for the child process.
			 */
			u.u_ofile[u.u_fpa_flags & U_FPA_FDBITS] = nfp;
			u.u_fpas_state = (u.u_fpas_state & FPA_PBITS)
			    | new_context;
			fpa->fp_state = u.u_fpas_state; /* write to FPA */
			/*
			 * Since recomputation will never call fork(),
			 * we avoid the allocation and coping of
			 * FPA exception frame to the child.
			 */
			u.u_fpa_fmtptr = NULL;
		}
#endif NFPA > 0
#endif sun
		u.u_r.r_val1 = p1->p_pid;
		u.u_r.r_val2 = 1;  /* child */
		u.u_start = time.tv_sec;
		u.u_acflag = AFORK;
#ifdef sun
		u.u_hole.uh_first = u.u_hole.uh_last = 0;
#endif

		/*
		 * Child must not inherit file-descriptor-oriented locks
		 * (i.e., System-V style record-locking)
		 */
		for (a = 0; a < NOFILE; a++) {
			u.u_pofile[a] &= ~UF_FDLOCK;
		}

		return;
	}
	u.u_r.r_val1 = p2->p_pid;

out:
	u.u_r.r_val2 = 0;
}

/*
 * Create a new process-- the internal version of
 * sys fork.
 * It returns 1 in the new process, 0 in the old.
 */
newproc(isvfork)
	int isvfork;
{
	register struct proc *p;
	register struct proc *rpp, *rip;
	register int n;
	register struct file *fp;

	p = NULL;
	/*
	 * First, just locate a slot for a process
	 * and copy the useful info from this process into it.
	 * The panic "cannot happen" because fork has already
	 * checked for the existence of a slot.
	 */
retry:
	mpid++;
	if (mpid >= 30000) {
		mpid = 0;
		goto retry;
	}
	for (rpp = proc; rpp < procNPROC; rpp++) {
		if (rpp->p_stat == NULL && p==NULL)
			p = rpp;
		if (rpp->p_pid==mpid || rpp->p_pgrp==mpid)
			goto retry;
	}
	if ((rpp = p) == NULL)
		panic("no procs");

	/*
	 * Make a proc table entry for the new process.
	 */
	rip = u.u_procp;
	rpp->p_stat = SIDL;
	timerclear(&rpp->p_realtimer.it_value);
	rpp->p_flag = SLOAD | (rip->p_flag & (SFAVORD|SPAGI));
	if (isvfork) {
		rpp->p_flag |= SVFORK;
		rpp->p_ndx = rip->p_ndx;
	} else
		rpp->p_ndx = rpp - proc;
	rpp->p_uid = rip->p_uid;
	rpp->p_suid = rip->p_suid;
	rpp->p_pgrp = rip->p_pgrp;
	rpp->p_nice = rip->p_nice;
	rpp->p_textp = isvfork ? 0 : rip->p_textp;
	rpp->p_pid = mpid;
	rpp->p_ppid = rip->p_pid;
	rpp->p_pptr = rip;
	rpp->p_time = 0;
	rpp->p_cpu = 0;
	rpp->p_sigmask = rip->p_sigmask;
	rpp->p_sigcatch = rip->p_sigcatch;
	rpp->p_sigignore = rip->p_sigignore;
	/* take along any pending signals like stops? */
	if (isvfork) {
		rpp->p_tsize = rpp->p_dsize = rpp->p_ssize = 0;
		rpp->p_szpt = clrnd(ctopt(UPAGES));
		forkstat.cntvfork++;
		forkstat.sizvfork += rip->p_dsize + rip->p_ssize;
	} else {
		rpp->p_tsize = rip->p_tsize;
		rpp->p_dsize = rip->p_dsize;
		rpp->p_ssize = rip->p_ssize;
		rpp->p_szpt = rip->p_szpt;
		forkstat.cntfork++;
		forkstat.sizfork += rip->p_dsize + rip->p_ssize;
	}
	rpp->p_rssize = 0;
	rpp->p_maxrss = rip->p_maxrss;
	rpp->p_wchan = 0;
	rpp->p_slptime = 0;
	rpp->p_pctcpu = 0;
	rpp->p_cpticks = 0;
	n = PIDHASH(rpp->p_pid);
	p->p_idhash = pidhash[n];
	pidhash[n] = rpp - proc;
	multprog++;

	/*
	 * Increase reference counts on shared objects.
	 */
	for (n = 0; n < NOFILE; n++) {
		fp = u.u_ofile[n];
		if (fp == NULL)
			continue;
		fp->f_count++;
	}
#ifdef sun
#if NFPA > 0
	/*
	 * F_count associated with /dev/fpa should not be
	 * incremented in the above for loop.
	 */
	if (u.u_fpa_flags)
		u.u_ofile[u.u_fpa_flags & U_FPA_FDBITS]->f_count--;
#endif NFPA > 0
#endif sun
	VN_HOLD(u.u_cdir);
	if (u.u_rdir)
		VN_HOLD(u.u_rdir);
	crhold(u.u_cred);

#ifdef IPCSHMEM
	/* PRE-VM-REWRITE */
	if (!isvfork)
		shmfork(rpp, rip);	/* bump ref-cnts on shared memory */
	/* END ... PRE-VM-REWRITE */
#endif IPCSHMEM

	/*
	 * Partially simulate the environment
	 * of the new process so that when it is actually
	 * created (by copying) it will look right.
	 * This begins the section where we must prevent the parent
	 * from being swapped.
	 */
	rip->p_flag |= SKEEP;
	if (procdup(rpp, isvfork))
		return (1);

	/*
	 * Make child runnable and add to run queue.
	 */
	(void) spl6();
	rpp->p_stat = SRUN;
	setrq(rpp);
	(void) spl0();

	/*
	 * Cause child to take a non-local goto as soon as it runs.
	 * On older systems this was done with SSWAP bit in proc
	 * table; on VAX we use u.u_pcb.pcb_sswap so don't need
	 * to do rpp->p_flag |= SSWAP.  Actually do nothing here.
	 */
	/* rpp->p_flag |= SSWAP; */

	/*
	 * Now can be swapped.
	 */
	rip->p_flag &= ~SKEEP;

	/*
	 * If vfork make chain from parent process to child
	 * (where virtal memory is temporarily).  Wait for
	 * child to finish, steal virtual memory back,
	 * and wakeup child to let it die.
	 */
	if (isvfork) {
		u.u_procp->p_xlink = rpp;
		u.u_procp->p_flag |= SNOVM;
		while (rpp->p_flag & SVFORK)
			(void) sleep((caddr_t)rpp, PZERO - 1);
		if ((rpp->p_flag & SLOAD) == 0)
			panic("newproc vfork");
		uaccess(rpp, Vfmap, &vfutl);
		u.u_procp->p_xlink = 0;
		vpassvm(rpp, u.u_procp, &vfutl, &u, Vfmap);
		vac_flush((caddr_t)&vfutl + KERNSTACK,
		    sizeof (struct user) - KERNSTACK);
		u.u_procp->p_flag &= ~SNOVM;
		rpp->p_ndx = rpp - proc;
		rpp->p_flag |= SVFDONE;
		wakeup((caddr_t)rpp);
	}

	/*
	 * 0 return means parent.
	 */
	return (0);
}
