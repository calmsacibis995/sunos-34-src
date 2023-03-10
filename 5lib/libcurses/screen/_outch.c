#ifndef lint
static	char sccsid[] = "@(#)_outch.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

/*
 * This routine is one of the main things
 * in this level of curses that depends on the outside
 * environment.
 */
#include "curses.ext"

int outchcount;

/*
 * Write out one character to the tty.
 */
_outch (c)
chtype c;
{
#ifdef DEBUG
# ifndef LONGDEBUG
	if (outf)
		if (c < ' ')
			fprintf(outf, "^%c", (c+'@')&0177);
		else
			fprintf(outf, "%c", c&0177);
# else LONGDEBUG
	if(outf) fprintf(outf, "_outch: char '%s' term %x file %x=%d\n",
		unctrl(c&0177), SP, SP->term_file, fileno(SP->term_file));
# endif LONGDEBUG
#endif DEBUG

	outchcount++;
	if (SP && SP->term_file)
		putc (c&0177, SP->term_file);
	else
		putc (c&0177, stdout);
}
