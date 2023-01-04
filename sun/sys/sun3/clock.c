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
#include "../machine/interreg.h"

struct timeval todget();

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

/*
 * Start the real-time clock.
 */
startrtclock()
{

	/*
	 * We will set things up to interrupt every 1/100 of a second.
	 * locore.s currently only calls hardclock every other clock
	 * interrupt, thus assuming 50 hz operation.
	 */
	if (hz != 50)
		panic("startrtclock");

	CLKADDR->clk_intrreg = CLK_INT_HSEC;	/* set 1/100 sec clock intr */
	set_clk_mode(IR_ENA_CLK5, 0);		/* turn on level 5 clock intr */
}

/*
 * Set and/or clear the desired clock bits in the interrupt
 * register.  We have to be extremely careful that we do it
 * in such a manner that we don't get ourselves lost.
 */
set_clk_mode(on, off)
	u_char on, off;
{
	register u_char interreg, dummy;

	/*
	 * make sure that we are only playing w/ 
	 * clock interrupt register bits
	 */
	on &= (IR_ENA_CLK7 | IR_ENA_CLK5);
	off &= (IR_ENA_CLK7 | IR_ENA_CLK5);

	/*
	 * Get a copy of current interrupt register,
	 * turning off any undesired bits (aka `off')
	 */
	interreg = *INTERREG & ~(off | IR_ENA_INT);
	*INTERREG &= ~IR_ENA_INT;

	/*
	 * Next we turns off the CLK5 and CLK7 bits to clear
	 * the flip-flops, then we disable clock interrupts.
	 * Now we can read the clock's interrupt register
	 * to clear any pending signals there.
	 */
	*INTERREG &= ~(IR_ENA_CLK7 | IR_ENA_CLK5);
	CLKADDR->clk_cmd = (CLK_CMD_NORMAL & ~CLK_CMD_INTRENA);
	dummy = CLKADDR->clk_intrreg;			/* clear clock */
#ifdef lint
	dummy = dummy;
#endif

	/*
	 * Now we set all the desired bits
	 * in the interrupt register, then
	 * we turn the clock back on and
	 * finally we can enable all interrupts.
	 */
	*INTERREG |= (interreg | on);			/* enable flip-flops */
	CLKADDR->clk_cmd = CLK_CMD_NORMAL;		/* enable clock intr */
	*INTERREG |= IR_ENA_INT;			/* enable interrupts */
}

int dosynctodr = 1;		/* if true, sync UNIX time to TOD */
int clkdrift = 0;		/* if true, show UNIX & TOD sync differences */
int synctodrval = 30;		/* number of seconds between synctodr */
extern int adjtimedelta;
#define ABS(x)	((x) < 0? -(x) : (x))

synctodr()
{
	struct timeval tv;
	int deltat, s;

	/*
	 * If adjtimedelta is non-zero, assume someone who
	 * knows better is already adjusting the time.
	 */
	if (dosynctodr && adjtimedelta == 0) {
		s = splclock();
		tv = todget();
		/*
		 * Set up new adjtimedelta value if
		 * we have drifted over a clock tick.
		 */
		deltat = 1000000 * (tv.tv_sec - time.tv_sec) +
		    (tv.tv_usec - time.tv_usec);
		if (ABS(deltat) > 1000000 / hz) {
			adjtimedelta = deltat;
			if (clkdrift)
				printf("<[%d]> ", deltat / 1000);
		}
		(void) splx(s);
	}
	timeout(synctodr, (caddr_t)0, synctodrval * hz);
}

/*
 * Initialize the system time, based on the time base which is, e.g.
 * from a filesystem.
 */
inittodr(base)
	time_t base;
{
	register long deltat;
	int s;

	s = splclock();
	time = todget();
	(void) splx(s);
	if (base < (86 - YRREF) * SECYR) {	/* ~1986 */
		printf("WARNING: preposterous time in file system");
		goto check;
	}
	if (time.tv_sec < SECYR) {
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
	if (dosynctodr)
		timeout(synctodr, (caddr_t)0, synctodrval * hz);
}

/*
 * For Sun-3, we use the Intersil ICM7170 for both the
 * real time clock and the time-of-day device.
 */

static u_int monthsec[12] = {
	31 * SECDAY,	/* Jan */
	28 * SECDAY,	/* Feb */
	31 * SECDAY,	/* Mar */
	30 * SECDAY,	/* Apr */
	31 * SECDAY,	/* May */
	30 * SECDAY,	/* Jun */
	31 * SECDAY,	/* Jul */
	31 * SECDAY,	/* Aug */
	30 * SECDAY,	/* Sep */
	31 * SECDAY,	/* Oct */
	30 * SECDAY,	/* Nov */
	31 * SECDAY	/* Dec */
};

#define	MONTHSEC(mon, yr)	\
	(((((yr) % 4) == 0) && ((mon) == 2))? 29*SECDAY : monthsec[(mon) - 1])

/*
 * Set the TOD based on the argument value; used when the TOD
 * has a preposterous value and also when the time is reset
 * by the settimeofday system call.  We run at splclock() to
 * avoid synctodr() from running and getting confused.
 */
resettodr()
{
	register int t;
	u_short hsec, sec, min, hour, day, mon, weekday, year;
	int s;

	s = splclock();

	/*
	 * Figure out the (adjusted) year
	 */
	t = time.tv_sec;
	for (year = (YRREF - YRBASE); t > SECYEAR(year); year++)
		t -= SECYEAR(year);

	/*
	 * Figure out what month this is by subtracting off
	 * time per month, adjust for leap year if appropriate.
	 */
	for (mon = 1; t >= 0; mon++)
		t -= MONTHSEC(mon, year);

	t += MONTHSEC(--mon, year);	/* back off one month */

	sec = t % 60;			/* seconds */
	t /= 60;
	min = t % 60;			/* minutes */
	t /= 60;
	hour = t % 24;			/* hours (24 hour format) */
	day = t / 24;			/* day of the month */
	day++;				/* adjust to start at 1 */
	weekday = day % 7;		/* not right, but it doesn't matter */

	hsec = time.tv_usec / 10000;

	CLKADDR->clk_cmd = (CLK_CMD_NORMAL & ~CLK_CMD_RUN);
	CLKADDR->clk_weekday = weekday;
	CLKADDR->clk_year = year;
	CLKADDR->clk_mon = mon;
	CLKADDR->clk_day = day;
	CLKADDR->clk_hour = hour;
	CLKADDR->clk_min = min;
	CLKADDR->clk_sec = sec;
	CLKADDR->clk_hsec = hsec;
	CLKADDR->clk_cmd = CLK_CMD_NORMAL;

	adjtimedelta = 0;		/* destroy any time delta */

	(void) splx(s);
}

/*
 * Read the current time from the clock chip and convert to UNIX form.
 * Assumes that the year in the counter chip is valid.
 */
struct timeval
todget()
{
	struct timeval tv;
	u_char now[CLK_WEEKDAY + 1];
	register int i, t = 0;
	register u_char *cp = (u_char *)CLKADDR;
	u_short year;

	for (i = CLK_HSEC; i <= CLK_WEEKDAY; i++)	/* read counters */
		now[i] = *cp++;

	if (now[CLK_MON] < 1 || now[CLK_MON] > 12 ||
	    now[CLK_DAY] < 1 || now[CLK_DAY] > 31 ||
	    now[CLK_WEEKDAY] > 6 || now[CLK_HOUR] > 23 ||
	    now[CLK_MIN] > 59 || now[CLK_SEC] > 59 ||
	    now[CLK_YEAR] < (YRREF - YRBASE)) {		/* not initialized */
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		return (tv);
	}

	/*
	 * Add the number of seconds for each year onto our time t.
	 * We start at YRREF - YRBASE (which is the chip's value
	 * for UNIX's YRREF year), and count up to the year value given
	 * by the chip, adding each years seconds value to the Unix
	 * time value we are calculating.
	 */
	for (year = YRREF - YRBASE; year < now[CLK_YEAR]; year++)
		t += SECYEAR(year);

	/*
	 * Now add in the seconds for each month that has gone
	 * by this year, adjusting for leap year if appropriate.
	 */
	for (i = 1; i < now[CLK_MON]; i++)
		t += MONTHSEC(i, year);

	t += (now[CLK_DAY] - 1) * SECDAY;
	t += now[CLK_HOUR] * (60*60);
	t += now[CLK_MIN] * 60;
	t += now[CLK_SEC];

	/*
	 * If t is negative, assume bogus time
	 * (year was too large) and use 0 seconds.
	 * XXX - tv_sec and tv_usec should be unsigned.
	 */
	if (t < 0) {
		tv.tv_sec = 0;
		tv.tv_usec = 0;
	} else {
		tv.tv_sec = t;
		tv.tv_usec = now[CLK_HSEC] * 10000;
	}
	return (tv);
}
