#ifndef lint
static	char *sccsid = "@(#)accton.c 1.1 86/09/25 SMI"; /* from UCB 4.1 */
#endif
main(argc, argv)
char **argv;
{
	extern errno;
	if (argc > 1)
		acct(argv[1]);
	else
		acct((char *)0);
	if (errno) {
		perror("accton");
		exit(1);
	}
	exit(0);
}
