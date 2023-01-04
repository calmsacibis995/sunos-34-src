#ifndef lint
static char sccsid[] = "@(#)ndctowld.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */
#include "coretypes.h"
#include "corevars.h"

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: map_ndc_to_world_2                                         */
/*                                                                          */
/*     PURPOSE: CONVERTS NORMALIZED DEVICE COORDINATES TO WORLD COORDINATES.*/
/*                                                                          */
/****************************************************************************/

map_ndc_to_world_2(ndcx,ndcy,wldx,wldy)
   float ndcx,ndcy,*wldx,*wldy;
{
    /* COMPUTE WORLD COORDINATES               */
    /* USING INVERSE VIEWING TRANSFORMATION    */

    register int nx, ny, round;
    register float *pf;
    float f;

    pf = &f;
    f = ndcx;
    f *= (float) MAX_NDC_COORD;
    nx = _core_roundf(&f);
    f = ndcy;
    f *= (float) MAX_NDC_COORD;
    ny = _core_roundf(&f);

    if ((nx -= _core_poffx) < 0)
	round = (-_core_scalex) >> 1;
    else
	round = _core_scalex >> 1;
    nx = ((nx << 15) + round) / _core_scalex;
    if ((ny -= _core_poffy) < 0)
	round = (-_core_scaley) >> 1;
    else
	round = _core_scaley >> 1;
    ny = ((ny << 15) + round) / _core_scaley;

    *wldx = (float) nx * _core_invwxform[0][0] +
	    (float) ny * _core_invwxform[1][0] +
	    (float) MAX_NDC_COORD * _core_invwxform[3][0];
    *wldy = (float)nx * _core_invwxform[0][1] +
	    (float)ny * _core_invwxform[1][1] +
	    (float) MAX_NDC_COORD * _core_invwxform[3][1];

    return(0);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: map_world_to_ndc_2                                         */
/*                                                                          */
/*     PURPOSE: CONVERTS WORLD COORDINATES TO NORMALIZED DEVICE COORDINATES.*/
/*                                                                          */
/****************************************************************************/

map_world_to_ndc_2(wldx,wldy,ndcx,ndcy) float wldx,wldy,*ndcx,*ndcy;
{
    pt_type p1, p2;
    ipt_type ip;

    /*
     * Transform world coords to current viewport NDC coords
     */
 
    p1.x = wldx;  p1.y = wldy;   p1.z = 0.;  p1.w = 1.0;
    _core_tranpt2( &p1, &p2);
    _core_pt3cnvrt( &p2, &ip);
    _core_vwpscale( &ip);
    *ndcx = (float)ip.x/(float) MAX_NDC_COORD;
    *ndcy = (float)ip.y/(float) MAX_NDC_COORD;

    return(0);
}
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: map_ndc_to_world_3                                         */
/*                                                                          */
/*     PURPOSE: CONVERTS NORMALIZED DEVICE COORDINATES TO WORLD COORDINATES.*/
/*                                                                          */
/****************************************************************************/

map_ndc_to_world_3(ndcx,ndcy,ndcz,wldx,wldy,wldz)
   float ndcx,ndcy,ndcz, *wldx,*wldy,*wldz;
{
    /*
     * Compute world coordinates using inverse view transform
     */

    register int nx, ny, nz, round;
    register float *pf;
    float f;

    pf = &f;
    f = ndcx;
    f *= (float) MAX_NDC_COORD;
    nx = _core_roundf(&f);
    f = ndcy;
    f *= (float) MAX_NDC_COORD;
    ny = _core_roundf(&f);
    f = ndcz;
    f *= (float) MAX_NDC_COORD;
    nz = _core_roundf(&f);

    if ((nx -= _core_poffx) < 0)
	round = (-_core_scalex) >> 1;
    else
	round = _core_scalex >> 1;
    nx = ((nx << 15) + round) / _core_scalex;
    if ((ny -= _core_poffy) < 0)
	round = (-_core_scaley) >> 1;
    else
	round = _core_scaley >> 1;
    ny = ((ny << 15) + round) / _core_scaley;
    if ((nz -= _core_poffz) < 0)
	round = (-_core_scalez) >> 1;
    else
	round = _core_scalez >> 1;
    nz = ((nz << 15) + round) / _core_scalez;

    *wldx = (float) nx * _core_invwxform[0][0] +
	    (float) ny * _core_invwxform[1][0] +
	    (float) nz * _core_invwxform[2][0] +
	    (float) MAX_NDC_COORD * _core_invwxform[3][0];
    *wldy = (float) nx * _core_invwxform[0][1] +
	    (float) ny * _core_invwxform[1][1] +
	    (float) nz * _core_invwxform[2][1] +
	    (float) MAX_NDC_COORD * _core_invwxform[3][1];
    *wldz = (float) nx * _core_invwxform[0][2] +
	    (float) ny * _core_invwxform[1][2] +
	    (float) nz * _core_invwxform[2][2] +
	    (float) MAX_NDC_COORD * _core_invwxform[3][2];

    return(0);
}



/****************************************************************************/
/*                                                                          */
/*     FUNCTION: map_world_to_ndc_3                                         */
/*                                                                          */
/*     PURPOSE: CONVERTS WORLD COORDINATES TO NORMALIZED DEVICE COORDINATES.*/
/*                                                                          */
/****************************************************************************/

map_world_to_ndc_3(wldx,wldy,wldz,ndcx,ndcy,ndcz)
   float wldx,wldy,wldz,  *ndcx,*ndcy,*ndcz;
{
    pt_type p1, p2;
    ipt_type ip;

    /*
     * View transform world coords to current viewport coords
     */

    p1.x = wldx;  p1.y = wldy;   p1.z = wldz;  p1.w = 1.0;
    _core_tranpt( &p1, &p2);
    _core_pt3cnvrt( &p2, &ip);
    _core_vwpscale( &ip);
    *ndcx = (float)ip.x/(float) MAX_NDC_COORD;
    *ndcy = (float)ip.y/(float) MAX_NDC_COORD;
    *ndcz = (float)ip.z/(float) MAX_NDC_COORD;

    return(0);
}

