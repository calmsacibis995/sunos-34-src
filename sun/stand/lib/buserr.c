#include <sys/types.h>
#include <machdep.h>
#include <setjmp.h>

u_long		last_fault = 0xffffffff;
berr_size	last_buserror = 0;
u_short		buserrcount = 0;

jmp_buf		buf_buserr;
int		use_buserrbuf = 0;

static char	sccsid[] = "@(#)buserr.c 1.1 9/25/86 Copyright Sun Micro";

dummy(){

asm("	.globl	_buserr");
asm("_buserr:");
asm("	subql	#2, sp			| align stack to long words");
asm("	movl	d1, sp@-		| save temp registers");
asm("	movl	d0, sp@-");
asm("	movl	a1, sp@-");
asm("	movl	a0, sp@-");
asm("	tstl	_use_buserrbuf		| check for buf jump");
asm("	beqs	nojump			| dont jump, print and rte");
asm("	movl	sp,sp@-			| give pointer to data acquired");
asm("	movl	#_buf_buserr, sp@-	| give jmp_buf");
asm("	jsr	__longjmp		| do _longjmp(buf_buserr, &businfo)");
asm("	addl	#8, sp			| realign stack");
asm("nojump:");
asm("	jsr	_printbuserror		| call print routine");
asm("	movl	sp@+, a0");
asm("	movl	sp@+, a1");
asm("	movl	sp@+, d0");
asm("	movl	sp@+, d1 		| restore temp registers");
asm("	addql	#2, sp			| realign for rte about to happen");
asm("	rte				| go back");
}

printbuserror(rega0, rega1, regd0, regd1, sr, pc, vss, fault)
u_long		rega0, rega1, regd0, regd1, sr, pc, vss, fault;
{
	berr_t	berr;

	berr.berr_whole = getberrreg();
	if ((0xff & berr.berr_whole) == last_buserror &&
		fault == last_fault && ++buserrcount){
		return;
	} else {
		last_fault = fault;
		last_buserror = 0xff & berr.berr_whole;
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
