
/* ======================================================================
   Author: Peter Costello
   Date :  June 15, 1983
   Purpose: Test out implicit loads of source register for each addressing
	modes.
   Algorithm:
   Timing:
   Error Handling:
   Bugs:
   ====================================================================== */
static char     sccsid[] = "@(#)autosrc.c 1.1 9/25/86 Copyright Sun Micro";

#include "sc.diag.h"
extern int halt_on_err;
#define Sfact 27	/* Was 41 */

/* Test implicit load of SRC register. Addressing Mode 0. */
auto_lsrc0(list_p,addr1,tnum)
   struct bd_list *list_p;
   ushort *addr1;
   short tnum;
{
   register int i;
   register ushort data1,data2;
   register ushort *addr;			/* Address in plane */
   char ch;

   addr = addr1;
   SC_Width = 0; SC_Msk1  = 0; SC_Msk2  = 0; SC_SftVal = 0;
   SC_Func  = 0x33;		/* Invert Source data */

   SC_Src1 = 0x5555; 
   SC_Src2 = 0x5555;
   addr = SC_Mem0;
   /* Should not load source on read or write */
   for (i=0;i<0x10000;i+=Sfact) {
      *addr = (ushort)i;	/* Phony Write */
      *addr = (ushort)i;	/* Phony Write */
      data2 = *addr; 		/* Phony Read */
      data1 = SC_Src1; 
      data2 = SC_Src2;
      if ((data1 != 0x5555)||(data2 != 0x5555)) {
         list_p->error[tnum] += 1;
         printf("Device #%d. Testing SRC. Word-Mode. %s.",
	         list_p->device,error_str[tnum]);
         printf("     Read: 0x%x  Expected: 0x%x\n",data1,0x5555);
	 if (halt_on_err) { 
	    printf("Hit any char to continue");
	    ch = getchar(); printf("\n");
	 }
      }
   }
}	/* End of auto_lsrc0 */

/* Test implicit load of SRC register. Addressing Mode 1. */
auto_plsrc1(list_p,addr1,tnum)
   struct bd_list *list_p;
   uchar *addr1;
   short tnum;
{
   register int i;
   register ushort data1,data2;
   register uchar *addr,data,data3;
   char ch;

   addr = addr1;
   SC_Width = 0; SC_Msk1  = 0; SC_Msk2  = 0; SC_SftVal = 0;
   SC_Func  = 0x33;		/* Invert Source data */

   SC_Src1 = (ushort)0x5555; 
   SC_Src2 = (ushort)0x5555;
   /* Should not load source on read or write */
   for (i=0;i<0x10000;i+=Sfact) {
      data = (uchar)i;
      *addr = data; 		/* Phony Write */
      *addr = (uchar)0x33;	/* Phony Write */
      data3 = *addr; 		/* Phony Read */
      data1 = SC_Src1; 
      data2 = SC_Src2;
      if ((data1 != 0x5555)||(data2 != 0x5555)) {
         list_p->error[tnum] += 1;
         printf("Device #%d. Testing SRC. Pixel-Mode. %s.",
	         list_p->device,error_str[tnum]);
         printf("     Read: 0x%x  Expected: 0x%x\n",data1,0x5555);
	 if (halt_on_err) { 
	    printf("Hit any char to continue");
	    ch = getchar(); printf("\n");
	 }
      }
   }
}	/* End of auto_plsrc1 */

/* Test implicit read load of SRC register. Addressing Mode 2. */
auto_lsrcr(list_p,addr1,tnum)
   struct bd_list *list_p;
   ushort *addr1;
   short tnum;
{
   register int i;
   register ushort *addr,data,data1,data2,data3;
   char ch;

   addr = addr1;
   SC_Width = 0; SC_Msk1  = 0; SC_Msk2  = 0; SC_SftVal = 0; /* Src2 -> Src1 */
   SC_Func  = 0x33;		/* SRC2 to SRC1, Inverted SRC1 to FB */

   /* Write to Src2. Verify that old Src2 moved to Src1 */
   for (i=1;i<0x10000;i+=41) {
      SC_Src1 = i-1; 				/* Prime Src1 */
      SC_Src2 = data = (ushort)i;		/* Prime Src2 */
      *addr = 0;				/* Write to FB Only */
      data3 = *addr;			 	/* Read to SRC2 */
      data1 = SC_Src1;
      data2 = SC_Src2;
      if ((data1 != data) ||
	  (data2 != ((ushort)(~(data-1)))) ||
	  (data3 != ((ushort)(~(data-1)))) ) {
         list_p->error[tnum] += 1;
         printf("Device #%d. Testing SRC load on Read. Addr Mode %d. %s.\n",
		list_p->device,2+(SC_Stat>>3),error_str[tnum]);
         printf("    SRC1: 0x%x  Expected: 0x%x  SRC2: 0x%x  Expected: 0x%x\n",
		data1,data,data2,(ushort)(~(data-1)));
         printf("    FB: 0x%x  Expected: 0x%x\n",data3,(ushort)(~(data-1)));
      }
   }

   /* Write to Src1. Verify that old Src1 moved to Src2 */
   SC_SftVal = 0x100;
   for (i=1;i<0x10000;i+=41) {
      SC_Src2 = i-1; 				/* Prime Src1 */
      SC_Src1 = data = (ushort)i;		/* Prime Src2 */
      *addr = 0;				/* Write to FB Only */
      data3 = *addr;
      data1 = SC_Src1;
      data2 = SC_Src2;
      if ((data1 != ((ushort)(~(data-1)))) ||
	  (data2 != ((ushort)data)) ||
	  (data3 != ((ushort)(~(data-1)))) ) {
         list_p->error[tnum] += 1;
         printf("Device #%d. Testing SRC load on Read. Addr Mode %d. %s.\n",
		list_p->device,2+(SC_Stat>>3),error_str[tnum]);
         printf("    SRC1: 0x%x  Expected: 0x%x  SRC2: 0x%x  Expected: 0x%x\n",
		data1,(ushort)(~(data-1)),data2,(ushort)data);
         printf("    FB: 0x%x  Expected: 0x%x\n",data3,(ushort)(~(data-1)));
	 if (halt_on_err) { 
	    printf("Hit any char to continue");
	    ch = getchar(); printf("\n");
	 }
      }
   }
}	/* Routine auto_lsrcr(list_p,addr,tnum) */

/* Test implicit load of SRC register. Addressing Mode 2. */
auto_lsrcw(list_p,addr1,tnum)
   struct bd_list *list_p;
   ushort *addr1;
   short tnum;
{
   register int i;
   register ushort *addr,data,data1,data2,data3;
   char ch;

   addr = addr1;
   SC_Width = 0; SC_Msk1  = 0; SC_Msk2  = 0; SC_SftVal = 0; /* Src2 -> Src1 */
   SC_Func  = 0x33;		/* SRC2 to SRC1, Inverted SRC1 to FB */

   /* Write to Src2. Verify that old Src2 moved to Src1 */
   SC_Src2 = 0; 				/* Prime Src2 */
   for (i=Sfact;i<0x10000;i+=Sfact) {
      *addr = data = (ushort)i;	  /* Load Src2. Old Src2 to Src1 to FB */
      data3 = *addr;
      data1 = SC_Src1;
      data2 = SC_Src2;
      if ((data1 != ((ushort)(data-Sfact))) ||
	  (data2 != data) ||
	  (data3 != ((ushort)(~(data-Sfact)))) ) {
         list_p->error[tnum] += 1;
         printf("Device #%d. Test SRC2->SRC1 on Write. Addr Mode %d. %s.\n",
		list_p->device,2+(SC_Stat>>3),error_str[tnum]);
         printf("    SRC1: 0x%x  Expected: 0x%x  SRC2: 0x%x  Expected: 0x%x\n",
		data1,(ushort)data-1,data2,data);
         printf("    FB: 0x%x  Expected: 0x%x\n",data3,(ushort)(~(data-1)));
	 if (halt_on_err) { 
	    printf("Hit 'c' to continue");
	    while (getchar()!='c');
	    printf("\n");
	 }
      }
   }

   /* Write to Src1. Verify that old Src1 moved to Src2 */
   SC_SftVal = 0x100;				/* Src1 -> Src2 */
   SC_Src1 = 0; 				/* Prime Src1 */
   for (i=Sfact;i<0x10000;i+=Sfact) {
      *addr = data = (ushort)i;	  /* On Wr: Load Src1 w/ data. Src2 w/ data-1 */
      data3 = *addr;
      data1 = SC_Src1;
      data2 = SC_Src2;
      if ((data1 != data) ||
	  (data2 != ((ushort)(data-Sfact))) ||
	  (data3 != ((ushort)(~(data-Sfact)))) ) {
         list_p->error[tnum] += 1;
         printf("Device #%d. Test SRC1->SRC2 on Write. Addr Mode %d. %s.\n",
		list_p->device,2+(SC_Stat>>3),error_str[tnum]);
         printf("    SRC1: 0x%x  Expected: 0x%x  SRC2: 0x%x  Expected: 0x%x\n",
		data1,data,data2,(ushort)data-1);
         printf("    FB: 0x%x  Expected: 0x%x\n",data3,(ushort)(~(data-1)));
	 if (halt_on_err) { 
	    printf("Hit any char to continue");
	    ch = getchar(); printf("\n");
	 }
      }
   }
}	/* Routine auto_lsrcw(list_p,addr,tnum) */

