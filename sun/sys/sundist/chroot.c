#ifndef lint
static	char sccsid[] = "@(#)chroot.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */


#include <stdio.h>

extern	int	errno;
main(argc, argv)
int	argc;
char	*argv[];
{
	char	*dir;
	char	*cmd;
	char	newcmd[256];
	char	*newargv[4];
	int	pid;
	int	status;

	argc--;
	argv++;
	if (argc != 2) {
		fprintf(stderr, "Usage: chroot dir cmd\n");
		exit(1);
	}
	dir = argv[0];
	cmd = argv[1];
	if (chroot(dir) != 0) {
		fprintf(stderr, "chroot to %s failed, errno %d\n",
		   dir, errno);
		exit(1);
	}
	newargv[0] = "/bin/sh";
	newargv[1] = "-c";
	newargv[2] = cmd;
	newargv[3] = NULL;
	pid = fork();
	if (pid == 0) { /* child */
		execvp(newargv[0], &newargv[0]);
		fprintf(stderr, "exec failed, errno %d\n", errno);
	} else if (pid == -1) {
		fprintf(stderr, "fork failed, errno %d\n", errno);
	}
	wait(&status);
}
