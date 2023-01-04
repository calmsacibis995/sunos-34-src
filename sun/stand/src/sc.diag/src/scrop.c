
/* ======================================================================
   Author: Peter Costello
   Date :  April 15, 1983
   Purpose:  Perform auto and manual tests on Raster Op Chips 
   Algorithm:
   Timing:
   Error Handling:
   Bugs:
   ====================================================================== */
static char     sccsid[] = "@(#)scrop.c 1.1 9/25/86 Copyright Sun Micro";

#include "sc.diag.h"

extern int halt_on_err;

/* Define automatic register tests for a single ROPC */
auto_rreg(list_p,tnum)
   struct bd_list *list_p;
   int tnum;
{
   test_reg(list_p,tnum,&SC_Dst,0xFFFF);
   test_reg(list_p,tnum,&SC_Src1,0xFFFF);
   test_reg(list_p,tnum,&SC_Src2,0xFFFF);
   test_reg(list_p,tnum,&SC_Pat,0xFFFF);
   test_reg(list_p,tnum,&SC_Msk1,0xFFFF);
   test_reg(list_p,tnum,&SC_Msk2,0xFFFF);
   test_reg(list_p,tnum,&SC_SftVal,0x010F);
   test_reg(list_p,tnum,&SC_Func,0x00FF);
   test_reg(list_p,tnum,&SC_Width,0xFFFF);
   test_reg(list_p,tnum,&SC_OpCnt,0xFFFF);
   test_reg(list_p,tnum,&SC_Flag,0x00FF);
}		/* End of routine auto_rreg */

/* Return a 16-bit psuedo-random number. */
int rseed=271;
int rpass=0;
#define rand (rseed = (rseed << 2) + rseed + 17623 + ((rpass++)&1))

/* define error routine for auto_rfun() */
rfun_err(list_p,tnum,dir,src,src1,src2,sftamt,dst,pat,func,r1,r2,r3)
   struct bd_list *list_p;
   int tnum;
   ushort dir,src,src1,src2,sftamt,dst,pat,func,r1,r2,r3;
{
   list_p->error[tnum] += 1;
   printf("Device #%d. Testing Function Unit on %s.\n",
	   list_p->device,error_str[tnum]);
   if (dir&1) {
      printf("     Shifting from SRC1 to SRC2. ");
   } else {
      printf("     Shifting from SRC2 to SRC1. ");
   }
   printf("Shift Amount = %d\n",sftamt);
   printf("     Source2  = 0x%x   Source1 = 0x%x\n",src2,src1);
   printf("     Extracted Src = 0x%x   Destination = 0x%x\n",src,dst);
   printf("     Pattern       = 0x%x   Function    = 0x%x\n",pat,func);
   printf("     Func Output   = 0x%x   FBuf Read   = 0x%x\n",r1,r2);
   printf("     Expected        0x%x\n\n",r3);

}		/* End of rfun_err() */

   
/* Test function outputs. Excercises Pat,Dst,Src,SftCnt */
auto_rfun(list_p,addr,tnum)
   struct bd_list *list_p;
   ushort *addr;		/* Address in that plane */
   int tnum;
{
   register int bit,i,j;
   register ushort src,dst,pat,func,result1;
   ushort src1,src2,result2,result3,fun[8];
   ushort sftamt,error,foo,foo1,foo2;
   char ch;
   ushort dir;			/* 0: Src2->Src1. 1: Src1->Src2. */

   Rop_Mode(0);			/* Use RasterOp mode #0 */

   SC_Width = 0;		/* Enable Mask1 and Mask2 continuously */
   SC_Msk1  = 0;		/* Enable all bits in Mask1 */
   SC_Msk2  = 0;		/* Enable all bits in Mask2 */

   for (sftamt=0;sftamt<17;sftamt+=1) {	/* For all possible shift amounts */
      dir = (sftamt == 0x10);
      SC_SftVal = ((dir<<8)|(sftamt));
      for (j=0;j<256;j+=1) {
         SC_Src1 = src1 = rand&0xFFFF;  	/* Random data */
         SC_Src2 = src2 = rand&0xFFFF;		/* Random data */
         SC_Dst  = dst  = rand&0xFFFF;		/* Random data */
         SC_Pat  = pat  = rand&0xFFFF;		/* Random data */
         SC_Func = func = rand&0x00FF;		/* Random data */
	 result1 = SC_Fout;

	 if (dir) {
	    SC_Src1 = src2;			/* Src1 -> Src2 */
	    *addr = src1;
	 } else {
	    SC_Src2 = src1;			/* Src2 -> Src1 */
            *addr = src2;
	 }

	 result2 = *addr;
	 foo = 16-sftamt;			/* Unroll for compiler bug */
	 foo = 16-sftamt;			/* Unroll for compiler bug */
	 src = src2 << foo;
         src |= (src1 >> sftamt);
	 src &= 0xFFFF;				/* Extracted source */
	
 	 error = 0; result3 = result1;		/* No errors yet */
	 if (result1 != result2) {
	    error = 1;				/* Data from ROPC not written */
	 }

	 /* Decompose function bits */
	 fun[0] =  (func & 0x1);
	 fun[1] = ((func & 0x2)>>1);
	 fun[2] = ((func & 0x4)>>2);
	 fun[3] = ((func & 0x8)>>3);
	 fun[4] = ((func & 0x10)>>4);
	 fun[5] = ((func & 0x20)>>5);
	 fun[6] = ((func & 0x40)>>6);
	 fun[7] = ((func & 0x80)>>7);
	
	 /* Check each bit in result */
	 i = 1;
	 for (bit=0;bit<16;bit++) {
	    foo  = ((pat&i)!=0);	/* Simplify expr for compiler */
	    foo1 = ((src&i)!=0);
	    foo2 = ((dst&i)!=0);
            foo  = (foo<<2) + (foo1<<1) + foo2;
	    if (((result1&i) != 0) != fun[foo]) {
	        error = 1;
		result3 ^= i;			/* Fix bit */
	    }
	    i <<= 1;
	 }

	 /* Print error if error */
	 if (error) {
	    printf("ERROR: J = %d\n",j);
	    rfun_err(list_p,tnum,dir,src,src1,src2,sftamt,
		             dst,pat,func,result1,result2,result3);
	    if (halt_on_err) {
	       printf("Hit any Character to Continue ('w' to loop forever) ");
	       ch = getchar(); printf("\n");
	       if ((ch == 'w')||(ch == 'W')) {
	         if (dir) {
		    while (1) {
	               SC_Src1 = src2;			/* Src1 -> Src2 */
	               *addr = src1;
	               result2 = *addr;
		       if (result2 != result3) printf("0x%x ",result2);
	            }
	         } else {
		    while (1) {
	               SC_Src2 = src1;			/* Src2 -> Src1 */
                       *addr = src2;
	               result2 = *addr;
		       if (result2 != result3) printf("0x%x ",result2);
	            }
	         }
	      }
	    }
         }
      }
   }
}		/* End of routine auto_rfun() */

/* Define auto test for right and left masks on ropc chip */
/* Test proper decrement and reload of Opcounter. At each step read
   diagnostic function output and write to frame buffer. Verify that
   right and left masks are enabled at the proper time. */

a_msk(list_p,addr,tnum,count,msk1,msk2)
   struct bd_list *list_p;
   ushort *addr;
   int tnum;
   int count;
   ushort msk1,msk2;
{
   register ushort *taddr,r1,r2;
   register int tcount;
   register ushort msk1i,msk2i;
   
   if (((((int)addr)&0x1FFFE) + (count << 1)) > 0x1FFFE) goto return1;

   ROP_Mode(2);		/* Do hidden Read cycles. LD_DST decrements OPCNT */
   SC_SftVal = 0;
   SC_Src2 = 0;
   SC_Func = 0;
   SC_Msk1 = 0;
   SC_Msk2 = 0;
   taddr = addr;
   for (tcount=count;tcount>=0;tcount-=1) {
      *taddr++ = 0;		/* Zero Memory */
   } 

   SC_Msk1 = msk1; msk1i = (~msk1)&0xFFFF;
   SC_Msk2 = msk2; msk2i = (~msk2)&0xFFFF;
   SC_OpCnt = (ushort) count;
   SC_Width = (ushort) count;
   SC_SftVal = 0;
   SC_Func = (ushort) 0xFF;
  
   /* Write Pattern */
   SC_Src2 = 0xFFFF;
   taddr = addr;
   for (tcount= count;tcount>=0;tcount-=1) {
      *taddr = 0xFFFF;		/* Write using HRead cycles */
      r1 = SC_Fout;
      r2 = *taddr++;		/* Read does not do LD_DST */

      if ((tcount == 0)&&(SC_OpCnt != count)) {
         printf("Device #%d. Error Testing Masks on %s.\n",
		list_p->device,error_str[tnum]);
	 printf("   ROPC Opcounter reads 0x%x. Expected 0x%x\n",
		SC_OpCnt,count);
      } else if ((tcount > 0)&&(SC_OpCnt != (tcount-1))) {
         printf("Device #%d. Error Testing Masks on %s.\n",
		list_p->device,error_str[tnum]);
	 printf("   ROPC Opcounter reads 0x%x. Expected 0x%x\n",
		SC_OpCnt,tcount-1);
      } else if (r1 != r2) {
         list_p->error[tnum] += 1;
         printf("Device #%d. Error Testing Masks on %s.\n",
		list_p->device,error_str[tnum]);
	 printf("   ROPC Func Output Reg = 0x%x. FB reads 0x%x.\n",r1,r2);

      } else if (count==0) {
	 if (r1 != (msk1i & msk2i)) {
            list_p->error[tnum] += 1;
            printf("Device #%d. Error Testing Masks on %s.\n",
		list_p->device,error_str[tnum]);
	    printf("   Count = 0x%x. TCount = 0x%x. Msk1 = 0x%x. Msk2 = 0x%x\n",
		count,tcount,msk1,msk2);
	    printf("   FB Reads 0x%x. Expected 0x%x\n",r1,(ushort)msk1i&msk2i);
	 }
      } else if (tcount == 0) {
	 if (r1 != msk1i) {
            list_p->error[tnum] += 1;
            printf("Device #%d. Error Testing Masks on %s.\n",
		list_p->device,error_str[tnum]);
	    printf("   Count = 0x%x. TCount = 0x%x. Msk1 = 0x%x. Msk2 = 0x%x\n",
		count,tcount,msk1,msk2);
	    printf("   FB Reads 0x%x. Expected 0x%x\n",r1,msk1i);
	 }
	 SC_Msk1 = 0; *(--taddr) = 0xFFFF;	/* Cosmetic Only */
      } else if (tcount == count) {
	 if (r1 != msk2i) {
            list_p->error[tnum] += 1;
            printf("Device #%d. Error Testing Masks on %s.\n",
		list_p->device,error_str[tnum]);
	    printf("   Count = 0x%x. TCount = 0x%x. Msk1 = 0x%x. Msk2 = 0x%x\n",
		count,tcount,msk1,msk2);
	    printf("   FB Reads 0x%x. Expected 0x%x\n",r1,msk2i);
	 }
      } else {
	 if (r1 != ((ushort)0xFFFF)) {
            list_p->error[tnum] += 1;
            printf("Device #%d. Error Testing Masks on %s.\n",
		list_p->device,error_str[tnum]);
	    printf("   Count = 0x%x. TCount = 0x%x. Msk1 = 0x%x. Msk2 = 0x%x\n",
		count,tcount,msk1,msk2);
	    printf("   FB Reads 0x%x. Expected 0xFFFF\n",r1);
	 }
      }
   }
return1: ;
}	/* Routine a_msk() */


auto_msk(list_p,addr,tnum)
   struct bd_list *list_p;
   ushort *addr;			/* Address in plane */
   int tnum;
{
   a_msk(list_p,addr,tnum,0x0000,0xFF0A,0xF584);
   a_msk(list_p,addr,tnum,0x0001,0xFF0A,0xF584);
   a_msk(list_p,addr,tnum,0x0011,0x0ACF,0x5C22);
   a_msk(list_p,addr,tnum,0x0123,0xF002,0x3500);
}

/* Define auto test for a single ROPC */
#define Load_read 1
#define Load_write 0

a_wropc(list_p,waddr,tnum)
   struct bd_list *list_p;
   ushort *waddr;			/* Address in plane */
   int tnum;
{
   ushort *maddr;
   maddr = waddr - 0x100000;		/* Sub 0x200000 from address */

   /* Test out registers */
   auto_rreg(list_p,tnum);

   /* Test function outputs (this includes source extraction) */
   auto_rfun(list_p,waddr,tnum);

   /* Test implicit load of DST register for each addressing mode */
   auto_ldst0(list_p,maddr,tnum);
   ROP_Mode(0); auto_ldst2(list_p,waddr,tnum);
   ROP_Mode(2); auto_ldst4(list_p,waddr,tnum);
   ROP_Mode(4); auto_ldst6(list_p,waddr,tnum);
   ROP_Mode(6); auto_ldst8(list_p,waddr,tnum);

   /* Test implicit load of SRC register for each addressing mode */
   auto_lsrc0(list_p,maddr,tnum);
   ROP_Mode(0); auto_lsrcw(list_p,waddr,tnum);
   ROP_Mode(2); auto_lsrcw(list_p,waddr,tnum);
   ROP_Mode(4); auto_lsrcr(list_p,waddr,tnum);
   ROP_Mode(6); auto_lsrcr(list_p,waddr,tnum);

   /* Test proper decrement and reload of Opcounter. At each step read
      diagnostic function output and write to frame buffer. Verify that
      right and left masks are enabled at the proper time. */
   auto_msk(list_p,waddr,tnum);

}	/* End of a_wropc() */

a_propc(list_p,paddr,tnum)
   struct bd_list *list_p;
   uchar *paddr;			/* Pixel Address */
   int tnum;
{
   register uchar *pmaddr;
   pmaddr = (uchar*)(((int)paddr) - 0x100000);	/* Subtract 0x100000 */

   auto_pldst1(list_p,pmaddr,tnum);
   /* ROP_Mode(1); auto_pldst(list_p,paddr,tnum,Load_read);
    * ROP_Mode(3); auto_pldst(list_p,paddr,tnum,Load_write);
    * ROP_Mode(5); auto_pldst(list_p,paddr,tnum,Load_read);
    * ROP_Mode(7); auto_pldst(list_p,paddr,tnum,Load_write);
    */

   auto_plsrc1(list_p,pmaddr,tnum);
   /* ROP_Mode(1); auto_plsrc(list_p,paddr,tnum,Load_write);
    * ROP_Mode(3); auto_plsrc(list_p,paddr,tnum,Load_write);
    * ROP_Mode(5); auto_plsrc(list_p,paddr,tnum,Load_write);
    * ROP_Mode(7); auto_plsrc(list_p,paddr,tnum,Load_write);
    */
}

extern int foobar;			/* Used by schecker.c */
/* Define automatic test for all ROPC. */
auto_rop(list_p)
   struct bd_list *list_p;
{
   foobar = 0; init_scolor();		/* Init color board */
   printf("0"); Set_Ropc(0); a_wropc(list_p,SC_RMem0,10);
   printf("1"); Set_Ropc(1); a_wropc(list_p,SC_RMem1,11);
   printf("2"); Set_Ropc(2); a_wropc(list_p,SC_RMem2,12);
   printf("3"); Set_Ropc(3); a_wropc(list_p,SC_RMem3,13);
   printf("4"); Set_Ropc(4); a_wropc(list_p,SC_RMem4,14);
   printf("5"); Set_Ropc(5); a_wropc(list_p,SC_RMem5,15);
   printf("6"); Set_Ropc(6); a_wropc(list_p,SC_RMem6,16);
   printf("7"); Set_Ropc(7); a_wropc(list_p,SC_RMem7,17);
   printf("P"); Set_Ropc(8); a_propc(list_p,SC_RPix,20);
}

   
int rop_plane;
manrop(list_p)
   struct bd_list *list_p;
{
   ushort *waddr;
   uchar *paddr;
   short more;
   int i,tnum;
   char ch;

   rop_plane = 7;
   Set_Ropc(rop_plane);
   waddr = 0; tnum = 10;
   more = 1;
   while (more) {
      printf("Manual Tests for RasterOp Chips\n");
      printf("   1: Select Ropc (Default = 7)\n");
      printf("   2: Register Tests\n");
      printf("   3: Auto Register Tests\n");
      printf("   4: Auto Function Unit Tests\n");
      printf("   5: Auto Destination Register Tests\n");
      printf("   6: Auto Source Register Tests\n");
      printf("   7: Auto Mask Tests\n");
      printf("   8: Continuous Auto Tests\n");
      printf("   9: Continuous Auto Function Unit Tests\n");
      printf("   A: Test Per-Plane Mask Register\n");
      printf("   Q: Quit\n");
      printf("Enter Choice: ");
      ch = getchar(); printf("\n");
      if (ch == '1') {
         printf("   Enter ROPC Plane ([0-7] or 8 for All): ");
         rop_plane = getn(); printf("\n");
	 Set_Ropc(rop_plane); tnum = 10 + rop_plane;
      } else if (ch == '2') {
         man_rop_reg(list_p,tnum);
      } else if (ch == '3') {
         auto_rreg(list_p,tnum);
      } else if (ch == '4') {
	printf("Enter Address (0x00000-0x1FFFE): ");
	waddr = SC_RMem0 + 0x10000*rop_plane + ((getnh()&0x1FFFE) >> 1); 
	printf("Halt on errors (y/n)? ");
	ch = getchar(); printf("\n");
	halt_on_err = 0;
	if ((ch == 'y')||(ch == 'Y')) halt_on_err = 1;
	printf("\nTesting Address 0x%x.\n",(int)waddr);
        auto_rfun(list_p,waddr,tnum);
      } else if (ch == '5') {
	printf("Addressing Mode (0-9): ");
	ch  = getchar(); printf("\n");
	if (ch == ' ') {
	   i = 0;
	} else {
	   i = (ch - '0');
	}
	if (i==0) {
	   printf("Enter Address (0x00000-0x1FFFE): ");	
	   waddr = SC_Mem0 + 0x10000*rop_plane;
	   waddr += ((getnh()&0x1FFFE)>>1); 
	   printf("\nTesting Address 0x%x.\n",(int)waddr);
	   printf("\n");
	} else if (i==1) {
	   printf("Enter Address (0x00000-0xFFFFF): ");
	   paddr = SC_Pix + (getnh()&0xFFFFF);
	   printf("\nTesting Address 0x%x.\n",(int)paddr);
	   printf("\n");
	} else if ((i & 0x1)==0) {
	   printf("Enter Address (0x00000-0x1FFFE): ");
	   waddr = SC_RMem0 + 0x10000*rop_plane;
	   waddr += ((getnh()&0x1FFFE)>>1);
	   printf("\nTesting Address 0x%x.\n",(int)waddr);
	   printf("\n");
	} else {
	   printf("Enter Address (0x00000-0xFFFFF): ");
	   paddr = SC_RPix + (getnh()&0xFFFFF);
	   printf("\nTesting Address 0x%x.\n",(int)paddr);
	   printf("\n");
	}

	if (i==0) {
   	   auto_ldst0(list_p,waddr,tnum);
	} else if (i==1) {
   	   auto_pldst1(list_p,paddr,tnum);
	} else {
	   ROP_Mode((i-2));
	   if (i==2) {
   	      auto_ldst2(list_p,waddr,tnum);
	   } else if (i==4) {
   	      auto_ldst4(list_p,waddr,tnum);
	   } else if (i==6) {
   	      auto_ldst6(list_p,waddr,tnum);
	   } else if (i==8) {
   	      auto_ldst8(list_p,waddr,tnum);
	   } else {
	      printf("Routine Not Implemented.\n");
	   } 
	}

      } else if (ch == '6') {
	printf("Addressing Mode (0-9): ");
	ch  = getchar(); printf("\n");
	i = (ch - '0');
	if (i==0) {
	   printf("Enter Address (0x00000-0x1FFFE): ");
	   waddr = SC_Mem0 + 0x10000*rop_plane;
	   waddr += ((getnh()&0x1FFFE)>>1);
	   printf("\nTesting Address 0x%x.\n",(int)waddr);
	   printf("\n");
	} else if (i==1) {
	   printf("Enter Address (0x00000-0xFFFFF): ");
	   paddr = SC_Pix + (getnh()&0xFFFFF);
	   printf("\nTesting Address 0x%x.\n",(int)paddr);
	   printf("\n");
	} else if ((i & 0x1)==0) {
	   printf("Enter Address (0x00000-0x1FFFE): ");
	   waddr = SC_RMem0 + 0x10000*rop_plane;
	   waddr += ((getnh()&0x1FFFE)>>1);
	   printf("\nTesting Address 0x%x.\n",(int)waddr);
	   printf("\n");
	} else {
	   printf("Enter Address (0x00000-0xFFFFF): ");
	   paddr = SC_RPix + (getnh()&0xFFFFF);
	   printf("\nTesting Address 0x%x.\n",(int)paddr);
	   printf("\n");
	}
	printf("Halt on errors (y/n)? ");
	ch = getchar(); printf("\n");
	halt_on_err = 0;
	if ((ch == 'y')||(ch == 'Y')) halt_on_err = 1;

	if (i==0) {
   	   auto_lsrc0(list_p,waddr,tnum);
	} else if (i==1) {
   	   auto_plsrc1(list_p,paddr,tnum);
	} else {
	   ROP_Mode((i-2));
	   if ((i==2)||(i==4)) {
   	      auto_lsrcw(list_p,waddr,tnum);
	   } else if ((i==6)||(i==8)) {
   	      auto_lsrcr(list_p,waddr,tnum);
	   } else if ((i==3)||(i==5)||(i==7)||(i==9)) {
	      printf("Routine Not Implemented.\n");
   	      /* auto_plsrc(list_p,paddr,tnum,Load_write); */
	   }
	}
      } else if (ch == '7') {
	printf("Enter Address Offset (0x00000-0x1FFFE): ");
	waddr = SC_RMem0 + 0x10000*rop_plane + ((getnh()&0x1FFFE)>>1); 
	printf("\nTesting Address 0x%x.\n",(int)waddr);
	printf("\n");
   	auto_msk(list_p,waddr,tnum);
      } else if (ch == '8') {
	i = 0;
	printf("Halt on errors (y/n)? ");
	ch = getchar(); printf("\n");
	halt_on_err = 0;
	if ((ch == 'y')||(ch == 'Y')) halt_on_err = 1;
	while (1) {
	   printf("Continuous Rasterop Tests. Pass #%d. Testing Plane ",i);
	   auto_rop(list_p);
	   i += 1;
	   printf("\n");
	}
      } else if (ch == '9') {
	printf("Enter Address (0x00000-0x1FFFE): ");
	waddr = SC_RMem0 + 0x10000*rop_plane + ((getnh()&0x1FFFE) >> 1); 
	printf("Halt on errors (y/n)? ");
	ch = getchar(); printf("\n");
	halt_on_err = 0;
	if ((ch == 'y')||(ch == 'Y')) halt_on_err = 1;
	printf("\nTesting Address 0x%x.\n",(int)waddr);
        while (1) auto_rfun(list_p,waddr,tnum);
      } else if ((ch == 'a')||(ch == 'A')) {
	printf("Testing Per-Plane Masking\n");
	pix_plmask(list_p);
	printf("Testing Per-Plane Loading of ROPC\n");
	word_plmask(list_p);
      } else if ((ch == 'q')||(ch == 'Q')) {
	 more = 0;
      }
   }
}		/* End of manrop() */

