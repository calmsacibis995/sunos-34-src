#ifndef lint
static  char sccsid[] = "@(#)support.c 1.1 86/09/25";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "../h/param.h"
#include "../h/errno.h"
#include "../h/time.h"
#include "../h/ptrace.h"
#include "../machine/mmu.h"
#include "../machine/pte.h"
#include "../machine/reg.h"
#include "../debug/debugger.h"

_exit()
{

	for (;;)
		;
}

int interrupted = 0;

getchar()
{
	register int c;

	while ((c = (*romp->v_mayget)()) == -1)
		;
	if (c == '\r')
		c = '\n';
	if (c == 0177 || c == '\b') {
		putchar('\b');
		putchar(' ');
		c = '\b';
	}
	putchar(c);
	return (c);
}

/*
 * Read a line into the given buffer and handles
 * erase (^H or DEL), kill (^U), and interrupt (^C) characters.
 * This routine ASSUMES a maximum input line size of LINEBUFSZ
 * to guard against overflow of the buffer from obnoxious users.
 */
gets(buf)
	char buf[];
{
	register char *lp = buf;
	register c;

	for (;;) {
		c = getchar() & 0177;
		switch(c) {
		case '\n':
		case '\r':
			*lp++ = '\0';
			return;
		case '\b':
			lp--;
			if (lp < buf)
				lp = buf;
			continue;
		case 'u'&037:			/* ^U */
			lp = buf;
			putchar('^');
			putchar('U');
			putchar('\n');
			continue;
		case 'c'&037:
			dointr(1);
			/*MAYBE REACHED*/
			/* fall through */
		default:
			if (lp < &buf[LINEBUFSZ-1]) {
				*lp++ = c;
			} else {
				putchar('\b');
				putchar(' ');
				putchar('\b');
			}
			break;
		}
	}
}

dointr(doit)
{

	putchar('^');
	putchar('C');
	interrupted = 1;
	if (abort_jmp && doit) {
		_longjmp(abort_jmp, 1);
		/*NOTREACHED*/
	}
}

/*
 * Check for ^C on input
 */
tryabort(doit)
{

	if ((*romp->v_mayget)() == ('c' & 037)) {
		dointr(doit);
		/*MAYBE REACHED*/
	}
}

/*
 * Implement pseudo ^S/^Q processing along w/ handling ^C
 * We need to strip off high order bits as monitor cannot
 * reliably figure out if the control key is depressed when
 * romp->v_mayget is called in certain circumstances.
 * Unfortunately, this means that s/q will work as well
 * as ^S/^Q and c as well as ^C when this guy is called.
 */
trypause()
{
	register int c;

	c = (*romp->v_mayget)() & 037;

	if (c == ('s' & 037)) {
		while ((c = (*romp->v_mayget)() & 037) != ('q' & 037)) {
			if (c == ('c' & 037)) {
				dointr(1);
				/*MAYBE REACHED*/
			}
		}
	} else if (c == ('c' & 037)) {
		dointr(1);
		/*MAYBE REACHED*/
	}
}

/*
 * Scaled down version of C Library printf.
 */
/*VARARGS1*/
printf(fmt, x1)
	char *fmt;
	unsigned x1;
{

	tryabort(1);
	prf(fmt, &x1);
}

prf(fmt, adx)
	register char *fmt;
	register u_int *adx;
{
	register int b, c;
	register char *s;

loop:
	while ((c = *fmt++) != '%') {
		if(c == '\0')
			return;
		putchar(c);
	}
again:
	c = *fmt++;
	switch (c) {

	case 'l':
		goto again;
	case 'x': case 'X':
		b = 16;
		goto number;
	case 'd': case 'D':
	case 'u':		/* what a joke */
		b = 10;
		goto number;
	case 'o': case 'O':
		b = 8;
number:
		printn((u_long)*adx, b);
		break;
	case 'c':
		b = *adx;
		putchar(b);
		break;
	case 's':
		s = (char *)*adx;
		while (c = *s++)
			putchar(c);
		break;
	}
	adx++;
	goto loop;
}

/*
 * Printn prints a number n in base b.
 * We don't use recursion to avoid deep kernel stacks.
 */
printn(n, b)
	register u_long n;
	register short b;
{
	char prbuf[11];
	register char *cp;

	if (b == 10 && (int)n < 0) {
		putchar('-');
		n = (unsigned)(-(int)n);
	}
	cp = prbuf;
	do {
		*cp++ = "0123456789abcdef"[n%b];
		n /= b;
	} while (n);
	do
		putchar(*--cp);
	while (cp > prbuf);
}

/*
 * Print a character on console.
 */
putchar(c)
	int c;
{

	(*romp->v_putchar)(c);
}

/*
 * Fake getpagesize() system call
 */
getpagesize()
{

	return (NBPG);
}

/*
 * Fake gettimeofday call
 * Needed for ctime - we are lazy and just
 * give a bogus approximate answer
 */
gettimeofday(tp, tzp)
	struct timeval *tp;
	struct timezone *tzp;
{

	tp->tv_sec = (1986 - 1970) * 365 * 24 * 60 * 60;	/* ~1986 */
	tzp->tz_minuteswest = 8 * 60;				/* PDT */
	tzp->tz_dsttime = DST_USA;
}

int errno;

caddr_t
sbrk(incr)
	int incr;
{
	extern char end[];
	static caddr_t lim;
	caddr_t val;
	register int i;

	if (nobrk) {
		printf("sbrk:  late call\n");
		errno = ENOMEM;
		return ((caddr_t)-1);
	}
	if (lim == 0)
		lim = (caddr_t)roundup((int)end, NBPG);
	incr = btoc(incr);
	if ((lim + ctob(incr)) >= (caddr_t)DEBUGEND) {
		printf("sbrk:  lim %x + %x exceeds %x\n", lim,
		    ctob(incr), DEBUGEND);
		errno = EINVAL;
		return ((caddr_t)-1);
	}

	val = lim;
	pagesused += incr;
	for (i = 0; i < incr; i++, lim += NBPG) {
		if (getsegmap(lim) == SEGINV) {
			register int j = (int)lim & ~SGOFSET;
			int last = j + NPAGSEG * NBPG;

			setsegmap(lim, --lastpm);

			for (; j < last; j += NBPG)
				Setpgmap(j, 0);
		}
		Setpgmap(lim, PG_V | PG_KW | PGT_OBMEM | --lastpg);
	}
	return (val);
}

#define	PHYSOFF(p, o) \
	((physadr)(p)+((o)/sizeof (((physadr)0)->r[0])))

/*
 * Fake ptrace - ignores pid and signals
 * Otherwise it's about the same except the "child" never runs,
 * flags are just set here to control action elsewhere.
 */
ptrace(request, pid, addr, data, addr2)
	enum ptracereq request;
	char *addr, *addr2;
{
	int rv = 0;
	register int i, val;
	register int *p;

	switch (request) {
	case PTRACE_TRACEME:	/* do nothing */
		break;

	case PTRACE_PEEKTEXT:
	case PTRACE_PEEKDATA:
		rv = peekl(addr);
		break;

	case PTRACE_PEEKUSER:
		if ((int)addr < 0 || (int)addr >= ctob(UPAGES)) {
			rv = -1;
			errno = EIO;
		} else
			rv = peekl((int *)PHYSOFF(UADDR, (int)addr));
		break;

	case PTRACE_POKEUSER:
		if ((int)addr < 0 || (int)addr >= ctob(UPAGES)) {
			rv = -1;
			errno = EIO;
		} else
			rv = pokel((int *)PHYSOFF(UADDR, (int)addr), data);
		break;
	
	case PTRACE_POKETEXT:
		rv = poketext(addr, data);
		break;

	case PTRACE_POKEDATA:
		rv = pokel(addr, data);
		break;

	case PTRACE_SINGLESTEP:
		dotrace = 1;
		/* fall through to ... */
	case PTRACE_CONT:
		dorun = 1;
		restoreu();		/* must be back to original u area */
		if ((int)addr != 1)
			reg->r_pc = (int)addr;
		break;

	case PTRACE_SETREGS:
		restoreu();		/* should be back to original u area */
		rv = scopy(addr, (caddr_t)reg, sizeof (struct regs));
		break;

	case PTRACE_GETREGS:
		restoreu();		/* should be back to original u area */
		rv = scopy((caddr_t)reg, addr, sizeof (struct regs));
		break;

	case PTRACE_WRITETEXT:
	case PTRACE_WRITEDATA:
		rv = scopy(addr2, addr, data);
		break;

	case PTRACE_READTEXT:
	case PTRACE_READDATA:
		rv = scopy(addr, addr2, data);
		break;

	case PTRACE_KILL:
	case PTRACE_ATTACH:
	case PTRACE_DETACH:
	default:
		errno = EINVAL;
		rv = -1;
		break;
	}
	return (rv);
}
