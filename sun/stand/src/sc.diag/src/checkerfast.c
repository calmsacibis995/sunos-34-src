static char     sccsid[] = "@(#)checkerfast.c 1.1 9/25/86 Copyright Sun Micro";

char *checkstr[20] = {"Checker (a0)","Checker (a1)","Checker (a2)",
	"Checker (a3)","Checker (a4)", "Checker (a5)","Checker (a6)",
	"Checker (a7)","Checker (a8)", "Checker (a9)","Checker (a10)",
	"Checker (a11)","Checker (a12)", "Checker (a13)","Checker (a14)",
	"Checker (a15)","Checker (a16)", "Checker (a17)","Checker (a18)",
	"Checker (a19)"};

typedef unsigned int uint;

/* This routine checks that memory is ok. */
checker_ver(abit,alo,data)
   	uint abit,*alo,data;
{

	register uint *addr,*ahi;
	register uint data0,data1;
	register int i,temp,cerrs,incr;

	ahi = (uint *)(((int)alo)+0x100000);
	data0 = data; data1 = ~data0;
	cerrs = 0;	    /* No errors yet */
 	incr = 1<<abit;

	/* Read back the checkerboard we just wrote. Because the
	   size of this memory block is a power of two, we won't encounter
	   any addressing problems by writing at some illegal loc. */
	addr = alo;
	while (addr < ahi) {
	   /* Read N-1 values of Data0 (where N = Addr_Incr) */
	   i = 1; 
	   do {
	      temp = *addr++;
              if (temp != data0) {
                 report(checkstr[abit],data0,temp,--addr); addr++; cerrs+=1;
	      }
	   } while (i+=1 < incr);

	   /* Read N-1 values of Data1 (where N = Addr_Incr) */
	   i = 1; 
	   do {
	      temp = *addr++;
              if (temp != data1) {
                 report(checkstr[abit],data0,temp,--addr); addr++; cerrs+=1;
	      }
	   } while (i+=1 < incr);
	}

	if (cerrs == 0) {
 	   printf("Spurious read error. %s test. ",checkstr[abit]);
	   printf("Addr/Data unknown.\n");
           report(checkstr[abit],data0,data0,alo); 
	   cerrs = 1;
	}
	return(cerrs);

}	/* End of checker_ver */


checker_a2(alo,data)
  	uint *alo,data;
{
   	register uint *addr;
	register uint data0,data1,temp0,temp1;
	register int i;

	data0 = data; data1 = ~data; temp0 = data0; temp1 = data1; 
	addr = alo;

	for (i=1<<14;i>0;i-=1) {     /* Fill memory with pattern */
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
	for (i=1<<15;i>0;i-=1) {	   /* Check memory */
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
		return(checker_ver(2,alo,data)); 
	} else {
	   	return(0);
	}

}

checker_a3(alo,data)
  	uint *alo,data;
{
   	register uint *addr;
	register uint data0,data1,temp0,temp1;
	register int i;

	data0 = data; data1 = ~data; temp0 = data0; temp1 = data1; addr = alo;

	for (i=1<<14;i>0;i-=1) {     /* Fill memory with pattern */
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
	for (i=1<<15;i>0;i-=1) {	   /* Check memory */
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
		return(checker_ver(3,alo,data)); 
	} else {
	   	return(0);
	}

}

checker_a4(alo,data)
  	uint *alo,data;
{
   	register uint *addr;
	register uint data0,data1,temp0,temp1;
	register int i;

	data0 = data; data1 = ~data; temp0 = data0; temp1 = data1; addr = alo;

	for (i=1<<14;i>0;i-=1) {     /* Fill memory with pattern */
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
	for (1<<15;i>0;i-=1) {	   /* Check memory */
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
		return(checker_ver(4,alo,data)); 
	} else {
	   	return(0);
	}
}


checker_ax(num,alo,data)	
  	uint num,*alo,data;		/* Num is the addr bit being tested */
{ 					/* check_a5 thru check_a19 */
   	register uint *addr;
	register uint data0,data1,temp0,temp1,j;
	register int i;

	data0 = data; data1 = ~data; temp0 = data0; temp1 = data1; addr = alo;

	for (i=1<<(19-num);i>0;i-=1) {     /* Fill memory with pattern */
	   for (j=num-4;j>0;j-=1) {
		*addr++ = data0; *addr++ = data0;
		*addr++ = data0; *addr++ = data0;
		*addr++ = data0; *addr++ = data0;
		*addr++ = data0; *addr++ = data0;
	   }
	   for (j=num-4;j>0;j-=1) {
		*addr++ = data1; *addr++ = data1;
		*addr++ = data1; *addr++ = data1;
		*addr++ = data1; *addr++ = data1;
		*addr++ = data1; *addr++ = data1;
	   }
	}
	
	addr = alo;
	for (i=1<<(19-num);i>0;i-=1) {	   /* Check memory */
	   for (j=num-4;j>0;j-=1) {
		data0 |= *addr; temp0 &= *addr++;
		data0 |= *addr; temp0 &= *addr++;
		data0 |= *addr; temp0 &= *addr++;
		data0 |= *addr; temp0 &= *addr++;
		data0 |= *addr; temp0 &= *addr++;
		data0 |= *addr; temp0 &= *addr++;
		data0 |= *addr; temp0 &= *addr++;
		data0 |= *addr; temp0 &= *addr++;
	   }
	   for (j=num-4;j>0;j-=1) {
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
		return(checker_ver(num,alo,data)); 
	} else {
	   	return(0);
	}
}

checkerfast(alo,ahi,data)
	uint *alo,*ahi,data;
{
	int cerrs;

	/* alo and ahi must be 1 MB apart */
	if ((((int)ahi)-((int)alo)) != 0x100000) {
	   printf("Prgram Error: Checkerfast hardwired for 1 MB memory\n");
	   return(0);
	} 
	cerrs  = checker_a2(alo,data);
	cerrs += checker_a3(alo,data);
	cerrs += checker_a4(alo,data);
	cerrs += checker_ax(5,alo,data);
	cerrs += checker_ax(6,alo,data);
	cerrs += checker_ax(7,alo,data);
	cerrs += checker_ax(8,alo,data);
	cerrs += checker_ax(9,alo,data);
	cerrs += checker_ax(10,alo,data);
	cerrs += checker_ax(11,alo,data);
	cerrs += checker_ax(12,alo,data);
	cerrs += checker_ax(13,alo,data);
	cerrs += checker_ax(14,alo,data);
	cerrs += checker_ax(15,alo,data);
	cerrs += checker_ax(16,alo,data);
	cerrs += checker_ax(17,alo,data);
	cerrs += checker_ax(18,alo,data);
	cerrs += checker_ax(19,alo,data);
	return(cerrs);
}

