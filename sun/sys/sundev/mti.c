#ifndef lint
static	char sccsid[] = "@(#)mti.c 1.1 86/09/25";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Systech MTI-800/1600 Multiple Terminal Interface driver
 *
 * This driver mimics dh.c; see it for explanation of common code.
 */
#include "bk.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/bk.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/map.h"

#include "../sundev/mbvar.h"
#include "../sundev/mtireg.h"
#include "../sundev/mtivar.h"
#include "../sundev/dmctl.h"

/*
 * Driver information for auto-configuration stuff.
 */
int	mtiprobe(), mtiattach(), mtiintr();
extern	struct mb_device *mtiinfo[];
struct	mb_driver mtidriver = {
	mtiprobe, 0, mtiattach, 0, 0, mtiintr,
	sizeof (struct mtireg), "mti", mtiinfo, 0, 0, 0,
};

extern	int nmti;	/* number of MTI lines configured in */

#define	OUTLINE	0x80	/* minor device flag for dialin/out on same line */
/*#define	OUTLINE	0	/* disable dialin/out */
#define	UNIT(x)	(minor(x)&~OUTLINE)
 
int	mtistart();
int	ttrstrt();
extern	struct tty mti_tty[];

/*
 * Software carrier.
 */
extern	u_short mtisoftCAR[];

extern	u_char mtiwbits[];	/* copy of writable modem control bits */
extern	u_char mtirbits[];	/* copy of readable modem control bits */
extern	char *mtibuf[];		/* pointers to DMA buffers */
#define	MTIBUFSIZE	CBSIZE	/* size of DMA buffers */

extern	struct mti_softc mti_softc[];

static	char mti_speeds[] =
	{ 0,0,1,2,3,4,9,5,6,7,8,10,12,14,15,13 };

static	char cmdlen[] =		/* number of bytes in normal commands */
	{ 1,1,1,0,2,2,1,5,6,0,1,1,6,1,1,0 };
static	char configlen[] =	/* number of bytes in configure commands */
	{ 5,8,0,0,3,5,3,3,5,2,0,0,0,0,0,0 };
static	char resplen[] =	/* number of bytes in responses */
	{ 3,2,2,1,1,1,1,2,6,1,1,1,5,1,1,1 };
 
#ifndef PORTSELECTOR
#define	ISPEED	B9600
#define	IFLAGS	(EVENP|ODDP|ECHO|CRMOD)
#else
#define	ISPEED	B4800
#define	IFLAGS	(EVENP|ODDP)
#endif

mtiprobe(reg)
	caddr_t reg;
{

	if (peek((short *)reg) < 0)
		return (0);
	return (sizeof (struct mtireg));
}

mtiattach(md)
	register struct mb_device *md;
{
	register struct tty *tp = &mti_tty[md->md_unit*16];
	register struct mtireg *mtiaddr = (struct mtireg *)md->md_addr;
	register int cntr;
	register char *p;

	p = (char *)rmalloc(iopbmap, (long)(16*MTIBUFSIZE));
	for (cntr = 0; cntr < 16; cntr++) {
		tp->t_addr = (caddr_t)mtiaddr;
		tp++;
		mtiwbits[md->md_unit*16+cntr] = MTI_ON;
		mtibuf[md->md_unit*16+cntr] = p;
		p += MTIBUFSIZE;
	}
	mtisoftCAR[md->md_unit] = md->md_flags;
	mtiaddr->mtiie = MTI_RA;	/* enable interrupts on resp avail */
}

/*ARGSUSED*/
mtiopen(dev, flag)
	dev_t dev;
{
	register struct tty *tp;
	register int unit;
 
	unit = UNIT(dev);
	if (unit >= nmti)
		return (ENXIO);
	tp = &mti_tty[unit];
	if (tp->t_addr == 0)
		return (ENXIO);
	tp->t_oproc = mtistart;
	(void) spl5();
again:
	tp->t_state |= TS_WOPEN;
	if ((tp->t_state & TS_ISOPEN) == 0) {
		ttychars(tp);
		tp->t_ospeed = tp->t_ispeed = ISPEED;
		tp->t_flags = IFLAGS;
		/* tp->t_state |= TS_HUPCLS; */
		tp->t_xflags = 0;
		tp->t_dev = dev&~OUTLINE;	/* needed for mticmd */
		mtiparam(unit);
	} else if (tp->t_state&TS_XCLUDE && u.u_uid != 0) {
		(void) spl0();
		return (EBUSY);
	} else if ((dev & OUTLINE) && !(tp->t_state&TS_OUT)) {
		(void) spl0();
		return (ENXIO);
	}
	(void) mtimctl(tp, MTI_ON, DMSET);
	if (dev & OUTLINE)
		tp->t_state |= TS_OUT|TS_CARR_ON;
	if (mtisoftCAR[unit>>4] & (1<<(unit&0xf)))
		tp->t_state |= TS_CARR_ON;
	if ((flag & FNDELAY) == 0)
		if (((tp->t_state & TS_CARR_ON) == 0) ||
		    ((tp->t_state&TS_OUT) && (dev&OUTLINE) == 0)) {
			tp->t_state |= TS_WOPEN;
			(void) sleep((caddr_t)&tp->t_state, TTIPRI);
			goto again;
		}
	(void) spl0();
	return ((*linesw[tp->t_line].l_open)(dev&~OUTLINE, tp));
}
 
/*ARGSUSED*/
mticlose(dev, flag)
	dev_t dev;
{
	register struct tty *tp;
	register int unit, s;
	label_t sq;
 
	unit = UNIT(dev);
	tp = &mti_tty[unit];
	/*
	 * Need to supress mtiintr throughout whole close sequence because
	 * one of last things that mtiintr does is a l_rint which can be nasty
	 * if part way through a l_close or ttyclose of that tp.
	 */
	s = spl5();
	sq = u.u_qsave;
	if (setjmp(&u.u_qsave)) {
		/* close was interrupted, reset state and force close */
		u.u_qsave = sq;
		tp->t_pgrp = 0;
		tp->t_state = 0;
		goto out;
	}
	(*linesw[tp->t_line].l_close)(tp);
	/* tp->t_state &= ~TS_OUT; */
	(void) mtimctl(tp, MTI_CR_BREAK, DMBIC);
	if ((tp->t_state&(TS_HUPCLS|TS_WOPEN)) || (tp->t_state&TS_ISOPEN)==0) {
		(void) mtimctl(tp, MTI_OFF, DMSET);
		(void) sleep((caddr_t)&lbolt, TTOPRI);
	}
	ttyclose(tp);
out:
	(void) splx(s);
	wakeup((caddr_t)&tp->t_state);
}
 
mtiread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;
 
	tp = &mti_tty[UNIT(dev)];
	(*linesw[tp->t_line].l_read)(tp, uio);
}
 
mtiwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;
 
	tp = &mti_tty[UNIT(dev)];
	(*linesw[tp->t_line].l_write)(tp, uio);
}

/*ARGSUSED*/
mtiselect(dev, rw)
	dev_t dev;
	int rw;
{

	return (ttselect(dev&~OUTLINE, rw));
}
 
mtiintr()
{
	register int mti;
	register struct mtireg *mtiaddr;
	register struct mti_softc *msc;
	int serviced = 0;
 
	for (mti = 0; mti < nmti>>4; mti++) {
		mtiaddr = (struct mtireg *)mti_tty[mti<<4].t_addr;
		if (mtiaddr == NULL)
			continue;
		msc = &mti_softc[mti];
		/*
		 * If command fifo is ready, see if we have any
		 * queued commands to send out.
		 */
		if (mtiaddr->mtistat&MTI_READY) {
			register struct clist *q;
			register int n;

			q = &msc->msc_cmdq;
			if ((n = getc(q)) > 0) {
				while (n--)
					mtiaddr->mticmd = getc(q);
				mtiaddr->mtigo = 1;
				serviced++;
				if (q->c_cc == 0)
					mtiaddr->mtiie = MTI_RA;
			} else
				mtiaddr->mtiie = MTI_RA;
		}
		if (mtiaddr->mtistat&MTI_RA) {
			register int have;
			register u_char *p, *p0;

			serviced++;
			/*
			 * Determine if we have an entire response yet.
			 * If not, keep accumulating response bytes.
			 */
			have = msc->msc_have;
			p0 = msc->msc_rbuf;
			p = p0 + have;
			while (mtiaddr->mtistat & MTI_VD) {
				*p++ = mtiaddr->mtiresp;
				if (++have >= resplen[*p0>>4]) {
					mtiresponse(mti, msc);
					have = mtiaddr->mticra;
					have = 0;
					p = p0;
				}
			}
			msc->msc_have = have;
		}
	}
	return (serviced);
}

mtiresponse(mti, msc)
	int mti;
	register struct mti_softc *msc;
{
	register struct tty *tp;
	register u_char *p = msc->msc_rbuf;
	register int cmd, s, c;
	int overrun = 0;;

	cmd = *p++;
	tp = &mti_tty[(mti<<4)+(cmd&0xf)];
	cmd &= 0xf0;
	/*
	 * First, read and handle the UART status
	 * byte for commands that return it.
	 */
	switch (cmd) {
	case MTI_ESCI:
	case MTI_RSTAT:
	case MTI_CONFIG:
	case MTI_BLKOUT:
		s = *p++;
		if ((s & MTI_SR_DSR) ||
		    (mtisoftCAR[mti]&(1<<(minor(tp->t_dev)&0xf))) ||
		    (tp->t_state & TS_OUT)) {
			/* carrier present */
			if ((tp->t_state & TS_CARR_ON) == 0) {
				wakeup((caddr_t)&tp->t_state);
				tp->t_state |= TS_CARR_ON;
			}
		} else {
			if ((tp->t_state&TS_CARR_ON) &&
			    (tp->t_flags&NOHANG)==0) {
				/* carrier lost */
				if (tp->t_state&TS_ISOPEN) {
					gsignal(tp->t_pgrp, SIGHUP);
					gsignal(tp->t_pgrp, SIGCONT);
					(void) mtimctl(tp, MTI_CR_DTR,
					    DMBIC);
					ttyflush(tp, FREAD|FWRITE);
				}
				tp->t_state &= ~TS_CARR_ON;
			}
		}
		mtirbits[minor(tp->t_dev)] = s&(MTI_SR_DCD|MTI_SR_DSR);
		break;
	}
	/*
	 * Finish processing the command.
	 */
	switch (cmd) {
	case MTI_ESCI:
		c = *p++;
		if ((tp->t_state & TS_ISOPEN) == 0) {
			wakeup((caddr_t)&tp->t_rawq);
#ifdef PORTSELECTOR
			if ((tp->t_state&TS_WOPEN) == 0)
#endif
			break;
		}
		if (s&MTI_SR_FE)
			if (tp->t_flags & RAW)
				c = 0;
			else
				c = tp->t_intrc;
		if ((s&MTI_SR_DO) && overrun == 0) {
			printf("mti%d,%d: silo overflow\n", mti,
			    minor(tp->t_dev)&0xF);
			overrun = 1;
		}
		if (s&MTI_SR_PE)	
			if (((tp->t_flags & (EVENP|ODDP)) == EVENP)
			  || ((tp->t_flags & (EVENP|ODDP)) == ODDP))
				break;
#if NBK > 0
		if (tp->t_line == NETLDISC) {
			c &= 0177;
			BKINPUT(c, tp);
		} else
#endif
			(*linesw[tp->t_line].l_rint)(c, tp);
		break;

	case MTI_RERR:
		c = *p++;
		printf("mti%d: error %x\n", mti, c);
		break;

	case MTI_BLKOUT:
		c = *p++;		/* get termination cause */
		if (c & 4)
			printf("mti%d: DMA output error\n", mti);
		c = *p++;	/* get number of characters transfered */
		c |= *p++ << 8;
		tp->t_state &= ~TS_BUSY;
		if (tp->t_state & TS_FLUSH)
			tp->t_state &= ~TS_FLUSH;
		else
			ndflush(&tp->t_outq, c);
		if (tp->t_line)
			(*linesw[tp->t_line].l_start)(tp);
		else
			mtistart(tp);
		break;

	case MTI_RSTAT:
	case MTI_CONFIG:
		break;

	case MTI_BLKIN:
	default:
		printf("mti%d: impossible response %x\n", mti, *msc->msc_rbuf);
	}
}
 
/*ARGSUSED*/
mtiioctl(dev, cmd, data, flag)
	dev_t dev;
	caddr_t data;
{
	register struct tty *tp;
	register int unit = UNIT(dev);
	int error;
 
	tp = &mti_tty[unit];
	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag);
	if (error < 0)
		error = ttioctl(tp, cmd, data, flag);
	if (error >= 0) {
		switch (cmd) {
		case TIOCSETP:
		case TIOCSETN:
		case TIOCLSET:
		case TIOCLBIS:
		case TIOCLBIC:
		case TIOCSETX:
			mtiparam(unit);
		}
		return (error);
	}
	switch(cmd) {

	case TIOCSBRK:
		(void) mtimctl(tp, MTI_CR_BREAK, DMBIS);
		break;
	case TIOCCBRK:
		(void) mtimctl(tp, MTI_CR_BREAK, DMBIC);
		break;
	case TIOCSDTR:
		(void) mtimctl(tp, MTI_CR_DTR, DMBIS);
		break;
	case TIOCCDTR:
		(void) mtimctl(tp, MTI_CR_DTR, DMBIC);
		break;
	case TIOCMSET:
		(void) mtimctl(tp, dmtomti(*(int *)data), DMSET);
		break;
	case TIOCMBIS:
		(void) mtimctl(tp, dmtomti(*(int *)data), DMBIS);
		break;
	case TIOCMBIC:
		(void) mtimctl(tp, dmtomti(*(int *)data), DMBIC);
		break;
	case TIOCMGET:
		*(int *)data = mtitodm(mtimctl(tp, 0, DMGET));
		break;
	default:
		return (ENOTTY);
	}
	return (0);
}

dmtomti(bits)
	register int bits;
{
	register int b = 0;

	if (bits & DML_DTR) b |= MTI_CR_DTR;
	if (bits & DML_CAR) b |= MTI_SR_DSR;
	if (bits & DML_RTS) b |= MTI_CR_RTS;
	if (bits & DML_DSR) b |= MTI_SR_DSR;
	return (b);
}

mtitodm(bits)
	register int bits;
{
	register int b = 0;

	if (bits & MTI_CR_DTR) b |= DML_DTR;
	if (bits & MTI_SR_DSR) b |= DML_CAR|DML_DSR;
	if (bits & MTI_CR_RTS) b |= DML_RTS;
	return (b);
}
 
mtiparam(unit)
	register int unit;
{
	register struct tty *tp;
	register int mr1, mr2;
	int s = spl5();
 
	tp = &mti_tty[unit];
	if (tp->t_ispeed == 0) {
		(void) mtimctl(tp, MTI_OFF, DMSET); /* hang up */
		(void) splx(s);
		return;
	}
	mr2 = mti_speeds[tp->t_ispeed&017] | MTI_MR2_INIT;
	if (tp->t_ispeed == B134)
		mr1 = MTI_MR1_BITS6|MTI_MR1_PENABLE|MTI_MR1_EPAR|MTI_MR1_X1_CLK;
	else if ((tp->t_flags&(RAW|LITOUT|PASS8)) ||
	    ((tp->t_flags&(EVENP|ODDP)) == 0))
		mr1 = MTI_MR1_BITS8|MTI_MR1_X1_CLK;
	else
		mr1 = MTI_MR1_BITS7|MTI_MR1_PENABLE|MTI_MR1_X1_CLK;
	if (tp->t_flags & EVENP)
		mr1 |= MTI_MR1_EPAR;
	if (tp->t_ispeed == B110 || tp->t_xflags & STOPB)
		mr1 |= MTI_MR1_2STOP;
	else if (tp->t_ispeed == B134)
		mr1 |= MTI_MR1_1_5STOP;
	else
		mr1 |= MTI_MR1_1STOP;
	mticmd(tp, MTI_CONFIG, MTIC_ASYNC, mr1, mr2,
	    MTI_CR_INIT | mtiwbits[unit]);
	mticmd(tp, MTI_ESCI);
	mticmd(tp, MTI_CONFIG, MTIC_MODEM, 1);
	mticmd(tp, MTI_CONFIG, MTIC_OUTPUT, 1);
	mticmd(tp, MTI_RSTAT);
	(void) splx(s);
}
 
mtistart(tp)
	register struct tty *tp;
{
	register int n, cc;
	register char *p;
	int s;
 
	s = spl5();
	if (tp->t_state & (TS_TIMEOUT|TS_BUSY|TS_TTSTOP))
		goto out;
	if (tp->t_outq.c_cc <= TTLOWAT(tp)) {
		if (tp->t_state&TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			wakeup((caddr_t)&tp->t_outq);
		}
		if (tp->t_wsel) {
			selwakeup(tp->t_wsel, tp->t_state & TS_WCOLL);
			tp->t_wsel = 0;
			tp->t_state &= ~TS_WCOLL;
		}
	}
	if (tp->t_outq.c_cc == 0)
		goto out;
	if (tp->t_flags & (RAW|LITOUT))
		cc = ndqb(&tp->t_outq, 0);
	else {
		cc = ndqb(&tp->t_outq, 0200);
		if (cc == 0) {
			cc = getc(&tp->t_outq);
			timeout(ttrstrt, (caddr_t)tp, (cc&0x7f) + 6);
			tp->t_state |= TS_TIMEOUT;
			goto out;
		}
	}
	p = mtibuf[minor(tp->t_dev)];
	swab(tp->t_outq.c_cf, p, cc);
	n = p - DVMA;
	mticmd(tp, MTI_BLKOUT, n&0xff, (n>>8)&0xff, (n>>16)&0xff, cc, 0);
	tp->t_state |= TS_BUSY;
out:
	(void) splx(s);
}

/*
 * Stop output on a line.
 */
/*ARGSUSED*/
mtistop(tp, flag)
	register struct tty *tp;
{
	int s = spl5();

	if (tp->t_state & TS_BUSY) {
		mticmd(tp, MTI_ABORTOUT);
		if ((tp->t_state&TS_TTSTOP)==0)
			tp->t_state |= TS_FLUSH;
	}
	(void) splx(s);
}
 
mtimctl(tp, bits, how)
	struct tty *tp;
	int bits, how;
{
	register int unit, mbits, s;

	unit = minor(tp->t_dev);
	s = spl5();
	mbits = mtiwbits[unit] | mtirbits[unit];
	switch (how) {
	case DMSET:
		mbits = bits;
		break;

	case DMBIS:
		mbits |= bits;
		break;

	case DMBIC:
		mbits &= ~bits;
		break;

	case DMGET:
		(void) splx(s);
		return (mbits);
	}
	mtiwbits[unit] = mbits&(MTI_CR_DTR|MTI_CR_RTS|MTI_CR_BREAK);
	mticmd(&mti_tty[unit], MTI_WCMD, mtiwbits[unit]|MTI_CR_INIT);
	(void) splx(s);
	return (mbits);
}

/*VARARGS2*/
/*ARGSUSED*/
mticmd(tp, cmd, a1, a2, a3, a4, a5, a6)
	struct tty *tp;
	int cmd;
{
	register struct mtireg *mtiaddr = (struct mtireg *)tp->t_addr;
	register int n, *p, useq = 0;
	register struct clist *q = &mti_softc[minor(tp->t_dev)>>4].msc_cmdq;

top:
	n = 0;
	if (q->c_cc)
		useq = 1;
	else
		while ((mtiaddr->mtistat & MTI_READY) == 0)
			if (n++ > 30) {		/* 30 determined empirically */
				mtiaddr->mtiie = MTI_RA|MTI_READY;
				if (mtiaddr->mtistat & MTI_READY)
					mtiaddr->mtiie = MTI_RA;
				else
					useq = 1;
				break;
			}
	if (!useq && mtiaddr->mtistat & MTI_ERR) {
		mtiaddr->mticmd = MTI_RERR;
		mtiaddr->mtigo = 1;
		goto top;
	}
	if (cmd == MTI_CONFIG)
		n = configlen[a1>>4];
	else
		n = cmdlen[cmd>>4];
	cmd |= minor(tp->t_dev)&0xf;
	p = &cmd;
	if (useq) {
		(void) putc(n, q);
		while (n--)
			(void) putc(*p++, q);
	} else {
		while (n--)
			mtiaddr->mticmd = *p++;
		mtiaddr->mtigo = 1;
	}
}
 
/*
 * Reset state of driver if Multibus reset was necessary.
 * Reset parameters and restart transmission on open lines.
 */
mtireset()
{
	register int unit;
	register struct tty *tp;
	register struct mb_device *md;

	for (unit = 0; unit < nmti; unit++) {
		md = mtiinfo[unit >> 4];
		if (md == 0 || md->md_alive == 0)
			continue;
		if ((unit&0xf) == 0)
			printf(" mti%d", unit>>4);
		tp = &mti_tty[unit];
		if (tp->t_state & (TS_ISOPEN|TS_WOPEN)) {
			mtiparam(unit);
			(void) mtimctl(tp, MTI_ON, DMSET);
			tp->t_state &= ~TS_BUSY;
			mtistart(tp);
		}
	}
}
