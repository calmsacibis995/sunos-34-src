
/* ======================================================================
   Author: Peter Costello
   Date :  June 15, 1983
   Purpose: Auto Tests on TTL color Maps.
   Algorithm:
   Timing:
   Error Handling:
   Bugs:
   ====================================================================== */
static char     sccsid[] = "@(#)autocmap.c 1.1 9/25/86 Copyright Sun Micro";

#include "sc.diag.h"
#define wait_sec(sec) i2=sec;j2=Sec1;while (i2-- > 0) while (j2-- > 0)

#define load_ecl						\
	while (SC_Retrace);					\
	SC_Stat = VEnable + UpECmap;				\
	while (!SC_Retrace);					\
	while (SC_Retrace);	/* ECL Cmap being Loaded */	\
	SC_Stat = VEnable

/* Write constant data to color maps */
write_const(rdata,gdata,bdata)
   ushort rdata,gdata,bdata;
{
   register short i,r,g,b;
   ushort *rcmap,*gcmap,*bcmap;

   rcmap = SC_Red_Cmap; r = rdata;
   gcmap = SC_Grn_Cmap; g = gdata;
   bcmap = SC_Blu_Cmap; b = bdata;

   for (i=0;i<256;i++) {
      *rcmap++ = rdata;
      *gcmap++ = gdata;
      *bcmap++ = bdata;
   }
}

/* Verify that cmap and given arrays are the same */
ver_cmap(list_p,tnum,red1,grn1,blu1)
   struct bd_list *list_p;
   int tnum;
   ushort *red1,*grn1,*blu1;
{
   register ushort data;
   register short i;
   register ushort *rcmap,*gcmap,*bcmap;

   rcmap = SC_Red_Cmap;
   gcmap = SC_Grn_Cmap;
   bcmap = SC_Blu_Cmap;

   for (i=0;i<256;i++) {
      data = *rcmap++ & 0xFF;
      if (data != red1[i]&0xFF) {
	 printf("Testing %s",error_str[tnum]);
         printf("Entry 0x%x. RED. Read 0x%x. Compare w/ 0x%x. Xor 0x%x\n",
		i,data,red1[i],(data ^ red1[i]));
         list_p->error[tnum] += 1;
      }
      data = *gcmap++ & 0xFF;
      if (data != grn1[i]&0xFF) {
	 printf("Testing %s",error_str[tnum]);
         printf("Entry 0x%x. GRN. Read 0x%x. Compare w/ 0x%x. Xor 0x%x\n",
		i,data,grn1[i],(data ^ grn1[i]));
         list_p->error[tnum] += 1;
      }
      data = *bcmap++ & 0xFF;
      if (data != blu1[i]&0xFF) {
	 printf("Testing %s",error_str[tnum]);
         printf("Entry 0x%x. BLU. Read 0x%x. Compare w/ 0x%x. Xor 0x%x\n",
		i,data,blu1[i],(data ^ blu1[i]));
         list_p->error[tnum] += 1;
      }
   }
}		/* End of procedure ver_cmap() */

/* This procedure loads the given arrays with the constants supplied */
load_array(r,g,b,red,grn,blu)
   ushort r,g,b;
   ushort *red,*grn,*blu;
{
   register short i;
   for (i=0;i<256;i++) {
      *red++ = r;
      *grn++ = g;
      *blu++ = b;
   }
}

load_ramp(red,grn,blu)
   ushort *red,*grn,*blu;
{
   register ushort i;
   for (i=0;i<256;i++) {
      *red++ = i;
      *grn++ = i;
      *blu++ = i;
   }
}


#define Cmap_Size 256		/* 256 entries per color map. */
ushort rcmap[256],gcmap[256],bcmap[256];

/* This test writes checkerboard patterns to the color maps. */
cmap_checker(list_p,tnum,rdata,gdata,bdata)
   struct bd_list *list_p;
   int tnum;
   ushort rdata,gdata,bdata;
{
  short addr_incr,entry,i;
  int i2, j2;
  register int j;

  addr_incr = 1;      /* Start by writing every other location with
 	  	         complementary data. Then write every two
			 locations with complementary data, etc. */

  do {		      /* For all possible address increments */

     entry = 0;
     do {
        /* Write N values of Data0 (where N = addr_incr) */
        rdata = ~rdata; rdata &= 0xff;   /* Invert all data. */ 
        gdata = ~gdata; gdata &= 0xff;   /* Invert all data. */ 
        bdata = ~bdata; bdata &= 0xff;   /* Invert all data. */ 

	for (i=addr_incr;i>0;i-=1) {
           rcmap[entry] = rdata;
           gcmap[entry] = gdata;
           bcmap[entry] = bdata;
	   entry += 1;
        }

        /* Write N values of ~Data0 (where N = addr_incr) */
        rdata = ~rdata; rdata &= 0xff;   /* Invert all data. */ 
        gdata = ~gdata; gdata &= 0xff;   /* Invert all data. */ 
        bdata = ~bdata; bdata &= 0xff;   /* Invert all data. */ 
	
	for (i=addr_incr;i>0;i-=1) {
           rcmap[entry] = rdata;
           gcmap[entry] = gdata;
           bcmap[entry] = bdata;
	   entry += 1;
        }

     } while (entry < 256);
 
     write_cmap(rcmap,gcmap,bcmap);
     load_ecl;		/* Load ECL once */

     ver_cmap(list_p,tnum,rcmap,gcmap,bcmap);

     addr_incr = addr_incr << 1;	/* Multiply incr by 2 */
     wait_sec (1);
   } while (addr_incr < 256);

}	/* End of cmap_checker test */


#define D0 (ushort)0xAA
#define D1 (ushort)0x55
#define D2 (ushort)0xCC
#define D3 (ushort)0x33
#define D4 (ushort)0x00
#define D5 (ushort)0xFF
#define D6 (ushort)0xC3
#define D7 (ushort)0x3C
#define D8 (ushort)0x28
#define D9 (ushort)0xB7

ushort red[256],grn[256],blu[256], dummy[256];
auto_cmap(list_p)
   struct bd_list *list_p;
{
   int i, i2, j2, tnum;

   tnum = 7;

   schecker();					/* Set up a checker-board */
   for (i=0;i<3;i++) {
      SC_Stat = VEnable;

      /* Simple Tests first */
      /* printf("Testing Shadow Cmaps: 1"); */
      load_array(D0,D1,D2,red,grn,blu);
      write_cmap(red,grn,blu);
      ver_cmap(list_p,tnum,red,grn,blu);
      wait_sec (1);

      /* printf("2"); */
      load_array(D4,D5,D6,red,grn,blu);
      write_cmap(red,grn,blu);
      ver_cmap(list_p,tnum,red,grn,blu);
      wait_sec (1);

      /* printf("3");*/
      load_array(D7,D8,D9,red,grn,blu);
      write_cmap(red,grn,blu);
      ver_cmap(list_p,tnum,red,grn,blu);
      wait_sec (1);

      /* printf("4"); */
      load_ramp(red,grn,blu);
      load_array(D4,D4,D4,dummy,grn,blu);
      write_cmap(red,grn,blu);
      ver_cmap(list_p,tnum,red,grn,blu);
      wait_sec (1);
       
      /* printf("5"); */
      load_ramp(red,grn,blu);
      load_array(D4,D4,D4,red,dummy,blu);
      write_cmap(red,grn,blu);
      ver_cmap(list_p,tnum,red,grn,blu);
      wait_sec (1);
       
      /* printf("6"); */
      load_ramp(red,grn,blu);
      load_array(D4,D4,D4,red,grn,dummy);
      write_cmap(red,grn,blu);
      ver_cmap(list_p,tnum,red,grn,blu);
      wait_sec (1);
       
      /* printf("7"); */
      load_ramp(red,grn,blu);
      write_cmap(red,grn,blu);
      ver_cmap(list_p,tnum,red,grn,blu);
      wait_sec (1);
       
      /* printf("8");*/
      cmap_checker(list_p,tnum,D0,D6,D8);

      /* printf("9");*/

      cmap_checker(list_p,tnum,D6,D1,D2);

   }
}		/* End of auto_cmap() */

