#ifndef lint 
static  char sccsid[] = "@(#)utilities.c 1.1 86/09/25 SMI"; /* from UCB 4.1 12/25/82 */
#endif
 
#include <errno.h>
#include "libmdefs.h"

int errno ;

int dminus(x) /* returns 0 for positive, nonzero for negative */
double x ;
{
return(*((char *) &x) & 0x80) ;
}

int dclass(x)
double x ;
{
union { double d; int i[1] } kluge ;
int exp ;

kluge.d = x ;
exp = kluge.i[0] & 0x7ff00000 ;
if (exp == 0)
	{
	if( ((kluge.i[0] & 0x000fffff) == 0) & (kluge.i[1] == 0) )
		return(zero) ; else return(subnormal) ;
	}
if (exp == 0x7ff00000)
	{
	if( ((kluge.i[0] & 0x000fffff) == 0) & (kluge.i[1] == 0) )
		return(inf) ; 
	if ((kluge.i[0] & 0x80000) == 0) return(snan) ; else return(qnan) ;
	}
return(normal) ;
}

double
dinf()  /* function to return double precision +infinity */
{
union { int i[2]; double d } kluge ;
kluge.i[0] = 0x7ff00000 ; kluge.i[1] = 0 ;
return(kluge.d) ; 
}

double
dnan(nancode) /* function to return error nan and set errno = edom */
int nancode ; /* nan code is ignored at present */
{
union { int i[2]; double d } kluge ;
errno = EDOM ;
kluge.i[0] = 0x7fffffff ; kluge.i[1] = 0xffffffff ;
return(kluge.d) ; /* 68881 error nan */ 
}

double
dquietnan(nan) /* function to return a quiet nan */
double nan ;
{
int nancode ;
union { int i[2]; double d ; } kluge ;
kluge.d = nan ;
kluge.i[0] = kluge.i[0] | 0x80000 ; /* Turn off signalling bit. */
return(kluge.d) ; 
}
