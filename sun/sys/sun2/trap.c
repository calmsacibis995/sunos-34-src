#ifndef lint
static	char sccsid[] = "@(#)trap.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * TODO:
 *	deal with more parity bits
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
#include "../machine/frame.h"
#include "../machine/buserr.h"
#include "../machine/mmu.h"
#include "../machine/cpu.h"
#include "../machine/psl.h"
#include "../machine/pte.h"
#include "../machine/reg.h"
#include "../machine/trap.h"

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
	"CHK instruction",
	"TRAPV instruction",
	"Priviledge violation",
	"Trace",
	"1010 emulator trap",
	"1111 emulator trap",
	"Vector address 0x30",
	"Vector address 0x34",
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

int	simzero = 0;	/* simulate user location zero */

#if defined(DEBUG) || defined(lint)
int tdebug  = 0;
int tudebug = 0;
int lodebug = 0;
#else
#define	tdebug	0
#define	tudebug	0
#define	lodebug	0
#endif defined(DEBUG) || defined(lint)

u_char	getsegmap();
long	getpgmap();

/*
 * Called from the trap handler when a processor trap occurs.
 * Returns amount to adjust the stack:  > 0 removes bus error
 * info, == 0 does nothing.
 */
/*ARGSUSED*/
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
			break;
		case SF_LONG8:
			locregs->r_sp += sizeof (struct bei_long8);
			break;
		default:
			panic("bad system stack format");
			/*NOTREACHED*/
		}
	}

	switch (type) {

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

		/*
		 * May have been expected by C (e.g., Multibus probe).
		 * "Probes" of on-board memory space may cause parity
		 * errors, so have to check this before checking for
		 * parity errors below.  Sadly this means some real
		 * parity errors may be masked, but there seems to be
		 * no way around it.
		 */
		if (nofault) {
			label_t *ftmp;

			ftmp = nofault;
			nofault = 0;
			longjmp(ftmp);
		}
		if (be & (BE_PARERR_L|BE_PARERR_U)) {
			parscan(be);
			/* NOTREACHED */
		}
		/* may be fault caused by transfer to/from user space */
		if (u.u_lofault == 0)
			goto die;
		{ struct bei_long8 *beip =
		    (struct bei_long8 *)&fmt.f_beibase;

		besize = sizeof (struct bei_long8);
		if (beip->bei_fcode == FC_UD && pagefault(beip->bei_accaddr))
			return (0);
		}
		if (lodebug) {
			showregs("lofault", type, locregs, &fmt, be);
			traceback((long)locregs->r_areg[6],
			    (long)locregs->r_sp);
		}
		locregs->r_pc = u.u_lofault;
		return (besize);

	case T_ADDRERR:			/* address error */
		/* may be fault caused by transfer to/from user space */
		if (u.u_lofault == 0)
			goto die;
		{ struct bei_long8 *beip =
		    (struct bei_long8 *)&fmt.f_beibase;

		if (beip->bei_fcode != FC_UD)
			goto die;
		besize = sizeof (struct bei_long8);
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
		besize = sizeof (struct bei_long8);
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

	case T_M_BADTRAP + USER:	/* (some) undefined trap */
	case T_ILLINST + USER:		/* illegal instruction fault */
		if (tudebug)
			showregs("USER ILLEGAL INSTRUCTION", type, locregs,
			    &fmt, be);
		u.u_code = fmt.f_vector;
		i = SIGILL;
		break;

	case T_ZERODIV + USER:		/* divide by zero */
	case T_CHKINST + USER:		/* CHK instruction */
	case T_TRAPV + USER:		/* TRAPV instr */
		u.u_code = fmt.f_vector;
		i = SIGFPE;
		break;

	/*
	 * If the user SP is above the stack segment,
	 * grow the stack automatically.
	 */
	case T_BUSERR + USER:
		if (be & (BE_PARERR_L|BE_PARERR_U)) {
			parscan(be);
			/* NOTREACHED */
		}

		if (be & BE_TIMEOUT)
			DELAY(2000);	/* allow for refresh recovery time */

		{ struct bei_long8 *beip =
		    (struct bei_long8 *)&fmt.f_beibase;

		besize = sizeof (struct bei_long8);
		if (be & (BE_TIMEOUT|BE_VMEBUSERR)) 
			goto pferr;

		if (pagefault(beip->bei_accaddr))
			return (0);
		if (grow((unsigned)beip->bei_accaddr)) {
			nosig = 1;
			besize = 0;
			goto out;
		}
		if (simzero)
			if (beip->bei_accaddr < 4 && beip->bei_rw &&
			    beip->bei_dfetch && !beip->bei_ifetch &&
			    !beip->bei_rmw) {
				if (simzero > 1)
					uprintf(
				"%s: pid %d: accessed location zero at pc %x\n",
					    u.u_comm, u.u_procp->p_pid,
					    locregs->r_pc);
				beip->bei_rerun = 1;
				beip->bei_dib = 0;
				nosig = 1;
				besize = 0;
				goto out;
			}
		}
	pferr:
		if (tudebug)
			showregs("USER BUS ERROR", type, locregs, &fmt, be);
		i = SIGSEGV;
		if (besize == 0)
			panic("besize");
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
	int be;
{
	int *r, s;
	int fcode, accaddr;

	s = spl7();
	printf("%s: %s\n", u.u_comm, str ? str : "");
	printf(
	"trap address 0x%x, pid %d, pc = %x, sr = %x, stkfmt %x, context %d\n",
	    fmtp->f_vector, u.u_procp->p_pid, locregs->r_pc, locregs->r_sr,
	    fmtp->f_stkfmt, getusercontext());
	type &= ~USER;
	if (type == T_BUSERR)
		printf("Bus Error Reg %b\n", be, BUSERR_BITS);
	if (type == T_BUSERR || type == T_ADDRERR) {
		struct bei_long8 *beip = (struct bei_long8 *)&fmtp->f_beibase;

		fcode = beip->bei_fcode;
		accaddr = beip->bei_accaddr;
		printf("access address %x ifetch %d dfetch %d ",
		    accaddr, beip->bei_ifetch, beip->bei_dfetch);
		printf("hibyte %d bytex %d rw %d fcode %d\n",
		    beip->bei_hibyte, beip->bei_bytex,
		    beip->bei_rw, fcode);
		if (fcode == FC_SD || fcode == FC_SP) {
			int uc;

			printf("KERNEL MODE\n");
			uc = getusercontext();
			setusercontext(KCONTEXT);
			printf("page map %x\n", getpgmap((caddr_t)accaddr));
			setusercontext(uc);
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

/*
 * Since parity errors are reported by a bus error on
 * the following cycle, we have no way of knowing the
 * address that caused the parity error.  Therefore,
 * we have to scan ALL OF PHYSICAL MEMORY looking for
 * the error.  Thank you Andy...
 */
parscan(bereg)
	register short bereg;		/* known to be d7 */
{
	register int page;
	register char *addr;
	register u_short data;
	extern u_short partest();
	extern char CADDR1[];
	extern char *panicstr;
	long oldmap = getpgmap(CADDR1);
	int found = 0;
	int s = spl7();

	disable_dvma();
	panicstr = "Parity Error";	/* force printf to console */
	printf("%s!  Bus Error Reg %b\n", panicstr, bereg, BUSERR_BITS);
	setusercontext(KCONTEXT);
	for (page = 0; page < btoc(*romp->v_memorysize); page++) {
		setpgmap(CADDR1, (long)(PG_V | PG_KR | page));
		for (addr = CADDR1; addr < CADDR1 + CLBYTES;
		    addr += sizeof (short)) {
			bereg = 0;
			data = partest((u_short *)addr);
			if (bereg) {
		printf("%s: Address 0x%x, Data 0x%x, Bus Error Reg %b\n",
				panicstr, ctob(page) + (addr - CADDR1),
				data, bereg, BUSERR_BITS);
				if (++found % 20 == 0) {
					printf("Delaying...");
					DELAY(30000000);
					printf("\n");
				}
			}
		}
	}
	if (found == 0)
		printf("Can't find parity error (transient?)\n");
	setpgmap(CADDR1, oldmap);
	enable_dvma();
	(void) splx(s);
	panic("parity error");
	/* NOTREACHED */
}
