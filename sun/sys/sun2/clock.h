/*	@(#)clock.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * The Sun 2 clock is based on the AMD Am9513 Timer.
 * Time-of-day provided by a separate chip.
 */
#ifdef LOCORE
#define	CLKADDR	0xEE0000
#else
struct am9513 {
	u_short	clk_data;	/* data register */
	u_short	clk_cmd;	/* command register */
};
#define	CLKADDR	((struct am9513 *)(0xEE0000))
#endif

/*
 * Carefully define the basic CPU clock rate so
 * that time-of-day calculations don't float
 *
 * Note that the CLK_BASIC is divided by 4 before we can count with it,
 * e.g. F1 ticks CLK_BASIC/4 times a second.
 */
#define	CLK_BASIC	19660800

#define	CLK_F1		0xB00 	/* F1 = pulse/1 */
#define	CLK_F2		0xC00	/* F2 = pulse/16 */
#define	CLK_F3		0xD00	/* F3 = pulse/256 */
#define	CLK_F4		0xE00	/* F4 = pulse/4096 */
#define	CLK_F5		0xF00	/* F5 = pulse/65536 */
#define	CLK_F1DIV	1
#define	CLK_F2DIV	16
#define	CLK_F3DIV	256
#define	CLK_F4DIV	4096
#define	CLK_F5DIV	65536

/* The following commands must have a single timer number added to them. */
#define	CLK_LMODE	0xFF00	/* load the mode register */
#define	CLK_LLOAD	0xFF08	/* load the load register */
#define	CLK_LHOLD	0xFF10	/* load the hold register */
#define	CLK_CLROUT	0xFFE0	/* clear toggling output pin */

/* The following commands must have a bit mask of timers added to them. */
#define	CLK_ARM		0xFF20	/* arm counters */
#define	CLK_LOAD	0xFF40	/* load counters */
#define	CLK_LOADARM	0xFF60	/* load, then arm, counters */
#define	CLK_SAVE	0xFFA0	/* save counters to hold reg */
#define	CLK_GO		CLK_ARM	/* make counters "go" */

/* The following commands are complete in themselves. */
#define	CLK_RESET	0xFFFF			/* reset clock */
#define	CLK_REFR	(CLK_CLROUT+CLKTIMER)	/* re-enable timer */
#define	CLK_REFR_NMI	(CLK_CLROUT+CLKNMI)	/* re-enable nmi timer */

/* These are mode register values for various uses */
#define	CLK_TICK_MODE		(CLK_F2+0x22)	/* F2 + Operating mode D */
#define	CLK_UART_MODE		(CLK_F1+0x22)	/* F1 + Operating mode D */
#define	CLK_FAST_LO_MODE	(CLK_F3+0x28)	/* F3, repeat, count up */
#define	CLK_FAST_HI_MODE	(0x0028)	/* TC of LO, repeat, count up */

#define	CLK_HZ(hz)	((CLK_BASIC/(4*CLK_F2DIV))/(hz)) /* hz to clk conv */	

/* These define which counters are used for what. */
#define	CLKNMI		1	/* Non Maskable Interrupts */
#define	CLKTIMER	2	/* Timer 2 */
#define	CLKUNUSED	3	/* Unused timer */
#define	CLKFAST_LO	4	/* Timer 4 for realtime, low  order */
#define	CLKFAST_HI	5	/* Timer 5 for realtime, high order */

#define	CLKNUM_TO_BIT(n)	(1 << ((n)-1))	/* For cmds that use bits */

/*
 * Number of microseconds that elapse for each
 * fastcounter `tick' (approximately 52).
 */
#define FASTCOUNTER_USEC	(1000000/(CLK_BASIC/(4*CLK_F3DIV)))
