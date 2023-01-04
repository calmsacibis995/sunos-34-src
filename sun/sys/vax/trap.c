#ifndef lint
static	char sccsid[] = "@(#)trap.c 1.1 86/09/25 SMI"; /* from UCB 6.1 83/08/18 */
#endif

#include "../machine/psl.h"
#include "../machine/reg.h"
#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "assym.s"
#include "../h/proc.h"
#include "../h/seg.h"
#include "../vax/trap.h"
#include "../h/acct.h"
#include "../h/kernel.h"
#ifdef SYSCALLTRACE
#include "../sys/syscalls.c"
#endif

#include "../vax/mtpr.h"

#define	USER	040		/* user-mode flag added to type */

struct	sysent	sysent[];
int	nsysent;

char	*trap_type[] = {
	"Reserved addressing mode",
	"Privileged instruction",
	"Reserved operand",
	"Breakpoint",
	"Xfc trap",
	"Syscall trap",
	"Arithmetic fault",
	"Ast trap",
	"Segmentation fault",
	"Protection fault",
	"Trace trap",
	"Compatibility mode trap",
#ifdef notdef
	"Page fault",
	"Page table fault",
#endif
};
#define	TRAP_TYPES	(sizeof trap_type / sizeof trap_type[0])

/*
 * Called from the trap handler when a processor trap occurs.
 */
/*ARGSUSED*/
trap(sp, type, code, pc, psl)
	int sp, type;
	unsigned code;
	int pc, psl;
{
	register int *locr0 = ((int *)&psl)-PS;
	register int i;
	register struct proc *p;
	struct timeval syst;

	syst = u.u_ru.ru_stime;
	if (USERMODE(locr0[PS])) {
		type |= USER;
		u.u_ar0 = locr0;
	}
	switch (type) {

	default:
		printf("trap type %d, code = %x, pc = %x\n", type, code, pc);
		type &= ~USER;
		traceback();
		if ((unsigned)type < TRAP_TYPES)
			panic(trap_type[type]);
		panic("trap");

	case T_PROTFLT+USER:	/* protection fault */
		i = SIGBUS;
		break;

	case T_PRIVINFLT+USER:	/* privileged instruction fault */
	case T_RESADFLT+USER:	/* reserved addressing fault */
	case T_RESOPFLT+USER:	/* resereved operand fault */
		u.u_code = type &~ USER;
		i = SIGILL;
		break;

	case T_ASTFLT+USER:
		astoff();
		if ((u.u_procp->p_flag & SOWEUPC) && u.u_prof.pr_scale) {
			addupc(pc, &u.u_prof, 1);
			u.u_procp->p_flag &= ~SOWEUPC;
		}
		goto out;

	case T_ARITHTRAP+USER:
		u.u_code = code;
		i = SIGFPE;
		break;

	/*
	 * If the user SP is above the stack segment,
	 * grow the stack automatically.
	 */
	case T_SEGFLT+USER:
		if (grow((unsigned)locr0[SP]) || grow(code))
			goto out;
		i = SIGSEGV;
		break;

	case T_TABLEFLT:	/* allow page table faults in kernel mode */
	case T_TABLEFLT+USER:   /* page table fault */
		panic("ptable fault");

	case T_PAGEFLT:		/* allow page faults in kernel mode */
	case T_PAGEFLT+USER:	/* page fault */
		i = u.u_error;
		pagein(code, 0);
		u.u_error = i;
		if (type == T_PAGEFLT)
			return;
		goto out;

	case T_BPTFLT+USER:	/* bpt instruction fault */
	case T_TRCTRAP+USER:	/* trace trap */
		locr0[PS] &= ~PSL_T;
		i = SIGTRAP;
		break;

	case T_XFCFLT+USER:	/* xfc instruction fault */
		i = SIGEMT;
		break;

	case T_COMPATFLT+USER:	/* compatibility mode fault */
		u.u_acflag |= ACOMPAT;
		u.u_code = code;
		i = SIGILL;
		break;
	}
	psignal(u.u_procp, i);
out:
	p = u.u_procp;
	if (p->p_cursig || ISSIG(p))
		psig();
	p->p_pri = p->p_usrpri;
	if (runrun) {
		/*
		 * Since we are u.u_procp, clock will normally just change
		 * our priority without moving us from one queue to another
		 * (since the running process is not on a queue.)
		 * If that happened after we setrq ourselves but before we
		 * swtch()'ed, we might not be on the queue indicated by
		 * our priority.
		 */
		(void) spl6();
		setrq(p);
		u.u_ru.ru_nivcsw++;
		swtch();
	}
	if (u.u_prof.pr_scale) {
		int ticks;
		struct timeval *tv = &u.u_ru.ru_stime;

		ticks = ((tv->tv_sec - syst.tv_sec) * 1000 +
			(tv->tv_usec - syst.tv_usec) / 1000) / (tick / 1000);
		if (ticks)
			addupc(locr0[PC], &u.u_prof, ticks);
	}
	curpri = p->p_pri;
}

#ifdef SYSCALLTRACE
int syscalltrace = 0;
#endif
/*
 * Called from the trap handler when a system call occurs
 */
/*ARGSUSED*/
syscall(sp, type, code, pc, psl)
	unsigned code;
{
	register int *locr0 = ((int *)&psl)-PS;
	register caddr_t params;		/* known to be r10 below */
	register int i;				/* known to be r9 below */
	register struct sysent *callp;
	register struct proc *p;
	int opc;
	struct timeval syst;

	syst = u.u_ru.ru_stime;
	if (!USERMODE(locr0[PS]))
		panic("syscall");
	u.u_ar0 = locr0;
	if (code == 139) {			/* XXX */
		sigcleanup();			/* XXX */
		goto done;			/* XXX */
	}
	params = (caddr_t)locr0[AP] + NBPW;
	u.u_error = 0;
	opc = pc - 2;
	if (code > 63)
		opc -= 2;
	callp = (code >= nsysent) ? &sysent[63] : &sysent[code];
	if (callp == sysent) {
		i = fuword(params);
		params += NBPW;
		callp = ((unsigned)i >= nsysent) ? &sysent[63] : &sysent[i];
	}
	if (i = callp->sy_narg * sizeof (int)) {
#ifndef lint
		asm("prober $3,r9,(r10)");		/* GROT */
		asm("bnequ ok");			/* GROT */
		u.u_error = EFAULT;			/* GROT */
		goto bad;				/* GROT */
asm("ok:");						/* GROT */
		asm("movc3 r9,(r10),_u+U_ARG");		/* GROT */
#else
		bcopy(params, (caddr_t)u.u_arg, (u_int)i);
#endif
	}
	u.u_ap = u.u_arg;
	u.u_r.r_val1 = 0;
	u.u_r.r_val2 = locr0[R1];
	if (setjmp(&u.u_qsave)) {
		if (u.u_error == 0 && u.u_eosys == JUSTRETURN)
			u.u_error = EINTR;
	} else {
		u.u_eosys = JUSTRETURN;
#ifdef SYSCALLTRACE
		if (syscalltrace) {
			register int i;
			char *cp;

			if (code >= nsysent)
				printf("0x%x", code);
			else
				printf("%s", syscallnames[code]);
			cp = "(";
			for (i= 0; i < callp->sy_narg; i++) {
				printf("%s%x", cp, u.u_arg[i]);
				cp = ", ";
			}
			if (i)
				putchar(')', 0);
			putchar('\n', 0);
		}
#endif
		(*(callp->sy_call))(u.u_ap);
	}
	locr0[PS] &= ~PSL_C;
	if (u.u_eosys == RESTARTSYS)
		pc = opc;
#ifdef notdef
	else if (u.u_eosys == SIMULATERTI)
		dorti();
#endif
	else if (u.u_error) {
#ifndef lint
bad:
#endif
		locr0[R0] = u.u_error;
		locr0[PS] |= PSL_C;	/* carry bit */
	} else {
		locr0[R0] = u.u_r.r_val1;
		locr0[R1] = u.u_r.r_val2;
	}
done:
	p = u.u_procp;
	if (p->p_cursig || ISSIG(p))
		psig();
	p->p_pri = p->p_usrpri;
	if (runrun) {
		/*
		 * Since we are u.u_procp, clock will normally just change
		 * our priority without moving us from one queue to another
		 * (since the running process is not on a queue.)
		 * If that happened after we setrq ourselves but before we
		 * swtch()'ed, we might not be on the queue indicated by
		 * our priority.
		 */
		(void) spl6();
		setrq(p);
		u.u_ru.ru_nivcsw++;
		swtch();
	}
	if (u.u_prof.pr_scale) {
		int ticks;
		struct timeval *tv = &u.u_ru.ru_stime;

		ticks = ((tv->tv_sec - syst.tv_sec) * 1000 +
			(tv->tv_usec - syst.tv_usec) / 1000) / (tick / 1000);
		if (ticks)
			addupc(locr0[PC], &u.u_prof, ticks);
	}
	curpri = p->p_pri;
}

/*
 * Indirect system call.
 * For now, just a stub.  Eventually move indirect
 * system call processing from above to here, like
 * was done for the Sun.
 */
indir()
{

	panic("indirect system call");
}

/*
 * nonexistent system call-- signal process (may want to handle it)
 * flag error if process won't see signal immediately
 * Q: should we do that all the time ??
 */
nosys()
{
	if (u.u_signal[SIGSYS] == SIG_IGN || u.u_signal[SIGSYS] == SIG_HOLD)
		u.u_error = EINVAL;
	psignal(u.u_procp, SIGSYS);
}

#include "../vax/frame.h"
/*
 * Print out a traceback for kernel traps
 */
traceback()
{
	register struct frame *fp;	/* r11 */
	register int sp;		/* r10 */
	struct frame *tospage;
	static int done = 0;
	register int *ap;

	if (panicstr && done++ > 0)
		return;

	asm("movl fp,r11");
	asm("movl sp,r10");
	tospage = (struct frame *)btoc(sp);
	printf("Begin traceback...fp = %x, sp = %x\n", fp, sp);
	while (btoc(((int)fp)) == (int)tospage) {
		if (fp == (struct frame *)fp->fr_savfp) {
			printf("FP loop at %x", fp);
			break;
		}
		ap = (int *)fp->fr_savap;
		printf("Called from %x, fp=%x, args=%x %x %x %x\n",
		    fp->fr_savpc, fp->fr_savfp,
		    ap[0], ap[1], ap[2], ap[3]);
		fp = (struct frame *)fp->fr_savfp;
	}
	printf("End traceback...\n");
DELAY(2000000);
}
