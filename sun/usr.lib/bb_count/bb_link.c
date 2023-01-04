#ifndef lint
static	char sccsid[] = "@(#)bb_link.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/file.h>
#include "strings.h"

struct bblink {
	char	*filename;		/* source file name */
	unsigned int	*counters;	/* array of basic block counters */
	int	ncntrs;			/* num of elements in counters[] */
	struct	bblink	*next;		/* linked list */
};
static	struct	bblink	*linkhdr;

int	___tcov_init;

/*
 * ___bb_link - link info for another file
 * Zero the counters and then build a structure and
 * insert in the linked list.
 */
___bb_link(file, cntrs, n)
char	*file;
unsigned int	*cntrs;
int	n;
{
	int	i;
	unsigned int	*bb;
	struct	bblink	*linkp;

	bb = cntrs;
	for (i = 0; i < n; i++) {
		*bb++ = 0;
	}
	linkp = (struct bblink *) malloc(sizeof(struct bblink));
	linkp->filename = strdup(file);
	linkp->counters = cntrs;
	linkp->ncntrs = n;
	linkp->next = linkhdr;
	linkhdr = linkp;
}

/*
 * ___tcov_exit - this routine is called from the standard exit routine
 * via the on_exit() routine.
 * It dumps the values of the counters to the appropriate
 * .d files.
 */
___tcov_exit(retcode)
int	retcode;
{
	FILE	*dotd_fp;
	FILE	*tmp_fp;
	unsigned int	*bb;
	int	line;
	int	count;
	int	c;
	int	n;
	int	fd;
	int	tries;
	struct	bblink 	*linkp;
	static	int	beenhere;
	char	*tmp = "/tmp/count.XXXXXX";
	char	*lock = "/tmp/tcov.lock";
	extern	int	errno;

	if (beenhere) {
		unlink(lock);
		_exit(retcode);
	}
	for (tries = 0; tries < 5; tries++) {
		fd = open(lock, O_CREAT | O_EXCL, 666);
		if (fd == -1) {
			sleep(1);
		} else {
			break;
		}
	}
	if (fd != -1) {
		close(fd);
	} else {
		fprintf(stderr, "Tcov lock file is busy - could not write data\n");
		_exit(retcode);
	}
	beenhere = 1;
	mktemp(tmp);
	for (linkp = linkhdr; linkp != NULL; linkp = linkp->next) {
		dotd_fp = fopen(linkp->filename, "r");
		if (dotd_fp == NULL) {
			fprintf(stderr, "count: Could not open %s, errno %d\n", 
				linkp->filename, errno);
			continue;
		}
		tmp_fp =  fopen(tmp, "w");
		if (tmp_fp == NULL) {
			fclose(dotd_fp);
			fprintf(stderr, "count: Could not create tmp file\n");
			continue;
		}
		bb = linkp->counters;
		n = linkp->ncntrs;
		while (n > 0 && fscanf(dotd_fp, "%d %d", &line, &count) == 2) {
			count += *bb;
			bb++;
			fprintf(tmp_fp, "%d %d\n", line, count);
			n--;
		}
		if (n != 0) {
			fprintf(stderr, "%s: Corrupt file: %d blocks left\n", 
				linkp->filename, n);
		}
		fclose(dotd_fp);
		fclose(tmp_fp);
		dotd_fp = fopen(linkp->filename, "w");
		if (dotd_fp == NULL) {
			unlink(tmp);
			continue;
		}
		tmp_fp = fopen(tmp, "r");
		if (tmp_fp == NULL) {
			unlink(tmp);
			fclose(dotd_fp);
			continue;
		}
		while ((c = getc(tmp_fp)) != EOF) {
			putc(c, dotd_fp);
		}
		fclose(dotd_fp);
		fclose(tmp_fp);
		unlink(tmp);
	}
	unlink(lock);
}

/*
 * The first time a routine is called that has been tcov'ed this
 * routine is called.  It will catch all the signals that if the 
 * process is killed by a signal we can still dump the data to the
 * .d files.
 */
___tcov_init_func()
{
	int	i;
	int	(*sig_hndlr)();
	extern	int	exit();

	for (i = 1; i <= NSIG; i++) {
		sig_hndlr = signal(i, exit);
		if (sig_hndlr != SIG_DFL) {
			signal(i, sig_hndlr);
		}
	}
	on_exit(___tcov_exit, 0);
	___tcov_init = 1;
}
