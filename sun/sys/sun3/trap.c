#ifndef lint
static	char sccsid[] = "@(#)trap.c 1.4 86/12/12";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/seg.h"
#include "../h/vm.h"
#include "../h/kernel.h"
#ifdef SYSCALLTRACE
#include "../sys/syscalls.c"
#endif

#include "../sun/fault.h"
#include "../sun/frame.h"
#include "../machine/buserr.h"
#include "../machine/memerr.h"
#include "../machine/mmu.h"
#include "../machine/cpu.h"
#include "../machine/psl.h"
#include "../machine/pte.h"
#include "../machine/reg.h"
#include "../machine/trap.h"

#include "fpa.h"
#if NFPA > 0
#include "../sundev/fpareg.h"
#endif NFPA > 0

#define	USER	0x400		/* user-mode flag added to type */

extern struct sysent sysent[];
extern int nsysent;

char	*trap_type[] = {
	"Vector address 0x0",
	"Vector address 0x4",
	"Bus error",
	"Address error",
	"Illegal instruction",
	"Divide by zero",
	"CHK, CHK2 instruction",
	"TRAPV, cpTRAPcc, cpTRAPcc instruction",
	"Priviledge violation",
	"Trace",
	"1010 emulator trap",
	"1111 emulator trap",
	"Vector address 0x30",
	"Coprocessor protocol error",
	"Stack format error",
	"Unitialized interrupt",
	"Vector address 0x40",
	"Vector address 0x44",
	"Vector address 0x48",
	"Vector address 0x4c",
	"Vector address 0x50",
	"Vector address 0x54",
	"Vector address 0x58",
	"Vector address 0x5c",
	"Spurious interrupt",
};
#define	TRAP_TYPES	(sizeof trap_type / sizeof trap_type[0])

#if defined(DEBUG) || defined(lint)
int tdebug  = 0;
int lodebug = 0;
int bedebug = 0;
#else
#define	tdebug	0
#define	lodebug	0
#define	bedebug	0
#endif defined(DEBUG) || defined(lint)

int tudebug = 0;

u_char	getsegmap();
long	getpgmap();

/*
 * Called from the trap handler when a processor trap occurs.
 * Returns amount to adjust the stack:  > 0 removes bus error
 * info, == 0 does nothing.
 */
int
trap(type, regs, fmt)
	int type;
	struct regs regs;
	struct stkfmt fmt;
{
	register struct regs *locregs = &regs;
	register int i = 0;
	register struct proc *p = u.u_procp;
	struct timeval syst;
	int nosig = 0;
	int besize = 0;
	int be = (type == T_BUSERR)? getbuserr() : 0;

	cnt.v_trap++;
	syst = u.u_ru.ru_stime;
	if (tdebug) {
		i = type/sizeof (int);
		if ((unsigned)i < TRAP_TYPES)
			printf("trap: %s\n", trap_type[i]);
		showregs("trap", type, locregs, &fmt, be);
	}
	if (USERMODE(locregs->r_sr)) {
		type |= USER;
		u.u_ar0 = &locregs->r_dreg[0];
	} else {
		/* reset sp value to adjusted system sp */ 
		locregs->r_sp = (int)&fmt.f_beibase;
		switch (fmt.f_stkfmt) {
		case SF_NORMAL:
		case SF_THROWAWAY:
			break;
		case SF_NORMAL6:
			locregs->r_sp += sizeof (struct bei_normal6);
			break;
		case SF_COPROC:
			locregs->r_sp += sizeof (struct bei_coproc);
			break;
		case SF_MEDIUM:
			locregs->r_sp += sizeof (struct bei_medium);
			break;
		case SF_LONGB:
			locregs->r_sp += sizeof (struct bei_longb);
			break;
		default:
			panic("bad system stack format");
			/*NOTREACHED*/
		}
	}

	switch (type) {

	case T_ADDRERR:			/* kernel address error */
		/* 
		 * On 68020, addresses errors can only happen
		 * when executing instructions on an odd boundary.
		 * Therefore, we cannot have gotten here as a
		 * result of copyin/copyout request - so panic.
		 */
	default:
	die:
		(void) spl7();
		showregs((char *)0, fmt.f_vector, locregs, &fmt, be);
		traceback((long)locregs->r_areg[6], (long)locregs->r_sp);
		i = fmt.f_vector/sizeof (int);
		if (i < TRAP_TYPES)
			panic(trap_type[i]);
		panic("trap");
		/*NOTREACHED*/

	case T_BUSERR:
		if (be & BE_TIMEOUT)
			DELAY(2000);	/* allow for refresh recovery time */

		/* may have been expected by C (e.g., Multibus probe) */
		if (nofault) {
			label_t *ftmp;

			ftmp = nofault;
			nofault = 0;
			longjmp(ftmp);
		}
#if NFPA > 0
		/*
		 * If enable register is not turned on, panic.
		 * In case of FPA bus error in the kernel mode, shutdown
		 * FPA, instead of panic'ing the system.
		 */
		if (be & BE_FPAENA)
			panic("FPA not enabled");
		if (be & BE_FPABERR) {
			showregs("FPA KERNEL BUS ERROR", fmt.f_vector, locregs,
			    &fmt, be);
			traceback((long)locregs->r_areg[6],
			    (long)locregs->r_sp);
			printf("FPA BUS ERROR: IERR == %x\n", fpa->fp_ierr);
			fpa_shutdown();
			i = SIGSEGV;
			break;
		}
#endif NFPA > 0
		/* may be fault caused by transfer to/from user space */
		if (u.u_lofault == 0)
			goto die;

		switch (fmt.f_stkfmt) {
		case SF_MEDIUM: {
			struct bei_medium *beip =
			    (struct bei_medium *)&fmt.f_beibase;

			besize = sizeof (struct bei_medium);
			if (beip->bei_dfault && pagefault(beip->bei_fault))
				return (0);
			break;
			}
		case SF_LONGB: {
			struct bei_longb *beip =
			    (struct bei_longb *)&fmt.f_beibase;

			besize = sizeof (struct bei_longb);
			if (beip->bei_dfault && pagefault(beip->bei_fault))
				return (0);
			break;
			}
		default:
			panic("bad bus error stack format");
		}

		if (lodebug) {
			showregs("lofault", type, locregs, &fmt, be);
			traceback((long)locregs->r_areg[6],
			    (long)locregs->r_sp);
		}
		locregs->r_pc = u.u_lofault;
		return (besize);

	case T_ADDRERR + USER:		/* user address error */
		if (tudebug)
			showregs("USER ADDRESS ERROR", type, locregs, &fmt, be);
		i = SIGBUS;
		switch (fmt.f_stkfmt) {
		case SF_MEDIUM:
			besize = sizeof (struct bei_medium);
			break;
		case SF_LONGB:
			besize = sizeof (struct bei_longb);
			break;
		default:
			panic("bad address error stack format");
		}
		break;

	case T_SPURIOUS:
	case T_SPURIOUS + USER:		/* spurious interrupt */
		i = spl7();
		printf("spurious level %d interrupt\n", (i & SR_INTPRI) >> 8);
		(void) splx(i);
		return (0);

	case T_PRIVVIO + USER:		/* privileged instruction fault */
		if (tudebug)
			showregs("USER PRIVILEGED INSTRUCTION", type, locregs,
			    &fmt, be);
		u.u_code = fmt.f_vector;
		i = SIGILL;
		break;

	case T_COPROCERR + USER:	/* coprocessor protocol error */
		/*
		 * Dump out obnoxious info to warn user
		 * that something isn't right w/ the 68881
		 */
		showregs("USER COPROCESSOR PROTOCOL ERROR", type, locregs,
		    &fmt, be);
		u.u_code = fmt.f_vector;
		i = SIGILL;
		break;

	case T_M_BADTRAP + USER:	/* (some) undefined trap */
	case T_ILLINST + USER:		/* illegal instruction fault */
		if (tudebug)
			showregs("USER ILLEGAL INSTRUCTION", type, locregs,
			    &fmt, be);
		u.u_code = fmt.f_vector;
		i = SIGILL;
		break;

	case T_M_FLOATERR + USER:	/* (some) floating error trap */
	case T_ZERODIV + USER:		/* divide by zero */
	case T_CHKINST + USER:		/* CHK [CHK2] instruction */
	case T_TRAPV + USER:		/* TRAPV [cpTRAPcc TRAPcc] instr */
		u.u_code = fmt.f_vector;
		i = SIGFPE;
		break;

	/*
	 * User bus error.  Try to handle FPA, then pagefault, and
	 * failing that, grow that stack automatically if a data fault.
	 */
	case T_BUSERR + USER:

		if (be & BE_TIMEOUT)
			DELAY(2000);	/* allow for refresh recovery time */

		/*
		 * Copy the "rerun" bits to the "fault" bits.
		 *
		 * This is what is going on here (don't believe
		 * the 2nd edition 68020 description in section
		 * 6.4.1, it is full of errors).  A rerun bit
		 * being on means that the prefetch failed.  A
		 * fault bit being on means the processor tried
		 * to use bad prefetch data.  Upon return via
		 * the RTE instruction, the '20 will retry the
		 * instruction access only if BOTH the rerun and
		 * the corresponding fault bit is on.
		 *
		 * We need to do guarantee that any time we have a
		 * fault that we have actually just run the cycle,
		 * otherwise the current external state (i.e. the
		 * bus error register) might not anything to do with
		 * what really happened to cause the prefetch to fail.
		 * For example the prefetch might have occured previous
		 * to an earlier bus error exception whose handling
		 * might have resolved the prefetch problem.  Thus by
		 * copying the "rerun" bits, we force the `20 to rerun
		 * every previously faulted prefetch upon return from
		 * this bus error.  This way we are guaranteed that we
		 * never get a "bogus" '20 internal bus error when it
		 * attempts to use a previously faulted prefetch.  On
		 * the downside, this hack might make the kernel fix up
		 * a prefetch fault that the '20 was not going to use.
		 * What we really need is a "don't know anything about
		 * a prefetch bit".  If we had something like that then 
		 * the '20 could know enough to rerun the prefetch, but
		 * only if it turns out that it really needs it.
		 *
		 * RISC does have its advantages.
		 *
		 * N.B.  This code depends on not having an executable
		 * where the last instruction in the text segment is
		 * too close the end of a page.  We don't want to get
		 * ourselves in trouble trying to fix up a fault beyond
		 * the end of the text segment.  But because the loader
		 * already pads out by an additional page when it sees
		 * this problem due to microcode bugs with the first
		 * year or so worth of '20 chips, we shouldn't get be
		 * in trouble here.
		 */

		switch (fmt.f_stkfmt) {
		case SF_MEDIUM: {
			struct bei_medium *beip =
			    (struct bei_medium *)&fmt.f_beibase;

			besize = sizeof (struct bei_medium);
			beip->bei_faultc = beip->bei_rerunc;
			beip->bei_faultb = beip->bei_rerunb;
			break;
			}
		case SF_LONGB: {
			struct bei_longb *beip =
			    (struct bei_longb *)&fmt.f_beibase;

			besize = sizeof (struct bei_longb);
			beip->bei_faultc = beip->bei_rerunc;
			beip->bei_faultb = beip->bei_rerunb;
			break;
			}
		default:
			panic("bad bus error stack format");
		}

#if NFPA > 0
		if (u.u_fpa_flags && (be & (BE_FPAENA | BE_FPABERR))) {
			/*
		 	 * FPA exception, either from ENABLE reg or FPA board.
		 	 * We save bus error PC in u.u_fpa_pc and save the
		 	 * current bus error information at u.u_fpa_fmtptr.
		 	 * Other information saved are: (high core) the bus
		 	 * error exception frame, a short of frame type and
		 	 * vector offset, the struct regs, a long of AST bits
			 * of p0lr, a short indicating the size of this area,
			 * and a short indicating the size to restore to
			 * the kernel stack.
		 	 * The exception frame is rte'ed to 68020 when
		 	 * u.u_fpa_pc equals kernel stack's PC in syscall: of
			 * locore.s.
		 	 */
			struct fpa_stack {
				short	fpst_save_size;
				short	fpst_alloc_size;
				long	fpst_ast_p0lr;
				struct regs fpst_regs;
				short	fpst_stkfmt;
				struct bei_longb  fpst_longb;
			} *fp;

			u.u_code = (be & BE_FPAENA) ? FPE_FPA_ENABLE
					: FPE_FPA_ERROR;
			if (u.u_fpa_fmtptr == NULL) {
				/* First FPA exception, alloc space */
				u.u_fpa_fmtptr = kmem_alloc(
				    sizeof (struct fpa_stack));
				fp = (struct fpa_stack *)u.u_fpa_fmtptr;
				fp->fpst_alloc_size = sizeof (struct fpa_stack);
			} else
				fp = (struct fpa_stack *)u.u_fpa_fmtptr;

			u.u_fpa_pc = u.u_ar0[PC];
			fp->fpst_ast_p0lr = u.u_pcb.pcb_p0lr & AST_CLR;
			/*
			 * Current kernel stack: (high core)exception stk,
			 * a short(stkfmt+vector offset), regs(low core).
			 * We save regs, a short(stkfmt+vector offset),
			 * and the exception stack.
			 */
			fp->fpst_save_size = sizeof (struct regs)
			    + sizeof (short) + besize;
			bcopy((caddr_t)&regs, (caddr_t)&fp->fpst_regs,
			    (u_int)fp->fpst_save_size);
			i = SIGFPE;
			break;
		}
#endif NFPA > 0

		if ((be & ~BE_INVALID) != 0) {
			/*
			 * Some other error indicated in the bus error
			 * invalid so there is nothing we can do now.
			 */
		} else if (fmt.f_stkfmt == SF_MEDIUM) {
			struct bei_medium *beip =
			    (struct bei_medium *)&fmt.f_beibase;

			if ((bedebug && (beip->bei_faultb || beip->bei_faultc))
			    || (bedebug > 1 && beip->bei_fault))
				printf("medium fault b %d %x, c %d %x, d %d %x\n",
				    beip->bei_faultb, locregs->r_pc + 4,
				    beip->bei_faultc, locregs->r_pc + 2,
				    beip->bei_dfault, beip->bei_fault);

			if (beip->bei_dfault) {
				if (pagefault(beip->bei_fault))
					return (0);
				if (grow((unsigned)beip->bei_fault)) {
					nosig = 1;
					besize = 0;
					goto out;
				}
			} else if (beip->bei_faultc &&
			    pagefault(locregs->r_pc + 2))
				return (0);
			else if (beip->bei_faultb &&
			    pagefault(locregs->r_pc + 4))
				return (0);
		} else {
			struct bei_longb *beip =
			    (struct bei_longb *)&fmt.f_beibase;

			if ((bedebug && (beip->bei_faultb || beip->bei_faultc))
			    || (bedebug > 1 && beip->bei_fault))
				printf("long fault b %d %x, c %d %x, d %d %x\n",
				    beip->bei_faultb, beip->bei_stageb,
				    beip->bei_faultc, beip->bei_stageb - 2,
				    beip->bei_dfault, beip->bei_fault);

			if (beip->bei_dfault) {
				if (pagefault(beip->bei_fault))
					return (0);
				if (grow((unsigned)beip->bei_fault)) {
					nosig = 1;
					besize = 0;
					goto out;
				}
			} else if (beip->bei_faultc &&
			    pagefault(beip->bei_stageb - 2))
				return (0);
			else if (beip->bei_faultb &&
			    pagefault(beip->bei_stageb))
				return (0);
		}
		if (tudebug)
			showregs("USER BUS ERROR", type, locregs, &fmt, be);
		i = SIGSEGV;
		break;

	case T_TRACE:			/* caused by tracing trap instr */
		u.u_pcb.pcb_p0lr |= TRACE_PENDING;
		return (0);

	case T_TRACE + USER:		/* trace trap */
		dotrace(locregs);
		goto out;

	case T_BRKPT + USER:		/* bpt instruction (trap #15) fault */
		i = SIGTRAP;
		break;

	case T_EMU1010 + USER:		/* 1010 emulator trap */
	case T_EMU1111 + USER:		/* 1111 emulator trap */
		u.u_code = fmt.f_vector;
		i = SIGEMT;
		break;
	}

	psignal(p, i);
out:
	if (u.u_pcb.pcb_p0lr & TRACE_PENDING)
		dotrace(locregs);
	if (p->p_cursig || ISSIG(p)) {
		if (nosig)
			u.u_pcb.pcb_p0lr |= AST_STEP;	/* get back here soon */
		else
			psig();
	}
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
		(void) spl0();
	}
	if (u.u_prof.pr_scale) {
		int ticks;
		struct timeval *tv = &u.u_ru.ru_stime;

		ticks = ((tv->tv_sec - syst.tv_sec) * 1000 +
			(tv->tv_usec - syst.tv_usec) / 1000) / (tick / 1000);
		if (ticks)
			addupc(locregs->r_pc, &u.u_prof, ticks);
	}
	curpri = p->p_pri;
	return (besize);
}

#ifdef SYSCALLTRACE
int syscalltrace = 0;
#endif
/*
 * Called from the trap handler when a system call occurs
 */
syscall(code, regs)
	int code;
	struct regs regs;
{
	struct timeval syst;
	short int syst_flag;

	cnt.v_syscall++;
	if (u.u_prof.pr_scale) {
		syst = u.u_ru.ru_stime;
		syst_flag = 1;
	} else
		syst_flag = 0;
#ifdef notdef
	if (!USERMODE(regs.r_sr))
		panic("syscall");
#endif
	{
	/*
	 * At this point we declare a number of register variables.
	 * syscall_setjmp (called below) does not preserve the values
	 * of register variables, so we limit their scope to this block.
	 */
	register struct regs *locregs;
	register struct sysent *callp;

	locregs = &regs;
	u.u_ar0 = &locregs->r_dreg[0];
	if (code < 0)
		code = 63;
	u.u_error = 0;
	callp = (code >= nsysent) ? &sysent[63] : &sysent[code];
	if (callp->sy_narg) {
		if (fulwds((caddr_t)locregs->r_sp + 2 * NBPW, (caddr_t)u.u_arg,
		    callp->sy_narg)) {
			u.u_error = EFAULT;
			goto bad;
		}
	}
	u.u_ap = u.u_arg;
	u.u_r.r_val1 = 0;
	u.u_r.r_val2 = regs.r_dreg[1];
	/*
	 * Syscall_setjmp is a special setjmp that only saves a6 and sp.
	 * The result is a significant speedup of this critical path,
	 * but meanwhile all the register variables have the wrong
	 * values after a longjmp returns here.
	 * This is the reason for the limited scope of the register
	 * variables in this routine - the values may go away here.
	 */
	if (syscall_setjmp(&u.u_qsave)) {
		if (u.u_error == 0 && u.u_eosys == JUSTRETURN)
			u.u_error = EINTR;
	} else {
		u.u_eosys = JUSTRETURN;
#ifdef SYSCALLTRACE
		if (syscalltrace) {
			register int i;
			char *cp;

			printf("%d: ", u.u_procp->p_pid);
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
	/* end of scope of register variables above */
	}
	if (u.u_eosys != JUSTRETURN) {
		if (u.u_eosys == RESTARTSYS)
			regs.r_pc -= 2;
#ifdef notdef
		else if (u.u_eosys == SIMULATERTI)
			dorti((caddr_t)regs.r_sp + 2 * NBPW + i);
#endif
	} else {
		regs.r_sp += sizeof (int);	/* pop syscall # */
		if (u.u_error) {
bad:
#ifdef SYSCALLTRACE
			if (syscalltrace)
				printf("syscall: error=%d\n", u.u_error);
#endif
			regs.r_dreg[0] = u.u_error;
			regs.r_sr |= SR_CC;	/* carry bit */
		} else {
			regs.r_sr &= ~SR_CC;
			regs.r_dreg[0] = u.u_r.r_val1;
			regs.r_dreg[1] = u.u_r.r_val2;
		}
	}
	if (u.u_pcb.pcb_p0lr & TRACE_PENDING)
		dotrace(&regs);
	{
	/* scope for use of register variable p */
	register struct proc *p;

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
		(void) spl0();
	}
	if (syst_flag) {
		int ticks;
		struct timeval *tv = &u.u_ru.ru_stime;

		ticks = ((tv->tv_sec - syst.tv_sec) * 1000 +
			(tv->tv_usec - syst.tv_usec) / 1000) / (tick / 1000);
		if (ticks)
			addupc(regs.r_pc, &u.u_prof, ticks);
	}
	curpri = p->p_pri;
	}
}

/*
 * Indirect system call.
 * Used to be handled above, in syscall, but then everyone
 * was paying a performance penalty for this rarely-used
 * (and questionable) feature.
 */
indir()
{
	register int code, i;
	register struct sysent *callp;

	code = u.u_arg[0];
	callp = (code < 1 || code >= nsysent) ?
		&sysent[63] : &sysent[code];
	if (i = callp->sy_narg) {
		if (fulwds((caddr_t)u.u_ar0[SP] + 3*NBPW, (caddr_t)u.u_arg,
		    i)) {
			u.u_error = EFAULT;
			return;
		}
	}
	(*(callp->sy_call))(u.u_ap);
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

/*
 * Handle trace traps, both real and delayed.
 */
dotrace(locregs)
	struct regs *locregs;
{
	register int r, s;
	struct proc *p = u.u_procp;

	s = spl6();
	r = u.u_pcb.pcb_p0lr&AST_CLR;
	u.u_pcb.pcb_p0lr &= ~AST_CLR;
	u.u_ar0[PS] &= ~PSL_T;
	(void) splx(s);
	if (r & TRACE_AST) {
		if ((p->p_flag&SOWEUPC) && u.u_prof.pr_scale) {
			addupc(locregs->r_pc, &u.u_prof, 1);
			p->p_flag &= ~SOWEUPC;
		}
		if ((r & TRACE_USER) == 0)
			return;
	}
	psignal(p, SIGTRAP);
}

/*
 * Print out a traceback for kernel traps
 */
traceback(afp, sp)
	long afp, sp;
{
	u_int tospage = btoc(sp);
	struct frame *fp = (struct frame *)afp;
	static int done = 0;

	if (panicstr && done++ > 0)
		return;

	printf("Begin traceback...fp = %x, sp = %x\n", fp, sp);
	while (btoc(fp) == tospage) {
		if (fp == fp->fr_savfp) {
			printf("FP loop at %x", fp);
			break;
		}
		printf("Called from %x, fp=%x, args=%x %x %x %x\n",
		    fp->fr_savpc, fp->fr_savfp,
		    fp->fr_arg[0], fp->fr_arg[1], fp->fr_arg[2], fp->fr_arg[3]);
		fp = fp->fr_savfp;
	}
	printf("End traceback...\n");
	DELAY(2000000);
}

showregs(str, type, locregs, fmtp, be)
	char *str;
	int type;
	struct regs *locregs;
	struct stkfmt *fmtp;
{
	int *r, s;
	int fcode, accaddr;
	char *why;

	s = spl7();
	printf("%s: %s\n", u.u_comm, str ? str : "");
	printf(
	"trap address 0x%x, pid %d, pc = %x, sr = %x, stkfmt %x, context %x\n",
	    fmtp->f_vector, u.u_procp->p_pid, locregs->r_pc, locregs->r_sr,
	    fmtp->f_stkfmt, getcontext());
	type &= ~USER;
	if (type == T_BUSERR)
		printf("Bus Error Reg %b\n", be, BUSERR_BITS);
	if (type == T_BUSERR || type == T_ADDRERR) {
		switch (fmtp->f_stkfmt) {
		case SF_MEDIUM: {
			struct bei_medium *beip =
			    (struct bei_medium *)&fmtp->f_beibase;

			fcode = beip->bei_fcode;
			if (beip->bei_dfault) {
				why = "data";
				accaddr = beip->bei_fault;
			} else if (beip->bei_faultc) {
				why = "stage c";
				accaddr = locregs->r_pc+2;
			} else if (beip->bei_faultb) {
				why = "stage b";
				accaddr = locregs->r_pc+4;
			} else {
				why = "unknown";
				accaddr = 0;
			}
			printf("%s fault address %x faultc %d faultb %d ",
			    why, accaddr, beip->bei_faultc, beip->bei_faultb);
			printf("dfault %d rw %d size %d fcode %d\n",
			    beip->bei_dfault, beip->bei_rw,
			    beip->bei_size, fcode);
			break;
			}
		case SF_LONGB: {
			struct bei_longb *beip =
			    (struct bei_longb *)&fmtp->f_beibase;

			fcode = beip->bei_fcode;
			if (beip->bei_dfault) {
				why = "data";
				accaddr = beip->bei_fault;
			} else if (beip->bei_faultc) {
				why = "stage c";
				accaddr = beip->bei_stageb-2;
			} else if (beip->bei_faultb) {
				why = "stage b";
				accaddr = beip->bei_stageb;
			} else {
				why = "unknown";
				accaddr = 0;
			}
			printf("%s fault address %x faultc %d faultb %d ",
			    why, accaddr, beip->bei_faultc, beip->bei_faultb);
			printf("dfault %d rw %d size %d fcode %d\n",
			    beip->bei_dfault, beip->bei_rw,
			    beip->bei_size, fcode);
			break;
			}
		default:
			panic("bad bus error stack format");
		}
		if (fcode == FC_SD || fcode == FC_SP) {
			printf("KERNEL MODE\n");
			printf("page map %x\n", getpgmap((caddr_t)accaddr));
		} else {
			int tss, dss, sss, v;
			struct pmeg *pmp;
			struct proc *p = u.u_procp;
			struct pte *pte;

			v = btop(accaddr);
			tss = tptov(p, 0);
			dss = dptov(p, 0);
			sss = sptov(p, p->p_ssize - 1);
			if (v >= tss && v < tss + p->p_tsize ||
			    v >= dss && v < dss + p->p_dsize ||
			    v >= sss && v < sss + p->p_ssize) {
				pmp = &pmeg[p->p_ctx->ctx_pmeg[v/NPAGSEG]];
				pte = vtopte(p, (unsigned)v);
				printf("pagefault, pmp %x, pte %x %x\n",
				    pmp, pte, *pte);
				printf("pme %x\n", getpgmap((caddr_t)accaddr));
			} else {
				printf("bad addr, v %d tss %d dss %d sss %d\n",
					v, tss, dss, sss);
			}
		}
	}
	r = &locregs->r_dreg[0];
	printf("D0-D7  %x %x %x %x %x %x %x %x\n",
	    r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7]);
	r = &locregs->r_areg[0];
	printf("A0-A7  %x %x %x %x %x %x %x %x\n",
	    r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7]);
	DELAY(2000000);
	(void) splx(s);
}
