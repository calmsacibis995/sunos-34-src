
static char	sccsid[] = "@(#)checker.c 1.1 9/25/86 Copyright SMI";


/* =================================================================
   This routine will be used to test the color graphics board
   hardware. It prints out all 256 colors in the color map in 
   a checker-board fashion.   PWC 2/14/82.
   Last Modified: PWC 4/14/82.
   ================================================================= */

#include "colorbuf.h"      /* Constants and routines to load color map */
 

/* This routine Writes out the checkerboard pattern. */
checker()
{
   int vector();       /* Function declaration */
   register short
      row,color,column;


   color = 0;         /* Start with color \#0 */
   row = 0;
   Set_CFunc(GR_copy);
   do {
      for (column=0;column<601;column+=40) {
	 set_fbuf_5x(column,row,8,30,color);
         color += 1;   /* Increment color */
      }
      row += 30;       /* Go down to next row of blocks on screen */
   }  while (color < 256);

}      /* End of Function checkerboard */

