/*
 * @(#)get.c 1.1 86/09/25 Copyright (c) 1985 by Sun Microsystems, Inc.
 */
#include "../mon/sunromvec.h"

getchar()
{
	register int c;

	while ((c = (*romp->v_mayget)()) == -1)
		;
	if (c == '\r')
		c = '\n';
	if (c == 0177 || c == '\b') {
		putchar('\b');
		putchar(' ');
		c = '\b';
	}
	putchar(c);
	return (c);
}

gets(buf)
	char *buf;
{
	register char *lp;
	register c;

	lp = buf;
	for (;;) {
		c = getchar() & 0177;
		switch(c) {
		case '\n':
		case '\r':
			c = '\n';
			*lp++ = '\0';
			return;
		case '\b':
			lp--;
			if (lp < buf)
				lp = buf;
			continue;
		case 'u'&037:			/* ^U */
			lp = buf;
			putchar('\n');
			continue;
		default:
			*lp++ = c;
		}
	}
}
