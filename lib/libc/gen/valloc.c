#ifndef lint
static	char sccsid[] = "@(#)valloc.c 1.1 86/09/24 SMI"; /* from UCB 4.3 83/07/01 */
#endif

extern	unsigned getpagesize();
extern	char	*memalign();

char *
valloc(size)
	unsigned size;
{
	static unsigned pagesize = 0;
	if (!pagesize)
		pagesize = getpagesize();
	return memalign(pagesize, size);
}
