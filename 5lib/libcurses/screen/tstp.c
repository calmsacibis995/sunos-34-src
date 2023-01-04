#ifndef lint
static	char sccsid[] = "@(#)tstp.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

# include	<signal.h>

# ifdef SIGTSTP

# include	"curses.ext"

/*
 * handle stop and start signals
 *
 * 3/5/81 (Berkeley) @(#)_tstp.c	1.1
 */
_tstp() {

# ifdef DEBUG
	if (outf) fflush(outf);
# endif
	_ll_move(lines-1, 0);
	endwin();
	fflush(stdout);
# ifdef SIGIO		/* supports 4.2BSD signal mechanism */
	/* reset signal handler so kill below stops us */
	signal(SIGTSTP, SIG_DFL);
# ifndef sigmask
#define	sigmask(s)	(1 << ((s)-1))
# endif
	(void) sigsetmask(sigblock(0) &~ sigmask(SIGTSTP));
# endif
	kill(0, SIGTSTP);
# ifdef SIGIO
	sigblock(sigmask(SIGTSTP));
# endif
	signal(SIGTSTP, _tstp);
	fixterm();
	SP->doclear = 1;
	wrefresh(curscr);
}
# endif
