/*	@(#)kern_synch.c 1.1 86/09/25 SMI; from UCB 4.26 83/05/21	*/

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/vnode.h"
#include "../h/vm.h"
#include "../h/kernel.h"
#include "../h/buf.h"

#ifdef vax
#include "../vax/mtpr.h"	/* XXX */
#endif
/*
 * Force switch among equal priority processes every 100ms.
 */
roundrobin()
{

	runrun++;
	aston();
	timeout(roundrobin, (caddr_t)0, hz / 10);
}

/* constants to digital decay and forget 90% of usage in 5*loadav time */
int	nrscale = 2;
#ifdef vax
/* vax C compiler won't convert float to int for initialization */
long	ccpu = 95*FSCALE/100;		/* exp(-1/20) */
#else
long	ccpu = 0.95122942450071400909 * FSCALE;		/* exp(-1/20) */
#endif

/*
 * Recompute process priorities, once a second
 */
schedcpu()
{
	register struct proc *p;
	register int s, a;
	register long b;

	wakeup((caddr_t)&lbolt);
	for (p = proc; p < procNPROC; p++) if (p->p_stat && p->p_stat!=SZOMB) {
		if (p->p_time != 127)
			p->p_time++;
		if (p->p_stat==SSLEEP || p->p_stat==SSTOP)
			if (p->p_slptime != 127)
				p->p_slptime++;
		if (p->p_flag&SLOAD)
			p->p_pctcpu = ((ccpu * p->p_pctcpu) +
			    ((FSCALE - ccpu) * (p->p_cpticks*FSCALE/hz)))
			    >> FSHIFT;
		p->p_cpticks = 0;
		b = avenrun[0] * nrscale;
		a = ((p->p_cpu & 0377) * b) / (b + FSCALE) + p->p_nice - NZERO;
		if (a < 0)
			a = 0;
		if (a > 255)
			a = 255;
		p->p_cpu = a;
		(void) setpri(p);
		s = spl6();	/* prevent state changes */
		if (p->p_pri >= PUSER) {
			if ((p != u.u_procp || noproc) &&
			    p->p_stat == SRUN &&
			    (p->p_flag & SLOAD) &&
			    p->p_pri != p->p_usrpri) {
				remrq(p);
				p->p_pri = p->p_usrpri;
				setrq(p);
			} else
				p->p_pri = p->p_usrpri;
		}
		(void) splx(s);
	}
	vmmeter();
	if (runin!=0) {
		runin = 0;
		wakeup((caddr_t)&runin);
	}
	if (bclnlist != NULL)
		wakeup((caddr_t)&proc[2]);
	timeout(schedcpu, (caddr_t)0, hz);
}

#define SQSIZE 0100	/* Must be power of 2 */
#define HASH(x)	(( (int) x >> 5) & (SQSIZE-1))
struct prochd slpque[SQSIZE];

/*
 * Give up the processor till a wakeup occurs
 * on chan, at which time the process
 * enters the scheduling queue at priority pri.
 * The most important effect of pri is that when
 * pri<=PZERO a signal cannot disturb the sleep;
 * if pri>PZERO signals will be processed.
 * If pri&PCATCH is set, signals will cause sleep
 * to return 1, rather than longjmp.
 * Callers of this routine must be prepared for
 * premature return, and check that the reason for
 * sleeping has gone away.
 */
int
sleep(chan, pri)
	caddr_t chan;
	int pri;
{
	register struct proc *rp;
	register struct prochd *hp;
	register int s;

	rp = u.u_procp;
	s = spl6();
	if (panicstr) {
		/*
		* After a panic, just give interrupts a chance,
		* then just return; don't run any other procs
		* or panic below, in case this is the idle process
		* and already asleep.
		*/
		(void) spl0();
		goto out;
	}
#ifdef sun
	if (intsvc() || chan==0 || rp->p_stat != SRUN)
#else
	if (chan==0 || rp->p_stat != SRUN)
#endif
		panic("sleep");
	rp->p_wchan = chan;
	rp->p_slptime = 0;
	rp->p_pri = pri & PMASK;
	/*
	 * put at end of sleep queue
	 */
	hp = &slpque[HASH(chan)];
	insque(rp, hp->ph_rlink);
	if (rp->p_pri > PZERO) {
		if (ISSIG(rp)) {
			if (rp->p_wchan)
				unsleep(rp);
			rp->p_stat = SRUN;
			(void) spl0();
			goto psig;
		}
		if (rp->p_wchan == 0)
			goto out;
		rp->p_stat = SSLEEP;
		(void) spl0();
		u.u_ru.ru_nvcsw++;
		swtch();
		if (ISSIG(rp))
			goto psig;
	} else {
		rp->p_stat = SSLEEP;
		(void) spl0();
		u.u_ru.ru_nvcsw++;
		swtch();
	}
out:
	(void) splx(s);
	return (0);

	/*
	 * If priority was low (>PZERO) and
	 * there has been a signal, execute non-local goto through
	 * u.u_qsave, aborting the system call in progress (see trap.c)
	 * unless PCATCH is set, in which case we just return 1 so our
	 * caller can release resources and unwind the system call.
	 */
psig:
	if (pri & PCATCH)
		return (1);
	longjmp(&u.u_qsave);
	/*NOTREACHED*/
}

/*
 * Remove a process from its wait queue
 */
unsleep(p)
	register struct proc *p;
{
	register s;

	s = spl6();
	if (p->p_wchan) {
		remque(p);
		p->p_rlink = 0;
		p->p_wchan = 0;
	}
	(void) splx(s);
}

/*
 * Wake up all processes sleeping on chan.
 */
wakeup(chan)
	register caddr_t chan;
{
	register struct proc *p;
	register struct prochd *hp;
	int s;

	s = spl6();
	hp = &slpque[HASH(chan)];
restart:
	for (p = hp->ph_link; p != (struct proc *)hp; ) {
		if (p->p_stat != SSLEEP && p->p_stat != SSTOP)
			panic("wakeup");
		if (p->p_wchan==chan) {
			remque(p);
			p->p_rlink = 0;
			p->p_wchan = 0;
			p->p_slptime = 0;
			if (p->p_stat == SSLEEP) {
				/* OPTIMIZED INLINE EXPANSION OF setrun(p) */
				p->p_stat = SRUN;
				if (p->p_flag & SLOAD)
					setrq(p);
				if (p->p_pri < curpri) {
					runrun++;
					aston();
				}
				if ((p->p_flag&SLOAD) == 0) {
					if (runout != 0) {
						runout = 0;
						wakeup((caddr_t)&runout);
					}
					wantin++;
				}
				/* END INLINE EXPANSION */
				goto restart;
			}
		} else
			p = p->p_link;
	}
	(void) splx(s);
}

/*
 * Wake up the first process sleeping on chan.
 *
 * Be very sure that the first process is really
 * the right one to wakeup.
 */
wakeup_one(chan)
	register caddr_t chan;
{
	register struct proc *p;
	register struct prochd *hp;
	int s;

	s = spl6();
	hp = &slpque[HASH(chan)];
	for (p = hp->ph_link; p != (struct proc *)hp; ) {
		if (p->p_stat != SSLEEP && p->p_stat != SSTOP)
			panic("wakeup_one");
		if (p->p_wchan==chan) {
			remque(p);
			p->p_rlink = 0;
			p->p_wchan = 0;
			p->p_slptime = 0;
			if (p->p_stat == SSLEEP) {
				/* OPTIMIZED INLINE EXPANSION OF setrun(p) */
				p->p_stat = SRUN;
				if (p->p_flag & SLOAD)
					setrq(p);
				if (p->p_pri < curpri) {
					runrun++;
					aston();
				}
				if ((p->p_flag&SLOAD) == 0) {
					if (runout != 0) {
						runout = 0;
						wakeup((caddr_t)&runout);
					}
					wantin++;
				}
				/* END INLINE EXPANSION */
				break;		/* all done */
			}
		} else
			p = p->p_link;
	}
	(void) splx(s);
}

/*
 * Initialize the (doubly-linked) run queues and sleep queues
 * to be empty.
 */
rqinit()
{
	register int i;

	for (i = 0; i < NQS; i++)
		qs[i].ph_link = qs[i].ph_rlink = (struct proc *)&qs[i];
	for (i = 0; i < SQSIZE; i++)
		slpque[i].ph_link = slpque[i].ph_rlink =
		    (struct proc *)&slpque[i];
}

/*
 * Set the process running;
 * arrange for it to be swapped in if necessary.
 */
setrun(p)
	register struct proc *p;
{
	register int s;

	s = spl6();
	switch (p->p_stat) {

	case 0:
	case SWAIT:
	case SRUN:
	case SZOMB:
	default:
		panic("setrun");

	case SSTOP:
	case SSLEEP:
		unsleep(p);		/* e.g. when sending signals */
		break;

	case SIDL:
		break;
	}
	p->p_stat = SRUN;
	if (p->p_flag & SLOAD)
		setrq(p);
	(void) splx(s);
	if (p->p_pri < curpri) {
		runrun++;
		aston();
	}
	if ((p->p_flag&SLOAD) == 0) {
		if (runout != 0) {
			runout = 0;
			wakeup((caddr_t)&runout);
		}
		wantin++;
	}
}

int fav_nice = -10;

/*
 * Set user priority.
 * The rescheduling flag (runrun)
 * is set if the priority is better
 * than the currently running process.
 */
setpri(pp)
	register struct proc *pp;
{
	register int p;

	p = (pp->p_cpu & 0377)/4;
	p += PUSER + 2*(pp->p_nice - NZERO);
	if (pp->p_flag & SFAVORD)
		p += 2*fav_nice;
	if (pp->p_rssize > pp->p_maxrss && freemem < desfree)
		p += 2*4;	/* effectively, nice(4) */
	if (p > 127)
		p = 127;
	if (p < 1)
		p = 1;
	if (p < curpri) {
		runrun++;
		aston();
	}
	pp->p_usrpri = p;
	return (p);
}
