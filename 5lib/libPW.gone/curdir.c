#ifndef lint
static	char sccsid[] = "@(#)curdir.c 1.1 86/09/24 SMI"; /* from S5R2 3.4 */
#endif

/*
	current directory.
	Places the full pathname of the current directory in `str'.
	Handles file systems not mounted on a root directory
	via /etc/mtab (see mtab(V)).
	NOTE: PWB systems don't use mtab(V), but they don't mount
	file systems anywhere but on a root directory (so far, at least).

	returns 0 on success
	< 0 on failure.

	Current directory on return:
		success: same as on entry
		failure: UNKNOWN!
*/
curdir(str)
char *str;
{
	extern char *getwd();

	if (getwd(str) == 0)
		return (-1);
	return (0);
}
