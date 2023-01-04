/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley Software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char *sccsid = "@(#)sh.dir.c 1.1 86/09/24 SMI; from UCB 5.3 6/11/85";
#endif

#include "sh.h"
#include "sh.dir.h"

/*
 * C Shell - directory management
 */

struct	directory *dfind();
char	*dfollow();
char	*dcanon();
struct	directory dhead;		/* "head" of loop */
int	printd;				/* force name to be printed */
static	char *fakev[] = { "dirs", NOSTR };

/*
 * dinit - initialize current working directory
 */
dinit(hp)
	char *hp;
{
	register char *cp;
	register struct directory *dp;
	char path[MAXPATHLEN];

	if (loginsh && hp)
		cp = hp;
	else {
		cp = getwd(path);
		if (cp == NULL) {
			(void) write(2, path, strlen(path));
			exit(1);
		}
	}
	dp = (struct directory *)calloc(sizeof (struct directory), 1);
	dp->di_name = savestr(cp);
	dp->di_count = 0;
	dhead.di_next = dhead.di_prev = dp;
	dp->di_next = dp->di_prev = &dhead;
	printd = 0;
	dnewcwd(dp);
}

/*
 * dodirs - list all directories in directory loop
 */
dodirs(v)
	char **v;
{
	register struct directory *dp;
	bool lflag;
	char *hp = value("home");

	if (*hp == '\0')
		hp = NOSTR;
	if (*++v != NOSTR)
		if (eq(*v, "-l") && *++v == NOSTR)
			lflag = 1;
		else
			error("Usage: dirs [ -l ]");
	else
		lflag = 0;
	dp = dcwd;
	do {
		if (dp == &dhead)
			continue;
		if (!lflag && hp != NOSTR) {
			dtildepr(hp, dp->di_name);
		} else
			printf("%s", dp->di_name);
		printf(" ");
	} while ((dp = dp->di_prev) != dcwd);
	printf("\n");
}

dtildepr(home, dir)
	register char *home, *dir;
{

	if (!eq(home, "/") && prefix(home, dir))
		printf("~%s", dir + strlen(home));
	else
		printf("%s", dir);
}

/*
 * dochngd - implement chdir command.
 */
dochngd(v)
	char **v;
{
	register char *cp;
	register struct directory *dp;

	printd = 0;
	if (*++v == NOSTR) {
		if ((cp = value("home")) == NOSTR || *cp == 0)
			bferr("No home directory");
		if (chdir(cp) < 0)
			bferr("Can't change to home directory");
		cp = savestr(cp);
	} else if ((dp = dfind(*v)) != 0) {
		printd = 1;
		if (chdir(dp->di_name) < 0)
			Perror(dp->di_name);
		dcwd->di_prev->di_next = dcwd->di_next;
		dcwd->di_next->di_prev = dcwd->di_prev;
		goto flushcwd;
	} else
		cp = dfollow(*v);
	dp = (struct directory *)calloc(sizeof (struct directory), 1);
	dp->di_name = cp;
	dp->di_count = 0;
	dp->di_next = dcwd->di_next;
	dp->di_prev = dcwd->di_prev;
	dp->di_prev->di_next = dp;
	dp->di_next->di_prev = dp;
flushcwd:
	dfree(dcwd);
	dnewcwd(dp);
}

/*
 * dfollow - change to arg directory; fall back on cdpath if not valid
 */
char *
dfollow(cp)
	register char *cp;
{
	register char *dp;
	struct varent *c;

	cp = globone(cp);
	if (chdir(cp) >= 0)
		goto gotcha;
	/*
	 * Try interpreting wrt successive components of cdpath.
	 */
	if (cp[0] != '/' && !prefix("./", cp) && !prefix("../", cp)
	    && (c = adrof("cdpath"))) {
		char **cdp;
		register char *p;
		char buf[MAXPATHLEN];

		for (cdp = c->vec; *cdp; cdp++) {
			for (dp = buf, p = *cdp; *dp++ = *p++;)
				;
			dp[-1] = '/';
			for (p = cp; *dp++ = *p++;)
				;
			if (chdir(buf) >= 0) {
				printd = 1;
				xfree(cp);
				cp = savestr(buf);
				goto gotcha;
			}
		}
	}
	/*
	 * Try dereferencing the variable named by the argument.
	 */
	dp = value(cp);
	if ((dp[0] == '/' || dp[0] == '.') && chdir(dp) >= 0) {
		xfree(cp);
		cp = savestr(dp);
		printd = 1;
		goto gotcha;
	}
	xfree(cp);			/* XXX, use after free */
	Perror(cp);

gotcha:
	if (*cp != '/') {
		register char *p, *q;
		int cwdlen;

		/*
		 * All in the name of efficiency?
		 */
		for (p = dcwd->di_name; *p++;)
			;
		if ((cwdlen = p - dcwd->di_name - 1) == 1)	/* root */
			cwdlen = 0;
		for (p = cp; *p++;)
			;
		dp = xalloc((unsigned) (cwdlen + (p - cp) + 1));
		for (p = dp, q = dcwd->di_name; *p++ = *q++;)
			;
		if (cwdlen)
			p[-1] = '/';
		else
			p--;			/* don't add a / after root */
		for (q = cp; *p++ = *q++;)
			;
		xfree(cp);
		cp = dp;
		dp += cwdlen;
	} else
		dp = cp;
	return dcanon(cp, dp);
}

/*
 * dopushd - push new directory onto directory stack.
 *	with no arguments exchange top and second.
 *	with numeric argument (+n) bring it to top.
 */
dopushd(v)
	char **v;
{
	register struct directory *dp;

	printd = 1;
	if (*++v == NOSTR) {
		if ((dp = dcwd->di_prev) == &dhead)
			dp = dhead.di_prev;
		if (dp == dcwd)
			bferr("No other directory");
		if (chdir(dp->di_name) < 0)
			Perror(dp->di_name);
		dp->di_prev->di_next = dp->di_next;
		dp->di_next->di_prev = dp->di_prev;
		dp->di_next = dcwd->di_next;
		dp->di_prev = dcwd;
		dcwd->di_next->di_prev = dp;
		dcwd->di_next = dp;
	} else if (dp = dfind(*v)) {
		if (chdir(dp->di_name) < 0)
			Perror(dp->di_name);
	} else {
		register char *cp;

		cp = dfollow(*v);
		dp = (struct directory *)calloc(sizeof (struct directory), 1);
		dp->di_name = cp;
		dp->di_count = 0;
		dp->di_prev = dcwd;
		dp->di_next = dcwd->di_next;
		dcwd->di_next = dp;
		dp->di_next->di_prev = dp;
	}
	dnewcwd(dp);
}

/*
 * dfind - find a directory if specified by numeric (+n) argument
 */
struct directory *
dfind(cp)
	register char *cp;
{
	register struct directory *dp;
	register int i;
	register char *ep;

	if (*cp++ != '+')
		return (0);
	for (ep = cp; digit(*ep); ep++)
		continue;
	if (*ep)
		return (0);
	i = getn(cp);
	if (i <= 0)
		return (0);
	for (dp = dcwd; i != 0; i--) {
		if ((dp = dp->di_prev) == &dhead)
			dp = dp->di_prev;
		if (dp == dcwd)
			bferr("Directory stack not that deep");
	}
	return (dp);
}

/*
 * dopopd - pop a directory out of the directory stack
 *	with a numeric argument just discard it.
 */
dopopd(v)
	char **v;
{
	register struct directory *dp, *p;

	printd = 1;
	if (*++v == NOSTR)
		dp = dcwd;
	else if ((dp = dfind(*v)) == 0)
		bferr("Invalid argument");
	if (dp->di_prev == &dhead && dp->di_next == &dhead)
		bferr("Directory stack empty");
	if (dp == dcwd) {
		if ((p = dp->di_prev) == &dhead)
			p = dhead.di_prev;
		if (chdir(p->di_name) < 0)
			Perror(p->di_name);
	}
	dp->di_prev->di_next = dp->di_next;
	dp->di_next->di_prev = dp->di_prev;
	if (dp == dcwd)
		dnewcwd(p);
	else
		dodirs(fakev);
	dfree(dp);
}

/*
 * dfree - free the directory (or keep it if it still has ref count)
 */
dfree(dp)
	register struct directory *dp;
{

	if (dp->di_count != 0)
		dp->di_next = dp->di_prev = 0;
	else
		xfree(dp->di_name), xfree((char *)dp);
}

/*
 * dcanon - canonicalize the pathname, removing excess ./ and ../ etc.
 *	We are of course assuming that the file system is standardly
 *	constructed (always have ..'s, directories have links).
 *
 *	If the hardpaths shell variable is set, resolve the
 *	resulting pathname to contain no symbolic link components.
 */
char *
dcanon(cp, p)
	register char *cp, *p;
{
	register char	*sp;	/* rightmost component currently under
				   consideration */
	register char	*p1,	/* general purpose */
			*p2;
	bool slash, dotdot, hardpaths;

	if (*cp != '/')
		abort();

	if (hardpaths = (adrof("hardpaths") != NULL)) {
		/*
		 * Be paranoid: don't trust the initial prefix
		 * to be symlink-free.
		 */
		p = cp;
	}

	/*
	 * Loop invariant: cp points to the overall path start,
	 * p to its as yet uncanonicalized trailing suffix.
	 */
	while (*p) {			/* for each component */
		sp = p;			/* save slash address */

		while (*++p == '/')	/* flush extra slashes */
			;
		if (p != ++sp)
			for (p1 = sp, p2 = p; *p1++ = *p2++;)
				;

		p = sp;			/* save start of component */
		slash = 0;
		while (*++p)		/* find next slash or end of path */
			if (*p == '/') {
				slash = 1;
				*p = '\0';
				break;
			}

		if (*sp == '\0') {
			/* component is null */
			if (--sp == cp)	/* if path is one char (i.e. /) */
				break;
			else
				*sp = '\0';
			continue;
		}

		if (sp[0] == '.' && sp[1] == '\0') {
			/* Squeeze out component consisting of "." */
			if (slash) {
				for (p1 = sp, p2 = p + 1; *p1++ = *p2++;)
					;
				p = --sp;
			} else if (--sp != cp)
				*sp = '\0';
			continue;
		}

		/*
		 * At this point we have a path of the form "x/yz",
		 * where "x" is null or rooted at "/", "y" is a single
		 * component, and "z" is possibly null.  The pointer cp
		 * points to the start of "x", sp to the start of "y",
		 * and p to the beginning of "z", which has been forced
		 * to a null.
		 */
		/*
		 * Process symbolic link component.  Provided that either
		 * the hardpaths shell variable is set or "y" is really
		 * ".." we replace the symlink with its contents.  The
		 * second condition for replacement is necessary to make
		 * the command "cd x/.." produce the same results as the
		 * sequence "cd x; cd ..".
		 *
		 * Note that the two conditions correspond to different
		 * potential symlinks.  When hardpaths is set, we must
		 * check "x/y"; otherwise, when "y" is known to be "..",
		 * we check "x".
		 */
		dotdot = sp[0] == '.' && sp[1] == '.' && sp[2] == '\0';
		if (hardpaths || dotdot) {
			char link[MAXPATHLEN];
			int cc;
			char *newcp;

			/*
			 * Isolate the end of the component that is to
			 * be checked for symlink-hood.
			 */
			sp--;
			if (! hardpaths)
				*sp = '\0';

			/*
			 * See whether the component is really a symlink by
			 * trying to read it.  If the read succeeds, it is.
			 */
			if ((hardpaths || sp > cp) &&
			    (cc = readlink(cp, link, sizeof link)) >= 0) {
				link[cc] = '\0';

				/* Restore path. */
				if (slash)
					*p = '/';

				/*
				 * Point p at the start of the trailing
				 * path following the symlink component.
				 * It's already there is hardpaths is set.
				 */
				if (! hardpaths) {
					/* Restore path as well. */
					*(p = sp) = '/';
				}

				/*
				 * Find length of p.
				 */
				for (p1 = p; *p1++;)
					;

				if (*link != '/') {
					/*
					 * Relative path: replace the symlink
					 * component with its value.  First,
					 * set sp to point to the slash at
					 * its beginning.  If hardpaths is
					 * set, this is already the case.
					 */
					if (! hardpaths) {
						while (*--sp != '/')
							;
					}

					/*
					 * Terminate the leading part of the
					 * path, including trailing slash.
					 */
					sp++;
					*sp = '\0';

					/*
					 * New length is: "x/" + link + "z"
					 */
					p1 = newcp = xalloc((unsigned)
						((sp - cp) + cc + (p1 - p)));
					/*
					 * Copy new path into newcp
					 */
					for (p2 = cp; *p1++ = *p2++;)
						;
					for (p1--, p2 = link; *p1++ = *p2++;)
						;
					for (p1--, p2 = p; *p1++ = *p2++;)
						;
					/*
					 * Restart canonicalization at
					 * expanded "/y".
					 */
					p = sp - cp - 1 + newcp;
				} else {
					/*
					 * New length is: link + "z"
					 */
					p1 = newcp = xalloc((unsigned)
						(cc + (p1 - p)));
					/*
					 * Copy new path into newcp
					 */
					for (p2 = link; *p1++ = *p2++;)
						;
					for (p1--, p2 = p; *p1++ = *p2++;)
						;
					/*
					 * Restart canonicalization at beginning
					 */
					p = newcp;
				}
				xfree(cp);
				cp = newcp;
				continue;	/* canonicalize the link */
			}

			/* The component wasn't a symlink after all. */
			if (! hardpaths)
				*sp = '/';
		}

		if (dotdot) {
			if (sp != cp)
				while (*--sp != '/')
					;
			if (slash) {
				for (p1 = sp + 1, p2 = p + 1; *p1++ = *p2++;)
					;
				p = sp;
			} else if (cp == sp)
				*++sp = '\0';
			else
				*sp = '\0';
			continue;
		}

		if (slash)
			*p = '/';
	}
	return cp;
}
 
/*
 * dnewcwd - make a new directory in the loop the current one
 *	and export its name to the PWD environment variable.
 */
dnewcwd(dp)
	register struct directory *dp;
{

	dcwd = dp;
#ifdef notdef
	/*
	 * If we have a fast version of getwd available
	 * and hardpaths is set, it would be reasonable
	 * here to verify that dcwd->di_name really does
	 * name the current directory.  Later...
	 */
#endif notdef
	set("cwd", savestr(dcwd->di_name));
	setenv("PWD", dcwd->di_name);
	if (printd)
		dodirs(fakev);
}
