
/* ======================================================================
   AUTHOR: Peter Costello
   Date :  April 15, 1983
   Purpose: This file contains some useful utility programs for the 
	Sun-2 color board.
   Error Handling:
   Bugs:
   ====================================================================== */
static char     sccsid[] = "@(#)scutils.c 1.1 9/25/86 Copyright Sun Micro";

#include "sc.diag.h" 
/* #define msec1 150 */
#define msec1 15
#define wait_msec(msec) i=msec;j=msec1;while (i-- > 0) while (j-- > 0)

int SCBase = 0x200000;
int SCWidth = 1152;
int SCHeight = 909;
int SCRop  = 0x500000;

/* ===============================================================
   Global Variable Definitions.
   Default colors for color map 0. Only entries 0 to 255 defined.
   =============================================================== */

ushort sc_red[256] = { 
        0,255, 85,170,
    
        3,  6,  9, 12,     6, 12, 18, 24,     9, 18, 27, 36,
       12, 24, 36, 48,    15, 30, 45, 60,    18, 36, 54, 72,
       21, 42, 63, 84,    24, 48, 72, 96,    27, 54, 81,108, 
       30, 60, 90,120,    33, 66, 99,132,    36, 72,108,144,
       39, 78,117,156,    42, 84,126,168,    45, 90,135,180,   
       48, 96,144,192,    51,102,153,204,    54,108,162,216,  
       57,114,171,228,    60,120,180,240,    63,126,189,252,  

	0, 80,160,240,     0, 80,160,240,     0, 80,160,240,
	0, 80,160,240,     0, 80,160,240,     0, 80,160,240,
	0, 80,160,240,     0, 80,160,240,     0, 80,160,240,
	0, 80,160,240,     0, 80,160,240,     0, 80,160,240,
	0, 80,160,240,     0, 80,160,240,     0, 80,160,240,
	0, 80,160,240,     0, 80,160,240,     0, 80,160,240,
	0, 80,160,240,     0, 80,160,240,     0, 80,160,240,

       60,120,180,240,    57,114,171,228,    54,108,162,216,    
       51,102,153,204,    48, 96,144,192,    45, 90,135,180,   
       42, 84,126,168,    39, 78,117,156,    36, 72,108,144,  
       33, 66, 99,132,    30, 60, 90,120,    27, 54, 81,108,   
       24, 48, 72, 96,    21, 42, 63, 84,    18, 36, 54, 72,    
       15, 30, 45, 60,    12, 24, 36, 48,     9, 18, 27, 36,    
        6, 12, 18, 24,     3,  6,  9, 12,     0,  0,  0,  0 };

ushort sc_grn[256] = { 
        0,255, 85,170,

       60,120,180,240,    57,114,171,228,    54,108,162,216,    
       51,102,153,204,    48, 96,144,192,    45, 90,135,180,   
       42, 84,126,168,    39, 78,117,156,    36, 72,108,144,  
       33, 66, 99,132,    30, 60, 90,120,    27, 54, 81,108,   
       24, 48, 72, 96,    21, 42, 63, 84,    18, 36, 54, 72,    
       15, 30, 45, 60,    12, 24, 36, 48,     9, 18, 27, 36,    
        6, 12, 18, 24,     3,  6,  9, 12,     0,  0,  0,  0,
    
        3,  6,  9, 12,     6, 12, 18, 24,     9, 18, 27, 36,
       12, 24, 36, 48,    15, 30, 45, 60,    18, 36, 54, 72,
       21, 42, 63, 84,    24, 48, 72, 96,    27, 54, 81,108, 
       30, 60, 90,120,    33, 66, 99,132,    36, 72,108,144,
       39, 78,117,156,    42, 84,126,168,    45, 90,135,180,   
       48, 96,144,192,    51,102,153,204,    54,108,162,216,  
       57,114,171,228,    60,120,180,240,    63,126,189,252,  

	0, 80,160,240,     0, 80,160,240,     0, 80,160,240,
	0, 80,160,240,     0, 80,160,240,     0, 80,160,240,
	0, 80,160,240,     0, 80,160,240,     0, 80,160,240,
	0, 80,160,240,     0, 80,160,240,     0, 80,160,240,
	0, 80,160,240,     0, 80,160,240,     0, 80,160,240,
	0, 80,160,240,     0, 80,160,240,     0, 80,160,240,
	0, 80,160,240,     0, 80,160,240,     0, 80,160,240 };

ushort sc_blu[256] = { 
        0,255, 85,170,

	0, 80,160,240,     0, 80,160,240,     0, 80,160,240,
	0, 80,160,240,     0, 80,160,240,     0, 80,160,240,
	0, 80,160,240,     0, 80,160,240,     0, 80,160,240,
	0, 80,160,240,     0, 80,160,240,     0, 80,160,240,
	0, 80,160,240,     0, 80,160,240,     0, 80,160,240,
	0, 80,160,240,     0, 80,160,240,     0, 80,160,240,
	0, 80,160,240,     0, 80,160,240,     0, 80,160,240,
       
       60,120,180,240,    57,114,171,228,    54,108,162,216,    
       51,102,153,204,    48, 96,144,192,    45, 90,135,180,   
       42, 84,126,168,    39, 78,117,156,    36, 72,108,144,  
       33, 66, 99,132,    30, 60, 90,120,    27, 54, 81,108,   
       24, 48, 72, 96,    21, 42, 63, 84,    18, 36, 54, 72,    
       15, 30, 45, 60,    12, 24, 36, 48,     9, 18, 27, 36,    
        6, 12, 18, 24,     3,  6,  9, 12,     0,  0,  0,  0,
    
        3,  6,  9, 12,     6, 12, 18, 24,     9, 18, 27, 36,
       12, 24, 36, 48,    15, 30, 45, 60,    18, 36, 54, 72,
       21, 42, 63, 84,    24, 48, 72, 96,    27, 54, 81,108, 
       30, 60, 90,120,    33, 66, 99,132,    36, 72,108,144,
       39, 78,117,156,    42, 84,126,168,    45, 90,135,180,   
       48, 96,144,192,    51,102,153,204,    54,108,162,216,  
       57,114,171,228,    60,120,180,240,    63,126,189,252 };


/* ======================================================================
   Author: Peter Costello
   Date :  April 15, 1983
   Purpose: This routine takes pointers to three memory arrays with which
      we will load into the color map using the color plane specified.
   Algorithm:
   Error Handling:
   ====================================================================== */

write_cmap(color1,color2,color3)
   ushort *color1,*color2,*color3;
{
   register ushort *rcolor,*gcolor,*bcolor;
   register int i;

   Acquire_Cmap;
   rcolor = SC_Red_Cmap;
   gcolor = SC_Grn_Cmap;
   bcolor = SC_Blu_Cmap;
   for (i=0;i<32;i++) {
      *rcolor++ = *color1++; *rcolor++ = *color1++;
      *rcolor++ = *color1++; *rcolor++ = *color1++;
      *rcolor++ = *color1++; *rcolor++ = *color1++;
      *rcolor++ = *color1++; *rcolor++ = *color1++;

      *gcolor++ = *color2++; *gcolor++ = *color2++;
      *gcolor++ = *color2++; *gcolor++ = *color2++;
      *gcolor++ = *color2++; *gcolor++ = *color2++;
      *gcolor++ = *color2++; *gcolor++ = *color2++;

      *bcolor++ = *color3++; *bcolor++ = *color3++;
      *bcolor++ = *color3++; *bcolor++ = *color3++;
      *bcolor++ = *color3++; *bcolor++ = *color3++;
      *bcolor++ = *color3++; *bcolor++ = *color3++;
   }
   Release_Cmap;
   i=0; while ((! SC_Retrace)&&(i<0x1000000)) i+=1;
   i=0; while ((  SC_Retrace)&&(i<0x1000000)) i+=1;
   Acquire_Cmap;
}


/* ======================================================================
   Author : Peter Costello
   Date   : April 21, 1982
   Purpose : This routine initializes the color board. It clears the 
	frame buffer to color 0, enables all video planes, loads the 
	default color map, and enables video.
   Algorithm :
   Error Handling : 
   ====================================================================== */

init_scolor()
{
   /* Enable all memory planes */
   Enable_All_Planes;

   /* Write Checkerboard to frame buffer */
   SC_Stat = 0;
   schecker();

   /* Set Color Maps to default values */
   write_cmap(sc_red,sc_grn,sc_blu);

   SC_Zoom = 0;
   SC_WPan = 0;
   SC_PPan = 0;
   SC_VZoom = 0xff;

   Init_Stat;		/* Init Status Register */
}
   
short bad_zoom[16] = {70,70,70,70,70,70,70,70,70,69,71,71,71,66,69,72};
          
/* ======================================================================
   Author: Peter Costello
   Date :  April 15, 1983
   Purpose: This routine zooms the display and sets the origin to the
	given parameter. The frame buffer coordinates used are (x,y)
	such that (0 <= x < 1152*(zoom+1)) and (0 <= y < 900*(zoom+1)).
   Algorithm:
   Error Handling: All parameters are assumed to be valid.
   Bugs:
   ====================================================================== */
int SC_X,SC_Y;			/* X and Y of Origin */
Set_Zoom(x,y,zoom)
   int x,y,zoom;
{  
   register ushort wpan, *PSC_WPan, *PSC_Stat;
   register uchar ppan,zreg, *PSC_PPan, *PSC_Zoom ;
   register int pix_origin;
   register short pixoff,temp,hoff,voff;
   int i, j;

   /* printf("DEBUG: Set_Zoom. X = %d. Y = %d. Zoom = 0x%x.\n",x,y,zoom); */
   SC_X = x; SC_Y = y;

   zoom += 1;			/* Normalize to  1..16 */
   pix_origin = (y/zoom)*SCWidth + (x/zoom);
   hoff = x % zoom;
   voff = (zoom - 1) - (y % zoom);

   /* This is funky. But figure out pan registers */
   if ((zoom % 4) == 0) {	/* Pan four phosphors at a time */
      pixoff = (hoff+1) / 4;	/* Round to nearest mod 4  boundary */
   } else if (zoom == 2) {	/* Pan two phosphors at a time */
      pixoff = 0;		/* Round down to nearest mod 2 boundary */
   } else {
      if ((zoom % 2)==0) hoff &= 0xFE;	/* Pan two phosphors at a time */
      temp = 0;
      while ((((temp * zoom) + hoff) % 4) != 0) {
         temp += 1;
	 if (pix_origin == 0) {
	    pix_origin = 0xFFFFF;	/* Wrap to bottom of screen */
	 } else {
	    pix_origin -= 1;
	 }
      }
      pixoff = ((temp * zoom) + hoff) / 4;
   }
      
   /* Test for illegal zoom and pan combination */
   if (pixoff > 15) printf("Procedure Set_Zoom: Program Error.\n");
   temp = pixoff + zoom * (3 + ((pix_origin % 0x10) / 4)); 
   if (temp  > bad_zoom[(zoom-1)]) { 
   /*   printf("Procedure Set_Zoom: Illegal Pan with given Zoom.\n"); */
   
   } else {
      wpan = (ushort)(pix_origin >> 4);
      ppan = (uchar)((pix_origin << 4) + pixoff);
      zreg = (uchar)((voff << 4) + (zoom-1));

      /* initialize pointers before waiting for retrace. Try to be as 
         speedy as possible. */
      PSC_WPan = (ushort*)(SC_Ctrl + 0x00B000);/* Word Pan Register pointer */
      PSC_Zoom = (uchar*)(SC_Ctrl + 0x00C001);/* Zoom Register Pointer*/
      PSC_PPan = (uchar*)(SC_Ctrl + 0x00D001);/* Pix Offset Register Ptr */
      PSC_Stat = (ushort*)(SC_Ctrl + 0x009000);

      while (*PSC_Stat & VBlank);/* Don't Update if about to latch new values */
      
      *PSC_WPan = wpan;
      *PSC_PPan = ppan;
      /* printf("DEBUG: Writing 0x%x to SC_Zoom.\n",zreg); */
      *PSC_Zoom = zreg;
   }
}			/* End of Procedure Set_Zoom */
   

/* ======================================================================
   Author: Peter Costello
   Date :  April 15, 1983
   Purpose: This routine performs a smooth scrolling from the current
	pan origin to a new origin defined by the parameters DX and DY.
	If the resulting pan were to cause the screen to wrap, the 
	routine aborts and returns false.
   Coordinate System: The coordinates used are in screen phosphors. For
	instance, at zoom 1 (range 0..15) the resolution of the frame
	buffer can be thought of as 2304 by 1800.
   Algorithm: Error Handling:
   Bugs:
   ====================================================================== */
Pan_Dx_Dy(dx,dy,speed)
   int dx,dy,speed;
{
   register int zoom;
   register int x,y;
   /* register int pix_origin,voff,hoff;  */
   
   /* The following code which has been commented out does not quite work
      because the resolution of 'X' is sometimes 2 (or 4) and dx may only
      change by 1, so this routine might never increment 'X'. */
      
   /* Read current zoom and pan parameters from color board. 
    * zoom = (SC_Zoom & 0x0F)+1;
    * pix_origin = (SC_WPan << 4) + (SC_PPan >> 4);
    * hoff = (SC_PPan & 0xF)*4;
    * voff = (zoom - 1) - (SC_Zoom >> 4);

    * Reconstruct current origin * 
    * while (hoff >= zoom) {
    *    hoff -= zoom;
    *    pix_origin += 1;
    * }
    * pix_origin &= 0xFFFFF;	 % Handles case where we wrap to line 0

    * if (hoff < 0) printf("Procedure Pan_Dx_Dy: Program Error.\n");
    * x = (pix_origin % SCWidth)*zoom + hoff;
    * y = (pix_origin / SCWidth)*zoom + voff;
    */
   
   /* Compute new origin */
   zoom = (SC_Zoom & 0x0F)+1;
   x = SC_X + dx;
   y = SC_Y + dy;

   if ((x < 0)||(x >= SCWidth*zoom)) return(0);		/* Exit. Error */
   if ((y < 0)||(y >= SCHeight*zoom)) return(0);	/* Exit. Error */

   while (speed-- > 0) {
      while (!SC_Retrace);		/* Wait for retrace to start */
      while (SC_Retrace);		/* Wait for retrace to end */
   }

   Set_Zoom(x,y,zoom-1);
   
   return(1);				/* Exit. No Error. */
}			/* End of Procedure Pan_Dx_Dy */


/* ======================================================================
   Author: Peter Costello
   Date :  April 15, 1983
   Purpose: Fill in a region with a color.
   Algorithm:
   Error Handling: Dx, Dy are assumed to be positive.
	Coordinate space is (0 <= x < 1152), (0 <= y < 910).
   Bugs:
   ====================================================================== */
Region(x,y,dx,dy,color)
   int x,y,dx,dy;
   uchar color;
{
   register int rdx, rdy, rSCWidth;
   register uchar *addr, rcolor;
   register short i;

   printf("X = %d. Y = %d. DX = %d. DY = %d. COLOR = 0x%x\n",x,y,dx,dy,color);
   if ((dx < 1)||(dy < 1)) {
      printf("Region(): Dx and Dy must be positive (origin @ upper left).\n");
   } else if (((y+dy)*SCWidth+x+dx)>0x100000) {
      printf("Region(): Area too large.\n");
   } else {
      addr = SC_Pix + y*SCWidth + x;
      rdx = dx;
      rdy = dy;
      rSCWidth = SCWidth;
      rcolor = color;
      while (dy-- > 0) {
         i = dx;
         /* print_xy(addr); */
         while (i-- > 0) {
	    *addr++ = color;
         }
         addr -= dx;
         addr += SCWidth;
      }
   }
}			/* End of Procedure Region */

