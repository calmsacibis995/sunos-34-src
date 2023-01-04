
static char	sccsid[] = "@(#)tcmap.c 1.1 9/25/86 Copyright SMI";


/* ======================================================================
   Author: Peter Costello
   Date :  October 21, 1982
   Purpose: Do all manual and repeatitive tests for the color map.
   Algorithm:
   Error Handling:
   ====================================================================== */

#include "cdiag.h"

uchar cmap1[256],cmap2[256],cmap3[256];

#define RD 0
#define WR 1
#define RD_WR 2

tcmap(list_p)
  struct bd_list *list_p;
{
  char ch;
  uchar register *carr1,*carr2,*carr3;
  uchar data,temp;
  int i;
  short j,quit;
  int cmap;
  int wrcmap(),rdcmap();

  quit = false;
  while (!quit) {
   printf("   A: Load color map with original values, and display. \n");
   printf("   B: Write solid value to color map. \n");
   printf("   C: Load Cmap with 0-84 red, 85-169 grn, rest blue. \n");
   printf("   D: Continously Load color map w/ orig values. Verify. \n");
   printf("   E: Display different color map.\n");
   printf("   F: Read from color map.\n");
   printf("   G: Write to color map.\n");
   printf("   H: Read/Write color map.\n");
   printf("   I: Memory Test on Color Maps.\n");
   printf("   J: Values 0-256 on locs 0-256 for RGB.\n");
   printf("   K: Continous color map memory test.\n");
   printf("   L: Test single location.\n");
   printf("   Q: Quit to top level.\n");
   printf("   Enter choice: ");
   /* scanf("%c",ch); */
   ch = getchar(); printf("\n");
    
   if ((ch == 'a')||(ch == 'A')) {
      /* printf("Enter default map to use (0-2): "); */
      /* scanf("%d", cmap); */
      /* cmap = getn(); printf("\n"); */

      /* if (cmap==0) { */
         carr1 = (uchar *)gr_red_c0;
         carr2 = (uchar *)gr_grn_c0;
         carr3 = (uchar *)gr_blu_c0;
      /* } else if (cmap==1) {
         carr1 = (uchar *)gr1_red_c1;
         carr2 = (uchar *)gr1_grn_c1;
         carr3 = (uchar *)gr1_blu_c1;
      } else if (cmap==2) {
         carr1 = (uchar *)gr2_red_c2;
         carr2 = (uchar *)gr2_grn_c2;
         carr3 = (uchar *)gr2_blu_c2;
      }		*/
      printf("Enter color map to write (0-3): ");
      /* scanf("%d", cmap); */
      cmap = getn(); printf("\n");
      cmap &= 0x03;

      write_cmap(wrcmap,carr1,carr2,carr3,cmap);

   } else if ((ch == 'b')||(ch == 'B')) {
      printf("Enter Red: ");
      /* scanf("%d",i); */
      i = getn(); printf("\n");
      printf("Red = %d \n",i);
      carr1 = (uchar *)cmap1;
      for (j=256;j>0;j--){
         *carr1++ = (uchar)i;
      }
      carr1 = (uchar *)cmap1;

      printf("Enter Green: ");
      /* scanf("%d",i); */
      i = getn(); printf("\n");
      printf("Green = %d \n",i);
      carr2 = (uchar *)cmap2;
      for (j=256;j>0;j--){
         *carr2++ = (uchar)i;
      }
      carr2 = (uchar *)cmap2;

      printf("Enter Blue: ");
      /* scanf("%d",i); */
      i = getn(); printf("\n");
      printf("Blue = %d \n",i);
      carr3 = (uchar *)cmap3;
      for (j=256;j>0;j--){
         *carr3++ = (uchar)i;
      }
      carr3 = (uchar*)cmap3;

      write_cmap(wrcmap,cmap1,cmap2,cmap3,0);

   } else if ((ch == 'c')||(ch == 'C')) {
      printf("Enter color map to write: ");
      /* scanf("%d",cmap); */
      cmap = getn(); printf("\n");
      cmap &= 0x03;		/* Only 0-3 valid */
      printf("Writing color map %d.\n",cmap);

      carr1 = (uchar*)cmap1;
      carr2 = (uchar*)cmap2;
      carr3 = (uchar*)cmap3;

      for (i=0;i<85;i++){
         *carr1++ = i*3;
         *carr2++ = 0;
         *carr3++ = 0;
      }

      for (i=0;i<85;i++){
         *carr2++ = i*3;
         *carr1++ = 0;
         *carr3++ = 0;
      }

      for (i=0;i<85;i++){
         *carr3++ = i*3;
         *carr1++ = 0;
         *carr2++ = 0;
      }

      carr1 = (uchar*)cmap1;
      carr2 = (uchar*)cmap2;
      carr3 = (uchar*)cmap3;

      write_cmap(wrcmap,carr1,carr2,carr3,cmap);

   } else if ((ch == 'd')||(ch == 'D')) {
      printf("Write, Read, and verify cmap continuously. \n");
      while (1) {
         write_cmap(wrcmap,gr_red_c0,gr_grn_c0,gr_blu_c0,0);
         read_cmap(rdcmap,cmap1,cmap2,cmap3,0);
         printf("Error count: Red: ");
         printf("%d ",cmap_sverify(gr_red_c0,cmap1) );
         printf(" Green: ");
         printf("%d ",cmap_sverify(gr_grn_c0,cmap2));
         printf(" Blue: ");
         printf("%d ",cmap_sverify(gr_blu_c0,cmap3));
      }

   } else if ((ch == 'e')||(ch == 'E')) {
      printf("Which color map do you wish to display(0-3),\n");
      printf("     (choice 4 toggles cmaps 0 and 1)      : ");
      /* scanf("%d",cmap); */
      cmap = getn(); printf("\n");
      if (cmap == 4) {
         while (1) {
	    if (GR_retrace) while (GR_retrace);	/* Wait for display */
	    cmap = (cmap+1)&1;
	    while (!GR_retrace);			/* Wait for retrace */
            Set_Video_Cmap(cmap);		/* Toggle cmap */
         }
      } else {
         cmap &= 0x03;
	 Set_Video_Cmap(cmap);
      }
	
   } else if ((ch == 'f')||(ch == 'F')) {
      cont_rwrite(RD);
   } else if ((ch == 'g')||(ch == 'G')) {
      cont_rwrite(WR);
   } else if ((ch == 'h')||(ch == 'H')) {
      cont_rwrite(RD_WR);
   } else if ((ch == 'i')||(ch == 'I')) {
      printf("Testing Color Maps.\n");
      list_p->error[0] = 0;
      testcmap(list_p,0);
      printf("Color Map Testing Complete. ");
      if (list_p->error[0] == 0) {
         printf("No Errors.\n");
      } else {
         printf("%d Errors.\n",list_p->error[0]);
      }
   } else if ((ch == 'j')||(ch == 'J')) {
      set_straight(list_p);
   } else if ((ch == 'k')||(ch == 'K')) {
      i = 1;
      printf("Testing Color Maps Continously.\n");
      list_p->error[0] = 0;
      while (1) {
         testcmap(list_p,0);
         if (list_p->error[0] == 0) {
            printf("Test %d Complete. No Errors.\n",i++);
         } else {
            printf("Test %d Complete. %d Errors.\n",i++,list_p->error[0]);
         }
      }
   } else if ((ch == 'l')||(ch == 'L')) {
      printf("Enter Color Map: ");
      cmap = getn(); printf("\n");
      printf("Enter Offset (Hex): ");
      i = getnh(); printf("\n");
      printf("Enter color (R/G/B): ");
      ch = getchar();
      if ((ch == 'r')||(ch == 'R')) {
         carr1 = (uchar*)(GR_bd_sel | GR_red_cmap | i);
      } else if ((ch == 'g')||(ch == 'G')) {
         carr1 = (uchar*)(GR_bd_sel | GR_grn_cmap | i);
      } else {
         carr1 = (uchar*)(GR_bd_sel | GR_blu_cmap | i);
      }
      printf("Enter Data (hex): ");
      data = getnh(); printf("\n");
      while (GR_retrace);		/* wait for end of retrace */
      while (! GR_retrace);
      *carr1 = data;
      temp = *carr1;
      printf("Wrote 0x%x. Read 0x%x.\n",data,temp);
      
   } else if ((ch == 'q')||(ch == 'Q')) {
      quit = true;
   } else {
      printf("   ILLEGAL INPUT.\n");
   }
  }
}



/* ======================================================================
   Author: Peter Costello
   Date :  April 21, 1982
   Purpose: Verify that the two arrays supplied are identical. If not, then
      print each location that differs. 
   Algorithm:
   Error Handling:
   ====================================================================== */

cmap_verify(list_p,col_str,mapnum,cmap1,cmap2)
   struct bd_list *list_p;
   char *col_str;
   int mapnum;
   uchar *cmap1,*cmap2;
{
   uchar register
      *ccmap1,*ccmap2,color1,color2;

   short register 
      i;

   ccmap1 = (uchar *)cmap1;
   ccmap2 = (uchar *)cmap2;
   for (i=0;i<256;i++) {
      color1 = *ccmap1++;
      color2 = *ccmap2++; 
      if (color1 != color2) {
         printf("Device #%d @ 0x%x. Error %s Color Map #%d. Color 0x%x.\n",
	        list_p->device,list_p->base,col_str,mapnum,i);
         printf("Wrote %d, Read %d \n",color1,color2);
      }
   }

}



/* ======================================================================
   Author: Peter Costello
   Date :  April 21, 1982
   Purpose: Compare the two 256 byte arrays, and return the number of 
       locations that differ. 
   Algorithm:
   Error Handling:
   ====================================================================== */

cmap_sverify(cmap1,cmap2)
   uchar *cmap1,*cmap2;
{
   uchar register
      *ccmap1,*ccmap2,color1,color2;

   short register 
      cnt,i;

   ccmap1 = (uchar *)cmap1;
   ccmap2 = (uchar *)cmap2;
   cnt = 0;
   for (i=0;i<256;i++) {
      color1 = *ccmap1++;
      color2 = *ccmap2++; 
      if (color1 != color2) {
         cnt += 1;
      }
   }
   return(cnt);
}

cont_rwrite(option)
   int option;
{
   uchar *cmap;
   uchar udata,udata1;
   short cmapnum,data,loc;
   char ch,once,color;

   printf("   Once or Continously (O/C)? ");
   once = getchar(); printf("\n");
   printf("   Color Map (0,1,2,3)? ");
   cmapnum = getn(); printf("\n");
   if ((cmapnum < 0)||(cmapnum > 3)) goto E_return; /* Error */

   Set_RW_Cmap(cmapnum);		/* Write this color map */
   Set_Video_Cmap(cmapnum);		/* Display this one too */

   printf("   Red,Green,or Blue (R,G,B)? ");
   color = getchar(); printf("\n");
   if ((color == 'r')||(color == 'R')) {
      cmap = (uchar*)(GR_bd_sel | GR_red_cmap);
   } else if ((color == 'g')||(color == 'G')) {
      cmap = (uchar*)(GR_bd_sel | GR_grn_cmap);
   } else if ((color == 'b')||(color == 'B')) {
      cmap = (uchar*)(GR_bd_sel | GR_blu_cmap);
   } else {
      goto E_return;		/* Error */
   }

   printf("   Location (0x00 - 0xFF)? ");
   loc = getnh(); printf("\n");
   cmap += loc;		/* Advance offset */

   if (option != RD) {
      printf("   Data Value (Hex)? ");
      data = getnh(); printf("\n");
      udata = (uchar)data;
   }

   if ((color == 'r')||(color == 'R')) {
      printf("Red ");
   } else if ((color == 'g')||(color == 'G')) {
      printf("Green ");
   } else {
      printf("Blue ");
   }
   printf("color map #%d. Location 0x%x. ",cmapnum,loc);

   if (option == WR) {
      if ((once == 'c')||(once == 'C')) {
	 printf("Writing 0x%x Continously...\n",data);
         while (1) *cmap = udata;
      } else {
         while (GR_retrace);		/* Wait for end of blanking */
         while (!GR_retrace);	 	/* Wait for start of retrace */
	 *cmap = udata;
	 printf("Wrote 0x%x.\n",data);
      }
   } else if (option == RD) {
      if ((once == 'c')||(once == 'C')) {
         printf("Reading Continuously...\n");
	 while (1) udata1 = *cmap;
      } else {
         while (GR_retrace);		/* Wait for end of blanking */
         while (!GR_retrace);		/* Wait for start of retrace */
         udata1 = *cmap;
	 printf("Read 0x%x.\n",udata1);
      }
   } else {				/* Read-Write option */
      if ((once == 'c')||(once == 'C')) {
         printf("Writing 0x%x then Reading Continuously...\n",data);
	 while (1) {*cmap = udata; udata1 = *cmap;}
      } else {
         while (GR_retrace);		/* Wait for end of blanking */
         while (!GR_retrace);		/* Wait for start of retrace */
 	 *cmap = udata;
         udata1 = *cmap;
	 printf("Wrote 0x%x. Read 0x%x.\n",udata,udata1);
      }
   }
E_return: ;		/* Error Return */
}	/* End of cont_rwrite */


/* Write values 0-256 to Memory locations 0-256 for RBG. */
set_straight(list_p)
   struct bd_list *list_p;
{
   uchar *carr1;
   short i;
   uchar data;
   int cmap;

   printf("Enter color map to write: ");
   /* scanf("%d", cmap); */
   cmap = getn(); printf("\n");
   cmap &= 0x03;		/* Only 0-3 valid */

   carr1 = (uchar*)cmap1;
   data = (uchar)0;
   for (i=0;i<256;i++) {
      *carr1++ = data++;
   }
   carr1 = (uchar*)cmap1;	/* Don't forget to point to start of array. */

   printf("Writing color map.\n");
   write_cmap(wrcmap,carr1,carr1,carr1,cmap);

   printf("Reading color map.\n");
   read_cmap(rdcmap,gr1_red_c1,gr1_grn_c1,gr1_blu_c1,cmap);
   printf("Verifying red color map. \n");
   cmap_verify(list_p,"Red",cmap,carr1,gr1_red_c1);
   printf("Verifying green color map. \n");
   cmap_verify(list_p,"Green",cmap,carr1,gr1_grn_c1);
   printf("Verifying blue color map. \n");
   cmap_verify(list_p,"Blue",cmap,carr1,gr1_blu_c1);

}
