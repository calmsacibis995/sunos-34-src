#ifndef lint
static	char sccsid[] = "@(#)oatof.c 1.1 86/09/24 SMI"; /* from UCB X.X XX/XX/XX */
#endif

/*
 *	atof:
 *	ascii to floating conversion
 */

#include <ctype.h>
#define NEXP 9

static double	twoe56	= 72057594037927936.;   /*2^56*/
static double	exp5[]	= {5.,25.,625.,390625., /* 5^1, 5^2, 5^4, 5^8 */
		152587890625.,			/* 5^16 */
		23283064365386962890625.,	/* 5^32 */
		542101086242752217003726400434970855712890625., /* 5^64 */
		293873587705571876992184134305561419454666389193021880377187926569604314863681793212890625.,			/* 5^128 */
		86361685550944446253863518628003995711160003644362813850237034701685918031624270579715075034722882265605472939461496635969950989468319466936530037770580747746862471103668212890625.
						/* 5^256 */
		};

double atof(p) register char *p;
{
	extern double ldexp();
	register c, exp = 0, eexp = 0;
	double fl = 0, flexp = 1.0;
	int bexp, neg = 1, negexp = 1;

	while((c = *p++) == ' ');
	if (c == '-') neg = -1;	else if (c == '+'); else --p;

	while ((c = *p++), isdigit(c))
		if (fl < twoe56) fl = 10*fl + (c-'0'); else exp++;
	if (c == '.')
	while ((c = *p++), isdigit(c))
		if (fl < twoe56)
		{
			fl = 10*fl + (c-'0');
			exp--;
		}
	if ((c == 'E') || (c == 'e'))
	{
		if ((c= *p++) == '+'); else if (c=='-') negexp = -1; else --p;
		while ((c = *p++), isdigit(c)) eexp = 10*eexp + (c-'0');
		if (negexp < 0) eexp = -eexp; exp += eexp;
	}
	bexp = exp;
	if (exp < 0) exp = -exp;

	for (c = 0; c < NEXP; c++)
	{
		if (exp & 01) flexp *= exp5[c];
		exp >>= 1; if (exp == 0) break;
	}

	if (bexp < 0) fl /= flexp; else fl *= flexp;
	fl = ldexp(fl, bexp);
	if (neg < 0) return(-fl); else return(fl);
}
