#ifndef lint
static  char sccsid[] = "@(#)demo_merge.c 1.3 87/01/07";
#endif

/****************************************************************************/
/*                           demo_merge.c                                   */
/*  Merges demo programs into one program to save disk space.               */
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
	if (!strcmp(filename, "canvas_demo")) {
		exit(canvas_demo_main(argc, argv));
		
	} else if (!strcmp(filename, "cursor_demo")) {
		exit(cursor_demo_main(argc, argv));
		
	} else if (!strcmp(filename, "framedemo")) { 
		exit(framedemo_main(argc, argv));
		
	} else if (!strcmp(filename, "spheresdemo")) {
		exit(spheresdemo_main(argc, argv));

	} else if (!strcmp(filename, "jumpdemo")) {
		exit(jumpdemo_main(argc, argv));

	} else if (!strcmp(filename, "bouncedemo")) {
		exit(bouncedemo_main(argc, argv));

	} else {
		printf("Couldn't run %s in demo_merge.main\n", argv[0]);
	}
	
	exit(0);
}
