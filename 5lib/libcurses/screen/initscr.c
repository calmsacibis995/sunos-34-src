#ifndef lint
static	char sccsid[] = "@(#)initscr.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

# include	"curses.ext"
# include	<signal.h>

char	*calloc();
char	*malloc();
extern	char	*getenv();

extern	WINDOW	*makenew();

/*
 *	This routine initializes the current and standard screen.
 *
 * 3/5/81 (Berkeley) @(#)initscr.c	1.2
 */

WINDOW *
initscr()
{
	register char *sp;
	struct screen *scp;
	extern char *_c_why_not;
	
# ifdef DEBUG
	if (outf == NULL) {
		if( ( outf = fopen("trace", "w") ) == NULL)
		{
			perror("trace");
			exit(-1);
		}
	}
#endif

	if( ( sp = getenv( "TERM" ) ) == NULL )
	{
		sp = Def_term;
	}
# ifdef DEBUG
	if(outf) fprintf(outf, "INITSCR: term = %s\n", sp);
# endif
	if( ( scp = newterm( sp, stdout, stdin ) ) == NULL )
	{
		_ec_quit(_c_why_not, sp);
	}
	return stdscr;
}
