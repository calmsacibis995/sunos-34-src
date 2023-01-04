#ifndef lint
static  char sccsid[] = "@(#)defaults_merge.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif

/****************************************************************************/
/*                           defaults_merge.c                               */
/*  Merges defaults programs into one program to save disk space.           */
/****************************************************************************/

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
	if (!strcmp(filename, "defaultsedit")) {
		defaultsedit_main(argc, argv);
		exit(0);
	} else if (!strcmp(filename, "indentdefaults")) {
		indentdefaults_main(argc, argv);
		exit(0);
	} else if (!strcmp(filename, "scrolldefaults")) {
		scrolldefaults_main(argc, argv);
		exit(0);
	} else if (!strcmp(filename, "defaults_to_mailrc")) {
		d2m_main(argc, argv);
		exit(0);
	} else if (!strcmp(filename, "mailrc_to_defaults")) { 
		m2d_main(argc, argv);
		exit(0);
	} else if (!strcmp(filename, "defaults_to_indentpro")) {
		defaults_to_indentpro_main(argc, argv);
		exit(0);
	} else if (!strcmp(filename, "indentpro_to_defaults")) { 
		indentpro_to_defaults_main(argc, argv);
		exit(0);
	} else if (!strcmp(filename, "stty_from_defaults")) { 
		stty_from_defaults_main(argc, argv);
		exit(0);
	} else if (!strcmp(filename, "defaults_from_input")) { 
		defaults_from_input_main(argc, argv);
		exit(0);
	} else if (!strcmp(filename, "input_from_defaults")) { 
		input_from_defaults_main(argc, argv);
		exit(0);
	} else {
		printf("Couldn't run %s in defaults_merge.main\n", argv[0]);
	}
	
	exit(0);
}

