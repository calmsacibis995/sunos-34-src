/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char sccsid[] = "@(#)ctime.c 1.4 87/03/16 SMI"; /* from UCB 5.5 86/03/09 */
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
 * of the last day of that month is given.
 *
 * In the northern hemisphere, Daylight Savings Time runs for a period in
 * the middle of the year; thus, days between the start day and the end
 * day have DST active.  In the southern hemisphere, Daylight Savings Time
 * runs from the beginning of the year to some time in the middle of the
 * year, and from some time later in the year to the end of the year; thus,
 * days after the start day or before the end day have DST active.
 */
struct dstab {
	int	dayyr;
	int	daylb;
	int	dayle;
};

/*
 * The U.S. tables, including the latest hack.
 */
static struct dstab usdaytab[] = {
	1970,	119,	303,	/* 1970: last Sun. in Apr - last Sun. in Oct */
	1971,	119,	303,	/* 1971: last Sun. in Apr - last Sun. in Oct */
	1972,	119,	303,	/* 1972: last Sun. in Apr - last Sun. in Oct */
	1973,	119,	303,	/* 1973: last Sun. in Apr - last Sun. in Oct */
	1974,	5,	303,	/* 1974: Jan 6 - last Sun. in Oct */
	1975,	58,	303,	/* 1975: Last Sun. in Feb - last Sun. in Oct */
	1976,	119,	303,	/* 1976: last Sun. in Apr - last Sun. in Oct */
	1977,	119,	303,	/* 1977: last Sun. in Apr - last Sun. in Oct */
	1978,	119,	303,	/* 1978: last Sun. in Apr - last Sun. in Oct */
	1979,	119,	303,	/* 1979: last Sun. in Apr - last Sun. in Oct */
	1980,	119,	303,	/* 1980: last Sun. in Apr - last Sun. in Oct */
	1981,	119,	303,	/* 1981: last Sun. in Apr - last Sun. in Oct */
	1982,	119,	303,	/* 1982: last Sun. in Apr - last Sun. in Oct */
	1983,	119,	303,	/* 1983: last Sun. in Apr - last Sun. in Oct */
	1984,	119,	303,	/* 1984: last Sun. in Apr - last Sun. in Oct */
	1985,	119,	303,	/* 1985: last Sun. in Apr - last Sun. in Oct */
	1986,	119,	303,	/* 1986: last Sun. in Apr - last Sun. in Oct */
	0,	-90,	303,	/* 1987 on: first Sun. in Apr - last Sun. in Oct */
};

/*
 * Canada, same as the US, except no early 70's fluctuations and no late 80's
 * hack.
 */
static struct dstab candaytab[] = {
	1970,	119,	303,	/* 1970: last Sun. in Apr - last Sun. in Oct */
	1971,	119,	303,	/* 1971: last Sun. in Apr - last Sun. in Oct */
	1972,	119,	303,	/* 1972: last Sun. in Apr - last Sun. in Oct */
	1973,	119,	303,	/* 1973: last Sun. in Apr - last Sun. in Oct */
	1974,	119,	303,	/* 1974: last Sun. in Apr - last Sun. in Oct */
	1975,	119,	303,	/* 1975: Last Sun. in Apr - last Sun. in Oct */
	1976,	119,	303,	/* 1976: last Sun. in Apr - last Sun. in Oct */
	1977,	119,	303,	/* 1977: last Sun. in Apr - last Sun. in Oct */
	1978,	119,	303,	/* 1978: last Sun. in Apr - last Sun. in Oct */
	1979,	119,	303,	/* 1979: last Sun. in Apr - last Sun. in Oct */
	1980,	119,	303,	/* 1980: last Sun. in Apr - last Sun. in Oct */
	1981,	119,	303,	/* 1981: last Sun. in Apr - last Sun. in Oct */
	1982,	119,	303,	/* 1982: last Sun. in Apr - last Sun. in Oct */
	1983,	119,	303,	/* 1983: last Sun. in Apr - last Sun. in Oct */
	1984,	119,	303,	/* 1984: last Sun. in Apr - last Sun. in Oct */
	1985,	119,	303,	/* 1985: last Sun. in Apr - last Sun. in Oct */
	1986,	119,	303,	/* 1986: last Sun. in Apr - last Sun. in Oct */
	0,	-90,	303,	/* 1987 on: first Sun. in Apr - last Sun. in Oct */
};

/*
 * The Australian tables, for states with DST that don't shift the ending time
 * starting in 1986, but shift it starting in 1987.
 */
static struct dstab ausdaytab[] = {
	1970,	400,	0,	/* 1970: no daylight saving at all */
	1971,	303,	0,	/* 1971: daylight saving from last Sun. in Oct */
	1972,	303,	57,	/* 1972: Jan 1 -> Feb 27 & last Sun. in Oct -> Dec 31 */
	1973,	303,	-59,	/* 1973: -> first Sun. in Mar, last Sun. in Oct -> */
	1974,	303,	-59,	/* 1974: -> first Sun. in Mar, last Sun. in Oct -> */
	1975,	303,	-59,	/* 1975: -> first Sun. in Mar, last Sun. in Oct -> */
	1976,	303,	-59,	/* 1976: -> first Sun. in Mar, last Sun. in Oct -> */
	1977,	303,	-59,	/* 1977: -> first Sun. in Mar, last Sun. in Oct -> */
	1978,	303,	-59,	/* 1978: -> first Sun. in Mar, last Sun. in Oct -> */
	1979,	303,	-59,	/* 1979: -> first Sun. in Mar, last Sun. in Oct -> */
	1980,	303,	-59,	/* 1980: -> first Sun. in Mar, last Sun. in Oct -> */
	1981,	303,	-59,	/* 1981: -> first Sun. in Mar, last Sun. in Oct -> */
	1982,	303,	-59,	/* 1982: -> first Sun. in Mar, last Sun. in Oct -> */
	1983,	303,	-59,	/* 1983: -> first Sun. in Mar, last Sun. in Oct -> */
	1984,	303,	-59,	/* 1984: -> first Sun. in Mar, last Sun. in Oct -> */
	1985,	303,	-59,	/* 1985: -> first Sun. in Mar, last Sun. in Oct -> */
	1986,	-290,	-59,	/* 1986: -> first Sun. in Mar, first Sun. after Oct 17 -> */
	0,	-290,	79,	/* 1987 on: -> last Sun. before Mar 21, first Sun. after Oct 17 -> */
};

/*
 * The Australian tables, for states with DST that do shift the ending time
 * starting in 1986.  NSW does so; there seems to be a difference of opinion
 * about which other states do.  There is also a variation in 1983, but
 * Robert Elz didn't have it at hand when last he reported.
 * Extending the 1986 shift on to infinity is Elz's best guess.
 */
static struct dstab ausaltdaytab[] = {
	1970,	400,	0,	/* 1970: no daylight saving at all */
	1971,	303,	0,	/* 1971: daylight saving from last Sun. in Oct */
	1972,	303,	57,	/* 1972: Jan 1 -> Feb 27 & last Sun. in Oct -> Dec 31 */
	1973,	303,	-59,	/* 1973: -> first Sun. in Mar, last Sun. in Oct -> */
	1974,	303,	-59,	/* 1974: -> first Sun. in Mar, last Sun. in Oct -> */
	1975,	303,	-59,	/* 1975: -> first Sun. in Mar, last Sun. in Oct -> */
	1976,	303,	-59,	/* 1976: -> first Sun. in Mar, last Sun. in Oct -> */
	1977,	303,	-59,	/* 1977: -> first Sun. in Mar, last Sun. in Oct -> */
	1978,	303,	-59,	/* 1978: -> first Sun. in Mar, last Sun. in Oct -> */
	1979,	303,	-59,	/* 1979: -> first Sun. in Mar, last Sun. in Oct -> */
	1980,	303,	-59,	/* 1980: -> first Sun. in Mar, last Sun. in Oct -> */
	1981,	303,	-59,	/* 1981: -> first Sun. in Mar, last Sun. in Oct -> */
	1982,	303,	-59,	/* 1982: -> first Sun. in Mar, last Sun. in Oct -> */
	1983,	303,	-59,	/* 1983: -> first Sun. in Mar, last Sun. in Oct -> */
	1984,	303,	-59,	/* 1984: -> first Sun. in Mar, last Sun. in Oct -> */
	1985,	303,	-59,	/* 1985: -> first Sun. in Mar, last Sun. in Oct -> */
	0,	-290,	79,	/* 1986 on: -> last Sun. before Mar 21, first Sun. after Oct 17 -> */
};

/*
 * The European tables, based on investigations by PTB, Braunschweig, FRG.
 * Believed correct for:
 *	GB:	United Kingdom and Eire
 *	WE:	Portugal, Poland (in fact MET, following WE dst rules)
 *	ME:	Belgium, Luxembourg, Netherlands, Denmark, Norway,
 *		Austria, Czechoslovakia, Sweden, Switzerland,
 *		FRG, GDR,  France, Spain, Hungary, Italy, Yugoslavia,
 *		Western USSR (in fact EET+1; following ME dst rules)
 *	EE:	Finland, Greece, Israel?
 *
 * Problematic cases are:
 *	WE:	Iceland (no dst)
 *	EE:	Rumania, Turkey (in fact timezone EET+1)
 * Terra incognita:
 *		Albania (MET), Bulgaria (EET), Cyprus (EET)
 *
 * Years before 1986 are suspect (complicated changes caused
 * e.g. by enlargement of the European Community).
 * Years before 1983 are VERY suspect (sigh!).
 */
static struct dstab gbdaytab[] = {	/* GB and Eire */
	0,	89,	303,	/* all years: last Sun. in March - last Sun. in Oct */
};

static struct dstab cedaytab[] = {	/* Continental European */
	0,	89,	272,	/* all years: last Sun. in March - last Sun. in Sep */
};

static struct dayrules {
	int		dst_type;	/* number obtained from system */
	int		dst_hrs;	/* hours to add when dst on */
	struct	dstab *	dst_rules;	/* one of the above */
	enum {STH,NTH}	dst_hemi;	/* southern, northern hemisphere */
	int		dst_ontime;	/* hour when DST turns on */
	int		dst_offtime;	/* hour when DST turns off */
} dayrules [] = {
	DST_USA,	1,	usdaytab,	NTH,	2,	1,
	DST_CAN,	1,	candaytab,	NTH,	2,	1,
	DST_AUST,	1,	ausdaytab,	STH,	2,	2,
	DST_AUSTALT,	1,	ausaltdaytab,	STH,	2,	2,
	DST_GB,		1,	gbdaytab,	NTH,	1,	1,
	DST_WET,	1,	cedaytab,	NTH,	1,	1,
	DST_MET,	1,	cedaytab,	NTH,	2,	2,
	DST_EET,	1,	cedaytab,	NTH,	3,	3,
	DST_RUM,	1,	cedaytab,	NTH,	0,	0,
	DST_TUR,	1,	cedaytab,	NTH,	1,	0,
	-1,
};

struct tm	*gmtime();
char		*ct_numb();
struct tm	*localtime();
char	*ctime();
char	*ct_num();
char	*asctime();

char *
ctime(t)
time_t *t;
{
	return(asctime(localtime(t)));
}

struct tm *
localtime(tim)
time_t *tim;
{
	register int dayno;
	register struct tm *ct;
	register daylbegin, daylend;
	register struct dayrules *dr;
	register struct dstab *ds;
	int year;
	time_t copyt;
	struct timeval curtime;
	static struct timezone zone;
	static int init = 0;

	if (!init) {
		gettimeofday(&curtime, &zone);
		init++;
	}
	copyt = *tim - (time_t)zone.tz_minuteswest*60;
	ct = gmtime(&copyt);
	dayno = ct->tm_yday;
	for (dr = dayrules; dr->dst_type >= 0; dr++)
		if (dr->dst_type == zone.tz_dsttime)
			break;
	if (dr->dst_type >= 0) {
		year = ct->tm_year + 1900;
		for (ds = dr->dst_rules; ds->dayyr; ds++)
			if (ds->dayyr == year)
				break;
		daylbegin = sunday(ct, ds->daylb);	/* Sun on which dst starts */
		daylend = sunday(ct, ds->dayle);	/* Sun on which dst ends */
		switch (dr->dst_hemi) {
		case NTH:
		    if (!(
		       (dayno>daylbegin
			|| (dayno==daylbegin && ct->tm_hour>=dr->dst_ontime)) &&
		       (dayno<daylend
			|| (dayno==daylend && ct->tm_hour<dr->dst_offtime))
		    ))
			    return(ct);
		    break;
		case STH:
		    if (!(
		       (dayno>daylbegin
			|| (dayno==daylbegin && ct->tm_hour>=dr->dst_ontime)) ||
		       (dayno<daylend
			|| (dayno==daylend && ct->tm_hour<dr->dst_offtime))
		    ))
			    return(ct);
		    break;
		default:
		    return(ct);
		}
	        copyt += dr->dst_hrs*60*60;
		ct = gmtime(&copyt);
		ct->tm_isdst++;
	}
	return(ct);
}

/*
 * The argument is a 0-origin day number.
 * The value is the day number of the last
 * Sunday on or before the day (if "d" is positive)
 * or of the first Sunday on or after the day (if "d" is
 * negative).
 */
static
sunday(t, d)
register struct tm *t;
register int d;
{
	register int offset;	/* 700 if before, -700 if after */

	offset = 700;
	if (d < 0) {
		offset = -700;
		d = -d;
	}
	if (d >= 58)
		d += dysize(t->tm_year) - 365;
	return(d - (d - t->tm_yday + t->tm_wday + offset) % 7);
}

struct tm *
gmtime(tim)
time_t *tim;
{
	register int d0, d1;
	long hms, day;
	register int *tp;
	static struct tm xtime;

	/*
	 * break initial number into days
	 */
	hms = *tim % 86400;
	day = *tim / 86400;
	if (hms<0) {
		hms += 86400;
		day -= 1;
	}
	tp = (int *)&xtime;

	/*
	 * generate hours:minutes:seconds
	 */
	*tp++ = hms%60;
	d1 = hms/60;
	*tp++ = d1%60;
	d1 /= 60;
	*tp++ = d1;

	/*
	 * day is the day number.
	 * generate day of the week.
	 * The addend is 4, because 1/1/1970 was Thursday.
	 */

	xtime.tm_wday = (day + 4)%7;

	/*
	 * year number
	 */
	if (day>=0) for(d1=70; day >= dysize(d1); d1++)
		day -= dysize(d1);
	else for (d1=70; day<0; d1--)
		day += dysize(d1-1);
	xtime.tm_year = d1;
	xtime.tm_yday = d0 = day;

	/*
	 * generate month
	 */

	if (dysize(d1)==366)
		dmsize[1] = 29;
	for(d1=0; d0 >= dmsize[d1]; d1++)
		d0 -= dmsize[d1];
	dmsize[1] = 28;
	*tp++ = d0+1;
	*tp++ = d1;
	xtime.tm_isdst = 0;
	return(&xtime);
}

char *
asctime(t)
struct tm *t;
{
	register char *cp, *ncp;
	register int *tp;

	cp = cbuf;
	for (ncp = "Day Mon 00 00:00:00 1900\n"; *cp++ = *ncp++;);
	ncp = &"SunMonTueWedThuFriSat"[3*t->tm_wday];
	cp = cbuf;
	*cp++ = *ncp++;
	*cp++ = *ncp++;
	*cp++ = *ncp++;
	cp++;
	tp = &t->tm_mon;
	ncp = &"JanFebMarAprMayJunJulAugSepOctNovDec"[(*tp)*3];
	*cp++ = *ncp++;
	*cp++ = *ncp++;
	*cp++ = *ncp++;
	cp = ct_numb(cp, *--tp);
	cp = ct_numb(cp, *--tp+100);
	cp = ct_numb(cp, *--tp+100);
	cp = ct_numb(cp, *--tp+100);
	if (t->tm_year>=100) {
		cp[1] = '2';
		cp[2] = '0' + (t->tm_year-100) / 100;
	}
	cp += 2;
	cp = ct_numb(cp, t->tm_year+100);
	return(cbuf);
}

dysize(y)
{
	if((y%4) == 0)
		return(366);
	return(365);
}

static char *
ct_numb(cp, n)
register char *cp;
{
	cp++;
	if (n>=10)
		*cp++ = (n/10)%10 + '0';
	else
		*cp++ = ' ';
	*cp++ = n%10 + '0';
	return(cp);
}
