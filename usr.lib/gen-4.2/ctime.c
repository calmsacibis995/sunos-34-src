/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char sccsid[] = "@(#)ctime.c 1.1 86/09/24 SMI"; /* from UCB 5.5 86/03/09 */
#endif not lint

/*
 * This routine converts time as follows.
 * The epoch is 0000 Jan 1 1970 GMT.
 * The argument time is in seconds since then.
 * The localtime(t) entry returns a pointer to an array
 * containing
 *  seconds (0-59)
 *  minutes (0-59)
 *  hours (0-23)
 *  day of month (1-31)
 *  month (0-11)
 *  year-1970
 *  weekday (0-6, Sun is 0)
 *  day of the year
 *  daylight savings flag
 *
 * The routine calls the system to determine the local
 * timezone and whether Daylight Saving Time is permitted locally.
 * (DST is then determined by the current local rules)
 *
 * The routine does not work
 * in Saudi Arabia which runs on Solar time.
 *
 * asctime(tvec))
 * where tvec is produced by localtime
 * returns a ptr to a character string
 * that has the ascii time in the form
 *	Thu Jan 01 00:00:00 1970\n\0
 *	0123456789012345678901234 5
 *	0	  1	    2
 *
 * ctime(t) just calls localtime, then asctime.
 */

#include <sys/time.h>
#include <sys/types.h>
#include <sys/timeb.h>

static	char	cbuf[26];
static	int	dmsize[12] =
{
	31,
	28,
	31,
	30,
	31,
	30,
	31,
	31,
	30,
	31,
	30,
	31
};

/*
 * The following tables specify the days that daylight savings time
 * started and ended for some year or, if the year in the table is
 * 0, for all years not explicitly mentioned in the table.
 * Both days are assumed to be Sundays.  For entries for specific years,
 * they are given as the day number of the Sunday of the change.  For
 * wildcard entries, it is assumed that the day is specified by a rule
 * of the form "first Sunday of <some month>" or "last Sunday of <some
 * month>."  In the former case, the negative of the day number of the
 * first day of that month is given; in the latter case, the day number
 * of the last day of th