#ifndef lint
static	char sccsid[] = "@(#)doscan.c 1.1 86/09/24 SMI"; /* from S5R2 2.6 */
#endif

/*LINTLIBRARY*/
#include <stdio.h>
#include <ctype.h>
#include <varargs.h>
#include <values.h>

#define NCHARS	(1 << BITSPERBYTE)

extern double atof();
extern char *memset();
extern int ungetc();

int
_doscan(iop, fmt, va_alist)
register FILE *iop;
register unsigned char *fmt;
va_list va_alist;
{
	extern unsigned char *setup();
	char tab[NCHARS];
	register int ch;
	int nmatch = 0, len, inchar, stow, size;

	/*******************************************************
	 * Main loop: reads format to determine a pattern,
	 *		and then goes to read input stream
	 *		in attempt to match the pattern.
	 *******************************************************/
	for( ; ; ) {
		if((ch = *fmt++) == '\0')
			return(nmatch); /* end of format */
		if(isspace(ch)) {
			while(isspace(inchar = getc(iop)))
				;
			if(inchar != EOF && ungetc(inchar, iop) != EOF)
				continue;
			break;
		}
		if(ch != '%' || (ch = *fmt++) == '%') {
			if((inchar = getc(iop)) == ch)
				continue;
			if(inchar != EOF && ungetc(inchar, iop) != EOF)
				return(nmatch); /* failed to match input */
			break;
		}
		if(ch == '*') {
			stow = 0;
			ch = *fmt++;
		} else
			stow = 1;

		for(len = 0; isdigit(ch); ch = *fmt++)
			len = len * 10 + ch - '0';
		if(len == 0)
			len = MAXINT;

		if((size = ch) == 'l' || size == 'h')
			ch = *fmt++;
		if(ch == '\0' ||
		    ch == '[' && (fmt = setup(fmt, tab)) == NULL)
			return(EOF); /* unexpected end of format */
		if(isupper(ch)) { /* no longer documented */
			size = 'l';
			ch = _tolower(ch);
		}
		if((size = (ch == 'c' || ch == 's' || ch == '[') ?
		    string(stow, ch, len, tab, iop, &va_alist) :
		    number(stow, ch, len, size, iop, &va_alist)) != 0)
			nmatch += stow;
		if(va_alist == NULL) /* end of input */
			break;
		if(size == 0)
			return(nmatch); /* failed to match input */
	}
	return(nmatch != 0 ? nmatch : EOF); /* end of input */
}

/***************************************************************
 * Functions to read the input stream in an attempt to match incoming
 * data to the current pattern from the main loop of _doscan().
 ***************************************************************/
static int
number(stow, type, len, size, iop, listp)
int stow, type, len, size;
register FILE *iop;
va_list *listp;
{
	char numbuf[64];
	register char *np = numbuf;
	register int c, base;
	int digitseen = 0, dotseen = 0, expseen = 0, floater = 0, negflg = 0;
	long lcval = 0;

	switch(type) {
	case 'e':
	case 'f':
	case 'g':
		floater++;
	case 'd':
	case 'u':
		base = 10;
		break;
	case 'o':
		base = 8;
		break;
	case 'x':
		base = 16;
		break;
	default:
		return(0); /* unrecognized conversion character */
	}
	while(isspace(c = getc(iop)))
		;
	switch(c) {
	case '-':
		negflg++;
		if(type == 'u')
			break;
	case '+': /* fall-through */
		len--;
		c = getc(iop);
	}
	if(!negflg || type != 'u')
	    for( ; --len >= 0 && np < &numbuf[63]; *np++ = c, c = getc(iop)) {
		if(isdigit(c)) {
			register int digit;
			digit = c - '0';
			if(base == 8) {
				if(digit >= 8)
					break;
				if(stow && !floater)
					lcval = (lcval<<3) + digit;
			} else {
				if(stow && !floater) {
					if(base == 10)
						lcval = (((lcval<<2) + lcval)<<1) + digit;
					else /* base == 16 */
						lcval = (lcval<<4) + digit;
				}
			}
			digitseen++;
			continue;
		} else if (base == 16 && isxdigit(c)) {
			register int digit;
			digit = c - (isupper(c) ? 'A' - 10 : 'a' - 10);
			if(stow && !floater)
				lcval = (lcval<<4) + digit;
			digitseen++;
			continue;
		}
		if(!floater)
			break;
		if(c == '.' && !dotseen++)
			continue;
		if((c == 'e' || c == 'E') && digitseen && !expseen++) {
			*np++ = c;
			c = getc(iop);
			if(isdigit(c) || c == '+' || c == '-')
				continue;
		}
		break;
	    }
	if(stow && digitseen)
		if(floater) {
			register double dval;
	
			*np = '\0';
			dval = atof(numbuf);
			if(negflg)
				dval = -dval;
			if(size == 'l')
				*va_arg(*listp, double *) = dval;
			else
				*va_arg(*listp, float *) = (float)dval;
		} else {
			/* suppress possible overflow on 2's-comp negation */
			if(negflg && lcval != HIBITL)
				lcval = -lcval;
			if(size == 'l')
				*va_arg(*listp, long *) = lcval;
			else if(size == 'h')
				*va_arg(*listp, short *) = (short)lcval;
			else
				*va_arg(*listp, int *) = (int)lcval;
		}
	if(c == EOF || ungetc(c, iop) == EOF)
		*listp = NULL; /* end of input */
	return(digitseen); /* successful match if non-zero */
}

static int
string(stow, type, len, tab, iop, listp)
register int stow, type, len;
register char *tab;
register FILE *iop;
va_list *listp;
{
	register int ch;
	register char *ptr;
	char *start;

	start = ptr = stow ? va_arg(*listp, char *) : NULL;
	if(type == 's') {
		while(isspace(ch = getc(iop)))
			;
		while(ch != EOF && !isspace(ch)) {
			if(stow)
				*ptr = ch;
			ptr++;
			if(--len <= 0)
				break;
			ch = getc(iop);
		}
	} else if(type == 'c') {
		if (len == MAXINT)
			len = 1;
		while((ch = getc(iop)) != EOF) {
			if(stow)
				*ptr = ch;
			ptr++;
			if(--len <= 0)
				break;
		}
	} else { /* type == '[' */
		while((ch = getc(iop)) != EOF && !tab[ch]) {
			if(stow)
				*ptr = ch;
			ptr++;
			if(--len <= 0)
				break;
		}
	}
	if(ch == EOF || (len > 0 && ungetc(ch, iop) == EOF))
		*listp = NULL; /* end of input */
	if(ptr == start)
		return(0); /* no match */
	if(stow && type != 'c')
		*ptr = '\0';
	return(1); /* successful match */
}

static unsigned char *
setup(fmt, tab)
register unsigned char *fmt;
register char *tab;
{
	register int b, c, d, t = 0;

	if(*fmt == '^') {
		t++;
		fmt++;
	}
	(void)memset(tab, !t, NCHARS);
	if((c = *fmt) == ']' || c == '-') { /* first char is special */
		tab[c] = t;
		fmt++;
	}
	while((c = *fmt++) != ']') {
		if(c == '\0')
			return(NULL); /* unexpected end of format */
		if(c == '-' && (d = *fmt) != ']' && (b = fmt[-2]) < d) {
			(void)memset(&tab[b], t, d - b + 1);
			fmt++;
		} else
			tab[c] = t;
	}
	return(fmt);
}
