#include <sys/types.h>
#include <ctype.h>

static char	sccsid[] = "@(#)prf.c 1.1 9/25/86 Copyright Sun Micro";
int		prompt_base = 16;

/*
 *	printf and input routines for the diagnostic environment
 *	printf knows about:
 *		l    - ignored
 *		d    - base 10, signed
 *		u    - base 10, unsigned
 *		x    - base 16, unsigned
 *		o    - base 8,  unsigned
 *		b    - binary,  unsigned
 *		s    - string
 *		c    - char
 *
 *	in addition, there are the following for input:
 *		ret = promptfor(char *, int)
 *			- printf string with int as argument, get
 *			a number with ktoi, or return the int if only
 *			hit return. Uses prompt_base as default base.
 *		ret = ktoi(char *, base)
 *			- parse the string using base as default, base
 *			is overridden by prefixing with 0x for 16,
 *			0t for decimal, or 0 for octal. recognizes
 *			a prefix minus sign and behaves accordingly.
 *	for posterity
 *		getn()
 *			- get a decimal number (uses ktoi)
 *		gethn()
 *			- get a hexadecimal number (uses ktoi)
 */

printf(fmt,arglist)
register char		*fmt;
u_long			arglist;
{
        register char		c, *s;
	register u_long		*args = &arglist;

	for(;;){

		/*
		 * Move the pointer along through the string.  Stop when
		 * you get to a '%' character.
		 */
		while((c = *fmt++) != '%') {
			if(c)
				putchar(c);

			else 	/* it's a zero, so end of string */
				return;
		}

		/*
		 * To get here, ran accross a '%' char.  What follows
		 * that?
		 */
		c = *fmt++;
		if (c == 'l')
			c = *fmt++;	/* just keep going */

		if (c == 'x' || c == 'd' || c == 'u' || c == 'o' || c == 'b'){

			printn((u_long)*args++, (c == 'x') ? 16 :
				((c == 'o') ? 8 :
				((c == 'b') ? 2 :
				((c == 'u') ? 10 : -10))));

		} else if (c == 's') {
			s = (char *) *args++;
			while(*s)
				putchar(*s++);

		} else if (c == 'c') {
			putchar(*args++);

		} else {
			putchar('%');
			putchar(*(fmt -1));
		}
	}
}


/*
 * Print an integer in base b. Negative base indicates signed.
 */
printn(n, b)
register u_long			n;
{
	char prbuf[33];
	register char *cp = prbuf;

	if(b < 0){
		b = -b;
		if((int)n < 0){
			putchar('-');
			n = (unsigned long)(-(int)n);
		}
	}
	do {
		*cp++ = "0123456789abcdef"[n%b];
		n /= b;
	} while (n);
	do
		putchar(*--cp);
	while(cp > prbuf);
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *				SPRINTN
 * Move an integer in base b to named string. Negative base indicates signed.
 */
sprintn(string, n, b)
char	*string;
register u_long
	n;
int	b;
{
	char	prbuf[33];
	register char
		*cp = prbuf;

	if(b < 0){
		b = -b;
		if((int)n < 0){
			*string++ = '-';
			n = (unsigned long)(-(int)n);
		}
	}
	do {
		*cp++ = "0123456789abcdef"[n%b];
		n /= b;
	} while (n);
	do
		*string++ = *--cp;
	while(cp > prbuf);
}




getn() {
	char	type[40];

	gets(type);
	return(ktoi(type, 10));
}


getnh() {
	char	type[40];

	gets(type);
	return(ktoi(type, 16));
}

promptfor(string, def)
char	*string;
int	def;
{
	char	type[40];

	printf(string, def);
	gets(type);
	if (*type)
		return(ktoi(type, prompt_base));
	else
		return(def);
}

static char digits[] =  "0123456789abcdef";
ktoi(p,base)
register char	*p;
register 	base;
{
	register	digit, ret = 0, sign = 0, not = 0;


	if (!strncmp(p,"-", 1)){
		sign = 1;
		++p;
	} else if (!strncmp(p,"~", 1)){
		not = 1;
		++p;
	}

	if (!strncmp(p,"0x",2)){
		base = 16;
		p += 2;
	}else if (!strncmp(p,"0t",2)){
		base = 10;
		p += 2;
	} else if (!strncmp(p,"0",1)){
		base = 8;
		++p;
	}

	for(;;){
		if (*p && *p != '\n' && (digit = index(&digits[0],
			(isupper(*p) ? tolower(*p) : *p)))){
			ret = ret*base + (digit - (int)&digits[0]);
			++p;
		}else{
			if (sign)
				return(-ret);
			else if (not)
				return(~ret);
			else
				return(ret);
		}
	}
}
