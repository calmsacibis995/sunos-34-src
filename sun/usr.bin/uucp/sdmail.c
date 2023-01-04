/*	sdmail.c	1.1	86/09/25	*/
	/*  sdmail 3.1  10/26/79  11:18:18  */
#include "uucp.h"
#include <pwd.h>



/*******
 *	sdmail(file, uid)
 *
 *	sdmail  -  this routine will determine the owner
 *	of the file (file), create a message string and
 *	call "mailst" to send the cleanup message.
 *	This is only implemented for local system
 *	mail at this time.
 */

sdmail(file, uid)
char *file;
{
	static struct passwd *pwd;
	struct passwd *getpwuid();
	char mstr[40];

	sprintf(mstr, "uuclean deleted file %s\n", file);
	if (pwd->pw_uid == uid) {
		mailst(pwd->pw_name, mstr);
	return(0);
	}

	setpwent();
	if ((pwd = getpwuid(uid)) != NULL) {
		mailst(pwd->pw_name, mstr);
	}
	return(0);
}


/***
 *	mailst(user, str)
 *	char *user, *str;
 *
 *	mailst  -  this routine will fork and execute
 *	a mail command sending string (str) to user (user).
 */

mailst(user, str)
char *user, *str;
{
	FILE *fp;
	extern FILE *popen(), *pclose();
	char cmd[100];

	sprintf(cmd, "mail %s", user);
	if ((fp = popen(cmd, "w")) == NULL)
		return;
	fprintf(fp, "From: uucp\nTo: %s\n\n%s\n", user, str);
	pclose(fp);
	return;
}
