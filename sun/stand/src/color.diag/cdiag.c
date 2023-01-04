
static char	sccsid[] = "@(#)cdiag.c 1.1 9/25/86 Copyright SMI";

/* ======================================================================
   Author: Peter Costello
   Date :  October 21, 1982
   Purpose: Perform diagnostics on PC version of color board.
   Algorithm: Perform the following tasks:
	1. Determine any color boards that are in the system.
	2. If no boards are found, prompt for board's base address.
	3. Ask user if he wants to perform a manual test. If an 'M' or 'm'
	   is typed within 15 seconds do a manual vs. auto test.

	AUTO TEST
	1. Test Function Register.
	2. Test Mask Register.
	3. Test Status Register. Take a level 0 through level 6 interrupt.
	4. Test address registers.
	5. Test color map and color map selection bits.
	6. Test Function Unit.
	7. Test frame buffer.
	8. Test 5-pixel wide updates.
	8. Write Screen 1/3 red, 1/3 green, 1/3 blue. (DACs must be visually
	   tested). Hold 20 seconds.
	9. Write checkerboard with default color map. Hold 20 seconds.
	10. Increment test number and error summary. Repeat auto test.

	MANUAL TEST
	A.  Add a device to device list.
        B.  Select a device for manual operation.
	1.  Write to board. Handle bus error. Used to test select logic.
	2.  Write	to 	FUNCTION reg.
	3.  Read	from	FUNCTION reg.
	4.  Write/Read	to/from FUNCTION reg.
	5.  Write	to 	MASK reg.
	6.  Read	from	MASK reg.
	7.  Write/Read	to/from MASK reg.
	8.  Write	to 	STATUS reg.
	9.  Read	from	STATUS reg.
	10. Write/Read	to/from STATUS reg.
	11. Write	to 	ADDRESS reg.
	12. Read	from	ADDRESS reg.
	13. Write/Read	to/from ADDRESS reg.
	14. Write	to 	COLOR MAP.
	15. Read	from	COLOR MAP.
	16. Write/Read	to/from COLOR MAP.
	17. Write	to 	FRAME BUFFER.
	18. Read	from	FRAME BUFFER.
	16. Write/Read	to/from FRAME BUFFER.
	20+ Commands from old version of diagnostic.

   Error Handling:
   ====================================================================== */


#include "cdiag.h"

/* This array of strings describes the eight functional areas of the board. */
char *error_str[] = {
	"Function Register",
	"Mask Register    ",
	"Status Register  ",
	"Interrupt Logic  ",
	"Address Registers",
	"Color Map        ",
	"Function Unit    ",
	"Frame Buffer     ",
	"5-pixel Updates  "  };


main()
{
   char ch;
   int tst_auto();
   int manual();
   int find_bds();
   int berr1();

   printf("\n\nSUN Color Board Diagnostic  REV. 1.1  9/25/86\n");

   find_bds();		/* Look for all color boards in system. */

   *((int*)0x0008) = (int)berr1;	/* Set up bus error handler for 
					   unexpected bus errors. */

rerun:					/* The place to go on timeouts.*/
   if (head == NULL) manual();		/* Call manual program */
   
   printf("\nManual or Auto Test ('M' or 'A') ? ");
   ch = getchar(); printf("\n");
   if ((ch == 'M')||(ch == 'm')) manual();

   tst_auto();
   goto theend;

/* Define bus error handler. */
avoid_warn_1:
asm("_berr1:");
   asm("	addl	#14,sp");
   printf("\nBus Error or Timeout.\n");
   printf("Restarting program.\n");
   goto rerun;

theend:;
}



/* Perform an auto diagnostic of all color boards in the system. */
tst_auto()
{
   struct bd_list *list_p;
   int i,testnum;
   short no_errs;
   
   testnum = 0;
   while (1) {
      testnum++;			/* Increment test number. */
      list_p = head;
      while (list_p != NULL) {
	 auto_test(list_p);		/* Test out this device */
	 list_p = list_p->next;
      }

      /* Print out Test Number, Device number, base address, and cumulative
	 errors. */
      printf("\n");
      list_p = head;
      while (list_p != NULL) {
         printf("\n");
         printf("Test #%d. Device #%d. Base Addr 0x%x.",testnum,list_p->device,
		list_p->base);
	 no_errs = TRUE;
	 for (i=0;i<Num_Tests;i++) {
	    if (list_p->error[i] != 0) {
	       no_errs = FALSE;
	       printf("\n        %s  %d Errors.",error_str[i],list_p->error[i]);
	    }
	 }
	 if (no_errs) printf(" No Errors.");

	 list_p = list_p->next;
      }
   }
}


/* Set new base address, call routines to test each category. */
auto_test(list_p)
   struct bd_list *list_p;
{
   register int secs,i;
   int wrcmap(),rdcmap();

   CGXBase = list_p->base;
   set_fbuf_5x(0,0,128,512,255);  /* Init Fbuf */

   printf("TESTING Device #%d @0x%x.\n",list_p->device,list_p->base);
   printf("        Testing Function Register\n");
   tfunc_reg(list_p,0);
   printf("        Testing Mask Register\n");
   tmask_reg(list_p,1);
   printf("        Testing Status Register\n");
   tstat_reg(list_p,2);
   printf("        Testing Interrupt Logic\n");
   tinterrupt(list_p,3);
   printf("        Testing Address Registers\n");
   taddress(list_p,4);
   printf("        Testing Color Map\n");
   testcmap(list_p,5);
   printf("        Testing Function Unit\n");
   testfunc(list_p,6);
   printf("        Testing Frame Buffer Memory\n");
   testmem(list_p,7);
   printf("        Testing 5-Pixel-Wide Mode\n");
   tpaint(list_p,8);
   /* printf("        Preparing Visual Test of RGB Drivers\n"); */
   for (secs=3;secs>=0;secs--) {
      for (i=Sec1;i>0;i--);		/* Wait 3 sec */
   }

   /* Write 3 primary colors to display */
   prime_color();

   for (secs=7;secs>=0;secs--) {
      for (i=Sec1;i>0;i--);		/* Wait 7 sec */
   }

   /* Set color map 0 to its default state. */
   write_cmap(wrcmap,gr_red_c0,gr_grn_c0,gr_blu_c0,0);
   Set_Video_Cmap(0);
   /* Now draw the checkerboard */
   checker();

   for (secs=5;secs>=0;secs--) {
      for (i=Sec1;i>0;i--);		/* Wait 1 sec */
   }

   /* display red ramp, green ramp, & finally blue ramp */
   display_red();
   for (secs=5;secs>=0;secs--) {
      for (i=Sec1;i>0;i--);		/* Wait 5 sec */
   }
   display_green();
   for (secs=5;secs>=0;secs--) {
      for (i=Sec1;i>0;i--);		/* Wait 5 sec */
   }
   display_blue();
   for (secs=5;secs>=0;secs--) {
      for (i=Sec1;i>0;i--);		/* Wait 5 sec */
   }
}

/* This routine draws RED to the left third of the screen. GREEN to the center
   third of the screen, and BLUE to the right third of the screen. */
prime_color()
{
   int wrcmap();

   gr1_red_c1[0] = 255;			/* RED */
   gr1_grn_c1[0] = 0;
   gr1_blu_c1[0] = 0;

   gr1_red_c1[1] = 0;			/* GREEN */
   gr1_grn_c1[1] = 255;
   gr1_blu_c1[1] = 0;

   gr1_red_c1[2] = 0;			/* BLUE */
   gr1_grn_c1[2] = 0;
   gr1_blu_c1[2] = 255;

   Set_CFunc(GR_copy);
   set_fbuf_5x(0 ,0,43,512,0);		/* Red in left third */
   set_fbuf_5x(213,0,43,512,1);		/* Green in center third */
   set_fbuf_5x(426,0,43,512,2);		/* Blue in right third */

   write_cmap(wrcmap,gr1_red_c1,gr1_grn_c1,gr1_blu_c1,0);
   Set_Video_Cmap(0);
}


