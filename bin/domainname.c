#ifndef lint
static	char sccsid[] = "@(#)domainname.c 1.1 86/09/24 Copyr 1984 Sun Micro";
#endif
/*
 * domainname -- get (or set domainname)
 */
#include <stdio.h>

char domainname[32];
extern int errno;

main(argc,argv)
	char *argv[];
{
	int	myerrno;

	argc--;
	argv++;
	if (argc) {
		if (setdomainname(*argv,strlen(*argv)))
			perror("setdomainname");
		myerrno = errno;
	} else {
		getdomainname(domainname,sizeof(domainname));
		myerrno = errno;
		printf("%s\n",domainname);
	}
	exit(myerrno);
}
