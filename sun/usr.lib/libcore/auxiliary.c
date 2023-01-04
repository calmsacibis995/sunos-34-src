#ifndef lint
static char sccsid[] = "@(#)auxiliary.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */
#include "coretypes.h"
#include "corevars.h"
#include <stdio.h>

static float cidmatrix[4][4] = {1.,0.,0.,0.,
				0.,1.,0.,0.,
				0.,0.,1.,0.,
				0.,0.,0.,1.};

/****************************************************************************/
/*     FUNCTION: _core_identity                                                   */
/*                                                                          */
/*     PURPOSE: SET A 4X4 ARRAY EQUAL TO THE IDENTITY MATRIX.               */
/****************************************************************************/

_core_identity(arrayptr) register float *arrayptr;
{
   register float *temptr; register short i;

   temptr = &cidmatrix[0][0];
   for(i = 0; i < 16; i++) *arrayptr++ = *temptr++;
}

/****************************************************************************/
/*     FUNCTION: _core_scalept                                                    */
/*                                                                          */
/*     PURPOSE: Multiply a point by a scalar				    */
/****************************************************************************/

_core_scalept( s, pt) float s; register pt_type *pt;
{
   pt->x *= s;
   pt->y *= s;
   pt->z *= s;
}
/*-------------------------------------------------------------------------*/
/* Integer square root */

/*  #define quotrem(a,b,q,r)						\
	d0 = a;								\
	d1 = b;								\
	divu(d0,d1);							\
	q = (short)d0;							\
	swap(d0);							\
	r = (short)d0;
*/

_core_jsqrt(n) register unsigned n;
{
    register unsigned q,r,x2,x;
    unsigned t;

    if (n > 0xFFFE0000) return 0xFFFF;	/* algorithm doesn't cover this case*/
    if (n == 0xFFFE0000) return 0xFFFE;	/* or this case */
    if (n < 2) return n;			/* or this case */
    t = x = n;
    while (t>>=2) x>>=1;
    x++;
    for(;;) {
	/* quotrem(n,x,q,r);      /* q = n/x, r = n%x */
	q = n/x;
	r = n%x;
	if (x <= q) {
		x2 = x+2;
		if (q < x2 || q==x2 && r==0) break;
	}
	x = (x + q)>>1;
    }
    return x;
}

