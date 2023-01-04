/*	uucpname.c	1.1	86/09/25	*/
	/*  uucpname  3.1  10/26/79  11:44:15  */

#include "uucp.h"
#include "uucpname.h"


#ifdef UUNAME
#include <stdio.h>
#endif

#ifdef UNAME
#include <sys/utsname.h>
#endif

#ifdef WHOAMI
#include <whoami.h>
#endif

/*******
 *	uucpname(name)		get the uucp name
 *
 *	return code - none
 */

uucpname(name)
char *name;
{
	char *s, *d;

#ifdef GETHOST
	char stmp[8];

	for (s = stmp; s < &stmp[sizeof stmp];)
		*s++ = 0;
	gethostname(stmp, sizeof stmp - 1);
	s = stmp;
#else

#ifdef UUNAME		/* This gets home site name from file  */
	FILE *uucpf;
	char stmp[10];

	s = stmp;
	if (((uucpf = fopen(SYSTEMNAME, "r")) == NULL &&
	     (uucpf = fopen("/etc/uucpname", "r")) == NULL) ||
		cfgets(s, 8, uucpf) == NULL) {
			s = "unknown";
	} else {
		for (d = stmp; *d && *d != '\n' && d < stmp + 8; d++)
			;
		*d = '\0';
	}
	if (uucpf != NULL)
		fclose(uucpf);
#else

#ifdef UNAME
	struct utsname utsn;

	uname(&utsn);
	s = utsn.nodename;
#else

	s = sysname;
#endif UNAME

#endif UUNAME

#endif GETHOST

	d = name;
	while ((*d = *s++) && d < name + 7)
		d++;
	*(name + 7) = '\0';
	return;
}
