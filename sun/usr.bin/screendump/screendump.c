#ifndef lint
static	char sccsid[] = "@(#)screendump.c 1.3 87/01/08 SMI";
#endif

/*
 * Copyright 1984, 1986 by Sun Microsystems, Inc.
 */

/*
 * Write frame buffer image to a file
 */

#include "screendump.h"

#ifndef	MERGE
#define	screendump_main	main
#endif

screendump_main(argc, argv)
	int argc;
	char *argv[];
{
	int c;

	int copy = 1;
	int type = RT_STANDARD;
	char *display_device = "/dev/fb";
	Pixrect *screen = NULL;

	/*
	 * Process command-line arguments.
	 */
	Progname = basename(argv[0]);

	opterr = 0;

	while ((c = getopt(argc, argv, "cef:t:")) != EOF)
		switch (c) {
		case 'c':
			copy = 0;
			break;
		case 'e':
			type = RT_BYTE_ENCODED;
			break;
		case 'f':
			display_device = optarg;
			break;
		case 't':
			type = atoi(optarg);
			break;
		case '?':
			error((char *) 0, 
			"Usage: %s [-f display] [-e] [-t type] [-c] [file]",
				Progname);
		}

	/*
	 * Open the indicated display device.
	 */
	if (!(screen = pr_open(display_device))) 
		error("%s %s", PR_IO_ERR_DISPLAY, display_device);

	/*
	 * Open the output file if specified.
	 */
	if (optind < argc && 
		freopen(argv[optind], "w", stdout) == NULL) 
		error("%s %s", PR_IO_ERR_OUTFILE, argv[optind]);

	/*
	 * Dump the full screen pixrect to the output file.
	 */
	if (pr_dump(screen, stdout, (colormap_t *) NULL, type, copy)) 
		error(PR_IO_ERR_RASWRITE);

	exit(0);
}
