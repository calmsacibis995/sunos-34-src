#ifndef lint
static char sccsid[] = "@(#)setmatrix.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */
#include "coretypes.h"
#include "corevars.h"

/************************************************************************/
/*	FUNCTION: _core_setmatrix						*/
/*									*/
/*	PURPOSE: Build the image transformation matrix from the view	*/
/*		state variables.					*/
/************************************************************************/
double sin(), cos();

_core_setmatrix(segptr) segstruc *segptr;

/* This transform scales; rotates (in x, y, z order) about point 0,0,0;
   then translates */

   {
   register float *pf;
   register segstruc *asegptr;
   float xform[4][4];
   int i,j,k;

   asegptr = segptr;
   if(identchk(asegptr))
	{
	asegptr->idenflag = TRUE;
	return(0);
	}
   asegptr->idenflag = FALSE;

   _core_identity( xform);
   if ( asegptr->type >= XLATE2) {
       xform[3][0] = asegptr->segats.translat[0];	/* translate	 */
       xform[3][1] = asegptr->segats.translat[1];
       xform[3][2] = asegptr->segats.translat[2];
       }
   if ( asegptr->type == XFORM2) {
	_core_push( xform);
	if(asegptr->segats.rotate[2] != 0.) {
	    _core_identity( xform);			/* rotate about z */
	    xform[0][0] = xform[1][1] = cos( asegptr->segats.rotate[2]);
	    xform[0][1] = sin( asegptr->segats.rotate[2]);
	    xform[1][0] = -xform[0][1];
	    _core_push( xform); _core_matcon();
	    }
	if(asegptr->segats.rotate[1] != 0.) {
	    _core_identity( xform);			/* rotate about y */
	    xform[0][0] = xform[2][2] = cos( asegptr->segats.rotate[1]);
	    xform[2][0] = sin( asegptr->segats.rotate[1]);
	    xform[0][2] = -xform[2][0];
	    _core_push( xform); _core_matcon();
	    }
	if(asegptr->segats.rotate[0] != 0.) {
	    _core_identity( xform);			/* rotate about x */
	    xform[1][1] = xform[2][2] = cos( asegptr->segats.rotate[0]);
	    xform[1][2] = sin( asegptr->segats.rotate[0]);
	    xform[2][1] = -xform[1][2];
	    _core_push( xform); _core_matcon();
	    }

	if(asegptr->segats.scale[0] != 1. ||
	   asegptr->segats.scale[1] != 1. ||
	   asegptr->segats.scale[2] != 1.)
	   {
	   _core_identity( xform);			/* scale  */
	   xform[0][0] = asegptr->segats.scale[0];
	   xform[1][1] = asegptr->segats.scale[1];
	   xform[2][2] = asegptr->segats.scale[2];
	   _core_push( xform); _core_matcon();
	   }
	_core_pop( xform);
	}

					/* convert xform to integer imxform  */
					/* don't bother with last column     */
					/* since it's never used	     */
   pf = (float *) xform;
   for (i=0; i<3; i++) { 		/* scale and rotate coefficients     */
      for (j=0; j<3; j++) {		/* have MSW as signed 15 bit  */
	 k = (int)(*pf);		/* integer part and LSW as signed 15 */
	 *pf -= (float) k;		/* bit fractional part ( 1= 2 to the */
	 *pf *= 32767.0;		/* -15 power, or 1/32768 ).          */
	 asegptr->imxform[i][j] = (k << 16) | (_core_roundf(pf) & 0xFFFF);
	 pf++;
	 }
      pf++;
      }

					/* translate coeffs are signed int.  */
   asegptr->imxform[3][0] = _core_roundf(pf);
   pf++;
   asegptr->imxform[3][1] = _core_roundf(pf);
   pf++;
   asegptr->imxform[3][2] = _core_roundf(pf);

   return(0);
   }


static float idxfrmparams[] = {1.,1.,1.,0.,0.,0.,0.,0.,0.};


/****************************************************************************/
/*     FUNCTION: identchk                                                   */
/*                                                                          */
/*     PURPOSE: CHECK TO SEE IF SEGMENT TRANSFORM PARAMETERS SPECIFY AN     */
/*              IDENTITY TRANSFORM					    */
/****************************************************************************/

static int identchk(asegptr) register segstruc *asegptr;
{
   register float *ptr1, *ptr2; register short i;

   ptr1 = idxfrmparams;
   ptr2 = &asegptr->segats.scale[0];
   for(i = 0; i < 9; i++)
	if (*ptr1++ != *ptr2++)
	    return(0);
   return(1);
}
