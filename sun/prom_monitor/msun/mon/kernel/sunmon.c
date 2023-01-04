/*
 * @(#)sunmon.c 2.63.1.1 85/02/19 Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * reset.c
 *
 * Brings the system from its knees (set up by assembler code at power-on)
 * to its feet.
 */

#include "../h/sunmon.h"
#include "../h/s2map.h"
#include "../h/s2addrs.h"
#include "../h/s2misc.h"
#include "../h/enable.h"
#include "../h/globram.h"
#include "../h/zsreg.h"
#include "../h/suntimer.h"
#include "../h/am9513.h"
#include "../h/keyboard.h"
#include "../h/sunromvec.h"
#include "../h/m68vectors.h"
#include "../h/framebuf.h"
#include "../h/pginit.h"
#include "../h/montrap.h"
#include "../h/dpy.h"
#include "../h/video.h"
#include "../h/s2led.h"
#include "structconst.h"

int dogreset();
void reset_uart();

/*
 * Table of map entries used to map video back on a watchdog.
 * This is because video might have been running in copy mode 'cuz of
 * Unix.  We take it back since we don't turn on copy mode in the
 * video control reg.
 */
struct pginit videoinit[] = {
	{VIDEOMEM_BASE, 1,		/* Video memory (if we have any) */
		{1, PMP_SUP_READ|PMP_SUP_WRITE|PMP_USER_READ|PMP_USER_WRITE,
#ifdef VME
				 VPM_IO, 0, 0, VIOPG_VIDEO}},
#else  VME
				 MPM_MEMORY, 0, 0, MEMPG_VIDEO}},
#endif VME
	{(char *)TIMER_BASE, PGINITEND,	/* Trailer for table end */
		{1, PMP_NONE, MPM_MEMORY, 0, 0, 0}},
};


/*
 * Table of page map entries used to map on-board I/O and PROM pages.
 */
struct pginit mapinit[] = {
#ifdef VME
	/*
	 * Initialization table for VME system
	 */
	{MAINMEM_BASE, 1,		/* 6 Megs of main memory space */
		{1, PMP_ALL, VPM_MEMORY, 0, 0, 0}},

	{HOLE_BASE, 0,			/* Hole mapped via Invalid Pmeg */
		{1, PMP_NONE, VPM_MEMORY, 0, 0, 0}},
	/* UNUSED_BASE is also initialized here. */

	{BUSIO_BASE, 1,			/* VME bus I/O space */
		{1, PMP_SUP_READ|PMP_SUP_WRITE|PMP_USER_READ|PMP_USER_WRITE,
			VPM_VME8, 0, 0, VMEPG_IO}},

	{VIDEOMEM_BASE, 1,		/* Video memory (if we have any) */
		{1, PMP_SUP_READ|PMP_SUP_WRITE|PMP_USER_READ|PMP_USER_WRITE,
				 VPM_IO, 0, 0, VIOPG_VIDEO}},

	{(char *)TIMER_BASE, 0,
		{1, PMP_SUP_READ|PMP_SUP_WRITE|PMP_USER_READ|PMP_USER_WRITE,
				VPM_IO, 0, 0, VIOPG_TIMER}},

	{(char *)ROP_BASE, 0,		/* No ROPC on VME system */
		{1, PMP_NONE, VPM_MEMORY, 0, 0, 0}},

	{(char *)CLOCK_BASE, 0,
		{1, PMP_SUP_READ|PMP_SUP_WRITE|PMP_USER_READ|PMP_USER_WRITE,
				VPM_VME0, 0, 0, 0x200800>>BYTES_PG_SHIFT}},

	{(char *)DES_BASE, 0,
		{1, PMP_SUP_READ|PMP_SUP_WRITE|PMP_USER_READ|PMP_USER_WRITE,
				VPM_IO, 0, 0, VIOPG_DES}},

	{(char *)PARALLEL_BASE, 0,		/* No parallel port on VME */
		{1, PMP_NONE, VPM_MEMORY, 0, 0, 0}},

	{(char *)SCSI_BASE, 0,
		{1, PMP_SUP_READ|PMP_SUP_WRITE|PMP_USER_READ|PMP_USER_WRITE,
				VPM_VME0, 0, 0, 0x200000>>BYTES_PG_SHIFT}},

	{(char *)ETHER_BASE, 0,
		{1, PMP_SUP_READ|PMP_SUP_WRITE|PMP_USER_READ|PMP_USER_WRITE,
				VPM_IO, 0, 0, VIOPG_ETHER}},

	{(char *)VIDEOCTL_BASE, 0,
		{1, PMP_SUP_READ|PMP_SUP_WRITE|PMP_USER_READ|PMP_USER_WRITE,
				VPM_IO, 0, 0, VIOPG_VIDEOCTL}},

	{LAST_IO_BASE, 0,		/* Unused are up to serial ports */
		{1, PMP_NONE, VPM_MEMORY, 0, 0, 0}},

	{(char *)KEYBMOUSE_BASE, 0,
		{1, PMP_SUP_READ|PMP_SUP_WRITE|PMP_USER_READ|PMP_USER_WRITE,
				VPM_IO, 0, 0, VIOPG_KBM}},

	{(char *)SERIAL0_BASE, 0,
		{1, PMP_SUP_READ|PMP_SUP_WRITE|PMP_USER_READ|PMP_USER_WRITE,
				VPM_IO, 0, 0, VIOPG_SERIAL0}},

	{BYTESPERPG+(char *)SERIAL0_BASE, 0,	/* Unused serial port space */
		{1, PMP_NONE, VPM_MEMORY, 0, 0, 0}},

	{(char *)PROM_BASE, 0,		/* Prom space */
		{1, PMP_ALL, VPM_IO, 0, 0, VIOPG_PROM}},

	{MBMEM_BASE, 1,			/* One Meg of VME bus Memory space */
		{1, PMP_ALL, VPM_VME0, 0, 0, 0}},

	{(char *)ADRSPC_SIZE, PGINITEND,/* Trailer for table end */
		{1, PMP_NONE, MPM_MEMORY, 0, 0, 0}},

#else  VME

	/*
	 * Initialization table for Multibus system
	 */
	{MAINMEM_BASE, 1,		/* 6 Megs of main memory space */
		{1, PMP_ALL, MPM_MEMORY, 0, 0, 0}},

	{HOLE_BASE, 0,			/* Hole mapped via Invalid Pmeg */
		{1, PMP_NONE, MPM_MEMORY, 0, 0, 0}},
	/* UNUSED_BASE is also initialized here. */

	{BUSIO_BASE, 1,			/* Multibus I/O space */
		{1, PMP_SUP_READ|PMP_SUP_WRITE, MPM_BUSIO, 0, 0, 0}},

	{VIDEOMEM_BASE, 1,		/* Video memory (if we have any) */
		{1, PMP_SUP_READ|PMP_SUP_WRITE|PMP_USER_READ|PMP_USER_WRITE,
				 MPM_MEMORY, 0, 0, MEMPG_VIDEO}},

	{(char *)TIMER_BASE, 0,
		{1, PMP_SUP_READ|PMP_SUP_WRITE|PMP_USER_READ|PMP_USER_WRITE,
				MPM_IO, 0, 0, MIOPG_TIMER}},

	{(char *)ROP_BASE, 0,
		{1, PMP_SUP_READ|PMP_SUP_WRITE|PMP_USER_READ|PMP_USER_WRITE,
				MPM_IO, 0, 0, MIOPG_ROP}},

	{(char *)CLOCK_BASE, 0,
		{1, PMP_SUP_READ|PMP_SUP_WRITE|PMP_USER_READ|PMP_USER_WRITE,
				MPM_IO, 0, 0, MIOPG_CLOCK}},

	{(char *)DES_BASE, 0,
		{1, PMP_SUP_READ|PMP_SUP_WRITE|PMP_USER_READ|PMP_USER_WRITE,
				MPM_IO, 0, 0, MIOPG_DES}},

	{(char *)PARALLEL_BASE, 0,
		{1, PMP_SUP_READ|PMP_SUP_WRITE|PMP_USER_READ|PMP_USER_WRITE,
				MPM_IO, 0, 0, MIOPG_PARALLEL}},

	{(char *)SCSI_BASE, 0,
		{1, PMP_SUP_READ|PMP_SUP_WRITE|PMP_USER_READ|PMP_USER_WRITE,
				MPM_BUSMEM, 0, 0,
				/*FIXME MBMEM_SCSI*/ 0x80000>>BYTES_PG_SHIFT}},

	{(char *)ETHER_BASE, 0,		/* No Ethernet on Multibus */
		{1, PMP_NONE, MPM_MEMORY, 0, 0, 0}},

	{(char *)VIDEOCTL_BASE, 0,
		{1, PMP_SUP_READ|PMP_SUP_WRITE|PMP_USER_READ|PMP_USER_WRITE,
				MPM_MEMORY, 0, 0, MEMPG_VIDEO_CTRL}},

	{LAST_IO_BASE, 0,		/* Unused are up to serial ports */
		{1, PMP_NONE, MPM_MEMORY, 0, 0, 0}},

	{(char *)KEYBMOUSE_BASE, 0,
		{1, PMP_SUP_READ|PMP_SUP_WRITE|PMP_USER_READ|PMP_USER_WRITE,
				MPM_MEMORY, 0, 0, MEMPG_VIDEO_ZSCC}},

	{(char *)SERIAL0_BASE, 0,
		{1, PMP_SUP_READ|PMP_SUP_WRITE|PMP_USER_READ|PMP_USER_WRITE,
				MPM_IO, 0, 0, MIOPG_SERIAL0}},

	{BYTESPERPG+(char *)SERIAL0_BASE, 0,	/* Unused serial port space */
		{1, PMP_NONE, MPM_MEMORY, 0, 0, 0}},

	/*
	 * Note: PROM space must be writeable, because if there is no
	 * frame buffer, we set g_keybzscc to point to a fake UART in
	 * PROM.  Writes to it are ignored and it always returns wired
	 * status.  This avoids the parity bus errors that result from
	 * trying to touch the real one (like in the NMI routine).
	 */
	{(char *)PROM_BASE, 0,		/* Prom space */
		{1, PMP_ALL, MPM_IO, 0, 0, MIOPG_PROM}},

	{MBMEM_BASE, 1,			/* One Meg of Multibus Memory space */
		{1, PMP_ALL, MPM_BUSMEM, 0, 0, 0}},

	{(char *)ADRSPC_SIZE, PGINITEND,/* Trailer for table end */
		{1, PMP_NONE, MPM_MEMORY, 0, 0, 0}},
#endif VME
};



#ifndef VME
/*
 * The following struct is used to fake out keyboard code if no Sun-2
 * frame buffer exists.  G_keybzscc is set to point to here, which always
 * "returns" zero in the status bits, and ignores any writes.  If we let
 * the NMI routine or other places really touch the location, we'd have
 * to trap bus errors in the NMI routine, a headache I'm not ready for.
 */
struct zscc_device fakeuart[2] = { {0, 0}, {0, 0}};
#endif VME


/*
 * monreset()
 *
 * Entered from assembler code (trap.s) on all resets, just before we
 * call the command interpreter.  We get to set up all of the I/O
 * devices and most of the rest of the software world.  We are still
 * in boot state.  Interrupts cannot occur until we enable them,
 * but we can still get bus/address errors, which fetch their vectors from
 * (as yet uninitialized) mappable memory (not prom).
 *
 * For power-on resets, we are in context 0.  Enough segments are mapped
 * straight-thru to hold all of main memory.  Enough pmegs are mapped
 * (starting with zero) to hold all good pages of main memory, as determined
 * by the power-up diagnostics.  Memory has been initialized to F's.
 *
 * For other resets, FIXME: maps?  Memory is untouched.
 *
 * The argument is an interrupt stack, the same one passed to monitor().
 * Like the one passed to monitor(), we can modify it and this changes
 * external reality (eg, the PC or registers which will be set up for
 * the user program).
 *
 * There are four kinds of resets possible:
 *	EVEC_RESET	Power-on reset (or equivalently, K2 command)
 *	EVEC_KCMD	K1 command from the keyboard
 *	EVEC_BOOTING	B command from the keyboard
 *	EVEC_DOG	Watchdog reset after CPU double bus fault
 *
 * The Dog is the scariest reset, since we want to disturb as little info
 * as possible (for debugging), but also want to bring the system back
 * under control.  The RESET is where we have to be thorough; bad boards
 * will often make it this far and we have to give good diagnosis of
 * what we find.  The other two are similar to each other and relatively easy.
 */

monreset(monintstack)
	struct monintstack monintstack;
{
	register char *p;
	register int time;
	int i;
	long bbuf[4];		/* Buffer for bus error recovery */

	if (r_vor == (unsigned)EVEC_DOG) {
		/*
		 * Since we assume maps are OK, get out of boot state and
		 * enable DVMA.
		 */
		gp->g_enable.ena_notboot = 1;
		gp->g_enable.ena_dvma    = 1;
		set_enable(gp->g_enable);
		goto DogSkip0;
	}

/*============================================================================
 * Following code is common to RESET, KCMD, and BOOTING resets.
 *============================================================================*/

	/*
	 * Set up the initial memory map.
	 *
	 * We have to do this even for power-up reset because diags
	 * map all the pmegs contiguously.  We need some at the top end.
	 * Fix in diags, later.  FIXME.
	 */
	set_leds(~L_SETUP_MAP);	/* I'll tell the world */
	r_scon = 0;			/* Initialize context regs */
	r_ucon = 0;
	mapmem();

	/*
	 * Make all interrupt vectors in low RAM.
	 */
	{
		extern trap(), bus_error(), addr_error(), nmi();

		register long *et;

		for (et = EVEC_AFTER; et ;) *--et = (long) trap;

		*et++ = (long) gp;		/* 0 = ptr to Globals area */
		*et++ = (long) romp;		/* 4 = ptr to Transfer Vector */

		*et++ = (long) bus_error;	/* 8 = bus error routine */
		*et++ = (long) addr_error;	/* C = address error routine */
		*EVEC_LEVEL7 = (long) nmi;	/* Non-maskable interrupts */
	}

	/*
	 * Invalidate nonexistent RAM pages.
	 */
	for (p = (char *)gp->g_memorysize;
		p < MAINMEM_BASE + MAINMEM_SIZE;
		p += BYTESPERPG) {
		setpgmap(p, PME_INVALID);
	}

	if (r_vor != (unsigned)EVEC_RESET)
		goto DogSkip0;

/*============================================================================
 * Following code is specific to RESET resets.
 *============================================================================*/

	/*
	 * Initialize the UARTs!
	 */
#ifdef FIXME
	gp->g_inzscc = SERIAL0_BASE+1;
	gp->g_outzscc = SERIAL0_BASE+1;
#else  FIXME
	/* Temporary hack: just put uart base there rather than device addr */
	gp->g_inzscc = SERIAL0_BASE;
	gp->g_outzscc = SERIAL0_BASE;
#endif FIXME

	/*
	 * Write all the assorted initialization commands to both
	 * halves of the UART chip.
	 */
	reset_uart(&SERIAL0_BASE[0].zscc_control, 1);
	reset_uart(&SERIAL0_BASE[1].zscc_control, 0);

/*============================================================================
 * Following code is common to all resets.
 *============================================================================*/

DogSkip0:

#ifdef	FRAMEBUF
	/*
	 * Determine presence of frame buffer.
	 */
	set_leds(~L_SETUP_FB);			/* Tell the world */

#ifdef S2FRAMEBUF
	GXBase = (int)VIDEOMEM_BASE;
	gp->g_fbthere = s2fbthere();

#else  S2FRAMEBUF
	/*
	 * Note that the test below doesn't distinguish MB memory from FB.
	 * But nobody cares, since nobody is fool enough to buy 3/4 meg of
	 * Multibus memory.
	 */
	GXBase = (int)(MBMEM_BASE + 0xC0000);
	if (setbus(bbuf))
		gp->g_fbthere = 0;   /* It's not there. */
	else {
		*(short*)GXBase = 0;	/* touch FB */
		gp->g_fbthere = 1;	/* It is indeed there. */
		unsetbus(bbuf);		/* Uncatch bus errors */
	}
#endif S2FRAMEBUF

	/*
	 * Locations 0x1000-0x1FFF are dedicated for font table
	 */
	gp->g_font = (unsigned short (*)[CHRSHORTS])0x1000;

	if (r_vor == (unsigned)EVEC_RESET) {
/*============================================================================
 * Following code is specific to RESET resets.
 *============================================================================*/
		/*
		 * This is a power-up reset.  Decide whether to use the
		 * frame buffer, based on whether the above tests show
		 * that it exists.
		 */
		if (gp->g_fbthere) {
			finit (0, 0);
			gp->g_outsink = OUTSCREEN;	/* Send output there */
			fwritestr ("\f", 1); /* Clear screen */
		} else {
			gp->g_outsink = OUTDEFAULT;	/* use UART */
		}
		banner();		/* Print startup banner */
	} else {
/*============================================================================
 * Following code is common to DOG, KCMD, and BOOTING resets.
 *============================================================================*/
		/*
		 * This is a Dog, K1, or Boot reset.  Reinitialize the video
		 * output if the board exists.  (The "reset" 
		 * instruction we issued caused a Multibus reset, which
		 * turned off video output from the board.)
		 * 
		 * Since finit sets up for non-copy mode, make sure that
		 * the maps for the screen are also set up that way.  This
		 * is vital for DOGs and a no-op for the rest.
		 *
		 * FIXME: finit() should not reset global state (eg,
		 * black-on-white.
		 */
		if (gp->g_fbthere) {
			setupmap(videoinit);	/* Initialize video mem map */
			finit (ax, ay);
		}
	}
#else	FRAMEBUF
	if (r_vor == (unsigned)EVEC_RESET) {
		gp->g_outsink = OUTDEFAULT;	/* use UART if no FB support */
	}
#endif	FRAMEBUF

/*============================================================================
 * Following code is common to all resets.
 *============================================================================*/

	/*
	 * Now that we can print messages on bus errors, turn on parity
	 * checking.
	 */
	gp->g_enable.ena_par_check = 1;
	set_enable(gp->g_enable);

	if (r_vor == (unsigned)EVEC_DOG) goto DogSkip1;

/*============================================================================
 * Following code is common to RESET, KCMD, and BOOTING resets.
 *============================================================================*/

	/*
	 * Reset and re-initialize the timer chip.  We reset the world
	 * as specified in the manual, select 16-bit bus mode, and clear
	 * the outputs of all the timers, since they are connected to
	 * interrupt lines.
	 */
	set_leds(~L_SETUP_KEYB);

	TIMER_BASE->clk_cmd = CLK_RESET;
	TIMER_BASE->clk_cmd = CLK_RESET;
	TIMER_BASE->clk_cmd = CLK_LOAD + CLK_ALL;
	TIMER_BASE->clk_cmd = CLK_16BIT;

	TIMER_BASE->clk_cmd = CLK_CLEAR + TIMER_NMI;
	TIMER_BASE->clk_cmd = CLK_CLEAR + TIMER_MISC;
	TIMER_BASE->clk_cmd = CLK_CLEAR + TIMER_STEAL;
	TIMER_BASE->clk_cmd = CLK_CLEAR + TIMER_USER;
	TIMER_BASE->clk_cmd = CLK_CLEAR + TIMER_SUPER;


/*============================================================================
 * Following code is common to all resets.
 *============================================================================*/
DogSkip1:

	/*
	 * Init NMI timer -- used for keyboard scanning
	 *
	 * Note that since the Sun-2 CPU board provides hardware refresh,
	 * this NMI timer exists mostly to scan the keyboard port and to
	 * profile the Unix system.
	 */
	TIMER_BASE->clk_cmd = CLK_CLEAR + TIMER_NMI;
	TIMER_BASE->clk_cmd = CLK_ACC_MODE + TIMER_NMI;
	TIMER_BASE->clk_data = NMIMODE;
	TIMER_BASE->clk_data = CLK_BASIC/(NMIFREQ*NMIDIVISOR);
	TIMER_BASE->clk_cmd = CLK_LOAD_ARM + CLK_BIT(TIMER_NMI);

	gp->g_debounce = 0;		/* For remembering BREAK state */

	/*
	 * Now that we need to take NMI's, set up the NMI vector and
	 * enable interrupts.  We need to set up the vector in case this
	 * is a watchdog; Unix now steals it from us, and the keyboard
	 * is unusable after a Dog.
	 */
	*EVEC_LEVEL7 = (long) nmi;	/* Install NMI handler */
	gp->g_enable.ena_ints = 1; set_enable(gp->g_enable);

	/*
	 * If booting, clear parity errors in all of memory.
	 * FIXME, if there is more memory than we can map, just clear what
	 * we can map.
	 */
	if (r_vor == (unsigned)EVEC_BOOTING) {
		clrparerrs(gp->g_memorysize > MAINMEM_SIZE? 
				MAINMEM_SIZE:
				gp->g_memorysize);
	}

#ifdef KEYBOARD

/* #ifndef KEYBS2 */
	if (r_vor == (unsigned)EVEC_DOG) goto DogSkip2;
	if (r_vor == (unsigned)EVEC_BOOTING) goto BootSkip4;
	if (r_vor == (unsigned)EVEC_KCMD) goto KcmdSkip4;
/* #endif KEYBS2 */

/*============================================================================
 * Following code is used for RESET resets.
 *============================================================================*/

	/*
	 * Initialize the keyboard state.
	 */
	gp->g_insource = INKEYB;	/* Assume keyboard input */
	gp->g_keybzscc = &KEYBMOUSE_BASE[1];
	initbuf (gp->g_keybuf, KEYBUFSIZE); /* Set up buffer pointers */

#ifndef KEYBS2
	gp->g_prevkey = NOTPRESENT;
#endif  KEYBS2
	gp->g_keystate = STARTUP;

	gp->g_keybid = KB_UNKNOWN;	/* Take compiled in default */

	/*
	 * Hunt around for the keyboard.
	 *
	 * Wait awhile for the keyboard to come online.  If we
	 * see activity in that period, we break out and select the
	 * keyboard as input.  If after awhile the keyboard is still
	 * not responding, assume it doesn't exist.
	 *
	 * On klunker keyboards, if something other than NOTPRESENT
	 * is there, assume that the keyboard
	 * is there but a latching key is down, and warn the user that
	 * no input will be accepted until they clear the keyboard.
	 *
	 * On klunkers we use a long delay because some early Micro Switch
	 * keyboards were delivered without resistors in their reset 
	 * circuits.  This causes them to take extraordinatily long to
	 * reset on power-up, with the result that if we make a quick test
	 * for keyboard-present, we never see the keyboard.
	 *
	 * On Sun-2 keyboards, we try to send the keyboard a reset
	 * command and see if it responds.  If we get no response within
	 * 100 ms, we skip out and use a serial port.
	 */

#ifdef KEYBVT
	{
		time = 20 + gp->g_nmiclock;	/* Give 20 ms == 10 bytes */
		do {
			if (gp->g_keybid != KB_UNKNOWN) goto KeybDone;
		} while (time > gp->g_nmiclock);

		gp->g_insource = INDEFAULT;  /* Force serial input */
		printf ("Using RS232 A input.\n");
KeybDone:
		;
	}
	/* Initialize the polled side of the parallel keyboard interface. */
	initgetkey(gp->g_keybid);

#else (!KEYBVT)
#ifdef KEYBKL
	{
		unsigned char akey;

		time = 3000 + gp->g_nmiclock;	/* Get current time */
		do {
			GETKEY(akey);
			if (akey == IDLEKEY) goto KeybDone;
		} while (time > gp->g_nmiclock);

		if (akey == NOTPRESENT) {
			gp->g_insource = INDEFAULT;  /* Force serial input */
			printf ("\nUsing RS232 A input.\n");
			goto KeybDone;
		} else {
			printf ("\nPlease clear keyboard to begin.\n");
		}
KeybDone:
		;
	}
	/* Initialize the polled side of the parallel keyboard interface. */
	initgetkey();

#else (!KEYBVT && !KEYBKL)
#ifdef KEYBS2

#ifndef VME
	/*
 	 * On Multibus, keyboard can't be there if there's no frame buffer...
	 * If we support the Sun-2 frame buffer, just use "g_fbthere";
	 * otherwise we have to look explicitly.
	 */
#ifdef S2FRAMEBUF
	if (!gp->g_fbthere)
#else  S2FRAMEBUF
	if (!s2fbthere())
#endif S2FRAMEBUF
	{
		/*
		 * We have to provide a "fake Uart" which the NMI routine
		 * (and sendtokbd()) can read, so they won't get bus errors
		 * trying to touch the keyboard.
		 */
		gp->g_keybzscc = fakeuart;
		goto NoGood;
	}
#endif VME

	/* 
	 * Initialize the keyboard UART, then reset the keyboard
	 * and see what it does.  If no response for awhile,
	 * assume no keyboard.
	 */
	reset_uart(&gp->g_keybzscc->zscc_control, 0);
	/*
	 * Reload time constants for BRG in Uart for keyboard (1200 baud)
	 */
	gp->g_keybzscc->zscc_control = 12;
		(*(long *)0) += 100;	/* Waste time */
	gp->g_keybzscc->zscc_control = ZSTIMECONST(ZSCC_PCLK, 1200);
		(*(long *)0) -= 100;	/* Waste time */
	gp->g_keybzscc->zscc_control = 13;
		(*(long *)0) += 100;	/* Waste time */
	gp->g_keybzscc->zscc_control = (ZSTIMECONST(ZSCC_PCLK, 1200))>>8;
		(*(long *)0) -= 100;	/* Waste time */

	for (i = 100000; i >= 0; i--) {
		if (sendtokbd(KBD_CMD_RESET)) goto SentReset;
	}
	goto NoGood;

SentReset:
	time = 100 + gp->g_nmiclock;	/* Give 100 ms == 12 bytes */
	do {
		if (gp->g_keybid != KB_UNKNOWN) {
			/* Blip the bell to let people know it works. */
			for (i = 1000; i >= 0; i--) {
				if (sendtokbd(KBD_CMD_BELL)) break;
			}
			for (i = 1000; i >= 0; i--) {
				if (sendtokbd(KBD_CMD_NOBELL)) break;
			}
			goto KeybDone;
		}
	} while (time > gp->g_nmiclock);

NoGood:
	gp->g_insource = INDEFAULT;  /* Force serial input */
	printf ("Using RS232 A input.\n");

KeybDone:
	/* Initialize the polled side of the keyboard interface. */
	initgetkey();

#else (!KEYBVT && !KEYBKL && !KEYBS2)
??? Your keyboard type is not specified.
#endif KEYBS2
#endif KEYBKL
#endif KEYBVT

#else  KEYBOARD

	gp->g_insource = INDEFAULT;    /* Assume UART input */

#endif KEYBOARD

#ifdef KEYBS2
	if (r_vor == (unsigned)EVEC_DOG) goto DogSkip2;
#endif KEYBS2

/*============================================================================
 * Following code is common to RESET, KCMD, and BOOTING resets.
 *============================================================================*/
BootSkip4:
KcmdSkip4:

	/*
	 * Initialize random software state in globram.
	 */
	r_usp = gp->g_memorysize;	/* reset User Stack pointer */
	/* It's ok that lint complains about the above line.
	   We set r_usp in calling stack frame; it is restored 
	   into the USP when we return.  We don't otherwise
	   reference it here. */

#ifdef BREAKTRAP
	gp->g_breaking = 0;	/* no break in progress */
	gp->g_breakaddr = 0;	/* initialize break address */
#endif BREAKTRAP

/*============================================================================
 * Following code is common to all resets.
 *============================================================================*/
DogSkip2:

	/*
	 * Set up watchdog reset recovery.  This is done as the last thing
	 * in case we take another dog while setting up.  In that case,
	 * we'll do a real power-on reset, which will auto-boot, rather
	 * than our sitting in a loop doing watchdogs all night.
	 *
	 * On reset, we'll map the PROM into low memory (its usual place,
	 * modulo segment size) and jump to dogreset.  Dogreset will put
	 * us back into boot state, jump to its normal location, and fix
	 * up the world.
	 *
	 * We map it into low memory because we know low memory has a pmeg.
	 */
	gp->g_resetaddr = ((long) dogreset) & ((BYTESPERPG*PGSPERSEG)-1);
	gp->g_resetaddrcomp = ~gp->g_resetaddr;
	gp->g_resetmap = PME_PROM;
	gp->g_resetmapcomp = ~gp->g_resetmap;

	/*
	 * We are done setting up after a reset.  Return to the reset
	 * code, which will call the command processor.
	 */
	return;

}


/*
 * This subroutine initializes large sections of the page map.
 * It is called to initially set up the maps, and also called when
 * setting up a "Sun-1 fake" environment.
 */
setupmap(table)
	struct pginit table[];
{
	register struct pginit *k;
	char *i;
	struct pgmapent pg;

	for (k = table; k->pi_incr != PGINITEND; k++) {
		i = k->pi_addr;
		pg = k->pi_pm;
		for (; i < k[1].pi_addr;
		       i += BYTESPERPG, pg.pm_page += k->pi_incr) {
			setpgmap(i, pg);
		}
	}
}


/*
 * Initial mapping of memory:
 *
 * We need an initial mapping of memory that will suffice
 * to get the monitor started and allow memory to be sized;
 * we set up a useful segment map, then a useful page map.
 *
 * We leave things in context 0.
 */
mapmem()
{
	register char *i;
	register unsigned short j;

	/* FIXME: must initialize all segments of all contexts (invalid) */
	/* Probably not needed, but play it safe. */
	setusercontext((context_t) 0);

	/*
	 * Map segments of context 0 to pmegs.
	 *
	 * Address Range	Mapped To		Why
	 * 0M - 6M	Pmegs 0x00 - 0x60	Main Memory & invalid memory
	 * 6M - 14M	The Invalid Pmeg	Not enuf pmegs to do otherwise
	 * 14M - 16M	Pmegs 0x60 - 0x7F	Global bus, I/O, DVMA, etc.
	 *   except
	 * INVPMEG_BASE	The Invalid Pmeg	To free up a pmeg for last seg
	 *   and
	 * last segment	INVPMEG_BASE's pmeg	So last seg can map to MB mem
	 *
	 * We want to use the last pmeg as the Invalid Pmeg, but we want
	 * to use the last virtual address as part of Multibus Memory
	 * space.  To do this, we swap the last pmeg and the one that
	 * would normally be mapped at the virtual address of the Invalid
	 * Pmeg.
	 */
	j = NUMPMEGS-1;
	i = (char *) ((SEGSPERCONTEXT-1) * PGSPERSEG * BYTESPERPG);
	for (;; i -= PGSPERSEG * BYTESPERPG) {
		if (i >= HOLE_BASE && i < HOLE_BASE+HOLE_SIZE) {
			setsegmap(i, SEG_INVALID);
		} else {
			setsegmap(i, (segnum_t)(j--));
		}
		if (i == 0) break;
	}

	setsegmap(INVPMEG_BASE, SEG_INVALID);
	setsegmap(SEG_INVALID * BYTESPERPG * PGSPERSEG,
		(int)INVPMEG_BASE / (BYTESPERPG * PGSPERSEG) );

	/*
	 * Map all of memory according to initialization table
	 *
	 * First, we must map in the initialization table, since it's
	 * accessed as data.  We map two pages in case it spans a
	 * page boundary.
	 */
	setpgmap(((char *)mapinit)           , PME_PROM); 
	setpgmap(((char *)mapinit)+BYTESPERPG, PME_PROM);
	setupmap(mapinit);		/* Initialize whole page map */

	/*
	 * Turn on things, now that we are mapped.
	 */
	gp->g_enable.ena_notboot = 1;	/* PROM is mapped in, safe now. */
	gp->g_enable.ena_dvma    = 1;	/* Maps are set, DVMA is OK. */
	set_enable(gp->g_enable);
}


#if defined(S2FRAMEBUF) || (defined(KEYBS2) & !defined(VME))
/*
 * Test for the existence of the Sun-2 Frame Buffer.
 */
int
s2fbthere()
{
	long bbuf[4];		/* Buffer for bus error recovery */

	/*
	 * The only way we can get parity errors is if the board does not
	 * exist; it always generates correct parity on the P2-bus, since
	 * it doesn't store parity in the frame buffer.
	 */
	if (setbus(bbuf)) {
		return 0;   /* It's not there. */
	} else {
		unsigned short i;
#		define TLOC *(unsigned short *)VIDEOMEM_BASE

		i = TLOC;
		if (i != TLOC) {
			return 0;
		} else {
			TLOC = ~i;
			if (~i != TLOC) {
				return 0;
			} else {
				TLOC = i;
				if (i != TLOC) {
					return 0;
				}
			}
		}
		
		unsetbus(bbuf);		/* Uncatch bus errors */
#		undef TLOC
	}
	return 1;
}
#endif defined(S2FRAMEBUF) || (defined(KEYBS2) & !defined(VME))
