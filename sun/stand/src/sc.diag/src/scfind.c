

/* ======================================================================
   Author: Peter Costello
   Date :  April 15, 1982
   Purpose: Find all Sun-2 color graphics boards in system.
   Algorithm: Write in memory mode to first pixel. Then overwrite it in
	word mode, and reread in in pixel mode.
   Error Handling: Take bus error. No board exists at this location.
   Bug Fixes: 
   ====================================================================== */
static char     sccsid[] = "@(#)scfind.c 1.1 9/25/86 Copyright Sun Micro";

#include "sc.diag.h"

struct bd_list *head;
struct bd_list bds[Max_Boards];

int bd_count = 0;	/* Number of boards found */

Map_Space(base)
   int base;
{
   register short j;
   register uint tbase,entry;

   tbase = 0x200000;

   if ((base & 0x3FFFFF) != 0) {
      printf("Error: Base at 0x%x. Board must lie on 4 MB boundary\n",base);
   } else if (base < 0x800000) {
      entry = ((uint)0xFE800000) + (base >> 11);
      for (j=0x800;j>0;j--) {
         setpgmap(tbase,entry);
         tbase += 0x800;
         entry += 1;
      }
   } else {
      if (base <= (Addr_Space-0x400000)) {
	 base -= 0x800000;
         entry = (uint)((uint)0xFEC00000) + (base >> 11);
         for (j=0x800;j>0;j--) {
            setpgmap(tbase,entry);
            tbase += 0x800;
            entry += 1;
         }
      }
   }
}

#define init_regs				\
   SC_Stat = 1; SC_WPan = 0; SC_Zoom = 0;	\
   SC_PPan = 0; SC_VZoom = 0xff; SC_Mask = 0xFF; SC_IVect = 0x54

#define init_ropc(x)							\
   Set_Ropc(x); SC_Dst = 0; SC_Src1 = 0; SC_Src2 = 0; SC_Pat = 0;	\
   SC_Msk1 = 0; SC_Msk2 = 0; SC_SftVal = 0; SC_Func = 0x33;		\
   SC_Width = 0; SC_OpCnt = 0

find_bds()
{
   int base,res;		/* Base currently testing. */
   int berr();

   head = NULL; bd_count = 0;
   
   /* Start at 0MB in VME physical memory, and look for the board upto the top
      of our physical address space. */
   *((int*)0x0008) = (int)berr;		/* Bus Error Handler */
   for (base=0;base<=(Addr_Space-0x400000);base+=0x400000) {
      Map_Space(base);		/* Set up Page Maps */

      init_regs;
      init_ropc(0); init_ropc(1); init_ropc(2); init_ropc(3);
      init_ropc(4); init_ropc(5); init_ropc(6); init_ropc(7);

      SC_Mask = 0xFF;
      *SC_Pix = 0;
      *SC_Mem0 = (ushort)0xFFFF;
      *SC_Mem1 = (ushort)0xFFFF;
      *SC_Mem3 = (ushort)0xFFFF;
      *SC_Mem5 = (ushort)0xFFFF;
      *SC_Mem6 = (ushort)0xFFFF;
      if (*SC_Pix == 0x6B) {
         /* Board at this address */
	 res = SC_Stat & Res_1k1k;
	 add_device(base,res);
         printf("Device #%d found at VME addr 0x%x.",bd_count,base);
         printf(" Virtual Addr = 0x200000.");
	 if (res) {
	    printf(" Res = 1024x1024.\n");
	 } else {
	    printf(" Res = 1152x900.\n");
	 }
      }
restart:;
   }

   if (bd_count == 0) {
	 printf("Warning: No Boards Detected in System.\n");
   }

   if (0) {			/* Bus Error Handler */
      asm("_berr:");
      asm("	addl	#58,sp");
      goto restart;
   }
}
	    

add_device(base,res)
   int base,res;
{
   struct bd_list *block;
   int i;
   
   if (bd_count == 0) {
      head = &(bds[bd_count]);
   } else {
      block = &(bds[(bd_count-1)]);
      block->next = &(bds[bd_count]);
   }
   block = &(bds[bd_count]);
   block->base = base;
   bd_count += 1;
   block->device = bd_count;
   block->res_1k1k = res;
   block->next = NULL;
   for (i=0;i<Num_Tests;i++) block->error[i] = 0;	/* No errors yet */

   Map_Space(base);	/* Set up Page Maps */
}
	 

