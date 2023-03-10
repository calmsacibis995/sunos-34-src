#ifndef lint
static	char sccsid[] = "@(#)getpar.c 1.1 86/09/24 SMI"; /* from UCB 4.3 05/28/83 */
#endif

# include	<stdio.h>
# include	"getpar.h"

/**
 **	get integer parameter
 **/

getintpar(s)
char	*s;
{
	register int	i;
	int		n;

	while (1)
	{
		if (testnl() && s)
			printf("%s: ", s);
		i = scanf("%d", &n);
		if (i < 0)
			exit(1);
		if (i > 0 && testterm())
			return (n);
		printf("invalid input; please enter an integer\n");
		skiptonl(0);
	}
}

/**
 **	get floating parameter
 **/

double getfltpar(s)
char	*s;
{
	register int		i;
	double			d;

	while (1)
	{
		if (testnl() && s)
			printf("%s: ", s);
		i = scanf("%lf", &d);
		if (i < 0)
			exit(1);
		if (i > 0 && testterm())
			return (d);
		printf("invalid input; please enter a double\n");
		skiptonl(0);
	}
}

/**
 **	get yes/no parameter
 **/

struct cvntab	Yntab[] =
{
	"y",	"es",	(int (*)())1,	0,
	"n",	"o",	(int (*)())0,	0,
	0
};

getynpar(s)
char	*s;
{
	struct cvntab		*r;

	r = getcodpar(s, Yntab);
	return ((int) r->value);
}


/**
 **	get coded parameter
 **/

struct cvntab *getcodpar(s, tab)
char		*s;
struct cvntab	tab[];
{
	char				input[100];
	register struct cvntab		*r;
	int				flag;
	register char			*p, *q;
	int				c;
	int				f;

	flag = 0;
	while (1)
	{
		flag |= (f = testnl());
		if (flag)
			printf("%s: ", s);
		if (f)
			cgetc(0);		/* throw out the newline */
		scanf("%*[ \t;]");
		if ((c = scanf("%[^ \t;\n]", input)) < 0)
			exit(1);
		if (c == 0)
			continue;
		flag = 1;

		/* if command list, print four per line */
		if (input[0] == '?' && input[1] == 0)
		{
			c = 4;
			for (r = tab; r->abrev; r++)
			{
				concat(r->abrev, r->full, input);
				printf("%14.14s", input);
				if (--c > 0)
					continue;
				c = 4;
				printf("\n");
			}
			if (c != 4)
				printf("\n");
			continue;
		}

		/* search for in table */
		for (r = tab; r->abrev; r++)
		{
			p = input;
			for (q = r->abrev; *q; q++)
				if (*p++ != *q)
					break;
			if (!*q)
			{
				for (q = r->full; *p && *q; q++, p++)
					if (*p != *q)
						break;
				if (!*p || !*q)
					break;
			}
		}

		/* check for not found */
		if (!r->abrev)
		{
			printf("invalid input; ? for valid inputs\n");
			skiptonl(0);
		}
		else
			return (r);
	}
}


/**
 **	get string parameter
 **/

getstrpar(s, r, l, t)
char	*s;
char	*r;
int	l;
char	*t;
{
	register int	i;
	char		format[20];
	register int	f;

	if (t == 0)
		t = " \t\n;";
	sprintf(format, "%%%d[^%s]", l, t);
	while (1)
	{
		if ((f = testnl()) && s)
			printf("%s: ", s);
		if (f)
			cgetc(0);
		scanf("%*[\t ;]");
		i = scanf(format, r);
		if (i < 0)
			exit(1);
		if (i != 0)
			return;
	}
}


/**
 **	test if newline is next valid character
 **/

testnl()
{
	register char		c;

	while ((c = cgetc(0)) != '\n')
		if ((c >= '0' && c <= '9') || c == '.' || c == '!' ||
				(c >= 'A' && c <= 'Z') ||
				(c >= 'a' && c <= 'z') || c == '-')
		{
			ungetc(c, stdin);
			return(0);
		}
	ungetc(c, stdin);
	return (1);
}


/**
 **	scan for newline
 **/

skiptonl(c)
char	c;
{
	while (c != '\n')
		if (!(c = cgetc(0)))
			return;
	ungetc('\n', stdin);
	return;
}


/**
 **	test for valid terminator
 **/

testterm()
{
	register char		c;

	if (!(c = cgetc(0)))
		return (1);
	if (c == '.')
		return (0);
	if (c == '\n' || c == ';')
		ungetc(c, stdin);
	return (1);
}


/*
**  TEST FOR SPECIFIED DELIMETER
**
**	The standard input is scanned for the parameter.  If found,
**	it is thrown away and non-zero is returned.  If not found,
**	zero is returned.
*/

readdelim(d)
char	d;
{
	register char	c;

	while (c = cgetc(0))
	{
		if (c == d)
			return (1);
		if (c == ' ')
			continue;
		ungetc(c, stdin);
		break;
	}
	return (0);
}
