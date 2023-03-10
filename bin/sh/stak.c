#ifndef lint
static	char sccsid[] = "@(#)stak.c 1.1 86/09/24 SMI"; /* from S5R2 1.4 */
#endif

/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */

#include	"defs.h"


/* ========	storage allocation	======== */

char *
getstak(asize)			/* allocate requested stack */
int	asize;
{
	register char	*oldstak;
	register int	size;

	size = round(asize, BYTESPERWORD);
	oldstak = stakbot;
	staktop = stakbot += size;
	if (staktop >= brkend)
		growstak(staktop);
	return(oldstak);
}

/*
 * set up stack for local use
 * should be followed by `endstak'
 */
char *
locstak()
{
	if (brkend - stakbot < BRKINCR)
	{
		if (setbrk(brkincr) == -1)
			error(nostack);
		if (brkincr < BRKMAX)
			brkincr += 256;
	}
	return(stakbot);
}

void
growstak(newtop)
char	*newtop;
{
	register unsigned	incr;

	incr = newtop - brkend + 1;
	if (brkincr > incr)
		incr = brkincr;
	if (setbrk(incr) == -1)
		error(nospace);
}

char *
savstak()
{
	assert(staktop == stakbot);
	return(stakbot);
}

char *
endstak(argp)		/* tidy up after `locstak' */
register char	*argp;
{
	register char	*oldstak;

	if (argp >= brkend)
		growstak(staktop);
	*argp++ = 0;
	oldstak = stakbot;
	stakbot = staktop = (char *)round(argp, BYTESPERWORD);
	return(oldstak);
}

tdystak(x)		/* try to bring stack back to x */
register char	*x;
{
	while ((char *)(stakbsy) > (char *)(x))
	{
		free(stakbsy);
		stakbsy = stakbsy->word;
	}
	staktop = stakbot = max((char *)(x), (char *)(stakbas));
	rmtemp(x);
}

stakchk()
{
	if ((brkend - stakbas) > BRKINCR + BRKINCR)
		setbrk(-BRKINCR);
}

char *
cpystak(x)
char	*x;
{
	return(endstak(movstrstak(x, locstak())));
}

char *
movstrstak(a, b)
register char	*a, *b;
{
	do
	{
		if (b >= brkend)
			growstak(b);
	}
	while (*b++ = *a++);
	return(--b);
}
