/*	cfgets.c	1.1	86/09/25	*/
/*********************************************************************
function:	cfgets
description:	get nonblank, non-comment, (possibly continued) line
programmer:	Alan S. Watt

history:
	11/04/81	original version
*********************************************************************/

#include <stdio.h>
#define COMMENT		'#'
#define CONTINUE	'\\'
#define EOLN		'\n'
#define EOS		'\0'

char *
cfgets (buf, siz, fil)
register char *buf;
int siz;
FILE *fil;
{
	register i, c, len;
	register char *s;
	char *fgets();

	for (i=0,s=buf; i = (fgets (s, siz-i, fil) != NULL); i = s - buf) {

		/* get last character of line */
		c = s[len = (strlen (s) - 1)];

		/* skip comments; make sure end of comment line seen */
		if (*s == COMMENT) {
			while (c != EOLN && c != EOF)
				c = fgetc (fil);
			*s = EOS;
		}

		/* skip blank lines */
		else if (*s != EOLN) {
			s += len;

			/* continue lines ending with CONTINUE */
			if (c != EOLN || *--s != CONTINUE)
				break;
		}
	}
	
	return (i ? buf : NULL);
}

#ifdef TEST
# include <stdio.h>
main ()
{
	char buf[512];

	while (cfgets (buf, sizeof buf, stdin))
		fputs (buf, stdout);
}
#endif TEST
