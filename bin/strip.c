#ifndef lint
static	char sccsid[] = "@(#)strip.c 1.1 86/09/24 SMI"; /* from UCB 4.5 83/07/06 */
#endif
/*
 * strip
 */
#include <a.out.h>
#include <signal.h>
#include <stdio.h>
#include <sys/file.h>

struct	exec head;
int	status;
int	pagesize;

main(argc, argv)
	char *argv[];
{
	register i;

	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	for (i = 1; i < argc; i++) {
		strip(argv[i]);
		if (status > 1)
			break;
	}
	exit(status);
}

strip(name)
	char *name;
{
	register f;
	long size;

	f = open(name, O_RDWR);
	if (f < 0) {
		fprintf(stderr, "strip: "); perror(name);
		status = 1;
		goto out;
	}
	if (read(f, (char *)&head, sizeof (head)) < 0 || N_BADMAG(head)) {
		printf("strip: %s not in a.out format\n", name);
		status = 1;
		goto out;
	}
	if ((head.a_syms == 0) && (head.a_trsize == 0) && (head.a_drsize ==0)) {
		printf("strip: %s already stripped\n", name);
		goto out;
	}
	head.a_syms = head.a_trsize = head.a_drsize = 0;
	size = N_SYMOFF(head);
	if (ftruncate(f, size) < 0) {
		fprintf(stderr, "strip: "); perror(name);
		status = 1;
		goto out;
	}
	(void) lseek(f, (long)0, L_SET);
	(void) write(f, (char *)&head, sizeof (head));
out:
	close(f);
}
