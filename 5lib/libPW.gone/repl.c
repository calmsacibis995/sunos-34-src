#ifndef lint
static	char sccsid[] = "@(#)repl.c 1.1 86/09/24 SMI"; /* from S5R2 3.2 */
#endif

/*
	Replace each occurrence of `old' with `new' in `str'.
	Return `str'.
*/

char *repl(str,old,new)
char *str;
char old,new;
{
	extern char *trnslat();

	return(trnslat(str, &old, &new, str));
}
