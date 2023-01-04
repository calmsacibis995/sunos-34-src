
/* ======================================================================
   Author: Peter Costello
   Date :  April 15, 1983
   Purpose: Test out mask register on color board.
   Algorithm:
   Timing:
   Error Handling:
   Bugs:
   ====================================================================== */
static char     sccsid[] = "@(#)plmask.c 1.1 9/25/86 Copyright Sun Micro";

#include "sc.diag.h"

pix_plmask(list_p)
   struct bd_list *list_p;
{
   uchar mask,data;
   short smask;
   
   smask = 256;
   while (smask > 0) {
      smask -= 1; mask = (uchar)smask;
      SC_Mask = (uchar)0xFF;
      *SC_Pix = 0;
      SC_Mask = mask;
      *SC_Pix = (uchar)0xFF;
      SC_Mask = (uchar)0xFF;

      data = *SC_Pix;
      if (data != mask) {
       printf("Device #%d. Testing Plane Mask.\n",list_p->device);
       printf("   Wrote 0xFF through Mask 0x%x. Read 0x%x through Mask 0xFF.\n",
		 mask,data);
       list_p->error[18] += 1;
      }

      *SC_Pix = (uchar)0xFF;
      SC_Mask = mask;
      data = *SC_Pix;
      if (data != mask) {
       printf("Device #%d. Testing Plane Mask.",list_p->device);
       printf("   Wrote 0xFF through Mask 0xFF. Read 0x%x through Mask 0x%x.\n",
		 data,mask);
       list_p->error[18] += 1;
      }
   }
}
      
word_plmask(list_p)
   struct bd_list *list_p;
{
   uchar mask,tmask,data;
   short smask,i;
   ushort *addr;
      
   Set_Ropc(8);					/* All Ropc in parallel */
   Rop_Mode(4);						
   SC_Width = 0; SC_Msk1 = 0; SC_Msk2 = 0; SC_SftVal = 0;

   smask = 255;
   while (smask > 0) {
   /* smask -= 1; mask = (uchar)smask;*/
      mask = (uchar)smask--;                    /* why? */
      SC_Mask = (uchar)0xFF; SC_Func = 0;
      *SC_RMem0 = 0;  				/* Clear all planes */

      SC_Mask = mask; SC_Src2 = 0x5555;
      SC_Func  = 0xCC;				/* Copy Source2 data */
      SC_Mask = ~mask; SC_Src2 = 0xCCCC;	
      SC_Func = 0x33;				/* Invert Source2 data */
      SC_Mask = mask;
      
      *SC_RMem0 = 0;				/* Set some planes */
      tmask = 1;

      addr = SC_Mem0;
      for (i=0;i<Planes;i++) {
         data = *addr;
	 if (((tmask & mask)&&(data != 0x5555)) ||
	     ((!(tmask & mask))&&(data != 0))) {
	     printf("Device #%d. Testing Plane Mask in word-mode. Plane %d.\n",
		 	list_p->device,i);
             printf("	Mask 0x%x. Wrote 0x5555. Read 0x%x.\n",
		 	mask,data);
	     list_p->error[19] += 1;
	 }
	 addr += 0x10000;		/* Add 128KB to base */
	 tmask <<= 1;
      }
   }
}		/* End of procedure word_plmask() */
         
