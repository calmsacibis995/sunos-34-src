
/* ======================================================================
   Author: Peter Costello
   Date :  April 15, 1983
   Purpose: Perform an auto test on the Sun-2 frame buffer memory.
   Algorithm: Constant data test, address test, checkerboard test,
	random data test.
   Timing:
   Error Handling:
   Bugs:
   ====================================================================== */
static char     sccsid[] = "@(#)automem.c 1.1 9/25/86 Copyright Sun Micro";

#include "sc.diag.h"

testmem(base,top)
	uint *base,*top;
{
        register int curerrs;
	Enable_All_Planes;

	/* printf("Base = 0x%x. Top = 0x%x.\n",((int)base),((int)top) ); */
        curerrs=0;
        printf("1");
        curerrs += testn(base,top,(uint)0x0);
	printf("2");
        curerrs += testn(base,top,(uint)0xFFFF0000);
	printf("3");
        curerrs += testn(base,top,(uint)0xFFFFFFFF);
	printf("4");
        curerrs += testn(base,top,(uint)0xAA55CC33);
	printf("5");
        curerrs += testn(base,top,(uint)0x00FF3355);
	printf("6");
        curerrs += testn(base,top,(uint)0xAA33CCEF);
	printf("7");
	curerrs += testadr(base,top);
	printf("8");
        curerrs += testrand(base,top);
	printf("9");
        curerrs += checkerfast(base,top,(uint)0xA53C5AC3); 
	printf("A");
        curerrs += checkerfastb(((uchar*)base),((uchar*)top),(uchar)0x0); 

        if (curerrs) {
           printf(". %d Errors.\n",curerrs);
        } else {
           printf(". No errors.\n");
        }
	return(curerrs);
}

auto_mem(list_p)
   struct bd_list *list_p;
{
	Enable_All_Planes;
   	printf("Word  Mode MemTests. Device #%d. Test #",list_p->device);
	list_p->error[10] += testmem(SC_Meml0,SC_Meml8);

   	printf("Pixel Mode MemTests. Device #%d. Test #",list_p->device);
	list_p->error[11] += testmem(SC_Pixl,SC_Pixlt);
}
   
