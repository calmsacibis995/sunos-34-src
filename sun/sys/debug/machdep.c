#ifndef lint
static  char sccsid[] = "@(#)machdep.c 1.1 86/09/25";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "../h/param.h"
#include "../h/errno.h"
#include "../h/vmmac.h"
#include "../machine/buserr.h"
#include "../machine/enable.h"
#include "../machine/mmu.h"
#include "../machine/cpu.h"
#include "../machine/pte.h"
#include "../machine/reg.h"
#include "../machine/scb.h"
#include "../machine/psl.h"
#include "../machine/trap.h"
#include "../debug/debugger.h"
#include "../mon/keyboard.h"
#ifdef sun3
#include "../sun3/clock.h"
#include "../sun3/interreg.h"
#endif sun3

extern int errno;
extern int dottysync;

#define	T_TRAP		0x80		/* vector address for trap #0 */

#define	CALL(func)	(*(int (*)())((int)(func) - (int)start + real))
#define	RELOC(adr)	((adr) - (int)start + real)

#ifdef sun3
#define	DVMA	0x0ff00000	/* 28 bit DVMA addr for sun3 */
int cpudelay = 3;
int cpu;
#endif sun3

#ifdef sun2
#define	DVMA	0x00f00000	/* 24 bit DVMA addr for sun2 */
int cpudelay = 5;
#endif sun2

#define	RTE	0x4e73			/* code for a 680x0 rte instruction */
#define	TRAP	0x4e40			/* code for a 680x0 trap intruction */
#define	TRAPBRK	(TRAP | TRAPBRKNO)	/* code for a trap breakpoint intr */

extern char estack[], etext[];

/*
 * Definitions for registers in jmp_buf
 */
#define	JB_PC	0
#define	JB_D2	3
#define	JB_D3	4
#define	JB_D4	5
#define	JB_D5	6
#define	JB_D6	7
#define	JB_D7	8
#define	JB_A2	9
#define	JB_A3	10
#define	JB_A4	11
#define	JB_A5	12
#define	JB_A6	13
#define	JB_A7	14

static jmp_buf state;			/* used for temporary games */

/*
 * This routine is called before we are relocated to the correct address,
 * so everything is done via funny macros when dealing w/ globals.
 */
startup(real)
	register int real;
{
	register int cnt, pg, i;
	register int *from, *to;
	int pm, lopm;
	int lastpmsav, lastpgsav;
	struct scb *vbr;
	func_t *fp;
	extern Setpgmap(), setkcontext(), getsegmap(), setsegmap();
	extern char start[], end[], etext[];

	(void) CALL(setkcontext)();

	cnt = btoc(end - ((int)start & ~PGOFSET));

	/*
	 * DOESN'T WORK WITH REV N PROTOTYPE PROMS!!!
	 * v_memorysize is the amount of physical memory while
	 * v_memoryavail is the amount of usable memory in versions
	 * equal or greater to 1.
	 */
	if (romp->v_romvec_version >= 1)
		pg = btoc(*romp->v_memoryavail) - cnt;
	else
		pg = btoc(*romp->v_memorysize) - cnt;

	/*
	 * Look for the lowest pmeg in use in the monitor and DVMA space
	 */
	lopm = SEGINV;
	for (i = MONSTART; i < MONEND; i += NBSG)
		if ((pm = CALL(getsegmap)(i)) < lopm)
			lopm = pm;

	for (i = DVMA; i < DVMA + 0x100000; i += NBSG)
		if ((pm = CALL(getsegmap)(i)) < lopm)
			lopm = pm;

	/*
	 * Adjust down from there to get our starting pmeg
	 * and save copies of last pmeg and page used.
	 */
	lopm -= (((int)end+SGOFSET)&~SGOFSET)-((int)start&~SGOFSET) >> SGSHIFT;
	lastpmsav = lopm;
	lastpgsav = pg;

	for (i = (int)start; i < (int)end; i += NBPG) {
		if (CALL(getsegmap)(i) == SEGINV) {
			register int j = i & ~SGOFSET;
			int last = j + NPAGSEG * NBPG;

			(void) CALL(setsegmap)(i, lopm++);

			for (; j < last; j += NBPG)
				(void) CALL(Setpgmap)(j, 0);
		}
		(void) CALL(Setpgmap)(i, PG_V | PG_KW | pg++);
	}

	/*
	 * Copy program up to correct address
	 */
	for (to = (int *)start, from = (int *)real; to < (int *)end; )
		*to++ = *from++;

	/*
	 * Now we can reference global variables,
	 * save page count and monitor's nmi routine address.
	 */
	lastpg = lastpgsav;
	lastpm = lastpmsav;
	pagesused = cnt;
	vbr = getvbr();
	monnmi = vbr->scb_autovec[7 - 1];

	/*
	 * initialize the exception vectors (except nmi, trace, and
	 * trap) to all point to our fault handling routine, then
	 * call scbsync() to take care of trace & trap routines.
	 */
	for (fp = (func_t *)vbr; fp < &vbr->scb_user[0]; fp++) {
		if (fp != &vbr->scb_autovec[7 - 1] && fp != &vbr->scb_trace &&
		    !(fp >= &vbr->scb_trap[0] && fp <= &vbr->scb_trap[16 - 1]))
			*fp = fault;
	}
	scbsync();

	/*
	 * Now make text (and dvec) read only,
	 * this also sets a stack redzone
	 */
	for (i = (int)start; i < (int)etext; i += NBPG) {
		pg = Getpgmap(i);
		Setpgmap(i, (pg & ~PG_PROT) | PG_KR);
	}

#ifdef sun3
	Setpgmap((caddr_t)CLKADDR, PG_V|PG_KW|PGT_OBIO|btop(OBIO_CLKADDR));
	Setpgmap((caddr_t)INTERREG, PG_V|PG_KW|PGT_OBIO|btop(OBIO_INTERREG));
	if ((cpu = getmachinetype()) == CPU_SUN3_260) {
		cpudelay = 2;
		vac_init();			/* invalidate entire cache */
	}
#endif sun3

	/*
	 * If we are using an old keyboard, avoid calling monitor's
	 * initgetkey() routine because of a bug which will toggle
	 * the state of the CAPS lock!
	 */
	if ((*romp->v_keybid & 0xF) == KB_VT100 ||
	    (*romp->v_keybid & 0xF) == KB_MS_103SD32)
		dottysync = 0;
}


scbsync()
{
	struct scb *vbr = getvbr();

	vbr->scb_trap[10] = trap;		/* install trap handler */
	if (vbr->scb_trace != trace) {
		ktrace = vbr->scb_trace;	/* save old handler */
		vbr->scb_trace = trace;		/* install new handler */
	}
	if (scbstop) {
		/*
		 * Set things up so that we call the debugger.
		 * Use _setjmp()/_longjmp() with some adjustments
		 * to pull this non-local goto off correctly.
		 */
		scbstop = 0;
		(void) _setjmp(state);
		state[JB_PC] = DVEC;			/* new pc value */
		state[JB_A7] = state[JB_A6];		/* pop off frame */
		state[JB_A6] = *(int *)state[JB_A6];	/* restore fp */
		_longjmp(state, 1);
		/*NOTREACHED*/
	}
}

#ifdef sun3
/*
 * Set and/or clear the desired clock bits in the interrupt
 * register.  We have to be extremely careful that we do it
 * in such a manner that we don't get ourselves lost.
 */
set_clk_mode(on, off)
	u_char on, off;
{
	register u_char interreg, dummy;

	/*
	 * make sure that we are only playing w/ 
	 * clock interrupt register bits
	 */
	on &= (IR_ENA_CLK7 | IR_ENA_CLK5);
	off &= (IR_ENA_CLK7 | IR_ENA_CLK5);

	/*
	 * Get a copy of current interrupt register,
	 * turning off any undesired bits (aka `off')
	 */
	interreg = *INTERREG & ~(off | IR_ENA_INT);
	*INTERREG &= ~IR_ENA_INT;

	/*
	 * Next we turns off the CLK5 and CLK7 bits to clear
	 * the flip-flops, then we disable clock interrupts.
	 * Now we can read the clock's interrupt register
	 * to clear any pending signals there.
	 */
	*INTERREG &= ~(IR_ENA_CLK7 | IR_ENA_CLK5);
	CLKADDR->clk_cmd = (CLK_CMD_NORMAL & ~CLK_CMD_INTRENA);
	dummy = CLKADDR->clk_intrreg;			/* clear clock */
#ifdef lint
	dummy = dummy;
#endif

	/*
	 * Now we set all the desired bits
	 * in the interrupt register, then
	 * we turn the clock back on and
	 * finally we can enable all interrupts.
	 */
	*INTERREG |= (interreg | on);			/* enable flip-flops */
	CLKADDR->clk_cmd = CLK_CMD_NORMAL;		/* enable clock intr */
	*INTERREG |= IR_ENA_INT;			/* enable interrupts */
}
#endif sun3

/*
 * Miscellanous fault error handler
 */
faulterr(dfc, sfc, regs, fmt)
	struct regs regs;
	struct stkfmt fmt;
{
	int ondebug_stack = (getsp() > (int)etext && getsp() < (int)estack);

	if (nofault && ondebug_stack) {
		jmp_buf_ptr sav = nofault;

		nofault = NULL;
		_longjmp(sav, 1);
		/*NOTREACHED*/
	}

	showregs(ondebug_stack ? "unexpected fault" : "unexpected exception",
	    &regs, &fmt);

	/*
	 * If we are on the debugger stack and
	 * abort_jmp is set, do a longjmp to it.
	 */
	if (abort_jmp && ondebug_stack) {
		_longjmp(abort_jmp, 1);
		/*NOTREACHED*/
	}

	/*
	 * Ok, the user faulted while not in the
	 * debugger.  Just return to locore which will
	 * bounce us back out to the main cmd loop
	 * so that the user can look around...
	 */
}

showregs(str, locregs, fmtp)
	char *str;
	struct regs *locregs;
	struct stkfmt *fmtp;
{
	int *r;
	int fcode, accaddr;
	char *why;

	printf("\n%s: %s\n", myname, str);
	printf("trap address 0x%x, pc = %x, sr = %x, stkfmt %x\n",
	    fmtp->f_vector, locregs->r_pc, locregs->r_sr, fmtp->f_stkfmt);
	switch (fmtp->f_stkfmt) {
#ifdef sun3
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
		    beip->bei_dfault, beip->bei_rw, beip->bei_size, fcode);
		break;
		}
#endif sun3
#ifdef sun2
	case SF_LONG8: {
		struct bei_long8 *beip =
		    (struct bei_long8 *)&fmtp->f_beibase;

		fcode = beip->bei_fcode;
		accaddr = beip->bei_accaddr;
		printf("access address %x ifetch %d dfetch %d ",
		    accaddr, beip->bei_ifetch, beip->bei_dfetch);
		printf("hibyte %d bytex %d rw %d fcode %d\n",
		    beip->bei_hibyte, beip->bei_bytex, beip->bei_rw, fcode);
		break;
		}
#endif sun2
	default:
		printf("bad bus error stack format %x\n", fmtp->f_stkfmt);
	}
	r = &locregs->r_dreg[0];
	printf("D0-D7  %x %x %x %x %x %x %x %x\n",
	    r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7]);
	r = &locregs->r_areg[0];
	printf("A0-A7  %x %x %x %x %x %x %x %x\n",
	    r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7]);
}

static jmp_buf_ptr saved_jb;
static jmp_buf jb;
extern int debugging;

poketext(addr, val)
	int *addr;
	int val;
{
	int pg = 0;
	struct scb *scb = getvbr();
	func_t oldbus;
	
	oldbus = scb->scb_buserr;
	scb->scb_buserr = faulterr;

again:
	saved_jb = nofault;
	nofault = jb;
	if (!_setjmp(jb)) {
		*addr = val;
		/* if we get here, it worked */
		nofault = saved_jb;
		if (pg) {
			/*
			 * Reset to page map to previous entry,
			 * but mark as modified
			 */
#ifdef sun3
			if (cpu == CPU_SUN3_260) {
				vac_pageflush(addr);
				if (btop(addr + sizeof (int) - 1) != btop(addr))
					vac_pageflush(addr + sizeof (int) - 1);
			}
#endif sun3
			Setpgmap(addr, pg | PG_M);
		}
		scb->scb_buserr = oldbus;
		return (0);
	}
	/* a fault occured */
	nofault = saved_jb;

	if (pg) {
		/*
		 * Reset to page map to previous entry.
		 */
#ifdef sun3
		if (cpu == CPU_SUN3_260) {
			vac_pageflush(addr);
			if (btop(addr + sizeof (int) - 1) != btop(addr))
				vac_pageflush(addr + sizeof (int) - 1);
		}
#endif sun3
		Setpgmap(addr, pg);
		if (debugging > 2)
			printf("poketext %X failed\n", addr);
		goto err;
	}

	/*
	 * See if the problem was a write protected page.
	 * If so, make the page writable, do the operation,
	 * and then put the protections back as we found them.
	 */
	pg = Getpgmap((int)addr);
	if ((pg & PG_V) == 0) {
		if (debugging > 2)
			printf("poketext: invalid page map %X at %X\n",
			    pg, addr);
		goto err;
	}
	if ((pg & PGT_MASK) != PGT_OBMEM) {
		if (debugging > 2)
			printf("poketext: incorrect page type %X at %X\n",
			    pg, addr);
		goto err;
	}

	/*
	 * Make page writable
	 */
#ifdef sun3
	if (cpu == CPU_SUN3_260) {
		vac_pageflush(addr);
		if (btop(addr + sizeof (int) - 1) != btop(addr))
			vac_pageflush(addr + sizeof (int) - 1);
	}
#endif sun3
	if ((pg & PG_PROT) == PG_KR)
		Setpgmap(addr, (pg & ~PG_PROT) | PG_KW);
	else if ((pg & PG_PROT) == PG_URKR)
		Setpgmap(addr, (pg & ~PG_PROT) | PG_UW);
	else {
		if (debugging > 2)
			printf("poketext: unknown page prot %X at %X\n",
			    pg, addr);
		goto err;
	}
	
	/*
	 * Now try again
	 */
	goto again;

err:
	scb->scb_buserr = oldbus;
	errno = EFAULT;
	return (-1);
}

scopy(from, to, count)
	register char *from;
	register char *to;
	register int count;
{
	register int val;

	for (; count > 0; count--) {
		if ((val = Peekc(from++)) == -1)
			goto err;
		if (pokec(to++, val) == -1)
			goto err;
	}
	return (0);
err:
	errno = EFAULT;
	return (-1);
}

/*
 * Setup a new context to run at routine using stack whose
 * top (end) is at sp.  Assumes that the current context
 * is to be initialized for mainregs and new context is
 * to be set up in debugregs.
 */
spawn(sp, routine)
	int *sp;
	func_t routine;
{
	int *fp;

	if (curregs != 0) {
		printf("bad call to spawn\n");
		exit(1);
	}
	if (!_setjmp(mainregs)) {
		*--sp = (int)exit;	/* routine to goto if routine returns */
		*--sp = 0;		/* terminate fp's */
		fp = sp;		/* save address of this fp */
		*--sp = (int)routine;
		*--sp = (int)fp;
		/* copy entire jump buffer to debugregs */
		bcopy((caddr_t)mainregs, (caddr_t)debugregs, sizeof (jmp_buf));
		debugregs[JB_A6] = (int)sp;	/* set fp */
		debugregs[JB_A7] = (int)sp;	/* set sp */
		curregs = debugregs;
		_longjmp(debugregs, 1);/* jump to new context */
		/*NOTREACHED*/
	}
}

static int upte[UPAGES];
static int uchange;

map_uarea(umap)
	int umap[];
{
	register int *up, addr;

	if (foundu == 0) {
		printf("%s internal error:  u area confusion\n", myname);
		return;
	}

	/*
	 * map in the new u area, it will get remapped
	 * to original values before returning to debuggee.
	 */
	for (up = umap, addr = UADDR; up < &umap[UPAGES]; up++, addr += NBPG) {
#ifdef sun3
		if (cpu == CPU_SUN3_260)
			vac_pageflush(addr);
#endif sun3
		Setpgmap(addr, *up);
	}
	uchange++;
}

/* save old u area mapping (if appropriate) */
saveu()
{
	register int *ip, ua;

	if (foundu) {
		for (ip = upte, ua = UADDR; ip < &upte[UPAGES];
		    ip++, ua += NBPG)
			*ip = Getpgmap(ua);
		uchange = 0;
	}
}

/* restore old u area mapping (if appropriate) */
restoreu()
{
	register int *ip, ua;

	if (uchange && foundu) {
		for (ip = upte, ua = UADDR; ip < &upte[UPAGES];
		    ip++, ua += NBPG) {
#ifdef sun3
			if (cpu == CPU_SUN3_260)
				vac_pageflush(ua);
#endif sun3
			Setpgmap(ua, *ip);
		}
		uchange = 0;
	}
}

doswitch()
{

	if (!_setjmp(curregs)) {
		/*
		 * Switch curregs to other descriptor
		 */
		if (curregs == mainregs) {
			saveu();
			curregs = debugregs;
		} else /* curregs == debugregs */ {
#ifdef sun3
			flush20();
#endif sun3
			restoreu();
			curregs = mainregs;
		}
		_longjmp(curregs, 1);
		/*NOTREACHED*/
	}
	/*
	 * else continue on in new context 
	 */
}

/*
 * tracestate is used to track tracing over instruction
 * when an `rte' instruction is used.  This allows us
 * to single step over an rte instruction into user code.
 * HOWEVER, the more difficult problem of rte'ing to
 * the middle of an rte back to user land cannot be
 * handled reliably (Motorola doesn't give enough visable
 * info in the exception frames to do this right).
 *
 *	TS_IDLE		 = normal state
 *	TS_RTE		 = debuggee executing `rte' w/ trace bit on
 */

#define	TS_IDLE		0
#define	TS_RTE		1

int tracestate = TS_IDLE;
int istrap = 0;

/*
 * Main interpreter command loop.
 */
cmd(dfc, sfc, regs, fmt)
	struct regs regs;
	struct stkfmt fmt;
{
	register struct scb *vbr = getvbr();
	func_t nmisav = vbr->scb_autovec[7 - 1];
	func_t buserrsav = vbr->scb_buserr;
	func_t addrerrsav = vbr->scb_addrerr;
	func_t illinstsav = vbr->scb_illinst;
	func_t zerodivsav = vbr->scb_zerodiv;
#ifdef sun3
	int resetclk = 0;
	u_char interreg;
#endif sun3
#ifdef sun2
	int uc;
	short enable;
#endif sun2
	int addr, t;

	dotrace = dorun = 0;
	reg = &regs;

	/*
	 * See if the sp says that we are already on the debugger stack
	 */
	addr = getsp();
	if (addr > (int)etext && addr < (int)estack) {
		printf("Already in debugger!\n");
		goto out;
	}

	if (fmt.f_vector == T_TRAP + TRAPBRKNO * sizeof (int))
		istrap = 1;
	else
		istrap = 0;

	/*
	 * Set up for monitor's input routines to work.  This
	 * requires that the monitor's nmi handler is installed,
	 * interrupts (and level7 clock) are enabled.  Also,
	 * for sun2 we make sure we are in the correct context.
	 */
	dorun = dotrace = 0;
	vbr->scb_autovec[7 - 1] = monnmi;
	vbr->scb_buserr = fault;
	vbr->scb_addrerr = fault;
	vbr->scb_illinst = fault;
	vbr->scb_zerodiv = fault;
#ifdef sun3
	if ((*INTERREG & (IR_ENA_INT | IR_ENA_CLK7)) !=
	    (IR_ENA_INT | IR_ENA_CLK7)) {
		resetclk = 1;
		interreg = *INTERREG;
		set_clk_mode(IR_ENA_CLK7, 0);
	}
#endif sun3
#ifdef sun2
	enable = getenable();
	setenable(enable | ENA_INTS);
	uc = getusercontext();
	setusercontext(KCONTEXT);
#endif sun2

	do {
		doswitch();
		t = (dotrace == 0 && dorun == 0);
		if (t)
			printf("cmd: nothing to do\n");
	} while (t);

	/*
	 * Restore state - context, interrupt regs, nmi and clock values
	 */
#ifdef sun2
	setusercontext(uc);
	setenable(enable);
#endif sun2
#ifdef sun3
	if (resetclk) {
		set_clk_mode(0, IR_ENA_CLK7);
		*INTERREG = interreg;
	}
#endif sun3
	vbr->scb_autovec[7 - 1] = nmisav;
	vbr->scb_buserr = buserrsav;
	vbr->scb_addrerr = addrerrsav;
	vbr->scb_illinst = illinstsav;
	vbr->scb_zerodiv = zerodivsav;
out:
	t = tracestate;
	if (dotrace == 0) {
		regs.r_ps &= ~PSL_T;
		tracestate = TS_IDLE;
		return (t == TS_RTE);
	}

	/*
	 * trying to single step, check tracestate
	 */
	if (t == TS_IDLE && trace_rte(&regs, &fmt)) {
		tracestate = TS_RTE;
	} else if (t == TS_RTE) {
		tracestate = TS_IDLE;
	}
	/*
	 * If we are about to execute an rte instruction to an sr
	 * which does not have the trace bit on, turn it on now.
	 */
	if ((addr = rte_addr(&regs, &fmt)) != 0) {
		int sr;

		if ((sr = peek(addr)) != -1 && (sr & PSL_T) == 0)
			(void) poke(addr, sr | PSL_T);
	}
	regs.r_ps |= PSL_T;
	return (t == TS_RTE);
}

/*
 * Return the address of the eventual sr for rte or 0 if none
 */
rte_addr(r, f)
	register struct regs *r;
	register struct stkfmt *f;
{
	int offset;

	if (peek(r->r_pc) != RTE)
		return (0);

	/*
	 * read the ps value for rte which is atop the stack,
	 */
	switch (f->f_stkfmt) {
	case  SF_NORMAL:
		offset = 0;
		break;
#ifdef sun3
	case  SF_THROWAWAY:
		offset = 0;
		break;
	case  SF_NORMAL6:
		offset = sizeof (struct bei_normal6);
		break;
	case  SF_COPROC:
		offset = sizeof (struct bei_coproc);
		break;
	case  SF_MEDIUM:
		offset = sizeof (struct bei_medium);
		break;
	case  SF_LONGB:
		offset = sizeof (struct bei_longb);
		break;
#endif sun3
#ifdef sun2
	case  SF_LONG8:
		offset = sizeof (struct bei_long8);
		break;
#endif sun2
	default:
		printf("rte_addr:  unknown stack format %x\n", f->f_stkfmt);
		return (0);
	}

	offset += 0xa;		/* sizeof ((long)sr + pc + (short)fmt) */

	return (r->r_sp + offset);
}

/*
 * Return true if we are about to execute an rte which will set the trace bit.
 */
trace_rte(r, f)
	register struct regs *r;
	register struct stkfmt *f;
{
	register int sr, addr;

	if (((addr = rte_addr(r, f)) == 0) ||	/* not an rte instruction */
	    ((sr = peek(addr)) == -1) ||	/* shouldn't happen */
	    ((sr & PSL_T) == 0))		/* the trace bit was off */
		return (0);
	else
		return (1);
}
