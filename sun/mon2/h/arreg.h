/*	@(#)arreg.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Header file for Archive tape driver.
 *
 * This file contains definitions of the control bits for the 
 * Sun Archive interface board, Rev A and Rev B (look the same to software).
 * It also defines the commands which the Archive
 * will accept, and the format of its Status information.
 *
 * Originally entered by Peter Costello, 13Sep82, for a standalone
 * Archive board debugging program.
 *
 * Modified to use bit-location-independent macros by John Gilmore,
 * 10Oct82, in preparation for the preliminary PC board version.
 *
 * Modified to use the preliminary PC board registers 2Nov82 by John
 * Gilmore.
 *
 * Finalized for release of tape unit and software 8Dec82 by John Gilmore.
 */

/* Macros for manipulating bits in the interface */
#ifdef REALCOMPILER
#define	Read(BIT) regs->BIT
#define	Assert(BIT) {regs->BIT = 1;DebugTrace;}
#define	Negate(BIT) {regs->BIT = 0;DebugTrace;}
#else
#define	Read(BIT) ((((struct ar_regs2 *)regs)->rctrl) & BIT)
#define	Assert(BIT) {((struct ar_regs2 *)regs)->wctrl |=  BIT; DebugTrace;}
#define	Negate(BIT) {((struct ar_regs2 *)regs)->wctrl &= ~BIT; DebugTrace;}
#endif REALCOMPILER

#define	DebugTrace \
       { if (DEBWRITE) \
		Dprintf (ar_ctrl_hdr, *(long*)(2+(char*)regs), ar_bits);}

/*
 * Macros for sending bytes of data or commands to the interface
 * They could test DIRC for debugging.
 * FIXME!  (value) is evaluated TWICE if printf is on!!!
 */
#define	SetData(value)	\
	{regs->XFERBYTE = value; DebugData(value);} \

#define	DebugData(value) \
	 if (DEBWRITE) Dprintf (ar_set_hdr, (value))

#define	GetData()	regs->XFERBYTE

/* Macro for unwedging the interface */
#define	UnWedge		regs->UNWEDGE = 1;

/*
 * ar_regs defines the hardware interface.
 *
 * The Sun Archive interface occupies 8 bytes of Multibus I/O space.
 * 4 of those bytes are unused (the high-order half of each word).
 * The remainder are registers.  The registers at offset 3 and 5 are
 * read/write and the writeable bits retain their settings in reads.
 * Register 1 is the data port and either writes to the 8 data lines,
 * or reads from them, depending on the tape controller DIRC signal.
 * (If you do the wrong thing (and are not in Burst mode) no trouble ensues,
 * your bits just get lost in a turned-off buffer somewhere.)
 * The register at offset 7 is not really a register.  A write to it
 * will pretend that the last byte read/written in Burst mode was ack-ed
 * by the tape drive, thus regaining CPU access to the control register
 * (which is also interlocked in Burst mode, so you can't turn off Burst
 * mode until after the ack of the final byte).  Reads from offset 7 have
 * no effect but can be used as a good logic analyzer trigger if you like.
 */
struct ar_regs {
	u_char		:8;

	u_char XFERBYTE;		/* Data byte for I/O */

	u_char		:8;

	u_char ENAREADY	:1;		/* Enable interrupt on EDGEREADY */
	u_char ENAEXCEP	:1;		/* Enable interrupt on EXCEPTION */
	u_char CATCHREADY:1;		/* Please notice leading edge o'READY */
	u_char BURST	:1;		/* Use burst mode (auto xfer/ack) */
	u_char XFER	:1;		/* XFER	wire to controller if !BURST */
	u_char RESET	:1;		/* RESET wire to controller */
	u_char REQUEST	:1;		/* REQUEST      '' */
	u_char ONLINE	:1;		/* ONLINE 	'' */

	u_char		:8;

	u_char MEMPARITY:1;		/* Enables parity check on MB mem */
	u_char		:1;		/* unused */
	u_char INTERRUPT:1;		/* Board is now requesting interrupt */
	u_char EDGEREADY:1;		/* A leading edge of READY was seen
					   since the last time CATCHREADY was
					   unset then set. */
	u_char ACK	:1;		/* ACK wire from tape controller */
	u_char EXCEPTION:1;		/* EXCEPTION 	'' */
	u_char DIRC	:1;		/* DIRC 	'' */
	u_char READY	:1;		/* READY 	'' */

	u_char		:8;

	u_char UNWEDGE;			/* Write to here unwedges Burst */
};

#ifndef REALCOMPILER
struct ar_regs2 {
	char filler0;
	char dataport;
	char filler2;
	u_char wctrl;			/* Write control reg */
	char filler4;
	u_char rctrl;
	char filler6;
	u_char unwedge;
};

#define	CATCHREADY 0x20
#define	BURST 0x10
#define	RESET 0x04
#define	REQUEST 0x02
#define	ONLINE 0x01

#define	EDGEREADY 0x10
#define	EXCEPTION 0x04
#define	READY 0x01

#endif

/* The following are commands that can be written to the data port while
   appropriately toggling REQUEST and READY. */
#define	AR_sel0    ((u_char)0x01)	/* Select Unit 0 */
#define	AR_sel1    ((u_char)0x02)	/* Select Unit 1 */
#define	AR_sel2    ((u_char)0x04)	/* Select Unit 2 */
#define	AR_sel3    ((u_char)0x08)	/* Select Unit 3 */
#define	AR_selLED  ((u_char)0x10)	/* Light the LED on the unit, to let
					   the user know that it's in use;
					   also with the light on, the tape
					   drive will tell us if they pull
					   out the cartridge. */

#define	AR_rewind  ((u_char)0x21)	/* Rewind tape */
#define	AR_erase   ((u_char)0x22)	/* Erase entire tape, BOT to EOT */
#define	AR_tension ((u_char)0x24)	/* Retension tape */

#define	AR_wrdata  ((u_char)0x40)	/* Write data */
#define	AR_wreof   ((u_char)0x60)	/* Write EOF */
#define	AR_rddata  ((u_char)0x80)	/* Read data */
#define	AR_rdeof   ((u_char)0xA0)	/* Read EOF */
#define	AR_rdstat  ((u_char)0xC0)	/* Read status */

/* This struct defines the 6 status bytes returned by the Archive during
   a Read Status command (AR_rdstat). */
struct arstatus {
	unsigned AnyEx0		:1;	/* Logical-OR of the next 7 bits */
	unsigned NoCart		:1;	/* No fully inserted cartridge */
	unsigned NoDrive	:1;	/* Drive not connected to ctrlr */
	unsigned WriteProt	:1;	/* Cart is write protected */
	unsigned EndOfMedium	:1;	/* End of last track reached on wr */
	unsigned HardErr	:1;	/* Hard (unrecoverable) I/O error */
	unsigned GotWrongBlock	:1;	/* Ctrlr sent us wrong bad block */
	unsigned FileMark	:1;	/* We just read a File Mark */

	unsigned AnyEx1		:1;	/* Logical-OR if the next 7 bits */
	unsigned InvalidCmd	:1;	/* We sent a bad command to ctrlr */
	unsigned NoData		:1;	/* No data block, blank tape */
	unsigned GettingFlakey	:1;	/* >= 8 retries on last block */
	unsigned BOT		:1;	/* Cartridge is at BOT */
	unsigned 		:1;
	unsigned 		:1;
	unsigned GotReset	:1;	/* Ctrlr reset since last Status */
	unsigned short	SoftErrs;	/* # soft errors (R or W) since last
					   time we looked */
	unsigned short	TapeStops;	/* # times tape stopped 'cuz CPU
					   didn't keep up (since last time) */
};

/*
 * The following string can be used as argument to a %b printf to print
 * the bits returned in the first shortword of drive status.
 */
#define	ARCH_BITS "\20\
\17NoCart\
\16NoDrive\
\15WriteProt\
\14EndMedium\
\13HardErr\
\12WrongBlock\
\11FileMark\
\7InvCmd\
\6NoData\
\5Flaking\
\4BOT\
\0034\
\0022\
\1GotReset"

/* 
 * Printf %b string for bits in control/status registers (packed into a short,
 * as returned by IOCTL MTIOCGET)
 */

#define	ARCH_CTRL_BITS "\20\
\20EnaReady\
\17EnaExcep\
\16CatchReady\
\15Burst\
\14Xfer\
\13Reset\
\12Request\
\11Online\
\6Interrupt\
\5EdgeReady\
\4Ack\
\3Exception\
\2Dirc\
\1Ready\
"

/* 
 * Printf %b string for bits in control/status registers (treated as long)
 * The long that you pass can be read from the hardware as:
 *	*(long *)(2+(char *)regs)
 */

#define	ARCH_LONG_CTRL_BITS "\20\
\30EnaReady\
\27EnaExcep\
\26CatchReady\
\25Burst\
\24Xfer\
\23Reset\
\22Request\
\21Online\
\6Interrupt\
\5EdgeReady\
\4Ack\
\3Exception\
\2Dirc\
\1Ready\
"
/*
 * Block size of the Archive tape unit.  Don't depend on it.
 */
#define	AR_BSIZE	512
#define	AR_BSHIFT	9	/* foo/AR_BSIZE == foo >> AR_BSHIFT */
