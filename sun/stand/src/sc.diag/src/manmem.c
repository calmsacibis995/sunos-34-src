


/* ======================================================================
   Author: Peter Costello
   Date :  October 21, 1982
   Purpose: Handle all manual and continuous tests for color frame buffer.
   Algorithm:
   Error Handling:
   ====================================================================== */
static char     sccsid[] = "@(#)manmem.c 1.1 9/25/86 Copyright Sun Micro";

#include "sc.diag.h"

extern int Paddr;			/* Makes routine scvect.c print addr */
extern int halt_on_err;			/* Makes memtest stop on error */

manmem(list_p)
   struct bd_list *list_p;
{
  char ch,ch1;
  short x,y,dx,dy;
  uchar color;
  register int i,j,k,l,quit;
  register uint *laddr;
  register ushort *addr,data,data1;
  int berr1(), berr2(),berr3();

  quit = false;
  /* SC_Stat = VEnable; */

  while (!quit) {
   printf("   1: write checkerboard.\n");
   printf("   2: Write a vertical line.\n");
   printf("   3: Write a horizontal line.\n");
   printf("   4: Verify a vertical line.\n");
   printf("   5: Verify horizontal line.\n");
   printf("   6: Fill Region with constant.\n");
   printf("   7: Print all vertical lines.\n");
   printf("   8: Print all horizontal lines.\n");
   printf("   9: Test single Location.\n");
   /* printf("   A: Write even horizontal lines in fbuf.\n"); */
   /* printf("   B: Write odd horizontal lines in fbuf.\n"); */
   /* printf("   C: Draw borders.\n"); */
   printf("   D: Auto Test.\n");
   printf("   E: Continuous Auto Test.\n");
   printf("   F: Fill Frame Buffer in Word-Mode.\n");
   printf("   G: Fill One Ram.\n");
   /* printf("   H: Enable prints in routine scvect.c\n"); */
   /* printf("   I: Disable prints in routine scvect.c\n"); */
   /* printf("   J: Write 16 vertical lines in word mode\n"); */
   printf("   K: Write horizontal line in word mode\n");
   printf("   L: Write even vertical lines in fbuf.\n");
   printf("   M: Write odd vertical lines in fbuf.\n");
   printf("   N: Access word-mode location 0. Trap Buserr.\n");
   printf("   O: Pound alternating word locations with data.\n");
   /* printf("   P: Verify all frame buffer in word-mode.\n"); */
   /* printf("   R: Write 32-bit values to Pixel Space. Trap timeout.\n"); */
   printf("   S: Scan word mode memory for a value.\n");
   printf("   T: Fill frame buffer with addresses.\n");
   printf("   U: Verify frame buffer with addresses.\n");
   printf("   Q: Quit.\n");
   printf("   Enter Choice: ");
   ch = getchar(); printf("\n");

   if (ch == '1') {
      schecker();
   } else if (ch == '2') {
      wr_vertical(); 
   } else if (ch == '3') {
      wr_horizontal(); 
   } else if (ch == '4') {
      vvert(); 
   } else if (ch == '5') {
      vhoriz(); 
   } else if (ch == '6') {
      printf("   Enter min X: ");
      x = getn(); printf("\n");
      printf("   Enter min Y: ");
      y = getn(); printf("\n");
      printf("   Enter DX: ");
      dx = getn(); printf("\n");
      printf("   Enter DY: ");
      dy = getn(); printf("\n");
      printf("   Enter Color (Hex): ");
      color = getnh(); printf("\n");
      Region(x,y,dx,dy,color);
   } else if (ch == '7') {
      for (i=0;i<SCWidth;i++) vector(i,0,i,SCHeight-1,((uchar)(i&0xFF)));
   } else if (ch == '8') {
      for (i=0;i<SCHeight;i++) vector(0,i,SCWidth-1,i,((uchar)(i&0xFF)));
   } else if (ch == '9') {
      test_single(list_p);
   } else if ((ch == 'a')||(ch == 'A')) {
      printf("   Enter Color (Hex): ");
      color = getnh(); printf("\n");
      for (i=0;i<SCHeight-1;i+=2) {
	 vector(0,i,SCWidth-1,i,color);
      }
   } else if ((ch == 'b')||(ch == 'B')) {
      printf("   Enter Color (Hex): ");
      color = getnh(); printf("\n");
      for (i=1;i<SCHeight-1;i+=2) {
	 vector(0,i,SCWidth-1,i,color);
      }
   } else if ((ch == 'c')||(ch == 'C')) {
      draw_borders();
   } else if ((ch == 'd')||(ch == 'D')) {
      list_p->error[10] = 0;
      list_p->error[11] = 0;
      printf("Word-Mode or Pix-Mode Tests (w/p)? ");
      ch = getchar(); printf("\n");

      printf("Halt On Errors (y/n)? ");
      ch1 = getchar(); printf("\n");
      halt_on_err = 0;
      if ((ch1=='y')||(ch1=='Y')) halt_on_err = 1;

      if ((ch=='w')||(ch=='W')) {
	 list_p->error[10] += testmem(SC_Meml0,SC_Meml8);
         printf("Word-Mode Errors: %d\n", list_p->error[10]);
      } else {
	 list_p->error[11] += testmem(SC_Pixl,SC_Pixlt);
         printf("Pixel-Mode Errors: %d\n", list_p->error[11]);
      }
   } else if ((ch == 'e')||(ch == 'E')) {
      list_p->error[10] = 0;
      list_p->error[11] = 0;
      i = 0;
      printf("Halt On Errors (y/n)? ");
      ch1 = getchar(); printf("\n");
      halt_on_err = 0;
      if ((ch1=='y')||(ch1=='Y')) halt_on_err = 1;

      while (1) {
         auto_mem(list_p);
	 printf("Passes: %d   Word-Mode Errors: %d    Pix-Mode Errors: %d\n",
	     	i++,list_p->error[10],list_p->error[11]);
      }
   } else if ((ch == 'f')||(ch == 'F')) {
      printf("   Enter Value: ");
      data = getnh(); printf("\n");
      for (addr=SC_Mem0;addr<SC_Mem8;) *addr++ = data;
   } else if ((ch == 'g')||(ch == 'G')) {
      fill_1ram();
   } else if ((ch == 'h')||(ch == 'H')) {
      Paddr = 1;
   } else if ((ch == 'i')||(ch == 'I')) {
      Paddr = 0;
   } else if ((ch == 'j')||(ch == 'J')) {
      write_16vert();
   } else if ((ch == 'k')||(ch == 'K')) {
      write_16hort();
   } else if ((ch == 'l')||(ch == 'L')) {
      printf("   Enter Color (Hex): ");
      color = getnh(); printf("\n");
      for (i=0;i<SCWidth;i+=2) {
	 vector(i,0,i,SCHeight-1,color);
      }
   } else if ((ch == 'm')||(ch == 'M')) {
      printf("   Enter Color (Hex): ");
      color = getnh(); printf("\n");
      for (i=1;i<SCWidth;i+=2) {
	 vector(i,0,i,SCHeight-1,color);
      }
   } else if ((ch == 'n')||(ch == 'N')) {
       printf("   Read, Write or Both (r/w/b)? ");
       ch = getchar(); printf("\n");
       addr = SC_Mem0;
       if ((ch=='r')||(ch=='R')) {
          printf("Reading continuously from Word-Mode Memory...\n");
          *((int*)0x0008) = (int)berr1;		/* Bus Error Routine */
          while (1) {
	     data = *addr; data = *addr; data = *addr; data = *addr; 
	     data = *addr; data = *addr; data = *addr; data = *addr; 
          }
          asm("_berr1:");
	  printf("B");
	  asm("	bset	#7,a7@(8)");		/* Inhibit rerun */
          asm("	rte");
       } else if ((ch=='w')||(ch=='W')) {
	  printf("Writing 0xAAAA continuously to Word-Mode Memory...\n");
	  *((int*)0x0008) = (int)berr2;		/* Bus Error Routine */
	  data = (ushort)0xAAAA;
	  while (1) {
berret:
	     *addr = data; *addr = data; *addr = data; *addr = data; 
	     *addr = data; *addr = data; *addr = data; *addr = data; 
	  }
	  asm("_berr2:");
	  printf("B");
	  asm("	bset	#7,a7@(8)");		/* Inhibit rerun */
          asm("	rte");
          /* asm("	addl	#58,sp");
          goto berret;               		/* Go do it again. */
       } else {
	  printf("Writing 0xAAAA and Reading continuously from");
          printf(" Word-Mode Memory...\n");
	  *((int*)0x0008) = (int)berr3;		/* Bus Error Routine */
	  data = (ushort)0xAAAA;
	  while (1) {
	     *addr = data; data1 = *addr; *addr = data; data1 = *addr;
	     *addr = data; data1 = *addr; *addr = data; data1 = *addr;
	  }
	  asm("_berr3:");
	  printf("B");
	  asm("	bset	#7,a7@(8)");		/* Inhibit rerun */
          asm("	rte");
 	  
       }

   } else if ((ch == 'o')||(ch == 'O')) {
       pound_double();
   } else if ((ch == 'p')||(ch == 'P')) {
       verify_all();
   } else if ((ch == 'r')||(ch == 'R')) {
       pix32_trap();
   } else if ((ch == 's')||(ch == 'S')) {
       scan_value();
   } else if ((ch == 't')||(ch == 'T')) {
       addr = SC_Mem0;
       for (i=0;i<0x100000;) {
 	   *addr++ = i; i+=2; j=21; j+=764;
 	   *addr++ = i; i+=2; j=21; j+=764;
 	   *addr++ = i; i+=2; j=21; j+=764;
 	   *addr++ = i; i+=2; j=21; j+=764;
       }
   } else if ((ch == 'u')||(ch == 'U')) {
       ver_addr();
   } else if ((ch == 'q')||(ch == 'Q')) {
      quit = 1;
   } else {
      printf("   Illegal Input.\n");
   }
  }
}			/* End of procedure scman() */



/* Write a color to a column */
wr_vertical()
{
   uchar color;
   int col;

   printf("   Enter Vertical Column (Hex): ");
   col = getnh(); printf("\n");
   printf("   Enter Color (Hex): ");
   color = getnh(); printf("\n");
   vector(col,0,col,SCHeight-1,color);
}

/* Verify addr at addr */
ver_addr()
{
   register int i;
   register ushort *addr,j,t;
   addr = SC_Mem0; j = 0;
   for (i=0;i<0x100000;i+=2) {
      t = *addr;
      if (t != j&0xFFFF) 
	 printf("Error: Expected: 0x%x, Read 0x%x, Addr 0x%x.\n",j,t,(int)addr);
      addr++; j += 2;
   }
}

/* Write a color to a row */
wr_horizontal()
{
   uchar color;
   int row;

   printf("   Enter Horizontal Row (Hex): ");
   row = getnh(); printf("\n");
   printf("Enter Color (Hex): ");
   color = getnh(); printf("\n");
   vector(0,row,SCWidth-1,row,color);
}

/* Verify color along a column. */
vvert()
{
   uchar color,temp;
   short col,i;
   uchar *addr;

   printf("   Enter Vertical Column (Hex): ");
   col = getnh(); printf("\n");
   printf("   Enter Color (Hex): ");
   color = getnh(); printf("\n");

   addr = SC_Pix + col;
   for (i=0;i<SCHeight;i++) {
      temp = *addr;
      if (temp != color) {
         printf("Error. X = 0x%x. Y = 0x%x. Rd 0x%x.\n",col,i,temp);
      }
      addr += SCWidth;
   }
}


/* Verify color along a row. */
vhoriz()
{
   short row,i;
   uchar color,temp;
   uchar *addr;

   printf("   Enter Horizontal Row (Hex): ");
   row = getnh(); printf("\n");
   printf("   Enter Color (Hex): ");
   color = getnh(); printf("\n");

   addr = SC_Pix + row*SCWidth;
   for (i=0;i<SCWidth;i++) {
      temp = *addr;
      if (temp != color) {
         printf("Error. X = 0x%x. Y = 0x%x. Rd 0x%x.\n",i,row,temp);
      }
      addr++;
   }
}


/* Continuously write then read data from a single location. */
test_single(list_p)
   struct bd_list *list_p;
{
   ushort *waddr;
   uchar  *baddr;
   int i,j,ropc,plane,x,y;
   char ch;
   int berr4();

   *((int*)0x0008) = (int)berr4;		/* Bus Error Routine */
   
   printf("   Addressing Modes:\n");
   printf("   0: Word-Mode Memory\n");
   printf("   1: Pixel-Mode Memory\n");
   printf("   2: Word-Mode with RasterOp\n");
   printf("   3: Pixel-Mode with RasterOp\n");
   printf("   4: Word-Mode with RasterOp and Hidden Read\n");
   printf("   5: Pixel-Mode with RasterOp and Hidden Read\n");
   printf("   6: Parallel Word-Mode with RasterOp\n");
   printf("   7: Parallel Pixel-Mode with RasterOp\n");
   printf("   8: Parallel Word-Mode with RasterOp and Hidden Read\n");
   printf("   9: Parallel Pixel-Mode with RasterOp and Hidden Read\n");
   printf("   Enter Choice: ");
   ch = getchar(); printf("\n");
   i = ch - '0';
   
   if ((i == 0)||(ch == ' ')) {
      waddr = SC_Mem0; j = 21;
   } else if (i == 1) {
      baddr = SC_Pix; j = 9;
   } else {
      ROP_Mode((i-2));
      if ((i % 2)==0) {
         waddr = SC_RMem0; j = 10;
      } else {
         baddr = SC_RPix; j = 20;
      }
   }

   if ((i % 2) == 0) {
      printf("   Enter Plane: ");
      plane = getnh(); printf("\n");
      j += plane;
      waddr += plane*0x10000;
      printf("   Enter Row (hex): ");
      y = getnh(); printf("\n");
      printf("   Enter Word on Row (hex): ");
      x = getnh(); printf("\n");
      waddr += y*72 + x;
      mtest_reg(list_p,j,waddr,(ushort)0xFFFF);

   } else {
      printf("   Enter X (hex): ");
      x = getnh(); printf("\n");
      printf("   Enter Y (hex): ");
      y = getnh(); printf("\n");
      baddr += y*SCWidth + x;
      mtest_breg(list_p,j,baddr,(uchar)0x00FF);

   }
   if (0) {	/* Bus error routine */
      asm("_berr4:");
      printf("B");
      asm("	bset	#7,a7@(8)");		/* Inhibit rerun */
      asm("	rte");
   }
}		/* End of routine test_single */


fill_1ram()
{
   uchar *addr,*taddr;
   char ch; 

   printf("   Enter Plane (0-7): ");
   ch = getchar(); printf("\n");
   SC_Mask = 1 << (ch - '0');

   printf("   Enter Column (0-F): ");
   ch = getchar(); printf("\n");
   taddr = SC_Pix + (ch - '0');

   printf("   Patterns:\n");
   printf("      0: All Zeros\n");
   printf("      1: All Ones\n");
   printf("      2: Alternating Zeros and Ones\n");
   printf("   Choice: ");
   ch = getchar(); printf("\n");

   if (ch == '0') {
      addr = taddr;
      while (addr < SC_Pixt) {
         *addr = 0; addr += 16;
         *addr = 0; addr += 16;
         *addr = 0; addr += 16;
         *addr = 0; addr += 16;
      }
      addr = taddr;
      while (addr < SC_Pixt) {
         *addr = 0; addr += 16;
         *addr = 0; addr += 16;
         *addr = 0; addr += 16;
         *addr = 0; addr += 16;
      }
   } else if (ch == '1') {
      addr = taddr;
      while (addr < SC_Pixt) {
         *addr = 1; addr += 16;
         *addr = 1; addr += 16;
         *addr = 1; addr += 16;
         *addr = 1; addr += 16;
      }
      addr = taddr;
      while (addr < SC_Pixt) {
         *addr = 1; addr += 16;
         *addr = 1; addr += 16;
         *addr = 1; addr += 16;
         *addr = 1; addr += 16;
      }
   } else if (ch == '2') {
      addr = taddr;
      while (addr < SC_Pixt) {
         *addr = 0; addr += 16;
         *addr = 1; addr += 16;
         *addr = 0; addr += 16;
         *addr = 1; addr += 16;
      }
      addr = taddr;
      while (addr < SC_Pixt) {
         *addr = 0; addr += 16;
         *addr = 1; addr += 16;
         *addr = 0; addr += 16;
         *addr = 1; addr += 16;
      }
   } else {
      printf("   Illegal Input\n");
   }
}

write_16vert()
{
   register int i;
   register ushort *addr,data;

   printf("   Enter Plane (0-7): ");
   data = getn(); printf("\n");
   addr = SC_Mem0 + 0x10000*data;
   
   printf("   Enter Word on Row (0-71): ");
   data = getn(); printf("\n");
   addr += data;

   printf("   Enter Datum: ");
   data = getnh(); printf("\n");

   for (i=0;i<SCHeight;i++) {
      *addr = data;
      addr += SCWidth/16;
   }
}		/* End of write_16vert() */

write_16hort()
{
   register int i,j;
   register ushort *addr,data;

   printf("   Enter Plane (0-7): ");
   data = getn(); printf("\n");
   addr = SC_Mem0 + 0x10000*data;
   
   printf("   Enter Row (0-0x400): ");
   data = getnh(); printf("\n");
   addr += data*72;

   printf("   Enter Datum (Hex): ");
   data = getnh(); printf("\n");

   j = SCWidth/16;
   for (i=0;i<j;i++) *addr++ = data;

}		/* End of write_16hort() */

pound_double()
{  register uint *addr1,*addr2,data1,data2,datab1,datab2;
   char ch;

   printf("Enter Address #1 (Hex): ");
   addr1 = (uint*)getnh();  printf("\n");
   printf("Enter Data #1 (Hex): ");
   data1 = getnh(); printf("\n");
   printf("Enter Address #2 (Hex): ");
   addr2 = (uint*)getnh();  printf("\n");
   printf("Enter Data #2 (Hex): ");
   data2 = getnh(); printf("\n");

   printf ("   0: Write continously       \n");
   printf ("   1: Write/read continously  \n");
   printf ("   Enter Choice: ");
   ch = getchar(); printf("\n");
    
   if ( ch == '0') 
      while (1) {
         *addr1 = data1; *addr2 = data2;
         *addr1 = data1; *addr2 = data2;
         *addr1 = data1; *addr2 = data2;
         *addr1 = data1; *addr2 = data2;
         *addr1 = data1; *addr2 = data2;
         *addr1 = data1; *addr2 = data2;
         }
   else if ( ch ==  '1') 
      while (1) {
         *addr1 = data1; *addr2 = data2;
         datab1 = *addr1; datab2 = *addr2;
         printf ("data1 %x datab1 %x data2 %x, datab2 %x \n", data1, datab1,
            data2, datab2);
/*
         *addr1 = data1; *addr2 = data2;
         datab1 = *addr1; datab2 = *addr2;
         *addr1 = data1; *addr2 = data2;
         datab1 = *addr1; datab2 = *addr2;
         *addr1 = data1; *addr2 = data2;
         datab1 = *addr1; datab2 = *addr2;
         *addr1 = data1; *addr2 = data2;
         datab1 = *addr1; datab2 = *addr2;
         *addr1 = data1; *addr2 = data2;
         datab1 = *addr1; datab2 = *addr2;
*/
         }
}	/* End of routine pound_double() */

verify_all()
{  register ushort *addr,data,temp;
   printf("Verify whole frame buffer with datum. Enter Datum (Hex): ");
   data = getnh(); printf("\n");

   for (addr=SC_Mem0;addr<SC_Mem8;) {
      temp = *addr++;
      if (data != temp) {
	printf("Error: Addr 0x%x. Wr 0x%x. Rd 0x%x.\n",((int)addr)-2,data,temp);
      }
      temp = *addr++;
      if (data != temp) {
	printf("Error: Addr 0x%x. Wr 0x%x. Rd 0x%x.\n",((int)addr)-2,data,temp);
      }
      temp = *addr++;
      if (data != temp) {
	printf("Error: Addr 0x%x. Wr 0x%x. Rd 0x%x.\n",((int)addr)-2,data,temp);
      }
      temp = *addr++;
      if (data != temp) {
	printf("Error: Addr 0x%x. Wr 0x%x. Rd 0x%x.\n",((int)addr)-2,data,temp);
      }
   }
}	/* End of routine verify_all */

pix32_trap()
{
   register uint *addr,data;
   int pix32();

   printf("Writing 0xAAAAAAAA continuously to pixel 0.\n");

   *((int*)0x0008) = (int)pix32;		/* Bus Error Routine */
   addr = SC_Pixl;
   data = (uint)0xAAAAAAAA;
   while (1) {
pix32:
      *addr = data; *addr = data; *addr = data; *addr = data;
   }
   asm("_pix32:");
   printf("B");
   asm("	bset	#7,a7@(8)");		/* Inhibit rerun */
   asm("	rte");
   goto pix32;               		/* Go do it again. */

}	/* End of routine to trap 32-bit pixel write timeouts */

scan_value()
{
   register ushort *addr,data;
   
   printf("Will scan memory for a value. Enter 16-bit hex Datum : ");
   data = getnh() & 0xFFFF;

   for (addr=SC_Mem0;addr<SC_Mem8;) {
      if (*addr++ == data) {
	 printf("Data matches value at address 0x%x\n",(int)(--addr)); addr++;
      }
      if (*addr++ == data) {
	 printf("Data matches value at address 0x%x\n",(int)(--addr)); addr++;
      }
      if (*addr++ == data) {
	 printf("Data matches value at address 0x%x\n",(int)(--addr)); addr++;
      }
      if (*addr++ == data) {
	 printf("Data matches value at address 0x%x\n",(int)(--addr)); addr++;
      }
   }
}	/* Routine scan_value() */
