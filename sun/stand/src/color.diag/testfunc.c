
static char	sccsid[] = "@(#)testfunc.c 1.1 9/25/86 Copyright SMI";


/* ======================================================================
   Author: Peter Costello
   Date :  July 12, 1982
   Purpose: Test out function unit of color board.
   Algorithm:
   Error Handling:
   ====================================================================== */

#include "cdiag.h"

testfunc(list_p,tnum) 
   struct bd_list *list_p;
   int tnum;
{
   testf(list_p,tnum,((uchar)0xFF));
   testf(list_p,tnum,((uchar)0x00));
   testf(list_p,tnum,((uchar)0xAA));
   testf(list_p,tnum,((uchar)0x55));
   testf(list_p,tnum,((uchar)0xCC));
   testf(list_p,tnum,((uchar)0x33));
   testf(list_p,tnum,((uchar)0x11));
   testf(list_p,tnum,((uchar)0xEE));
}

testf(list_p,tnum,data)
   struct bd_list *list_p;
   int tnum;
   uchar data;
{
   uchar register *xloc,*yloc;
   uchar temp;

   yloc = (uchar*)(GR_bd_sel | GR_y_select | GR_set0 | 0);
   xloc = (uchar*)(GR_bd_sel | GR_x_select | GR_update | GR_set0 | 0);

   *yloc = TOUCH;	/* Set y address */

   Set_CMask((uchar)0x00);		/* Use low half of function Register */
   Set_CFunc((uchar)0x3C);
   *xloc = data;
   temp = *xloc;	/* Empty pipeline */
   temp = *xloc;
   if (temp != data) {
      func_err(list_p,tnum,data,temp,"Source Data");
   }
   
   Set_CFunc((uchar)0xF0);
   *xloc = data;
   temp = *xloc;	/* Empty pipeline */
   temp = *xloc;
   if (temp != 0) {
      func_err(list_p,tnum,data,temp,"Zeros");
   }
   
   Set_CFunc((uchar)0x0F);
   *xloc = data;
   temp = *xloc;	/* Empty pipeline */
   temp = *xloc;
   if (temp != ((uchar)0xFF) ) {
      func_err(list_p,tnum,data,temp,"Ones");
   }
   
   Set_CFunc((uchar)0xC3);
   *xloc = data;
   temp = *xloc;	/* Empty pipeline */
   temp = *xloc;
   if (temp != (~data) ) {
      func_err(list_p,tnum,data,temp,"Inverted Source Data");
   }
   
   Set_CFunc((uchar)0x0C);
   *xloc = data;
   Set_CFunc((uchar)0x05);
   *xloc = data;
   temp = *xloc;	/* Empty pipeline */
   temp = *xloc;
   if (temp != (~data)) {
      func_err(list_p,tnum,data,temp,"Old Data Inverted");
   }

   Set_CMask((uchar)0xFF);	 /* Use high half of function Register */
   Set_CFunc((uchar)0xC3);
   *xloc = data;
   temp = *xloc;	/* Empty pipeline */
   temp = *xloc;
   if (temp != data) {
      func_err(list_p,tnum,data,temp,"Source Data");
   }
   
   Set_CFunc((uchar)0x0F);
   *xloc = data;
   temp = *xloc;	/* Empty pipeline */
   temp = *xloc;
   if (temp != 0) {
      func_err(list_p,tnum,data,temp,"Zeros");
   }
   
   Set_CFunc((uchar)0xF0);
   *xloc = data;
   temp = *xloc;	/* Empty pipeline */
   temp = *xloc;
   if (temp != ((uchar)0xFF) ) {
      func_err(list_p,tnum,data,temp,"Ones");
   }
   
   Set_CFunc((uchar)0x3C);
   *xloc = data;
   temp = *xloc;	/* Empty pipeline */
   temp = *xloc;
   if (temp != (~data) ) {
      func_err(list_p,tnum,data,temp,"Inverted Source Data");
   }
   
   Set_CMask((uchar)0xFF);	 /* Use high half of function Register */
   Set_CFunc((uchar)0xC3);
   *xloc = data;
   Set_CFunc((uchar)0x50);
   *xloc = data;
   temp = *xloc;	/* Empty pipeline */
   temp = *xloc;
   if (temp != (~data)) {
      func_err(list_p,tnum,data,temp,"Old Data Inverted");
   }
   
   Set_CFunc((uchar)0xF0);
   Set_CMask(data);
   *xloc = data;
   temp = *xloc;	/* Empty pipeline */
   temp = *xloc;
   if (temp != data) {
      func_err(list_p,tnum,data,temp,"Mask");
   }

   Set_CFunc((uchar)0x0F);
   Set_CMask(data);
   *xloc = data;
   temp = *xloc;	/* Empty pipeline */
   temp = *xloc;
   if (temp != (~data)) {
      func_err(list_p,tnum,data,temp,"Inverted Mask");
   }

}	/* End of function unit test */

func_err(list_p,tnum,wrote,read,str)
   struct bd_list *list_p;
   int tnum;
   uchar wrote,read;
   char *str;
{

   printf("Device #%d @ 0x%x. Function Unit. Function write %s.\n",
           list_p->device,list_p->base);
   printf("                   Wrote 0x%x. Read 0x%x.\n",wrote,read);
   list_p->error[tnum] += 1;

}


