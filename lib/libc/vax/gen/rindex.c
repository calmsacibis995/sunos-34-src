#ifndef lint
static	char sccsid[] = "@(#)rindex.c 1.1 86/09/24 SMI"; /* from UCB 4.1 80/12/21 */
#endif

/*
 * Return the ptr in sp at which the character c last
 * appears; NULL if not found
 */

#define NULL 0

char *
rindex(sp, c)
	register char *sp, c;
{
	register char *r;

	r = NULL;
	do {
		if (*sp == c)
			r = sp;
	} while (*sp++);
	return (r);
}
