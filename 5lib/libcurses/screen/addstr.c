#ifndef lint
static	char sccsid[] = "@(#)addstr.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

# include	"curses.ext"

/*
 *	This routine adds a string starting at (_cury,_curx)
 *
 * 1/26/81 (Berkeley) @(#)addstr.c	1.1
 */
waddstr(win,str)
register WINDOW	*win; 
register char	*str;
{
# ifdef DEBUG
	if(outf)
	{
		if( win == stdscr )
		{
			fprintf(outf, "WADDSTR(stdscr, ");
		}
		else
		{
			fprintf(outf, "WADDSTR(%o, ", win);
		}
		fprintf(outf, "\"%s\")\n", str);
	}
# endif	DEBUG
	while( *str )
	{
		if( waddch( win, ( chtype ) *str++ ) == ERR )
		{
			return ERR;
		}
	}
	return OK;
}
