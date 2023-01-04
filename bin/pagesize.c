#ifndef lint
static	char sccsid[] = "@(#)pagesize.c 1.1 86/09/24 SMI"; /* from UCB 4.1 82/11/07 */
#endif
/*
 * getpagesize
 */

main()
{

	printf("%d\n", getpagesize());
	exit(0);
}
