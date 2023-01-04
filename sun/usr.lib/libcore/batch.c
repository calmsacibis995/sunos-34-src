#ifndef lint
static char sccsid[] = "@(#)batch.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */
#include "coretypes.h"
#include "corevars.h"

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: begin_batch_of_updates                                     */
/*                                                                          */
/*     PURPOSE: DENOTES THE BEGINNING OF A BATCH OF CHANGES TO THE PICTURE. */
/*                                                                          */
/****************************************************************************/

begin_batch_of_updates()
   {
   char *funcname;
   int errnum;

   funcname = "begin_batch_of_updates";

   if(_core_batchupd) {
      errnum = 17;
      _core_errhand(funcname,errnum);
      return(1);
      }

   _core_batchupd = TRUE;

   return(0);
   }

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: end_batch_of_updates                                       */
/*                                                                          */
/*     PURPOSE: DENOTES THE END OF A BATCH OF CHANGES TO THE PICTURE.       */
/*                                                                          */
/****************************************************************************/

end_batch_of_updates()
{
   char *funcname;
   int errnum;

   funcname = "end_batch_of_updates";

   if(!_core_batchupd) {
      errnum = 18;
      _core_errhand(funcname,errnum);
      return(1);
      }

/*** NEW FRAME ALL SURFACES WHOSE NEW FRAME NEEDED FLAG IS SET ***/
   _core_repaint(FALSE);

/*** eventually should add drain buffer command to DD ***/
/*** to be sent after each primitive command, only if no batch ***/

   _core_batchupd = FALSE;
   return(0);
   }
