#ifndef lint
static	char sccsid[] = "@(#)insert_brackets.c 1.6 87/01/07";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 *	insert_brackets -- Filter to insert/remove brackets from stdin and
 *	write the result to stdout.  Left and right bracket strings obtained
 *	as argv[1] and argv[2], respectively.  Note that the .textswrc file
 *	is processed such that one layer of \'s are removed.  Therefore, to
 *	get the string \fP specify either \\\\fP or "\\fP" (literally).  Strings
 *	so read by the filter are processed according to K&R rules for 
 *	the backslash so that the string \\fP becomes \fP.  Notice that K&R
 *	recognizes \f as formfeed (ascii octal 14), so that without the second
 *	\ in \\fP the result becomes <FF>P.
 */
#include <stdio.h>
#include <ctype.h>
#include <strings.h>

#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif

#define EXIT_BADARGS	1
#define EXIT_NOTEMP		2
#define EXIT_NOCLOSE	3

#define STREQ(s,t)	(strcmp((s),(t))==0)

static char *	process_string();
static char		octal_char();

#ifdef STANDALONE
main(ac, av)
#else
int insert_brackets_main(ac, av)
#endif STANDALONE
	int 	ac;
	char	**av;
{
	char	*temp = "/tmp/insXXXXXX";
	FILE	*tfp, *fopen();
	int	c;
	char	*left, *right;
	int		len1, len2;
	char	*head, *tail, *malloc(), *mktemp();
	char	*h, *t;
	long	flen, ftell();
	
	if (ac != 3) {
		(void)fprintf(stderr, "Usage: %s leftbracket rightbracket\n", av[0]);
		while ((c = getchar()) != EOF)
			putchar(c);
		
		EXIT(EXIT_BADARGS);
	}

	left = process_string(av[1]);
	right = process_string(av[2]);
	len1 = strlen(left);
	len2 = strlen(right);

	head = malloc((unsigned)(len1 + 1));
	tail = malloc((unsigned)(len2 + 1));

	/* Try to create the temporary file; if failure then just copy
	 * input to output
	 */
	(void)mktemp(temp);
	if ((tfp = fopen(temp, "w+")) == NULL) {
		(void)fprintf(stderr, "%s: can't open /tmp file\n", av[0]);
		while ((c =  getchar()) != EOF)
			putchar(c);

		EXIT(EXIT_NOTEMP);
	}

	/* Scan through the data extracting head and tail strings. */
	flen = 0L;
	h = head;
	t = tail;
	while ((c = getchar()) != EOF) {
		flen += 1L;

		if (h - head < len1)
			*h++ = c;

		if (t - tail < len2) {
			*t++ = c;
		} else {
			(void)strcpy(tail, tail + 1);
			tail[len2 - 1] = c;
		}

		putc(c, tfp);
				
	}

	*h = *t = '\0';

	/* If left matches head and right matches tail then copy the temporary
	 * removing head and tail parts, otherwise, write everything to stdout.
	 */
	if (STREQ(left, head) && STREQ(right, tail)
			&& flen >= (long) (len1 + len2)) {
		(void)fseek(tfp, (long) len1, 0);
		while (ftell(tfp) < flen - (long) len2) {
			c = getc(tfp);
			putchar(c);
		}
		
	} else {
		(void)fseek(tfp, 0L, 0);
		fputs(left, stdout);
		while ((c = getc(tfp)) != EOF)
			putchar(c);
		fputs(right, stdout);
	}

	/* Cleanup the temporary file */
	if (unlink(temp) != 0) {
		(void)fprintf(stderr, "%s: couldn't delete /tmp file\n", av[0]);
		EXIT(EXIT_NOCLOSE);
	}

	EXIT(0);

}

/*
 * 	process_string(s) -- returns string s with K&R backslash conventions
 *		applied.  Note that s is not overwritten; a new string is returned.
 */
static char *
process_string(source)
	char	*source;
{
	char	*dest, *malloc();
	char	*t, *u;

	dest = malloc((unsigned)(strlen(source) + 1));

	for (t = source, u = dest; *t ; t++, u++ ) {
		/* The following treatment of backslashed characters is due to
		 * K&R, p. 180-1.
		 */
		if (*t == '\\')
			switch (*(++t)) {
			case 0:
				t--;
				break;
			case 'n':
				*u = '\n';
				break;
			case 't':
				*u = '\t';
				break;
			case 'b':
				*u = '\b';
				break;
			case 'r':
				*u = '\r';
				break;
			case 'f':
				*u = '\f';
				break;
			case '\\':
				*u = '\\';
				break;
			case '\'':
				*u = '\'';
				break;
			case '0':  case '1':  case '2':  case '3':  case '4':  
			case '5':  case '6':  case '7': 
				*u = octal_char(&t);
				t--;
				break;
			default:
				*u = *t;
				break;
			}
		else
			*u = *t;

	}

	*u = '\0';
	return (dest);

}

/*
 *	octal_char(s) -- return the char value represented by the string
 *		pointer pointed to by s.  The double indirection is used to 
 *		consume characters in the string.  Note that on return the
 *		string pointer points one beyond the end of the octal sequence.
 *		An octal sequence is defined by K&R p. 180-1; it is 1 to 3
 *		octal digits.
 */
static char
octal_char(str)
	char	**str;
{
	register int	i;
	char	v;

	v = 0;
	for (i = 0; i < 3 && isdigit(**str) && **str <= '7'; i++, (*str)++)
		v = 8 * v + (**str - '0');

	return (v);

}
	
			
				
				
