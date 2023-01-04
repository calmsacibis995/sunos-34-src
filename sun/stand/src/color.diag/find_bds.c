
static char	sccsid[] = "@(#)find_bds.c 1.1 9/25/86 Copyright SMI";

/* ======================================================================
   Author: Peter Costello
   Date :  October 21, 1982
   Purpose: Find all color graphics boards in system.
   Algorithm: Set function register to write inverted data. Now write data1
	to frame buffer, read it back. Now write data2 to frame buffer. If 
	value read back is the inverse of data1, then we can assume that this
	is a color board.
   Error Handling: Take bus error. No board exists at this location.
   Bug Fixes: Sometimes finds a board where none existed. Added one extra
  	test.
   ====================================================================== */

#include "cdiag.h"

struct bd_list *head;
struct bd_list bds[Max_Boards];

int bd_count = 0;	/* Number of boards found */

find_bds()
{
   int base;		/* Base currently testing. */
   uchar *xaddr,*yaddr;
   uchar data,data1,data2;
   struct bd_list block;
   int berr();

   data1 = (uchar)0x5C;
   data2 = (uchar)0x33;

   head = NULL;
   *((int*)0x0008) = (int)berr;		/* Bus Error Handler */
   
   /* Start at 1MB in physical memory, and look for the board upto the top
      of our physical address space. */
   for (base=64;base<(Addr_Space>>14);base++) {
      CGXBase = base << 14;
      /* printf("Testing location 0x%x.\n",CGXBase); */

      Set_CFunc(GR_copy_invert);
      Set_CMask(0);
      xaddr = (uchar*)(GR_bd_sel + GR_x_select + GR_update + GR_set0);
      yaddr = (uchar*)(GR_bd_sel + GR_y_select + GR_set0);
   
      *yaddr = TOUCH;	/* Set Y address */
      *xaddr = data1;
      data   = *xaddr;
      *xaddr = data2;
      data   = *xaddr;
      /* printf("Wrote 0x%x. Read byte. Wrote 0x%x. Read 0x%x.\n",
		 data1,~data2,data); */
      if (data == (~data1)) {
         Set_CFunc(GR_set);
	 *xaddr = data1;
     	 data = *xaddr;
	 data = *xaddr;
	 if (data == (uchar)0xFF) {
            /* Board at this address */
	    add_device(CGXBase);
            printf("Device #%d found at address 0x%x.\n",head->device,CGXBase);
	 }
      }
restart:;
   }
   if (head == NULL) {
      printf("WARNING: No Boards Detected in System. Using Manual mode.\n");
   }
   goto endofit;

avoid_warn_1:
   asm("_berr:");
   asm("	addl	#14,sp");	/* FIXME, breaks on 68010/20 */
   goto restart;

endofit:;
}
	    

add_device(base)
   int base;
{
   struct bd_list *block;
   int i;
   
   if (head == NULL) {
      head = &(bds[bd_count]);
   } else {
      block = &(bds[(bd_count-1)]);
      block->next = &(bds[bd_count]);
   }
   block = &(bds[bd_count++]);
   block->base = base;
   block->device = bd_count;
   block->next = NULL;
   for (i=0;i<Num_Tests;i++) block->error[i] = 0;	/* No errors yet */
}
	 

