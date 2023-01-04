


/* ======================================================================
   Author: Peter Costello
   Date :  April 15, 1983
   Purpose: Do all manual and repeatitive tests for the color map.
   Algorithm:
   Error Handling:
   ====================================================================== */
static char     sccsid[] = "@(#)mancmap.c 1.1 9/25/86 Copyright Sun Micro";

#include "sc.diag.h"

mancmap(list_p)
  struct bd_list *list_p;
{
  char ch;
  ushort data;
  short i,quit,errs,high;
  ushort *addr,*red,*grn,*blu;
  ushort tred,tgrn,tblu;

  quit = false;

  SC_Stat = VEnable;		/* Load ECL cmap no more */
  while (!quit) {
   printf("Manual CMAP tests. Only Auto tests affect status reg.\n");
   printf("   1: Acquire access to TTL cmap\n");
   printf("   2: Relinquish access to TTL cmap\n");
   printf("   3: Load TTL -> ECL cmap once\n");
   printf("   4: Load cmap with default arrays\n");
   printf("   5: Verify cmap with default arrays\n");
   printf("   6: Load cmap with solid value\n");
   printf("   7: Verify cmap with solid value\n");
   printf("   8: Set 0-255 red, 256-511 grn, 512-767 blue\n");
   printf("   9: Test single location\n");
   printf("   A: Auto test\n");
   printf("   B: Continous auto test\n");
   printf("   C: Load cmap with ramp\n");
   printf("   D: Verify cmap with ramp\n");
   printf("   Q: Quit\n");
   printf("   Enter choice: ");
   ch = getchar(); printf("\n");
    
   if (ch == '1') {
      SC_Stat = VEnable;		/* Turn Off UpECmap */
   } else if (ch == '2') {
      SC_Stat = VEnable | UpECmap;	/* Turn On UpECmap */
   } else if (ch == '3') {
      while (SC_Retrace);
      SC_Stat = VEnable | UpECmap;	/* TTL -> ECL */
      while (!SC_Retrace);		/* Wait for start of retrace */
      while (SC_Retrace);		/* Wait for end of retrace */
      SC_Stat = VEnable;		/* Load ECL cmap no more */

   } else if (ch == '4') {		/* Load default cmap */
      /* schecker();			/* Load checker board */
      write_cmap(sc_red,sc_grn,sc_blu);	/* Write default arrays */
   } else if (ch == '5') {		/* Verify default cmap */
      red = SC_Red_Cmap;
      grn = SC_Grn_Cmap;
      blu = SC_Blu_Cmap;
      errs = 0;
      for (i=0;i<256;i++) {
	 data = *red++;
	 if (data != sc_red[i]) {
	    errs += 1;
	    printf("Entry 0x%x. RED. Read 0x%x. Compare w/ 0x%x. Xor 0x%x\n",
			i,data,sc_red[i],(data ^ sc_red[i]));
  	 }
	 data = *grn++;
	 if (data != sc_grn[i]) {
	    errs += 1;
	    printf("Entry 0x%x. GRN. Read 0x%x. Compare w/ 0x%x. Xor 0x%x\n",
			i,data,sc_grn[i],(data ^ sc_grn[i]));
  	 }
	 data = *blu++;
	 if (data != sc_blu[i]) {
	    errs += 1;
	    printf("Entry 0x%x. BLU. Read 0x%x. Compare w/ 0x%x. Xor 0x%x\n",
			i,data,sc_blu[i],(data ^ sc_blu[i]));
  	 }
      }
      printf("Total Errors: %d\n",errs);

   } else if (ch == '6') {	/* Load cmap with solid value */
      printf("Enter Red Intensity (0x0-0xFF): ");
      tred = getnh(); printf("\n");
      printf("Enter Grn Intensity (0x0-0xFF): ");
      tgrn = getnh(); printf("\n");
      printf("Enter Blu Intensity (0x0-0xFF): ");
      tblu = getnh(); printf("\n");

      printf("For range (low:high), Enter Low  (0-255): ");
      i = getn(); printf("\n");
      printf("For range (low:high), Enter High (0-255): ");
      data = getn(); printf("\n");

      red = SC_Red_Cmap;
      grn = SC_Grn_Cmap;
      blu = SC_Blu_Cmap;
      while ((i>=0)&&(i<256)&&(i<=data)) {
	 red[i] = tred;
	 grn[i] = tgrn;
	 blu[i] = tblu;
	 i += 1;
      }

   } else if (ch == '7') { 	/* Verify Cmap with solid value */
      printf("Enter Red Intensity (0x0-0xFF): ");
      tred = getnh(); printf("\n");
      printf("Enter Grn Intensity (0x0-0xFF): ");
      tgrn = getnh(); printf("\n");
      printf("Enter Blu Intensity (0x0-0xFF): ");
      tblu = getnh(); printf("\n");

      printf("For range (low:high), Enter Low  (0-255): ");
      i = getn(); printf("\n");
      printf("For range (low:high), Enter High (0-255): ");
      high = getn(); printf("\n");

      red = SC_Red_Cmap;
      grn = SC_Grn_Cmap;
      blu = SC_Blu_Cmap;
      errs = 0;
      while ((i>=0)&&(i<256)&&(i<=high)) {
	 data = red[i];
	 if (data != tred) {
	    errs += 1;
	    printf("Entry 0x%x. RED. Read 0x%x. Compare w/ 0x%x. Xor 0x%x\n",
			i,data,tred,(data ^ tred));
  	 }
	 data = grn[i];
	 if (data != tgrn) {
	    errs += 1;
	    printf("Entry 0x%x. GRN. Read 0x%x. Compare w/ 0x%x. Xor 0x%x\n",
			i,data,tgrn,(data ^ tgrn));
  	 }
	 data = blu[i];
	 if (data != tblu) {
	    errs += 1;
	    printf("Entry 0x%x. BLU. Read 0x%x. Compare w/ 0x%x. Xor 0x%x\n",
			i,data,tblu,(data ^ tblu));
  	 }
	 i += 1;
      }
      printf("Total Errors: %d\n",errs);

   } else if (ch=='8') {
      /* First third red, second third green, last third blue. */
      red = SC_Red_Cmap;
      grn = SC_Grn_Cmap;
      blu = SC_Blu_Cmap;
      for (i=0;i<256;i+=3) { *red++ = i; *grn++ = 0; *blu++ = 0; }
      for (i=0;i<256;i+=3) { *red++ = 0; *grn++ = i; *blu++ = 0; }
      for (i=0;i<256;i+=3) { *red++ = 0; *grn++ = 0; *blu++ = i; }
      *red++ = 0; *grn++ = 0; *blu++ = 0;
         
   } else if (ch=='9') { 	/* Test Single Location */
      printf("Red, Grn, or Blu cmap (rgb): ");
      ch = getchar(); printf("\n");
      if ((ch=='r')||(ch=='R')) {
	 addr = SC_Red_Cmap; 
      } else if ((ch=='g')||(ch=='G')) {
	 addr = SC_Grn_Cmap; 
      } else if ((ch=='b')||(ch=='B')) {
	 addr = SC_Blu_Cmap; 
      } else {
         ch = 'x';
      }
      if (ch != 'x') {
         printf("Offset (0x0-0xFF): ");
	 data = getnh(); printf("\n");
	 addr += data;		/* Advance n addresses */
	 mtest_reg(list_p,7,addr,(ushort)0xFF);
      }

   } else if ((ch=='a')||(ch=='A')) { 		/* Auto Test */
      /* printf("Testing Color Maps.\n"); */
      list_p->error[7] = 0;
      auto_cmap(list_p);
      printf("Color Map Testing Complete. ");
      if (list_p->error[7] == 0) {
         printf("No Errors.\n");
      } else {
         printf("%d Errors.\n",list_p->error[7]);
      }
   } else if ((ch=='b')||(ch=='B')) { 		/* Continuous Auto Test */
      i = 1;
      printf("Testing Color Maps Continously.\n");
      list_p->error[7] = 0;
      while (1) {
         auto_cmap(list_p);
         if (list_p->error[7] == 0) {
            printf("Test %d Complete. No Errors.\n",i++);
         } else {
            printf("Test %d Complete. %d Errors.\n",i++,list_p->error[0]);
         }
      }

   } else if ((ch=='c')||(ch=='C')) { 		/* Load ramp */
      red = SC_Red_Cmap;
      grn = SC_Grn_Cmap;
      blu = SC_Blu_Cmap;
      for (i=0;i<256;i++) {
	 *red++ = i;
	 *grn++ = i;
	 *blu++ = i;
      }

   } else if ((ch=='d')||(ch=='D')) {
      errs = 0;
      red = SC_Red_Cmap;
      grn = SC_Grn_Cmap;
      blu = SC_Blu_Cmap;
      for (i=0;i<256;i++) {
	 data = red[i];
	 if (data != i) {
	    errs += 1;
	    printf("Entry 0x%x. RED. Read 0x%x. Compare w/ 0x%x. Xor 0x%x\n",
			i,data,i,(data ^ i));
  	 }
	 data = grn[i];
	 if (data != i) {
	    errs += 1;
	    printf("Entry 0x%x. GRN. Read 0x%x. Compare w/ 0x%x. Xor 0x%x\n",
			i,data,i,(data ^ i));
  	 }
	 data = blu[i];
	 if (data != i) {
	    errs += 1;
	    printf("Entry 0x%x. BLU. Read 0x%x. Compare w/ 0x%x. Xor 0x%x\n",
			i,data,i,(data ^ i));
  	 }
      }
      printf("Total Errors: %d\n",errs);

   } else if ((ch=='q')||(ch=='Q')) { 		/* Quit */
      quit = 1;
   }
  }
}		/* End of Procedure tcmap() */
   
