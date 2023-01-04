#ifndef lint
static	char sccsid[] = "@(#)getlogin.c 1.1 86/09/24 SMI"; /* from UCB 4.2 82/11/14 */
#endif

#include <utmp.h>

static	char UTMP[]	= "/etc/utmp";
static	struct utmp ubuf;

char *
getlogin()
{
	register int me, uf;
	register char *cp;

	if (!(me = ttyslot()))
		return(0);
	if ((uf = open(UTMP, 0)) < 0)
		return (0);
	lseek (uf, (long)(me*sizeof(ubuf)), 0);
	if (read(uf, (char *)&ubuf, sizeof (ubuf)) != sizeof (ubuf)) {
		close(uf);
		return (0);
	}
	close(uf);
	if (ubuf.ut_name[0] == '\0')
		return (0);
	ubuf.ut_name[sizeof (ubuf.ut_name)] = ' ';
	for (cp = ubuf.ut_name; *cp++ != ' '; )
		;
	*--cp = '\0';
	if (ubuf.ut_name[0] == '\0')
		return (0);
	return (ubuf.ut_name);
}
