#ifndef lint
static	char sccsid[] = "@(#)cmdline.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Ttysw initialization, destruction and error procedures
 */

/* Overwrite existing argv with rest of arg list past this point */
/* Assumes NULL terminates argv */
cmdline_scrunch(argcptr, argv)
	int *argcptr;
	register char **argv;
{
	while (*argv) {
		*argv = *(argv+1);
		argv++;
	}
	/*(*argcptr)--;*/
	*argcptr = *argcptr-1;
}

