
/* ======================================================================
   Author: Peter Costello
   Date :  April 15, 1983
   Purpose: Test out DACs in sun-2 color board.
   Algorithm:
   Timing:
   Error Handling:
   Bugs:
   ====================================================================== */
static char     sccsid[] = "@(#)scdac.c 1.1 9/25/86 Copyright Sun Micro";

#include "sc.diag.h"

uint store[288];
quad_hramp(y1,y2,cbase)
   int y1,y2,cbase;
{
   register int i,j;
   register uint *addr,*st;
   register uchar *caddr;
   
   caddr = SC_Pix; addr = SC_Pixl; SC_Mask = 0xff;
   if (SCWidth==1152) {
      for (i=0;i<64;i++) *caddr++ = 0;
      for (i=64;i<568;i++) *caddr++ = (uchar)(cbase + ((i-56)>>3));
      for (i=568;i<1078;i++) *caddr++ = (uchar)(cbase + ((1077-i)>>3));
      for (i=1078;i<1152;i++) *caddr++ = 0;
      for (i=0;i<288;i++) store[i] = *addr++;
      for (i=1;i<900;i++) {
	 st = store;
  	 for (j=18;j>0;j--) {
	    *addr++ = *st++; *addr++ = *st++; *addr++ = *st++; *addr++ = *st++;
	    *addr++ = *st++; *addr++ = *st++; *addr++ = *st++; *addr++ = *st++;
	    *addr++ = *st++; *addr++ = *st++; *addr++ = *st++; *addr++ = *st++;
	    *addr++ = *st++; *addr++ = *st++; *addr++ = *st++; *addr++ = *st++;
	 }
      }
  } else {
      for (i=0;i<512;i++) *caddr++ = (uchar)(cbase+(i>>3));
      for (i=512;i<1024;i++) *caddr++ = (uchar)(cbase+((1023-i)>>3));
      for (i=0;i<256;i++) store[i] = *addr++;
      for (i=1;i<1024;i++) {
	 st = store;
  	 for (j=16;j>0;j--) {
	    *addr++ = *st++; *addr++ = *st++; *addr++ = *st++; *addr++ = *st++;
	    *addr++ = *st++; *addr++ = *st++; *addr++ = *st++; *addr++ = *st++;
	    *addr++ = *st++; *addr++ = *st++; *addr++ = *st++; *addr++ = *st++;
	    *addr++ = *st++; *addr++ = *st++; *addr++ = *st++; *addr++ = *st++;
	 }
      }
  }
}

rgbw_hramp()
{
   if (SCWidth==1152) {
      quad_hramp(0,224,0);
      quad_hramp(225,449,64);
      quad_hramp(450,674,128);
      quad_hramp(675,905,192);
   } else {
      quad_hramp(  0,255,0);
      quad_hramp(256,511,64);
      quad_hramp(512,767,128);
      quad_hramp(768,1023,192);
   }
}

single_hramp(y1,y2,cbase)
   int y1,y2,cbase;
{
   register int i,j;
   register uint *addr,*st;
   register uchar *caddr;

   caddr = SC_Pix; addr = SC_Pixl; SC_Mask = 0xff;
   if (SCWidth == 1152) {
      for (i=0;i<64;i+=1) *caddr++ = (uchar)(cbase+((63-i)>>1));
      for (i=64;i<574;i+=1) *caddr++ = (uchar)(cbase+((i-62)>>1));
      for (i=574;i<1083;i+=1) *caddr++ = (uchar)(cbase+((1082-i)>>1));
      for (i=1083;i<1152;i+=1) *caddr++ = (uchar)(cbase+((i-1081)>>1));
      for (i=0;i<288;i+=1) store[i] = *addr++;
      for (i=1;i<900;i+=1) {
	 st = store;
  	 for (j=18;j>0;j-=1) {
	    *addr++ = *st++; *addr++ = *st++; *addr++ = *st++; *addr++ = *st++;
	    *addr++ = *st++; *addr++ = *st++; *addr++ = *st++; *addr++ = *st++;
	    *addr++ = *st++; *addr++ = *st++; *addr++ = *st++; *addr++ = *st++;
	    *addr++ = *st++; *addr++ = *st++; *addr++ = *st++; *addr++ = *st++;
	 }
      }
   } else {
      for (i=0;i<512;i+=1) *caddr++ = (uchar)(cbase+(i>>1));
      for (i=512;i<1024;i+=1) *caddr++ = (uchar)(cbase+((1023-i)>>1));
      for (i=0;i<256;i+=1) store[i] = *addr++;
      for (i=1;i<1024;i+=1) {
	 st = store;
  	 for (j=16;j>0;j-=1) {
	    *addr++ = *st++; *addr++ = *st++; *addr++ = *st++; *addr++ = *st++;
	    *addr++ = *st++; *addr++ = *st++; *addr++ = *st++; *addr++ = *st++;
	    *addr++ = *st++; *addr++ = *st++; *addr++ = *st++; *addr++ = *st++;
	    *addr++ = *st++; *addr++ = *st++; *addr++ = *st++; *addr++ = *st++;
	 }
      }
   }
}

double_hramp(y1,y2,cbase)
   int y1,y2,cbase;
{
   register int i,j;
   register uint *addr,*st;
   register uchar *caddr;

   caddr = SC_Pix; addr = SC_Pixl; SC_Mask = 0xff;
   if (SCWidth==1152) {
      for (i=0;i<64;i++) *caddr++ = (uchar)cbase+(63-i);
      for (i=64;i<319;i++) *caddr++ = (uchar)cbase+(i-63);
      for (i=319;i<574;i++) *caddr++ = (uchar)cbase+(573-i);
      for (i=574;i<829;i++) *caddr++ = (uchar)cbase+(i-573);
      for (i=829;i<1084;i++) *caddr++ = (uchar)cbase+(1083-i);
      for (i=1084;i<1152;i++) *caddr++ = (uchar)cbase+(i-1083);
      for (i=0;i<288;i++) store[i] = *addr++;
      for (i=1;i<900;i++) {
	 st = store;
  	 for (j=18;j>0;j--) {
	    *addr++ = *st++; *addr++ = *st++; *addr++ = *st++; *addr++ = *st++;
	    *addr++ = *st++; *addr++ = *st++; *addr++ = *st++; *addr++ = *st++;
	    *addr++ = *st++; *addr++ = *st++; *addr++ = *st++; *addr++ = *st++;
	    *addr++ = *st++; *addr++ = *st++; *addr++ = *st++; *addr++ = *st++;
	 }
      }
   } else {
      for (i=0;i<256;i++) *caddr++ =(uchar)cbase+i;
      for (i=256;i<512;i++) *caddr++ = (uchar)cbase+(511-i);
      for (i=512;i<768;i++) *caddr++ = (uchar)cbase+(i-511);
      for (i=768;i<1024;i++) *caddr++ = (uchar)cbase+(1023-i);
      for (i=0;i<256;i++) store[i] = *addr++;
      for (i=1;i<1024;i++) {
	 st = store;
  	 for (j=16;j>0;j--) {
	    *addr++ = *st++; *addr++ = *st++; *addr++ = *st++; *addr++ = *st++;
	    *addr++ = *st++; *addr++ = *st++; *addr++ = *st++; *addr++ = *st++;
	    *addr++ = *st++; *addr++ = *st++; *addr++ = *st++; *addr++ = *st++;
	    *addr++ = *st++; *addr++ = *st++; *addr++ = *st++; *addr++ = *st++;
	 }
      }
   }
}

vert_ramp(x1,x2,cbase)
   int x1,x2,cbase;
{
   int i;
   if (SCWidth==1152) {
      for (i=0;i<512;i++) vector(x1,i,x2,i,(uchar)cbase+(i>>1));
      for (i=512;i<900;i++) vector(x1,i,x2,i,(uchar)cbase+((1021-i)>>1));
   } else {
      for (i=0;i<512;i++) vector(x1,i,x2,i,(uchar)cbase+(i>>1));
      for (i=512;i<1024;i++) vector(x1,i,x2,i,(uchar)cbase+((1021-i)>>1));
   }
}

/* Draw borders on monitor */
draw_borders()
{
   register uint *addr;
   register int i;

   Enable_All_Planes;
   addr = SC_Pixl;
   for (i=0X4000;i>0;i--) {
      *addr++ = 0; *addr++ = 0; *addr++ = 0; *addr++ = 0;
      *addr++ = 0; *addr++ = 0; *addr++ = 0; *addr++ = 0;
      *addr++ = 0; *addr++ = 0; *addr++ = 0; *addr++ = 0;
      *addr++ = 0; *addr++ = 0; *addr++ = 0; *addr++ = 0;
   }

   load_white(); 		/* Color 0 is Black. Color 255 is White. */
   if (SCWidth==1152) {
      vector(0,0,1150,0,255);
      vector(1150,0,1150,899,255);
      vector(1150,899,0,899,255);
      vector(0,899,0,0,255);
   } else {
      vector(0,0,1022,0,255);
      vector(1022,0,1022,1022,255);
      vector(1022,1022,0,1022,255);
      vector(0,1022,0,0,255);
   }
}	/* End of routine to draw borders */

load_red()
{ ushort *red,*grn,*blu;
  int i;
  SC_Stat = VEnable;
  red = SC_Red_Cmap; grn = SC_Grn_Cmap; blu = SC_Blu_Cmap;
  for (i=0;i<256;i++) {*red++ = (ushort)i; *grn++ = 0; *blu++ = 0;}
  SC_Stat = VEnable | UpECmap;
}

load_grn()
{ ushort *red,*grn,*blu;
  int i;
  SC_Stat = VEnable;
  red = SC_Red_Cmap; grn = SC_Grn_Cmap; blu = SC_Blu_Cmap;
  for (i=0;i<256;i++) {*red++ = 0; *grn++ = (ushort)i; *blu++ = 0;}
  SC_Stat = VEnable | UpECmap;
}

load_blu()
{ ushort *red,*grn,*blu;
  int i;
  SC_Stat = VEnable;
  red = SC_Red_Cmap; grn = SC_Grn_Cmap; blu = SC_Blu_Cmap;
  for (i=0;i<256;i++) {*red++ = 0; *grn++ = 0; *blu++ = (ushort)i;}
  SC_Stat = VEnable | UpECmap;
}

load_white()
{ ushort *red,*grn,*blu;
  int i;
  SC_Stat = VEnable;
  red = SC_Red_Cmap; grn = SC_Grn_Cmap; blu = SC_Blu_Cmap;
  for (i=0;i<256;i++) {*red++=(ushort)i; *grn++=(ushort)i; *blu++=(ushort)i;}
  SC_Stat = VEnable | UpECmap;
}

load_stable()
{ ushort *red,*grn,*blu;
  int i;
  SC_Stat = VEnable;
  red = SC_Red_Cmap; grn = SC_Grn_Cmap; blu = SC_Blu_Cmap;
  *red++=(ushort)0; *grn++=(ushort)0; *blu++=(ushort)0;
  for (i=1;i<255;i++) {*red++=(ushort)0xFF; *grn++=(ushort)0; *blu++=(ushort)0;}
  *red++=(ushort)0xFF; *grn++=(ushort)0xFF; *blu++=(ushort)0xFF;
  SC_Stat = VEnable | UpECmap;
}

load_black()
{ ushort *red,*grn,*blu;
  int i;
  SC_Stat = VEnable;
  red = SC_Red_Cmap; grn = SC_Grn_Cmap; blu = SC_Blu_Cmap;
  for (i=0;i<256;i++) {*red++ = 0; *grn++ = 0; *blu++ = 0;}
  SC_Stat = VEnable | UpECmap;
}

load_colors()
{ ushort *red,*grn,*blu;
  int i;
  SC_Stat = VEnable;
  red = SC_Red_Cmap; grn = SC_Grn_Cmap; blu = SC_Blu_Cmap;
  for (i=0;i<256;i+=4) {*red++ = (ushort)i; *grn++ = 0; *blu++ = 0;}
  for (i=0;i<256;i+=4) {*red++ = 0; *grn++ = (ushort)i; *blu++ = 0;}
  for (i=0;i<256;i+=4) {*red++ = 0; *grn++ = 0; *blu++ = (ushort)i;}
  for (i=0;i<256;i+=4) {*red++=(ushort)i; *grn++=(ushort)i; *blu++=(ushort)i;}
  SC_Stat = VEnable | UpECmap;
}

/* Write 16 pixels with one color, next 16 with another */
alt_colors()
{ register ushort *red,*grn,*blu;
  register uchar *addr;
  register uchar i,j;

  SC_Stat = VEnable;
  red = SC_Red_Cmap; grn = SC_Grn_Cmap; blu = SC_Blu_Cmap;
  for (i=0;i<256;i+=1) {*red++ = 0xFF; *grn++ = 0; *blu++ = 0;}

  i = 0x7F;
  j = 0x80;
  red[i] = 0; grn[i] = i; blu[i] = 0;
  red[j] = 0; grn[j] = j; blu[j] = 0;
  SC_Stat = VEnable | UpECmap;

  for (addr=SC_Pix;addr<SC_Pixt; ) {
      *addr++ = i; *addr++ = i; *addr++ = i; *addr++ = i;
      *addr++ = i; *addr++ = i; *addr++ = i; *addr++ = i;
      *addr++ = i; *addr++ = i; *addr++ = i; *addr++ = i;
      *addr++ = i; *addr++ = i; *addr++ = i; *addr++ = i;
      *addr++ = j; *addr++ = j; *addr++ = j; *addr++ = j;
      *addr++ = j; *addr++ = j; *addr++ = j; *addr++ = j;
      *addr++ = j; *addr++ = j; *addr++ = j; *addr++ = j;
      *addr++ = j; *addr++ = j; *addr++ = j; *addr++ = j;
  }

}	/* End of routine alt_colors() */

screen_stability()
{
  register uint *addr,i;

  load_stable();
  addr = SC_Meml0;
  for (i=0x4000;i>0;i--) {
     *addr++ = 0x55555555; *addr++ = 0x55555555;
     *addr++ = 0x55555555; *addr++ = 0x55555555;
     *addr++ = 0x55555555; *addr++ = 0x55555555;
     *addr++ = 0x55555555; *addr++ = 0x55555555;

     *addr++ = 0x55555555; *addr++ = 0x55555555;
     *addr++ = 0x55555555; *addr++ = 0x55555555;
     *addr++ = 0x55555555; *addr++ = 0x55555555;
     *addr++ = 0x55555555; *addr++ = 0x55555555;
  }
}


#define Sec1 150000	/* Decrementing this takes 1 second. */
wait_sec(sec)
   int sec;
{
   register int i,j;
   for (i=sec;i>0;i-=1) {		
      for (j=Sec1;j>0; ) j -= 1;	
   }
}


#define SEC 4
/* Perform auto tests on DACs and monitor */
auto_dac()
{
   if (SCWidth==1152) single_hramp(0,905,0);
   if (SCWidth==1024) single_hramp(0,1023,0);
   printf("Draw Red, ");
   load_red(); 
   wait_sec(SEC);

   printf("Green, ");
   load_grn(); 
   wait_sec(SEC);

   printf("Blue, ");
   load_blu(); 
   wait_sec(SEC);

   printf("White, ");
   load_white(); 
   wait_sec(SEC);

   printf("All, ");
   rgbw_hramp();
   load_colors();
   wait_sec(SEC);

   printf("Stability,");
   screen_stability();
   wait_sec(SEC);

   printf("Borders.\n");
   draw_borders();
   wait_sec(SEC);
}		/* End of auto DAC tests. */

test_dac()
{
   char ch;

   if (SCWidth==1152) single_hramp(0,905,0);
   if (SCWidth==1024) single_hramp(0,1023,0);
   load_red(); 
   printf("\nCheck red ramp monotonicity (Hit any char to continue)");
   ch = getchar(); printf("\n");

   load_grn(); 
   printf("Check green ramp monotonicity (Hit any char to continue)");
   ch = getchar(); printf("\n");

   load_blu(); 
   printf("Check blue ramp monotonicity (Hit any char to continue)");
   ch = getchar(); printf("\n");

   load_white(); 
   printf("Check white ramp monotonicity (Hit any char to continue)");
   ch = getchar(); printf("\n");

   alt_colors();
   printf("Adjust -5.2 precision voltage to remove DAC glitches.");
   printf(" (Hit any char ...)"); ch = getchar(); printf("\n");
   
   screen_stability();
   printf("Verify that DAC output is a stable gray pattern.");
   printf(" (Hit any char to continue)"); ch = getchar(); printf("\n");

   draw_borders();
   printf("Verify that all screen borders are visible.");
   printf(" (Hit any char to continue)"); ch = getchar(); printf("\n\n");

}		/* End of DAC verification. */


mdac()
{
   short i,more;
   char ch;

   more = 1;
   while (more) {
      printf("D-to-A Tests.\n");
      printf("   1: Print Horizontal Red Ramp\n");
      printf("   2: Print Horizontal Grn Ramp\n");
      printf("   3: Print Horizontal Blu Ramp\n");
      printf("   4: Print Horizontal White Ramp\n");
      /* printf("   5: Print Double Horizontal Red Ramp\n"); */
      /* printf("   6: Print Double Horizontal Grn Ramp\n"); */
      /* printf("   7: Print Double Horizontal Blu Ramp\n"); */
      /* printf("   8: Print Double Horizontal White Ramp\n"); */
      printf("   9: Print Vertical Red Ramp\n");
      printf("   A: Print Vertical Grn Ramp\n");
      printf("   B: Print Vertical Blu Ramp\n");
      printf("   C: Print Vertical White Ramp\n");
      printf("   D: Print Simultaneous RGBW horizontal Ramps\n");
      printf("   E: Print Screen Borders x=(0:1152) y=(0:899)\n");
      /* printf("   F: Copy Sun-1 Frame Buffer to memory plane 0\n"); */
      printf("   G: Write alternating bars of color to test DAC glitches\n");
      /* printf("   H: Continuous wait_sec(6)\n"); */
      /* printf("   I: Continuous load_white()\n"); */
      /* printf("   J: Continuous single_hramp()\n"); */
      printf("   K: Continuous auto tests\n");
      printf("   L: Test Screen Stability\n");
      printf("   Q: Quit\n");
      printf("   Enter Choice: ");
      ch = getchar(); printf("\n");

      if (ch == '1') {		/* Horizontal Ramps */
	 load_red(); 
    	 if (SCWidth==1152) single_hramp(0,905,0);
    	 if (SCWidth==1024) single_hramp(0,1023,0);
      } else if (ch == '2') {
	 load_grn(); 
    	 if (SCWidth==1152) single_hramp(0,905,0);
    	 if (SCWidth==1024) single_hramp(0,1023,0);
      } else if (ch == '3') {
	 load_blu(); 
    	 if (SCWidth==1152) single_hramp(0,905,0);
    	 if (SCWidth==1024) single_hramp(0,1023,0);
      } else if (ch == '4') {
	 load_white(); 
    	 if (SCWidth==1152) single_hramp(0,905,0);
    	 if (SCWidth==1024) single_hramp(0,1023,0);

      } else if (ch == '9') {		/* Vertical Ramps */
	 load_red(); 
    	 if (SCWidth==1152) vert_ramp(0,1151,0);
    	 if (SCWidth==1024) vert_ramp(0,1023,0);
      } else if ((ch=='a')||(ch=='A')) {
	 load_grn(); 
    	 if (SCWidth==1152) vert_ramp(0,1151,0);
    	 if (SCWidth==1024) vert_ramp(0,1023,0);
      } else if ((ch=='b')||(ch=='B')) {
	 load_blu(); 
    	 if (SCWidth==1152) vert_ramp(0,1151,0);
    	 if (SCWidth==1024) vert_ramp(0,1023,0);
      } else if ((ch=='c')||(ch=='C')) {
	 load_white(); 
    	 if (SCWidth==1152) vert_ramp(0,1151,0);
    	 if (SCWidth==1024) vert_ramp(0,1023,0);

      } else if ((ch=='d')||(ch=='D')) {	/* RGBW Hor Ramps */
	 load_colors(); 
 	 rgbw_hramp();

      } else if ((ch=='e')||(ch=='E')) {	/* Draw borders */
	 draw_borders();
      } else if ((ch=='g')||(ch=='G')) {
          alt_colors();
	  
      } else if ((ch=='k')||(ch=='K')) {
	 i = 0;
	 while (1) {
  	    auto_dac();
	    printf("Auto DAC Test #%d\n",i++);
	 }
      } else if ((ch=='l')||(ch=='L')) {
	 screen_stability();
      } else if ((ch=='q')||(ch=='Q')) {
	 more = 0;
      }
   }
}	/* End of DAC tests. dac(). */

