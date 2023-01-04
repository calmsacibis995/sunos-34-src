/*	@(#)subr_prf.c 1.1 86/09/25 SMI; from UCB 6.1 83/07/29	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/seg.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/reboot.h"
#include "../h/vm.h"
#include "../h/msgbuf.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/tty.h"
#include "../h/debug.h"

#ifdef vax
#include "../vax/mtpr.h"
#endif
#ifdef sun
#include "../sun/consdev.h"
#endif

/*
 * In case console is off,
 * panicstr contains argument to last
 * call to panic.
 */
char	*panicstr;

/*
 * Place to save regs on panic
 * so adb can find them later.
 */
label_t panic_regs;

int	noprintf = 0;	/* patch to non-zero to suppress kernel printf's */

/*
 * Scaled down version of C Library printf.
 * Used to print diagnostic information directly on console tty.
 * Since it is not interrupt driven, all system activities are
 * suspended.  Printf should not be used for chit-chat.
 *
 * One additional format: %b is supported to decode error registers.
 * Usage is:
 *	printf("reg=%b\n", regval, "<base><arg>*");
 * Where <base> is the output base expressed as a control character,
 * e.g. \10 gives octal; \20 gives hex.  Each arg is a sequence of
 * characters, the first of which gives the bit number to be inspected
 * (origin 1), and the next characters (up to a control character, i.e.
 * a character <= 32), give the name of the register.  Thus
 *	printf("reg=%b\n", 3, "\10\2BITTWO\1BITONE\n");
 * would produce output:
 *	reg=2<BITTWO,BITONE>
 */
/*VARARGS1*/
printf(fmt, x1)
	char *fmt;
	unsigned x1;
{

	prf(fmt, &x1, 0);
}

/*
 * Uprintf prints to the current user's terminal,
 * guarantees not to sleep (so can be called by interrupt routines)
 * and does no watermark checking - (so no verbose messages).
 */
/*VARARGS1*/
uprintf(fmt, x1)
	char *fmt;
	unsigned x1;
{

	prf(fmt, &x1, 2);
}

prf(fmt, adx, touser)
	register char *fmt;
	register u_int *adx;
{
	register int b, c, i;
	char *s;
	int any;

loop:
	while ((c = *fmt++) != '%') {
		if(c == '\0')
			return;
		putchar(c, touser);
	}
again:
	c = *fmt++;
	/* THIS CODE IS VAX DEPENDENT IN HANDLING %l? AND %c */
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
		printn((u_long)*adx, b, touser);
		break;
	case 'c':
		b = *adx;
		for (i = 24; i >= 0; i -= 8)
			if (c = (b >> i) & 0x7f)
				putchar(c, touser);
		break;
	case 'b':
		b = *adx++;
		s = (char *)*adx;
		printn((u_long)b, *s++, touser);
		any = 0;
		if (b) {
			putchar('<', touser);
			while (i = *s++) {
				if (b & (1 << (i-1))) {
					if (any)
						putchar(',', touser);
					any = 1;
					for (; (c = *s) > 32; s++)
						putchar(c, touser);
				} else
					for (; *s > 32; s++)
						;
			}
			if (any)
				putchar('>', touser);
		}
		break;

	case 's':
		s = (char *)*adx;
		while (c = *s++)
			putchar(c, touser);
		break;

	case '%':
		putchar('%', touser);
		break;
	}
	adx++;
	goto loop;
}

/*
 * Printn prints a number n in base b.
 * We don't use recursion to avoid deep kernel stacks.
 */
printn(n, b, touser)
	u_long n;
{
	char prbuf[11];
	register char *cp;

	if (b == 10 && (int)n < 0) {
		putchar('-', touser);
		n = (unsigned)(-(int)n);
	}
	cp = prbuf;
	do {
		*cp++ = "0123456789abcdef"[n%b];
		n /= b;
	} while (n);
	do
		putchar(*--cp, touser);
	while (cp > prbuf);
}

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: mesg", and then reboots.
 * If we are called twice, then we avoid trying to
 * sync the disks as this often leads to recursive panics.
 */

int panic_opt = RB_AUTOBOOT;

panic(s)
	char *s;
{
	if (panicstr)
		panic_opt |= RB_NOSYNC;
	else {
		panicstr = s;
		noprintf = 0;	/* turn printfs on */
		if (u.u_procp && u.u_procp->p_addr) {
#ifdef sun	/* XXX */
			(void) setjmp(&u.u_pcb.pcb_regs);
			panic_regs = u.u_pcb.pcb_regs;
#endif
		}
	}
	printf("panic: %s\n", s);
	boot(RB_PANIC, panic_opt);
}

/*
 * Warn that a system table is full.
 */
tablefull(tab)
	char *tab;
{

	printf("%s: table is full\n", tab);
}

/*
 * Hard error is the preface to plaintive error messages
 * about failing disk transfers.
 */
harderr(bp, cp)
	struct buf *bp;
	char *cp;
{

	printf("%s%d%c: hard error sn%d ", cp,
	    dkunit(bp), 'a'+(minor(bp->b_dev)&07), bp->b_blkno);
}

#ifdef DEBUG
/*
 * Called by the ASSERT macro in debug.h when an assertion fails.
 */
assfail(a, f, l)
	char *a, *f;
	int l;
{

	noprintf = 0;
	panicstr = "";
	printf("assertion failed: %s, file: %s, line: %d\n", a, f, l);
	panicstr = 0;
	panic("assertion failed");
}
#endif

/*
 * Print a character on console or users terminal.
 * If destination is console then the last MSGBUFS characters
 * are saved in msgbuf for inspection later.
 */
/*ARGSUSED*/
putchar(c, touser)
	register int c, touser;
{
	register struct tty *tp = 0;
#ifdef sun
	extern int msgbufinit;
#endif

	if (touser && (tp = u.u_ttyp) == NULL)
		touser = 0;
#ifdef sun
	if (!touser && panicstr == 0 && consdev != rconsdev)
		tp = cdevsw[major(consdev)].d_ttys+minor(consdev);
#endif
	if (tp && (tp->t_state&TS_CARR_ON) && !noprintf) {
		register int s = spl6();

		if (c == '\n')
			(void) ttyoutput('\r', tp);
		(void) ttyoutput(c, tp);
		ttstart(tp);
		(void) splx(s);
	} else
		tp = 0;
	if (touser)
		return;
	if (c != '\0' && c != '\r' && c != 0177
#ifdef vax
	    && mfpr(MAPEN)
#endif
#ifdef sun
	    && msgbufinit
#endif
	    ) {
		if (msgbuf.msg_magic != MSG_MAGIC) {
			register int i;

			msgbuf.msg_bufx = 0;
			msgbuf.msg_magic = MSG_MAGIC;
			for (i=0; i < MSG_BSIZE; i++)
				msgbuf.msg_bufc[i] = 0;
		}
		if (msgbuf.msg_bufx < 0 || msgbuf.msg_bufx >= MSG_BSIZE)
			msgbuf.msg_bufx = 0;
		msgbuf.msg_bufc[msgbuf.msg_bufx++] = c;
	}
	if (!tp && c && !noprintf)
		cnputc(c);
}

/*
 * Print out the message buffer.
 */
int	prtmsgbuflines = 12;

prtmsgbuf()
{
	register int c, l;
	static int ndx;

	for (l = 0; l < prtmsgbuflines; l++)
		do {
			if (ndx < 0 || ndx >= MSG_BSIZE) {
				char *msg = "\n<top of msgbuf>\n";

				while (*msg)
					cnputc(*msg++);
				ndx = 0;
			}
			cnputc(c = msgbuf.msg_bufc[ndx++]);
		} while (c != '\n');
}
