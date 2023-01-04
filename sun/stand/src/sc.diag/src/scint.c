
/* ======================================================================
   Author: Peter Costello
   Date :  April 15, 1983
   Purpose: Test out Interrupts.
   Algorithm:
   Timing:
   Error Handling:
   Bugs:
   ====================================================================== */
static char     sccsid[] = "@(#)scint.c 1.1 9/25/86 Copyright Sun Micro";

#include "sc.diag.h"

int i_happen;
/* This routine handles interrupts by turning them off and returning. */
reentrant(i_hand)
{
   /* printf("Interrupt\n"); */
   i_happen = 1;
   SC_Stat &= ~Inten;
}		/* End of interrupt handler */


/* Auto test of interrupts */
auto_int(list_p)
   struct bd_list *list_p;
{
   int i_hand();
   int i,last,toggle,no_print;

   *((int*)(0x54<<2)) = (int)i_hand;
   SC_IVect = 0x54;
   
   no_print = 1;
   last = SC_Retrace;
   toggle = 0;
   i = 2000000;
   while ((i > 0)&&(!toggle)) {
      if (last ^ SC_Retrace) toggle = 1;
      i -= 1;
   }
   if (!toggle) {
      printf("Device #%d. Retrace bit in Status Register Never Toggles.\n",
		list_p->device);
      list_p->error[6] += 1;
   } else {
      intlevel(1);
      for (i=50;i>0;i-=1) {
         while (SC_Retrace);		/* Wait till end of retrace */
         SC_Stat = (VEnable + Inten);	/* Enable interrupts */
         while (!SC_Retrace);
         while (SC_Retrace);		/* Interrupt starts after this */
	 if ((!i_happen)&&(no_print)) {
            printf("Device #%d. No interrupt when Expected.\n",
			list_p->device);
            list_p->error[6] += 1;
	    no_print = 0;
	 }
	 i_happen = 0;
      }
      SC_Stat = VEnable;		/* Turn off interrupts */
      intlevel(7);			/* Turn off all CPU interrupts */
   }
}		/* End of procedure auto_int() */


/* manual test of interrupts */
man_int(list_p)
   struct bd_list *list_p;
{
   int i_hand(),i;
   short more;
   char ch;

   more = 1;
   while (more) {
      printf("Interrupt Tests\n");
      printf("   1: Enable interrupts on CPU\n");
      printf("   2: Disable interrupts on CPU\n");
      printf("   3: Enable, Trap, & Reset interrupts. Repeat 50 times.\n");
      printf("   4: Enable, Trap, & Reset interrupts. Repeat Forever.\n");
      printf("   5: Set User Interrupt vector.\n");
      printf("   6: Set All User Interrupt vectors.\n");
      printf("   Q: Quit\n");
      printf("   Enter Choice: ");
      ch = getchar(); printf("\n");

      if (ch == '1') {
         intlevel(1);
      } else if (ch == '2') {
         intlevel(7);
      } else if (ch == '3') {
	 list_p->error[6] = 0;
	 auto_int(list_p);
	 if (list_p->error[6] == 0) printf("      DONE. No Errors.\n");
      } else if (ch == '4') {
	 SC_Stat = VEnable + Inten;
         *((int*)(0x54<<2)) = (int)i_hand;
         SC_IVect = 0x54;
	 intlevel(1);
	 while (1) {
	    i_happen = 0;
            SC_Stat = (VEnable + Inten);	/* Enable interrupts */
	    while (!i_happen);
	 }
      } else if (ch == '5') {
	 printf("Enter User Vector (0xBF - 0x0): ");
         i = getnh();
	 *((int*)(0x100+(i<<2))) = (int)i_hand;
	 printf("Set color board interrupt vector to 0x%x (y/n)? ");
	 if ((ch!='n')||(ch!='N')) SC_IVect = i;
      } else if (ch == '6') {
	 for (i=0x100;i<0x400;i+=4) *((int*)(i)) = (int)i_hand;
      } else if ((ch == 'q')||(ch == 'Q')) {
	 more = 0;
      }
   }
}		/* End of Procedure man_int() */
