
static char	sccsid[] = "@(#)testmem.c 1.1 9/25/86 Copyright SMI";



/* ======================================================================
   Author: Peter Costello
   Date :  July 12, 1982
   Purpose:  Test out frame buffer memory. 
   Algorithm:
   Error Handling:
  ====================================================================== */

#include "cdiag.h"

imm_test(list_p,tnum,data0)
	struct bd_list *list_p;
	int tnum;
	uchar data0;
{
	register uchar *xloc,*yloc,*top_xloc,*top_yloc;
	register uchar data,temp;

	data = data0;
 	top_xloc = (uchar*)(GR_bd_sel|GR_update|GR_x_select|GR_set0|640);
 	top_yloc = (uchar*)(GR_bd_sel|GR_y_select|GR_set0|512);

	/* Write Data and read immediately. */
	yloc = (uchar*)(GR_bd_sel|GR_y_select|GR_set0|0);
	do {
	   *yloc++ = TOUCH;
	   xloc = (uchar*)(GR_bd_sel|GR_x_select|GR_set0|GR_update|0);
	   do {
	      *xloc = data;
	      temp = *xloc;
	      temp = *xloc++;
	      if (temp != data) {
		 xloc++;	/* increment. Because report subs 2 from x */
	         report("Immediate Data",list_p,tnum,xloc,yloc,data,temp);
		 xloc--;	/* Restore X */
	      }
 	   } while (xloc < top_xloc);
	} while (yloc < top_yloc);

}	/* End of Immediate data test */

const_test(list_p,tnum,data0)
	struct bd_list *list_p;
	int tnum;
	uchar data0;
{
	register uchar *xloc,*yloc,*top_xloc,*top_yloc;
	register uchar data,temp;

	data = data0;
 	top_xloc = (uchar*)(GR_bd_sel|GR_update|GR_x_select|GR_set0|640);
 	top_yloc = (uchar*)(GR_bd_sel|GR_y_select|GR_set0|512);

	/* Write Data */
	yloc = (uchar*)(GR_bd_sel|GR_y_select|GR_set0|0);
	do {
	   *yloc++ = TOUCH;
	   xloc = (uchar*)(GR_bd_sel|GR_x_select|GR_set0|GR_update|0);
	   do {
	      *xloc++ = data;
 	   } while (xloc < top_xloc);
	} while (yloc < top_yloc);

	/* Verify Data */
	yloc = (uchar*)(GR_bd_sel|GR_y_select|GR_set0|0);
	do {
	   *yloc++ = TOUCH;	/* Set y read address */
	   xloc = (uchar*)(GR_bd_sel|GR_x_select|GR_set0|GR_update|0);
	   temp = *xloc++;	/* Prefetch */
	   do {
	      temp = *xloc++; 
	      if (temp != data) {
	         report("Constant Data",list_p,tnum,xloc,yloc,data,temp);
	      }
 	   } while (xloc < top_xloc);
	} while (yloc < top_yloc);

}	/* End of constant data test */


addr_test(list_p,tnum)
	struct bd_list *list_p;
	int tnum;
{
 	register uchar *xloc,*yloc,*top_xloc,*top_yloc;
	register uchar data,temp;

 	top_xloc = (uchar*)(GR_bd_sel|GR_update|GR_x_select|GR_set0|640);
 	top_yloc = (uchar*)(GR_bd_sel|GR_y_select|GR_set0|512);

	/* Write Data */
	yloc = (uchar*)(GR_bd_sel|GR_y_select|GR_set0|0);
	do {
	   *yloc++ = TOUCH;
	   data = 0;
	   xloc = (uchar*)(GR_bd_sel|GR_x_select|GR_update|GR_set0|0);
	   do {
	      *xloc++ = data++;
 	   } while (xloc < top_xloc);
	} while (yloc < top_yloc);

	/* Verify Data */
	yloc = (uchar*)(GR_bd_sel|GR_y_select|GR_set0|0);
	do {
	   *yloc++ = TOUCH;	/* Set y read address */
	   data = 0;
	   xloc = (uchar*)(GR_bd_sel|GR_x_select|GR_update|GR_set0|0);
	   temp = *xloc++;	/* Prefetch */
	   do {
	      temp = *xloc++; 
	      if (temp != data) {
	         report("Address",list_p,tnum,xloc,yloc,data,temp);
	      }
	      data++;
 	   } while (xloc < top_xloc);
	} while (yloc < top_yloc);

}	/* End of addr_test */


#define FB_Mem_Size ((int)(5*(64*1024))) /* Size of frame buffer in bytes */

chk_test(list_p,tnum,data0)
	struct bd_list *list_p;
	int tnum;
   	uchar data0;
{
 	register uchar *xloc,*yloc,*top_xloc,*top_yloc;
	register uchar data,temp;
	int i,addr_incr;

	data = data0;
	addr_incr = 0x1;      /* Start by writing every other location with
			       complementary data. Then write every two
			       locations with complementary data, etc. */

 	top_xloc = (uchar*)(GR_bd_sel|GR_update|GR_x_select|GR_set0|640);
 	top_yloc = (uchar*)(GR_bd_sel|GR_y_select|GR_set0|512);

	do {		    /* For all possible address increments */
	   /* printf("DEBUG: Addr_Incr = 0x%x.\n",addr_incr); */

	   yloc = (uchar*)(GR_bd_sel|GR_y_select|GR_set0|0);
	   *yloc++ = TOUCH;
	   xloc = (uchar*)(GR_bd_sel|GR_update|GR_x_select|GR_set0|0);

	   do {   	    /* For all possible Y locations */
	      data = (~data);   /* Invert */ 
	      /* Write N values of Data0 (where N = addr_incr) */
	      i = 0;
	      do {
		 *xloc++ = data;
		 if (xloc >= top_xloc) {
	            xloc = (uchar*)(GR_bd_sel|GR_update|GR_x_select|GR_set0|0);
		    *yloc++ = TOUCH;
		 }
	         i += 1;
	      } while ((i < addr_incr)&&(yloc <= top_yloc));
	      /* printf("DEBUG: Wrote 0x%x %d times.\n",data,i); */
		 
	      
	      /* Write N values of ~Data0 (where N = addr_incr) */
	      data = (~data);
	      i = 0;
	      if (yloc <= top_yloc) {
	         do {
		    *xloc++ = data;
		    if (xloc >= top_xloc) {
	               xloc = (uchar*)
			      (GR_bd_sel|GR_update|GR_x_select|GR_set0|0);
		       *yloc++ = TOUCH;
		    }
		    i += 1;
	         } while ((i < addr_incr)&&(yloc <= top_yloc));
	      }
	      /* printf("DEBUG: Wrote 0x%x %d times.\n",data,i); */

	   } while (yloc <= top_yloc); 

	      
	   /* Now read back the checkerboard we just wrote. */

	   yloc = (uchar*)(GR_bd_sel|GR_y_select|GR_set0|0);
	   *yloc++ = TOUCH;		/* Set y read address */
	   xloc = (uchar*)(GR_bd_sel|GR_update|GR_x_select|GR_set0|0);

	   temp = *xloc++; 	/* Prefetch */
	   do {   	    /* For all possible Y locations */
	      data = (~data);   /* Invert */ 
	      /* Read N values of Data0 (where N = addr_incr) */
	      i = 0;
	      do {
		 temp = *xloc++;
	         if (temp != data) {
		    report("Checker",list_p,tnum,xloc,yloc,data,temp);
		 }
		 if (xloc > top_xloc) {
	            xloc = (uchar*)(GR_bd_sel|GR_update|GR_x_select|GR_set0|0);
	            *yloc++ = TOUCH;	/* Set y read address */
		    temp = *xloc++;	/* Prefetch */
		 }
		 i += 1;
	      } while ((i < addr_incr)&&(yloc <= top_yloc));
		 
	      
	      /* Read N values of ~Data0 (where N = addr_incr) */
	      data = (~data);
	      if (yloc <= top_yloc) {
	         i = 0;
	         do {
		    temp = *xloc++;
	            if (temp != data) {
		       report("Checker",list_p,tnum,xloc,yloc,data,temp);
		    }
		    if (xloc > top_xloc) {
	               xloc = (uchar*)
			      (GR_bd_sel|GR_update|GR_x_select|GR_set0|0);
	   	       *yloc++ = TOUCH;		/* Set y read address */
		       temp = *xloc++;	/* Prefetch */
		    }
		    i += 1;
	         } while ((i < addr_incr)&&(yloc <= top_yloc));
   	      }

	   } while (yloc <= top_yloc);

           addr_incr <<= 1;   			/* Multiply by 2 */ 
	
	} while (addr_incr < FB_Mem_Size); 

}	/* End of checker test */


#define MOD %			/* Modulus operator */
report(name,list_p,tnum,xloc,yloc,wrote,read)
	char *name;
	struct bd_list *list_p;
	int tnum;
   	uchar *xloc,*yloc;
	uchar wrote,read;
{
	short register x,y,i;
	uchar bbits;

	x = (short)(((long)xloc) & 0x3FF) - 2;
	y = (short)(((long)yloc) & 0x1FF) - 1;

        printf("Device #%d @ 0x%x. %s Test. X = %d. Y = %d.\n",
	        list_p->device,list_p->base,name,x,y);
	printf("	Wr: 0x%x. Rd: 0x%x.",
		wrote,read);
	/* Now compute bad chip location */
	x = (x MOD 5);			/* Bank 0 to 4 */
	x += 1;				/* Row location */
	bbits = (wrote ^ read);		/* Bad bits */
	for (i=0;i<8;i++) {
	   if (bbits & (1<<i)) printf(" M%d0%d.",x,i);
	}
	printf("\n");
	list_p->error[tnum] += 1;
}



/* This routine calls the other routines */
testmem(list_p,tnum)
	struct bd_list *list_p;
	int tnum;
{
	int wrcmap();
	short i;

        write_cmap(wrcmap,gr_red_c0,gr_grn_c0,gr_blu_c0,0);
   	Set_Video_Cmap(0);
 	Set_CFunc(GR_copy);		/* Set function reg to copy. */
	*GR_sreg = GR_disp_on;		/* Set status reg to some ok value */

	/* imm_test(list_p,tnum,((uchar)0x33) ); */
	/* imm_test(list_p,tnum,((uchar)0xFF) ); */
	/* imm_test(list_p,tnum,((uchar)0x0F) ); */
	/* imm_test(list_p,tnum,((uchar)0xAA) ); */

	const_test(list_p,tnum,((uchar)0x33) );
	const_test(list_p,tnum,((uchar)0x55) );
	const_test(list_p,tnum,((uchar)0xFF) );
	const_test(list_p,tnum,((uchar)0x00) );
	const_test(list_p,tnum,((uchar)0xAA) );
	const_test(list_p,tnum,((uchar)0xBB) );
	const_test(list_p,tnum,((uchar)0xCC) );

	addr_test(list_p,tnum);

        /* Set color map to 1/2 red, 1/2 blue for checker test */
	for (i=0;i<128;i++) {
	   gr1_red_c1[i] = 255;
	   gr1_grn_c1[i] = 0;
	   gr1_blu_c1[i] = 0;
	}
	for (i=128;i<256;i++) {
	   gr1_red_c1[i] = 0;
	   gr1_grn_c1[i] = 0;
	   gr1_blu_c1[i] = 255;
	}
        write_cmap(wrcmap,gr1_red_c1,gr1_grn_c1,gr1_blu_c1,0);

	/* Do the checker test */
	chk_test(list_p,tnum,((uchar)0xA3) );

}	/* End of testing of memory */
