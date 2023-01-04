/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley Software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char *sccsid = "@(#)sh.misc.c 1.1 86/09/24 SMI; from UCB 5.2 6/6/85";
#endif

#include "sh.h"

/*
 * C Shell
 */

letter(c)
	register char c;
{

	return (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '_');
}

digit(c)
	register char c;
{

	return (c >= '0' && c <= '9');
}

alnum(c)
	register char c;
{
	return (letter(c) || digit(c));
}

any(c, s)
	register int c;
	register char *s;
{

	while (*s)
		if (*s++ == c)
			return(1);
	return(0);
}

onlyread(cp)
	char *cp;
{
	extern char end[];

	return (cp < end);
}

xfree(cp)
	char *cp;
{
	extern char end[];

	if (cp >= end && cp < (char *) &cp)
		free(cp);
}

char *
savestr(s)
	register char *s;
{
	char *n;
	register char *p;

	if (s == 0)
		s = "";
	for (p = s; *p++;)
		;
	n = p = xalloc((unsigned) (p - s));
	while (*p++ = *s++)
		;
	return (n);
}

char *
calloc(i, j)
	register unsigned i;
	unsigned j;
{
	register char *cp, *dp;

	i *= j;
	dp = cp = xalloc(i);
	if (i != 0)
		do
			*dp++ = 0;
		while (--i);
	return (cp);
}

nomem(i)
	unsigned i;
{
#ifdef debug
	static char *av[2] = {0, 0};
#endif

	child++;
#ifndef debug
	error("Out of memory");
#ifdef lint
	i = i;
#endif
#else
	showall(av);
	printf("i=%d: Out of memory\n", i);
	chdir("/usr/bill/cshcore");
	abort();
#endif
	return 0;		/* fool lint */
}

char **
blkend(up)
	register char **up;
{

	while (*up)
		up++;
	return (up);
}
 
blkpr(av)
	register char **av;
{

	for (; *av; av++) {
		printf("%s", *av);
		if (av[1])
			printf(" ");
	}
}

blklen(av)
	register char **av;
{
	register int i = 0;

	while (*av++)
		i++;
	return (i);
}

char **
blkcpy(oav, bv)
	char **oav;
	register char **bv;
{
	register char **av = oav;

	while (*av++ = *bv++)
		continue;
	return (oav);
}

char **
blkcat(up, vp)
	char **up, **vp;
{

	(void) blkcpy(blkend(up), vp);
	return (up);
}

blkfree(av0)
	char **av0;
{
	register char **av = av0;

	for (; *av; av++)
		XFREE(*av)
	XFREE((char *)av0)
}

char **
saveblk(v)
	register char **v;
{
	register char **newv =
		(char **) calloc((unsigned) (blklen(v) + 1), sizeof (char **));
	char **onewv = newv;

	while (*v)
		*newv++ = savestr(*v++);
	return (onewv);
}

char *
strspl(cp, dp)
	char *cp, *dp;
{
	char *ep;
	register char *p, *q;

	for (p = cp; *p++;)
		;
	for (q = dp; *q++;)
		;
	ep = xalloc((unsigned) ((p - cp) + (q - dp) - 1));
	for (p = ep, q = cp; *p++ = *q++;)
		;
	for (p--, q = dp; *p++ = *q++;)
		;
	return (ep);
}

char **
blkspl(up, vp)
	register char **up, **vp;
{
	register char **wp =
		(char **) calloc((unsigned) (blklen(up) + blklen(vp) + 1),
			sizeof (char **));

	(void) blkcpy(wp, up);
	return (blkcat(wp, vp));
}

lastchr(cp)
	register char *cp;
{

	if (!*cp)
		return (0);
	while (cp[1])
		cp++;
	return (*cp);
}

/*
 * This routine is called after an error to close up
 * any units which may have been left open accidentally.
 * Since the yellow pages keeps sockets open, close them first.
 */
closem()
{
	register int f;
	char name[256];
	
	getdomainname(name, sizeof(name));
	yp_unbind(name);
	for (f = 0; f < NOFILE; f++)
		if (f != SHIN && f != SHOUT && f != SHDIAG && f != OLDSTD &&
		    f != FSHTTY)
			(void) close(f);
}

/*
 * Close files before executing a file.
 * We could be MUCH more intelligent, since (on a version 7 system)
 * we need only close files here during a source, the other
 * shell fd's being in units 16-19 which are closed automatically!
 *
 * XXX	This routine will become unnecessary after we modify the kernel
 *	to conform to 4.3bsd semantics for close-on-exec.  In the
 *	interim, doexec() calls it.
 */
closech()
{
	register int f;
	static int nofile;

	if (nofile == 0)
		nofile = getdtablesize();

	SHIN = 0; SHOUT = 1; SHDIAG = 2; OLDSTD = 0;
	for (f = 3; f < nofile; f++)
		close(f);
}

donefds()
{

	(void) close(0);
	(void) close(1);
	(void) close(2);
	didfds = 0;
}

/*
 * Move descriptor i to j.
 * If j is -1 then we just want to get i to a safe place,
 * i.e. to a unit > 2.  This also happens in dcopy.
 */
dmove(i, j)
	register int i, j;
{

	if (i == j || i < 0)
		return (i);
	if (j >= 0) {
		(void) dup2(i, j);
		return (j);
	}
	j = dcopy(i, j);
	if (j != i)
		(void) close(i);
	return (j);
}

dcopy(i, j)
	register int i, j;
{

	if (i == j || i < 0 || j < 0 && i > 2)
		return (i);
	if (j >= 0) {
		(void) dup2(i, j);
		return (j);
	}
	(void) close(j);
	return (renum(i, j));
}

renum(i, j)
	register int i, j;
{
	register int k = dup(i);

	if (k < 0)
		return (-1);
	if (j == -1 && k > 2)
		return (k);
	if (k != j) {
		j = renum(k, j);
		(void) close(k);
		return (j);
	}
	return (k);
}

#ifndef copy
copy(to, from, size)
	register char *to, *from;
	register int size;
{

	if (size)
		do
			*to++ = *from++;
		while (--size != 0);
}
#endif

/*
 * Left shift a command argument list, discarding
 * the first c arguments.  Used in "shift" commands
 * as well as by commands like "repeat".
 */
lshift(v, c)
	register char **v;
	register int c;
{
	register char **u = v;

	while (*u && --c >= 0)
		xfree(*u++);
	(void) blkcpy(v, u);
}

number(cp)
	char *cp;
{

	if (*cp == '-') {
		cp++;
		if (!digit(*cp++))
			return (0);
	}
	while (*cp && digit(*cp))
		cp++;
	return (*cp == 0);
}

char **
copyblk(v)
	register char **v;
{
	register char **nv =
		(char **) calloc((unsigned) (blklen(v) + 1), sizeof (char **));

	return (blkcpy(nv, v));
}

char *
strend(cp)
	register char *cp;
{

	while (*cp)
		cp++;
	return (cp);
}

char *
strip(cp)
	char *cp;
{
	register char *dp = cp;

	while (*dp++ &= TRIM)
		continue;
	return (cp);
}

udvar(name)
	char *name;
{

	setname(name);
	bferr("Undefined variable");
}

prefix(sub, str)
	register char *sub, *str;
{

	for (;;) {
		if (*sub == 0)
			return (1);
		if (*str == 0)
			return (0);
		if (*sub++ != *str++)
			return (0);
	}
}
