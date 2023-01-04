/*
 * @(#)ar.c 1.4 83/09/19 Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Prom Driver for Archive "Intelligent" Streaming Tape
 *
 * Device name is ar (Archive)
 */

#define NSTD	2
int arstd[NSTD] = { 0x200, 0x208 };

/* Debug flag for armachine() state changes */
/* #define PRF */

/* Debugging flag traces all writes to control reg */
#define DEBWRITE 0

/*
 * Globular definitions and such
 */
typedef unsigned char u_char;
#define Usec13	13		/* loop count for 13 microsec */
#define Usec20	20		/* Ditto for 20 microsec */

#include "../h/sunmon.h"
#include "../h/arreg.h"
#include "../h/sasun.h"
#include "../h/bootparam.h"
#include "../h/msg.h"

/* Turn off %b printfs from Assert, SetData, and Negate */
#undef DebugTrace
#define DebugTrace ;
#undef DebugData
#define DebugData(foo) ;

/* 
 * States into which the tape drive can get.
 */
enum ARstates {
	FINstate = 0x00, IDLEstate, CMDstate,	/* Finished, Idle, Command  */
	WFMinit,				/* Write File Mark */
	RFMinit,				/* Read to File Mark */
	REWinit,				/* Rewind tape */
	TENSEinit,				/* Retension tape */
	ERASEinit,				/* Erase tape */
	SELinit,				/* Select a drive */
	DESELinit,				/* Deselect all drives */
	RDSTinit,				/* Read status */
	CLOSEinit,				/* Deassert ONLINE */
	READinit, READcmd, READburst, READfin, READidle,	/* Read */
	WRinit, WRcmd,   WRburst,   WRfin,   WRidle,		/* Write */
};

/*
 * Software state per tape controller.
 */
struct _softc {
	enum ARstates sc_state;	/* Current state of hard/software */
	struct arstatus sc_status; /* Status at last "Read status" cmd */
	char *	sc_bufptr;	/* Pointer to buffer to read/write */
	struct ar_regs* sc_regs;/* Address of I/O registers */
};

/*
 * Boot from Archive tape
 */
arboot(bp)
	register struct bootparam *bp;
{
	struct _softc sc[1];	/* The software state structure */
	register struct ar_regs *regs;
	register int tense;
	int ctlr;

	/*
	 * Determine if there exist a device at address <reg>.
	 */
	if ((ctlr = bp->bp_ctlr) < NSTD)
		ctlr = arstd[ctlr];
	regs = (struct ar_regs *)(DEF_MBIO_VA + ctlr);
	if (pokec((char *)(&regs->UNWEDGE), 0)) {
		printf(msg_noctlr, ctlr);
		return (-1);
	}

	if (aropen(sc, regs)) {
		Negate(ONLINE);		/* Turn off light, rewind tape */
		return (-1);		/* Indicate error */
	}

	for (tense=0; tense <= 1; tense++) {
		if (tense) {
			printf("Retensing...\n");
			arcommand(sc, TENSEinit);
		}
		sc->sc_bufptr = (char *)LOADADDR;
		while (!arcommand(sc, READinit))
			sc->sc_bufptr += AR_BSIZE;

		Negate(ONLINE);			/* Rewind tape */

		/* If we saw filemark, and read more than zero blocks, OK. */
		if (sc->sc_status.FileMark && sc->sc_bufptr > (char *)LOADADDR) 
			return(LOADADDR);
	}
	return (-1);
}

/*
 * Open an Archive tape drive for standalone use.
 */
aropen(sc, regs)
	register struct _softc *sc;
	register struct ar_regs *regs;
{
	register int count;

	/*
	 * Initialize the controller
	 */

#define SPININIT 1000000

	sc->sc_state = IDLEstate;
	sc->sc_bufptr = (char *) 0x1ff001;	/* very funny buff addr */
	sc->sc_regs = regs;
/* When adding new fields to softc, be sure to initialize them here. */

	UnWedge;		/* Take it out of burst mode wedge */
	count = 0;				/* Clear reg to store */
	((struct ar_regs2 *)regs)->wctrl = count;	/* Clear all the bits */

	count = SPININIT;			/* Wait for tape motion stop */
	while (!Read(READY) && !Read(EXCEPTION)) {
		if (!--count) break;
	}

	/* Tape is ready or exceptional.  Reset it for good measure. */
	Assert(RESET);
	count = Usec13;
	while (--count) ;
	Negate(RESET);

	count = SPININIT;			/* Wait for EXC after reset */
	while (!Read(EXCEPTION)) {
		if (!--count) {
			printf("ar: drive not responding\n");
			return 1;	/* Return error */
		}
	}

	/* Now read back status from the reset. */
	Assert(ONLINE);		/* Must do first so RDST microcode doesn't
				   play games with READY line.  See comments
				   in aropen(). */
	if (arcommand (sc, RDSTinit)) {
ErrorExit:
		Negate(ONLINE);
		return 1;
	}

	if(sc->sc_status.NoDrive) {
 		printf("ar: no drive\n");
		goto ErrorExit;
	} else if (sc->sc_status.NoCart) {
		printf ("ar: no cartridge\n");
		goto ErrorExit;
	}

	/* We're done opening the tape drive.  Return success. */
	return 0;
}


/*
 * Begin execution of a device command for the device pointed to by
 * sc.  The command begins execution in state newstate.
 *
 * This is HARDWARE oriented software.  It doesn't know or care of
 * state of buffers, etc.  Its result reflects what the hardware is
 * doing, not what the software is doing.
 *
 * The device is assumed to be in one of the FIN or IDLE states:
 *	IDLEstate, FINstate, READfin, READidle, WRfin, or WRidle.
 *
 * Our result is:
 *	0	if the operation completed normally
 *	1	if the operation completed with an error
 */
arcommand(sc, newstate)
	register struct _softc *sc;
	register enum ARstates newstate;
{
	register struct ar_regs *regs = sc->sc_regs;

	/*
	 * Continuous Reads and Writes need not go back to READinit
	 * or WRinit; they can just tail-end into the Burst state.
	 */
	if (sc->sc_state == READidle) {
		if (newstate == READinit) newstate = READburst;
	}

	sc->sc_state = newstate;

	for (;;) {
		if (Read(EXCEPTION)) {
			/*
			 * An error has occurred.  Deal with it somehow.
			 * If we are reading status, just do it; else do our own
			 * RDST to find out the problem, and cancel the current
			 * operation.
			 */
			if (sc->sc_state == RDSTinit) goto JustDoIt;
			sc->sc_state = RDSTinit;
			armachine(sc);	/* Read status */
			if (!sc->sc_status.FileMark) {
				printf("ar: %x error\n",
				   *(unsigned short*)&sc->sc_status);
			}
			return 1;	/* Indicate error */
		} else {
			if (Read(READY)) {
JustDoIt:			if (armachine(sc)) return 0; /* if done */
			}
		}
	}
}


/*
 * State machine for archive tape drive controller.
 *
 * This actually accomplishes things w.r.t. the tape drive.
 *
 * Returns 0 if operation still in progress, 1 if finished.
 */

static int
armachine(sc)
register struct _softc *sc;
{
	register struct ar_regs *regs = sc->sc_regs;
	register int count;
	register char *byteptr;

#ifdef PRF
printf ("[%x", sc->sc_state);
#endif

	switch (sc->sc_state) {

	case RFMinit:
		SetData(AR_rdeof);
		Assert(REQUEST);
		goto CmdState;

	case TENSEinit:
		SetData(AR_tension);
		Assert(REQUEST);
		goto CmdState;

	RDSTagain:
	case RDSTinit:
		byteptr = (char *) &sc->sc_status;

		/* We could have either READY or EXCEPTION; remember which */
		count = 0; if (Read(READY)) count = 1;
		SetData(AR_rdstat);
		Assert(REQUEST);

		/*
		 * Now wait for READY indicating command accepted.
		 * Check for Exception, if we started with READY.  
		 * (It's not legal to do RDST all the time (!).)
		 */
		while (!Read(READY)) {
			if (count) if (Read(EXCEPTION)) goto RDSTagain;
		}

		/* Negate REQUEST, wait for READY to drop. */
		Negate(CATCHREADY); Assert(CATCHREADY);
		Negate(REQUEST);

		/* Now xfer a byte or six. */
		do {
			/* Wait for edge of READY */
			while (!Read(EDGEREADY)) {
				if (Read(EXCEPTION)) goto RDSTagain;
			}
			*byteptr++ = GetData();
			Negate(CATCHREADY); Assert(CATCHREADY);
			Assert(REQUEST);	/* Tell controller we have it */
			/* Ready will fall within 250ns of our REQUEST, but
			   we're supposed to keep it high for 20us */
			count = Usec20;
			while (--count) ;
			Negate (REQUEST);
		} while (byteptr <
			(char *)(&sc->sc_status) + sizeof(sc->sc_status));

		while (!Read(READY)) if (Read(EXCEPTION)) goto RDSTagain;
		goto IdleState;

	case READinit:
		SetData(AR_rddata);
		Assert(REQUEST);
		goto NextState;

	case READcmd:
		Negate(REQUEST);
		while (Read(READY)) ;	/* Wait til READY drops */
		goto NextState;

	case READburst:
		/* Read a block of data from the tape drive. */
		Assert(BURST);			/* Begin block xfer */
		count = 513;
		byteptr = sc->sc_bufptr;
		while (--count) *byteptr++ = GetData(); /* read data */
		Negate(BURST);			/* Eliminate burst mode */
		goto NextState;

	case CMDstate:
		/* All commands that stop interacting once you say "do it" */
		Negate(REQUEST);	/* Done with command */
		while (Read(READY)) ;	/* Wait til ready drops */
		sc->sc_state = FINstate;
		goto DisAble;		/* Return "Done" before final int */

	IdleState:		/* Drive is idle; set IDLEstate and disable */
		sc->sc_state = IDLEstate;
		goto DisAble;

	case READfin:		/* Entry after reading a block */
	case FINstate:		/* Entry after any other command */
		/*
		 * Go to next sequential state - WRidle, READidle, IDLEstate.
		 * Disable interrupts, and return.  arstart_cmd() will later
		 * put us into READ/WRburst or some commandinit state.
		 */
		sc->sc_state = (enum ARstates) (1 + (int)sc->sc_state);
DisAble:
#ifdef PRF
printf ("=>%x]", sc->sc_state);
#endif
		return 1;			/* Tell caller op is done */

	case READidle:		/* Reading blocks, but don't need one now */
	case IDLEstate:		/* Issuing commands, but don't have one now */
		goto DisAble;	/* Turn off interrupt enable again */

	NextState:
		/* Go to next sequential state */
		sc->sc_state = (enum ARstates) (1 + (int)sc->sc_state);
		/* I wish somebody would fix the fucking language defn here
		   so this messing about wouldn't be needed. */
		break;

	CmdState:
		sc->sc_state = CMDstate;
		break;

	}

#ifdef PRF
printf ("->%x]", sc->sc_state);
#endif
	return 0;
}
