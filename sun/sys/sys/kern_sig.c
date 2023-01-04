/*	@(#)kern_sig.c 1.1 86/09/25 SMI; from UCB 5.23 83/06/24	*/

#include "../machine/pte.h"
#include "../machine/psl.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/proc.h"
#include "../h/timeb.h"
#include "../h/times.h"
#include "../h/buf.h"
#include "../h/text.h"
#include "../h/seg.h"
#include "../h/vm.h"
#include "../h/acct.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/core.h"

#ifdef sun
#include "fpa.h"
#if NFPA > 0
#include "../sundev/fpareg.h"
#endif NFPA > 0
#endif sun

#define	cantmask	(sigmask(SIGKILL)|sigmask(SIGCONT)|sigmask(SIGSTOP))

/*
 * Generalized interface signal handler.
 */
sigvec()
{
	register struct a {
		int	signo;
		struct	sigvec *nsv;
		struct	sigvec *osv;
	} *uap = (struct a  *)u.u_ap;
	struct sigvec vec;
	register struct sigvec *sv;
	register int sig;
	int bit;

	sig = uap->signo;
	if (sig <= 0 || sig >= NSIG || sig == SIGKILL || sig == SIGSTOP) {
		u.u_error = EINVAL;
		return;
	}
	sv = &vec;
	if (uap->osv) {
		sv->sv_handler = u.u_signal[sig];
		sv->sv_mask = u.u_sigmask[sig];
		bit = sigmask(sig);
		sv->sv_flags = 0;
		if ((u.u_sigonstack & bit) != 0)
			sv->sv_flags |= SV_ONSTACK;
		if ((u.u_sigintr & bit) != 0)
			sv->sv_flags |= SV_INTERRUPT;
		if ((u.u_sigreset & bit) != 0)
			sv->sv_flags |= SV_RESETHAND;
		u.u_error =
		    copyout((caddr_t)sv, (caddr_t)uap->osv, sizeof (vec));
		if (u.u_error)
			return;
	}
	if (uap->nsv) {
		u.u_error =
		    copyin((caddr_t)uap->nsv, (caddr_t)sv, sizeof (vec));
		if (u.u_error)
			return;
		if (sig == SIGCONT && sv->sv_handler == SIG_IGN) {
			u.u_error = EINVAL;
			return;
		}
		setsigvec(sig, sv);
	}
}

setsigvec(sig, sv)
	int sig;
	register struct sigvec *sv;
{
	register struct proc *p;
	register int bit;

	bit = sigmask(sig);
	p = u.u_procp;
	/*
	 * Change setting atomically.
	 */
	(void) spl6();
	u.u_signal[sig] = sv->sv_handler;
	u.u_sigmask[sig] = sv->sv_mask &~ cantmask;
	if (sv->sv_flags & SV_INTERRUPT)
		u.u_sigintr |= bit;
	else
		u.u_sigintr &= ~bit;
	if (sv->sv_flags & SV_ONSTACK)
		u.u_sigonstack |= bit;
	else
		u.u_sigonstack &= ~bit;
	if (sv->sv_flags & SV_RESETHAND)
		u.u_sigreset |= bit;
	else
		u.u_sigreset &= ~bit;
	if (sv->sv_handler == SIG_IGN) {
		p->p_sig &= ~bit;		/* never to be seen again */
		p->p_sigignore |= bit;
		p->p_sigcatch &= ~bit;
	} else {
		p->p_sigignore &= ~bit;
		if (sv->sv_handler == SIG_DFL)
			p->p_sigcatch &= ~bit;
		else
			p->p_sigcatch |= bit;
	}
	(void) spl0();
}

sigblock()
{
	struct a {
		int	mask;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;

	(void) spl6();
	u.u_r.r_val1 = p->p_sigmask;
	p->p_sigmask |= uap->mask &~ cantmask;
	(void) spl0();
}

sigsetmask()
{
	struct a {
		int	mask;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;

	(void) spl6();
	u.u_r.r_val1 = p->p_sigmask;
	p->p_sigmask = uap->mask &~ cantmask;
	(void) spl0();
}

sigpause()
{
	struct a {
		int	mask;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;

	/*
	 * When returning from sigpause, we want
	 * the old mask to be restored after the
	 * signal handler has finished.  Thus, we
	 * save it here and mark the proc structure
	 * to indicate this (should be in u.).
	 */
	u.u_oldmask = p->p_sigmask;
	p->p_flag |= SOMASK;
	p->p_sigmask = uap->mask &~ cantmask;
	for (;;)
		(void) sleep((caddr_t)&u, PSLEP);
	/*NOTREACHED*/
}
#undef cantmask

sigstack()
{
	register struct a {
		struct	sigstack *nss;
		struct	sigstack *oss;
	} *uap = (struct a *)u.u_ap;
	struct sigstack ss;

	if (uap->oss) {
		u.u_error = copyout((caddr_t)&u.u_sigstack, (caddr_t)uap->oss, 
		    sizeof (struct sigstack));
		if (u.u_error)
			return;
	}
	if (uap->nss) {
		u.u_error =
		    copyin((caddr_t)uap->nss, (caddr_t)&ss, sizeof (ss));
		if (u.u_error == 0)
			u.u_sigstack = ss;
	}
}

kill()
{
	register struct a {
		int	pid;
		int	signo;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p;

	if (uap->signo < 0 || uap->signo > NSIG) {
		u.u_error = EINVAL;
		return;
	}
	if (uap->pid > 0) {
		/* kill single process */
		p = pfind(uap->pid);
		if (p == 0) {
			u.u_error = ESRCH;
			return;
		}
		if (u.u_uid && u.u_uid != p->p_uid)
			u.u_error = EPERM;
		else if (uap->signo)
			psignal(p, uap->signo);
		return;
	}
	switch (uap->pid) {
	case -1:		/* broadcast signal */
		u.u_error = killpg1(uap->signo, 0, 1);
		break;
	case 0:			/* signal own process group */
		u.u_error = killpg1(uap->signo, 0, 0);
		break;
	default:		/* negative explicit process group */
		u.u_error = killpg1(uap->signo, -uap->pid, 0);
		break;
	}
	return;
}

killpg()
{
	register struct a {
		int	pgrp;
		int	signo;
	} *uap = (struct a *)u.u_ap;

	if (uap->signo < 0 || uap->signo > NSIG) {
		u.u_error = EINVAL;
		return;
	}
	u.u_error = killpg1(uap->signo, uap->pgrp, 0);
}

/* KILL CODE SHOULDNT KNOW ABOUT PROCESS INTERNALS !?! */

killpg1(signo, pgrp, all)
	int signo, pgrp, all;
{
	register struct proc *p;
	int f, error = 0;

	if (!all && pgrp == 0) {
		/*
		 * Zero process id means send to my process group.
		 */
		pgrp = u.u_procp->p_pgrp;
		if (pgrp == 0)
			return (ESRCH);
	}
	for (f = 0, p = proc; p < procNPROC; p++) {
		if (p->p_stat == NULL)
			continue;
		/*
		 * If not sending to all processes, only send to processes
		 * in the specified process group.
		 * Don't send to "init" (child of process 0).
		 * Don't send to system processes.
		 * If sending to all processes, don't send to ourselves.
		 */
		if ((p->p_pgrp != pgrp && !all) || p->p_ppid == 0 ||
		    (p->p_flag&SSYS) || (all && p == u.u_procp))
			continue;
		if (u.u_uid != 0 && u.u_uid != p->p_uid &&
		    (signo != SIGCONT || !inferior(p))) {
			if (!all)
				error = EPERM;
			continue;
		}
		f++;
		if (signo)
			psignal(p, signo);
	}
	return (error ? error : (f == 0 ? ESRCH : 0));
}

/*
 * Send the specified signal to
 * all processes with 'pgrp' as
 * process group.
 */
gsignal(pgrp, sig)
	register int pgrp;
{
	register struct proc *p;

	if (pgrp == 0)
		return;
	for(p = proc; p < procNPROC; p++)
		if (p->p_pgrp == pgrp)
			psignal(p, sig);
}

/*
 * Send the specified signal to
 * the specified process.
 */
psignal(p, sig)
	register struct proc *p;
	register int sig;
{
	register int s;
	register int (*action)();
	int mask;

	if ((unsigned)sig >= NSIG)
		return;
	mask = sigmask(sig);

	/*
	 * If proc is traced, always give parent a chance.
	 */
	if (p->p_flag & STRC)
		action = SIG_DFL;
	else {
		/*
		 * If the signal is being ignored,
		 * then we forget about it immediately.
		 */
		if (p->p_sigignore & mask)
			return;
		if (p->p_sigmask & mask)
			action = SIG_HOLD;
		else if (p->p_sigcatch & mask)
			action = SIG_CATCH;
		else
			action = SIG_DFL;
	}
#define	stops	(sigmask(SIGSTOP)|sigmask(SIGTSTP)| \
			sigmask(SIGTTIN)|sigmask(SIGTTOU))
	if (sig) {
		p->p_sig |= mask;
		switch (sig) {

		case SIGTERM:
			if ((p->p_flag&STRC) || action != SIG_DFL)
				break;
			/* fall into ... */

		case SIGKILL:
			if (p->p_nice > NZERO)
				p->p_nice = NZERO;
			break;

		case SIGCONT:
			p->p_sig &= ~stops;
			break;

		case SIGSTOP:
		case SIGTSTP:
		case SIGTTIN:
		case SIGTTOU:
			p->p_sig &= ~sigmask(SIGCONT);
			break;
		}
	}
#undef stops
	/*
	 * Defer further processing for signals which are held.
	 */
	if (action == SIG_HOLD)
		return;
	s = spl6();
	switch (p->p_stat) {

	case SSLEEP:
		/*
		 * If process is sleeping at negative priority
		 * we can't interrupt the sleep... the signal will
		 * be noticed when the process returns through
		 * trap() or syscall().
		 */
		if (p->p_pri <= PZERO)
			goto out;
		/*
		 * Process is sleeping and traced... make it runnable
		 * so it can discover the signal in issig() and stop
		 * for the parent.
		 */
		if (p->p_flag&STRC)
			goto run;
		switch (sig) {

		case SIGSTOP:
		case SIGTSTP:
		case SIGTTIN:
		case SIGTTOU:
			/*
			 * These are the signals which by default
			 * stop a process.
			 */
			if (action != SIG_DFL)
				goto run;
			/*
			 * Don't clog system with children of init
			 * stopped from the keyboard.
			 */
			if (sig != SIGSTOP && p->p_pptr == &proc[1]) {
				psignal(p, SIGKILL);
				p->p_sig &= ~mask;
				(void) splx(s);
				return;
			}
			/*
			 * If a child in vfork(), stopping could
			 * cause deadlock.
			 */
			if (p->p_flag&SVFORK)
				goto out;
			p->p_sig &= ~mask;
			p->p_cursig = sig;
			stop(p);
			goto out;

		case SIGIO:
		case SIGURG:
		case SIGCHLD:
		case SIGWINCH:
			/*
			 * These signals are special in that they
			 * don't get propogated... if the process
			 * isn't interested, forget it.
			 */
			if (action != SIG_DFL)
				goto run;
			p->p_sig &= ~mask;		/* take it away */
			goto out;

		default:
			/*
			 * All other signals cause the process to run
			 */
			goto run;
		}
		/*NOTREACHED*/

	case SSTOP:
		/*
		 * If traced process is already stopped,
		 * then no further action is necessary.
		 */
		if (p->p_flag&STRC)
			goto out;
		switch (sig) {

		case SIGKILL:
			/*
			 * Kill signal always sets processes running.
			 */
			goto run;

		case SIGCONT:
			/*
			 * If the process catches SIGCONT, let it handle
			 * the signal itself.  If it isn't waiting on
			 * an event, then it goes back to run state.
			 * Otherwise, process goes back to sleep state.
			 */
			if (action != SIG_DFL || p->p_wchan == 0)
				goto run;
			p->p_stat = SSLEEP;
			goto out;

		case SIGSTOP:
		case SIGTSTP:
		case SIGTTIN:
		case SIGTTOU:
			/*
			 * Already stopped, don't need to stop again.
			 * (If we did the shell could get confused.)
			 */
			p->p_sig &= ~mask;		/* take it away */
			goto out;

		default:
			/*
			 * If process is sleeping interruptibly, then
			 * unstick it so that when it is continued
			 * it can look at the signal.
			 * But don't setrun the process as its not to
			 * be unstopped by the signal alone.
			 */
			if (p->p_wchan && p->p_pri > PZERO)
				unsleep(p);
			goto out;
		}
		/*NOTREACHED*/

	default:
		/*
		 * SRUN, SIDL, SZOMB do nothing with the signal,
		 * other than kicking ourselves if we are running.
		 * It will either never be noticed, or noticed very soon.
		 */
		if (p == u.u_procp && !noproc)
#ifdef vax
#include "../vax/mtpr.h"
#endif vax
			aston();
		goto out;
	}
	/*NOTREACHED*/
run:
	/*
	 * Raise priority to at least PUSER.
	 */
	if (p->p_pri > PUSER)
		if ((p != u.u_procp || noproc) && p->p_stat == SRUN &&
		    (p->p_flag & SLOAD)) {
			remrq(p);
			p->p_pri = PUSER;
			setrq(p);
		} else
			p->p_pri = PUSER;
	setrun(p);
out:
	(void) splx(s);
}

/*
 * Returns true if the current
 * process has a signal to process.
 * The signal to process is put in p_cursig.
 * This is asked at least once each time a process enters the
 * system (though this can usually be done without actually
 * calling issig by checking the pending signal masks.)
 * A signal does not do anything
 * directly to a process; it sets
 * a flag that asks the process to
 * do something to itself.
 */
issig()
{
	register struct proc *p;
	register int sig;
	int sigbits, mask;

	p = u.u_procp;
	for (;;) {
		sigbits = p->p_sig &~ p->p_sigmask;
		if ((p->p_flag&STRC) == 0)
			sigbits &= ~p->p_sigignore;
		if (p->p_flag&SVFORK)
#define bit(a) (1<<(a-1))
			sigbits &= ~(bit(SIGSTOP)|bit(SIGTSTP)|bit(SIGTTIN)|bit(SIGTTOU));
		if (sigbits == 0)
			break;
		sig = ffs((long)sigbits);
		mask = sigmask(sig);
		p->p_sig &= ~mask;		/* take the signal! */
		p->p_cursig = sig;
		if (p->p_flag&STRC && (p->p_flag&SVFORK) == 0) {
			/*
			 * If traced, always stop, and stay
			 * stopped until released by the parent.
			 */
			do {
				stop(p);
				swtch();
			} while (!procxmt() && p->p_flag&STRC);

			/*
			 * If the traced bit got turned off,
			 * then put the signal taken above back into p_sig
			 * and go back up to the top to rescan signals.
			 * This ensures that p_sig* and u_signal are consistent.
			 */
			if ((p->p_flag&STRC) == 0) {
/*
				p->p_sig |= mask;
*/
				continue;
			}

			/*
			 * If parent wants us to take the signal,
			 * then it will leave it in p->p_cursig;
			 * otherwise we just look for signals again.
			 */
			sig = p->p_cursig;
			if (sig == 0)
				continue;

			/*
			 * If signal is being masked put it back
			 * into p_sig and look for other signals.
			 */
			mask = sigmask(sig);
			if (p->p_sigmask & mask) {
				p->p_sig |= mask;
				continue;
			}
		}
		switch (u.u_signal[sig]) {

		case SIG_DFL:
			/*
			 * Don't take default actions on system processes.
			 */
			if (p->p_ppid == 0)
				break;
			switch (sig) {

			case SIGTSTP:
			case SIGTTIN:
			case SIGTTOU:
				/*
				 * Children of init aren't allowed to stop
				 * on signals from the keyboard.
				 */
				if (p->p_pptr == &proc[1]) {
					psignal(p, SIGKILL);
					continue;
				}
				/* fall into ... */

			case SIGSTOP:
				if (p->p_flag&STRC)
					continue;
				stop(p);
				swtch();
				continue;

			case SIGCONT:
			case SIGCHLD:
			case SIGURG:
			case SIGIO:
			case SIGWINCH:
				/*
				 * These signals are normally not
				 * sent if the action is the default.
				 */
				continue;		/* == ignore */

			default:
				goto send;
			}
			/*NOTREACHED*/

		case SIG_HOLD:
		case SIG_IGN:
			/*
			 * Masking above should prevent us
			 * ever trying to take action on a held
			 * or ignored signal, unless process is traced.
			 */
			if ((p->p_flag&STRC) == 0)
				printf("issig\n");
			continue;

		default:
			/*
			 * This signal has an action, let
			 * psig process it.
			 */
			goto send;
		}
		/*NOTREACHED*/
	}
	/*
	 * Didn't find a signal to send.
	 */
	p->p_cursig = 0;
	return (0);

send:
	/*
	 * Let psig process the signal.
	 */
	return (sig);
}

/*
 * Put the argument process into the stopped
 * state and notify the parent via wakeup and/or signal.
 */
stop(p)
	register struct proc *p;
{

	p->p_stat = SSTOP;
	p->p_flag &= ~SWTED;
	/*
	 * Avoid sending signal to parent if process is traced
	 */
	if (p->p_flag&STRC) {
		wakeup((caddr_t)p->p_tptr);
		return;
	}
	wakeup((caddr_t)p->p_pptr);
	psignal(p->p_pptr, SIGCHLD);
}

/*
 * Perform the action specified by
 * the current signal.
 * The usual sequence is:
 *	if (issig())
 *		psig();
 * The signal bit has already been cleared by issig,
 * and the current signal number stored in p->p_cursig.
 */
psig()
{
	register struct proc *p = u.u_procp;
	register int sig = p->p_cursig;
	int mask = sigmask(sig), returnmask;
	register int (*action)();

	if (sig == 0)
		panic("psig");
	action = u.u_signal[sig];
	if (action != SIG_DFL) {
		if (action == SIG_IGN || (p->p_sigmask & mask))
			panic("psig action");
		u.u_error = 0;
		/*
		 * Set the new mask value and also defer further
		 * occurences of this signal (unless we're simulating
		 * the old signal facilities). 
		 *
		 * Special case: user has done a sigpause.  Here the
		 * current mask is not of interest, but rather the
		 * mask from before the sigpause is what we want restored
		 * after the signal processing is completed.
		 */
		(void) spl6();
		if (u.u_sigreset & mask) {
			if (sig != SIGILL && sig != SIGTRAP) {
				u.u_signal[sig] = SIG_DFL;
				p->p_sigcatch &= ~mask;
			}
			mask = 0;
		}
		if (p->p_flag & SOMASK) {
			returnmask = u.u_oldmask;
			p->p_flag &= ~SOMASK;
		} else
			returnmask = p->p_sigmask;
		p->p_sigmask |= u.u_sigmask[sig] | mask;
		(void) spl0();
		u.u_ru.ru_nsignals++;
		sendsig(action, sig, returnmask);
		p->p_cursig = 0;
		return;
	}
	u.u_acflag |= AXSIG;
	switch (sig) {

	case SIGILL:
	case SIGIOT:
	case SIGBUS:
	case SIGQUIT:
	case SIGTRAP:
	case SIGEMT:
	case SIGFPE:
	case SIGSEGV:
	case SIGSYS:
		u.u_arg[0] = sig;
		if (core())
			sig += 0200;
	}
	exit(sig);
}

/*
 * Create a core image on the file "core"
 * If you are looking for protection glitches,
 * there are probably a wealth of them here
 * when this occurs to a suid command.
 *
 * It writes a struct core
 * followed by the entire
 * data+stack segments
 * and user area.
 */
core()
{
	struct vnode *vp;
	struct vattr vattr;
	struct core *corep;
	int len;
	extern char *strncpy();
	int offset = 0, fd;

	if (u.u_uid != u.u_ruid || u.u_gid != u.u_rgid)
		return (0);
	if (ctob(u.u_dsize + u.u_ssize) + sizeof (struct core) >=
	    u.u_rlimit[RLIMIT_CORE].rlim_cur)
		return (0);
	u.u_error = 0;

	vattr_null(&vattr);
	vattr.va_type = VREG;
	vattr.va_mode = 0644;
#ifdef MORECORE
	u.u_error =
	    vn_create("core.more", UIOSEG_KERNEL, &vattr, NONEXCL, VWRITE, &vp);
#else
	u.u_error =
	    vn_create("core", UIOSEG_KERNEL, &vattr, NONEXCL, VWRITE, &vp);
#endif MORECORE
	if (u.u_error)
		return (0);
	if (vattr.va_nlink != 1) {
		u.u_error = EFAULT;
		goto out;
	}
	/* unmap funky devices in the user address space */
	for (fd = 0; fd < NOFILE; fd++) 
		if (u.u_ofile[fd] && (u.u_pofile[fd] & UF_MAPPED))
			munmapfd(fd);
	vattr_null(&vattr);
	vattr.va_size = 0;
	VOP_SETATTR(vp, &vattr, u.u_cred);
	u.u_acflag |= ACORE;

	/*
	 * Dump the specific areas of the u area into the new
	 * core structure for examination by debuggers.  The
	 * new format is now independent of the user structure and
	 * only the information needed by the debuggers is included.
	 */
	corep = (struct core *)kmem_alloc(sizeof (struct core));
	bzero((caddr_t)corep, sizeof (struct core));
	corep->c_magic = CORE_MAGIC;
	corep->c_len = sizeof (struct core);
	corep->c_regs = *(struct regs *)u.u_ar0;
#ifdef sun
	u.u_fp_status.fps_flags = EXT_FPS_FLAGS(&u.u_fp_istate);
	u.u_fp_status.fps_code = u.u_code;
	corep->c_fpstatus = u.u_fp_status;

#if NFPA > 0
	/*
	 * Dump FPA regs only when u.u_fpa_flags is nonzero and 
	 * FPA_LOAD_BIT is off.
	 * The reason is that if FPA_LOAD_BIT is on, it means that there
	 * is no microcode in FPA board, we cannot access FPA data regs.
	 */
	if ((corep->c_fparegs.fpar_flags = u.u_fpa_flags) &&
	    !(fpa->fp_state & FPA_LOAD_BIT)) {
		/* loop until FPA pipe become stable */
		CDELAY(fpa->fp_pipe_status & FPA_STABLE, 300);
		if (fpa->fp_pipe_status & FPA_STABLE) {
			corep->c_fparegs.fpar_status =
			    *((struct fpa_status *) &fpa->fp_state);
			fpa->fp_clear_pipe = 0; /* clear pipe to read data */
			bcopy((char *)fpa->fp_data,
			    (char *)corep->c_fparegs.fpar_data,
			    sizeof (fpa->fp_data));
		} else {
			fpa_shutdown();
			printf("FPA not stable in core(), FPA is shutdown!\n");
		}
	}
#endif NFPA > 0
#else sun
	corep->c_ucode = u.u_code;
#endif sun
	corep->c_aouthdr = u.u_exdata.Ux_A;
	corep->c_signo = u.u_arg[0];
	corep->c_tsize = ctob(u.u_tsize);
	corep->c_dsize = ctob(u.u_dsize);
	corep->c_ssize = ctob(u.u_ssize);
	len = min(MAXCOMLEN, CORE_NAMELEN);
	(void) strncpy(corep->c_cmdname, u.u_comm, len);
	corep->c_cmdname[len] = '\0';
	u.u_error = vn_rdwr(UIO_WRITE, vp,
	    (caddr_t)corep, sizeof (struct core),
	    0, UIOSEG_KERNEL, IO_UNIT, (int *)0);
	offset += sizeof (struct core);
	kmem_free((caddr_t)corep, sizeof (struct core));

#ifdef MORECORE
	/*
	 * snap the page table and text segment too
	 */
	if (u.u_error == 0)
		u.u_error = vn_rdwr(UIO_WRITE, vp,
		    (caddr_t)tptopte(u.u_procp, 0),
		    ctob(CLSIZE),
		    offset, UIOSEG_KERNEL, IO_UNIT, (int *)0);
	offset += ctob(CLSIZE);

	if (u.u_error == 0) {
		u.u_error = vn_rdwr(UIO_WRITE, vp,
		    (caddr_t)ctob(tptov(u.u_procp, 0)),
		    ctob(u.u_tsize),
		    offset, UIOSEG_USER, IO_UNIT, (int *)0);
	}
	offset += ctob(u.u_tsize);

#endif MORECORE
	if (u.u_error == 0)
#ifdef sun
		if (u.u_hole.uh_last != 0) {
			u.u_error = vn_rdwr(UIO_WRITE, vp,
			    (caddr_t)ctob(dptov(u.u_procp, 0)),
			    ctob(u.u_hole.uh_first),
			    offset, UIOSEG_USER, IO_UNIT, (int *)0);

			if (u.u_error == 0)
				u.u_error = vn_rdwr(UIO_WRITE, vp,
				    (caddr_t)ctob(dptov(u.u_procp,
				    u.u_hole.uh_last + CLSIZE)),
				    ctob(u.u_dsize - u.u_hole.uh_last - CLSIZE),
				    offset + ctob(u.u_hole.uh_last + CLSIZE),
				    UIOSEG_USER, IO_UNIT, (int *)0);
		} else
#endif
		u.u_error = vn_rdwr(UIO_WRITE, vp,
		    (caddr_t)ctob(dptov(u.u_procp, 0)),
		    ctob(u.u_dsize),
		    offset, UIOSEG_USER, IO_UNIT, (int *)0);
	offset += ctob(u.u_dsize);

	if (u.u_error == 0)
		u.u_error = vn_rdwr(UIO_WRITE, vp,
		    (caddr_t)ctob(sptov(u.u_procp, u.u_ssize - 1)),
		    ctob(u.u_ssize),
		    offset, UIOSEG_USER, IO_UNIT, (int *)0);
	offset += ctob(u.u_ssize);

	if (u.u_error == 0)
		u.u_error = vn_rdwr(UIO_WRITE, vp,
		    (caddr_t)&u, ctob(UPAGES),
		    offset, UIOSEG_KERNEL, IO_UNIT, (int *)0);
out:
	VN_RELE(vp);
	return (u.u_error == 0);
}
