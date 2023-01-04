

/* ======================================================================
   Author: Susan Rohani 
   Date :  Jan. 2, 1985
   Purpose: Test out sun-2 color monitor.
   Algorithm:
   Timing:
   Error Handling:
   Bugs:
   ====================================================================== */
static char     sccsid[] = "@(#)briefmon.c 1.1 9/25/86 Copyright Sun Micro";

extern load_colors ();

#include "sc.diag.h"

#define SEC 4

/* This circle drawing algorthim is described in Danielson:
   Incremental Curve Generation IEEE Transaction on Computers, September
   1970.
*/

drwcircl (x_center, y_center, radius, color)


int x_center, y_center, radius, color;

{

  int x_start, y_start, x, y, fvalue;
  int x_real_circle, y_real_circle, x_inc_count, y_inc_count;
  int yy;
  register uchar *addr;
  register int f, a, b, fxa, fxb,  dfdx, dfdy, dx,dy;

#define TRUE 1
#define FALSE 0
 
fvalue = 0;
x=2 * radius;
y = 0;

x_start = x;
y_start = y;

x_inc_count = 0;
y_inc_count = 0;
x_real_circle = radius;
y_real_circle = 0;

dfdy = 2 * y;
dfdx = 2 * x;

do
{
  f = fvalue >= 0;
  a = dfdx   >= 0;
  b = dfdy   >= 0;

  fxa = f ^ a;
  fxb = f ^ b;

/*
  fxa = f ^ ((int)(dfdx >= 0));
  fxb = f ^ ((int)(dfdy >= 0));
*/
  dx = dy = 0;
  if (   fxa &&   b ) dx =  1;
  if ( ! fxa && ! b ) dx = -1;
  if (   fxb && ! a ) dy =  1;
  if ( ! fxb &&   a ) dy = -1;

/*  
         printf("x %3d y %3d fv %3d f %3d dfdx %3d a %1d dfdy %3d b %1d fxa %1d fxb %1d axd %1d bxd %1d dx %2d dy %2d\n",
               x,y,fvalue,f,dfdx,a,
               dfdy,b,fxa,fxb,axd,bxd,dx,dy);
*/
  if ( dx != 0 ) {

     x += dx;
     x_inc_count ++;
     if ( x_inc_count%2 == 0) {
         x_real_circle += dx;
         yy = y_real_circle + y_center;
         addr = SC_Pix + (yy << 10) + (x_real_circle + x_center);
         if (SCWidth == 1152) addr += (yy)<<7;
         *addr = color;
/*         printf("x %10d y %10d \n", x_real_circle, y_real_circle); */
         }
      }

     if ( dy != 0) {
        y += dy;
        y_inc_count++;
        if ( y_inc_count%2 == 0) {
           y_real_circle += dy;
         yy = y_real_circle + y_center;
         addr = SC_Pix + (yy << 10) + (x_real_circle + x_center);
         if (SCWidth == 1152) addr += (yy)<<7;
          *addr = color;
/*         printf("x %10d y %10d \n", x_real_circle, y_real_circle); */
        }
      }
   fvalue += dx * dfdx + dy * dfdy + 1;
 
   dfdx = x<<1;
   dfdy = y<<1;
   }
   while ((x != x_start) || (y != y_start));
 }

/* Perform brief monitor test */
boxes (x_num, y_num, width_box, height_box)
int x_num, y_num, width_box, height_box;
{
int row, column, x_start, y_start, x_SPACE_BOX, y_SPACE_BOX;

x_SPACE_BOX = (SCWidth - width_box) / (x_num - 1);
y_SPACE_BOX = (SCHeight - height_box) / (y_num - 1);

Region(0,0,1152,899,255);
for (row = 0; row < x_num; row++)
   for (column = 0; column < y_num; column++) {
      x_start = row * x_SPACE_BOX;
      y_start = column * y_SPACE_BOX;
      Region (x_start, y_start, width_box, height_box, 0);
   }

}

briefmon()
{
   
   char ch;
   ushort red[256],grn[256],blu[256], i, j;
   int r;
   uchar color;
   load_ramp(red,grn,blu);
   write_cmap(red,grn,blu);

   if (SCWidth==1152) single_hramp(0,905,0);
   if (SCWidth==1024) single_hramp(0,1023,0);
   printf("\nCheck white display (Hit any char to continue)");
   ch = getchar(); printf("\n");
   Region(0,0,1152,899,255);

   printf("Check linearity (Hit any char to continue)");
   ch = getchar(); printf("\n");
   draw_borders();
   vector(576,0,576,SCHeight-1,255);
   vector(0,450,SCWidth-1,450,255);

   printf("Check screen convergence (Hit any char to continue)");
   ch = getchar(); printf("\n");

   vector(576,0,576,SCHeight-1,0);
   vector(0,450,SCWidth-1,450,0);

   for (i=0; i<=(SCWidth/32); i++) {
      vector (i*32, 0, i*32, SCHeight-1, 255);}

   for (i=0; i<=(SCHeight/43); i++) {
      vector (0, i*43, SCWidth-1, i*43, 255);}

   for (i=0; i<(SCWidth/32); i++) {
     for (j=0; j<(SCHeight/43); j++) {
        vector (16+i*32, 21+j*43,16+i*32, 21+j*43, 255);
     }
   }

   printf("(Hit any char to continue)");
   ch = getchar(); printf("\n");

   /* ringing test */
   boxes (3, 3, 100, 100);

   /* Bleeding test */
   printf("(Hit any char to continue)");
   ch = getchar(); printf("\n");
   boxes (10, 10, 8,8);

   /* circle tests */
   load_colors ();
   Region(0,0,1152,899,255);
   r = 10;
   i = 0;
   do {
	   color = 64 * (i/4) + 39 + (i%4)*8;
	   i++;
	   drwcircl (576, 450, r, color & 0xFF);
	   vector (576-r, 450-r, 576-r, 450+r, color & 0xFF);
	   vector (576-r, 450+r, 576+r, 450+r, color & 0xFF);
	   vector (576+r, 450+r, 576+r, 450-r, color & 0xFF);
	   vector (576+r, 450-r, 576-r, 450-r, color & 0xFF);
	   r = (r * 141421) / 100000;
           color += 20;
	   }
   while (r < 440);

   printf("(Hit any char to continue)");
   ch = getchar(); printf("\n");

   /* Run diagnoal lines. used y = mx +b */
}	/* End of briefmon tests.  */

