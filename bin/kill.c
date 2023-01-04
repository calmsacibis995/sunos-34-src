#ifndef lint
static	char sccsid[] = "@(#)kill.c 1.1 86/09/24 SMI"; /* from UCB 4.3 85/06/07 */
#endif
/*
 * kill - send signal to process
 */

#include <stdio.h>
#include <signal.h>
#include <ctype.h>

char *signm[] = { 0,
"HUP", "INT", "QUIT", "ILL", "TRAP", "IOT", "EMT", "FPE",	/* 1-8 */
"KILL", "BUS", "SEGV", "SYS", "PIPE", "ALRM", "TERM", "URG",	/* 9-16 */
"STOP", "TSTP", "CONT", "CHLD", "TTIN", "TTOU", "IO", "XCPU",	/* 17-24 */
"XFSZ", "VTALRM", "PROF", "WINCH", "LOST", "USR1", "USR2", 0,	/* 25-32 */
};

void usage();

main(argc, argv)
char **argv;
{
	register signo, pid, res;
	int errlev, neg, zero;
	extern char *sys_errlist[];
	extern errno;

	if (argc <= 1) {
		usage();
		/*NOTREACHED*/
	}
	if (*argv[1] == '-') {
		if (argv[1][1] == 'l') {
			for (signo = 1; signo <= NSIG; signo++) {
				if (signm[signo])
					printf("%s ", signm[signo]);
				if (signo == 16)
					printf("\n");
			}
			printf("\n");
			exit(0);
		} else if (isdigit(argv[1][1])) {
			signo = atoi(argv[1]+1);
			if (signo < 0 || signo > NSIG) {
				printf("kill: %s: number out of range\n",
				    argv[1]);
				exit(1);
			}
		} else {
			char *name = argv[1]+1;
			for (signo = 1; signo <= NSIG; signo++)
				if (signm[signo] && !strcmp(signm[signo], name))
					goto foundsig;
			printf("kill: %s: unknown signal; kill -l lists signals\n", name);
			exit(1);
foundsig:
			;
		}
		argc--;
		argv++;
	} else
		signo = SIGTERM;
	argv++;
	while (argc > 1) {
		neg = zero = 0;
		if (**argv == '-') neg++;
		else if (**argv == '0') zero++;
		else if (**argv<'0' || **argv>'9') {
			usage();
			/*NOTREACHED*/
		}
		pid = atoi(*argv);
		if (	((pid == 0) && !zero)
		     || ((pid < 0) && !neg)
#ifndef RT
		     || (pid > 32000)
		     || (pid < -32000)
#endif
		      ) {
			usage();
			/*NOTREACHED*/
		}
		res = kill(pid, signo);
		if (res<0) {
			if (pid > 0)
				fprintf(stderr, "kill: process %d: %s\n", pid,
				    sys_errlist[errno]);
			else
				fprintf(stderr, "kill: process group %d: %s\n",
				    pid, sys_errlist[errno]);
			errlev = 1;
		}
		argc--;
		argv++;
	}
	return(errlev);
}

void
usage()
{
	fprintf(stderr, "usage: kill [ -sig ] pid ...\n");
	fprintf(stderr, "for a list of signals: kill -l\n");
	exit(2);
}
