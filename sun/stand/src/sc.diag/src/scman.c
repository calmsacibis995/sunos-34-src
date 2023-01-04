

/* ======================================================================
   Author : Peter Costello
   Date   : April 15, 1983
   Purpose : This is an interactive color board diagnostic program.
   Algorithm :
   Error Handling : 
   ====================================================================== */
static char     sccsid[] = "@(#)scman.c 1.1 9/25/86 Copyright Sun Micro";

#include "sc.diag.h"

extern int foobar;		/* Used in schecker.c */
int first_time = 1;
scman()       
{
    struct bd_list *list_p;
    int i,found,in_err,base;
    char ch;
    ushort temp;

    int berr();	       		/* Handle Bus Error */
    int berr1();       		/* Handle Bus Error */
    int buserr();	       	/* Handle Bus Error */
    int buserd();	       	/* Handle Bus Error */
    int i_hand();

    printf("\n\nINTERACTIVE COLOR BOARD TESTER\n\n");

    *((int*)0x0008) = (int)berr;	/* Bus Error Routine */

    list_p = head;
    if (list_p != NULL) {
       	i = head->base; Map_Space(i);
	if (list_p->res_1k1k) {
	   SCWidth = 1024; SCHeight = 1024;
	} else {
	   SCWidth = 1152; SCHeight = 909;
	}
        printf("Set Up Defaults (y/n)? ");
   	ch = getchar(); printf("\n");
	if ((ch!='n')&&(ch!='N')) {
	   foobar = 0;			/* Draw original checkerboard */
	   init_scolor();
        }
    } else {				/* No boards detected */
	printf("Assuming 1152x900 board at VME addr 0x400000.");
  	printf(" Virtual addr 0x200000.\n");
	SCWidth = 1152; SCHeight = 909;
	add_device((int)0x400000,(int)0);
        list_p = head;
    }
    first_time = 0;

    in_err = False;		/* Get listing first time through */
    while (1) {
       if (! in_err) {
          printf("1: Add Device\n");
          printf("2: Select Device\n");
          printf("3: Access Board Continously\n");
          printf("4: Test Control Registers\n");
	  printf("5: Test Interrupts\n");
	  printf("6: Test Color Maps\n");
	  printf("7: Test Frame Buffer\n");
	  printf("8: Test ROPC Units\n");
	  printf("9: Test Zoom and Pan\n");
	  printf("A: Test DACs and Monitor\n");
	  printf("B: Brief Monitor Tests\n");
	  printf("C: Perform Auto Test\n");
       }
       printf("Enter Choice: ");
       ch = getchar(); printf("\n");
       in_err = False;

       if (ch == '1') {
          printf("Enter Device Address (HEX): ");
	  base = getnh(); printf("\n");
	  printf("Standard 1152x900 display (vs 1024x1024) (y/n)? ");
	  ch = getchar();
	  if ((ch == 'n')||(ch == 'N')) {
	     add_device(base,1);
             list_p = head;
	  } else {
	     add_device(base,0);
             list_p = head;
	  }
       } else if (ch == '2') {
	  if (head == NULL) {
	     printf("ERROR: No Devices Known to Program. ADD one.\n");
	  } else {
	     list_p = head;
	     printf("Devices Known to Program:\n");
	     while (list_p != NULL) {
	        printf("     Device #%d. Base at 0x%x. ",list_p->device,
		   list_p->base);
		if (list_p->res_1k1k) {
	 	   printf("(1024 x 1024).\n");
		} else {
	 	   printf("(1152 x 900).\n");
		}
	        list_p = list_p->next;
	     }
	     do {
	        printf("Enter Device Number: ");
		i = getn(); printf("\n");
	        found = FALSE;
		list_p = head;
	        while ((!found)&&(list_p != NULL)) {
	 	   if (list_p->device == i) {
		      found = TRUE;
		      Map_Space(list_p->base);
		      if (list_p->res_1k1k) {
		         SCWidth = 1024; SCHeight = 1024;
		      } else {
		         SCWidth = 1152; SCHeight = 900;
		      }
		      /* init_scolor();		/* Init device */
	           } else {
		      list_p = list_p->next;
		   }
	        }
	     } while (!found);
	  }
       } else if (ch == '3') {
          printf("   Read or Write (r/w)? ");
	  ch = getchar(); printf("\n");
     	  if ((ch=='r')||(ch=='R')) {
	    printf("Reading continuously from Word-Pan Reg...\n");
	    *((int*)0x0008) = (int)buserd;	/* Bus Error Routine */
	    while (1) {
berrdt:
	       temp = SC_WPan;
	    }
	    asm("_buserd:");
            asm("	addl	#58,sp");
            goto berrdt;               		/* Go do it again. */
	  } else {
	    printf("Writing 0xAAAA continuously to Word-Pan Reg...\n");
	    *((int*)0x0008) = (int)buserr;	/* Bus Error Routine */
	    while (1) {
berret:
	       SC_WPan = 0xAAAA;
	    }
	    asm("_buserr:");
            asm("	addl	#58,sp");
            goto berret;               		/* Go do it again. */
	  }

       } else if (ch == '4') {
	  man_reg(list_p);			/* Test control registers */
       } else if (ch == '5') {
	  man_int(list_p);			/* Test Interrupts */
       } else if (ch == '6') {
	  mancmap(list_p);
       } else if (ch == '7') {
	  manmem(list_p);
       } else if (ch == '8') {
	  manrop(list_p);
       } else if (ch == '9') {
	  manzoom(list_p);
       } else if ((ch == 'a')||(ch == 'A')) {
	  mdac();
       } else if ((ch == 'b')||(ch == 'B')) {
	  briefmon();
       } else if ((ch == 'c')||(ch == 'C')) {
	  test_dac();
	  scauto(list_p);
       } else {
	  in_err = TRUE;
       }
    }

/* General Purpose Bus Error Handler. */
busfoo:
	  asm("_berr:");
	  printf("BERR ");

	  asm("	bset	#7,a7@(8)");		/* Inhibit rerun */
          asm("	rte");
          goto busfoo;               		/* Got an unexpected error */

}		/* End of routine scman() */

