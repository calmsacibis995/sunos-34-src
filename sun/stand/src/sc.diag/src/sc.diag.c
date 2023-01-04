

/* ======================================================================
   Author: Peter Costello
   Date :  October 21, 1982
   Purpose: SUN-2 Color diagnostics.
   Algorithm: Perform the following tasks:
	1. Determine any color boards that are in the system.
	2. If no boards are found, prompt for board's base address.
	3. Ask user if he wants to perform a manual test. If an 'M' or 'm'
	   is typed within 15 seconds do a manual vs. auto test.

	AUTO TEST
	1. Register Tests
		Status register
		Per-Plane mask register
		Word Pan register
		Line Offset and Zoom register
		Pixel Pan register
		Variable Zoom Register
	2. Interrupt Tests
	3. Color Map Tests
	4. Memory-mode Frame Buffer Tests
		Imemtest on word data. 32-bit writes/reads.
		Constant data test on pixel data. 8-bit writes. 8-bit reads.
		Imemtest on pixel data. 32-bit writes. 8-bit reads.
	5. Per-Plane Mask Tests
		Test mask register for word/pixel read/write data access
	6. ROPC and Data Path Tests
		ROPC units with diagnostic read-back
		Test loading multiple ROPC in parallel using mask register
		Word-mode ROPC tests on whole frame buffer (no dst data)
		Pixel-mode ROPC tests on whole frame buffer (no dst data)
		Test Destination data paths (no source data)
	7. Zoom and Pan Tests
		For all zoom factors, do smooth panning up, down, right,
			left, and diagonally. Use variable scroll rate.
	8. Dac Tests
		Display outline of frame buffer
		Display RGB simultaneously
		Display color ramps 
		Display alternating rows for flicker-effect.
	9. Increment test number and error summary. Repeat auto test.

	MANUAL TEST
	The Manual Tests include all the functions of the Auto Tests. In
	addition, the following features are also provided:
		A.  Add a device to device list.
        	B.  Select a device for manual operation.
		C.  Write continuously to board. Handle bus error.

   Error Handling: The perennial problem.
   Bugs: Undoubtably several.
   ====================================================================== */

static char     sccsid[] = "@(#)sc.diag.c 1.1 9/25/86 Copyright Sun Micro";

#include "sc.diag.h"
#define wait_sec(sec) i2=sec;j2=Sec1;while (i2-- > 0) while (j2-- > 0)

/* This array of strings describes the eight functional areas of the board. */
char *error_str[] = {
	"Status Reg",			/*  0 */
	"Plane Mask Reg",		/*  1 */
	"Word Pan Reg",			/*  2 */
	"LOff & Zoom Reg",		/*  3 */
	"Pixel Pan Reg",		/*  4 */
	"Varible Zoom Reg",		/*  5 */
	"Interrupts",			/*  6 */
	"Shadow Color Map",		/*  7 */
	"FB Word-Memory",		/*  8 */
	"FB Pixel-Memory",		/*  9 */
	"ROPC Plane 0",			/* 10 */
	"ROPC Plane 1",			/* 11 */
	"ROPC Plane 2",			/* 12 */
	"ROPC Plane 3",			/* 13 */
	"ROPC Plane 4",			/* 14 */
	"ROPC Plane 5",			/* 15 */
	"ROPC Plane 6",			/* 16 */
	"ROPC Plane 7",			/* 17 */
	"Pix-mode Plane Masking",	/* 18 */
	"Word-mode Plane Masking",	/* 19 */
	"ROPC Pixel Memory",		/* 20 */
	"FB Word Memory Plane 0",	/* 21 */
	"FB Word Memory Plane 1",	/* 22 */
	"FB Word Memory Plane 2",	/* 23 */
	"FB Word Memory Plane 3",	/* 24 */
	"FB Word Memory Plane 4",	/* 25 */
	"FB Word Memory Plane 5",	/* 26 */
	"FB Word Memory Plane 6",	/* 27 */
	"FB Word Memory Plane 7",	/* 28 */
	"Interrupt Vector Reg" };	/* 29 */

extern int halt_on_err;
extern int foobar;		/* Used by schecker.c */

main()
{
   register int *a6,*a5,*a4,*a3,*a2;
   register int d7,d6,d5,d4,d3,d2;
   char ch;
   int berr1();

   asm("	movl	#0xC0000,a7") ; ;

   printf("\n\nSUN-2 COLOR BOARD DIAGNOSTIC PROGRAM.\n\n");

   head = NULL;
   find_bds();		/* Look for all color boards in system. */

   *((int*)0x0008) = (int)berr1;	/* Bus error handler */

   if (head == NULL) scman();		/* Call manual program */
   
   printf("\nManual or Auto Test ('M' or 'A') ? ");
   ch = getchar(); printf("\n");
   if ((ch != 'A')&&(ch != 'a')) scman();

   tst_auto();				/* This routine never ends */

/* Define bus error handler. */
asm("_berr1:");
   asm("	addl	#58,sp");
   printf("\nBus Error or Timeout. Run Manual Diagnostics.\n");
   printf("A6 = 0x%x; A5 = 0x%x; A4 = 0x%x\n",(int)a6,(int)a5,(int)a4);
   printf("A3 = 0x%x; A2 = 0x%x\n",(int)a3,(int)a2);
   printf("D7 = 0x%x; D6 = 0x%x; D5 = 0x%x\n",d7,d6,d5);
   printf("D4 = 0x%x; D3 = 0x%x; D2 = 0x%x\n",d4,d3,d2);
   scman();

}

/* Set new base address, call routines to test each category. */
scauto(list_p)
   struct bd_list *list_p;
{
   int i2, j2;

   Map_Space(list_p->base);
   if (list_p->res_1k1k) {
      SCWidth = 1024; SCHeight = 1024;
   } else {
      SCWidth = 1152; SCHeight = 1024;
   }
   foobar = 0; init_scolor();		/* Init color board */
   
   printf("Test On-Board Registers\n");
   auto_reg(list_p);			/* Test out registers */
   printf("Test Interrupts\n");
   auto_int(list_p);			/* Test out interrupts */
   printf("Test Color Map\n");
   auto_cmap(list_p);
   load_colors();			/* Nice color pattern */
   wait_sec (1);
   halt_on_err = 0; auto_mem(list_p);   /* Test memory */

   foobar = 0; init_scolor();		/* Init color board */
   printf("Test Digital-to-Analog Converters and Monitor\n");
   auto_dac();
   foobar = 0; init_scolor();		/* Init color board */
   printf("Test Zoom and Pan \n");
   auto_zoom(list_p);
   printf("Test ROPC. Plane ");
   auto_rop(list_p);
   printf("\nTest Per-Plane Masking\n");
   pix_plmask(list_p);
   printf("Test Per-Plane Loading of Ropc\n");
   word_plmask(list_p);

}		/* End of auto_test for a single color board */



/* Perform an auto diagnostic of all color boards in the system. */
tst_auto()
{
   struct bd_list *list_p;
   int i,testnum;
   ushort no_errs;
   
   /* Test out DACs for all boards in system */
   list_p = head;
   while (list_p != NULL) {
      Map_Space(list_p->base);
      test_dac();
      list_p = list_p->next;
   }

   testnum = 0;
   while (1) {
      testnum++;			/* Increment test number. */
      list_p = head;
      while (list_p != NULL) {
	 scauto(list_p);		/* Test out this device */
	 list_p = list_p->next;
      }

      /* Print out Test Number, Device number, base address, and cumulative
	 errors. */
      list_p = head;
      while (list_p != NULL) {
         printf("Test #%d. Device #%d. Base Addr 0x%x",testnum,list_p->device,
		list_p->base);
	 no_errs = TRUE;
	 for (i=0;i<Num_Tests;i++) {
	    if (list_p->error[i] != 0) {
	       no_errs = FALSE;
	       printf("\n        %s  %d Errors.",error_str[i],list_p->error[i]);
	    }
	 }
	 if (no_errs) printf(" No Errors.");
   	 printf("\n\n");

	 list_p = list_p->next;
      }
   }
}		/* End of routine tst_auto */


