#ifndef lint
static	char sccsid[] = "@(#)ttyhist.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#ifdef TTYHIST

/*
 * tty subwindow history mechanism.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <suntool/ttysw.h>
#include "ttysw_impl.h"
#include "ttyansi.h"
#include "charimage.h"

/*
 * Open the tty history file and turn history on.
 */
ttyhist_open(ttysw0, hfile)
	caddr_t ttysw0;
	char *hfile;
{
	struct ttysubwindow *ttysw = (struct ttysubwindow *)ttysw0;
	char *t;
	char file[100];
	extern char *getenv(), *rindex();

	if (ttysw->ttysw_hist)
		return;
	if (hfile == (char *)NULL &&
	    (hfile = getenv("TTYHIST")) == (char *)NULL)
		hfile = "/usr/tmp/%s.history";
	t = rindex(ttyname(ttysw->ttysw_tty), '/') + 1;
	sprintf(file, hfile, t);
	if ((ttysw->ttysw_hist = fopen(file, "w+")) == (FILE *)NULL) {
		fprintf(stderr, "Can't open tty history file %s\n", file);
		return;
	}
	/* don't let anyone else look at it */
	fchmod(fileno(ttysw->ttysw_hist), 0600);
}

/*
 * Close the tty history file.
 */
ttyhist_close(ttysw0)
	caddr_t ttysw0;
{
	struct ttysubwindow *ttysw = (struct ttysubwindow *)ttysw0;
	register int i;

	if (ttysw->ttysw_hist == (FILE *)NULL)
		return;
	if (ttysw->ttysw_opt&TTYOPT_HISTORY) {
		/*
		 * Flush out the last screenfull.
		 */
		for (i = bottom; i <= top; i--)
			if (length(image[i]))
				break;
		ttyhist_write(ttysw, i - top + 1);
	}
	fclose(ttysw->ttysw_hist);
	ttysw->ttysw_hist = (FILE *)NULL;
}

/*
 * Turn tty history on.
 */
ttyhist_on(ttysw)
	struct ttysubwindow *ttysw;
{

	if (ttysw->ttysw_hist == (FILE *)NULL)
		ttyhist_open(ttysw, (char *)NULL);
}

/*
 * Turn tty history off.
 */
ttyhist_off(ttysw)
	struct ttysubwindow *ttysw;
{

	if (ttysw->ttysw_hist != (FILE *)NULL)
		fflush(ttysw->ttysw_hist);
}

/*
 * Write the first n lines of the screen to the history file.
 */
ttyhist_write(ttysw, n)
	struct ttysubwindow *ttysw;
	int n;
{
	register int i;
	register char *line;

	if (ttysw->ttysw_hist == (FILE *)NULL)
		return;
	for (i = top; i < top + n; i++) {
		line = image[i];
		if (length(line) != 0)
			fwrite(line, 1, length(line), ttysw->ttysw_hist);
		if (length(line) != right)
			putc('\n', ttysw->ttysw_hist);
	}
}

/*
 * Flush any buffering of writes to the history file.
 */
ttyhist_flush(ttysw)
	struct ttysubwindow *ttysw;
{

	if (ttysw->ttysw_hist == (FILE *)NULL)
		return;
	fflush(ttysw->ttysw_hist);
}
#endif
