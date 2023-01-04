#ifndef lint
static	char sccsid[] = "@(#)bcopy.c 1.1 86/09/24 SMI";
#endif

/*
 * Copy s1 to s2, always copy n bytes.
 */
void
bcopy(s1, s2, len)
	register char *s1, *s2;
	int len;
{
	register int n;

	if ((n = len) <= 0)
		return;
	do
		*s2++ = *s1++;
	while (--n);
}
