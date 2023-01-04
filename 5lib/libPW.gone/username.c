#ifndef lint
static	char sccsid[] = "@(#)username.c 1.1 86/09/24 SMI"; /* from S5R2 3.1 */
#endif

/*
	Gets user's login name.
	Note that the argument must be an integer.
	Returns pointer to login name on success,
	pointer to string representation of used ID on failure.

	Remembers user ID and login name for subsequent calls.
*/

#include <pwd.h>

char *
username(uid)
register int uid;
{
	register struct passwd *pwd;
	static int ouid;
	static char lnam[9], *lptr;

	if (ouid!=uid || ouid==0) {
		if ((pwd = getpwuid(uid)) == 0)
			sprintf(lnam,"%d",uid);
		else {
			(void) strncpy(lnam, pwd->pw_name, 9);
			lnam[8] = '\0';
		}
		lptr = lnam;
		ouid = uid;
	}
	return(lptr);
}
