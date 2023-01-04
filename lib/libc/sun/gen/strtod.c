#ifdef sccsid
static  char sccsid[] = "@(#)strtod.c 1.1 86/09/24 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <errno.h>
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

double
_atod( cp, n, decpt, sign )
    char *cp;
    int n;
    int decpt, sign;
{
    register unsigned v, vv;
    struct quad qv;
    register short i;
    struct unpkd scale, val;
    union doo ff;
    int fudge;
    if (n>17){ 
	n = 17;
	fudge = cp[17] >= '5';
    } else 
	fudge = 0;
    /* collect digits */
    _atoq( cp, n, &qv );
    if (fudge){
	/* qv += fudge */
	if ((qv.q[1] += fudge) == 0)
	     qv.q[0] += 1;
    }
    /* special case */
    if (qv.q[0]==0 && qv.q[1]==0){
zero:
	ff.bah.sign = sign;
	ff.bah.exp = 0;
	ff.bah.mantissa1 = 0;
	ff.bah.mantissa2 = 0;
	return ff.d;
    }
    /* normalize */
    val.mantissa[0] = qv.q[0];
    val.mantissa[1] = qv.q[1];
    val.exp = 63;
    _unorm( &val );
    /* find scale factor */
    i =  n - decpt ;
    if (i<0){
	i = -i;
	if (i> 335) goto infinity;  /* exponent too big */
	_umult3( &_d_pot[i%N_D_POT], &_d_big_pot[i/N_D_POT], &scale );
    } else {
	if (i> 335) 
		{     /* exponent too small */
		errno = ERANGE ;
		goto zero ;
		}
	_umult3( &_d_r_pot[i%N_D_POT], &_d_r_big_pot[i/N_D_POT], &scale );
    }
    /* multiply value by scale factor */
    _umult3( &val, &scale, &val );
    /* pack */
    val.exp += DOUBLEBIAS;
    if (val.exp <= 0){
	/*
	 * We want to denormalize the mantissa by shifting right
	 * (1-exp) places, then rounding. The routine _uentier (written
	 * for use by ecvt() ) will shift right by (63-n) places, then round.
	 * So to use it, we must set the exp field to (63-(1-exp)) or
	 * (exp + 62).
	 */
	val.exp += 62 ;
	_uentier( &val );
	val.exp = 0;
    }
    /* must someday deal with other rounding modes here,  too */
    /* will take sign into account when rounding toward 0, oo */
    v = val.mantissa[0] & ~HOB;
    vv = val.mantissa[1];
    if (NEAREST){
	/* see if addition of rounding fudge factor caused carry-out */
	if ((vv+=(1<<10)) < (1<<10)){
	    /* it did, carry across */
	    if ((++v)&HOB){
		v = 0; /* carryed all the way through -- all bits off now */
		val.exp += 1;
	    }
	}
    }
    /* see if (biased) exponent out of range */
    if (val.exp >= 2047 ){
infinity: /* signed infinity */
	errno = ERANGE ;
	val.exp = 2047;
	v = 0;
	vv = 0;
    }
    ff.bah.exp = val.exp;
    ff.bah.sign = sign;
    ff.bah.mantissa1 = v>>11 ;
    ff.bah.mantissa2 = (v<<21) | (vv>>11);
    return ff.d;
}

double 
strtod( cp, ptr )
    char * cp;
char **ptr ;
{
    int sign = 0;
    int ndbp = 0;
    int nd = 0;
    int exp = 0;
    int esign = 0;
    int sig = 0; /* significant digits found */
    char sanitized[NDIG];
    char * sp = sanitized;
    register char c;

char *dummy ;
if (ptr == (char **) 0) ptr = &dummy ; /* for atof calls */
*ptr = cp ;

    /* Atof recognizes               */
    /* an optional string of spaces, */
    while (*cp == ' ')
	cp++;
    /* then an optional sign, */
    if ((c= *cp) == '-' ){
	sign = 1;
	cp++;
    }else if (c=='+')
	cp++;
    /* then a string of digits ... */
    while(( c= *cp ) == '0' )
	{
	cp++;
	*ptr = cp ;
	}
    while(( c= *(cp++) ) >= '0' && c <= '9' ){
	sig=1;
	if (nd<=NDIG){
	    *sp++ = c;
	    nd++;
	}
	ndbp++;
    }
    /* ... optionally containing a decimal point, */
    if ( c == '.' ){
	if (!sig){
	    while(( c= *cp ) == '0' ){
		cp++;
		*ptr = cp ;
		ndbp--;
	    }
	}
	while(( c= *(cp++) ) >= '0' && c <= '9' ){
	    sig = 1;
	    if (nd<=NDIG){
		*sp++ = c;
		nd++;
	    }
	}
    }
    if (!sig)
	return 0.0;
*ptr = cp ;
    /* then an optional 'e' or 'E' */
    if ( c == 'e'|| c == 'E' )
	c = *cp++;
    /* followed by an optionally signed integer. */
    if ( c == '-' ){
	esign = 1;
	c = *cp++;
    }else if (c == '+' )
	c = *cp++;
    while( c>='0' && c <= '9' ){
	exp = 10*exp + c-'0';
	c = *cp++;
    }
*ptr = cp ;
    if (esign)
	exp =  - exp;
    return _atod( sanitized, nd, ndbp+exp, sign );
}
