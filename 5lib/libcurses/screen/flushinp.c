#ifndef lint
static	char sccsid[] = "@(#)flushinp.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

flushinp()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "flushinp(), file %x, SP %x\n", SP->term_file, SP);
#endif
#ifdef USG
	ioctl(cur_term -> Filedes, TCFLSH, 0);
#else
# ifdef BSD4_2
	{
	int readonly = 1;	/* flush input queue only */

	ioctl(cur_term -> Filedes, TIOCFLUSH, &readonly);
	}
# else
	/* for insurance against someone using their own buffer: */
	ioctl(cur_term -> Filedes, TIOCGETP, &(cur_term->Nttyb));

	/*
	 * SETP waits on output and flushes input as side effect.
	 * Really want an ioctl like TCFLSH but Berkeley doesn't have one.
	 */
	ioctl(cur_term -> Filedes, TIOCSETP, &(cur_term->Nttyb));
# endif
#endif
	/*
	 * Have to doupdate() because, if we've stopped output due to
	 * typeahead, now that typeahead is gone, so we'd better catch up.
	 */
	doupdate();
}
