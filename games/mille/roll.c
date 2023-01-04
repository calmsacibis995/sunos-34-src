#ifndef lint
static	char sccsid[] = "@(#)roll.c 1.1 86/09/24 SMI"; /* from UCB 1.1 04/01/82 */
#endif

/*
 *	This routine rolls ndie nside-sided dice.
 */

# define	reg	register

# ifndef BSD
# define	MAXRAND	32767L

roll(ndie, nsides)
int	ndie, nsides; {

	reg long	tot;
	reg unsigned	n;

	tot = 0;
	n = ndie;
	while (n--)
		tot += rand();
	return (int) ((tot * (long) nsides) / ((long) MAXRAND + 1)) + ndie;
}

# else

roll(ndie, nsides)
reg int	ndie, nsides; {

	reg int		tot, r;
	reg double	num_sides;

	num_sides = nsides;
	tot = 0;
	while (ndie--)
		tot += (r = rand()) * (num_sides / 017777777777) + 1;
	return tot;
}
# endif

