
static char	sccsid[] = "@(#)manual.c 1.1 9/25/86 Copyright SMI";

/* ======================================================================
   Author : Peter Costello
   Date   : April 21, 1982
   Purpose : This is an interactive color board diagnostic program.
   Algorithm :
   Error Handling : 
   ====================================================================== */

#include "cdiag.h"
#include "m68000.h"
#include "vectors.h"
#include "reentrant.h"

manual()       
{
    int base;
    short register i,j,color;
    short choice,in_err,cmap,found,device;
    char ch,ch1;
    uchar data,data1;
    struct bd_list *list_p;

    int bserr();	       	/* Handle Bus Error */
    int buserr();	       	/* Handle Bus Error */
    int i_hand();

    uchar register
        *carr1,
        *carr2,
        *carr3,
        cmap1[256],
        cmap2[256],
        cmap3[256];
        

    printf("\n\nINTERACTIVE COLOR BOARD TESTER\n\n");

    /* if( CGXfind()) exit( 1); */

    list_p = head;		/* Default to select head device */
    *((int*)0x0008) = (int)bserr;	/* Bus Error Routine */
    if (list_p != NULL) {
       CGXBase = list_p->base;
       *GR_sreg = GR_disp_on;	/* Turn on display. */
    }

    in_err = False;		/* Get listing first time through */
    while (1) {
       if (! in_err) {
          printf("1: ADD Device.\n");
          printf("2: SELECT Device.\n");
          printf("3: Trap TIMEOUT. Write continuously to board.\n");
          printf("4: Test FUNCTION Reg.\n");
          printf("5: Test MASK Reg.\n");
          printf("6: Test STATUS Reg.\n");
          printf("7: Test COLOR MAP.\n");
          printf("8: Test FRAME BUFFER.\n");
          printf("9: Test FUNCTION UNIT.\n");
	  printf("10: Test INTERRUPTS.\n");
          printf("11: AUTO TEST.\n");
	  printf("12: Test DACs.\n");
       }
       printf("CHOICE? ");
       /* scanf("%d",choice); */
       choice = getn(); printf("\n");
       printf("\n");
       in_err = FALSE;

       if (choice==1) {
          printf("Enter Device Address (HEX): ");
	  /* scanf("%x",base); */
	  base = getnh(); printf("\n");
	  add_device(base);
       } else if (choice == 2) {
	  if (head == NULL) {
	     printf("ERROR: No Devices Known to Program. ADD one.\n");
	  } else {
	     list_p = head;
	     printf("Devices Known to Program:\n");
	     while (list_p != NULL) {
	        printf("     Device #%d. Base at 0x%x.\n",list_p->device,
		   list_p->base);
	        list_p = list_p->next;
	     }
	     do {
	        printf("Enter Device Number: ");
	        /* scanf("%d",device); */
		device = getn(); printf("\n");
	        found = FALSE;
		list_p = head;
	        while ((!found)&&(list_p != NULL)) {
	 	   if (list_p->device == device) {
		      found = TRUE;
		      CGXBase = list_p->base;
    		      *GR_sreg = GR_disp_on;	/* Turn on display. */
	           } else {
		      list_p = list_p->next;
		   }
	        }
	     } while (!found);
	  }
       } else if (choice == 3) {
berrt:    printf("Trap Bus Error for device #%d. Base 0x%x.\n",
			list_p->device,list_p->base);
	  printf("Writing continuously to board...(Hit Abort to Exit)\n");
	  *((int*)0x0008) = (int)buserr;	/* Bus Error Routine */
	  while (1) {
berret:
	     Set_CFunc(GR_copy);
	  }
       } else if (choice == 4) {
	  printf("   FUNCTION Reg. Read,Write, or Both. (R/W/B)? ");
	  /* scanf("%c",ch); */
	  ch = getchar(); printf("\n");
	  printf("   Once or Continuously (O/C)? ");
	  /* scanf("%c",ch1); */
	  ch1 = getchar(); printf("\n");
	  if ((ch == 'r')||(ch == 'R')) {
	     if ((ch1 == 'c')||(ch1 == 'C')) {
		   printf("Reading function continuously...\n");
		while (1) Read_CFunc(data);
	     } else {
		Read_CFunc(data);
	        printf("   Read 0x%x\n",data);
	     }
	  } else {
	     printf("   Write Function (HEX)? ");
	     /* scanf("%x",data); */
	     data = getnh(); printf("\n");
	     if ((ch == 'w')||(ch == 'W')) {
	        if ((ch1 == 'c')||(ch1 == 'C')) {
		   printf("Writing function continuously...\n");
		   while (1) Set_CFunc(data);
		} else {
	           Set_CFunc(data);
		}
	     } else {
	        if ((ch1 == 'c')||(ch1 == 'C')) {
		   printf("Writing Func continuously...\n");
		   while (1) {Set_CFunc(data); Read_CFunc(data1);}
		} else {
		   Set_CFunc(data);
		   Read_CFunc(data1);
	           printf("   Wrote 0x%x. Read 0x%x.\n",data,data1);
		}
	     }
	  }
       } else if (choice == 5) {
	  printf("   MASK Reg. Read,Write, or Both. (R/W/B)? ");
	  /* scanf("%c",ch); */
	  ch = getchar(); printf("\n");
	  printf("   Once or Continuously (O/C)? ");
	  /* scanf("%c",ch1); */
	  ch1 = getchar(); printf("\n");
	  if ((ch == 'r')||(ch == 'R')) {
	     if ((ch1 == 'c')||(ch1 == 'C')) {
		   printf("Reading continuously...\n");
		while (1) Read_CMask(data);
	     } else {
		Read_CMask(data);
	        printf("   Read 0x%x\n",data);
	     }
	  } else {
	     printf("   Write Data (HEX)? ");
	     /* scanf("%x",data); */
	     data = getnh(); printf("\n");
	     if ((ch == 'w')||(ch == 'W')) {
	        if ((ch1 == 'c')||(ch1 == 'C')) {
		   printf("Writing continuously...\n");
		   while (1) Set_CMask(data);
		} else {
	           Set_CMask(data);
		}
	     } else {
	        if ((ch1 == 'c')||(ch1 == 'C')) {
		   printf("Writing continuously...\n");
		   while (1) {Set_CMask(data);Read_CMask(data1);}
		} else {
		   Set_CMask(data);
		   Read_CMask(data1);
	           printf("   Wrote 0x%x. Read 0x%x\n",data,data1);
		}
	     }
	  }
       } else if (choice == 6) {
	  printf("   Enable Processor interrupts? (Y/N) ");
	  /* scanf("%c",ch); */
	  ch = getchar(); printf("\n");
	  if ((ch == 'y')||(ch1 == 'Y')) {
	     printf("   Processor enabled to accept interrupts.\n");
	     IRQ1Vect = (int)i_hand;
	     IRQ2Vect = (int)i_hand;
	     IRQ3Vect = (int)i_hand;
	     IRQ4Vect = (int)i_hand;
	     /* IRQ5Vect = (int)i_hand; */
	     /* IRQ6Vect = (int)i_hand; */
	     intlevel(1);
	  } else {
	     printf("   Processor will accept no interrupts.\n");
	     intlevel(7);
	  }
	  printf("   Status Reg. Read,Write, or Both. (R/W/B)? ");
	  /* scanf("%c",ch); */
	  ch = getchar(); printf("\n");
	  printf("   Once or Continuously (O/C)? ");
	  /* scanf("%c",ch1); */
	  ch1 = getchar(); printf("\n");
	  if ((ch == 'r')||(ch == 'R')) {
	     if ((ch1 == 'c')||(ch1 == 'C')) {
		printf("Reading continuously...\n");
		while (1) data = *GR_sreg;
	     } else {
	        printf("   Read 0x%x\n",*GR_sreg);
	     }
	  } else {
	     printf("   Write Data (HEX)? ");
	     /* scanf("%x",data); */
	     data = getnh(); printf("\n");
	     if ((ch == 'w')||(ch == 'W')) {
	        if ((ch1 == 'c')||(ch1 == 'C')) {
		   printf("Writing continuously...\n");
		   while (1) *GR_sreg = data;
		} else {
	           *GR_sreg = data;
		}
	     } else {
	        if ((ch1 == 'c')||(ch1 == 'C')) {
		   printf("Writing 0x%x and Reading continuously...\n",data);
		   while (1) {*GR_sreg = data; data1 = *GR_sreg;}
		} else {
		   *GR_sreg = data;
		   data1 = *GR_sreg;
	           printf("   Wrote 0x%x. Read 0x%x\n",data,data1);
		}
	     }
	  }
       } else if (choice == 7) {
	  tcmap(list_p);
       } else if (choice == 8) {
	  tfbuf(list_p);
       } else if (choice == 9) {
          printf("Testing Function Unit.\n");
	  testfunc(list_p,0);
	  printf("Function Unit Test complete.\n");
       } else if (choice == 10) {
	  tinterrupt(list_p,0);
       } else if (choice == 11) {
	  auto_test(list_p);
       } else if (choice == 12) {
	  printf("Write Ramp pattern for red, green, or blue (r/g/b)? ");
	  ch = getchar(); printf("\n");
	  if ((ch == 'r')||(ch == 'R')) {
	     display_red();
	  } else if ((ch == 'g')||(ch == 'G')) {
	     display_green();
	  } else {
	     display_blue();
	  }
       } else {
	  in_err = TRUE;
       }
    }
    ;

avoid_warn_1:
asm("_bserr:");
    asm("	addl	#14,sp");
    goto berrt;               /* Go handle bus error. */

avoid_warn_2:
asm("_buserr:");
    asm("	addl	#14,sp");
    goto berret;               /* Go do it again. */

    ;
}

/* This routine handles interrupts by turning them off and returning. */
reentrant(i_hand)
{
   uchar data;

   /* printf("Handling Interrupt.\n"); */
   data = *GR_sreg;
   *GR_sreg = 0;		/* Turn off interrupts */
   *GR_sreg = data;		/* Restore status register */
}

