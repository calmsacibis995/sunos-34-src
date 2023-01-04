#ifndef lint
static	char sccsid[] = "@(#)setgid.c 1.1 86/09/24 SMI"; /* from UCB 4.1 83/06/30 */
#endif

/*
 * Backwards compatible setgid.
 */
setgid(gid)
	int gid;
{

	return (setregid(gid, gid));
}
