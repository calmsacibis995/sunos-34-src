#ifndef lint
static	char sccsid[] = "@(#)nl.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

nl()	
{
#ifdef USG
	(cur_term->Nttyb).c_iflag |= ICRNL;
	(cur_term->Nttyb).c_oflag |= ONLCR;
# ifdef DEBUG
	if(outf) fprintf(outf, "nl(), file %x, SP %x, flags %x,%x\n", SP->term_file, SP, cur_term->Nttyb.c_iflag, cur_term->Nttyb.c_oflag);
# endif
#else
	(cur_term->Nttyb).sg_flags |= CRMOD;
# ifdef DEBUG
	if(outf) fprintf(outf, "nl(), file %x, SP %x, flags %x\n", SP->term_file, SP, cur_term->Nttyb.sg_flags);
# endif
#endif
	reset_prog_mode();
}
