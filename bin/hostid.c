#ifndef lint
static	char sccsid[] = "@(#)hostid.c 1.1 86/09/24 SMI"; /* from UCB 4.1 82/11/07 */
#endif

/*
 * hostid
 */
main(argc, argv)
	int argc;
	char **argv;
{
	printf("%x\n", gethostid());
}
