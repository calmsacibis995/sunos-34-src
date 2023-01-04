#ifndef lint
static	char sccsid[] = "@(#)strncpy.c 1.1 86/09/24 SMI"; /* from UCB 4.1 80/12/21 */
#endif

/*
 * Copy s2 to s1, truncating or null-padding to always copy n bytes
 * return s1
 */

char *
strncpy(s1, s2, n)
	register char *s1, *s2;
{
	register i;
	register char *os1;

	os1 = s1;
	for (i = 0; i < n; i++)
		if ((*s1++ = *s2++) == '\0') {
			while (++i < n)
				*s1++ = '\0';
			return (os1);
		}
	return (os1);
}
