#ifndef lint
static	char sccsid[] = "@(#)setterm.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

# include	"curses.ext"
# include	<signal.h>

char	*calloc();
char	*malloc();
extern	char	*getenv();

extern	WINDOW	*makenew();

/*
 * Low level interface, for compatibility with old curses.
 */
setterm(type)
char *type;
{
	setupterm(type, 1, 0);
}
