#ifndef lint
static char sccsid[] = "@(#)inqsegatt.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */
#include "coretypes.h"
#include "corevars.h"

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_segment_detectability                              */
/*                                                                          */
/****************************************************************************/

inquire_segment_detectability(segname,detectbl) int segname; int *detectbl;
{
   char *funcname;
   int errnum;
   segstruc *segptr;

   funcname = "inquire_segment_detectability";
   for (segptr = &_core_segment[0]; segptr < &_core_segment[SEGNUM]; segptr++) {
      if((segptr->type != DELETED) && (segname == segptr->segname)) {
	 *detectbl = segptr->segats.detectbl;
	 return(0);
	 }
      if (segptr->type == EMPTY) {
	 errnum = 29; _core_errhand(funcname,errnum);
	 return(errnum);
	 }
      }
   errnum = 29; _core_errhand(funcname,errnum);
   return(errnum);
   }


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_segment_highlighting                               */
/*                                                                          */
/****************************************************************************/

inquire_segment_highlighting(segname,highlght) int segname; int *highlght;
{
   char *funcname;
   int errnum;
   segstruc *segptr;

   funcname = "inquire_segment_highlighting";
   for (segptr = &_core_segment[0]; segptr < &_core_segment[SEGNUM]; segptr++) {
      if((segptr->type != DELETED) && (segname == segptr->segname)) {
	 *highlght = segptr->segats.highlght;
	 return(0);
	 }
      if (segptr->type == EMPTY) {
	 errnum = 29; _core_errhand(funcname,errnum);
	 return(errnum);
	 }
      }
   errnum = 29; _core_errhand(funcname,errnum);
   return(errnum);
   }

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_image_transformation_type                          */
/*                                                                          */
/****************************************************************************/
inquire_image_transformation_type(segtype) int *segtype;
{
   *segtype = _core_csegtype;
   return(0);
   }

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_segment_image_transformation_type                  */
/*                                                                          */
/****************************************************************************/
inquire_segment_image_transformation_type(segname,segtype)
	int segname; int *segtype;
{
   char *funcname;
   int errnum;
   segstruc *segptr;

   funcname = "inquire_segment_image_transformation_type";
   for (segptr = &_core_segment[0]; segptr < &_core_segment[SEGNUM]; segptr++) {
      if((segptr->type != DELETED) && (segname == segptr->segname)) {
	 *segtype = segptr->type;
	 return(0);
	 }
      if (segptr->type == EMPTY) {
	 errnum = 29; _core_errhand(funcname,errnum);
	 return(errnum);
	 }
      }
   errnum = 29; _core_errhand(funcname,errnum);
   return(errnum);
   }


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_segment_visibility                                 */
/*                                                                          */
/****************************************************************************/

inquire_segment_visibility(segname,visbilty) int segname; int *visbilty;
{
   char *funcname;
   int errnum;
   segstruc *segptr;

   funcname = "inquire_segment_visibility";
   for (segptr = &_core_segment[0]; segptr < &_core_segment[SEGNUM]; segptr++) {
      if((segptr->type != DELETED) && (segname == segptr->segname)) {
	 *visbilty = segptr->segats.visbilty;
	 return(0);
	 }
      if (segptr->type == EMPTY) {
	 errnum = 29;
	 _core_errhand(funcname,errnum);
	 return(1);
	 }
      }
   errnum = 29;
   _core_errhand(funcname,errnum);
   return(1);
   }


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_segment_image_transformation_2                     */
/*                                                                          */
/****************************************************************************/

inquire_segment_image_transformation_2(segname,sx,sy,a,tx,ty)
   int segname; float *sx,*sy,*a,*tx,*ty;
{
   char *funcname;
   int errnum;
   segstruc *segptr;

   funcname = "inquire_segment_image_transformation_2";
   for (segptr = &_core_segment[0]; segptr < &_core_segment[SEGNUM]; segptr++) {
      if((segptr->type != DELETED) && (segname == segptr->segname)) {
	 if( segptr->type < XLATE2) {
	    _core_errhand(funcname,30);
	    return(2);
	    }
	 *a = segptr->segats.rotate[2];
	 *sx = segptr->segats.scale[0];
	 *sy = segptr->segats.scale[1];
	 *tx = segptr->segats.translat[0]/ (float) MAX_NDC_COORD;
	 *ty = segptr->segats.translat[1]/ (float) MAX_NDC_COORD;
	 return(0);
	 }
      if (segptr->type == EMPTY) {
	 _core_errhand(funcname,29);
	 return(1);
	 }
      }
   _core_errhand(funcname,29);
   return(1);
   }

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_segment_image_translate_2                          */
/*                                                                          */
/****************************************************************************/

inquire_segment_image_translate_2(segname,tx,ty)
   int segname; float *tx,*ty;
{
   char *funcname;
   int errnum;
   segstruc *segptr;

   funcname = "inquire_segment_image_translate_2";
   for (segptr = &_core_segment[0]; segptr < &_core_segment[SEGNUM]; segptr++) {
      if((segptr->type != DELETED) && (segname == segptr->segname)) {
	 if(segptr->type == NORETAIN || segptr->type == RETAIN) {
	    _core_errhand(funcname,30);
	    return(2);
	    }
	 *tx = segptr->segats.translat[0]/ (float) MAX_NDC_COORD;
	 *ty = segptr->segats.translat[1]/ (float) MAX_NDC_COORD;
	 return(0);
	 }
      if (segptr->type == EMPTY) {
	 _core_errhand(funcname,29);
	 return(1);
	 }
      }
   _core_errhand(funcname,29);
   return(1);
   }


