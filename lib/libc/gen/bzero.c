#ifndef lint
static	char sccsid[] = "@(#)bzero.c 1.1 86/09/24 SMI";
#endif

/*
 * Set an array of n chars starting at sp to zero.
 */
void
bzero(sp, len)
	register char *sp;
	int len;
{
	register int n;

	if ((n = len) <= 0)
		return;
	do
		*sp++ = 0;
	while (--n);
}
