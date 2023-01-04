#ifndef lint
static	char sccsid[] = "@(#)groups.c 1.1 86/09/25 SMI"; /* from UCB 4.8 83/09/25 */
#endif

/*
 * groups
 */

#include <sys/param.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>

int	groups[NGROUPS];

main(argc, argv)
	int argc;
	char *argv[];
{
	int ngroups, i;
	char *sep = "";
	struct group *gr;

	if (argc > 1)
		showgroups(argv[1]);
	ngroups = getgroups(NGROUPS, groups);
	for (i = 0; i < ngroups; i++) {
		gr = getgrgid(groups[i]);
		if (gr == NULL)
			printf("%s%d", sep, groups[i]);
		else
			printf("%s%s", sep, gr->gr_name);
		sep = " ";
	}
	printf("\n");
	exit(0);
}

showgroups(user)
	register char *user;
{
	register struct group *gr;
	register struct passwd *pw;
	register char **cp;
	char *sep = "";

	if ((pw = getpwnam(user)) == NULL) {
		fprintf(stderr, "No such user\n");
		exit(1);
	}
	while (gr = getgrent()) {
		if (pw->pw_gid == gr->gr_gid) {
			printf("%s%s", sep, gr->gr_name);
			sep = " ";
			continue;
		}	
		for (cp = gr->gr_mem; cp && *cp; cp++)
			if (strcmp(*cp, user) == 0) {
				printf("%s%s", sep, gr->gr_name);
				sep = " ";
				break;
			}
	}
	printf("\n");
	exit(0);
}
