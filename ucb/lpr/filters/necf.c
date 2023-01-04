#ifndef lint
static	char sccsid[] = "@(#)necf.c 1.1 86/09/25 SMI"; /* from UCB 1.6 83/04/29 */
#endif

#include <stdio.h>
#include <sgtty.h>

#define PAGESIZE	66

main()
{
	extern char *rindex();
	static char sobuf[BUFSIZ];
	char line[256];
	register char c, *cp;
	register lnumber;

	setbuf(stdout, sobuf);
#ifdef SHEETFEEDER
	printf("\033=\033\033\033O\f");
#else
	printf("\033=");
#endif
	lnumber = 0;
	while (fgets(line, sizeof(line), stdin) != NULL) {
#ifdef SHEETFEEDER
		if (lnumber == PAGESIZE-1) {
			putchar('\f');
			lnumber = 0;
		}
		if (lnumber >= 2) {
#endif
#ifdef TTY
			if ((cp = rindex(line, '\n')) != NULL)
				*cp = '\r';
#endif
			printf("%s", line);
#ifdef SHEETFEEDER
		}
		lnumber++;
#endif
	}
	fflush (stdout);
}
