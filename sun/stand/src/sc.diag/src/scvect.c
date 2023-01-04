


/* ======================================================================
   Author: Peter Costello
   Date :  April 21, 1982
   Purpose: This program draws a vector in the color frame buffer. 
      The vector will go from location (x0,y0) to (x1,y1).
   Algorithm: This program uses a Bresenham Algorithm and draws points
      in pixel mode. 
   Error Handling:
   ====================================================================== */

static char     sccsid[] = "@(#)scvect.c 1.1 9/25/86 Copyright Sun Micro";

#include "sc.diag.h"

int Paddr = 0;			/* Enables print of addresses */

vector(x0,y0,x1,y1,color)
   int x0,y0,x1,y1;
   uchar color;
{
   register uchar *addr;
   register short dx,dy,i,e,quadrant;
   int berr(),ivect;
   char ch;

   ivect = *((int*)0x0008);
   *((int*)0x0008) = (int)berr;

   Enable_All_Planes;		/* Enable all memory planes */
   dx = x1-x0; dy = y0-y1;	/* Origin in upper left */
   
   /* printf("SCVECT: X0 = %d. Y0 = %d.",x0,y0);
   printf(" DX = %d. DY = %d (%d).\n",dx,dy,-dy); */

   if (dy == 0) {
      addr = SC_Pix + (y0 << 10) + x0;
      if (SCWidth != 1024) addr += (y0 << 7);
      if (dx >= 0) {
         while (dx >= 0) {
	    /* if (Paddr) print_xy(addr); */
	    *addr++ = color;
	    dx -= 1;
	 }
      } else {
	 while (dx <= 0) {
	    /* if (Paddr) print_xy(addr); */
	    *addr-- = color;
	    dx += 1;
	 }
      }
   } else if (dx == 0) {
      /* printf("Debug: SCVECT. Drawing vertical line."); */
      addr = SC_Pix + (y0 << 10) + x0;
      if (SCWidth != 1024) addr += (y0 << 7);
      if (dy >= 0) {
         while (dy >= 0) { 
	    /* if (((int)addr<0x300000)||((int)addr>=0x400000)) { 
                printf("SCVECT: X0 = %d. Y0 = %d.",x0,y0);
                printf("SCVECT: X1 = %d. Y1 = %d.",x1,y1);
		print_xy(addr); 
	    } */
	    *addr = color;
	    addr -= SCWidth;
 	    dy -= 1; 	
	 }
      } else {
         while (dy <= 0) {
	    /* if (Paddr) print_xy(addr); */
	    /* if (((int)addr<0x300000)||((int)addr>=0x400000)) { 
                printf("SCVECT: X0 = %d. Y0 = %d.",x0,y0);
                printf("SCVECT: X1 = %d. Y1 = %d.",x1,y1);
		print_xy(addr); 
	    } */
	    *addr = color;
	    addr += SCWidth;
 	    dy += 1; 	
	 }
      }

   } else {  /* Do generalized processing */
      /* Make dx always positive */
      if (dx < 0) {
	 x0 += dx; y0 += dy;
	 dx = -dx; dy = -dy;
      }

      /* We are always in quadrants 1 or 4 now. */
      quadrant = 4 - (dy>0)*3;

      /* Now make dy positive */
      if (dy < 0) dy = -dy;

      addr = SC_Pix + (y0 << 10) + x0;
      if (SCWidth != 1024) addr += (y0 << 7);
      if (dx > dy) {
	 e = dy<<1 - dx;
	 if (quadrant == 1) {
	    for (i=0;i<dx;i++) {
	    /* if (Paddr) print_xy(addr); */
	       *addr++ = color;
	       if (e > 0) {
		  addr -= SCWidth;		/* Go up a line */
	          e += (dy - dx)<<1;
	       } else {
	          e += dy<<1;
	       }
	    }
	 } else {
	    for (i=0;i<dx;i++) {
	    /* if (Paddr) print_xy(addr); */
	       *addr++ = color;
	       if (e > 0) {
		  addr += SCWidth;		/* Go down a line */
	          e += (dy - dx)<<1;
	       } else {
	          e += dy<<1;
	       }
	    }
	 }
      } else {		/* dy > dx */
	 e = dx<<1 - dy;
	 if (quadrant == 1) {
	    for (i=0;i<dy;i++) {
	    /* if (Paddr) print_xy(addr); */
	       if (e > 0) {
		  *addr++ = color;		/* Go over a pixel */
	          e += (dx - dy)<<1;
	       } else {
		  *addr = color;
	          e += dx<<1;
	       }
	       addr -= SCWidth; 			/* Go up a line */
	    }
	 } else {
	    for (i=0;i<dy;i++) {
	    /* if (Paddr) print_xy(addr); */
	       if (e > 0) {
		  *addr++ = color; 		/* Go over a pixel */
	          e += (dx - dy)<<1;
	       } else {
	          *addr = color;
	          e += dx<<1;
	       }
	       addr += SCWidth;			/* Go down a line */
	    }
	 }
      }
   }
   if (0) {
      asm("_berr:");
      printf("Timeout in SCVECT.C\n");
      printf("DX = %d, DY = -%d, COLOR = 0x%x\n",dx,-dy,color);
      printf("I = %d, E = %d, QUADRANT = %d\n",i,e,quadrant);
      printf("Addr = 0x%x\n",(int)addr);
      printf("Hit Any Char to Continue");
      ch = getchar(); printf("\n");
      asm("	rte");
   }
   *((int*)0x0008) = ivect;

}		/* End of Procedure vector */
	       
