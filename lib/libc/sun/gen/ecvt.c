#ifdef sccsid
static	char sccsid[] = "@(#)ecvt.c 1.1 86/09/24 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "unpkd.h"

/*
 * some day these will be functions to determine the current floating
 * point mode.
 */
#define WARNING 0 /* no warning on underflow */
#define NEAREST 1 /* round to nearest        */
#define NDIG	100
union doo{
    double d;
    struct {
	unsigned
	    sign :1,
	    exp  :11,
	    mantissa1:(32-11-1),
	    mantissa2;
    } bah;
};

static int
ilog( up )
    struct unpkd *up;
{
    /* guess floor(log10( n )) */
    int n;
    n = (up->exp << 4) | (( up->mantissa[0] >> 27) & 0xf );
    return (n*LTWO10) >> 16 ;
}

static void
sshift( charstring, shiftcount, nchar )
    register char *charstring;
    register shiftcount, nchar;
{
    register char * source, *dest;
    source = charstring+nchar-1;
    dest = source+shiftcount;
    while (nchar--){
	*dest-- = *source--;
    }
    while (dest>=charstring)
	*dest-- = '0';
}

static char *
cvt( d, n, decpt, sign, eflag )
    double d;
    int n, eflag;
    int *decpt, *sign;
{
    int log, lambda;
    short iscale;
    int junk, note;
    char * zeros, * _qtoa();
    union doo ff;
    struct unpkd  x, y,  scale;
    static struct quad qpot[] =  {
	{ 0, 1 },
	{ 0, 10 },
	{ 0, 100 },
	{ 0, 1000 },
	{ 0, 10000 },
	{ 0, 100000 },
	{ 0, 1000000 },
	{ 0, 10000000 },
	{ 0, 100000000 },
	{ 0, 1000000000 },
	{ 2, 1410065408 },
	{ 23, 1215752192 },
	{ 232, 3567587328 },
	{ 2328, 1316134912 },
	{ 23283, 276447232 },
	{ 232830, 2764472320 },
	{ 2328306, 1874919424 },
	{ 23283064, 1569325056 },
    };
    static char mystring[ NDIG];

    ff.d = d;
    if (!d){
	/* special case zero */
	zeros = mystring;
	if (n >= NDIG) n = NDIG - 1;
	while (n--)
	    *zeros++ = '0';
	*decpt = *sign = 0;
	return mystring;
    }
    *sign = ff.bah.sign ;
    if (ff.bah.exp){
	/* unpack usual number */
	x.mantissa[0] = HOB|(ff.bah.mantissa1<<11)|(ff.bah.mantissa2>>21);
	x.mantissa[1] = ff.bah.mantissa2<<11;
	x.exp = ff.bah.exp-DOUBLEBIAS;
    } else {
	/* unpack unusual number */
	x.mantissa[0] = (ff.bah.mantissa1<<12)|(ff.bah.mantissa2>>20);
	x.mantissa[1] = ff.bah.mantissa2<<12;
	x.exp = -DOUBLEBIAS;
	if (!WARNING){
	    /*
	     * special wack case: if in warning mode (i.e. never )
	     * print out denormalized numbers as denormalized decimals
	     */
	    if ((x.mantissa[0]&HOB) == 0){
		/* normalize unnormalized number */
		_unorm( &x );
	    }
	}
    }
    log = ilog( &x );
    /* for f-format, we want n digits to the RIGHT of the decimal */
    /* for e-format, we want n digits TOTAL */
    junk = 0;
    if (!eflag){
	if (log >= 0)
	    n += log+1;
    }
tryagain:
    if ( n > 17 ){
	/* we really only deal with 17 places */
	junk += n -17;
	n = 17;
    }
    /* want to turn number into an integer: multiply by 10^iscale */
    /* compute 10^iscale in unpacked extended format 		  */
    iscale = n-log-1;
    if (iscale<0){
	iscale = -iscale;
	_umult3( &_d_r_pot[iscale%N_D_POT], &_d_r_big_pot[iscale/N_D_POT], &scale );
    } else {
	_umult3( &_d_pot[iscale%N_D_POT], &_d_big_pot[iscale/N_D_POT], &scale );
    }
    /* multiply out */
    _umult3( &scale, &x, &y );
    /* extract integer part of extended, unpacked number */
    _uentier( &y );
    /* make sure our log guess was right */
    switch  (_qcmp( y.mantissa, &qpot[ n ])){
    case 1: /* too big */
	log++;
	if (!eflag && log>0) n++;
	goto tryagain;
    case 0 : /* equal */
	log++;
	if (n > 0){
	note = 1;
	if (!eflag) junk++;
	}
	else note = 0 ;
	break;
    default:
	note = 0;
    }
    *decpt = log+1;
    /* print it */
    zeros = _qtoa( y.mantissa , mystring );
    zeros -= note;
    if (zeros < mystring+n){
	/* this is a WARNING: pad with leading zeros */
	sshift( mystring, mystring+n-zeros, zeros-mystring);
	zeros = mystring+n;
    }
    if (junk+n >= NDIG) junk = NDIG - 1 - n;
    while (junk--)
	*zeros++ = '0';
    *zeros = '\0';
    return mystring;
}

char *
ecvt( arg, ndigits, decpt, sign )
    double arg;
    int ndigits, *decpt, *sign;
{
    return cvt( arg, ndigits, decpt, sign, 1);
}

char *
fcvt( arg, ndigits, decpt, sign )
    double arg;
    int ndigits, *decpt, *sign;
{
    register char *stuff, *digit;
    int d;
    stuff =  cvt( arg, ndigits, decpt, sign, 0);
    if (*decpt < 0 ){
	/* yeucch */
	d =  *decpt;
	if ( -d >ndigits){
	    /* none left */
	    ndigits = 0;
	    /* *decpt -= 1; */
	} else {
	    ndigits += d;
	    if (stuff[ndigits] >= '5'){
		/* double yeucch */
		digit = &stuff[ndigits-1];
		while (digit >= stuff){
		    if ((*digit +=1) <= '9')
			break;
		    *digit-- = '0';
		}
		if (digit < stuff){
		    stuff[0] = '1';
		    *decpt += 1;
		    ndigits = 1;
		}
	    }
	}
	stuff[ndigits] = '\0' ;
    }
    return stuff;
}
