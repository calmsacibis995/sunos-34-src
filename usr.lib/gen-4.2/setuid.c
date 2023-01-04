#ifndef lint
static	char sccsid[] = "@(#)setuid.c 1.1 86/09/24 SMI"; /* from UCB 4.1 83/06/30 */
#endif

/*
 * Backwards compatible setuid.
 */
setuid(uid)
	int uid;
{

	return (setreuid(uid, uid));
}
