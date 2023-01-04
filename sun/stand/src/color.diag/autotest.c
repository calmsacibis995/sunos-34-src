
static char	sccsid[] = "@(#)autotest.c 1.1 9/25/86 Copyright SMI";


/* ======================================================================
   Author: Peter Costello
   Date :  October 21, 1982
   Purpose: Do some of the auto tests. 
   Algorithm:
   Error Handling:
   ====================================================================== */

#include "m68000.h"
#include "vectors.h"
#include "reentrant.h"
#include "cdiag.h"

/* Test out the status register. Assume that the interrupt level is NOT 
   set to level 7. */

/* Define interrupt routine which handles a spurious interrupt caused by 
   writting random data to the status register. */
reentrant(i_ret)
{
   uchar data;

   /* printf("Servicing Interrupt.\n"); */
   data = *GR_sreg;
   *GR_sreg = 0;	/* Toggle off interrupt enable bit */
   *GR_sreg = data;	/* Restore value */
}

tstat_reg(list_p,tnum)
   struct bd_list *list_p;
   int tnum;
{
   uchar data,data1;
   int vector[7];    		/* Saved vectors */
   int i_ret();

   /* Set up the interrupt handler. It just returns. */
   vector[1] = IRQ1Vect; IRQ1Vect = (int)i_ret;
   vector[2] = IRQ2Vect; IRQ2Vect = (int)i_ret;
   vector[3] = IRQ3Vect; IRQ3Vect = (int)i_ret;
   vector[4] = IRQ4Vect; IRQ4Vect = (int)i_ret;
   /* vector[5] = IRQ5Vect; IRQ5Vect = (int)i_ret; */
   /* vector[6] = IRQ6Vect; IRQ6Vect = (int)i_ret; */

   for (data1=127;data1>0;data1--) {
      *GR_sreg = data1;
      data = (*GR_sreg & 0x7F);		/* Get rid of MSB. */
      if (data != data1) {
	 /* Error in status reg. */
         printf("Device #%d @ 0x%x. Wrote 0x%x to Status Reg. Read 0x%x.\n",
		list_p->device,list_p->base,data1,data);
	 list_p->error[tnum] += 1;
      }
   }
   *GR_sreg = GR_disp_on;	/* Turn on display, and thats it */

   /* Restore interrupt handlers. */
   IRQ1Vect = vector[1];
   IRQ2Vect = vector[2];
   IRQ3Vect = vector[3];
   IRQ4Vect = vector[4];
   /* IRQ5Vect = vector[5]; */
   /* IRQ6Vect = vector[6]; */

}


short int_ok;
/* Test out interrupt logic */
reentrant(tstint)
{
   *GR_sreg = GR_disp_on;	/* Turn off interrupts */
   /* printf("Servicing Interrupt.\n"); */
   int_ok = TRUE;
}

tinterrupt(list_p,tnum)
   struct bd_list *list_p;
   int tnum;
{
   int vector[7];    		/* Saved vectors */
   int tstint();

   /* Set up the interrupt handler. */
   vector[1] = IRQ1Vect; IRQ1Vect = (int)tstint;
   vector[2] = IRQ2Vect; IRQ2Vect = (int)tstint;
   vector[3] = IRQ3Vect; IRQ3Vect = (int)tstint;
   vector[4] = IRQ4Vect; IRQ4Vect = (int)tstint;
   /* vector[5] = IRQ5Vect; IRQ5Vect = (int)tstint; */
   /* vector[6] = IRQ6Vect; IRQ6Vect = (int)tstint; */
   intlevel(1);			/* Enable interrupts */

   int_ok = FALSE;		/* Assume bad */

   while (! GR_retrace);	/* Wait for next vretrace. */
   while (GR_retrace);		/* Start of retrace. */
   *GR_sreg |= GR_inten;	/* Enable interrupts */
   while (!GR_retrace);		/* Wait for next vretrace and interrupt. */
   while (GR_retrace);		/* Give interrupt 1 msec to be handled. */

   if (! int_ok) {
      printf("Device #%d @ 0x%x. No interrupt received when expected.\n",
	      list_p->device,list_p->base);
      list_p->error[tnum] += 1;
   }

   *GR_sreg = GR_disp_on;	/* Turn on display, and thats it */

   /* Restore interrupt handlers. */
   IRQ1Vect = vector[1];
   IRQ2Vect = vector[2];
   IRQ3Vect = vector[3];
   IRQ4Vect = vector[4];
   /* IRQ5Vect = vector[5]; */
   /* IRQ6Vect = vector[6]; */

}


/* This routine tests out the function register */
tfunc_reg(list_p,tnum)
   struct bd_list *list_p;
   int tnum;
{
   uchar data,data1;

   for (data1=255;data1>0;data1--) {
      Set_CFunc(data1);		/* Write Function reg */
      Read_CFunc(data);		/* Read Function reg */
      if (data != data1) {
	 /* Error in function reg. */
         printf("Device #%d @ 0x%x. Wrote 0x%x to Function Reg. Read 0x%x.\n",
		list_p->device,list_p->base,data1,data);
	 list_p->error[tnum] += 1;
      }
   }
}



/* This routine tests out the mask register */
tmask_reg(list_p,tnum)
   struct bd_list *list_p;
   int tnum;
{
   uchar data,data1;

   for (data1=255;data1>0;data1--) {
      Set_CMask(data1);
      Read_CMask(data);
      if (data != data1) {
	 /* Error in mask reg. */
         printf("Device #%d @ 0x%x. Wrote 0x%x to Mask Reg. Read 0x%x.\n",
		list_p->device,list_p->base,data1,data);
	 list_p->error[tnum] += 1;
      }
   }
}

   
/* This routine tests out the address registers. */
taddress(list_p,tnum)
   struct bd_list *list_p;
   int tnum;
{
   txaddr(list_p,tnum,GR_set0);	/* Bits A9-A0 used. */
   txaddr(list_p,tnum,GR_set1);
   tyaddr(list_p,tnum,GR_set0);	/* Bits A8-A0 used. */
   tyaddr(list_p,tnum,GR_set1);
}

txaddr(list_p,tnum,cset)
   struct bd_list *list_p;
   int tnum,cset;
{
   short addr,taddr;
   uchar *xwaddr,*xrhaddr,*xrladdr;

   xwaddr = (uchar*)(GR_bd_sel + GR_x_select + cset);
   xrhaddr = (uchar*)(GR_bd_sel + GR_x_rhaddr + cset);
   xrladdr = (uchar*)(GR_bd_sel + GR_x_rladdr + cset);
   for (addr = 0;addr < 1024;addr++) {
      *xwaddr++ = TOUCH;		/* Set x address */
      taddr = (short)((*xrhaddr & 0x03) << 8);
      taddr |= (short)((*xrladdr & 0xFF));
      if (taddr != addr) {
         printf("Device #%d @ 0x%x. Wrote X-Address 0x%x. Set %d. Read 0x%x.\n",
                 list_p->device,list_p->base,addr,(cset != 0),taddr);
         list_p->error[tnum] += 1;
      }
   }
}


tyaddr(list_p,tnum,cset)
   struct bd_list *list_p;
   int tnum,cset;
{
   short addr,taddr;
   uchar *ywaddr,*yrhaddr,*yrladdr;

   ywaddr = (uchar*)(GR_bd_sel + GR_y_select + cset);
   yrhaddr = (uchar*)(GR_bd_sel + GR_y_rhaddr + cset);
   yrladdr = (uchar*)(GR_bd_sel + GR_y_rladdr + cset);
   for (addr = 0;addr < 512;addr++) {
      *ywaddr++ = TOUCH;		/* Set y address */
      taddr = (short)((*yrhaddr & 0x01) << 8);
      taddr |= (short)((*yrladdr & 0xFF));
      if (taddr != addr) {
         printf("Device #%d @ 0x%x. Wrote X-Address 0x%x. Set %d. Read 0x%x.\n",
                 list_p->device,list_p->base,addr,(cset != 0),taddr);
         list_p->error[tnum] += 1;
      }
   }
}


/* Test out 5-pixel wide updates. Assume that memory has already been tested.
   No sense in confusing people if our memory is bad.
   We test out paint mode for all possible beginning x-location (0 to 639). */
tpaint(list_p,tnum)
   struct bd_list *list_p;
   int tnum;
{
   paintit(list_p,tnum,(uchar)0xA5,(uchar)0xC3);
   paintit(list_p,tnum,(uchar)0x00,(uchar)0xFF);
}

paintit(list_p,tnum,bdata,fdata)
   struct bd_list *list_p;
   int tnum;
   uchar bdata,fdata;
{
   register uchar *xaddr,*yaddr,*x5addr,*taddr;
   register uchar data;
   register short i,x,x5;

   Set_CFunc(GR_copy);			/* Copy data unaltered. */
   yaddr = (uchar*)(GR_bd_sel|GR_y_select|GR_set0|0);
   xaddr = (uchar*)(GR_bd_sel|GR_x_select|GR_update|GR_set0|0);
   x5addr = (uchar*)(GR_bd_sel|GR_x_select|GR_update|GR_set0|0);

   *yaddr = TOUCH;			/* Don't bother with vertical addr */
   for (x5=0;x5<128;x5++) {
      for (x=0;x<5;x++) {
         *GR_sreg = GR_disp_on;		/* No paint mode */
         taddr = x5addr;		/* Point to first pixel - MOD 5. */
	 /* printf("DEBUG: taddr = 0x%x. Writing next 5 to background.\n",
		taddr); */
         *taddr++ = bdata;		/* Clearing to background color */
         *taddr++ = bdata;		/* Clearing to background color */
         *taddr++ = bdata;		/* Clearing to background color */
         *taddr++ = bdata;		/* Clearing to background color */
         *taddr++ = bdata;		/* Clearing to background color */

         *GR_sreg |= GR_paint;		/* Turn on paint mode */
	 /* printf("DEBUG: Turn on paint mode. Sreg = 0x%x.\n",*GR_sreg); */
	 /* printf("DEBUG: Write paint @ addr 0x%x.\n",xaddr); */
         *xaddr++ = fdata;		/* Write same five pixels. */
      
         taddr = x5addr;
         data  = *taddr++;		/* Empty pipeline */
         for (i=0;i<5;i++) {		/* Now verify */
	    /* printf("DEBUG. Verify addr 0x%x.\n",taddr); */
            data = *taddr++;
            if (data != fdata) {	/* Error */
               printf("Device #%d @ 0x%x. ",list_p->device,list_p->base);
               printf("Paint-Mode Error. Y = 0. Wrote X = 0x%x to 0x%x with ",
		   (x5*5),(x5*5+5));
               printf("0x%x.\n",fdata);
               printf("     Read 0x%x at X = 0x%x.\n",data,(x5*5+i));
	       printf("     Wrote paint-mode pixel at Xaddr = 0x%x.\n",xaddr);
               list_p->error[tnum] += 1;
	    }
         }
      }
      x5addr += 5;			/* Advance 5-pixels. */
   }

   *GR_sreg = GR_disp_on;		/* Turn off paint mode. */

}		/* End of test of paint-mode logic. */
	 
	 
   
