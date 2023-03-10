/*    @(#)tty_pty.c 1.4 86/12/30 SMI; from UCB 6.14 9/4/85        */

/*
 * Pseudo-teletype Driver
 * (Actually two drivers, requiring two entries in 'cdevsw')
 */
#include "pty.h"

#if NPTY > 0
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../h/proc.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#ifdef sun
#include "../sun/consdev.h"
#define sigmask(m)	(1 << ((m)-1))
#endif

#if NPTY == 1
#undef NPTY
#define	NPTY	48		/* crude XXX */
#endif

#define BUFSIZ 100		/* Chunk size iomoved to/from user */

/*
 * pts == /dev/tty[pqrs]?
 * ptc == /dev/pty[pqrs]?
 */
struct	tty pt_tty[NPTY];
struct	pt_ioctl {
	int	pt_flags;		/* flags, see below */
	struct	proc *pt_selr, *pt_selw;/* procs selecting on read and write*/
	struct	proc *pt_sele;		/* proc selecting on exception */
	u_char	pt_send;		/* pending message to controller */
	u_char	pt_ucntl;		/* pending iocontrol for controller */
	struct clist pt_stuffq;		/* chars to be stuffed to input */
} pt_ioctl[NPTY];
int	npty = NPTY;		/* for pstat -t */
int	pt_smajor;		/* for console hook */

#define	PF_RCOLL	0x01
#define	PF_WCOLL	0x02
#define	PF_NBIO		0x04
#define	PF_PKT		0x08		/* packet mode */
#define	PF_STOPPED	0x10		/* user told stopped */
#define	PF_REMOTE	0x20		/* remote and flow controlled input */
#define	PF_NOSTOP	0x40
#define PF_UCNTL	0x80		/* user control mode */

/*ARGSUSED*/
ptsopen(dev, flag)
	dev_t dev;
{
	register struct tty *tp;
	int error;

#ifdef lint
	npty = npty;
#endif
	if (minor(dev) >= NPTY)
		return (ENXIO);
	tp = &pt_tty[minor(dev)];
	if ((tp->t_state & TS_ISOPEN) == 0) {
		ttychars(tp);		/* Set up default chars */
		tp->t_ispeed = tp->t_ospeed = EXTB;
		tp->t_flags = 0;	/* No features (nor raw mode) */
	} else if (tp->t_state&TS_XCLUDE && u.u_uid != 0)
		return (EBUSY);
	if (tp->t_oproc)			/* Ctrlr still around. */
		tp->t_state |= TS_CARR_ON;
	while ((tp->t_state & TS_CARR_ON) == 0) {
		tp->t_state |= TS_WOPEN;
		(void) sleep((caddr_t)&tp->t_rawq, TTIPRI);
	}
	error = (*linesw[tp->t_line].l_open)(dev, tp);
	ptcwakeup(tp, FREAD|FWRITE);
	return (error);
}

ptsclose(dev)
	dev_t dev;
{
	register struct tty *tp;

	tp = &pt_tty[minor(dev)];
	(*linesw[tp->t_line].l_close)(tp);
	ttyclose(tp);
	ptcwakeup(tp, FREAD|FWRITE);
}

ptsread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp = &pt_tty[minor(dev)];
	register struct pt_ioctl *pti = &pt_ioctl[minor(dev)];
	int error = 0;

again:
	if (pti->pt_flags & PF_REMOTE) {
		while (tp == u.u_ttyp && u.u_procp->p_pgrp != tp->t_pgrp) {
			if ((u.u_procp->p_sigignore & sigmask(SIGTTIN)) ||
			    (u.u_procp->p_sigmask & sigmask(SIGTTIN)) ||
			    u.u_procp->p_flag&SVFORK)
				return (EIO);
			gsignal(u.u_procp->p_pgrp, SIGTTIN);
			(void) sleep((caddr_t)&lbolt, TTIPRI);
		}
		if (tp->t_canq.c_cc == 0) {
			if (tp->t_state & TS_NBIO)
				return (EWOULDBLOCK);
			(void) sleep((caddr_t)&tp->t_canq, TTIPRI);
			goto again;
		}
		while (tp->t_canq.c_cc > 1 && uio->uio_resid > 0)
			if (ureadc(getc(&tp->t_canq), uio) < 0) {
				error = EFAULT;
				break;
			}
		if (tp->t_canq.c_cc == 1)
			(void) getc(&tp->t_canq);
		if (tp->t_canq.c_cc)
			return (error);
	} else
		if (tp->t_oproc) {
			  /*
			   * if the REMOTE bit comes on while we are sleeping,
			   * then we don't want to return here
			   */
			error = (*linesw[tp->t_line].l_read)(tp, uio);
			if (pti->pt_flags & PF_REMOTE) goto again;
		}
	ptcwakeup(tp, FWRITE);
	return (error);
}

/*
 * Write to pseudo-tty.
 * Wakeups of controlling tty will happen
 * indirectly, when tty driver calls ptsstart.
 */
ptswrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;

	tp = &pt_tty[minor(dev)];
	if (tp->t_oproc == 0)
		return (EIO);
	return ((*linesw[tp->t_line].l_write)(tp, uio));
}

/*
 * Start output on pseudo-tty.
 * Wake up process selecting or sleeping for input from controlling tty.
 */
ptsstart(tp)
	struct tty *tp;
{
	register struct pt_ioctl *pti = &pt_ioctl[minor(tp->t_dev)];

	if (tp->t_state & TS_TTSTOP)
		return;
	if (pti->pt_flags & PF_STOPPED) {
		pti->pt_flags &= ~PF_STOPPED;
		pti->pt_send = TIOCPKT_START;
	}
	ptcwakeup(tp, FREAD);
}

ptcwakeup(tp, flag)
	struct tty *tp;
{
	struct pt_ioctl *pti = &pt_ioctl[minor(tp->t_dev)];

	if (flag & FREAD) {
		if (pti->pt_selr) {
			selwakeup(pti->pt_selr, pti->pt_flags & PF_RCOLL);
			pti->pt_selr = 0;
			pti->pt_flags &= ~PF_RCOLL;
		}
		wakeup((caddr_t)&tp->t_outq.c_cf);
	}
	if (flag & FWRITE) {
		if (pti->pt_selw) {
			selwakeup(pti->pt_selw, pti->pt_flags & PF_WCOLL);
			pti->pt_selw = 0;
			pti->pt_flags &= ~PF_WCOLL;
		}
		wakeup((caddr_t)&tp->t_rawq.c_cf);
	}
}

/*ARGSUSED*/
ptcopen(dev, flag)
	dev_t dev;
	int flag;
{
	register struct tty *tp;
	struct pt_ioctl *pti;

	if (minor(dev) >= NPTY)
		return (ENXIO);
	tp = &pt_tty[minor(dev)];
	if (tp->t_oproc)
		return (EIO);
	tp->t_oproc = ptsstart;
	if (tp->t_state & TS_WOPEN)
		wakeup((caddr_t)&tp->t_rawq);
	tp->t_state |= TS_CARR_ON;
	pti = &pt_ioctl[minor(dev)];
	pti->pt_flags = 0;
	pti->pt_send = 0;
	pti->pt_ucntl = 0;
	if (pt_smajor == 0) {
		register struct cdevsw *c;

		for (c = &cdevsw[nchrdev]; --c >= cdevsw; )
			if (c->d_open == ptsopen) {
				pt_smajor = c - cdevsw;
				return (0);
			}
		panic("ptcopen");
	}
	return (0);
}

ptcclose(dev)
	dev_t dev;
{
	register struct tty *tp;
	struct pt_ioctl *pti = &pt_ioctl[minor(dev)];

	tp = &pt_tty[minor(dev)];
	tp->t_oproc = 0;		/* mark closed */
	if (tp->t_line)
		(*linesw[tp->t_line].l_close)(tp);
	if (tp->t_state & TS_ISOPEN)
		gsignal(tp->t_pgrp, SIGHUP);
	tp->t_state &= ~TS_CARR_ON;	/* virtual carrier gone */
#ifdef sun
	if (major(consdev) == pt_smajor && minor(consdev) == minor(dev))
		consdev = rconsdev;
#endif
	while (pti->pt_stuffq.c_cc > 0) {
		(void)getc(&pti->pt_stuffq);
		wakeup((caddr_t)&pti->pt_stuffq.c_cf);
	}
	ttyflush(tp, FREAD|FWRITE);
}

ptcread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp = &pt_tty[minor(dev)];
	struct pt_ioctl *pti = &pt_ioctl[minor(dev)];
	char buf[BUFSIZ];
	int error = 0, cc;

	/*
	 * We want to block until the slave
	 * is open, and there's something to read;
	 * but if we lost the slave or we're NBIO,
	 * then return the appropriate error instead.
	 */
	for (;;) {
		if (tp->t_state&TS_ISOPEN) {
			if (pti->pt_flags&PF_PKT && pti->pt_send) {
				error = ureadc((int)pti->pt_send, uio);
				if (error)
					return (error);
				pti->pt_send = 0;
				return (0);
			}
			if (pti->pt_flags&PF_UCNTL && pti->pt_ucntl) {
			  /* 
			   * User-control mode: pass ioctl code in first byte.
			   */
			    error = ureadc((int)pti->pt_ucntl, uio);
			    if (error)
				return (error);
			    pti->pt_ucntl = 0;
			    return (0);
			}
			if (tp->t_outq.c_cc && (tp->t_state&TS_TTSTOP) == 0)
				break;
			if (pti->pt_flags&PF_UCNTL 
			    && (pti->pt_stuffq.c_cc > 0)) {
			  /* 
			   * User-control mode: pass any stuffed characters
			   * AFTER output
			   */
			    error = ureadc( (TIOCSTI&0xff), uio);
			    while (error==0 && pti->pt_stuffq.c_cc > 0 &&
				   uio->uio_resid > 0) {
				error = ureadc( getc(&pti->pt_stuffq), uio);
				}
			    wakeup((caddr_t)&pti->pt_stuffq.c_cf);
			    if (error)
				return (error);
			    return (0);
			}
		}
		if ((tp->t_state&TS_CARR_ON) == 0)
			return (EIO);
		if (pti->pt_flags&PF_NBIO)
			return (EWOULDBLOCK);
		(void) sleep((caddr_t)&tp->t_outq.c_cf, TTIPRI);
	}
	if (pti->pt_flags & (PF_PKT|PF_UCNTL))
		error = ureadc(0, uio);
	while (uio->uio_resid > 0 && error == 0) {
		cc = q_to_b(&tp->t_outq, buf, MIN(uio->uio_resid, BUFSIZ));
		if (cc <= 0)
			break;
		error = uiomove(buf, cc, UIO_READ, uio);
	}
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
	return (error);
}

ptsstop(tp, flush)
	register struct tty *tp;
	int flush;
{
	struct pt_ioctl *pti = &pt_ioctl[minor(tp->t_dev)];
	int flag;

	/* note: FLUSHREAD and FLUSHWRITE already ok */
	if (flush == 0) {
		flush = TIOCPKT_STOP;
		pti->pt_flags |= PF_STOPPED;
	} else
		pti->pt_flags &= ~PF_STOPPED;
	pti->pt_send |= flush;
	/* change of perspective */
	flag = 0;
	if (flush & FREAD)
		flag |= FWRITE;
	if (flush & FWRITE)
		flag |= FREAD;
	if (pti->pt_sele) {
		selwakeup(pti->pt_sele, 0);
		pti->pt_sele = 0;
	}
	ptcwakeup(tp, flag);
}

ptcselect(dev, rw)
	dev_t dev;
	int rw;
{
	register struct tty *tp = &pt_tty[minor(dev)];
	struct pt_ioctl *pti = &pt_ioctl[minor(dev)];
	struct proc *p;
	int s, block;

	if ((tp->t_state&TS_CARR_ON) == 0)
		return (1);
	s = spl5();
	switch (rw) {

	case FREAD:
		if ((tp->t_state&TS_ISOPEN) &&
		     ((tp->t_outq.c_cc && (tp->t_state&TS_TTSTOP) == 0) 
		 || (pti->pt_flags&PF_UCNTL && 
		     (pti->pt_ucntl || pti->pt_stuffq.c_cc) )) ) {
			(void) splx(s);
			return (1);
		}
		if ((p = pti->pt_selr) && p->p_wchan == (caddr_t)&selwait)
			pti->pt_flags |= PF_RCOLL;
		else
			pti->pt_selr = u.u_procp;
		break;

	case FWRITE:
		block = 0;
		if (pti->pt_flags & PF_REMOTE) {
			if (tp->t_canq.c_cc)
				block++;
		} else {
			if (tp->t_rawq.c_cc + tp->t_canq.c_cc >= TTYHOG-2)
				block++;
		}
		if ( (tp->t_state&TS_ISOPEN) && !block) {
			(void) splx(s);
			return (1);
		}
		if ((p = pti->pt_selw) && p->p_wchan == (caddr_t)&selwait)
			pti->pt_flags |= PF_WCOLL;
		else
			pti->pt_selw = u.u_procp;
		break;

	case 0:			/* "exceptional conditions" */
		if ((tp->t_state&TS_ISOPEN) &&
		    (pti->pt_flags&PF_PKT && pti->pt_send ||
		     pti->pt_flags&PF_UCNTL && pti->pt_ucntl)) {
			(void) splx(s);
			return (1);
		}
		pti->pt_sele = u.u_procp;
		break;
	}
	(void) splx(s);
	return (0);
}

ptcwrite(dev, uio)
	dev_t dev;
	register struct uio *uio;
{
	register struct tty *tp = &pt_tty[minor(dev)];
	register struct iovec *iov;
	register char *cp;
	register int cc = 0;
	char locbuf[BUFSIZ];
	int cnt = 0;
	struct pt_ioctl *pti = &pt_ioctl[minor(dev)];
	int error = 0;

again:
	if ((tp->t_state&TS_ISOPEN) == 0)
		goto block;
	if (pti->pt_flags & PF_REMOTE) {
		if (tp->t_canq.c_cc)
			goto block;
		while (uio->uio_iovcnt > 0 && tp->t_canq.c_cc < TTYHOG - 1) {
			iov = uio->uio_iov;
			if (iov->iov_len == 0) {
				uio->uio_iovcnt--;	
				uio->uio_iov++;
				continue;
			}
			if (cc == 0) {
				cc = MIN(iov->iov_len, BUFSIZ);
				cc = MIN(cc, TTYHOG - 1 - tp->t_canq.c_cc);
				cp = locbuf;
				error = uiomove(cp, cc, UIO_WRITE, uio);
				if (error)
					return (error);
				/* check again for safety */
				if ((tp->t_state&TS_ISOPEN) == 0)
					return (EIO);
			}
			if (cc)
				(void) b_to_q(cp, cc, &tp->t_canq);
			cc = 0;
		}
		(void) putc(0, &tp->t_canq);
		ttwakeup(tp);
		wakeup((caddr_t)&tp->t_canq);
		return (0);
	}
	while (uio->uio_iovcnt > 0) {
		iov = uio->uio_iov;
		if (cc == 0) {
			if (iov->iov_len == 0) {
				uio->uio_iovcnt--;	
				uio->uio_iov++;
				continue;
			}
			cc = MIN(iov->iov_len, BUFSIZ);
			cp = locbuf;
			error = uiomove(cp, cc, UIO_WRITE, uio);
			if (error)
				return (error);
			/* check again for safety */
			if ((tp->t_state&TS_ISOPEN) == 0)
				return (EIO);
		}
		while (cc > 0) {
			if ((tp->t_rawq.c_cc + tp->t_canq.c_cc) >= TTYHOG - 2 &&
			    (tp->t_canq.c_cc > 0 ||
			       tp->t_flags & (RAW|CBREAK))) {
				wakeup((caddr_t)&tp->t_rawq);
				goto block;
			}
			(*linesw[tp->t_line].l_rint)(*cp++, tp);
			cnt++;
			cc--;
		}
		cc = 0;
	}
	return (0);
block:
	/*
	 * Come here to wait for slave to open, for space
	 * in outq, or space in rawq.
	 */
	if ((tp->t_state&TS_CARR_ON) == 0)
		return (EIO);
	if (pti->pt_flags & PF_NBIO) {
		iov->iov_base -= cc;
		iov->iov_len += cc;
		uio->uio_resid += cc;
		uio->uio_offset -= cc;
		if (cnt == 0)
			return (EWOULDBLOCK);
		return (0);
	}
	(void) sleep((caddr_t)&tp->t_rawq.c_cf, TTOPRI);
	goto again;
}

/*ARGSUSED*/
ptyioctl(dev, cmd, data, flag)
	caddr_t data;
	dev_t dev;
{
	register struct tty *tp = &pt_tty[minor(dev)];
	register struct pt_ioctl *pti = &pt_ioctl[minor(dev)];
	int stop, error;

	/* IF CONTROLLER STTY THEN MUST FLUSH TO PREVENT A HANG ??? */
	if (cdevsw[major(dev)].d_open == ptcopen)
		switch (cmd) {

		case TIOCPKT:
			if (*(int *)data) {
				if (pti->pt_flags & PF_UCNTL)
					return (EINVAL);
				pti->pt_flags |= PF_PKT;
			} else
				pti->pt_flags &= ~PF_PKT;
			return (0);

		case TIOCTCNTL:
			if (*(int *)data) {
				if (pti->pt_flags & PF_PKT)
					return (EINVAL);
				pti->pt_flags |= PF_UCNTL;
			} else {
				pti->pt_flags &= ~PF_UCNTL;
			}
			return (0);

		case TIOCREMOTE:
			if (*(int *)data)
				pti->pt_flags |= PF_REMOTE;
			else {
				pti->pt_flags &= ~PF_REMOTE;
				wakeup((caddr_t)&tp->t_canq);
			}
			return (0);

		case TIOCSIGNAL:
			gsignal(tp->t_pgrp, *(int *)data);
			return(0);

		case FIONBIO:
			if (*(int *)data)
				pti->pt_flags |= PF_NBIO;
			else
				pti->pt_flags &= ~PF_NBIO;
			return (0);

		case TIOCSETP:
			while (getc(&tp->t_outq) >= 0)
				;
			break;
		}
	switch (cmd) {

	case TIOCSSIZE:		/* XXX */
		/*
		 * If the upper 16 bits of the number of lines is
		 * non-zero, it was probably a TIOCSWINSZ instead,
		 * (which, alas, shares the same "ioctl" code).
		 * with both "ws_row" and "ws_col" non-zero.
		 * We pass TIOCSWINSZ on to the regular TTY driver.
		 */
		if ((((struct ttysize *)data)->ts_lines&0xffff0000) != 0)
			break;

	case _N_TIOCSSIZE:
		tp->t_nlines = ((struct ttysize *)data)->ts_lines;
		tp->t_ncols = ((struct ttysize *)data)->ts_cols;
		return (0);

	case TIOCGSIZE:
	case _N_TIOCGSIZE:	/* XXX */
		((struct ttysize *)data)->ts_lines = tp->t_nlines;
		((struct ttysize *)data)->ts_cols = tp->t_ncols;
		return (0);

#ifdef sun
	case TIOCCONS:
	case _N_TIOCCONS:	/* XXX */
		if ((tp->t_state & TS_ISOPEN) == 0)
			return (EINVAL);
		consdev = makedev(pt_smajor, minor(dev));
		return (0);
#endif
	case TIOCSTI:		/* snarf stuffed input if in REMOTE mode */
		if ((pti->pt_flags & PF_REMOTE)==0) break;
		if (pti->pt_flags & PF_UCNTL) {
			while (pti->pt_stuffq.c_cc > TTYHOG)
			    (void) sleep((caddr_t)&pti->pt_stuffq.c_cf, TTIPRI);
			(void) putc(*(char *)data, &pti->pt_stuffq);
			ptcwakeup(tp, FREAD);
		}
		return (0);
	}
	error = ttioctl(tp, cmd, data, flag);
	if ( (pti->pt_flags & PF_UCNTL)   &&
	     (cmd & (IOC_INOUT | 0xff00)) == (IOC_IN|('t'<<8) )  &&
	     (cmd & 0xff) ) {
		pti->pt_ucntl = (u_char)cmd & 0xff;
		ptcwakeup(tp, FREAD);
		return(0);
	}
	if (error < 0) {
		error = ENOTTY;
	}
	stop = (tp->t_flags & RAW) == 0 &&
	    tp->t_stopc == CTRL(s) && tp->t_startc == CTRL(q);
	if (pti->pt_flags & PF_NOSTOP) {
		if (stop) {
			pti->pt_send &= ~TIOCPKT_NOSTOP;
			pti->pt_send |= TIOCPKT_DOSTOP;
			pti->pt_flags &= ~PF_NOSTOP;
			ptcwakeup(tp, FREAD);
		}
	} else {
		if (!stop) {
			pti->pt_send &= ~TIOCPKT_DOSTOP;
			pti->pt_send |= TIOCPKT_NOSTOP;
			pti->pt_flags |= PF_NOSTOP;
			ptcwakeup(tp, FREAD);
		}
	}
	return (error);
}
#endif
