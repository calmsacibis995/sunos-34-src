#ifndef lint
static	char sccsid[] = "@(#)yes.c 1.1 86/09/25 SMI"; /* from UCB 4.1 10/08/80 */
#endif
main(argc, argv)
char **argv;
{
	for (;;)
		printf("%s\n", argc>1? argv[1]: "y");
}
