#ifndef lint
static	char sccsid[] = "@(#)clock.c 1.1 86/09/25";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "../h/param.h"
#include "../h/time.h"
#include "../h/kernel.h"

#include "../machine/clock.h"
#include "../machine/scb.h"

/*
 * Machine-dependent clock routines.
 *
 * Startrtclock restarts the real-time clock, which provides
 * hardclock interrupts to kern_clock.c.
 *
 * Inittodr initializes the time of day hardware which provides
 * date functions.  Its primary function is to use some file
 * system information in case the hardare clock lost state.
 *
 * Resettodr restores the time of day hardware after a time change.
 */

static level5_clock_started;

/*
 * Start the real-time clock.
 */
startrtclock()
{
#include "pi.h"
#if NPI > 0
#define	PI_RATE	800
	extern int clknopiscan();
	extern int clkpiscan();
	extern short clkrate;

	/* parallel keyboard set ppiscan early */
	if (scb.scb_autovec[5-1] != clkpiscan) {
		scb.scb_autovec[5-1] = clknopiscan;
		start_level5_clock(hz);
	} else {
		clkrate = PI_RATE / hz;		/* compute clock rate */
		start_level5_clock(PI_RATE);
	}
#else NPI
	start_level5_clock(hz);
#endif NPI
	level5_clock_started = 1;
}

start_level5_clock(hz_val)
	int hz_val;
{

	CLKADDR->clk_cmd = CLK_LMODE+CLKTIMER;	/* issue load-mode command */
	CLKADDR->clk_data = CLK_TICK_MODE;	/* set mode */
	CLKADDR->clk_cmd = CLK_LLOAD+CLKTIMER;	/* issue load-register cmd */
	CLKADDR->clk_data = CLK_HZ(hz_val);	/* set load register */
	CLKADDR->clk_cmd = CLK_ARM+CLKNUM_TO_BIT(CLKTIMER); /* Start clock */
}

#if NPI > 0
/*
 * Start level 5 clock going at PI_RATE instead of hz so
 * piscan gets interrupted often enough.
 */
start_piscan_clock()
{
	register int s;
	extern short clkrate;

	s = spl5();
	if (level5_clock_started) {/* don't start clock before startrtclock */
		start_level5_clock(PI_RATE);
		clkrate = PI_RATE / hz;		/* compute clock rate */
	}
	(void) splx(s);
}
#endif NPI

static int todexists = 0;
static struct timeval (*tod_get)();
static (*tod_set)();

int dosynctodr = 1;		/* if true, sync UNIX time to TOD */
int clkdrift = 0;		/* if true, show UNIX & TOD sync differences */
int synctodrval = 30;		/* number of seconds between synctodr */
extern int adjtimedelta;
#define ABS(x)	((x) < 0? -(x) : (x))

synctodr()
{
	struct timeval tv1, tv2;
	int deltat, s;

	/*
	 * If adjtimedelta is non-zero, assume someone who
	 * knows better is already adjusting the time.
	 */
	if (dosynctodr && adjtimedelta == 0) {
		s = splclock();
		tv2 = time;
		tv1 = (*tod_get)(tv2);
		if (timerisset(&tv1)) {
			/*
			 * Read the time from tod device ok,
			 * now set up new adjtimedelta value
			 * if we have drifted over a clock tick.
			 */
			deltat = 1000000 * (tv1.tv_sec - tv2.tv_sec) +
			    (tv1.tv_usec - tv2.tv_usec);
			if (ABS(deltat) > 1000000 / hz) {
				adjtimedelta = deltat;
				if (clkdrift)
					printf("<[%d]> ", deltat / 1000);
			}
		}
		(void) splx(s);
	}
	timeout(synctodr, (caddr_t)0, synctodrval * hz);
}

/*
 * Called from TOD device probe routine
 * to attach a TOD clock to the system
 */
tod_attach(get, set)
	struct timeval (*get)();
	int (*set)();
{

	todexists = 1;
	tod_get = get;
	tod_set = set;
}

#define	SECDAY		(24*60*60)	/* seconds per day */
#define	SECYR		(365*SECDAY)	/* per common year */
#define	YRREF		70		/* UNIX time starts in 1970 */

/*
 * Initialize the system time, based on the time base which is, e.g.
 * from a filesystem.  Base provides the time to within a year
 * and the time of day clock provides the rest.
 */
inittodr(base)
	time_t base;
{
	register long deltat;
	int s;

	time.tv_sec = base;
	if (todexists == 0) {
		printf("WARNING: no TOD clock");
		goto check;
	}
	s = splclock();
	time = (*tod_get)(time);
	(void) splx(s);
	if (base < (86 - YRREF) * SECYR) {	/* ~1986 */
		printf("WARNING: preposterous time in file system");
		goto check;
	}
	if (time.tv_sec == 0) {
		time.tv_sec = base;
		printf("WARNING: TOD clock not initialized");
		resettodr();
		goto check;
	}
	deltat = time.tv_sec - base;
	/*
	 * See if we gained/lost two or more days;
	 * if so, assume something is amiss.
	 */
	if (deltat < 0)
		deltat = -deltat;
	if (deltat < 2*SECDAY)
		goto out;
	printf("WARNING: clock %s %d days",
	    time.tv_sec < base ? "lost" : "gained", deltat / SECDAY);
check:
	printf(" -- CHECK AND RESET THE DATE!\n");
out:
	if (todexists && dosynctodr)
		timeout(synctodr, (caddr_t)0, synctodrval * hz);
}

/*
 * Reset the TODR based on the time value; used when the TODR
 * has a preposterous value and also when the time is reset
 * by the settimeofday system call.  We call tod_set at splclock()
 * to avoid synctodr() from running and getting confused.
 */
resettodr()
{
	int s;

	s = splclock();
	if (todexists)
		(*tod_set)(time);
	adjtimedelta = 0;		/* destroy any time delta */
	(void) splx(s);
}

#ifdef DEBUG
/*
 * Because of the confusing names used thoroughout Unix for the
 * various clocks, we can't call this the "realtime" clock, which
 * indeed it is, so we call it the fast counter.
 *
 * There are two things you can do with it:  Set it to a certain value,
 * and get its value.  Setting it [re]initializes it.
 *
 * Since the "fast counter" is an u_long, it takes two 16-bit
 * AM9513 counters to implement it.
 */
set_fastcounter(value)
	u_long value;
{
	/* Set both clocks' mode registers. */
	CLKADDR->clk_cmd = CLK_LMODE + CLKFAST_LO;
	CLKADDR->clk_data = CLK_FAST_LO_MODE;
	CLKADDR->clk_cmd = CLK_LMODE + CLKFAST_HI;
	CLKADDR->clk_data = CLK_FAST_HI_MODE;

	/* Load in the initial value */
	CLKADDR->clk_cmd = CLK_LLOAD + CLKFAST_LO;
	CLKADDR->clk_data = (short)value;
	CLKADDR->clk_cmd = CLK_LLOAD + CLKFAST_HI;
	CLKADDR->clk_data = (short)(value >> 16);
	CLKADDR->clk_cmd = CLK_LOAD
			   + CLKNUM_TO_BIT(CLKFAST_HI)
			   + CLKNUM_TO_BIT(CLKFAST_LO);

	/* Set up for retriggering (when each counter overflows) */
	CLKADDR->clk_cmd = CLK_LLOAD + CLKFAST_LO;
	CLKADDR->clk_data = 0;
	CLKADDR->clk_cmd = CLK_LLOAD + CLKFAST_HI;
	CLKADDR->clk_data = 0;

	/* Start the counters! */
	CLKADDR->clk_cmd = CLK_ARM
			   + CLKNUM_TO_BIT(CLKFAST_HI)
			   + CLKNUM_TO_BIT(CLKFAST_LO);
}

u_long
get_fastcounter()
{
	u_long value;
	char once = 0;

	/*
	 * We save both counters into their Hold registers, read
	 * them, and re-save if a possible carry problem occurred.
	 */
	do {
		CLKADDR->clk_cmd = CLK_SAVE
				   + CLKNUM_TO_BIT(CLKFAST_HI)
				   + CLKNUM_TO_BIT(CLKFAST_LO);
		CLKADDR->clk_cmd = CLK_LHOLD + CLKFAST_LO;
		value = (u_short)(CLKADDR->clk_data);
	} while (value == 0 && once++ == 0);
	CLKADDR->clk_cmd = CLK_LHOLD + CLKFAST_HI;
	value |= ((u_short)(CLKADDR->clk_data)) << 16;

	return (value);
}
#endif DEBUG
