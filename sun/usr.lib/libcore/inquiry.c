#ifndef lint
static char sccsid[] = "@(#)inquiry.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */
#include "coretypes.h"
#include "corevars.h"


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_viewing_parameters                                 */
/*                                                                          */
/*     PURPOSE: INQUIRE CURRENT VIEWING PARAMETERS.                         */
/*                                                                          */
/****************************************************************************/

inquire_viewing_parameters(viewparm) vwprmtype *viewparm;
{
    float *fp, f;

    *viewparm = _core_vwstate;

    fp = (float *) (&viewparm->viewport.xmin);
    f = (float) MAX_NDC_COORD;
						/* give user NDC 0..1 */
    *fp++ = (float) _core_vwstate.viewport.xmin / f;
    *fp++ = (float) _core_vwstate.viewport.xmax / f;
    *fp++ = (float) _core_vwstate.viewport.ymin / f;
    *fp++ = (float) _core_vwstate.viewport.ymax / f;
    *fp++ = (float) _core_vwstate.viewport.zmin / f;
    *fp++ = (float) _core_vwstate.viewport.zmax / f;

    _core_ndcset |= 1;
    return(0);
    }

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_window                                             */
/*                                                                          */
/*     PURPOSE: INQUIRE CURRENT WORLD COORDINATE WINDOW                     */
/*                                                                          */
/****************************************************************************/

inquire_window(umin, umax, vmin, vmax)
float *umin, *umax, *vmin, *vmax;
	{
	*umin = _core_vwstate.window.xmin;
	*umax = _core_vwstate.window.xmax;
	*vmin = _core_vwstate.window.ymin;
	*vmax = _core_vwstate.window.ymax;
	}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_view_up_2                                          */
/*                                                                          */
/*     PURPOSE: INQUIRE CURRENT 2-D VIEW UP DIRECTION                       */
/*                                                                          */
/****************************************************************************/

inquire_view_up_2(dx_up, dy_up)
float *dx_up, *dy_up;
	{
	*dx_up = _core_vwstate.vwupdir[0];
	*dy_up = _core_vwstate.vwupdir[1];
	}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_ndc_space_2                                        */
/*                                                                          */
/*     PURPOSE: INQUIRE CURRENT 2-D NDC SPACE LIMITS                        */
/*                                                                          */
/****************************************************************************/

inquire_ndc_space_2(width, height)
float *width, *height;
	{
	*width = _core_ndc.width;
	*height = _core_ndc.height;
	}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_viewport_2                                         */
/*                                                                          */
/*     PURPOSE: INQUIRE CURRENT 2-D VIEWPORT                                */
/*                                                                          */
/****************************************************************************/

inquire_viewport_2(xmin, xmax, ymin, ymax)
float *xmin, *xmax, *ymin, *ymax;
	{
	float f;

	f = (float) MAX_NDC_COORD;
	*xmin = (float) _core_vwstate.viewport.xmin / f;
	*xmax = (float) _core_vwstate.viewport.xmax / f;
	*ymin = (float) _core_vwstate.viewport.ymin / f;
	*ymax = (float) _core_vwstate.viewport.ymax / f;
	_core_ndcset |= 1;
	}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_view_reference_point                               */
/*                                                                          */
/*     PURPOSE: INQUIRE CURRENT 3-D VIEW REFERENCE POINT                    */
/*                                                                          */
/****************************************************************************/

inquire_view_reference_point(x_ref, y_ref, z_ref)
float *x_ref, *y_ref, *z_ref;
	{
	*x_ref = _core_vwstate.vwrefpt[0];
	*y_ref = _core_vwstate.vwrefpt[1];
	*z_ref = _core_vwstate.vwrefpt[2];
	}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_view_plane_normal                                  */
/*                                                                          */
/*     PURPOSE: INQUIRE CURRENT 3-D VIEW PLANE NORMAL                       */
/*                                                                          */
/****************************************************************************/

inquire_view_plane_normal(dx_norm, dy_norm, dz_norm)
float *dx_norm, *dy_norm, *dz_norm;
	{
	*dx_norm = _core_vwstate.vwplnorm[0];
	*dy_norm = _core_vwstate.vwplnorm[1];
	*dz_norm = _core_vwstate.vwplnorm[2];
	}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_view_plane_distance                                */
/*                                                                          */
/*     PURPOSE: INQUIRE CURRENT 3-D VIEW PLANE DISTANCE                     */
/*                                                                          */
/****************************************************************************/

inquire_view_plane_distance(view_distance)
float *view_distance;
	{
	*view_distance = _core_vwstate.viewdis;
	}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_view_depth                                         */
/*                                                                          */
/*     PURPOSE: INQUIRE CURRENT 3-D VIEW DEPTH                              */
/*                                                                          */
/****************************************************************************/

inquire_view_depth(front_distance, back_distance)
float *front_distance, *back_distance;
	{
	*front_distance = _core_vwstate.frontdis;
	*back_distance = _core_vwstate.backdis;
	}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_projection                                         */
/*                                                                          */
/*     PURPOSE: INQUIRE CURRENT 3-D VIEWING PROJECTION                      */
/*                                                                          */
/****************************************************************************/

inquire_projection(projection_type, dx_proj, dy_proj, dz_proj)
int *projection_type;
float *dx_proj, *dy_proj, *dz_proj;
	{
	*projection_type = _core_vwstate.projtype;
	*dx_proj = _core_vwstate.projdir[0];
	*dy_proj = _core_vwstate.projdir[1];
	*dz_proj = _core_vwstate.projdir[2];
	}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_view_up_3                                          */
/*                                                                          */
/*     PURPOSE: INQUIRE CURRENT 3-D VIEW UP DIRECTION                       */
/*                                                                          */
/****************************************************************************/

inquire_view_up_3(dx_up, dy_up, dz_up)
float *dx_up, *dy_up, *dz_up;
	{
	*dx_up = _core_vwstate.vwupdir[0];
	*dy_up = _core_vwstate.vwupdir[1];
	*dz_up = _core_vwstate.vwupdir[2];
	}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_ndc_space_3                                        */
/*                                                                          */
/*     PURPOSE: INQUIRE CURRENT 3-D NDC SPACE LIMITS                        */
/*                                                                          */
/****************************************************************************/

inquire_ndc_space_3(width, height, depth)
float *width, *height, *depth;
	{
	*width = _core_ndc.width;
	*height = _core_ndc.height;
	*depth = _core_ndc.depth;
	}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_viewport_3                                         */
/*                                                                          */
/*     PURPOSE: INQUIRE CURRENT 3-D VIEWPORT                                */
/*                                                                          */
/****************************************************************************/

inquire_viewport_3(xmin, xmax, ymin, ymax, zmin, zmax)
float *xmin, *xmax, *ymin, *ymax, *zmin, *zmax;
	{
	float f;

	f = (float) MAX_NDC_COORD;
	*xmin = (float) _core_vwstate.viewport.xmin / f;
	*xmax = (float) _core_vwstate.viewport.xmax / f;
	*ymin = (float) _core_vwstate.viewport.ymin / f;
	*ymax = (float) _core_vwstate.viewport.ymax / f;
	*zmin = (float) _core_vwstate.viewport.zmin / f;
	*zmax = (float) _core_vwstate.viewport.zmax / f;
	_core_ndcset |= 1;
	}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_inverse_composite_matrix                           */
/*                                                                          */
/*     PURPOSE: Get the inverse of the concatenated world and view matrix.  */
/*                                                                          */
/****************************************************************************/

inquire_inverse_composite_matrix(arrayptr) float *arrayptr;
{
   float *matrxptr; short i;

   matrxptr = &_core_invwxform[0][0];
   for(i=0; i<16; i++) {
      *arrayptr++ = *matrxptr++;
      }
   return(0);
   }

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_world_coordinate_matrix_2                          */
/*                                                                          */
/*     PURPOSE: INQUIRE CURRENT WORLD or 'MODELLING' TRANSFORMATION.        */
/*                                                                          */
/****************************************************************************/

inquire_world_coordinate_matrix_2(arr) float *arr;
{
   *arr++ =_core_modxform[0][0]; *arr++ =_core_modxform[0][1]; *arr++ =_core_modxform[0][3];
   *arr++ =_core_modxform[1][0]; *arr++ =_core_modxform[1][1]; *arr++ =_core_modxform[1][3];
   *arr++ =_core_modxform[2][0]; *arr++ =_core_modxform[2][1]; *arr++ =_core_modxform[2][3];
   return(0);
   }

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_world_coordinate_matrix_3                          */
/*                                                                          */
/*     PURPOSE: INQUIRE CURRENT WORLD or 'MODELLING' TRANSFORMATION.        */
/*                                                                          */
/****************************************************************************/

inquire_world_coordinate_matrix_3(arrayptr) float *arrayptr;
{
   float *matrxptr; short i;

   matrxptr = &_core_modxform[0][0];
   for(i=0; i<16; i++) {
      *arrayptr++ = *matrxptr++;
      }
   return(0);
   }

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_viewing_control_parameters                           */
/*                                                                          */
/*     PURPOSE: INQUIRE VIEWING CONTROL PARAMETERS.                           */
/*                                                                          */
/****************************************************************************/

inquire_viewing_control_parameters(windowclip, frontclip, backclip, type)
int *windowclip, *frontclip, *backclip, *type;
   {
   *windowclip = _core_wndwclip;
   *frontclip = _core_frontclip;
   *backclip = _core_backclip;
   *type = _core_coordsys;
   }

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_retained_segment_surfaces                          */
/*                                                                          */
/*     PURPOSE: INQUIRE WHICH LOGICAL VIEW SURFACES WERE SELECTED WHEN THE  */
/*              SEGMENT 'segname' WAS CREATED.                              */
/*                                                                          */
/****************************************************************************/

inquire_retained_segment_surfaces(segname,arraycnt,surfaray,surfnum)
   int segname;
   int *surfnum;
   struct vwsurf surfaray[];
   int arraycnt;
{
   char *funcname;
   register int index;
   short found;
   segstruc *segptr;

   funcname = "inquire_retained_segment_surfaces";
   found = FALSE;

   for(segptr = &_core_segment[0];segptr < &_core_segment[SEGNUM];segptr++) {
      if(segname == segptr->segname) {
	 found = TRUE; break;
	 }
      }
   if(!found) {                /*** SPECIFIED SEGMENT EXIST ?? ***/
      _core_errhand(funcname,14); return(1);
      }

   /*
    *	COPY NUMBER AND LOGICAL NAMES OF VIEW SURFACES SELECTED
    *	WHEN SEGMENT WAS CREATED.
    */

   *surfnum = segptr->vsurfnum;

   for(index = 0;index < segptr->vsurfnum;index++) {
      if(index >= arraycnt) break;
      else surfaray[index] = segptr->vsurfptr[index]->vsurf;
      }
   return(0);
   }


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_retained_segment_names                             */
/*                                                                          */
/*     PURPOSE: INQUIRE EXISTING SEGMENT NAMES.                             */
/*                                                                          */
/****************************************************************************/

inquire_retained_segment_names(listcnt, seglist, segcnt)
   int seglist[];
   int listcnt;
   int *segcnt;
{
   char *funcname;
   register int index;
   segstruc *segptr;

   index = 0;
   funcname = "inquire_retained_segment_names";
   *segcnt = _core_segnum;
   for(segptr = &_core_segment[0];segptr < &_core_segment[SEGNUM];segptr++) {
      if(segptr->type > NORETAIN) {   /* SEGMENT CURRENTLY EXIST? */
	 if(index >= listcnt) break;
	 else seglist[index++] = segptr->segname;
      }
   }
   return(0);
}
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_open_retained_segment                              */
/*                                                                          */
/*     PURPOSE: INQUIRE SEGMENT NAME OF OPEN SEGMENT.                       */
/*                                                                          */
/****************************************************************************/

inquire_open_retained_segment( segname) int *segname;
{
    if (_core_osexists) *segname = _core_openseg->segname;
    else *segname = 0;
    return(0);
}
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_open_temporary_segment                             */
/*                                                                          */
/*     PURPOSE: INQUIRE IF TEMPORARY SEGMENT IS OPEN.                       */
/*                                                                          */
/****************************************************************************/

inquire_open_temporary_segment( open) int *open;
{
    *open = (_core_osexists && (_core_openseg->type == NORETAIN));
    return(0);
}
