/*
 * @(#)common.c 1.1 86/09/25 Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Common code for various bootstrap routines.
 */

bzero(p, n)
	register char *p;
	register int n;
{
	register char zeero = 0;

	while (n > 0)
		*p++ = zeero, n--;	/* Avoid clr for 68000, still... */
}

bcopy(src, dest, count)
	register char *src, *dest;
	register int count;
{
	while (--count != -1)
		*dest++ = *src++;
}

/*
 * Compare strings:  s1>s2: >0  s1==s2: 0  s1<s2: <0
 */
strcmp(s1, s2)
	register char *s1, *s2;
{

	while (*s1 == *s2++)
		if (*s1++=='\0')
			return (0);
	return (*s1 - *--s2);
}

/*
 * Returns the number of
 * non-NULL bytes in string argument.
 */
strlen(s)
	register char *s;
{
	register int n;

	n = 0;
	while (*s++)
		n++;
	return (n);
}
