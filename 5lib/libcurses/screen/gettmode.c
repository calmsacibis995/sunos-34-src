#ifndef lint
static	char sccsid[] = "@(#)gettmode.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

# include	"curses.ext"
# include	<signal.h>

char	*calloc();
char	*malloc();
extern	char	*getenv();

extern	WINDOW	*makenew();

gettmode()
{
	/* No-op included only for upward compatibility. */
}
