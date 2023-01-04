
static char	sccsid[] = "@(#)colorbuf.c 1.1 9/25/86 Copyright SMI";

/*  This file contains some routines to load the color map and 
    initialize the color graphics board.
    PWC 2/12/82.      

    Modified for PC board. 10/21/82.
*/


# include "colorbuf.h" 

int CGXBase = 0x1EC000;

/* ===============================================================
   Global Variable Definitions.
   Default colors for color map 0.
   =============================================================== */

unsigned char       /* Must be eight bit numbers */
    gr_red_c0[256] = {
        0, 85,170,255,
    
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

unsigned char    /* Must be eight bits */
    gr_grn_c0[256] = {
        0, 85,170,255,

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

unsigned char
    gr_blu_c0[256] = {
        0, 85,170,255,

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

uchar
   gr1_red_c1[256],
   gr1_grn_c1[256],
   gr1_blu_c1[256];
    
/* ======================================================================
   Author: Peter Costello
   Date :  April 21, 1982
   Purpose: This routine goes into paint mode and write a block of the 
      color frame buffer as quickly as possible. The pixels specified
      will be written only on fixed 5 pixel boundaries. 
   Data: The user is responsible for making sure that the values supplied
      lie in the frame buffer. DX5 and DY must be non-negative.
   Algorithm:
   Error Handling:
   ====================================================================== */

set_fbuf_5x(x,y,dx5,dy,color)
   short x,y,dx5,dy,color;
{

   uchar register 
      *xaddr,
      *yaddr,
      ch_color,
      sreg_data;

   short register
      i,j;

   ch_color = (uchar) color;
   sreg_data = *GR_sreg;	/* Save old state */
   *GR_sreg |= GR_paint;
   yaddr = (uchar *) (GR_bd_sel + GR_y_select + GR_set0 + y);
  
   for (j=dy;j>0;j--) {
      *yaddr++ = ch_color;    /* Set row */
      xaddr = (uchar *) (GR_bd_sel + GR_x_select + GR_update + GR_set0 + x);

      for (i= dx5;i>0;i--) {  
         *xaddr = ch_color;
         xaddr += 5;
      }
   }

   *GR_sreg = sreg_data;	/* Restore color board state */

}    /* End of procedure set_fbuf */



short gr_curr_color;   /* The third of the color map we are writing. 
                          1 = reds. 2 = greens. 3 = blues. */
uchar *gr1_color,*gr2_color,*gr3_color;

/* ======================================================================
   Author: Peter Costello
   Date :  April 21, 1982
   Purpose: This routine takes pointers to three memory arrays with which
      we will load into the color map using the color plane specified.
   Algorithm:
   Error Handling:
   ====================================================================== */

write_cmap(inthandler,color1,color2,color3,new_cplane)
   int inthandler;     /* Really a procedure pointer */
   uchar *color1,*color2,*color3;
   short new_cplane;

{

/* Temp var */
   short j,i;
   uchar *color,tcolor;

   gr1_color = (uchar *)color1;
   gr2_color = (uchar *)color2;
   gr3_color = (uchar *)color3;
   Set_RW_Cmap(new_cplane);
   
   gr_curr_color = 1;    /* Start by writing reds. */

   wrcmap();
      
}    /* End of routine write_cmap */ 



/* ======================================================================
   Author: Peter Costello
   Date :  April 21, 1982
   Purpose: This routine handles the interrupt set up by the routine
      Write_cmap. It loads the color map with the values given, clears
      the interrupt, and sets up the new colorplane in use. 
   Algorithm:
   Error Handling:
   ====================================================================== */

wrcmap()
{
   uchar register
      *color,*color_map;
   short register
      i;

   do {

      while( !GR_retrace);
      if (gr_curr_color==1) {
         color = (uchar *)gr1_color;
         color_map = (uchar *) (GR_bd_sel + GR_red_cmap);
      } else if (gr_curr_color==2) {
         color = (uchar *)gr2_color;
         color_map = (uchar *) (GR_bd_sel + GR_grn_cmap);
      } else {
         color = (uchar *)gr3_color;
         color_map = (uchar *) (GR_bd_sel + GR_blu_cmap);
      } 
   
      /* Do 8 blocks of 32 moves to the color_map */
      for (i=8;i>0;i--) {
         *color_map++ = *color++;
         *color_map++ = *color++;
         *color_map++ = *color++;
         *color_map++ = *color++;
         *color_map++ = *color++;
         *color_map++ = *color++;
         *color_map++ = *color++;
         *color_map++ = *color++;
   
         *color_map++ = *color++;
         *color_map++ = *color++;
         *color_map++ = *color++;
         *color_map++ = *color++;
         *color_map++ = *color++;
         *color_map++ = *color++;
         *color_map++ = *color++;
         *color_map++ = *color++;

         *color_map++ = *color++;
         *color_map++ = *color++;
         *color_map++ = *color++;
         *color_map++ = *color++;
         *color_map++ = *color++;
         *color_map++ = *color++;
         *color_map++ = *color++;
         *color_map++ = *color++;
   
         *color_map++ = *color++;
         *color_map++ = *color++;
         *color_map++ = *color++;
         *color_map++ = *color++;
         *color_map++ = *color++;
         *color_map++ = *color++;
         *color_map++ = *color++;
         *color_map++ = *color++;
   
      }
      /* Go on to next color if we are still in vertical retrace */
      if (GR_retrace) gr_curr_color += 1;

   } while (gr_curr_color <= 3);

}      /* End of function Wrcmap to write colormap in one int cycle */



/* ======================================================================
   Author: Peter Costello
   Date :  April 25, 1982
   Purpose: This routine takes pointers to three memory arrays with which
      we will load from the color map using the color plane specified.
   Algorithm:
   Error Handling:
   ====================================================================== */

read_cmap(inthandler,color1,color2,color3,cplane)
   int inthandler;     /* Really a procedure pointer */
   uchar *color1,*color2,*color3;
   short cplane;
{
   gr1_color = (uchar *) color1;
   gr2_color = (uchar *) color2;
   gr3_color = (uchar *) color3;

   Set_RW_Cmap(cplane);
   
   gr_curr_color = 1;    /* Start by writing reds. */

   rdcmap();
      
}    /* End of routine read_cmap */ 



/* ======================================================================
   Author: Peter Costello
   Date :  April 21, 1982
   Purpose: This routine handles the interrupt set up by the routine
      Read_cmap. It loads the arrays from the color plane specified.
   Algorithm:
   Error Handling:
   ====================================================================== */

rdcmap()
{
   uchar register
      *color, *color_map;
   short register
      i;

   /* uchar data;		-- Turn off interrupts --
      data = *GR_sreg;
      *GR_sreg = 0;
      *GR_sreg = data;
    */

   do {

      while( !GR_retrace);
      if (gr_curr_color==1) {
         color = (uchar *)gr1_color;
         color_map = (uchar *) (GR_bd_sel + GR_red_cmap);
      } else if (gr_curr_color==2) {
         color = (uchar *)gr2_color;
         color_map = (uchar *) (GR_bd_sel + GR_grn_cmap);
      } else {
         color = (uchar *)gr3_color;
         color_map = (uchar *) (GR_bd_sel + GR_blu_cmap);
      } 
   
      /* Do 8 blocks of 32 moves from the color_map */
      for (i=8;i>0;i--) {
         *color++ = *color_map++;
         *color++ = *color_map++;
         *color++ = *color_map++;
         *color++ = *color_map++;
         *color++ = *color_map++;
         *color++ = *color_map++;
         *color++ = *color_map++;
         *color++ = *color_map++;
   
         *color++ = *color_map++;
         *color++ = *color_map++;
         *color++ = *color_map++;
         *color++ = *color_map++;
         *color++ = *color_map++;
         *color++ = *color_map++;
         *color++ = *color_map++;
         *color++ = *color_map++;

         *color++ = *color_map++;
         *color++ = *color_map++;
         *color++ = *color_map++;
         *color++ = *color_map++;
         *color++ = *color_map++;
         *color++ = *color_map++;
         *color++ = *color_map++;
         *color++ = *color_map++;

         *color++ = *color_map++;
         *color++ = *color_map++;
         *color++ = *color_map++;
         *color++ = *color_map++;
         *color++ = *color_map++;
         *color++ = *color_map++;
         *color++ = *color_map++;
         *color++ = *color_map++;
   
      }
      /* Go on to next color in color map if we are still in vertical retrace */
      if (GR_retrace) gr_curr_color += 1;

   } while (gr_curr_color <= 3);


}      /* End of procedure rdcmap */
          

/* ======================================================================
   Author : Peter Costello
   Date   : April 21, 1982
   Purpose : This routine initializes the color board. It clears the 
       frame buffer to color 0. It initializes the color_register to 
       zero. Sets the function register to load data from the multibus,
       and initializes the status register to color_plane zero, display
       on, and all other bits off. 
   Algorithm :
   Error Handling : 
   ====================================================================== */

init_color_fbuf()
{
   int set_fbuf_5x();  /* Routine does a fast clear of the frame buffer */

   /* First disable DAC outputs and everything else. */
   *GR_sreg = 0;

    /* Set function register to write through */
    Set_CFunc(GR_copy);

    /* Clear mask register */
    Set_CMask(0);

    /* Clear the frame buffer */
    set_fbuf_5x(0,0,128,512,0);   /* Set to color 0 */

    /* set sreg to real data */
    *GR_sreg = GR_disp_on;

}   /* End of init_color_fbuf */

          
