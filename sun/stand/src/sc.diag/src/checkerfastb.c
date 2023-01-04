static char     sccsid[] = "@(#)checkerfastb.c 1.1 9/25/86 Copyright Sun Micro";

char *checkstrb[20] = {"Checker (a0)","Checker (a1)","Checker (a2)",
	"Checker (a3)","Checker (a4)", "Checker (a5)","Checker (a6)",
	"Checker (a7)","Checker (a8)", "Checker (a9)","Checker (a10)",
	"Checker (a11)","Checker (a12)", "Checker (a13)","Checker (a14)",
	"Checker (a15)","Checker (a16)", "Checker (a17)","Checker (a18)",
	"Checker (a19)"};

typedef unsigned char uchar;

/* This routine checks that memory is ok. */
checker_verb(abit,alo,data)
   	uchar abit,*alo,data;
{

	register uchar *addr,*ahi;
	register uchar data0,data1;
	register int i,temp,cerrs,incr;

	ahi = (uchar *)(((int)alo)+0x100000);
	data0 = data; data1 = ~data0;
	cerrs = 0;	    /* No errors yet */
 	incr = 1 << abit;

	/* Read back the checkerboard we just wrote. Because the
	   size of this memory block is a power of two, we won't encounter
	   any addressing problems by writing at some illegal loc. */
	addr = alo;
	while (addr < ahi) {
	   /* Read N values of Data0 (where N = Addr_Incr) */
	   i = 0; 
	   do {
	      temp = *addr++;
              if (temp != data0) {
                 report(checkstrb[abit],data0,temp,--addr); addr++; cerrs+=1;
	      }
	      i += 1;
	   } while (i < incr);

	   /* Read N values of Data1 (where N = Addr_Incr) */
	   i = 0; 
	   do {
	      temp = *addr++;
              if (temp != data1) {
                 report(checkstrb[abit],data1,temp,--addr); addr++; cerrs+=1;
	      }
	      i += 1;
	   } while (i < incr);
	}

	if (cerrs == 0) {
 	   printf("Spurious read error. %s test. ",checkstrb[abit]);
	   printf("Addr/Data unknown.\n");
           report(checkstrb[abit],data0,data0,alo); 
	   cerrs = 1;
	}
	return(cerrs);

}	/* End of checker_verb */


checker_a0b(alo,data)
  	uchar *alo,data;
{
   	register uchar *addr;
	register uchar data0,data1,temp0,temp1;
	register int i;

	data0 = data; data1 = ~data; temp0 = data0; temp1 = data1; 
	addr = alo;

	for (i=1<<16;i>0;i-=1) {     /* Fill memory with pattern */
		*addr++ = data0; *addr++ = data1;
		*addr++ = data0; *addr++ = data1;
		*addr++ = data0; *addr++ = data1;
		*addr++ = data0; *addr++ = data1;
		*addr++ = data0; *addr++ = data1;
		*addr++ = data0; *addr++ = data1;
		*addr++ = data0; *addr++ = data1;
		*addr++ = data0; *addr++ = data1;
	}
	
	addr = alo;
	for (i=1<<17;i>0;i-=1) {	   /* Check memory */
		data0 |= *addr; temp0 &= *addr++;
		data1 |= *addr; temp1 &= *addr++;
		data0 |= *addr; temp0 &= *addr++;
		data1 |= *addr; temp1 &= *addr++;
		data0 |= *addr; temp0 &= *addr++;
		data1 |= *addr; temp1 &= *addr++;
		data0 |= *addr; temp0 &= *addr++;
		data1 |= *addr; temp1 &= *addr++;
	}

	if ((data0 != data)||(temp0 != data)||
	    (data1 != ~data)||(temp1 != ~data)) {
		return(checker_verb(0,alo,data)); 
	} else {
	   	return(0);
	}

}

checker_a1b(alo,data)
  	uchar *alo,data;
{
   	register uchar *addr;
	register uchar data0,data1,temp0,temp1;
	register int i;

	data0 = data; data1 = ~data; temp0 = data0; temp1 = data1; 
	addr = alo;

	for (i=1<<16;i>0;i-=1) {     /* Fill memory with pattern */
		*addr++ = data0; *addr++ = data0;
		*addr++ = data1; *addr++ = data1;
		*addr++ = data0; *addr++ = data0;
		*addr++ = data1; *addr++ = data1;
		*addr++ = data0; *addr++ = data0;
		*addr++ = data1; *addr++ = data1;
		*addr++ = data0; *addr++ = data0;
		*addr++ = data1; *addr++ = data1;
	}
	
	addr = alo;
	for (i=1<<17;i>0;i-=1) {	   /* Check memory */
		data0 |= *addr; temp0 &= *addr++;
		data0 |= *addr; temp0 &= *addr++;
		data1 |= *addr; temp1 &= *addr++;
		data1 |= *addr; temp1 &= *addr++;
		data0 |= *addr; temp0 &= *addr++;
		data0 |= *addr; temp0 &= *addr++;
		data1 |= *addr; temp1 &= *addr++;
		data1 |= *addr; temp1 &= *addr++;
	}

	if ((data0 != data)||(temp0 != data)||
	    (data1 != ~data)||(temp1 != ~data)) {
		return(checker_verb(1,alo,data)); 
	} else {
	   	return(0);
	}

}

checker_a2b(alo,data)
  	uchar *alo,data;
{
   	register uchar *addr;
	register uchar data0,data1,temp0,temp1;
	register int i;

	data0 = data; data1 = ~data; temp0 = data0; temp1 = data1; addr = alo;

	for (i=1<<16;i>0;i-=1) {     /* Fill memory with pattern */
		*addr++ = data0; *addr++ = data0;
		*addr++ = data0; *addr++ = data0;
		*addr++ = data1; *addr++ = data1;
		*addr++ = data1; *addr++ = data1;
		*addr++ = data0; *addr++ = data0;
		*addr++ = data0; *addr++ = data0;
		*addr++ = data1; *addr++ = data1;
		*addr++ = data1; *addr++ = data1;
	}
	
	addr = alo;
	for (1<<17;i>0;i-=1) {	   /* Check memory */
		data0 |= *addr; temp0 &= *addr++;
		data0 |= *addr; temp0 &= *addr++;
		data0 |= *addr; temp0 &= *addr++;
		data0 |= *addr; temp0 &= *addr++;
		data1 |= *addr; temp1 &= *addr++;
		data1 |= *addr; temp1 &= *addr++;
		data1 |= *addr; temp1 &= *addr++;
		data1 |= *addr; temp1 &= *addr++;
	}

	if ((data0 != data)||(temp0 != data)||
	    (data1 != ~data)||(temp1 != ~data)) {
		return(checker_verb(2,alo,data)); 
	} else {
	   	return(0);
	}
}


checker_axb(num,alo,data)	
  	uchar num,*alo,data;		/* Num is the addr bit being tested */
{ 					/* check_a3 thru check_a19 */
   	register uchar *addr;
	register uchar data0,data1,temp0,temp1;
	register int i,j;

	data0 = data; data1 = ~data; temp0 = data0; temp1 = data1; addr = alo;

	for (i=1<<(19-num);i>0;i-=1) {     /* Fill memory with pattern */
	   for (j=num-2;j>0;j-=1) {
		*addr++ = data0; *addr++ = data0;
		*addr++ = data0; *addr++ = data0;
		*addr++ = data0; *addr++ = data0;
		*addr++ = data0; *addr++ = data0;
	   }
	   for (j=num-2;j>0;j-=1) {
		*addr++ = data1; *addr++ = data1;
		*addr++ = data1; *addr++ = data1;
		*addr++ = data1; *addr++ = data1;
		*addr++ = data1; *addr++ = data1;
	   }
	}
	
	addr = alo;
	for (i=1<<(19-num);i>0;i-=1) {	   /* Check memory */
	   for (j=num-2;j>0;j-=1) {
		data0 |= *addr; temp0 &= *addr++;
		data0 |= *addr; temp0 &= *addr++;
		data0 |= *addr; temp0 &= *addr++;
		data0 |= *addr; temp0 &= *addr++;
		data0 |= *addr; temp0 &= *addr++;
		data0 |= *addr; temp0 &= *addr++;
		data0 |= *addr; temp0 &= *addr++;
		data0 |= *addr; temp0 &= *addr++;
	   }
	   for (j=num-2;j>0;j-=1) {
		data1 |= *addr; temp1 &= *addr++;
		data1 |= *addr; temp1 &= *addr++;
		data1 |= *addr; temp1 &= *addr++;
		data1 |= *addr; temp1 &= *addr++;
		data1 |= *addr; temp1 &= *addr++;
		data1 |= *addr; temp1 &= *addr++;
		data1 |= *addr; temp1 &= *addr++;
		data1 |= *addr; temp1 &= *addr++;
	   }
	}

	if ((data0 != data)||(temp0 != data)||
	    (data1 != ~data)||(temp1 != ~data)) {
		return(checker_verb(num,alo,data)); 
	} else {
	   	return(0);
	}
}

checkerfastb(alo,ahi,data)
	uchar *alo,*ahi,data;
{
	int cerrs;

	/* alo and ahi must be 1 MB apart */
	if ((((int)ahi)-((int)alo)) != 0x100000) {
	   printf("Prgram Error: Checkerfast hardwired for 1 MB memory\n");
	   return(0);
	} 
	cerrs  = checker_a0b(alo,data);
	cerrs += checker_a1b(alo,data);
	cerrs += checker_a2b(alo,data);
	cerrs += checker_axb(3,alo,data);
	cerrs += checker_axb(4,alo,data);
	cerrs += checker_axb(5,alo,data);
	cerrs += checker_axb(6,alo,data);
	cerrs += checker_axb(7,alo,data);
	cerrs += checker_axb(8,alo,data);
	cerrs += checker_axb(9,alo,data);
	cerrs += checker_axb(10,alo,data);
	cerrs += checker_axb(11,alo,data);
	cerrs += checker_axb(12,alo,data);
	cerrs += checker_axb(13,alo,data);
	cerrs += checker_axb(14,alo,data);
	cerrs += checker_axb(15,alo,data);
	cerrs += checker_axb(16,alo,data);
	cerrs += checker_axb(17,alo,data);
	cerrs += checker_axb(18,alo,data);
	cerrs += checker_axb(19,alo,data);
	return(cerrs);
}

