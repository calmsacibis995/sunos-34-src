#ifndef lint
static	char sccsid[] = "@(#)satoi.c 1.1 86/09/24 SMI"; /* from S5R2 3.1 */
#endif

# include	"sys/types.h"
# include	"macros.h"

char *satoi(p,ip)
register char *p;
register int *ip;
{
	register int sum;

	sum = 0;
	while (numeric(*p))
		sum = sum * 10 + (*p++ - '0');
	*ip = sum;
	return(p);
}
