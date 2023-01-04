

/* ======================================================================
   Author: Peter Costello
   Date :  April 15, 1983
   Purpose:  Perform auto and manual tests for zoom and pan.
   Algorithm:
   Timing:
   Error Handling:
   Bugs:
   ====================================================================== */
static char     sccsid[] = "@(#)sczoom.c 1.1 9/25/86 Copyright Sun Micro";

#include "sc.diag.h"
#define wait_sec(sec) i=sec;j=Sec1;while (i-- > 0) while (j-- > 0)

/* Auto Zoom and Pan Tests. For each zoom factor, move origin down & up,
   to lower-right and back, to right, to lower_left and back, and left to
   original starting point. Now test that the lower 'N' lines do not zoom
   when the rest does. */
a_zoom(zoom,speed)
   int zoom,speed;
{
   Set_Zoom(0,0,zoom);			/* Go to origin and set new zoom */
   while (Pan_Dx_Dy(0,1,speed)); 	/* Move origin down */
   while (Pan_Dx_Dy(0,-1,speed));	/* Move origin up */
   while (Pan_Dx_Dy(1,0,speed));	/* Move origin right */
   while (Pan_Dx_Dy(-1,0,speed));	/* Move origin left */
   while (Pan_Dx_Dy(1,1,speed));	/* Move origin right and down */
   while (Pan_Dx_Dy(-1,-1,speed));	/* Move origin left and up */
}		/* End of routine to test zoom */
       
/* Do auto zoom tests */
auto_zoom(list_p)
   struct bd_list *list_p;
{
   short zoom;

 /*  for (zoom=0;zoom<16;zoom++) { */
  for (zoom=0;zoom<8;zoom++) { 
      printf (" zoom = %d \n", zoom);
      schecker();			/* Draw a checkerboard */
      SC_Stat = VEnable;
      a_zoom(zoom,0);	  		/* Try 0 sec between moves */
      schecker_verify(list_p);		/* Verify memory */
   }
   Set_Zoom(0,0,0);
}
   
manzoom(list_p)
   struct bd_list *list_p;
{
  char ch;
  ushort data;
  int x,y,x1,y1,dx,dy,i,j,more;

  more = 1;
  while (more) {
   printf("Zoom and Pan Tests\n");
   printf("   1: Alter Zoom\n");
   printf("   2: Alter Origin (Absolute)\n");
   printf("   3: Alter Origin (Relative)\n");
   printf("   4: Set No Zoom Line Number\n");
   printf("   5: Excercise Pan\n");
   printf("   6: Auto Test\n");
   printf("   7: Toggle Origin\n");
   printf("   Q: Quit\n");
   printf("   Enter Choice: ");
   ch = getchar(); printf("\n");

   if (ch == '1') {
      printf("   Enter New Zoom Factor (0..9): ");
      i = getn(); printf("\n");
      data = SC_Zoom;
      data &= 0x00F0;
      data |= i & 0x000F;
      SC_Zoom = data;
   } else if (ch == '2') {
      printf("   Enter X0 (0*zoom..1152*zoom): ");
      x = getn(); printf("\n");
      printf("   Enter Y0 (0*zoom..899*zoom): ");
      y = getn(); printf("\n");
      j = SC_Zoom & 0xF;
      Set_Zoom(x,y,j);
   } else if (ch == '3') {
      printf("   Enter DX: ");
      dx = getn(); printf("\n");
      printf("   Enter DY: ");
      dy = getn(); printf("\n");
      i = Pan_Dx_Dy(dx,dy,0);
   } else if (ch == '4') {
      printf("   Enter No Zoom Line Number (0-1024): ");
      data = getn(); printf("\n");
      SC_VZoom = (data>>2);
   } else if (ch == '5') {
      printf("   Enter number of pixels motion per frame: ");
      i = getn(); printf("\n");
      j = SC_Zoom & 0xF;
      a_zoom(j,i);
   } else if (ch == '6') {
      auto_zoom(list_p);
   } else if (ch == '7') {
      if (SCWidth==1024) printf("   Enter X0 (0*zoom..1024*zoom): ");
      if (SCWidth==1152) printf("   Enter X0 (0*zoom..1152*zoom): ");
      x = getn(); printf("\n");
      if (SCWidth==1024) printf("   Enter Y0 (0*zoom..1023*zoom): ");
      if (SCWidth==1152) printf("   Enter Y0 (0*zoom..899*zoom): ");
      y = getn(); printf("\n");
      if (SCWidth==1024) printf("   Enter X1 (0*zoom..1024*zoom): ");
      if (SCWidth==1152) printf("   Enter X1 (0*zoom..1152*zoom): ");
      x1 = getn(); printf("\n");
      if (SCWidth==1024) printf("   Enter Y1 (0*zoom..1023*zoom): ");
      if (SCWidth==1152) printf("   Enter Y1 (0*zoom..899*zoom): ");
      y1 = getn(); printf("\n");
      while (1) {
         while (!SC_Retrace);		/* Wait */
         Set_Zoom(x,y,(int)SC_Zoom&0x000F);
         while (SC_Retrace);		/* Wait */
         while (!SC_Retrace);		/* Wait */
         Set_Zoom(x1,y1,(int)SC_Zoom&0x000F);
         while (SC_Retrace);		/* Wait */
      }
   } else if ((ch == 'q')||(ch == 'Q')) {
      more = 0;
   }
  } 
}		/* End of routine manzoom() */
   

