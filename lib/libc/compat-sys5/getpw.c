#ifndef lint
static  char sccsid[] = "@(#)getpw.c 1.1 86/09/24  Copyr 1984 Sun Micro";
#endif

/* 
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include <pwd.h>

struct passwd *getpwuid();

getpw(uid, buf)
	int uid;
	char buf[];
{
	struct passwd *pw;
	char numbuf[20];

	pw = getpwuid(uid);
	if(pw == 0)
		return 1;
	strcpy(buf, pw->pw_name);
	strcat(buf, ":");
	strcat(buf, pw->pw_passwd);
	if (*pw->pw_age != '\0') {
		strcat(buf, ",");
		strcat(buf, pw->pw_age);
	}
	strcat(buf, ":");
	sprintf(numbuf, "%d", pw->pw_uid);
	strcat(buf, numbuf);
	strcat(buf, ":");
	sprintf(numbuf, "%d", pw->pw_gid);
	strcat(buf, numbuf);
	strcat(buf, ":");
	strcat(buf, pw->pw_gecos);
	strcat(buf, ":");
	strcat(buf, pw->pw_dir);
	strcat(buf, ":");
	strcat(buf, pw->pw_shell);
	return 0;
}
