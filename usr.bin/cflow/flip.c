#ifndef lint
static	char sccsid[] = "@(#)flip.c 1.1 86/09/25 SMI"; /* from S5R2 1.2 */
#endif

#include <stdio.h>
#include <ctype.h>

main()
	{
	char line[BUFSIZ], *pl, *gets();

	while (pl = gets(line))
		{
		while (*pl != ':')
			++pl;
		*pl++ = '\0';
		while (isspace(*pl))
			++pl;
		printf("%s : %s\n", pl, line);
		}
	}
