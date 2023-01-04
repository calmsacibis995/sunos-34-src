#ifndef lint
static	char sccsid[] = "@(#)ttyslot.c 1.1 86/09/24 SMI"; /* from UCB 4.1 80/12/21 */
#endif

/*
 * Return the number of the slot in the utmp file
 * corresponding to the current user: try for file 0, 1, 2.
 * Definition is the line number in the /etc/ttys file.
 *
 * We do this by finding out the name (in /dev) of the tty on files 0/1/2,
 * then searching the /etc/ttys file for that name.  Neither is a particularly
 * fast process.
 */

#include <stdio.h>

char	*ttyname();
char	*rindex();

static	char	ttys[]	= "/etc/ttys";

#define	NULL	0

int
ttyslot()
{
	register char *tp, *p;
	register s;
	register FILE *tf;
	char line[32];
	register char *lp;

	if ((tp=ttyname(0))==NULL && (tp=ttyname(1))==NULL && (tp=ttyname(2))==NULL)
		return(0);
	if ((p = rindex(tp, '/')) == NULL)
		p = tp;
	else
		p++;
	if ((tf=fopen(ttys, "r")) == NULL)
		return(0);
	s = 0;
	while (NULL != fgets(line, sizeof(line), tf)) {

		/* Clean out the trailing \n (sigh) */
		lp = line + strlen(line)-1;
		if (*lp == '\n') *lp = '\0';

		s++;
		if (strcmp(p, line+2)==0) {
			(void)fclose(tf);
			return(s);
		}
	}
	(void)fclose(tf);
	return(0);
}

