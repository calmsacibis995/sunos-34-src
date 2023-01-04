#ifndef lint
static	char sccsid[] = "@(#)toolmerge.c 1.1 86/09/25 SMI";
#endif

/*
 * Sun Microsystems, Inc.
 */

/*
 * 	Overview:	Tool Merger: The size of the libraries used by tools
 *			is large.  Substantial memory size saving can be
 *			realized by merging together a group of tools.
 *			The text for every instance of each of the merged tools
 *			is shared.  Each merged tool should be a symbolic
 *			link to the shared object file.
 */

main(argc, argv)
	int argc;
	char **argv;
{
	extern	char *rindex();
	char	*filename = rindex(argv[0], '/');

	if (!filename)
		filename = argv[0];
	else
		filename++;
	/* Order by static global data usage */
	if (strcmp(filename, "selection_svc") == 0) {
		selection_svc_main(argc, argv);
		exit(0);
	} else if (strcmp(filename, "suntools") == 0) {
		suntools_main(argc, argv);
		exit(0);
	} else if (strcmp(filename, "setup.window") == 0) {
		setup_main(argc, argv);
		exit(0);
	} else {
		printf("Couldn't run %s in toolmerge.main\n", argv[0]);
	}
}
