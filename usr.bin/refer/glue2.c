#ifndef lint
static	char sccsid[] = "@(#)glue2.c 1.1 86/09/25 SMI"; /* from UCB 4.1 5/6/83 */
#endif

char refdir[50];

savedir()
{
	if (refdir[0]==0)
		corout ("", refdir, "/bin/pwd", "", 50);
	trimnl(refdir);
}

restodir()
{
	chdir(refdir);
}
