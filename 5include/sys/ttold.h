/*	@(#)ttold.h 1.1 86/09/24 SMI; from S5R2 6.1	*/

struct tchars {
	char	t_intrc;	/* interrupt */
	char	t_quitc;	/* quit */
	char	t_startc;	/* start output */
	char	t_stopc;	/* stop output */
	char	t_eofc;		/* end-of-file */
	char	t_brkc;		/* input delimiter (like nl) */
};
struct ltchars {
	char	t_suspc;	/* stop process signal */
	char	t_dsuspc;	/* delayed stop process signal */
	char	t_rprntc;	/* reprint line */
	char	t_flushc;	/* flush output (toggles) */
	char	t_werasc;	/* word erase */
	char	t_lnextc;	/* literal next character */
};

/*
 * Structure for TIOCGETP and TIOCSETP ioctls.
 */

#ifndef _SGTTYB_
#define	_SGTTYB_
struct sgttyb {
	char	sg_ispeed;		/* input speed */
	char	sg_ospeed;		/* output speed */
	char	sg_erase;		/* erase character */
	char	sg_kill;		/* kill character */
	short	sg_flags;		/* mode flags */
};
#endif

/*
 * Window/terminal size structure.
 * This information is stored by the kernel
 * in order to provide a consistent interface,
 * but is not used by the kernel.
 *
 * Type must be "unsigned short" so that types.h not required.
 */
struct winsize {
	unsigned short	ws_row;		/* rows, in characters */
	unsigned short	ws_col;		/* columns, in characters */
	unsigned short	ws_xpixel;	/* horizontal size, pixels - not used */
	unsigned short	ws_ypixel;	/* vertical size, pixels - not used */
};

/*
 * Sun version of same.
 */
struct ttysize {
	int	ts_lines;	/* number of lines on terminal */
	int	ts_cols;	/* number of columns on terminal */
};

#include	<sys/ioccom.h>

/*
 * 4.3BSD tty ioctl commands
 */
#define	TIOCGETD	_IOR(t, 0, int)		/* get line discipline */
#define	TIOCSETD	_IOW(t, 1, int)		/* set line discipline */
#define	TIOCHPCL	_IO(t, 2)		/* hang up on last close */
#define	TIOCMODG	_IOR(t, 3, int)		/* get modem control state */
#define	TIOCMODS	_IOW(t, 4, int)		/* set modem control state */
#define		TIOCM_LE	0001		/* line enable */
#define		TIOCM_DTR	0002		/* data terminal ready */
#define		TIOCM_RTS	0004		/* request to send */
#define		TIOCM_ST	0010		/* secondary transmit */
#define		TIOCM_SR	0020		/* secondary receive */
#define		TIOCM_CTS	0040		/* clear to send */
#define		TIOCM_CAR	0100		/* carrier detect */
#define		TIOCM_CD	TIOCM_CAR
#define		TIOCM_RNG	0200		/* ring */
#define		TIOCM_RI	TIOCM_RNG
#define		TIOCM_DSR	0400		/* data set ready */
#define	TIOCGETP	_IOR(t, 8,struct sgttyb)/* get parameters -- gtty */
#define	TIOCSETP	_IOW(t, 9,struct sgttyb)/* set parameters -- stty */
#define	TIOCSETN	_IOW(t,10,struct sgttyb)/* as above, but no flushtty */
#define	TIOCEXCL	_IO(t, 13)		/* set exclusive use of tty */
#define	TIOCNXCL	_IO(t, 14)		/* reset exclusive use of tty */
#define	TIOCFLUSH	_IOW(t, 16, int)	/* flush buffers */
#define	TIOCSETC	_IOW(t,17,struct tchars)/* set special characters */
#define	TIOCGETC	_IOR(t,18,struct tchars)/* get special characters */
#define		O_TANDEM	0x00000001	/* send stopc on out q full */
#define		O_CBREAK	0x00000002	/* half-cooked mode */
#define		O_LCASE		0x00000004	/* simulate lower case */
#define		O_ECHO		0x00000008	/* echo input */
#define		O_CRMOD		0x00000010	/* map \r to \r\n on output */
#define		O_RAW		0x00000020	/* no i/o processing */
#define		O_ODDP		0x00000040	/* get/send odd parity */
#define		O_EVENP		0x00000080	/* get/send even parity */
#define		O_ANYP		0x000000c0	/* get any parity/send none */
#define		O_NLDELAY	0x00000300	/* \n delay */
#define			O_NL0	0x00000000
#define			O_NL1	0x00000100	/* tty 37 */
#define			O_NL2	0x00000200	/* vt05 */
#define			O_NL3	0x00000300
#define		O_TBDELAY	0x00000c00	/* horizontal tab delay */
#define			O_TAB0	0x00000000
#define			O_TAB1	0x00000400	/* tty 37 */
#define			O_TAB2	0x00000800
#define		O_XTABS		0x00000c00	/* expand tabs on output */
#define		O_CRDELAY	0x00003000	/* \r delay */
#define			O_CR0	0x00000000
#define			O_CR1	0x00001000	/* tn 300 */
#define			O_CR2	0x00002000	/* tty 37 */
#define			O_CR3	0x00003000	/* concept 100 */
#define		O_VTDELAY	0x00004000	/* vertical tab delay */
#define			O_FF0	0x00000000
#define			O_FF1	0x00004000	/* tty 37 */
#define		O_BSDELAY	0x00008000	/* \b delay */
#define			O_BS0	0x00000000
#define			O_BS1	0x00008000
#define 	O_ALLDELAY	(O_NLDELAY|O_TBDELAY|O_CRDELAY|O_VTDELAY|O_BSDELAY)
#define		O_CRTBS		0x00010000	/* do backspacing for crt */
#define		O_PRTERA	0x00020000	/* \ ... / erase */
#define		O_CRTERA	0x00040000	/* " \b " to wipe out char */
#define		O_TILDE		0x00080000	/* hazeltine tilde kludge */
#define		O_MDMBUF	0x00100000	/* start/stop output on carrier intr */
#define		O_LITOUT	0x00200000	/* literal output */
#define		O_TOSTOP	0x00400000	/* SIGSTOP on background output */
#define		O_FLUSHO	0x00800000	/* flush output to terminal */
#define		O_NOHANG	0x01000000	/* no SIGHUP on carrier drop */
#define		O_L001000	0x02000000
#define		O_CRTKIL	0x04000000	/* kill line with " \b " */
#define		O_PASS8		0x08000000
#define		O_CTLECH	0x10000000	/* echo control chars as ^X */
#define		O_PENDIN	0x20000000	/* tp->t_rawq needs reread */
#define		O_DECCTQ	0x40000000	/* only ^Q starts after ^S */
#define		O_NOFLSH	0x80000000	/* no output flush on signal */
/* locals, from 127 down */
#define	TIOCLBIS	_IOW(t, 127, int)	/* bis local mode bits */
#define	TIOCLBIC	_IOW(t, 126, int)	/* bic local mode bits */
#define	TIOCLSET	_IOW(t, 125, int)	/* set entire local mode word */
#define	TIOCLGET	_IOR(t, 124, int)	/* get local modes */
#define		LCRTBS		(O_CRTBS>>16)
#define		LPRTERA		(O_PRTERA>>16)
#define		LCRTERA		(O_CRTERA>>16)
#define		LTILDE		(O_TILDE>>16)
#define		LMDMBUF		(O_MDMBUF>>16)
#define		LLITOUT		(O_LITOUT>>16)
#define		LTOSTOP		(O_TOSTOP>>16)
#define		LFLUSHO		(O_FLUSHO>>16)
#define		LNOHANG		(O_NOHANG>>16)
#define		LCRTKIL		(O_CRTKIL>>16)
#define		LPASS8		(O_PASS8>>16)
#define		LCTLECH		(O_CTLECH>>16)
#define		LPENDIN		(O_PENDIN>>16)
#define		LDECCTQ		(O_DECCTQ>>16)
#define		LNOFLSH		(O_NOFLSH>>16)
#define	TIOCSBRK	_IO(t, 123)		/* set break bit */
#define	TIOCCBRK	_IO(t, 122)		/* clear break bit */
#define	TIOCSDTR	_IO(t, 121)		/* set data terminal ready */
#define	TIOCCDTR	_IO(t, 120)		/* clear data terminal ready */
#define	TIOCGPGRP	_IOR(t, 119, int)	/* get pgrp of tty */
#define	TIOCSPGRP	_IOW(t, 118, int)	/* set pgrp of tty */
#define	TIOCSLTC	_IOW(t,117,struct ltchars)/* set local special chars */
#define	TIOCGLTC	_IOR(t,116,struct ltchars)/* get local special chars */
#define	TIOCOUTQ	_IOR(t, 115, int)	/* output queue size */
#define	TIOCSTI		_IOW(t, 114, char)	/* simulate terminal input */
#define	TIOCNOTTY	_IO(t, 113)		/* void tty association */
#define	TIOCPKT		_IOW(t, 112, int)	/* pty: set/clear packet mode */
#define		TIOCPKT_DATA		0x00	/* data packet */
#define		TIOCPKT_FLUSHREAD	0x01	/* flush packet */
#define		TIOCPKT_FLUSHWRITE	0x02	/* flush packet */
#define		TIOCPKT_STOP		0x04	/* stop output */
#define		TIOCPKT_START		0x08	/* start output */
#define		TIOCPKT_NOSTOP		0x10	/* no more ^S, ^Q */
#define		TIOCPKT_DOSTOP		0x20	/* now do ^S ^Q */
#define	TIOCSTOP	_IO(t, 111)		/* stop output, like ^S */
#define	TIOCSTART	_IO(t, 110)		/* start output, like ^Q */
#define	TIOCMSET	_IOW(t, 109, int)	/* set all modem bits */
#define	TIOCMBIS	_IOW(t, 108, int)	/* bis modem bits */
#define	TIOCMBIC	_IOW(t, 107, int)	/* bic modem bits */
#define	TIOCMGET	_IOR(t, 106, int)	/* get all modem bits */
#define	TIOCREMOTE	_IOW(t, 105, int)	/* remote input editing */
#define	TIOCGWINSZ	_IOR(t, 104, struct winsize)	/* get window size */
#define	TIOCSWINSZ	_IOW(t, 103, struct winsize)	/* set window size */
#define	TIOCUCNTL	_IOW(t, 102, int)	/* pty: set/clr usr cntl mode */

/*
 * Sun-specific ioctls, which will be moved to the Sun-specific range.
 * The old codes will be kept around for binary compatibility; the
 * codes for TIOCCONS and TIOCGSIZE don't collide with the 4.3BSD codes
 * because the structure size and copy direction fields are different.
 * The new codes are here to ease the transition internally, not for
 * customer use.
 *
 * Unfortunately, the old TIOCSSIZE code does collide with TIOCSWINSZ,
 * but they can be disambiguated by checking whether a "struct ttysize"
 * structure's "ts_lines" field is greater than 64K or not.  If so,
 * it's almost certainly a "struct winsize" instead.
 */
#define	_O_TIOCCONS	_IO(t, 104)		/* get console I/O */
#define	_O_TIOCSSIZE	_IOW(t,103,struct ttysize)/* get tty size */
#define	_O_TIOCGSIZE	_IOR(t,102,struct ttysize)/* get tty size */
#define	_N_TIOCCONS	_IO(t, 36)		/* get console I/O */
#define	_N_TIOCSSIZE	_IOW(t,37,struct ttysize)/* set tty size */
#define	_N_TIOCGSIZE	_IOR(t,38,struct ttysize)/* get tty size */

/*
 * Sun-specific ioctls.
 */
#define TIOCTCNTL	_IOW(t, 32, int)	/* pty: set/clr intercept ioctl mode */
#define TIOCSIGNAL	_IOW(t, 33, int)	/* pty: send signal to slave */
#define	TIOCSETX	_IOW(t, 34, int)	/* set extra modes for S5 compatibility */
#define	TIOCGETX	_IOR(t, 35, int)	/* get extra modes for S5 compatibility */
#define		NOPOST		0x00000001	/* no processing on output (LITOUT with 7 bits + parity) */
#define		NOISIG		0x00000002	/* disable all signal-generating characters */
#define		STOPB		0x00000004	/* two stop bits */
#define	TIOCCONS	_IO(t, 104)		/* get console I/O */
#define	TIOCSSIZE	_IOW(t,103,struct ttysize)/* set tty size */
#define	TIOCGSIZE	_IOR(t,102,struct ttysize)/* get tty size */

#define	OTTYDISC	0		/* old, v7 std tty driver */
#define	NETLDISC	1		/* line discip for berk net */
#define	NTTYDISC	2		/* new tty discipline */
#define	TABLDISC	3		/* hitachi tablet discipline */
#define	NTABLDISC	4		/* gtco tablet discipline */
#define	MOUSELDISC	5		/* mouse discipline */
#define	KBDLDISC	6		/* up/down keyboard trans (console) */
