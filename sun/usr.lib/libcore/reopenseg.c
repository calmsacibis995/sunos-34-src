#ifndef lint
static char sccsid[] = "@(#)reopenseg.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */
#include "coretypes.h"
#include "corevars.h"

/****************************************************************************/
_core_reopensegment()
   {
   int index;
   ddargtype ddstruct;
   register viewsurf *surfp;
   					/* delete the segment from devices */
   for (index = 0; index < _core_openseg->vsurfnum; index++)
      {
      surfp = _core_openseg->vsurfptr[index];
      if (surfp->dehardwr)
	 {
	 ddstruct.instance = surfp->vsurf.instance;
	 ddstruct.opcode = DELETE;
	 ddstruct.logical = _core_openseg->segname;
	 (*surfp->vsurf.dd)(&ddstruct);
	 }
					/* reopen the segment by calling DD's */
      if (surfp->segopclo)
	 {
	 ddstruct.instance = surfp->vsurf.instance;
	 ddstruct.opcode = OPENSEG;
	 ddstruct.logical = _core_openseg->segname;
	 (*surfp->vsurf.dd)(&ddstruct);
	 				/* replace what was there, leave open */
	 _core_segdraw(_core_openseg,index,FALSE);
	 }
      }
   return(0);
   }

