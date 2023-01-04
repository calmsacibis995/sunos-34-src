/*
 * @(#)printf.c 2.6 84/01/05 Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Scaled down version of C Library printf.
 * Only %s %d %x %c are recognized.
 */
/*VARARGS0*/
printf(fmt, x1)
register unsigned char *fmt;
unsigned x1;
{
	register unsigned c;
	register unsigned int *adx;
	register unsigned char *s;

	adx = &x1;
	while ((c = *fmt++) != '\0') {
		if (c != '%')
			putchar(c);
		else {
			c = *fmt++;
			switch (c) {

			case 'c':
				c = (unsigned char)*adx;
				putchar(c);
				break;

			case 's':
				s = (unsigned char *)*adx;
				while (c = *s++)
					putchar(c);
				break;

			case 'd':
				printn((unsigned long)*adx, 10);
				break;

			case 'x':
				printn((unsigned long)*adx, 16);
				break;

			}
			adx++;
		}
	}
}

/*
 * Print an unsigned integer in base b.
 */
printn(n, b)
unsigned long n;
unsigned long b;
{
	unsigned long a;

	if(a = n/b) {
		printn(a, b);
		/*
		 * The following 4 lines are a kludge which cause inline
		 * multiplies to be used.  Compiler won't generate inline
		 * multiply except by constants (hack) even though it's
		 * exxactly the same to multiply by a random unsigned short.
		 * When there's no need to kludge, replace by:
		 * 	n -= a * (unsigned short)b;
		 */
		if (16 == (char) b) 
			n -= a * 16;
		else
			n -= a * 10;
	}
	putchar((unsigned char)"0123456789ABCDEF"[n]);
}
