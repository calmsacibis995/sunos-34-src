#ifndef lint
static	char sccsid[] = "@(#)cbreak.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

cbreak()
{
#ifdef USG
	(cur_term->Nttyb).c_lflag &= ~ICANON;
	(cur_term->Nttyb).c_cc[VMIN] = 1;
	(cur_term->Nttyb).c_cc[VTIME] = 1;
# ifdef DEBUG
	if(outf) fprintf(outf, "crmode(), file %x, SP %x, flags %x\n", SP->term_file, SP, cur_term->Nttyb.c_lflag);
# endif
#else
	(cur_term->Nttyb).sg_flags |= CBREAK;
# ifdef DEBUG
	if(outf) fprintf(outf, "crmode(), file %x, SP %x, flags %x\n", SP->term_file, SP, cur_term->Nttyb.sg_flags);
# endif
#endif
	SP->fl_rawmode = TRUE;
	reset_prog_mode();
}
