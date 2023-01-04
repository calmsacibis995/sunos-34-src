static char     sccsid[] = "@(#)bwmon.c 1.1 9/25/86 Copyright Sun Micro";


#include "../include/framebuf.h"
#include "scbuf.h"

int GXBase = 0x1C0000;

bwmon()
{
	register ushort *xaddr,*yaddr,*caddr,temp;
 	register int h,w;

	yaddr = (ushort *)(GXBase | GXselectY);
	caddr = SC_Mem0;

	GXwidth = 16;
        GXfunction 	= GXcopy;
	for (h=900;h>0;h-=1) {
	   xaddr = (ushort *)(GXBase | GXsource | GXselectX);
	   *yaddr++ = 1;		/* Set Y address on B&W */
	   temp = *xaddr; xaddr += 16;	/* Prime pixel pipeline */
	   for (w=16;w>0;w-=1) {
	       *caddr++ = *xaddr; xaddr += 16;
	       *caddr++ = *xaddr; xaddr += 16;
	       *caddr++ = *xaddr; xaddr += 16;
	       *caddr++ = *xaddr; xaddr += 16;
	   }
	   if (SCWidth == 1152) caddr += 8;	/* Advance to next line */
	}
}		/* Routine read_bwmon */

