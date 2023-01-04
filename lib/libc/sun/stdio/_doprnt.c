#ifndef lint
static	char sccsid[] = "@(#)_doprnt.c 1.1 86/09/24 SMI";
#endif

/*
 *	_doprnt: common code for printf, fprintf, sprintf
 *	Floating-point code is included or not, depending
 *	on whether the preprocessor variable FLOAT is 1 or 0.
 */
#ifndef FLOAT
#define	FLOAT	1	/* YES! we want floating */
#endif

#include <stdio.h>
#include <ctype.h>
#include <varargs.h>
#include "../param.h"
#include "../../stdio/stdiom.h"

#define max(a,b)	((a) > (b) ? (a) : (b))
#define min(a,b)	((a) < (b) ? (a) : (b))

/* If this symbol is nonzero, allow '0' as a flag */
/* If this symbol is nonzero, allow '0' as a flag */
#define FZERO 1

#if FLOAT
/*
 *	System-supplied routines for floating conversion
 */
char *fcvt();
char *ecvt();
#endif

char *_utoa();	/* fast integer=>ascii decimal conversion */

#define emitchar(c)   { if (--filecnt < 0) { \
				register FILE *iop = file; \
				if (((iop->_flag & (_IOLBF|_IONBF)) == 0 \
				    || -filecnt >= iop->_bufsiz)) { \
					iop->_ptr = fileptr; \
					(void) _xflsbuf(iop); \
					fileptr = iop->_ptr; \
					filecnt = iop->_cnt; \
					filecnt--; \
				    } \
			} \
			*fileptr++ = (unsigned)(c); \
			count++; \
                      }

static char nullstr[] = "(null)";
static char lowerhex[] = "0123456789abcdef";
static char upperhex[] = "0123456789ABCDEF";

_doprnt(format, args, file)
	char *format;
	va_list args;
	FILE *file;
{
	/* Current position in format */
	register char *cp;

	/* Starting and ending points for value to be printed */
	register char *bp;
	char *p;

	/* Pointer and count for I/O buffer */
	register unsigned char *fileptr;
	register int filecnt;

	/* Field width and precision */
	int width;
	register int prec;

	/* Format code */
	char fcode;

	/* Number of padding zeroes required on the left */
	int lzero;

	/* Flags - nonzero if corresponding character appears in format */
	bool fplus;		/* + */
	bool fminus;		/* - */
	bool fblank;		/* blank */
	bool fsharp;		/* # */
#if FZERO
	bool fzero;		/* 0 */
#endif

	/* Pointer to sign, "0x", "0X", or empty */
	char *prefix;
#if FLOAT
	/* Exponent or empty */
	char *suffix;

	/* Buffer to create exponent */
	char expbuf[MAXESIZ + 2];  /* "e+xxx\0" */

	/* Number of padding zeroes required on the right */
	int rzero;

	/* The value being converted, if real */
	double dval;

	/* Output values from fcvt and ecvt */
	int decpt, sign;

	/* Scratch */
	int k;
	int nblank;

	/* Values are developed in this buffer */
	char buf[max(MAXDIGS, max(MAXFCVT + MAXEXP, MAXECVT) + 1)];
#else
	char buf[MAXDIGS];
#endif
	/* The value being converted, if integer */
	register unsigned long val;

	/* Work variables */
	register int n;
	register char c;
	char radix;

	/* count of output characters */
	register int count;

	cp = format;
	if ((c = *cp++) != '\0') {
		/*
		 * We know we're going to write something; make sure
		 * we can write and set up buffers, etc..
		 */
		if (_WRTCHK(file))
			return(EOF);
	} else
		return(0);	/* no fault, no error */

	count = 0;
	fileptr = file->_ptr;
	filecnt = file->_cnt;

	/*
	 *	The main loop -- this loop goes through one iteration
	 *	for each ordinary character or format specification.
	 */
	do {
		if (c != '%') {
			/* Ordinary (non-%) character */
			emitchar(c);
		} else {
			/*
			 *	% has been spotted!
			 *
			 *	First, try the 99% cases.
			 *	then parse the format specification.
			 *
			 *	Note that this code assumes the Sun
			 *	Workstation environment (all params
			 *	passed as int == long, no interrupts
			 *	for fixed point overflow from negating
			 *	the most negative number).
			 */
		skipit:
			switch(c = *cp++) {

			case 'l':
			case 'h':
				/* Quickly ignore long & short specifiers */
				goto skipit;

			case 's':
				bp = va_arg(args, char *);
				if (bp == NULL)
					bp = nullstr;
				while (c = *bp++)
					emitchar(c);
				continue;

			case 'c':
				c = va_arg(args, int);
			emitc:
				emitchar(c);
				continue;

			case 'd':
			case 'D':
				val = va_arg(args, int);
				if ((long) val < 0) {
					emitchar('-');
					val = -val;
				}
				goto udcommon;

			case 'U':
			case 'u':
				val = va_arg(args, unsigned);
			udcommon:
				(void)_utoa(val, buf);
				bp = buf;
				while (c = *bp++) {
					emitchar(c);
				}
				continue;

			case 'X':
				{
				register char *stringp = upperhex;
				val = va_arg(args, unsigned);
				bp = buf + MAXDIGS;
				if (val == 0)
					goto zero;
				while (val) {
					*--bp = stringp[val%16];
					val /= 16;
				}
				}
				goto intout;

			case 'x':
				{
				register char *stringp = lowerhex;
				val = va_arg(args, unsigned);
				bp = buf + MAXDIGS;
				if (val == 0)
					goto zero;
				while (val) {
					*--bp = stringp[val%16];
					val /= 16;
				}
				}
				goto intout;

			case 'O':
			case 'o':
				{
				register char *stringp = lowerhex;
				val = va_arg(args, unsigned);
				bp = buf + MAXDIGS;
				if (val == 0)
					goto zero;
				while (val) {
					*--bp = stringp[val%8];
					val /= 8;
				}
				}
				/* Common code to output integers */
			intout:
				p = buf + MAXDIGS;
				while (bp < p) {
					c = *bp++;
					emitchar(c);
				}
				continue;

			zero:
				c = '0';
				goto emitc;

			default:
				/*
				 * let AT&T deal with it
				 */
				cp-= 2;
			}

			/* Scan the <flags> */
			fplus = 0;
			fminus = 0;
			fblank = 0;
			fsharp = 0;
#if FZERO
			fzero = 0;
#endif
		scan:	switch (*++cp) {
			case '+':
				fplus = 1;
				goto scan;
			case '-':
				fminus = 1;
				goto scan;
			case ' ':
				fblank = 1;
				goto scan;
			case '#':
				fsharp = 1;
				goto scan;
#if FZERO
			case '0':
				fzero = 1;
				goto scan;
#endif
			}

			/* Scan the field width */
			if (*cp == '*') {
				width = va_arg(args, int);
				if (width < 0) {
					width = -width;
					fminus = 1;
				}
				cp++;
			} else {
				width = 0;
				while (isdigit(*cp)) {
					n = tonumber(*cp++);
					width = width * 10 + n;
				}
			}

			/* Scan the precision */
			if (*cp == '.') {

				/* '*' instead of digits? */
				if (*++cp == '*') {
					prec = va_arg(args, int);
					cp++;
				} else {
					prec = 0;
					while (isdigit(*cp)) {
						n = tonumber(*cp++);
						prec = prec * 10 + n;
					}
				}
			} else
				prec = -1;

			/*
			 *	The character addressed by cp must be the
			 *	format letter -- there is nothing left for
			 *	it to be.
			 *
			 *	The status of the +, -, #, blank, and 0
			 *	flags are reflected in the variables
			 *	"fplus", "fminus", "fsharp", "fblank",
			 *	and "fzero", respectively.
			 *	"width" and "prec" contain numbers
			 *	corresponding to the digit strings
			 *	before and after the decimal point,
			 *	respectively. If there was no decimal
			 *	point, "prec" is -1.
			 *
			 *	The following switch sets things up
			 *	for printing.  What ultimately gets
			 *	printed will be padding blanks, a prefix,
			 *	left padding zeroes, a value, right padding
			 *	zeroes, a suffix, and more padding
			 *	blanks.  Padding blanks will not appear
			 *	simultaneously on both the left and the
			 *	right.  Each case in this switch will
			 *	compute the value, and leave in several
			 *	variables the information necessary to
			 *	construct what is to be printed.
			 *
			 *	The prefix is a sign, a blank, "0x", "0X",
			 *	or null, and is addressed by "prefix".
			 *
			 *	The suffix is either null or an exponent,
			 *	and is addressed by "suffix".
			 *
			 *	The value to be printed starts at "bp"
			 *	and continues up to and not including "p".
			 *
			 *	"lzero" and "rzero" will contain the number
			 *	of padding zeroes required on the left
			 *	and right, respectively.  If either of
			 *	these variables is negative, it will be
			 *	treated as if it were zero.
			 *
			 *	The number of padding blanks, and whether
			 *	they go on the left or the right, will be
			 *	computed on exit from the switch.
			 */
			
			lzero = 0;
			prefix = "";
#if FLOAT
			rzero = 0;
			suffix = prefix;
#endif
		next:
			switch (fcode = *cp++) {

			/* toss the length modifier, if any */
			case 'l':
			case 'h':
				goto next;

			/*
			 *	fixed point representations
			 *
			 *	"radix" is the radix for the conversion.
			 *	Conversion is unsigned unless fcode is 'd'.
			 *	We assume a 2's complement machine and
			 *	that fixed point overflow (from negating
			 *	the largest negative int) is ignored.
			 */

			case 'D':
			case 'U':
			case 'd':
			case 'u':
				radix = 10;
				goto fixed;

			case 'O':
			case 'o':
				radix = 8;
				goto fixed;

			case 'X':
			case 'x':
				radix = 16;

			fixed:
				/* Establish default precision */
				if (prec < 0)
					prec = 1;

				/* Fetch the argument to be printed */
				val = va_arg(args, unsigned);

				/* If signed conversion, establish sign */
				if (fcode == 'd' || fcode == 'D') {
					if ((long) val < 0) {
						prefix = "-";
						val = -val;
					} else if (fplus)
						prefix = "+";
					else if (fblank)
						prefix = " ";
				}
#if FZERO
				if (fzero) {
					n = width - strlen(prefix);
					if (n > prec)
						prec = n;
				}
#endif
				/* Set translate table for digits */
				{
				register char *stringp;
				if (fcode == 'X')
					stringp = upperhex;
				else
					stringp = lowerhex;

				/* Develop the digits of the value */
				bp = buf + MAXDIGS;
				switch(radix) {
				case 8:	/*octal*/
					while (val) {
						*--bp = stringp[val%8];
						val /= 8;
					}
					break;
				case 16:/*hex*/
					while (val) {
						*--bp = stringp[val%16];
						val /= 16;
					}
					break;
				default:
					while (val) {
						*--bp = stringp[val%10];
						val /= 10;
					}
					break;
				} /* switch */
				}

				/* Calculate padding zero requirement */
				p = buf + MAXDIGS;
				lzero = bp - p + prec;

				/* Handle the # flag */
				if (fsharp && bp != p) {
					switch (fcode) {
					case 'o':
						if (lzero < 1)
							lzero = 1;
						break;
					case 'x':
						prefix = "0x";
						break;
					case 'X':
						prefix = "0X";
						break;
					}
				}
				break;
#if FLOAT
			case 'E':
			case 'e':
				/*
				 *	E-format.  The general strategy
				 *	here is fairly easy: we take
				 *	what ecvt gives us and re-format it.
				 */

				/* Establish default precision */
				if (prec < 0)
					prec = 6;

				/* Fetch the value */
				dval = va_arg(args, double);

				/* test for nasty ieee indeterminate forms */
				if (isinf(dval))
					goto ieee_infinity;
				if (isnan(dval))
					goto ieee_nan;

				/* Develop the mantissa */
				bp = ecvt(dval, min(prec + 1, MAXECVT),
				    &decpt, &sign);

				/* Determine the prefix */
			e_merge:
				if (sign)
					prefix = "-";
				else if (fplus)
					prefix = "+";
				else if (fblank)
					prefix = " ";

				{
				register char *stringp;
				/* Place the first digit in the buffer */
				stringp = &buf[0];
				*stringp++ = *bp != '\0'? *bp++: '0';

				/* Put in a decimal point if needed */
				if (prec != 0 || fsharp)
					*stringp++ = '.';

				/* Create the rest of the mantissa */
				rzero = prec;
				while (rzero > 0 && *bp!= '\0') {
					--rzero;
					*stringp++ = *bp++;
				}
				p = stringp;
				}

				bp = &buf[0];

				/* Create the exponent */
				suffix = &expbuf[MAXESIZ+2];
				*--suffix = '\0';
				if (dval) {
					n = decpt - 1;
					if (n < 0)
						n = -n;
					if (n < 100) {
						/*
						 * the manual says 2 digits.
						 * MAXESIZ+1 allows for 3.
						 * If we don't need a 3rd
						 * digit, back up by one.
						 */
						*--suffix = '\0';
					}
					while (n != 0) {
						*--suffix = todigit(n % 10);
						n /= 10;
					}
				} else {
					/* zero exponent; back up 1 digit */
					*--suffix = '\0';
				}
				/* Prepend leading zeroes to the exponent */
				while (suffix > &expbuf[2])
					*--suffix = '0';

				/* Put in the exponent sign */
				*--suffix = (decpt > 0 || !dval )? '+': '-';

				/* Put in the e; note kludge in 'g' format */
				*--suffix = fcode;

				break;

			case 'f':
				/*
				 *	F-format floating point.  This is
				 *	a good deal less simple than E-format.
				 *	The overall strategy will be to call
				 *	fcvt, reformat its result into buf,
				 *	and calculate how many trailing
				 *	zeroes will be required.  There will
				 *	never be any leading zeroes needed.
				 */

				/* Establish default precision */
				if (prec < 0)
					prec = 6;

				/* Fetch the value */
				dval = va_arg(args, double);

				/* test for nasty ieee indeterminate forms */
				if (isinf(dval))
					goto ieee_infinity;
				if (isnan(dval))
					goto ieee_nan;

				/* Do the conversion */
				bp = fcvt(dval, min(prec, MAXFCVT),
				    &decpt, &sign);

				/* Determine the prefix */
			f_merge:
				if (sign && decpt > -prec &&
				    *bp != '\0' && *bp != '0')
					prefix = "-";
				else if (fplus)
					prefix = "+";
				else if (fblank)
					prefix = " ";

				{
				register char *stringp;
				/* Initialize buffer pointer */
				stringp = &buf[0];

				/* Emit the digits before the decimal point */
				n = decpt;
				k = 0;
				if (n <= 0)
					*stringp++ = '0';
				else
					do
						if (*bp == '\0' || k >= MAXFSIG)
							*stringp++ = '0';
						else {
							*stringp++ = *bp++;
							++k;
						}
					while (--n != 0);

				/* Decide whether we need a decimal point */
				if (fsharp || prec > 0)
					*stringp++ = '.';

				/* Digits(if any) after the decimal point */
				n = min(prec, MAXFCVT);
				rzero = prec - n;
				while (--n >= 0) {
					if (++decpt <= 0 || *bp == '\0' ||
					    k >= MAXFSIG)
						*stringp++ = '0';
					else {
						*stringp++ = *bp++;
						++k;
					}
				}
#if FZERO
				if (fzero)
					/* Calculate padding zero requirement */
					lzero = width - (strlen(prefix)
					    + (stringp - buf) + rzero);
#endif
				p = stringp;
				}

				bp = &buf[0];
				break;

			case 'G':
			case 'g':
				/*
				 *	g-format.  We play around a bit
				 *	and then jump into e or f, as needed.
				 */
			
				/* Establish default precision */
				if (prec < 0)
					prec = 6;
				else if (prec == 0)
					prec = 1;

				/* Fetch the value */
				dval = va_arg(args, double);

				/* test for nasty ieee indeterminate forms */
				if (isinf(dval))
					goto ieee_infinity;
				if (isnan(dval))
					goto ieee_nan;

				/* Do the conversion */
				bp = ecvt(dval, min(prec, MAXECVT),
				    &decpt, &sign);
				if (!dval)
					decpt = 1;

				k = prec;
				if (!fsharp) {
					n = strlen(bp);
					if (n < k)
						k = n;
					while (k >= 1 && bp[k-1] == '0')
						--k;
				}
					
				if (decpt < -3 || decpt > prec) {
					prec = k - 1;
					fcode += 'e' - 'g';  /* 'e' or 'E' */
					goto e_merge;
				} else {
					prec = k - decpt;
					goto f_merge;
				}

				/* come here to handle all floating indeterminates */
ieee_infinity:
				bp = "Infinity";
				p  = bp + 8;
				break;
ieee_nan:
				bp = "NaN";
				p  = bp + 3;
				break;

#endif
			case 'c':
				buf[0] = va_arg(args, int);
				bp = &buf[0];
				p = bp + 1;
				break;

			case 's':
				bp = va_arg(args, char *);
				if (prec < 0)
					prec = MAXINT;
				/* avoid *(0) */
				if (bp == NULL)
					bp = nullstr;
				for (n=0; *bp++ != '\0' && n < prec; n++)
					;
				p = --bp;
				bp -= n;
				break;

			case '\0':
				/* well, what's the punch line? */
				goto out;

		/*	case '%':	*/
			default:
				p = bp = &fcode;
				p++;
				break;

			}
			/* Calculate number of padding blanks */
			nblank = width
#if FLOAT
				- (rzero < 0? 0: rzero)
				- strlen(suffix)
#endif
				- (p - bp)
				- (lzero < 0? 0: lzero)
				- strlen(prefix);

			/* Blanks on left if required */
			if (!fminus)
				while (--nblank >= 0)
					emitchar(' ');

			/* Prefix, if any */
			while (*prefix != '\0') {
				emitchar(*prefix);
				prefix++;
			}

			/* Zeroes on the left */
			while (--lzero >= 0)
				emitchar('0');
			
			/* The value itself */
			while (bp < p) {
				emitchar(*bp);
				bp++;
			}
#if FLOAT
			/* Zeroes on the right */
			while (--rzero >= 0)
				emitchar('0');

			/* The suffix */
			while (*suffix != '\0') {
				emitchar(*suffix);
				suffix++;
			}
#endif
			/* Blanks on the right if required */
			if (fminus)
				while (--nblank >= 0)
					emitchar(' ');
		} /* else */
	} while ((c = *cp++) != '\0');	/* do */
out:
	file->_ptr = fileptr;
	file->_cnt = filecnt;
	if (file->_flag & (_IONBF | _IOLBF) &&
	    (file->_flag & _IONBF ||
	     memchr((char *)file->_base, '\n', fileptr - file->_base) != NULL))
		(void) _xflsbuf(file);
	return (ferror(file)? EOF: count);
}
