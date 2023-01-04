#ifndef lint
static  char sccsid[] = "@(#)misc_merge.c 1.4 87/01/07";
#endif

/****************************************************************************/
/*                           misc_merge.c                                   */
/*  Merges lockscreen.c, lockscreen_default.c, overview.c, tektool.c         */
/*  into one program to save disk space.                                    */
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
	if (!strcmp(filename, "lockscreen")) {
		exit(lockscreen_main(argc, argv));
		
	} else if (!strcmp(filename, "lockscreen_default")) {
		exit(lockscreen_default_main(argc, argv));
		
	} else if (!strcmp(filename, "overview")) { 
		exit(overview_main(argc, argv));
		
	} else if (!strcmp(filename, "tektool")) {
		exit(tektool_main(argc, argv));
		
	} else {
		printf("Couldn't run %s in lock_over_tek.main\n", argv[0]);
	}
	
	exit(0);
}
