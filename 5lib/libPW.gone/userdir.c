#ifndef lint
static	char sccsid[] = "@(#)userdir.c 1.1 86/09/24 SMI"; /* from S5R2 3. */
#endif

/*
	Gets user's login directory.
	The argument must be an integer.
	Note the assumption about position of directory field in
	password file (no group id in password file).
	Returns pointer to login directory on success,
	0 on failure.
        Remembers user ID and login directory for subsequent calls.
*/

#include <pwd.h>

char *
userdir(uid)
register int uid;
{
	register struct passwd *pwd;
	static int ouid;
	static char ldir[33];

	if (ouid!=uid || ouid==0) {
		if ((pwd = getpwuid(uid)) == 0)
			return((char *)0);
		(void) strncpy(ldir, pwd->pw_dir, 33);
		ldir[32] = '\0';
	}
	return(ldir);
}
