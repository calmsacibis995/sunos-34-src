
/* ======================================================================
   Author: Peter Costello
   Date :  April 15, 1983
   Purpose: Test out registers on Sun-2 Color. One routine does an
	auto-test. One does a manual test. All registers are treated
	here as word registers.
   Algorithm:
   Error Handling:
   Bugs:
   ====================================================================== */
static char     sccsid[] = "@(#)screg.c 1.1 9/25/86 Copyright Sun Micro";

#include "sc.diag.h"
extern char *error_str[];

/* This routine tests out a register. */
/* The Mask parameter defines which bits are read/write. */
test_reg(list_p,tnum,reg,mask)
   struct bd_list *list_p;
   int tnum;
   ushort *reg,mask;
{
   register ushort *reg1,data,data1,data2,data3;
   int i;

   reg1 = reg;
   for (i=mask;i>=0;i-=3) {
      data1 = i & mask;
      *reg1 = data1;
      data = *reg1 & mask;
      if (data != data1) {
	 /* Error in Register */
	 data2 = *reg1; data3 = *reg1;
	 printf("Device #%d. %s. Addr 0x%x. Wr 0x%x. Rd 0x%x, 0x%x, 0x%x.\n",
		list_p->device,error_str[tnum],(int)reg,data1,data,data2,data3);
         list_p->error[tnum] += 1;
      }
   }
}

/* This routine tests out a register and halts on an error. */
/* The Mask parameter defines which bits are read/write. */
test_halt(list_p,tnum,reg,mask)
   struct bd_list *list_p;
   int tnum;
   ushort *reg,mask;
{
   register ushort *reg1,data,data1,foo;
   int i;
   char ch;

   reg1 = reg;
   foo = 0; i = 0xFFFF;
   while ((i >= 0)||(foo)) {
      data1 = i & mask;
      *reg1 = data1;
      data = *reg1;
      data &= mask;
      if (data != data1) {
	 /* Error in Register */
	 if (foo) {
	    printf("0x%x ",data);
	 } else {
	    printf("Device #%d. %s. Wrote 0x%x. Read 0x%x.\n",
		list_p->device,error_str[tnum],data1,data);
            list_p->error[tnum] += 1;
	    printf("Hit any Character to Continue (r to read forever)");
	    ch = getchar(); printf("\n");
	    if ((ch=='r')||(ch=='R')) {
	       while (1) {
		  data = *reg1;
		  if (data != data1) printf("0x%x ",data);
	       }
	    }
	 }
      }
      i -= 1;
   }
}

/* This routine tests out a register. */
/* The Mask parameter defines which bits are read/write. */
test_altreg(list_p,tnum,reg,data,data1,print)
   struct bd_list *list_p;
   int tnum;
   ushort *reg,data,data1;
   int print;
{
   register ushort *reg1,datax,data1x;

   reg1 = reg;
   while (1) {
      *reg1 = data;
      datax = *reg1;
      *reg1 = data1;
      data1x = *reg1;
      if (print) {
         if (datax != data) {
	    printf("Device #%d. %s. Wrote 0x%x. Read 0x%x.\n",
		list_p->device,error_str[tnum],data1,data);
	 }
         if (data1x != data1) {
	    printf("Device #%d. %s. Wrote 0x%x. Read 0x%x.\n",
		list_p->device,error_str[tnum],data1,data);
	 }
      }
   }
}
	
/* Test out a register on 256 byte boundaries. */
/* The Mask parameter defines which bits are read/write. */
test_reg100(list_p,tnum,reg,mask,xinc)
   struct bd_list *list_p;
   int tnum;
   ushort *reg,mask;
   int xinc;
{
   register ushort *reg1,data,data1;
   int i;

   reg1 = reg;
   for (i=0;i<10000;i+=xinc) {
      data1 = i & mask;
      *reg1 = data1;
      data = *reg1;
      data &= mask;
      if (data != data1) {
	 /* Error in Register */
	 printf("Device #%d. %s. Wrote 0x%x. Read 0x%x.\n",
		list_p->device,error_str[tnum],data1,data);
      }
   }
}

/* This routine tests out an 8-bit register. */
/* The Mask parameter defines which bits are read/write. */
test_breg(list_p,tnum,reg,mask)
   struct bd_list *list_p;
   int tnum;
   uchar *reg,mask;
{
   register uchar *reg1,data,data1,data2,data3;
   int i;

   reg1 = reg;
   for (i=0;i<0x10000;i+=1) {
      data1 = i & mask;
      *reg1 = data1;
      data = *reg1;
      data &= mask;
      if (data != data1) {
	 /* Error in Register */
 	 data2 = *reg1; data3 = *reg1;
	 printf("Device #%d. %s. Wrote 0x%x. Read 0x%x, 0x%x, 0x%x.\n",
		list_p->device,error_str[tnum],data1,data,data2,data3);
         list_p->error[tnum] += 1;
      }
   }
}

/* This routine tests out an 8-bit register and halts on an error. */
/* The Mask parameter defines which bits are read/write. */
test_bhalt(list_p,tnum,reg,mask)
   struct bd_list *list_p;
   int tnum;
   uchar *reg,mask;
{
   register uchar *reg1,data,data1;
   int i;
   char ch;

   reg1 = reg;
   for (i=0;i<0x10000;i+=1) {
      data1 = i & mask;
      *reg1 = data1;
      data = *reg1;
      data &= mask;
      if (data != data1) {
	 /* Error in Register */
	 printf("Device #%d. %s. Wrote 0x%x. Read 0x%x.\n",
		list_p->device,error_str[tnum],data1,data);
	 printf("Hit any Character to Continue ");
	 ch = getchar(); printf("\n");
         list_p->error[tnum] += 1;
      }
   }
}

/* Test constantly a location (16-bit). Increment datum. */
test_incr(reg)
   ushort *reg;
{
   register ushort *reg1,data,temp;

   reg1 = reg;
   data = 0;
   while (1) {
      *reg1 = data;
      temp = *reg;
      data += 1;
   }
}

/* Test constantly a location (8-bit). Increment datum. */
test_bincr(reg)
   uchar *reg;
{
   register uchar *reg1,data,temp;

   reg1 = reg;
   data = 0;
   while (1) {
      *reg1 = data;
      temp = *reg;
      data += 1;
   }
}

/* Define automatic test for all control registers */
auto_reg(list_p)
   struct bd_list *list_p;
{
   test_reg(list_p,(int)0,&SCW_Stat,(ushort)0x003F);
   SC_Stat = VEnable;
   test_reg(list_p,(int)1,&SCW_Mask,(ushort)0x00FF); SCW_Mask = 0xFF;
   test_reg(list_p,(int)2,&SCW_WPan,(ushort)0xFFFF); SCW_WPan = 0;
   test_reg(list_p,(int)3,&SCW_Zoom,(ushort)0x00FF); SCW_Zoom = 0;
   test_reg(list_p,(int)4,&SCW_PPan,(ushort)0x00FF); SCW_PPan = 0;
   test_reg(list_p,(int)5,&SCW_VZoom,(ushort)0x00FF); SCW_VZoom = 0xFF;
   test_reg(list_p,(int)29,&SCW_IVect,(ushort)0x00FF); SCW_IVect = 0x54;
}


#define getnum(x) printf("   Enter Datum(hex): ");		\
	          x = getnh(); printf("\n")

/* ======================================================================
   Author: Peter Costello
   Date :  April 15, 1983
   Purpose: Do manual tests of registers (on 16-bit words)
   Algorithm:
   Error Handling:
   Bugs:
   ====================================================================== */
mtest_reg(list_p,tnum,reg1,mask)
   struct bd_list *list_p;
   int tnum;
   ushort *reg1,mask;
{
   register ushort data,	/* D7 */
		   data1,	/* D6 */
		   more; 	/* D5 */
   register ushort *reg;	/* A5 */
   int foo;
   char ch;

   reg = reg1;
   printf("Testing Address 0x%x.\n",(int)reg);
   more = 1;
   while (more) {
      printf("Test %s \n",error_str[tnum]);
      printf("   1: Read Once\n");
      printf("   2: Read Continuously\n");
      printf("   3: Write Once\n");
      printf("   4: Write Continuously\n");
      printf("   5: Write/Read Once\n");
      printf("   6: Write/Read Continuously\n");
      printf("   7: Write/Read/Compare and increment data by 1\n");
      printf("   8: Write/Read & Increment Data Continuously\n");
      printf("   9: Write/Read/Compare and stop/read-forever on error\n");
      printf("   A: Write/Read/Compare and increment data by n\n");
      printf("   B: Write/Read alternating data\n");
      printf("   Q: Quit\n");
      printf("   Enter Choice: ");
      ch = getchar(); printf("\n");
      if (ch == '1') {
	 data = *reg & mask;
	 printf("%s. Read: 0x%x.\n",error_str[tnum],data);
      } else if (ch == '2') {
	 data = *reg & mask;
	 printf("%s. First Read: 0x%x.\n",error_str[tnum],data);

	 while (1) {
	    if (data != (*reg & mask)) printf("E");
	 }

      } else if (ch == '3') {
	 getnum(data);
	 *reg = data;
	 printf("%s. Wrote: 0x%x.\n",error_str[tnum],data);
      } else if (ch == '4') {
	 getnum(data);
	 *reg = data;
	 printf("%s. First Write: 0x%x.\n",error_str[tnum],data);

	 /* while (1) *reg = data; */
label4:  asm("	movw	d7,a5@");
	 goto label4;

      } else if (ch == '5') {
	 getnum(data);
	 *reg = data;
  	 data1 = *reg;
    	 data1 &= mask;
	 printf("%s. Wrote: 0x%x. Read: 0x%x.\n",error_str[tnum],data,data1);
      } else if (ch == '6') {
	 getnum(data);
	 *reg = data;
  	 data1 = *reg & mask;
	 printf("%s. First Write: 0x%x. First Read: 0x%x.\n",
		error_str[tnum],data,data1);

	 while (1) {
	    *reg = data; 
	    if (data != (*reg & mask)) printf("E");
  	 }

      } else if (ch == '7') {
         test_reg(list_p,tnum,reg,mask);
      } else if (ch == '8') {
         test_incr(reg);
      } else if (ch == '9') {
         test_halt(list_p,tnum,reg,mask);
      } else if ((ch == 'a')||(ch == 'A')) {
	 printf("   Enter increment(hex): ");
         foo = getnh(); printf("\n");
 	 test_reg100(list_p,tnum,reg,mask,foo);
      } else if ((ch == 'b')||(ch == 'B')) {
	 printf("   Enter first Datum(hex): ");
         data = getnh(); printf("\n");
	 printf("   Enter second Datum(hex): ");
         data1 = getnh(); printf("\n");
	 printf("   Print Error Messages (y/n)? ");
	 ch = getchar(); printf("\n");
         if ((ch=='n')||(ch=='N')) {
	    test_altreg(list_p,tnum,reg,data,data1,0);
	 } else {
	    test_altreg(list_p,tnum,reg,data,data1,1);
	 }
      } else if ((ch == 'Q')||(ch == 'q')) {
	 more = 0;
      }
   }
}		/* End of mtest_reg */

/* ======================================================================
   Author: Peter Costello
   Date :  April 15, 1983
   Purpose: Do manual tests of registers (on 8-bit words)
   Algorithm:
   Error Handling:
   Bugs:
   ====================================================================== */
mtest_breg(list_p,tnum,reg1,mask)
   struct bd_list *list_p;
   int tnum;
   uchar *reg1,mask;
{
   register uchar  data,	/* D7 */
		   data1,	/* D6 */
		   more; 	/* D5 */
   register uchar *reg;		/* A5 */
   char ch;

   reg = reg1;
   printf("Testing Address 0x%x.\n",(int)reg);
   more = 1;
   while (more) {
      printf("Test %s \n",error_str[tnum]);
      printf("   1: Read Once\n");
      printf("   2: Read Continuously\n");
      printf("   3: Write Once\n");
      printf("   4: Write Continuously\n");
      printf("   5: Write/Read Once\n");
      printf("   6: Write/Read Continuously\n");
      printf("   7: Write/Read/Compare 64K times\n");
      printf("   8: Write/Read & Increment Data Continuously\n");
      printf("   9: Write/Read/Compare and stop on error\n");
      printf("   Q: Quit\n");
      printf("   Enter Choice: ");
      ch = getchar(); printf("\n");
      if (ch == '1') {
	 data = *reg & mask;
	 printf("%s. Read: 0x%x.\n",error_str[tnum],data);
      } else if (ch == '2') {
	 data = *reg & mask;
	 printf("%s. First Read: 0x%x.\n",error_str[tnum],data);

	 while (1) {
	    if (data != (*reg & mask)) printf("E");
	 }

      } else if (ch == '3') {
	 getnum(data);
	 *reg = data;
	 printf("%s. Wrote: 0x%x.\n",error_str[tnum],data);
      } else if (ch == '4') {
	 getnum(data);
	 *reg = data;
	 printf("%s. First Write: 0x%x.\n",error_str[tnum],data);

	 /* while (1) *reg = data; */
label4:  asm("	movb	d7,a5@");
	 goto label4;

      } else if (ch == '5') {
	 getnum(data);
	 *reg = data;
  	 data1 = *reg & mask;
	 printf("%s. Wrote: 0x%x. Read: 0x%x.\n",error_str[tnum],data,data1);
      } else if (ch == '6') {
	 getnum(data);
	 *reg = data;
  	 data1 = *reg & mask;
	 printf("%s. First Write: 0x%x. First Read: 0x%x.\n",
		error_str[tnum],data,data1);

	 while (1) {
	    *reg = data; 
	    if (data != (*reg & mask)) printf("E");
	 }

      } else if (ch == '7') {
         test_breg(list_p,tnum,reg,mask);
      } else if (ch == '8') {
         test_bincr(reg);
      } else if (ch == '9') {
         test_bhalt(list_p,tnum,reg,mask);
      } else if ((ch == 'Q')||(ch == 'q')) {
	 more = 0;
      }
   }
}		/* End of mtestb_reg */


/* Manual Register Test. Prompt user for the register he wants to check. */
man_reg(list_p)
   struct bd_list *list_p;
{
   short more;
   char ch;

   more = 1;
   while (more) {
      printf("MANUAL REGISTER TESTS\n");
      printf("   1: Status Register\n");
      printf("   2: Per_Plane Mask Register\n");
      printf("   3: Word Pan Register\n");
      printf("   4: Pixel Pan Register\n");
      printf("   5: Line Offset and Zoom Register\n");
      printf("   6: Variable Zoom Register\n");
      printf("   7: Interrupt Vector Register\n");
      printf("   Q: Quit\n");
      printf("   Enter Choice: ");
      ch = getchar(); printf("\n");
      if (ch == '1') {
         mtest_reg(list_p,0,&SCW_Stat,(ushort)0x003F);
      } else if (ch == '2') {
         mtest_reg(list_p,1,&SCW_Mask,(ushort)0x00FF);
      } else if (ch == '3') {
         mtest_reg(list_p,2,&SC_WPan,(ushort)0xFFFF);
      } else if (ch == '4') {
         mtest_reg(list_p,4,&SCW_PPan,(ushort)0x00FF);
      } else if (ch == '5') {
         mtest_reg(list_p,3,&SCW_Zoom,(ushort)0x00FF);
      } else if (ch == '6') {
         mtest_reg(list_p,5,&SCW_VZoom,(ushort)0x00FF);
      } else if (ch == '7') {
         mtest_reg(list_p,29,&SCW_IVect,(ushort)0x00FF);
      } else if ((ch == 'q')||(ch == 'Q')) {
	 more = 0;
      }
   }
}		/* End of procedure man_reg() */


/* Do manual tests on a ROPC's registers. Assume ROPC already selected.  */
man_rop_reg(list_p,tnum)
   struct bd_list *list_p;
   int tnum;
{
   short more;
   char ch;

   more = 1;
   while (more) {
      printf("Testing Registers on ROPC for plane %d\n",tnum-10);
      printf("   1: Destination Register\n");
      printf("   2: Source 1 Register\n");
      printf("   3: Source 2 Register\n");
      printf("   4: Pattern Register\n");
      printf("   5: Mask 1 Register\n");
      printf("   6: Mask 2 Register\n");
      printf("   7: Shift Value Register\n");
      printf("   8: Function Register\n");
      printf("   9: Width Register\n");
      printf("   A: Op Count Register\n");
      printf("   B: Function Output Register\n");
      printf("   C: Manual Load Destination Register\n");
      printf("   D: Manual Load Source Register\n");
      printf("   E: Flag Register\n");
      printf("   Q: Quit\n");
      printf("   Enter Choice: ");
      ch = getchar(); printf("\n");
      if (ch == '1') {
         mtest_reg(list_p,tnum,&SC_Dst,0xFFFF);
      } else if (ch == '2') {
         mtest_reg(list_p,tnum,&SC_Src1,0xFFFF);
      } else if (ch == '3') {
         mtest_reg(list_p,tnum,&SC_Src2,0xFFFF);
      } else if (ch == '4') {
         mtest_reg(list_p,tnum,&SC_Pat,0xFFFF);
      } else if (ch == '5') {
         mtest_reg(list_p,tnum,&SC_Msk1,0xFFFF);
      } else if (ch == '6') {
         mtest_reg(list_p,tnum,&SC_Msk2,0xFFFF);
      } else if (ch == '7') {
         mtest_reg(list_p,tnum,&SC_SftVal,0x010F);
      } else if (ch == '8') {
         mtest_reg(list_p,tnum,&SC_Func,0x00FF);
      } else if (ch == '9') {
         mtest_reg(list_p,tnum,&SC_Width,0xFFFF);
      } else if ((ch == 'a')||(ch == 'A')) {
         mtest_reg(list_p,tnum,&SC_OpCnt,0xFFFF);
      } else if ((ch == 'b')||(ch == 'B')) {
         mtest_reg(list_p,tnum,&SC_Fout,0xFFFF);
      } else if ((ch == 'c')||(ch == 'C')) {
         mtest_reg(list_p,tnum,&SC_Ldst,0xFFFF);
      } else if ((ch == 'd')||(ch == 'D')) {
         mtest_reg(list_p,tnum,&SC_Lsrc,0xFFFF);
      } else if ((ch == 'e')||(ch == 'E')) {
         mtest_reg(list_p,tnum,&SC_Flag,0x00FF);
      } else if ((ch == 'q')||(ch == 'Q')) {
         more = 0;
      }
   }
}		/* End of procedure man_rop_reg() */

