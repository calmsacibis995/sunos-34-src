
/*
 * memtest.c
 *
 * MC68000 memory test program
 *
 */
static char     sccsid[] = "@(#)memtest.c 1.1 9/25/86 Copyright Sun Micro";

typedef unsigned int uint;

int seed = 433;
int halt_on_err = 0;

testn(a1,b1,n1)    /* fill memory with n, read it back */
uint *a1,*b1,n1;
{
        register uint *tp,temp0,temp1;
	register uint *a,*b,n;
        int errs=0;

	a = a1; b = b1; n = n1;
        for (tp=a;tp<b;) {	/* 32 unrolled writes */
		*tp++=n; *tp++=n; *tp++=n; *tp++=n;
		*tp++=n; *tp++=n; *tp++=n; *tp++=n;
		*tp++=n; *tp++=n; *tp++=n; *tp++=n;
		*tp++=n; *tp++=n; *tp++=n; *tp++=n;
		*tp++=n; *tp++=n; *tp++=n; *tp++=n;
		*tp++=n; *tp++=n; *tp++=n; *tp++=n;
		*tp++=n; *tp++=n; *tp++=n; *tp++=n;
		*tp++=n; *tp++=n; *tp++=n; *tp++=n;
	}

	temp0 = n; temp1 = n;
        for (tp=a;tp<b;) {	/* 16 unrolled reads */
		temp0 |= *tp; temp1 &= *tp++; temp0 |= *tp; temp1 &= *tp++;
		temp0 |= *tp; temp1 &= *tp++; temp0 |= *tp; temp1 &= *tp++;
		temp0 |= *tp; temp1 &= *tp++; temp0 |= *tp; temp1 &= *tp++;
		temp0 |= *tp; temp1 &= *tp++; temp0 |= *tp; temp1 &= *tp++;
		temp0 |= *tp; temp1 &= *tp++; temp0 |= *tp; temp1 &= *tp++;
		temp0 |= *tp; temp1 &= *tp++; temp0 |= *tp; temp1 &= *tp++;
		temp0 |= *tp; temp1 &= *tp++; temp0 |= *tp; temp1 &= *tp++;
		temp0 |= *tp; temp1 &= *tp++; temp0 |= *tp; temp1 &= *tp++;
	}
	if ((temp0 != n)||(temp1 != n)) {	/* Check word by word */
		for (tp=a;tp<b;) {
		   temp0 = *tp++;
                   if (temp0 != n) {
                        errs++; report("constant",n,temp0,--tp); tp++;
                   }
		}
	        if (errs==0) {
		   printf("Spurious Read Error. Constant Data Test\n"); 
		   report("constant",0,n,n,a); 
		   errs = 1;
		}
	}
        return(errs);
}

testadr(a1,b1)    /* fill each long with its address, read it back */
uint *a1;
uint *b1;
{
	register uint temp,t2;
	register uint *tq;
	register uint *a,*b;
        int errs=0;

	a = a1; b = b1;
        for (tq=a;tq<b;) {	/* 16 unrolled writes */
		*tq++=(uint)tq; *tq++=(uint)tq; *tq++=(uint)tq; *tq++=(uint)tq;
		*tq++=(uint)tq; *tq++=(uint)tq; *tq++=(uint)tq; *tq++=(uint)tq;
		*tq++=(uint)tq; *tq++=(uint)tq; *tq++=(uint)tq; *tq++=(uint)tq;
		*tq++=(uint)tq; *tq++=(uint)tq; *tq++=(uint)tq; *tq++=(uint)tq;
	}

        for (tq=a;tq<b;) {	/* 8 unrolled reads */
		t2 = (uint)tq; temp = *tq++;
                if (temp != t2) {
                        errs++; report("variable",t2,temp,--tq); tq++;
                }
		t2 = (uint)tq; temp = *tq++;
                if (temp != t2) {
                        errs++; report("variable",t2,temp,--tq); tq++;
                }
		t2 = (uint)tq; temp = *tq++;
                if (temp != t2) {
                        errs++; report("variable",t2,temp,--tq); tq++;
                }
		t2 = (uint)tq; temp = *tq++;
                if (temp != t2) {
                        errs++; report("variable",t2,temp,--tq); tq++;
                }
		t2 = (uint)tq; temp = *tq++;
                if (temp != t2) {
                        errs++; report("variable",t2,temp,--tq); tq++;
                }
		t2 = (uint)tq; temp = *tq++;
                if (temp != t2) {
                        errs++; report("variable",t2,temp,--tq); tq++;
                }
		t2 = (uint)tq; temp = *tq++;
                if (temp != t2) {
                        errs++; report("variable",t2,temp,--tq); tq++;
                }
		t2 = (uint)tq; temp = *tq++;
                if (temp != t2) {
                        errs++; report("variable",t2,temp,--tq); tq++;
                }
	}
        return(errs);
}



testrand(a1,b1)   /* random data test */
  uint *a1;
  uint *b1;
{
        int errs = 0;
        register uint rand;
        register uint *tp,temp;
        register uint *a,*b;

	a = a1; b = b1;
        rand = seed;
        for (tp=a;tp<b;) {	/* 8 unrolled writes */
                rand += (rand<<2) + 17623; *tp++ = (uint)rand;
                rand += (rand<<2) + 17623; *tp++ = (uint)rand;
                rand += (rand<<2) + 17623; *tp++ = (uint)rand;
                rand += (rand<<2) + 17623; *tp++ = (uint)rand;

                rand += (rand<<2) + 17623; *tp++ = (uint)rand;
                rand += (rand<<2) + 17623; *tp++ = (uint)rand;
                rand += (rand<<2) + 17623; *tp++ = (uint)rand;
                rand += (rand<<2) + 17623; *tp++ = (uint)rand;
        }

        rand = seed;
        for (tp=a;tp<b;) { 	/* 4 unrolled reads */
                rand += (rand<<2) + 17623; temp = *tp++;
                if (temp != rand) {
                        errs++; report("random",rand,temp,--tp); tp++;
                }
                rand += (rand<<2) + 17623; temp = *tp++;
                if (temp != rand) {
                        errs++; report("random",rand,temp,--tp); tp++;
                }
                rand += (rand<<2) + 17623; temp = *tp++;
                if (temp != rand) {
                        errs++; report("random",rand,temp,--tp); tp++;
                }
                rand += (rand<<2) + 17623; temp = *tp++;
                if (temp != rand) {
                        errs++; report("random",rand,temp,--tp); tp++;
                }
        }
        seed = rand + 34567;
        return(errs);
}

report(type,expect,got,adr)
char *type;
uint expect,got;
uint *adr;
{
   int log_error();
   char ch;

   /* Write general error message */
   printf("\nError: %s data test; expected %x, got %x at address %x\n",
           type, expect, got, adr);
   if (halt_on_err) {
      printf("   Hit any Character to Continue ");
      ch = getchar(); printf("\n");
   }
   printf("	Read #2: 0x%x, Read #3: 0x%x, Read #4: 0x%x\n",*adr,*adr,*adr);
}

