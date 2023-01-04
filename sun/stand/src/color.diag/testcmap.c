
static char	sccsid[] = "@(#)testcmap.c 1.1 9/25/86 Copyright SMI";




/* ======================================================================
   Author: Peter Costello
   Date :  July 12, 1982
   Purpose:  Test color maps.
   Algorithm:
   Error Handling:
  ====================================================================== */

#include "cdiag.h"

uchar rcmap0[256],gcmap0[256],bcmap0[256];
uchar rcmap1[256],gcmap1[256],bcmap1[256];
uchar rcmap2[256],gcmap2[256],bcmap2[256];
uchar rcmap3[256],gcmap3[256],bcmap3[256];
uchar rcmapa[256],gcmapa[256],bcmapa[256];


/* Write constant data to color maps */
write_const(rdata,gdata,bdata,
	    rcmap,gcmap,bcmap,cmap)
   uchar rdata,gdata,bdata;
   uchar *rcmap,*gcmap,*bcmap;
   int cmap;
{
   short i;
   int wrcmap();

   for (i=0;i<256;i++) {
      *rcmap++ = rdata;
      *gcmap++ = gdata;
      *bcmap++ = bdata;
   }
   rcmap -= 256;
   gcmap -= 256;
   bcmap -= 256;

   write_cmap(wrcmap,rcmap,gcmap,bcmap,cmap);
      
}

/* Verify that two arrays are the same */
ver_cmap(list_p,tnum,wrmap,rdmap,col_str,mapnum)
   struct bd_list *list_p;
   int tnum;
   uchar *wrmap,*rdmap;
   char *col_str;
   int mapnum;
{
   short i,errcnt,stop;
   uchar *cmap,errs[20];

   for (i=0;i<256;i++) {
      if (*wrmap++ != *rdmap++) {		/* ERROR */
  	 --wrmap; --rdmap;
         printf("Device #%d @ 0x%x. Error %s Color Map #%d. Color 0x%x.",
	        list_p->device,list_p->base,col_str,mapnum,i);
	 printf(" Wr: 0x%x. Rd: 0x%x.\n",
		*wrmap,*rdmap);

	 stop = 0;
 	 if (*col_str=='R') {
	    cmap = (uchar*)(GR_bd_sel | GR_red_cmap | i);
	 } else if (*col_str=='G') {
	    cmap = (uchar*)(GR_bd_sel | GR_grn_cmap | i);
	 } else if (*col_str=='B') {
	    cmap = (uchar*)(GR_bd_sel | GR_blu_cmap | i);
	 } else {
	    stop = 1;
	 }

	 if (! stop) {
	    while (GR_retrace);			/* Wait for end of retrace */
	    while (!GR_retrace);
	    for (errcnt=1;errcnt<6;errcnt++) {
	       errs[errcnt] =  *cmap;
	    }
 	    printf("      ");
	    for (errcnt=1;errcnt<6;errcnt++) {
	       printf("Reread: 0x%x. ",errs[errcnt]); 
	    }
	    printf("\n");
	 }

         wrmap++; rdmap++;
	 list_p->error[tnum] += 1;
      }
   }
}
/* This routine verifies the color maps. The data written to the maps should
   be stored in 'rcmap0,gcmap0,bcmap0,...' for red,green,blue, and maps 0-3. */
verify_cmap(list_p,tnum)
   struct bd_list *list_p;
   int tnum;
{
   int rdcmap();

   read_cmap(rdcmap,rcmapa,gcmapa,bcmapa,0);
   ver_cmap(list_p,tnum,rcmap0,rcmapa,"Red",0);
   ver_cmap(list_p,tnum,gcmap0,gcmapa,"Green",0);
   ver_cmap(list_p,tnum,bcmap0,bcmapa,"Blue",0);

   read_cmap(rdcmap,rcmapa,gcmapa,bcmapa,1);
   ver_cmap(list_p,tnum,rcmap1,rcmapa,"Red",1);
   ver_cmap(list_p,tnum,gcmap1,gcmapa,"Green",1);
   ver_cmap(list_p,tnum,bcmap1,bcmapa,"Blue",1);

   read_cmap(rdcmap,rcmapa,gcmapa,bcmapa,2);
   ver_cmap(list_p,tnum,rcmap2,rcmapa,"Red",2);
   ver_cmap(list_p,tnum,gcmap2,gcmapa,"Green",2);
   ver_cmap(list_p,tnum,bcmap2,bcmapa,"Blue",2);

   read_cmap(rdcmap,rcmapa,gcmapa,bcmapa,3);
   ver_cmap(list_p,tnum,rcmap3,rcmapa,"Red",3);
   ver_cmap(list_p,tnum,gcmap3,gcmapa,"Green",3);
   ver_cmap(list_p,tnum,bcmap3,bcmapa,"Blue",3);
}


#define D0 (uchar)0xAA
#define D1 (uchar)0x55
#define D2 (uchar)0xCC
#define D3 (uchar)0x33
#define D4 (uchar)0x00
#define D5 (uchar)0xFF
#define D6 (uchar)0xC3
#define D7 (uchar)0x3C

/* These constant data tests also test out the independence of address lines
   C.A8 and C.A9 which are connected to the color map ram. They also test that
   the red,green,blue select logic functions properly. */
test_const(list_p,tnum)
   struct bd_list *list_p;
   int tnum;
{
   write_const(D0,D1,D2,rcmap0,gcmap0,bcmap0,0);
   write_const(D1,D2,D3,rcmap1,gcmap1,bcmap1,1);
   write_const(D2,D3,D4,rcmap2,gcmap2,bcmap2,2);
   write_const(D3,D4,D1,rcmap3,gcmap3,bcmap3,3);
   verify_cmap(list_p,tnum);

   write_const(D5,D1,D2,rcmap0,gcmap0,bcmap0,0);
   write_const(D2,D7,D3,rcmap1,gcmap1,bcmap1,1);
   write_const(D1,D6,D4,rcmap2,gcmap2,bcmap2,2);
   write_const(D3,D5,D3,rcmap3,gcmap3,bcmap3,3);
   verify_cmap(list_p,tnum);
}


#define Cmap_Size 256		/* 256 entries per color map. */

/* This test writes checkerboard patterns to the color maps. It only tests
   the independence of address lines A7 to A0, since the constant data test
   checks the independence of address lines A8 and A9 to the color maps. */
cmap_checker(list_p,tnum,rdata,gdata,bdata)
   struct bd_list *list_p;
   int tnum;
   uchar rdata,gdata,bdata;
{
  uchar *rcmap,*gcmap,*bcmap;
  short addr_incr,entry,i;
  int wrcmap();
  int rdcmap();

  addr_incr = 1;      /* Start by writing every other location with
 	  	         complementary data. Then write every two
			 locations with complementary data, etc. */

  do {		    /* For all possible address increments */
     entry = 0;
     rcmap = rcmap0;	/* Pointer to an array */
     gcmap = gcmap0;	/* Pointer to an array */
     bcmap = bcmap0;	/* Pointer to an array */
     do {
        /* Write N values of Data0 (where N = addr_incr) */
        rdata = (~rdata);   /* Invert all data. */ 
        gdata = (~gdata);   /* Invert all data. */ 
        bdata = (~bdata);   /* Invert all data. */ 
        i = 0;
        do {
           *rcmap++ = rdata;
           *gcmap++ = gdata;
           *bcmap++ = bdata;
           i += 1;
        } while (i < addr_incr);
        entry += addr_incr;

        /* Write N values of ~Data0 (where N = addr_incr) */
        rdata = (~rdata);
        gdata = (~rdata);
        bdata = (~rdata);
        i = 0;
        do {
           *rcmap++ = rdata;
           *gcmap++ = gdata;
           *bcmap++ = bdata;
           i += 1;
        } while (i < addr_incr);
        entry += addr_incr;
     } while (entry < Cmap_Size);
 
     write_cmap(wrcmap,rcmap0,gcmap0,bcmap0,0);
     write_cmap(wrcmap,rcmap0,gcmap0,bcmap0,1);
     write_cmap(wrcmap,rcmap0,gcmap0,bcmap0,2);
     write_cmap(wrcmap,rcmap0,gcmap0,bcmap0,3);

     read_cmap(rdcmap,rcmapa,gcmapa,bcmapa,0);
     ver_cmap(list_p,tnum,rcmap0,rcmapa,"Red",0);
     ver_cmap(list_p,tnum,gcmap0,gcmapa,"Green",0);
     ver_cmap(list_p,tnum,bcmap0,bcmapa,"Blue",0);

     read_cmap(rdcmap,rcmapa,gcmapa,bcmapa,1);
     ver_cmap(list_p,tnum,rcmap0,rcmapa,"Red",1);
     ver_cmap(list_p,tnum,gcmap0,gcmapa,"Green",1);
     ver_cmap(list_p,tnum,bcmap0,bcmapa,"Blue",1);
  
     read_cmap(rdcmap,rcmapa,gcmapa,bcmapa,2);
     ver_cmap(list_p,tnum,rcmap0,rcmapa,"Red",2);
     ver_cmap(list_p,tnum,gcmap0,gcmapa,"Green",2);
     ver_cmap(list_p,tnum,bcmap0,bcmapa,"Blue",2);
  
     read_cmap(rdcmap,rcmapa,gcmapa,bcmapa,3);
     ver_cmap(list_p,tnum,rcmap0,rcmapa,"Red",3);
     ver_cmap(list_p,tnum,gcmap0,gcmapa,"Green",3);
     ver_cmap(list_p,tnum,bcmap0,bcmapa,"Blue",3);

     addr_incr <<= 1;		/* Multiply incr by 2 */
   } while (addr_incr < Cmap_Size);

}	/* End of cmap_checker test */


#define X0 ((uchar)0xCA)
#define X1 ((uchar)0x33)
#define X2 ((uchar)0xAB)

/* This routine calls the other routines */
testcmap(list_p,tnum)
   struct bd_list *list_p;
   int tnum;
{
   int i;

   checker();					/* Set up a checker-board */
   for (i=0;i<5;i++) {
      test_const(list_p,tnum);			/* Do a constant data test */
      cmap_checker(list_p,tnum,X0,X1,X2);	/* Checkerboard test */
   }
}

