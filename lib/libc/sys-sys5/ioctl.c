#ifndef lint
static	char sccsid[] = "@(#)ioctl.c 1.4 87/03/16 SMI";
#endif

/*
	ioctl -- system call emulation for 4.2BSD

	Note: this REQUIRES the Sun-modified 4.2BSD tty driver.
*/

#include	<errno.h>
#include	<sys/termio.h>
#include	<sys/ttold.h>

/*	extended Sun mode bits:	*/
#define X_NOPOST	0x00000001	/* no output processing (LITOUT with parity) */
#define X_NOISIG	0x00000002	/* disable all signal characters */
#define X_STOPB		0x00000004	/* two stop bits */

extern int	_ioctl(), _nap();

static void	new_tty();		/* enter new line discipline */
static int	termio_get();		/* get S5 "termio" from 4.2 modes */
static int	termio_set();		/* set 4.2 modes from S5 "termio" */


int
ioctl( fildes, request, arg )		/* returns 0 if ok, else -1 */
	int		fildes; 	/* file descriptor */
	int		request;	/* command */
	int		arg;		/* command arguments */
	{
	struct sgttyb	tb;

	switch ( request )
		{
	case TCGETA:
		new_tty( fildes );
		return termio_get( fildes, (struct termio *)arg );

	case TCSETA:
		new_tty( fildes );
		return termio_set( fildes, (struct termio *)arg, TIOCSETN );

	case TCSETAW:			/* sorry, best we can do */
	case TCSETAF:
		new_tty( fildes );
		return termio_set( fildes, (struct termio *)arg, TIOCSETP );

	case TCSBRK:
		if ( _ioctl( fildes, TIOCGETP, (char *)&tb ) < 0 )
			return -1;	/* errno already set */
		if ( _ioctl( fildes, TIOCSETP, (char *)&tb ) < 0 )
			return -1;	/* errno already set */
		/* output is now drained */

		if ( arg == 0 ) 	/* send break */
			{
			if ( _ioctl( fildes, TIOCSBRK, (char *)0 ) < 0 )
				return -1;	/* errno already set */
			(void)_nap( 250000L );	/* 0.25 second delay */
			return _ioctl( fildes, TIOCCBRK, (char *)0 );
			}
		else
			return 0;

	case TCXONC:
		return _ioctl( fildes, arg == 0 ? TIOCSTOP : TIOCSTART,
			       (char *)0
			     );

	case TCFLSH:
		if ( arg < 0 || arg > 2 )
		{
			errno = EINVAL;
			return -1;
		}
		{
		int	rw = arg + 1;	/* stupid syscall design */

		return _ioctl( fildes, TIOCFLUSH, (char *)&rw );
		}

	default:
		return _ioctl( fildes, request, (char *)arg );
		}
	}


static void
new_tty( fildes )		/* make sure new tty handler is used */
	{
	static int	ldisc = OTTYDISC;	/* line discipline */

	if ( ldisc != NTTYDISC		/* first time this process */
	  && (_ioctl( fildes, TIOCGETD, &ldisc ) != 0	/* unknown */
	   || ldisc != NTTYDISC		/* known but not "new tty" */
	     )
	   )	{
		ldisc = NTTYDISC;	/* force new tty handler */
		(void)_ioctl( fildes, TIOCSETD, &ldisc );
		}
	}


static int
termio_get( fildes, argp )	/* map 4.2BSD modes to termio */
	int			fildes; /* file descriptor */
	register struct termio	*argp;	/* where to put unpacking */
	{
	struct sgttyb		tb;	/* 4.2BSD TIOCGETP buffer */
	int			lm;	/* 4.2BSD TIOCLGET buffer */
	int			xflag;	/* Sun TIOCGETX buffer */
	struct tchars		tc;	/* 4.2BSD TIOCGETC buffer */
	struct ltchars		ltc;	/* 4.2BSD TIOCGLTC buffer */
	register int		flag, lflag;

	if ( _ioctl( fildes, TIOCGETP, (char *)&tb ) < 0
	    || _ioctl( fildes, TIOCLGET, (char *)&lm) < 0
	    || _ioctl( fildes, TIOCGETX, (char *)&xflag ) < 0
	    || _ioctl( fildes, TIOCGETC, (char *)&tc ) < 0 )
		return -1;		/* errno already set */
	argp->c_cc[VERASE] = tb.sg_erase;
	argp->c_cc[VKILL] = tb.sg_kill;
	flag = tb.sg_flags;	/* for speed */
	lflag = lm;
	if ( (argp->c_cc[VINTR] = tc.t_intrc) == (unsigned char)-1 )
		argp->c_cc[VINTR] = CNUL;
	if ( (argp->c_cc[VQUIT] = tc.t_quitc) == (unsigned char)-1 )
		argp->c_cc[VQUIT] = CNUL;
	argp->c_lflag = 0;
	if ( tc.t_startc == (char)-1 && tc.t_stopc == (char)-1 )
		argp->c_iflag = 0;	/* no IXON */
	else
		argp->c_iflag = (lflag & LDECCTQ) != 0 ? IXON
						       : IXON | IXANY;
	argp->c_cc[VMIN] = 1;		/* until we decide we're in ICANON mode */
	argp->c_cc[VTIME] = 0;
	argp->c_cc[VEOL2] = CNUL;
	if ( _ioctl( fildes, TIOCGLTC, (char *)&ltc ) < 0
					/* old tty handler */
	  || (argp->c_cc[VSWTCH] = ltc.t_suspc) == (unsigned char)-1
	   )
		argp->c_cc[VSWTCH] = CNUL;

	argp->c_oflag = (unsigned short)(flag & O_TBDELAY) << 1;

	if ( (argp->c_cflag = (tb.sg_ispeed & CBAUD) | CREAD)
	      == (B110 | CREAD)
	   )
		argp->c_cflag |= CSTOPB;

	if ( (lflag & (LMDMBUF | LNOHANG)) == LNOHANG )
		argp->c_cflag |= CLOCAL;
	if ( (lflag & LTOSTOP) != 0 )
		argp->c_cflag |= LOBLK;
	if ( (xflag & NOPOST) == 0 )
		argp->c_oflag |= OPOST;
	if ( (flag & O_LCASE) != 0 )
		{
		argp->c_iflag |= IUCLC;
		argp->c_oflag |= OLCUC;
		argp->c_lflag |= XCASE;
		}
	if ( (flag & O_ECHO) != 0 )
		argp->c_lflag |= ECHO;
	if ( (lflag & LNOFLSH) != 0 )
		argp->c_lflag |= NOFLSH;
	argp->c_lflag |= ECHOK;
	if ( (flag & O_CRMOD) != 0 )
		{
		argp->c_iflag |= ICRNL;
		argp->c_oflag |= ONLCR;
		if ( (flag & O_NL2) != 0 )	/* O_NL2 or O_NL3 */
			argp->c_oflag |= NL1;
		else if ( (flag & O_NL1) != 0 ) /* O_NL1 */
			argp->c_oflag |= CR1;
		else if ( (flag & O_CR2) != 0 ) /* O_CR2 or O_CR3 */
			argp->c_oflag |= ONOCR | CR3;	/* approx. */
		else if ( (flag & O_CR1) != 0 ) /* O_CR1 */
			argp->c_oflag |= ONOCR | CR2;	/* approx. */
		}
	else	{
		argp->c_oflag |= ONLRET;
		if ( (flag & O_NL1) != 0 )
			argp->c_oflag |= CR1;
		if ( (flag & O_NL2) != 0 )
			argp->c_oflag |= CR2;
		}
	if ( (flag & O_VTDELAY) != 0 )
		argp->c_oflag |= FF1 | VT1;
	if ( (flag & O_BSDELAY) != 0 )
		argp->c_oflag |= BS1;
	if ( (flag & O_RAW) != 0 )
		{
		argp->c_cflag |= CS8;
		argp->c_iflag &= ~(ICRNL | IUCLC);
		argp->c_lflag &= ~XCASE;
		argp->c_oflag &= ~OPOST;
		}
	else	{
		if ( (lflag & LPASS8) != 0 )
			argp->c_cflag |= CS8;
		else
			{
			if ( (lflag & LLITOUT) != 0 )
				argp->c_cflag |= CS8;
			else
				argp->c_cflag |= CS7 | PARENB;
			argp->c_iflag |= ISTRIP;
			}
		if ( (lflag & LLITOUT) != 0 )
			argp->c_oflag &= ~OPOST;
		argp->c_iflag |= BRKINT | IGNPAR | INPCK;
		if ( (xflag & NOISIG) == 0 )
			argp->c_lflag |= ISIG;
		if ( (flag & O_CBREAK) == 0 )
			{
			argp->c_lflag |= ICANON;
			if ( (argp->c_cc[VEOF] = tc.t_eofc) == (unsigned char)-1 )
				argp->c_cc[VEOF] = CNUL;
			if ( (argp->c_cc[VEOL] = tc.t_brkc) == (unsigned char)-1 )
				argp->c_cc[VEOL] = CNUL;
			}
		}
	if ( (flag & O_ODDP) != 0 )
		if ( (flag & O_EVENP) != 0 )
			argp->c_iflag &= ~INPCK;
		else
			argp->c_cflag |= PARODD;
	if ( (lflag & LCRTERA) != 0 )
		argp->c_lflag |= ECHOE;
	if ( (flag & O_TANDEM) != 0 )
		argp->c_iflag |= IXOFF;
	if ( (xflag & STOPB) != 0 )
		argp->c_cflag |= CSTOPB;

	argp->c_line = 0;		/* default line discipline */

	return 0;
	}


static int
termio_set( fildes, argp, request )	/* map termio to 4.2BSD modes */
	int			fildes; /* file descriptor */
	register struct termio	*argp;	/* -> desired state info */
	int			request;/* request to set modes with */
	{
	struct sgttyb		tb;	/* 4.2BSD TIOCSETP buffer */
	int			lm;	/* 4.2BSD TIOCLSET buffer */
	int			xflag;	/* Sun TIOCSETX buffer */
	struct tchars		tc;	/* 4.2BSD TIOCSETC buffer */
	struct ltchars		ltc;	/* 4.2BSD TIOCSLTC buffer */
	register int		flag, lflag;

	if ( (argp->c_lflag & ICANON) != 0 )	/* no MIN, TIME */
		{
		if ( (tc.t_eofc = argp->c_cc[VEOF]) == CNUL )
			tc.t_eofc = (char)-1;
		if ( (tc.t_brkc = argp->c_cc[VEOL]) == CNUL )
			tc.t_brkc = (char)-1;
		}
	else if ( _ioctl( fildes, TIOCGETC, (char *)&tc ) < 0 )
		return -1;		/* errno already set */
	if ( (tc.t_intrc = argp->c_cc[VINTR]) == CNUL )
		tc.t_intrc = (char)-1;
	if ( (tc.t_quitc = argp->c_cc[VQUIT]) == CNUL )
		tc.t_quitc = (char)-1;
	if ( (argp->c_iflag & IXON) == 0 )
		tc.t_startc = tc.t_stopc = (char)-1;	/* disable */
	else	{
		tc.t_startc = CSTART;
		tc.t_stopc = CSTOP;
		}
	if ( _ioctl( fildes, TIOCSETC, (char *)&tc ) < 0 )
		return -1;		/* errno already set */

	if ( _ioctl( fildes, TIOCGLTC, (char *)&ltc ) == 0 )
		{			/* new tty handler */		
		if ( (ltc.t_suspc = argp->c_cc[VSWTCH]) == CNUL )
			ltc.t_suspc = (char)-1;
		if ( _ioctl( fildes, TIOCSLTC, (char *)&ltc ) < 0 )
			return -1;	/* errno already set */
		}

	if ( (argp->c_cflag & HUPCL) != 0
	  && _ioctl( fildes, TIOCHPCL, (char *)0 ) < 0
	   )
		return -1;		/* errno already set */

	tb.sg_erase = argp->c_cc[VERASE];
	tb.sg_kill = argp->c_cc[VKILL];
	tb.sg_ispeed = tb.sg_ospeed = argp->c_cflag & CBAUD;

	flag = 0;
	lflag = LCTLECH;		/* everybody gets this */
	xflag = 0;

	if ( (argp->c_cflag & (CSIZE|PARENB)) == CS8 )
		{
		/*
		 * 8-bit characters.
		 */
		if ( ( (argp->c_oflag & OPOST) == 0 ||
	        	(argp->c_oflag & (OLCUC|ONLCR|OCRNL|ONOCR|ONLRET|NLDLY|CRDLY|TABDLY|BSDLY|VTDLY|FFDLY)) == 0 ) )
			{
			/*
			 * 8-bit characters and no output processing.
			 */
			if ( (argp->c_iflag & (BRKINT|IGNPAR|PARMRK|INPCK|ISTRIP|INLCR|IGNCR|ICRNL|IUCLC|IXON)) == 0
			    && (argp->c_lflag & (ISIG|ICANON|XCASE)) == 0 )
				/*
				 * 8-bit characters, no output processing,
				 * and no input processing - use RAW mode.
				 */
				flag |= O_RAW;
			else
				/*
				 * 8-bit characters, no output processing,
				 * and normal input processing - use LITOUT
				 * mode.
				 */
				lflag |= LLITOUT|LPASS8;
			}
		else
			/*
			 * 8-bit characters, and normal input and output
			 * processing - use PASS8 mode.  (This does not
			 * give an 8-bit data path on output, but there's
			 * not much you can do about that without making
			 * bigger driver changes than we wanted to for 3.2.)
			 */
			lflag |= LPASS8;
		}
	if ( (argp->c_lflag & ISIG) == 0 )
		xflag |= NOISIG;
	if ( (argp->c_lflag & ICANON) == 0 )
		flag |= O_CBREAK;
	if ( (argp->c_iflag & IUCLC) != 0
	    && (argp->c_oflag & OLCUC) != 0
	    && (argp->c_lflag & XCASE) != 0 )
		flag |= O_LCASE;
	if ( (argp->c_lflag & ECHO) != 0 )
		flag |= O_ECHO;
	if ( (argp->c_lflag & ECHOE) != 0 )
		{
		lflag |= LCRTBS|LCRTERA;
		if ( tb.sg_ospeed >= B1200 )
			lflag |= LCRTKIL;
		}
	else
		lflag |= LPRTERA;
	if ( (argp->c_lflag & NOFLSH) != 0 )
		lflag |= LNOFLSH;
	if ( (argp->c_cflag & CSTOPB) != 0 )
		xflag |= STOPB;
	if ( (argp->c_cflag & PARODD) != 0 )
		flag |= O_ODDP;
	else if ( (argp->c_iflag & INPCK) != 0 )
		flag |= O_EVENP;
	else
		flag |= O_ODDP | O_EVENP;
	if ( (argp->c_cflag & CLOCAL) != 0 )
		lflag |= LNOHANG;
	if ( (argp->c_oflag & OPOST) == 0 )
		xflag |= NOPOST;
	if ( (argp->c_cflag & LOBLK) != 0 )
		lflag |= LTOSTOP;
	/*
	 * Don't test ONLCR; it may be left on, with OPOST being turned
	 * off instead.
	 */
	if ( (argp->c_iflag & ICRNL) != 0 )
		{
		flag |= O_CRMOD;
		if ( (argp->c_oflag & CRDLY) == CR1 )
			flag |= O_NL1;	/* sorry `bout that */
		else if ( (argp->c_oflag & CRDLY) == CR2 )
			flag |= O_CR1;	/* approximation */
		else if ( (argp->c_oflag & CRDLY) != 0 )
			flag |= O_CR2;	/* approximation to CR3 */
		}
	else if ( (argp->c_oflag & ONLRET) != 0 )
		{
		if ( (argp->c_oflag & CR2) != 0 )	/* CR2 or CR3 */
			flag |= O_NL2;
		else if ( (argp->c_oflag & CR1) != 0 )	/* CR1 */
			flag |= O_NL1;
		}
	else
		if ( (argp->c_oflag & NLDLY) != 0 )
			flag |= O_NL2;
	flag |= (long)((argp->c_oflag & TABDLY) >> 1);
	if ( (argp->c_oflag & (VTDLY | FFDLY)) != 0 )
		flag |= O_VTDELAY;
	if ( (argp->c_oflag & BSDLY) != 0 )
		flag |= O_BSDELAY;
	if ( (argp->c_iflag & (IXON | IXANY)) == IXON )
		lflag |= LDECCTQ;
	if ( (argp->c_iflag & IXOFF) != 0 )
		flag |= O_TANDEM;

	if ( _ioctl( fildes, TIOCSETX, (char *)&xflag ) < 0 )
		return -1;		/* errno already set */

	tb.sg_flags = flag;
	lm = lflag;

	if ( _ioctl( fildes, TIOCLSET, (char *)&lm ) < 0
	    || _ioctl( fildes, request, (char *)&tb ) < 0
	   )
		return -1;		/* errno already set */

	return 0;
	}
