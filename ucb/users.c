#ifndef lint
static	char sccsid[] = "@(#)users.c 1.1 86/09/25 SMI"; /* from UCB 4.2 04/28/83 */
#endif
/*
 * users
 */
char	*malloc();

#include <stdio.h>
#include <utmp.h>

#define NMAX sizeof(utmp.ut_name)
#define LMAX sizeof(utmp.ut_line)

struct utmp utmp;

main(argc, argv)
char **argv;
{
	register char *tp, *s;
	register FILE *fi;

	s = "/etc/utmp";
	if(argc == 2)
		s = argv[1];
	if ((fi = fopen(s, "r")) == NULL) {
		perror(s);
		exit(1);
	}
	while (fread((char *)&utmp, sizeof(utmp), 1, fi) == 1) {
		if(utmp.ut_name[0] == '\0')
			continue;
		if (nonuser(utmp))
			continue;
		putline();
	}
	summary();
	exit(0);
}

char	*names[128];
char	**namp = names;
putline()
{
	char temp[NMAX+1];
	strncpy(temp, utmp.ut_name, NMAX);
	temp[NMAX] = 0;
	*namp = malloc(strlen(temp) + 1);
	strcpy(*namp++, temp);
}

scmp(p, q)
char **p, **q;
{
	return(strcmp(*p, *q));
}
summary()
{
	register char **p;

	qsort(names, namp - names, sizeof names[0], scmp);
	for (p=names; p < namp; p++) {
		if (p != names)
			printf(" ");
		printf("%s", *p);
	}
	printf("\n");
}
