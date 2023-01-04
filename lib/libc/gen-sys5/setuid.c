#ifndef lint
static	char sccsid[] = "@(#)setuid.c 1.1 86/09/24 SMI";
#endif

/*
 * SVID-compatible setuid.
 */
setuid(uid)
	int uid;
{

	if (geteuid() == 0)
		return (setreuid(uid, uid));
	else
		return (setreuid(-1, uid));
}
