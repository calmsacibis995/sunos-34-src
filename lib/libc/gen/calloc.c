#ifndef lint
static	char sccsid[] = "@(#)calloc.c 1.1 86/09/24 SMI"; /* from UCB 4.1 80/12/21 */
#endif

/*
 * calloc - allocate and clear memory block
 */
#define CHARPERINT (sizeof(int)/sizeof(char))
#define NULL 0

char *
calloc(num, size)
	unsigned num, size;
{
	register char *mp;
	char *malloc();

	num *= size;
	mp = malloc(num);
	if (mp == NULL)
		return(NULL);
	bzero(mp, num);
	return (mp);
}

cfree(p, num, size)
	char *p;
	unsigned num, size;
{

	free(p);
}
