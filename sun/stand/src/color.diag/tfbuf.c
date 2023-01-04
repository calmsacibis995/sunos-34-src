
static char	sccsid[] = "@(#)tfbuf.c 1.1 9/25/86 Copyright SMI";

/* ======================================================================
   Author: Peter Costello
   Date :  October 21, 1982
   Purpose: Handle all manual and continuous tests for color frame buffer.
   Algorithm:
   Error Handling:
   ====================================================================== */

#include "cdiag.h"

tfbuf(list_p)
   struct bd_list *list_p;
{
  char ch;
  uchar color,data;
  short i,j,quit;

  quit = false;
  *GR_sreg = GR_disp_on;		/* Turn off paint mode, ints, etc */

  while (!quit) {
   printf("   A: write even lines in fbuf. (odd after multibus).\n");
   printf("   B: write odd lines in fbuf.\n");
   printf("   C: write checkerboard.\n");
   printf("   D: Write a vertical line.\n");
   printf("   E: Write a horizontal line.\n");
   printf("   F: Verify a vertical line as correct.\n");
   printf("   G: Print all vertical lines.\n");
   printf("   H: Verify horizontal line.\n");
   printf("   I: Test Frame buffer memory.\n");
   printf("   J: Test Paint-Mode logic.\n");
   printf("   K: Test single Location.\n");
   printf("   L: Fill frame buffer with constant.\n");
   printf("   M: Modified immediate data test #1. Wr,Rd,Rd,Reread.\n");
   printf("   N: Modified immediate data test #2. Wr,Rd,Rd,Rd,Reread.\n");
   printf("   Q: Quit to top level.\n");
   printf("   ENTER CHOICE: ");
   /* scanf("%c",ch); */
   ch = getchar(); printf("\n");

   if ((ch == 'a')||(ch == 'A')) {
      printf("Enter Color (Hex): ");
      /* scanf("%x", color); */
      color = getnh(); printf("\n");
      for (j=480;j>=0;j+= -2) {
         set_fbuf_5x(0,j,128,1,color); 
      }

   } else if ((ch == 'b')||(ch == 'B')) {
      printf("Enter Color (Hex): ");
      /* scanf("%x", color); */
      color = getnh();
      for (j=481;j>=0;j+= -2) {
         set_fbuf_5x(2,j,128,1,color); 
      }

   } else if ((ch == 'c')||(ch == 'C')) {
      Set_CFunc(GR_copy);
      checker();

   } else if ((ch == 'd')||(ch == 'D')) {
      wr_vertical(); 

   } else if ((ch == 'e')||(ch == 'E')) {
      wr_horizontal(list_p);

   } else if ((ch == 'f')||(ch == 'F')) {
      vvert(list_p);     			/* verify line */

   } else if ((ch == 'g')||(ch == 'G')) {
      wr_a_vert();   				/* write all vertical lines. */

   } else if ((ch == 'h')||(ch == 'H')) {
      vhorizon(list_p);     			/* Verify horizontal line */

   } else if ((ch == 'i')||(ch == 'I')) {
      printf("Testing color board memory.\n");
      list_p->error[0] = 0;			/* No errors */
      testmem(list_p,0);
      printf("Memory test complete.");
      if (list_p->error[0] == 0) {
	 printf(" No Errors.\n");
      } else {
	 printf(" %d Errors.\n",list_p->error[0]);
      }
   } else if ((ch == 'j')||(ch == 'J')) {
      tpaint(list_p,0);
      printf("Test of Paint-Mode complete.\n");
   } else if ((ch == 'k')||(ch == 'K')) {
      tloc();
   } else if ((ch == 'l')||(ch == 'L')) {
      fillfbuf();
   } else if ((ch == 'm')||(ch == 'M')) {
      printf("   Data Value (Hex)? ");
      data = getnh(); printf("\n");
      testimm(list_p,0,data);
   } else if ((ch == 'n')||(ch == 'N')) {
      printf("   Data Value (Hex)? ");
      data = getnh(); printf("\n");
      testimm2(list_p,0,data);
   } else if ((ch == 'q')||(ch == 'Q')) {
      quit = true;
   } else {
      printf("   Illegal Input.\n");
   }
  }
}


wr_vertical(list_p)
   struct bd_list *list_p;
{
   int vector();
   int vert_verify();
   short color,line;

   printf("Enter Vertical Column (Hex): ");
   /* scanf("%x", line); */
   line = getnh(); printf("\n");

   printf("Enter Color (Hex): ");
   /* scanf("%x", color); */
   color = getnh(); printf("\n");

   vector(line,0,0,510,color);
   vert_verify(list_p,line,color);   /* Verify */
}

wr_horizontal()
{
   int vector();
   short color,line;

   printf("Enter Horizontal Row (Hex): ");
   /* scanf("%x", line); */
   line = getnh(); printf("\n");

   printf("Enter Color (Hex): ");
   /* scanf("%x", color); */
   color = getnh(); printf("\n");

   vector(0,line,640,0,color);
}


vert_verify(list_p,line,color)
   struct bd_list *list_p;
   short line,color;
{
   /* Verify color along this vertical line */
   uchar register
      *xaddr,*yaddr,ccolor,tcolor;
   short register
      i;

   ccolor = (uchar) color;
   xaddr = (uchar*)(GR_bd_sel + GR_x_select + line);
   yaddr = (uchar*)(GR_bd_sel + GR_y_select + GR_update);
   tcolor = *xaddr;      /* Set x read reg */
   tcolor = *yaddr++;    /* Get read behind value */
    
   for (i=0;i<500;i++) {
      tcolor = *yaddr++;
      if (tcolor != ccolor) {
         printf("Device #%d @ 0x%x. Memory error. X = 0x%x. Y = 0x%x.",
	        list_p->device,list_p->base,line,i);
         printf("Wr 0x%x. Rd 0x%x.\n",ccolor,tcolor);
      }
   } 
}     /* End of function vert_verify */

       
vvert(list_p)
   struct bd_list *list_p;
{
   int vert_verify();
   short color,line;

   printf("Enter Vertical Column (Hex): ");
   /* scanf("%x", line); */
   line = getnh(); printf("\n");

   printf("Enter Color (Hex): ");
   /* scanf("%x", color); */
   color = getnh(); printf("\n");

   printf("Verifying Vertical line.\n");
   vert_verify(list_p,line,color);   /* Verify */
}


wr_a_vert()
{
   int vector();
   int vert_verify();
   short i,color,line;

   color = 0;
   for (i=0;i<256;i++) {
      vector(i,0,0,510,color);
      color++;
   }
   color = 255;
   for (i=256;i<512;i++) {
      vector(i,0,0,510,color);
      color--;
   }
   color = 0;
   for (i=512;i<640;i++) {
      vector(i,0,0,510,color);
      color++;
   }

}
   
       
vhorizon(list_p)
   struct bd_list *list_p;
{
   short register i,color,line;
   uchar register  ccolor,tcolor,*xaddr,*yaddr;

   printf("Enter Horizontal Row (Hex): ");
   /* scanf("%x", line); */
   line = getnh(); printf("\n");

   printf("Enter Color (Hex): ");
   /* scanf("%x", color); */
   color = getnh(); printf("\n");

   ccolor = (uchar)color;
   xaddr = (uchar*)(GR_bd_sel + GR_x_select + GR_set0 + GR_update);
   yaddr = (uchar*)(GR_bd_sel + GR_y_select + GR_set0 + line);
   *yaddr = TOUCH;      /* set y read */
   tcolor = *xaddr++;   /* prefetch */
   for (i=0;i<640;i++){
      tcolor = *xaddr++;
      if (tcolor != ccolor) {
         printf("Device #%d @ 0x%x. Memory error. X = 0x%x. Y = 0x%x.",
	        list_p->device,list_p->base,i,line);
         printf("Wr 0x%x. Rd 0x%x.\n",ccolor,tcolor);
      }
   }
}

/* Continuously write then read data from a single location. */
tloc()
{
   uchar *xaddr,*yaddr;
   int x,y;
   char ch;
   uchar data,temp;

   printf("   Once or Continuously (O/C)? ");
   ch = getchar();
   printf("   X Offset (Hex)? ");
   x = getnh(); printf("\n");
   printf("   Y Offset (Hex)? ");
   y = getnh(); printf("\n");
   printf("   Data (Hex)? ");
   data = (uchar)getnh(); printf("\n");

   xaddr = (uchar*)(GR_bd_sel|GR_x_select|GR_update|GR_set0|x);
   yaddr = (uchar*)(GR_bd_sel|GR_y_select|GR_set0|y);

   Set_CFunc(GR_copy);
   *GR_sreg = GR_disp_on;

   *yaddr = TOUCH;	/* Set y address */
   if ((ch=='c')||(ch=='C')) {
      while (1) {
         *xaddr = data;
         temp = *xaddr;	/* Prefetch */
         temp = *xaddr;
         if (temp != data) {
	    printf("Error.\n");
         }
      }
   } else {
      *xaddr = data;
      temp = *xaddr; temp = *xaddr;
      printf("   Wrote 0x%x. Read 0x%x.\n",data,temp);
   }
}

/* Fill frame buffer with constant data. */
fillfbuf()
{
   uchar *xaddr,*yaddr,data;
   short x,y;

   printf("   Data Value (Hex)? ");
   data = (uchar)getnh(); printf("\n");

   Set_CFunc(GR_copy);
   yaddr = (uchar*)(GR_bd_sel|GR_y_select|GR_set0|0);
   for (y=0;y<512;y++) {
      *yaddr++ = TOUCH;		/* Set y address */
      xaddr = (uchar*)(GR_bd_sel|GR_x_select|GR_update|GR_set0|0);
      for (x=0;x<640;x++) {
         *xaddr++ = data;
      }
   }
}

/* Modified immediate data test. */
testimm(list_p,tnum,data0)
	struct bd_list *list_p;
	int tnum;
	uchar data0;
{
	register uchar *xloc,*yloc,*top_xloc,*top_yloc;
	register uchar data,temp;

	data = data0;
 	top_xloc = (uchar*)(GR_bd_sel|GR_update|GR_x_select|GR_set0|640);
 	top_yloc = (uchar*)(GR_bd_sel|GR_y_select|GR_set0|512);
	Set_CFunc(GR_copy);
 	Set_CMask(0);

	/* Write Data and read immediately. */
	yloc = (uchar*)(GR_bd_sel|GR_y_select|GR_set0|0);
	do {
	   *yloc++ = TOUCH;
	   xloc = (uchar*)(GR_bd_sel|GR_x_select|GR_set0|GR_update|0);
	   do {
	      *xloc = data;
	      temp = *xloc;
	      temp = *xloc++;
	      if (temp != data) {
		 xloc++;	/* increment. Because report subs 2 from x */
	         report("Immediate Data",list_p,tnum,xloc,yloc,data,temp);
		 xloc--;	/* Restore x */
		 xloc--;
		 temp= *xloc;	/* Re-read data */
		 printf(" 	Rereading data. Read 0x%x.",temp);
		 printf(" And retesting Location.\n");	
	      }
 	   } while (xloc < top_xloc);
	} while (yloc < top_yloc);

}	/* End of Immediate data test */

/* Modified immediate data test #2. */
testimm2(list_p,tnum,data0)
	struct bd_list *list_p;
	int tnum;
	uchar data0;
{
	register uchar *xloc,*yloc,*top_xloc,*top_yloc;
	register uchar data,temp;

	data = data0;
 	top_xloc = (uchar*)(GR_bd_sel|GR_update|GR_x_select|GR_set0|640);
 	top_yloc = (uchar*)(GR_bd_sel|GR_y_select|GR_set0|512);
	Set_CFunc(GR_copy);
 	Set_CMask(0);

	/* Write Data and read immediately. */
	yloc = (uchar*)(GR_bd_sel|GR_y_select|GR_set0|0);
	do {
	   *yloc++ = TOUCH;
	   xloc = (uchar*)(GR_bd_sel|GR_x_select|GR_set0|GR_update|0);
	   do {
	      *xloc = data;
	      temp = *xloc;
	      temp = *xloc;	/* Do one extra read. */
	      temp = *xloc++;
	      if (temp != data) {
		 xloc++;	/* increment. Because report subs 2 from x */
	         report("Immediate Data",list_p,tnum,xloc,yloc,data,temp);
		 xloc--;	/* Restore x */
		 xloc--;
		 temp= *xloc;	/* Re-read data */
		 printf(" 	Rereading data. Read 0x%x.",temp);
		 printf(" And retesting Location.\n");	
	      }
 	   } while (xloc < top_xloc);
	} while (yloc < top_yloc);

}	/* End of Immediate data test */


