static char	sccsid[] = "@(#)mem_errs.c 1.1 9/25/86 Copyright Sun Microsystems";

#include <sys/types.h>
#include <machdep.h>
#include <sunromvec.h>
#include <token.h>

#define MAXERRSLOGD 100

extern int	datamode, errmessmode, errmode, toterr, buserrcount;

struct berrlog {
	u_char fberr;
	u_short fsr;
	u_long fpc;
	u_long fvss;
	u_long ffault;
	struct berr_field fberr_field;
} berrlog[MAXERRSLOGD];

struct derrlog {
	char *fname;
	u_long faddr;
	u_long fexp;
	u_long fobs;
} derrlog[MAXERRSLOGD];

int
errhand(tname, taddr, exp, obs)
char *tname;
u_long taddr, exp, obs;
{
	char		c;

	toterr++;
	if (toterr < MAXERRSLOGD) {
		derrlog[toterr].fname = tname;
		derrlog[toterr].faddr = taddr;
		derrlog[toterr].fexp = exp;
		derrlog[toterr].fobs = obs;
	}

	if (errmessmode)
		printf("\n>> %s failed @ 0x%x  exp (0x%x)  obs (0x%x)\n",
			tname, taddr, exp, obs);

/*
	emap(taddr, exp, obs);
 */

	switch (errmode) {
		case 0 : 		/* continue */
			return(0);
			break;
		case 1 :		/* wait */
			for (;;) {
				if ((c = maygetchar()) > 0)
					return (c);
			}
			break;
		case 2 :		/* scopeloop */
			switch (datamode) {
				case 0 :
					bloop(tname, taddr, exp, obs);
					return(c);
					break;
				case 1 :
					wloop(tname, taddr, exp, obs);
					return(c);
					break;
				case 2 :
					lloop(tname, taddr, exp, obs);
					return(c);
					break;
			}
			break;
	}
	return(0);
}

/*
emap(taddr, exp, obs)
u_long taddr, exp, obs;
{
	int		bank, loc, bl;
	u_long		fbits, bmask;

	bank = (taddr & 0xc0000) >> 17;
	if ((taddr & 0x200) == 0) bank++;
	printf(">> failure in row %d", bank);

	loc = 1000 + (((bank >> 1) + 2) * 100) + (bank % 2) * 20;

	fbits = exp ^ obs;

	if (fbits) {
		printf("  :  location ");
		for (bmask = 1, bl = 0; bmask < 0x10000; bmask <<= 1, bl++) {
			if (bl == 8) bl = 10;
			if (fbits & bmask) printf(" U%d", loc + bl);
		}

	}
	printf("\n");
}
 */

disperrlog() {

	int i, loglimit;
	char c;

	if ((toterr == 0) && (buserrcount == 0)) {
		printf("\nNo errors logged!\n");
		return(0);
	} else {
		printf("\nErrors:  %d data errors.", toterr);
		printf("\n         %d bus  errors.\n", buserrcount);
	}

	if (toterr < MAXERRSLOGD)
		loglimit = toterr;
	else
		loglimit = MAXERRSLOGD;

	/* print out the data error log */
	for (i = 1 ; i <= loglimit ; i++) {
		printf("\n>> %s failed @ 0x%x  exp (0x%x)  obs (0x%x)\n",
			derrlog[i].fname, derrlog[i].faddr,
			derrlog[i].fexp, derrlog[i].fobs);

/*
		emap(derrlog[i].faddr, derrlog[i].fexp, derrlog[i].fobs);
 */

		if ((c = maygetchar()) != -1) {
			switch (c) {
				case '\023' :
					c = getchar();
					break;
				default :
					return('q');
			}
		}
	}

	if (buserrcount < MAXERRSLOGD)
		loglimit = buserrcount;
	else
		loglimit = MAXERRSLOGD;

	/* print out the bus error log */
	for (i = 1 ; i <= loglimit ; i++) {

	printf("\nbus error: 0x%x\nsr = 0x%x\tpc = 0x%x\tvss = 0x%x\t@ 0x%x\n",
		berrlog[i].fberr, berrlog[i].fsr, berrlog[i].fpc,
		berrlog[i].fvss, berrlog[i].ffault);

		if (berrlog[i].fberr_field.berr_pagevalid)
			printf("VALID ");
		else
			printf("INVALID ");
		if (berrlog[i].fberr_field.berr_busmaster)
			printf("BUSMASTER ");
		if (berrlog[i].fberr_field.berr_proterr)
			printf("PROTECTION ");
		if (berrlog[i].fberr_field.berr_timeout)
			printf("TIMEOUT ");
		if (berrlog[i].fberr_field.berr_parerru)
			printf("UPPER PARITY ");
		if (berrlog[i].fberr_field.berr_parerrl)
			printf("LOWER PARITY");
		printf("\n");

		if ((c = maygetchar()) != -1) {
			switch (c) {
				case '\023' :
					c = getchar();
					break;
				default :
					return('q');
			}
		}
	}

	return(0);
}

dumb(){
asm("	.globl	_mbuserr");
asm("_mbuserr:");
asm("	subql	#2, sp			| align stack to long words");
asm("	movl	d1, sp@-		| save temp registers");
asm("	movl	d0, sp@-");
asm("	movl	a1, sp@-");
asm("	movl	a0, sp@-");
asm("	jsr	_pbuserror		| call print routine");
asm("	movl	sp@+, a0");
asm("	movl	sp@+, a1");
asm("	movl	sp@+, d0");
asm("	movl	sp@+, d1 		| restore temp registers");
asm("	addql	#2, sp			| realign for rte about to happen");
asm("	rte				| go back");
}

pbuserror(rega0, rega1, regd0, regd1, sr, pc, vss, fault)
u_long		rega0, rega1, regd0, regd1, sr, pc, vss, fault;
{
	berr_t	berr;

	berr.berr_whole = getberrreg();

	buserrcount++;
	if (buserrcount < MAXERRSLOGD) {
		berrlog[buserrcount].fberr = berr.berr_whole & 0xff;
		berrlog[buserrcount].fsr = sr & 0xffff;
		berrlog[buserrcount].fpc = pc;
		berrlog[buserrcount].fvss = vss & 0xffff;
		berrlog[buserrcount].ffault = fault;
		berrlog[buserrcount].fberr_field = berr.berr_field;
	}

	printf("\nbus error: 0x%x\nsr = 0x%x\tpc = 0x%x\tvss = 0x%x\t@ 0x%x\n",
		berr.berr_whole & 0xff, sr & 0xffff, pc, vss & 0xffff, fault);

	if (berr.berr_field.berr_pagevalid)
		printf("VALID ");
	else
		printf("INVALID ");
	if (berr.berr_field.berr_busmaster)
		printf("BUSMASTER ");
	if (berr.berr_field.berr_proterr)
		printf("PROTECTION ");
	if (berr.berr_field.berr_timeout)
		printf("TIMEOUT ");
	if (berr.berr_field.berr_parerru)
		printf("UPPER PARITY ");
	if (berr.berr_field.berr_parerrl)
		printf("LOWER PARITY");
	printf("\n");
}

