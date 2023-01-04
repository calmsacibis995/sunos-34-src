#ifndef lint
static	char sccsid[] = "@(#)setruid.c 1.1 86/09/24 SMI"; /* from UCB 4.1 83/06/30 */
#endif

setruid(ruid)
	int ruid;
{

	return (setreuid(ruid, -1));
}
