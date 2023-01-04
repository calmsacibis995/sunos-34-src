#ifndef lint
static	char sccsid[] = "@(#)prf.c 1.1 86/09/25";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "diag.h"
#include <sys/types.h>


/*
 * Scaled down version of C Library printf.
 * Only %s %u %d (==%u) %o %x %c %D %O %X are recognized.
 * Used to print diagnostic information
 * directly on console tty.
 * Since it is not interrupt driven,
 * all system activities are pretty much
 * suspended.
 * Printf should not be used for chit-chat.
 */
/*VARARGS1*/
printf(fmt, x1)
	register char *fmt;
	unsigned x1;
{
	register c;
	register unsigned int *adx;
	char *s;

	adx = &x1;
loop:
	while ((c = *fmt++) != '%') {
		if (c == '\0')
			return;
		putchar(c);
	}
	c = *fmt++;
	if (c == 'd' || c == 'u' || c == 'o' || c == 'x') {
		printn((u_long)*adx, c=='o'? 8: (c=='x'? 16:10));
	} else if (c == 'c') {
		putchar((int)*adx);
	} else if (c == 's') {
		s = (char *)*adx;
		while(c = *s++)
			putchar(c);
	} else if (c == 'D') {
		printn(*(u_long *)adx, 10);
		adx += (sizeof (long) / sizeof (int)) - 1;
	} else if (c == 'X') {
		printn(*(u_long *)adx, 16);
		adx += (sizeof (long) / sizeof (int)) - 1;
	} else if (c == 'O') {
		printn(*(u_long *)adx, 8);
		adx += (sizeof (long) / sizeof (int)) - 1;
	}
	adx++;
	goto loop;
}

/*
 * Print an unsigned integer in base b.
 */
printn(n, b)
	u_long n;
{
	register u_long a;

	if (b == 10 && (int)n < 0) {
		putchar('-');
		n = (u_long)(-(int)n);
	}
	if (a = n/b)
		printn(a, b);
	putchar("0123456789ABCDEF"[(int)(n%b)]);
}

/*
 * Read a line into the given buffer and handles
 * erase (^H or DEL), kill (^U), and interrupt (^C) characters.
 * This routine ASSUMES a maximum input line size of LINEBUFSZ
 * to guard against overflow of the buffer from obnoxious users.
 */
gets(buf)
	register char buf[];
{
	register char *lp = buf;
	register c;

	for (;;) {
		c = getchar() & 0177;

		switch (c) {

		case '\n':
		case '\r':
			*lp = '\0';
			putchar('\n');
			return;

		case 0177:
			putchar('\b');
			/* fall through */
		case '\b':
			putchar(' ');
			putchar('\b');
			if (--lp < buf)
				lp = buf;
			break;

		case 'u'&037:
			lp = buf;
			putchar('\n');
			break;

		case 'c'&037:
			putchar('\n');
			_longjmp(abort_jmp, 1);
			/*NOTREACHED*/

		default:
			if (lp < &buf[LINEBUFSZ-1]) {
				*lp++ = c;
			} else {
				putchar('\b');
				putchar(' ');
				putchar('\b');
			}
			break;
		}
	}
}

getn()
{
	char buf[LINEBUFSZ], *p;
	int num, err;

	for (;;) {
		gets(buf);
		for (p = buf;  p < &buf[LINEBUFSZ]; p++)
			if (*p != ' ' && *p != '\t')
				break;
		num = atoi(p, &err);
		if (*p == '\0' || p >= &buf[LINEBUFSZ] || err)
			printf("Bad number, re-enter: ");
		else
			break;
	}
	return (num);
}

getbn()
{
	char buf[LINEBUFSZ], *p;
	int num, err;

	for (;;) {
		gets(buf);
		for (p = buf;  p < &buf[LINEBUFSZ]; p++)
			if (*p != ' ' && *p != '\t')
				break;
		num = atobn(p, &err);
		if (*p == '\0' || p >= &buf[LINEBUFSZ] || err)
			printf("Bad number, re-enter: ");
		else
			break;
	}
	return (num);
}

putbn(n)
{
	int cyl, head, sec, nspc;

	nspc = nhead * nsect - nspare;
	cyl = n / nspc;
	sec = n % nspc;
	head = sec / nsect;
	sec %= nsect;
	printf("%d/%d/%d", cyl, head, sec);
}

getx()
{
	char buf[LINEBUFSZ], *p;
	int num, err;

	for (;;) {
		gets(buf);
		for (p = buf;  p < &buf[LINEBUFSZ]; p++)
			if (*p != ' ' && *p != '\t')
				break;
		num = atox(p, &err);
		if (*p == '\0' || p >= &buf[LINEBUFSZ] || err)
			printf("Bad number, re-enter: ");
		else
			break;
	}
	return (num);
}

pgetn(s)
char *s;
{

	printf(s);
	return (getn());
}

pgetbn(s)
char *s;
{

	printf(s);
	return (getbn());
}

atoi(s, err)
char *s;
int *err;
{
	int num = 0;
	char c;
	int minus = 0;

	*err = 0;
	if (*s == '-') {
	    minus++;
	    s++;
	}
	while (c = *s++) {
		switch (c) {

		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
		case '8': case '9':
			num = num*10 + (c - '0');
			break;

		default:
			*err = -1;
			return 0;
		}
	}
	if (minus)
	    num = -num;
	return (num);
}

atobn(p, err)
char *p;
int *err;
{
	int cyl = 0, head = 0, blk = 0;

	*err = 0;
	while (*p >= '0' && *p <= '9')
		cyl = 10*cyl + *p++ - '0';
	if (*p == '/') {
		p++;
		while (*p >= '0' && *p <= '9')
			head = 10*head + *p++ - '0';
		if (*p == '/') {
			p++;
			while (*p >= '0' && *p <= '9')
				blk = 10*blk + *p++ - '0';
		}
	} else {
		blk = cyl;
		cyl = 0;
	}
	if (*p)
		*err = -1;
	return ((cyl*(nhead*nsect-nspare))+(head*nsect)+blk);
}

atox(s, err)
char *s;
int *err;
{
	int num = 0;
	char c;

	*err = 0;
	while (c = *s++) {
		switch (c) {

		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
		case '8': case '9':
			num = num*16 + (c - '0');
			break;

		case 'a': case 'b': case 'c':
		case 'd': case 'e': case 'f':
			num = num*16 + (10 + c - 'a');
			break;

		case 'A': case 'B': case 'C':
		case 'D': case 'E': case 'F':
			num = num*16 + (10 + c - 'A');
			break;

		default:
			*err = -1;
			return 0;
		}
	}
	return (num);
}
