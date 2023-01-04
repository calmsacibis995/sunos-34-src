
/* ======================================================================
   Author: Peter Costello
   Date :  April 15, 1983
   Purpose: Draw a 16 x 16 grid on the frame buffer. Each square 72 
	pixels on a side. Each box numbered from zero moving left to 
	right, top to bottom.
   Algorithm:
   Timing:
   Error Handling:
   Bugs:
   ====================================================================== */
static char     sccsid[] = "@(#)schecker.c 1.1 9/25/86 Copyright Sun Micro";

#include "sc.diag.h"

int foobar = 0;

/* Make this fast */
schecker()
{
   register uint *iaddr,icolor;
   register uchar color;
   register short tblk,trow,rblk,rhi;

   rhi = 56;
   if (SCWidth==1024) rhi = 64;

   /* init_scolor();				/* Load default cmap */
   color = 0;
   iaddr = SC_Pixl;
   for (rblk=16;rblk>0;rblk-=1) {
      for (trow=rhi;trow>0;trow-=1) {
	 for (tblk=16;tblk>0;tblk-=1) {
	    icolor = (uint)(color<<24)|(color<<16)|(color<<8)|(color);
	    *iaddr++ = icolor; *iaddr++ = icolor; 	/*  8 pixels */
	    *iaddr++ = icolor; *iaddr++ = icolor;	/* 16 pixels */
	    *iaddr++ = icolor; *iaddr++ = icolor; 	/* 24 pixels */
	    *iaddr++ = icolor; *iaddr++ = icolor;	/* 32 pixels */
	    *iaddr++ = icolor; *iaddr++ = icolor;	/* 40 pixels */
	    *iaddr++ = icolor; *iaddr++ = icolor;	/* 48 pixels */
	    *iaddr++ = icolor; *iaddr++ = icolor;	/* 56 pixels */
	    *iaddr++ = icolor; *iaddr++ = icolor; 	/* 64 pixels */
	    if (SCWidth != 1024) {
	       *iaddr++ = icolor; *iaddr++ = icolor;	/* 72 pixels */
	    }
	    color += 1;
	 }
         color -= 16;
      }
      color += 16;
/*     if (foobar) color = 0;*/
   }
   foobar = (foobar+1)&0x1;

   for ( ;iaddr<SC_Pixlt;) *iaddr++ = 0; 	/* Zero last 13 lines */

}		/* End of schecker() */

schecker_verify(list_p)
   struct bd_list *list_p;
{
   register uchar *addr;
   register uchar data,color;
   register short trow,rblk,tcol,tblk,rhi,cwi;

   rhi = 56; if (SCWidth==1024) rhi = 64;
   cwi = 72; if (SCWidth==1024) cwi = 64;
   color = 0;
   addr = SC_Pix;
   for (rblk=16;rblk>0;rblk--) {
      for (trow=rhi;trow>0;trow--) {
	 for (tblk=16;tblk>0;tblk--) {
	    for (tcol=cwi;tcol>0;tcol--) {
		data = *addr++;
		if (data != color) {
		   printf("Device #%d. Checker Verify.",list_p->device);
                   printf(">>> %x %x %x %x <<<",rblk,trow,tblk,tcol);
		   printf(" Pix Addr 0x%x. Wr 0x%x. Rd 0x%x.\n",
			   ((int)addr)-SCBase-2,color,data);
		}
	    }
	    color += 1;
	 }
         color -= 16;
      }
      color += 16;
   }
}		/* End of schecker_verify() */

