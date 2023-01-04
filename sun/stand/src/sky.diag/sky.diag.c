#ifndef lint
static	char sccsid[] = "@(#)sky.diag.c 1.27 85/05/09 Copyright Sun Microsystems";
#endif

/*
 * ****************************************************************************
 * Program Name    : sky.diag	Stand-Alone Sky Board Diagnostic
 * Usage           : bsd()/stand/sky.diag [-t] [passcount]
 * Option(s)       : -t  Generate a functional trace.
 * Source File     : venus:/usr/standdev/sky.diag.c
 * Original Engr   : Sunny
 * Date            : 05/09/85
 * Function        : sky.diag tests the Multibus or VMEbus Sky board
 * Revision #1 Engr: 
 * Date            : 
 * Change(s)       :
 * ****************************************************************************
 */

#include <sys/types.h>
#include <machdep.h>
#include <sundev/skyreg.h>
#include "../../include/exception.h"
#include <sunromvec.h>

#define NULL		0
#define FALSE		0
#define TRUE		!FALSE
#define SKYADDR		(struct skyreg *)(PTOB(BTOP(&end + 0x1000 + PAGEMASK)))
#define UADDRSTART	0x1000		/* to avoid address/ucode pairs */
#define	S_ADD		0x1001
#define	S_SUB		0x1007
#define	S_MUL		0x100b
#define	S_DIV		0x1013
#define	S_LOG		0x102d

extern		end;		/* end+ 0x1000 is stack in SA environment */
extern u_long	skyucode[], skyucodesize;

int trace=FALSE;

union	iffi {		/* used to hold and convert operands */
	u_long	i;
	float	f;
} iffi, p1, p2, soft, hard;

u_short	ops[] = { S_ADD, S_SUB, S_MUL, S_DIV };

/*					*/
/*	Sky Terminal Error Handler	*/
/*					*/
skydie(info)
register struct ex_info	*info;
{
	printf("\nsky.diag:	can't find Sky board!  Check configuration.\n");
	(*romp->v_exit_to_mon)();
}

main()
{
	register	npass;
	u_long		i;
	int		buf[80];
         
	printf("\n%s\nStandAlone Sky Fast Floating Point Processor Board Diagnostic ",sccsid);

	exception_handler = skydie;
	ex_vector->e_buserr = ex_handler;		/* grab buserr */

	if ((getidprom(1) & 0xff) == 0x02) {
		printf("(VME-bus)\n");
		map(SKYADDR, PAGESIZE, 0x7f8800, PM_BUSIO); /* map in VME */
	} else {

		printf("(Multibus)\n");
		map(SKYADDR, PAGESIZE, 0x2000, PM_BUSIO); /* map in Multibus */
	}

	printf("How many passes? (CR=5; 0=forever):  ");
	gets(buf);
	if (buf[0] == NULL) {
		npass=5;
	} else {
		npass=atoi(buf);
	}

	if(microcram()){			/* load code & test */
		printf("\nsky.diag:	microcode load FAILED\n");
		return(-1);
	} else {
		printf("sky.diag:0\t");
		if (npass==0) {
			for(i=0;;i++){
				if(skytest()){
					printf("\nsky.diag:	FAILED on pass %d\n",i+1);
					break;
				} else {
					printf("\rsky.diag:%d\t",i+1);
				}
			}
		} else {
			for(i=0;i<npass;i++){
				if(skytest()){
					printf("\nsky.diag:	FAILED on pass %d\n",i+1);
					break;
				} else {
					printf("\rsky.diag:%d\t",i+1);
				}
			}
		}
		printf("\rsky.diag:%d passes PASSED                                            \n",i);
	}
}
/*								*/
/*	This routine crams microcode into the Sky board's RAM	*/
/*								*/
int
microcram()
{
	register struct skyreg	*s = SKYADDR;
	register u_long		ucode, uaddr = UADDRSTART;
	register		i;

	/* halt the board, in case its running */

	s->sky_status = SKY_IHALT;
	s->sky_status = SKY_IHALT;

	/* cram microcode at it -- read it back each time to check */

	for (i = 0 ; i < skyucodesize ; i++, uaddr++){

		s->sky_command = uaddr;				/* set addr */
		s->sky_ucode =   skyucode[i];			/* set data */
		if ((ucode = s->sky_ucode) != skyucode[i]){	/* read back */
			printf(
		"\nsky.diag:	ucode verify error +0x%x exp(0x%x) obs(0x%x)\n",
				uaddr, skyucode[i], ucode);
			return(-1);
		}
	}

	/* start the thing up */

	s->sky_status = SKY_RESET;
	s->sky_command = SKY_START0;
	s->sky_command = SKY_START0;
	s->sky_command = SKY_START1;
	s->sky_status  = SKY_RUNENB;
	printf("sky.diag:	loaded %d words of ucode\n", skyucodesize);
	return(0);
}

/*						*/
/*	This routine tests the Sky board	*/
/*						*/
skytest()
{
	register struct skyreg *s = SKYADDR;
	register u_long	sdata, rdata;  
	register i, j;


	/* do a restore/save */

	printf("context restore/save\t");
	srandom(1);
	s->sky_command = SKY_RESTOR;
	for (i=0; i<8; i++)
		s->sky_data = random();

	srandom(1);
	s->sky_command = SKY_SAVE;
	for (i=0; i<8; i++){
		if ((rdata = s->sky_data) != (sdata = random())){
			printf("FAILED\n+0x%x exp(0x%x) obs(0x%x)\n",
				i, sdata, rdata);
			printf("sky.diag:	context restore/save FAILED\n");
			return(-1);
		}
	}

	/* test log(1.0) */
	printf("LOG  ");

	iffi.f = 1.0;		/* set up value for data */

	s->sky_command = S_LOG;
	s->sky_data = iffi.i;

	/* wait for result */

	i = 1000;
	while(--i && !(s->sky_status & SKY_IORDY));
	if (!i){
		printf("timed out on log(1.0) read\n");
		printf("sky.diag:	hung in I/O busy state:  FAILED\n");
		return(-1);
	}

	sdata = s->sky_data;
	if (sdata != 0){
		printf("FAILED on log(1.0) (0x%x)\n", sdata);
		printf("sky.diag:	FAILED on logarithm of 1.0\n");
		return(-1);
	}

	for (i = 0; i < sizeof(ops)/sizeof(*ops) ; i++){
		switch(ops[i]){
			case S_ADD:
				printf("ADD  ");
				break;
			case S_SUB:
				printf("SUB  ");
				break;
			case S_MUL:
				printf("MUL  ");
				break;
			case S_DIV:
				printf("DIV  ");
				break;
			default:
				printf("program error:  unknown opcode %x\n",
				ops[i]);
				break;
		}
		for( j = 0 ; j < 256 ; j++){
			if (j){
				p1.f = (float)random() / (float)random();
				p2.f = (float)random() / (float)random();
			} else {
				p1.f = p2.f = 1.0;
			}
			hard.i = skyop(s, ops[i], p1.i, p2.i);
			switch(ops[i]){
				case S_ADD:
					soft.f = p1.f + p2.f;
					break;
				case S_SUB:
					soft.f = p1.f - p2.f;
					break;
				case S_MUL:
					soft.f = p1.f * p2.f;
					break;
				case S_DIV:
					soft.f = p1.f / p2.f;
					break;
				default:
					printf("program error:  unknown opcode %x\n", ops[i]);
					break;
			}
			if (soft.f != hard.f) {
				printf("op = 0x%x FAILED\n", ops[i]);
				printf(
"sky.diag:	argument 1\t argument 2\t s software\t h hardware\n");
				printf(
"sky.diag:	0x%x\t 0x%x\t s 0x%x\t h 0x%x\n", p1.i, p2.i, soft.i, hard.i);
				printf(
"sky.diag:	FAILED on single precision arithmetic\n");
				return(-1);
			}
		}
	}
	return(0);
}

/*								*/
/*	This routine performs a math operation via Sky board	*/
/*								*/
skyop(s, opcode, a, b)
register struct skyreg	*s;
register u_short	opcode;
register u_long		a, b;		/* really are floats though */
{
	register	i = 1000;

	s->sky_command = opcode;
	s->sky_data = a;
	s->sky_data = b;
	if (opcode == S_DIV){
		while(--i && !(s->sky_status & SKY_IORDY));
		if (!i){
			printf("\nsky.diag:	skyop timed out on 0x%x opcode\n",
				opcode);
			return(-1);
		}
	}
	return(s->sky_data);
}

