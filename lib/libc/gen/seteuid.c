#ifndef lint
static	char sccsid[] = "@(#)seteuid.c 1.1 86/09/24 SMI"; /* from UCB 4.1 83/06/30 */
#endif

seteuid(euid)
	int euid;
{

	return (setreuid(-1, euid));
}
