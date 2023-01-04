/* ======================================================================
   Author: Peter Costello
   Date :  June 15, 1983
   Purpose: Test Implicit loading of destination registers on ROPC
	using all possible addressing modes.
   Algorithm:
   Timing:
   Error Handling:
   Bugs:
   ====================================================================== */
static char     sccsid[] = "@(#)autodst.c 1.1 9/25/86 Copyright Sun Micro";

#include "sc.diag.h"
extern int halt_on_err;
char ch;

/* Test implicit loading of destination register. Addressing Mode 0. */
auto_ldst0(list_p,addr1,tnum)
   struct bd_list *list_p;
   ushort *addr1;
   short tnum;
{
   register int i;
   register ushort *addr,data,data1,data2;

   addr = addr1;
   SC_Width = 0; SC_Msk1  = 0; SC_Msk2  = 0; SC_SftVal = 0;
   SC_Func  = 0x33;		/* Invert Source data */

   /* Reads and Writes have no effect. */
   for (i=0;i<0x10000;i+=41) {
      data = (ushort)i;
      SC_Dst = (ushort)0x5555;
      data2 = *addr; 		/* Phony Read */
      *addr = (ushort)0x5555; 	/* Phony write */
      *addr = data; 		/* Phony Write */
      *addr = (ushort)0x3333;	/* Phony Write */
      data2 = *addr;		/* Phony Read */
      data1 = SC_Dst;
      if (data1 != (ushort)0x5555) {
        list_p->error[tnum] += 1;
        printf("\nDevice #%d. Testing Dst. Addressing Mode 0. %s.\n",
	         list_p->device,error_str[tnum]);
        printf("     Read: 0x%x  Expected: 0x5555\n",data1);
	if (halt_on_err) { 
	   printf("Hit any char to continue");
	   ch = getchar(); printf("\n");
	}
      }
   }
}		/* End of auto_ldst0() */

/* Test implicit loading of destination register. Addressing Mode 1. */
auto_pldst1(list_p,addr1,tnum)
   struct bd_list *list_p;
   uchar *addr1;
   short tnum;
{
   register int i;
   register ushort data1;
   register uchar *addr,data,data2;

   addr = addr1;
   SC_Width = 0; SC_Msk1  = 0; SC_Msk2  = 0; SC_SftVal = 0;
   SC_Func  = 0x33;		/* Invert Source data */

   /* Reads and writes have no effect. */
   for (i=0;i<0x10000;i+=41) {
      data = (uchar)i;
      SC_Dst = (ushort)0x5555;
      data2 = *addr; 		/* Phony Read */
      *addr = (uchar)0x55; 	/* Phony write */
      *addr = data; 		/* Phony Write */
      *addr = (uchar)0x33;	/* Phony Write */
      data2 = *addr;		/* Phony Read */
      data1 = SC_Dst;
      if (data1 != (ushort)0x5555) {
        list_p->error[tnum] += 1;
        printf("\nDevice #%d. Testing Dst. Addressing Mode 1. %s.\n",
	        list_p->device,error_str[tnum]);
        printf("     Addr: 0x%x  Read: 0x%x  Expected: 0x5555\n",
		(int)addr,data1);
	if (halt_on_err) { 
	   printf("Hit any char to continue");
	   ch = getchar(); printf("\n");
	}
      }
   }
}		/* End of auto_pldst1() */

/* Test implicit loading of destination register. ROPC Word Memory. */
auto_ldst2(list_p,addr1,tnum)
   struct bd_list *list_p;
   ushort *addr1;
   short tnum;
{
   register int i;
   register ushort *addr,data,data1,data2;

   addr = addr1;
   SC_Width = 0; SC_Msk1  = 0; SC_Msk2  = 0; SC_SftVal = 0; SC_Pat = 0x5555;
   SC_Func  = 0x33; 		/* Invert Source data */

   for (i=0;i<0x10000;i+=41) {
      data = (ushort)i;
      data2 = (ushort)data<<1 + data;
      SC_Dst = 0;		/* Stuff with invalid value */
      *addr = data;		/* Load SRC2 with data */
      *addr = data2; 		/* Write data to SRC1 to FB */
      data1 = *addr; 		/* Read to Dst */
      *addr = data2; 		/* Write data2 to SRC1 to FB */
      *addr = data2; 		/* Write data2 to SRC1 to FB */
      data1 = SC_Dst;

      if ((data1 != ((ushort)~data)) ||
          (SC_Src1 != ((ushort)data2)) ||
          (SC_Src2 != ((ushort)data2)) ) {
         list_p->error[tnum] += 1;
         printf("\nDevice #%d. Testing Dst. Addressing Mode 2. %s.\n",
	         list_p->device,error_str[tnum]);
         printf("     Dst: 0x%x  Expected: 0x%x\n",data1,(ushort)(~data));
         printf("     Src1: 0x%x  Expected: 0x%x\n",SC_Src1,(ushort)data2);
         printf("     Src2: 0x%x  Expected: 0x%x\n",SC_Src2,(ushort)data2);
	 if (halt_on_err) { 
	    printf("Hit any char to continue");
	    ch = getchar(); printf("\n");
	 }
      }
   }
}		/* End of auto_ldst2() */

/* Test implicit loading of destination register. ROPC Word Memory. */
auto_ldst4(list_p,addr1,tnum)
   struct bd_list *list_p;
   ushort *addr1;
   short tnum;
{
   register int i;
   register ushort *addr,data,data1,data2;

   addr = addr1;
   SC_Width = 0; SC_Msk1  = 0; SC_Msk2  = 0; SC_SftVal = 0; SC_Pat = 0x5555;
   SC_Func  = 0x33;		/* Invert Source data */

   for (i=0;i<0x10000;i+=41) {
      data = (ushort)i;
      data2 = (ushort)data<<1 + data;
      SC_Dst = (ushort)data;

      *addr = data; 		/* Write data to SRC2. SRC2 to SRC1 to FB */
      *addr = data2; 		/* Write data to SRC1 to FB */
      *addr = data2; 		/* HRead of ~data to DST */
      data1 = *addr; 		/* Phony Read does not touch Dst */
      data1 = SC_Dst;

      if ((data1 != ((ushort)~data)) ||
          (SC_Src1 != ((ushort)data2)) ||
          (SC_Src2 != ((ushort)data2)) ) {
         list_p->error[tnum] += 1;
         printf("\nDevice #%d. Testing Dst. Addressing Mode 4. %s.\n",
	         list_p->device,error_str[tnum]);
         printf("     Dst: 0x%x  Expected: 0x%x\n",data1,(ushort)(~data));
         printf("     Src1: 0x%x  Expected: 0x%x\n",SC_Src1,(ushort)data2);
         printf("     Src2: 0x%x  Expected: 0x%x\n",SC_Src2,(ushort)data2);
	 if (halt_on_err) { 
	    printf("Hit any char to continue");
	    ch = getchar(); printf("\n");
	 }
      }
   }
}		/* End of auto_ldst4() */


/* Test implicit loading of destination register. ROPC Word Memory. */
auto_ldst6(list_p,addr1,tnum)
   struct bd_list *list_p;
   ushort *addr1;
   short tnum;
{
   register int i;
   register ushort *addr,data,data1,data2;

   Rop_Mode(4); addr = addr1;
   SC_Width = 0; SC_Msk1  = 0; SC_Msk2  = 0; SC_SftVal = 0; SC_Pat = 0x5555;
   SC_Func  = 0x33; 		/* Invert Source data */

   data = *addr;		/* Load masks */
   for (i=0;i<0x10000;i+=41) {
      data = (ushort)i;
      data2 = (ushort)data<<1 + data;
      SC_Dst = 0;		/* Stuff with invalid value */
      SC_Src1 = data;		/* Load SRC1 with data */
      SC_Src2 = data2;		/* Load SRC2 with invalid data */
      *addr = data2; 		/* Write inverted data from SRC1 to FB */
      data1 = *addr; 		/* Read data to Dst and Src2. */
      *addr = data2; 		/* Write ~0 to FB */
      *addr = data2; 		/* Write ~0 to FB */
      data1 = SC_Dst;

      if ((data1 != ((ushort)~data)) ||
          (SC_Src1 != ((ushort)data2)) ||
          (SC_Src2 != data1)) {
         list_p->error[tnum] += 1;
         printf("\nDevice #%d. Testing Dst. Addressing Mode 6. %s.\n",
	         list_p->device,error_str[tnum]);
         printf("     Dst: 0x%x  Expected: 0x%x\n",data1,(ushort)(~data));
         printf("     Src1: 0x%x  Expected: 0x%x\n",SC_Src1,(ushort)data2);
         printf("     Src2: 0x%x  Expected: 0x%x\n",SC_Src2,(ushort)(~data));
	 if (halt_on_err) { 
	    printf("Hit any char to continue");
	    ch = getchar(); printf("\n");
	 }
      }
   }
}		/* End of auto_ldst6() */


/* Test implicit loading of destination register. ROPC Word Memory. */
auto_ldst8(list_p,addr1,tnum)
   struct bd_list *list_p;
   ushort *addr1;
   short tnum;
{
   register int i;
   register ushort *addr,data,data1,data2;

   addr = addr1;
   SC_Width = 0; SC_Msk1  = 0; SC_Msk2  = 0; SC_SftVal = 0; SC_Pat = 0x5555;
   SC_Func  = 0x33; 		/* Invert Source data */

   for (i=0;i<0x10000;i+=41) {
      data = (ushort)i;
      data2 = (ushort)data<<1 + data;
      SC_Dst = 0;		/* Stuff with invalid value */
      SC_Src1 = data;		/* Load SRC1 with data */
      SC_Src2 = data2;		/* Load SRC2 with invalid data */
      *addr = data2; 		/* Write inverted data from SRC1 to FB */
      data1 = *addr; 		/* Read data to SRC2 */
      *addr = data2; 		/* HRead inverted data to DST */
      data1 = SC_Dst;

      if ((data1 != ((ushort)~data)) ||
          (SC_Src1 != ((ushort)data2)) ||
          (SC_Src2 != ((ushort)~data)) ) {
         list_p->error[tnum] += 1;
         printf("\nDevice #%d. Testing Dst. Addressing Mode 8. %s.\n",
	         list_p->device,error_str[tnum]);
         printf("     Dst: 0x%x  Expected: 0x%x\n",data1,(ushort)(~data));
         printf("     Src1: 0x%x  Expected: 0x%x\n",SC_Src1,(ushort)data2);
         printf("     Src2: 0x%x  Expected: 0x%x\n",SC_Src2,(ushort)(~data));
	 if (halt_on_err) { 
	    printf("Hit any char to continue");
	    ch = getchar(); printf("\n");
	 }
      }
   }
}		/* End of auto_ldst8() */

