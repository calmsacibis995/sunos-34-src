/*
 * @(#)commands.c 2.41.1.1 85/03/14 Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/* Always define it until we finger out what to do with DVMA */
#define FAKES1BOOT

/*
 * commands.c
 *
 * Sun ROM monitor: command interpreter and execution
 */

#include "../h/sunmon.h"
#include "../h/globram.h"
#include "../h/m68vectors.h"
#include "../h/statreg.h"
#include "../h/am9513.h"
#include "../h/s2addrs.h"
#include "../h/s2map.h"
#include "../h/buserr.h"
#include "../h/s2misc.h"
#include "../h/pginit.h"
#include "../h/montrap.h"
#include "../h/s2led.h"
#include "../h/keyboard.h"

extern unsigned char peekchar(), getone();

/*
 * Opcode definitions -
 *	The monitor occasionally stuffs some opcodes into
 *	memory.  These are defined here.
 */
#define	OPC_TRAP1	0x4E41		/* trap 1 */

int trap();		/* Trap handler for trace and bkpt traps */
int bootreset();	/* Call it (no return) to reset and boot */

#ifdef FAKES1BOOT
/*
 * Table of page map entries used to fake up the Sun-1 environment
 * to make it easy to convert programs gradually.
 */
struct pginit fakemapinit[] = {
	/* This is usable for 1M system */
	{(char *)0x0C0000, 0, 		/* Drop out high 256K as MB mem */
		{1, PMP_NONE, MPM_MEMORY, 0, 0, 0}},
	{(char *)0x100000, 1,		/* On-board "multibus" memory */
		{1, PMP_ALL, MPM_MEMORY, 0, 0, 0x180}},
	{(char *)0x140000, 1,		/* Real MB mem from 140000 - 1F0000 */
		{1, PMP_ALL, MPM_BUSMEM, 0, 0, 0x80}},
#ifdef VME
	{(char *)0x1F0000, 1,		/* VME 16 bit space 1f0000 - 200000 */
		{1, PMP_ALL, VPM_VME8, 0, 0, 0xFE0}},
#else VME
	{(char *)0x1F0000, 1,		/* Real MB I/O space 1F0000 - 200000 */
		{1, PMP_ALL, MPM_BUSIO, 0, 0, 0}},
#endif VME
	{(char *)0x200000, 0,		/* Prom space */
		{1, PMP_ALL, MPM_IO, 0, 0, MIOPG_PROM}},
	{(char *)0x200000+BYTESPERPG, PGINITEND,	/* Leave rest alone */
		{1, PMP_NONE, MPM_MEMORY, 0, 0, 0}},
};

/*
 * Second half of fakeup map entries...map 256K of DVMA/Multibus virtual addrs
 * to the same on-board memory where 0x100000 - 0x140000 is mapped.
 * This makes it look like it's mapped to Multibus memory, since Multibus
 * devices can get to it at the corresponding addresses.
 */
struct pginit fakemapinit2[] = {
	{MBMEM_BASE, 1,
		{1, PMP_ALL, MPM_MEMORY, 0, 0, 0x180}},
	{MBMEM_BASE+0x40000, PGINITEND,/* Trailer for table end */
		{1, PMP_NONE, MPM_MEMORY, 0, 0, 0}},
};

#endif FAKES1BOOT


/*
 * Prints and/or modifies a location.  The address and length (2, 4, or 8
 * hex digits) are arguments.  The result is 1 if the location was modified,
 * 2 if it was not, but a cr was typed, and 0 if anything else was typed.
 */
queryval(adr,len,space)
	register int adr;
	int len, space;
{
	register unsigned char c;
	register unsigned long val;
	register int retval;

	while (' ' == (c = peekchar())) getone();

	if (ishex(c) < 0) {
		/* No value was supplied.  Read current value, print it, then
		   see if s/he wants to modify it. */
		printf(": ");
		switch (len) {			/* Get the value from storage */
		default:val = getsb(adr,space); break;
		case 4: val = getsw(adr,space); break;
		case 8: val = getsl(adr,space); break;
		}
		printhex((int)val,len);

		/* Value is printed.  If there's more on the line, and it's
		   hex, use it; else if there's nonhex, return for continuation,
		   else if there's nothing more on the line, quit. */
		if ('\r' != c) {
			getone();
			while (' ' == (c = peekchar())) getone();
			if (ishex(c) >= 0) goto UseIt;
			putchar ('\n');
			if ('\r' == c)	return 0;	/* Done with command */
			else		return 1;	/* Use rest of line */
		}

		printf("? ");	/* No arg supplied; ask for one */
		getline(1);
		c = peekchar();
		/* CR typed here means "don't store, but continue" */
		if (c == '\r')		return 1;	/* cr typed */
		if (ishex(c) < 0)	return 0;/* non-hex, non-cr */
		val = getnum();
		retval = 1;		/* Continue after storing */
	} else {
		/* Value supplied on command line or with prev value --
		   use it, and quit if end of line, or continue if more */
UseIt:
		val = getnum();
		printf (" -> ");
		printhex ((int)val, len);
		putchar ('\n');
		while (' ' == (c = peekchar())) getone();
		if (c == '\r')	retval = 0;	/* Quit after storing */
		else		retval = 1;	/* Continue after storing */
	}

	switch (len) {		/* Put it back in storage */
	default:putsb(adr,space,val); break;
	case 4: putsw(adr,space,val); break;
	case 8: putsl(adr,space,val); break;
	}
	return retval;
}

/* Register names */
char reg_names[][3]= {
	"D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7",
	"A0", "A1", "A2", "A3", "A4", "A5", "A6", "SS",
	"US", "SF", "DF", "VB", "SC", "UC", "SR", "PC",
};

unsigned short k2[] = {19567, 30309, 8313, 28533, 29216, 25455, 30062,
	29810, 31020, 8290, 30068, 8302, 25974, 25970, 8308, 29301, 29556,
	8297, 29811, 8295, 28534, 25970, 28269, 25966, 29742, 2560};

/*
 * Display a register, get new value (if any)
 * If right answer:increment and repeat until last reg
 *
 * Arguments are first reg (&r_d0) and first reg to print.
 */
openreg(rbase, radx)
	long *rbase;
	register long *radx;
{
	register char (*r)[3];

	radx += (getnum()&0xF);	/* only use last hex digit */
	r = reg_names + (radx - rbase);
	for(; r < reg_names+(sizeof reg_names/sizeof *reg_names); r++, radx++) {
		printf(r);	
		if (!queryval ((int)radx, 8, FC_SD)) break;
	}
}

#ifdef BREAKTRAP
/* Sets up a break point in address space <space> */
dobreak(space)
int space;
{
	register short *bp= gp->g_breakaddr;
	register short *bpa;

	if (bp && (getsw(bp,space) != OPC_TRAP1))
	    /* oops? got bashed */
	    bp = 0;

	if (peekchar() == '\r') {
		printhex ( (int)bp, ADDR_LEN);
		printf (" now.\n");
	} else {
		bpa= (short*)getnum();
		if (bp) {
			/* Restore user's old instruction */
			putsw(bp, space, gp->g_breakval);
			/* Restore supervisor's TRAP #1 routine */
			*EVEC_TRAP1 = gp->g_breakvec;
		}
		if (bpa) {
			/* Save new instruction */
			gp->g_breakval = getsw(bpa, space);
			/* Install bkpt atop new instruction */
			putsw(bpa, space, OPC_TRAP1);
			/* Install new TRAP #1 vector in supervisor space */
			gp->g_breakvec = *EVEC_TRAP1;
			*EVEC_TRAP1 = (long)trap;
			printf ("Break %x installed\n", gp->g_breakval);
		}
		gp->g_breakaddr = bpa;

	};
}
#endif BREAKTRAP

/*
 * This mini-monitor does only a few things:
 *	a <dig>		open A regs
 *	b <filename>	bootload file and start it
 *	c [<address>]	continue [at <address>]
 *	d <dig>		open D regs
 *	e <hex number> 	open address (as word)
 *	g [<address>]	start (call) [at <address>]
 *	k		reset stack [k1 maps, k2 hard reset]
 *	l <hex number> 	open address (as long)
 *	m <hex number>	open segment map
 *	o <hex number>	open address (as byte)
 *	p <hex number>	open page map
 *	r	 	open PC,SR,SS,US
 *	t [y|n|c cmd]	trace yes, no, or continuous
 *	u [specs]	use different console device[s]
 *	z		set simple breakpoint
 */
monitor(monintstack)
	struct monintstack monintstack;
	/* Note: names beginning r_ are references to monintstack */
{
	register char c;
	register int goadx;
	int (*calladx)();
	char translationsave;
	register int space;
#if defined(BREAKTRAP) | defined(TRACE)
	char justtooktrace = 0;
#endif

	/*
	 * Default address space for storage-reference commands
	 * depends on whether the interrupted program was in supervisor
	 * or user state.
	 */
	if (r_sr & SR_SUPERVISOR) 
		space = FC_SD;
	else
		space = FC_UD;

	switch(r_vor) {		/* what caused entry into monitor? */

	case EVEC_TRAPE:	/* "Exit to monitor" trap instruc */
		break;

	case EVEC_KCMD:		/* "K1" command */
		set_leds(~L_RUNNING);		/* Set LED's to normal state */
		break;		/* Just reenter monitor */

	case EVEC_BOOTING:
		/* Call boot routine after setting up maps, etc. */
		set_leds(~L_RUNNING);		/* Set LED's to normal state */
BootHere:
#ifdef FAKES1BOOT
		/*
		 * For Sun-2 a normal boot sets up a default "looks like 
		 * Sun-1" environment.  We turn 256K of on-board memory into
		 * Multibus, put the proms in the right places, etc.
		 * The code and tables to do this are all in sunmon.c.
		 */
		{
			setupmap(fakemapinit);
			setupmap(fakemapinit2);
		}
#endif FAKES1BOOT
		goadx = boot (gp->g_lineptr);
		if (goadx > 0) {
			/* Boot was OK, goto loaded code */
			r_pc = goadx;
			goto ContinueTrace;
		}
		break;		/* Then, or else, fall into monitor */

	case EVEC_DOG:
		printf("\nWatchdog reset!\n");
		set_leds(~L_RUNNING);		/* Set LED's to normal state */
		break;

	case EVEC_RESET:
		set_leds(~L_RUNNING);		/* Set LED's to normal state */
#ifdef DIAGLOOP
		/* See if the loopback connector is set up for diags */
		switch (diagloop()) {

		case 2:
			/* Delay to see screen, then reenter diags */
			space = 3000000;	/* Loop a few million times */
			goto print_and_loop;

		case 1:
			/* Delay a little bit then reenter diags. */
			space = 50000;

print_and_loop:
			printf("Test loop due to J1600(1-2)\n");
			while (space-- != 0) ;
			/* Reenter the diagnostics */
			goto K2_command;
		
		default:
			/* Don't do diagnostic loopback */
			;
		}
#endif DIAGLOOP

#ifdef AUTOBOOT
		printf ("Auto-boot in progress...\n");
		gp->g_lineptr = gp->g_linebuf;	/* Empty argument to boot cmd */
		gp->g_linebuf[0] = 0;
		goto BootHere;		/* Go do the boot */
#else  AUTOBOOT
		/* just enter monitor */
		break;
#endif AUTOBOOT

	case EVEC_ABORT:
		printf("\nAbort");
		goto PCPrint;

#ifdef	BREAKTRAP
	case EVEC_TRAP1:
		r_pc -= 2;		/* back up to broken word */
	BreakPrint:
		printf("\nBreak ");
		printhex(gp->g_breakval, 4);   /* Print opcode */
		justtooktrace = 1;	/* Allow quick resume with CR */
		goto PCPrint;
#endif	BREAKTRAP

	case EVEC_TRACE:
#ifdef	BREAKTRAP
		if (gp->g_breaking) {
		    /* this is single step past	broken instruction */
		    if (getsw(gp->g_breakaddr, space) == gp->g_breakval) {
		    	/* good - not bashed since we set it, so reset it */
			putsw(gp->g_breakaddr, space, OPC_TRAP1);
		    }
		    if (gp->g_breaking == 1) {
			/* Trace was not previously enabled.  Cancel it and
			   resume the breakpointed program. */
			r_sr &= ~SR_TRACE;
			gp->g_breaking = 0;
			*EVEC_TRACE = gp->g_breaktrvec; /* Fix sup trc vec */
			return;	/* return to instr after bkpt */
		    }
		    /* Trace was previously enabled.  Take trace trap now. */
		    gp->g_breaking = 0;
		}
		/*
		 * OK, we got a trace trap, but not because we just executed
		 * the instruction at a breakpoint (and are tracing to put
		 * back the breakpoint there).
		 *
		 * If a breakpoint it set at the next instruction, pretend
		 * that's why we got entered.  (Bkpt at loc x takes precedence
		 * over trace at loc x.)
		 */
		if ((long)gp->g_breakaddr == r_pc) goto BreakPrint;
#endif	BREAKTRAP
#ifdef  TRACE
		r_sr |= SR_TRACE;	/* Assume we wanna keep tracing. */
		/* This causes MOVE-to-SR, RTE, etc, to NOT stop the trace. */
		/* Only a mon command will turn the trace bit off, once set */
		justtooktrace = 1;
		printf ("Trace ");
		printhex (*(short *)r_pc, 4);
		goto PCPrint;
#else   TRACE
		goto DeFault;
#endif  TRACE

	case EVEC_BUSERR:
		if (!gp->g_because.be_valid) printf ("Invalid Page ");
		else if (gp->g_because.be_proterr)  printf ("Protection ");
		if (gp->g_because.be_vmebuserr) printf ("I/O ");
		if (gp->g_because.be_timeout)  printf ("Timeout ");
		if (gp->g_because.be_parerr_u) {
			if (!gp->g_because.be_parerr_l) {printf ("Upper Byte ");}
ParErr:			printf("Parity ");
		} else if (gp->g_because.be_parerr_l) {
			printf ("Lower Byte ");
			goto ParErr;
		}
		printf("Bus ");		/* Ensure we note it's a bus err */
		goto AccAddPrint;

	case EVEC_ADDERR:
		printf("Address ");
AccAddPrint:	printf("Error, addr: ");
		printhex(gp->g_bestack.AOB,8);
PCPrint:	printf(" at ");
		printhex( (int)r_pc, ADDR_LEN);
		putchar('\n');
		break;
		
	default: 
	DeFault:
		printf("\nException %x", r_vor);
		goto PCPrint;
	}

	r_highsr = 0;		/* clear extraneous high word */

	translationsave = gp->g_translation;
	gp->g_translation = TR_ASCII;

#ifdef TRACE
	if (gp->g_tracecmds) {
		/* Auto-execute a command after a trace trap if no keyin */
		if (r_vor != (short)EVEC_TRACE || mayget() >= 0) {
			gp->g_tracecmds = 0;
		} else {
			gp->g_lineptr = gp->g_tracecmds;
			goto TraceCont;
		}
	}
#endif TRACE

	for (;;) {
#ifdef TRACE
		/* While running a continuous trace, execute sequential
		   commands in turn without calling getline().  When we
		   see a command followed by a null, that's the end of
		   the string.  There, if no key has been pressed, we 
		   resume the user program.  If a key has been pressed,
		   we call getline() to get back to the keyboard. */
		if (gp->g_tracecmds) {
			gp->g_lineptr++;
			if (*gp->g_lineptr) goto TraceCont;
			if (mayget() < 0) goto ContinueTrace;
		}
#endif TRACE

		/* Else get a line of input */
		putchar('>');
		getline(1);

#if defined(BREAKTRAP) | defined(TRACE)
		if (justtooktrace && peekchar() == '\r') goto ContinueTrace;

		justtooktrace = 0;
#endif

#ifdef TRACE
TraceCont:
#endif TRACE
		while(c= getone()) 		/* Skip control chars */
		    if (c >= '0') break;

		while (peekchar() == ' ')
		    getone();	/* remove any spaces before argument */

		if (c == '\0') continue;

		switch(c&UPCASE) {	/* slight hack, force upper case */

		case 'A':	/* open A register */
			openreg(&r_d0, &r_a0);
			goto setcontexts;	/* Might have modified UC/SC */

		case 'B':	/* Bootload from net, disk, or whatever */
			gp->g_linebuf[gp->g_linesize] = 0;
			/* If first char is not '!', reset system too */
			if ('!' == peekchar()) {
				getone();	/* Skip the - */
#ifdef FAKES1BOOT
	/*
	 * For Sun-2 a normal boot sets up a default "looks like 
	 * Sun-1" environment.  We turn 256K of on-board memory into
	 * Multibus, put the proms in the right places, etc.
	 * The code and tables to do this are all in sunmon.c.
	 *
	 * FIXME: this code should not be replicated twice, but
	 * should exist in one place somewhere.  JCG 24Feb83
	 */
	{
		setupmap(fakemapinit);
		setupmap(fakemapinit2);
	}
#endif FAKES1BOOT
				r_pc = boot (gp->g_lineptr);
			} else {
				bootreset();
				/*NOTREACHED*/
			}
			break;

		case 'C':	/* Continue */
			if(ishex(peekchar()) >= 0) r_pc = getnum();
		ContinueTrace:
#ifdef BREAKTRAP
			if ( (long)gp->g_breakaddr == r_pc) {
			    /* Continuing directly to bkpt, do real instruc. */
			    /* Restore the old instruction, save current trace
			       state, and enable trace.  When we take the trace
			       trap, we'll reinstall the breakpoint. */
			    putsw(gp->g_breakaddr, space, gp->g_breakval);
			    gp->g_breaking = 1 + (r_sr & SR_TRACE);
			    gp->g_breaktrvec = *EVEC_TRACE;
			    *EVEC_TRACE = (long)trap;
			    r_sr |= SR_TRACE;
			}
#endif BREAKTRAP
			gp->g_translation = translationsave;
			return;

		case 'D':	/* open D register */
			openreg(&r_d0, &r_d0);
			goto setcontexts;	/* Might have modified UC/SC */

		case 'E':	/* open memory (short word) */
			for(goadx = getnum()&(ADRSPC_SIZE-2); ;goadx+=2) {
				printhex(goadx, ADDR_LEN);
				if (!queryval(goadx,4,space)) break;
			}
			break;

		case 'G':	/* Go */
			if (ishex (peekchar()) >= 0)
				*((long*)&calladx) = getnum();
			else
				*((long*)&calladx) = r_pc;
			gp->g_linebuf[gp->g_linesize] = 0;
			while (peekchar() == ' ') getone(); /* skip blanks */
			calladx(gp->g_lineptr);
			break;

#ifdef HELPFUL
		case 'H':	/* Help */
			givehelp();
			break;
#endif HELPFUL

		case 'K':	/* reset monitor */
			switch (getnum()) {

			case 0:		/* K0 - Reset instruction only */
				resetinstr(); 
				/*
				 * Reinitialize the video output if the board
				 * exists.  (The "reset" instruction we issued
				 * caused a Multibus reset, which
				 * turned off video output from the board.)
				 */
				if (gp->g_fbthere) {
					finit (ax, ay);
				}
				break;

			case 1:		/* K1 - Reset 'most everything */
				softreset();
				/*NOTREACHED*/

			case 0xB:	/* KB - Print power-up banner only */
				banner();
				break;

			default: 	/* K2 - Reset just like power-up */
				/*
				 * If K2, reset video with via funny chars.
				 * Convince hardreset() that we're poweron by
				 * resetting the timer chip.
				 */
				if (*gp->g_lineptr == 2) printf(k2);
				else {
K2_command:
					TIMER_BASE->clk_cmd = CLK_RESET;
					resetinstr(); 
					hardreset(); /*NOTREACHED*/
				}
			}
			break;			

		case 'L':	/* open memory (long word) */
			for(goadx = getnum()&(ADRSPC_SIZE-2); ;goadx+=4) {
				printhex(goadx, ADDR_LEN);
				if (!queryval(goadx,8,space)) break;
			}
			break;

#ifdef MAPCMDS
		case 'M':	/* open segment map entry */
			if (space >= 4) setusercontext(r_scon);
			else		setusercontext(r_ucon);
			for (goadx = getnum() & (ADRSPC_SIZE-1)
					      & ~(PGSPERSEG*BYTESPERPG-1);
					goadx < ADRSPC_SIZE;
					goadx += PGSPERSEG*BYTESPERPG)  {
				printf("SegMap ");
				printhex(goadx, ADDR_LEN);
				if (!queryval((int)SEGMAPADR(goadx),2,FC_MAP))
					break;
			}
			goto setcontexts;	/* Restore UC */
#endif MAPCMDS

		case 'O':	/* open memory (byte) */
			for(goadx = getnum()&(ADRSPC_SIZE-1); ;goadx++) {
				printhex(goadx, ADDR_LEN);
				if (!queryval(goadx,2,space)) break;
			}
			break;

#ifdef MAPCMDS
		case 'P':	/* set page map */
			if (space >= 4) setusercontext(r_scon);
			else		setusercontext(r_ucon);
			for (goadx = getnum()&(ADRSPC_SIZE-1)&~(BYTESPERPG-1);
					goadx < ADRSPC_SIZE;
					goadx += BYTESPERPG) {
				printf("PageMap ");
				printhex(goadx, ADDR_LEN);
				{ register segnum_t seggy;
					seggy = getsegmap(goadx);
					printf(" [");
					printhex(seggy,2);
					putchar(']');
				}
				if (!queryval(goadx,8,FC_MAP))
					 break;
			}
			goto setcontexts;	/* Restore UC */
#endif MAPCMDS

		case 'R':	/* open registers */
			openreg(&r_d0, &r_ssp);
		setcontexts:
			setusercontext(r_ucon);	/* Set/restore user context */
			setsupcontext(r_scon);	/* and system context */
			break;

		case 'S':
			if (goadx = getnum()) space = goadx&7;
			else {
				printf ("FC%x space\n", space);
			}
			break;

#ifdef TRACE
		case 'T':	/* Trace: ty = on; tcxxx = on w/cmd; else off */
			switch (UPCASE & getone()) {
			case 'C':
				{	/* Turn semis into CR's */
					register unsigned char *foo;
					foo = gp->g_lineptr;
					gp->g_tracecmds = foo;
					while (*foo) {
						if (*foo++ == ';')
							*--foo = '\r';
					}
				}
				goto DoTrace;
			case 'Y':
				gp->g_tracecmds = 0;
			DoTrace:
				printf ("Tracing...\n");
				r_sr |= SR_TRACE;
				gp->g_tracevec = *EVEC_TRACE;
				*EVEC_TRACE = (long)trap;
				goto ContinueTrace;
			default:
				r_sr &= ~SR_TRACE;
				*EVEC_TRACE = gp->g_tracevec;
				printf("Trace Off\n");
			}
			break;
#endif TRACE

		case 'U':	/* Use different console I/O */
			usecmd();
			break;
#ifdef BREAKTRAP
		case 'Z':	/* Zap breakpoint in, 0 removes, none shows */
			dobreak(space);
			break;
#endif BREAKTRAP
		default:
			printf("What?\n");
			break;
		}	/* end of switch */
	}		/* End of loop forever */
}			/* END of procedure monitor() */


/*
 * Subroutine callable via ROM Vector Table which will cause the system
 * to reboot using a specified (or defaulted) argument string, as if it
 * had been typed by the user.
 *
 * Just copy the string to linbuf so it won't get trashed, and call
 * bootreset() which will do all the rest.
 */
boot_me(string)
	char *string;
{

	gp->g_lineptr = gp->g_linebuf;
	while (*gp->g_lineptr++ = *string++) ;
	gp->g_lineptr = gp->g_linebuf;
	bootreset();	/*NOTREACHED*/
}
