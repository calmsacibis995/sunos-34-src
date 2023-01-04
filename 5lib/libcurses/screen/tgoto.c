#ifndef lint
static	char sccsid[] = "@(#)tgoto.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

/*
 * tgoto: function included only for upward compatibility with old termcap
 * library.  Assumes exactly two parameters in the wrong order.
 */
char *
tgoto(cap, col, row)
char *cap;
int col, row;
{
	char *cp;
	char *tparm();

	cp = tparm(cap, row, col);
	return cp;
}
