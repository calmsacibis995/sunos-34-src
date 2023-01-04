
static char	sccsid[] = "@(#)colorvect.c 1.1 9/25/86 Copyright SMI";


/* ======================================================================
   Author: Peter Costello
   Date :  April 21, 1982
   Purpose: This program draws a vector in the color frame buffer. 
      The vector will go from location (x,y) to (x+dx,y+dy).
   Algorithm: This program uses a Bresenham Algorithm. 
   Error Handling:
   ====================================================================== */


#include "colorbuf.h"

vector(x,y,dx,dy,color)
   short x,y,dx,dy,color;

{
   char unsigned register 
      *addr1,         /* destination x or y address */
      *addr2;         /* destination y or x address */
   register short
      i,lgdelta,smdelta,error,quadrant;
   uchar register
      ccolor,sreg_old;

   ccolor = (uchar)color;
   sreg_old = *GR_sreg;			/* Save Status reg */
   *GR_sreg  &= (~GR_paint);     	/* Exit Paint-Mode */

   if (dy == 0) {    /* This is a special case, mabye go into 
                       paint-mode to draw five pixels at a time */
      addr2 = (uchar *)((GR_bd_sel + GR_y_select) + y);
      *addr2 = TOUCH;      /* Set y write address register */
      addr1 = (uchar *)((GR_bd_sel + GR_x_select + GR_update) + x);

      if (dx == 0) {   /* Most common call to this routine */
         *addr1 = ccolor;
     
      } else if (dx > 15) {  /* Use paint mode for middle of segment */
         /* Set first four pixels on line individually */
         for (i=4;i>0;i--) {
            *addr1++ = ccolor;
         }
         
         /* Go into paint mode to write five pixels at a time */
         *GR_sreg |= GR_paint;
         for (i=dx-7;i>0;i-=5) {    /* advance x addr five at a time */
            *addr1 = ccolor;
            addr1  += 5;
         }
       
         /* advance to fifth pixel from second endpoint. Write in paint mode */
         addr1 = (uchar *)
                 ((GR_bd_sel + GR_x_select + GR_update) + (x + dx - 4));
         *addr1++ = ccolor;

         /* Go out of PAINT mode, and write last four pixels one at a time */
         *GR_sreg &= (~GR_paint);
         for (i=4;i>0;i--) {
            *addr1++ = ccolor;
         }

      } else if (dx < -15) {
         /* Set first four pixels on line individually */
         for (i=4;i>0;i--) {
            *addr1-- = ccolor;
         }
         
         /* Go into paint mode to write five pixels at a time */
         *GR_sreg |= GR_paint;
         for (i=dx+7;i<0;i+=5) {  /* decr x addr five at a time */
            *addr1 = ccolor;
            addr1  += 5;
         }

         /* Go 5 pixels from second endpoint. Write one in paint mode. */
         addr1 = (uchar *)
                 ((GR_bd_sel + GR_x_select + GR_update) + (x + dx + 4));
         *addr1-- = ccolor;

         /* Go out of PAINT mode */
         *GR_sreg &= ~GR_paint;
         /* Write last four pixels one at a time */
         for (i=4;i>0;i--) {
            *addr1-- = ccolor;
         }


      } else if (dx < 0) {
         /* Step through all pixels individually */
         for (i=dx;i<=0;i++) {
            *addr1-- = ccolor;
         }
      } else {
         /* Step  through all pixels individually */
         for (i=dx;i>=0;i--) {
            *addr1++ = ccolor;
         }
      }
   } else if (dx == 0) {  /* set y address reg to update reg */
      addr1 = (uchar *)((GR_bd_sel + GR_x_select) + x);
      *addr1 = TOUCH;
      addr2 = (uchar *)((GR_bd_sel + GR_y_select + GR_update) + y);

      if (dy > 0) {   /* Increment y address */
         for (i=dy;i>=0;i--) {
            *addr2++ = ccolor;
         } 

      } else {
         for (i=dy;i<=0;i++) {
            *addr2-- = ccolor;
         }
      }
   } else {  /* Do generalized processing */
      /* Compute which quadrant we are in:
           3 = traditional quadrant I.
           1 = traditional quadrant II.
           0 = traditional quadrant III.
           2 = traditional quadrant IV.       */
      quadrant = ((dx > 0)*2 + (dy > 0));

      /* make dx and dy positive */
      if (dy < 0) dy *= -1;
      if (dx < 0) dx *= -1;

      if (dy > dx) {
         lgdelta = dy;
         smdelta = dx;
         addr1 = (uchar *)((GR_bd_sel + GR_y_select + GR_update) + y);
         addr2 = (uchar *)((GR_bd_sel + GR_x_select) + x);
         if (quadrant == 1) {
            quadrant = 2;
         } else if (quadrant == 2) {
            quadrant = 1;
         }
      } else {
         lgdelta = dx;
         smdelta = dy;
         addr1 = (uchar *)((GR_bd_sel + GR_x_select + GR_update) + x);
         addr2 = (uchar *)((GR_bd_sel + GR_y_select) + y);
      }
      /* The quadrant assignments have now been modified. They no
         longer map to cartesian coordinates. The quadrant number
         now carries the following information:
            quadrant = 3; lgdelta positive; smdelta positive.
            quadrant = 0; lgdelta negative; smdelta negative.
            quadrant = 2; lgdelta positive; smdelta negative.
            quadrant = 1; lgdelta negative; smdelta positive.
      */

      i = lgdelta;
      error = 2*smdelta - lgdelta;
      *addr2 = TOUCH;   /* Set address Reg of slowest varying coord */

      switch( quadrant ) {
         case 3: {          /* Both deltas Positive */
            do {
               *addr1++ = ccolor;     /* Write out a pixel */
               if (error > 0) {
                  *++addr2 = TOUCH;  /* Incr slowest varying coord */
                  error += 2*(smdelta - lgdelta);
               } else {
                  error += 2*smdelta;
               }
            } while (--i > 0);
            break;
         }
         case 0: {          /* Both deltas Negative */
            do {
               *addr1-- = ccolor;     /* Write out a pixel */
               if (error > 0) {
                  *--addr2 = TOUCH;  /* decr slowest varying coord */
                  error += 2*(smdelta - lgdelta);
               } else {
                  error += 2*smdelta;
               }
            } while (--i > 0);
            break;
         }
         case 2: {         /* Lgdelta positive. Smdelta negative. */
            do {
               *addr1++ = ccolor;     /* Write out a pixel */
               if (error > 0) {
                  *--addr2 = TOUCH;  /* Decr slowest varying coord */
                  error += 2*(smdelta - lgdelta);
               } else {
                  error += 2*smdelta;
               }
            } while (--i > 0);
            break;
         }
         case 1: {         /* Lgdelta negative. Smdelta positive. */
            do {
               *addr1-- = ccolor;     /* Write out a pixel */
               if (error > 0) {
                  *++addr2 = TOUCH;  /* Incr slowest varying coord */
                  error += 2*(smdelta - lgdelta);
               } else {
                  error += 2*smdelta;
               }
            } while (--i > 0);
            break;
         }
         default : {        /* This is an error */
            printf("ERROR in COLORVECT.C\n");
         }
      }   
   }
   *GR_sreg = sreg_old;		/* Restore Status Register */
}        /* End of function VECTOR */
