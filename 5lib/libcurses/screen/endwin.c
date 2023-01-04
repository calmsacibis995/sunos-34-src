#ifndef lint
static	char sccsid[] = "@(#)endwin.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

#include	"curses.ext"

/*
 * Clean things up before exiting.
 * endwin is TRUE if we have called endwin - this avoids calling it twice.
 */

extern	int	_endwin;

extern	int	_c_clean();
extern	int	_fixdelay();
extern	int	_outch();
extern	int	_pos();
extern	int	doupdate();
extern	int	reset_shell_mode();
extern	int	tputs();

int
endwin()
{
	int saveci = SP->check_input;

	if (_endwin)
		return;

	/* Flush out any output not output due to typeahead */
	SP->check_input = 9999;
	doupdate();
	SP->check_input = saveci;	/* in case of another initscr */

	_fixdelay(TRUE, FALSE);
	if (stdscr->_use_meta)
		tputs(meta_off, 1, _outch);
	_pos(lines-1, 0);
	_c_clean();
	_endwin = TRUE;
	reset_shell_mode();
	fflush(stdout);
}
