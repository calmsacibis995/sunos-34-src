#ifndef lint
static	char sccsid[] = "@(#)expand.c 1.1 86/09/24 SMI"; /* from S5R2 1.4 */
#endif

/*
 *	UNIX shell
 *
 *	Bell Telephone Laboratories
 *
 */

#include	"defs.h"
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/dir.h>

/*
 * globals (file name generation)
 *
 * "*" in params matches r.e ".*"
 * "?" in params matches r.e. "."
 * "[...]" in params matches character class
 * "[...a-z...]" in params matches a through z.
 *
 */
extern int	addg();


expand(as, rcnt)
	char	*as;
{
	int	count;
	DIR	*dirf;
	BOOL	dir = 0;
	char	*rescan = 0;
	register char	*s, *cs;
	struct argnod	*schain = gchain;
	struct stat statb;
	BOOL	slash;

	if (trapnote & SIGSET)
		return(0);
	s = cs = as;

	/*
	 * check for meta chars
	 */
	{
		register BOOL open;

		slash = 0;
		open = 0;
		do
		{
			switch (*cs++)
			{
			case 0:
				if (rcnt && slash)
					break;
				else
					return(0);

			case '/':
				slash++;
				open = 0;
				continue;

			case '[':
				open++;
				continue;

			case ']':
				if (open == 0)
					continue;

			case '?':
			case '*':
				if (rcnt > slash)
					continue;
				else
					cs--;
				break;


			default:
				continue;
			}
			break;
		} while (TRUE);
	}

	for (;;)
	{
		if (cs == s)
		{
			s = nullstr;
			break;
		}
		else if (*--cs == '/')
		{
			*cs = 0;
			if (s == cs)
				s = "/";
			break;
		}
	}

	if (dirf = opendir(*s ? s : "."))
	{
		if (fstat(dirf->dd_fd, &statb) != -1 &&
		    (statb.st_mode & S_IFMT) == S_IFDIR)
			dir++;
		else
			closedir(dirf);
	}

	count = 0;
	if (*cs == 0)
		*cs++ = 0200;
	if (dir)		/* check for rescan */
	{
		register char *rs;
		struct direct *e;

		rs = cs;
		do
		{
			if (*rs == '/')
			{
				rescan = rs;
				*rs = 0;
				gchain = 0;
			}
		} while (*rs++);

		if (setjmp(INTbuf) == 0)
			trapjmp = 1;
		while ((e = readdir(dirf)) && (trapnote & SIGSET) == 0)
		{
			if (e->d_name[0] == '.' && *cs != '.')
#ifndef BOURNE
				continue;
#else
			{
				if (e->d_name[1] == 0)
					continue;
				if (e->d_name[1] == '.' && e->d_name[2] == 0)
					continue;
			}
#endif

			if (gmatch(e->d_name, cs))
			{
				addg(s, e->d_name, rescan);
				count++;
			}
		}
		closedir(dirf);
		trapjmp = 0;

		if (rescan)
		{
			register struct argnod	*rchain;

			rchain = gchain;
			gchain = schain;
			if (count)
			{
				count = 0;
				while (rchain)
				{
					count += expand(rchain->argval, slash + 1);
					rchain = rchain->argnxt;
				}
			}
			*rescan = '/';
		}
	}

	{
		register char	c;

		s = as;
		while (c = *s)
			*s++ = (c & STRIP ? c : '/');
	}
	return(count);
}


gmatch(s, p)
register char	*s, *p;
{
	register int	scc;
	char		c;

	if (scc = *s++)
	{
		if ((scc &= STRIP) == 0)
			scc=0200;
	}
	switch (c = *p++)
	{
	case '[':
		{
			BOOL ok;
			int lc;
			int notflag = 0;

			ok = 0;
			lc = 077777;
			if (*p == '!')
			{
				notflag = 1;
				p++;
			}
			while (c = *p++)
			{
				if (c == ']')
					return(ok ? gmatch(s, p) : 0);
				else if (c == MINUS)
				{
					if (notflag)
					{
						if (scc < lc || scc > *(p++))
							ok++;
						else
							return(0);
					}
					else
					{
						if (lc <= scc && scc <= (*p++))
							ok++;
					}
				}
				else
				{
					lc = c & STRIP;
					if (notflag)
					{
						if (scc && scc != lc)
							ok++;
						else
							return(0);
					}
					else
					{
						if (scc == lc)
							ok++;
					}
				}
			}
			return(0);
		}

	default:
		if ((c & STRIP) != scc)
			return(0);

	case '?':
		return(scc ? gmatch(s, p) : 0);

	case '*':
		while (*p == '*')
			p++;

		if (*p == 0)
			return(1);
		--s;
		while (*s)
		{
			if (gmatch(s++, p))
				return(1);
		}
		return(0);

	case 0:
		return(scc == 0);
	}
}

static int
addg(as1, as2, as3)
char	*as1, *as2, *as3;
{
	register char	*s1, *s2;
	register int	c;

	s2 = locstak() + BYTESPERWORD;
	s1 = as1;
	while (c = *s1++)
	{
		if (s2 >= brkend)
			growstak(s2);
		if ((c &= STRIP) == 0)
		{
			*s2++ = '/';
			break;
		}
		*s2++ = c;
	}
	s1 = as2;
	for (;;)
	{
		if (s2 >= brkend)
			growstak(s2);
		if ((*s2 = *s1++) == 0)
			break;
		s2++;
	}
	if (s1 = as3)
	{
		if (s2 >= brkend)
			growstak(s2);
		*s2++ = '/';
		do
		{
			if (s2 >= brkend)
				growstak(s2);
		}
		while (*s2++ = *++s1);
	}
	makearg(endstak(s2));
}

makearg(args)
	register struct argnod *args;
{
	args->argnxt = gchain;
	gchain = args;
}
