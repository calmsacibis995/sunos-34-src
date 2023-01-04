#ifndef lint
static	char sccsid[] = "@(#)date.c 1.1 86/09/24 SMI"; /* from UCB 4.5 83/07/01 */
#endif
/*
 * Date - print and set date
 */
#include <stdio.h>
#include <sys/time.h>
#include <utmp.h>
#define WTMP "/usr/adm/wtmp"

struct	timeval tv;
struct	timezone tz;
char	*ap, *ep, *sp;
int	uflag;

#ifdef S5EMUL
extern long timezone;
extern char *tzname[2];
#else
char	*timezone();
#endif

#define	MONTH	itoa(tp->tm_mon+1,cp,2)
#define	DAY	itoa(tp->tm_mday,cp,2)
#define	YEAR	itoa(tp->tm_year,cp,2)
#define	HOUR	itoa(tp->tm_hour,cp,2)
#define	MINUTE	itoa(tp->tm_min,cp,2)
#define	SECOND	itoa(tp->tm_sec,cp,2)
#define	JULIAN	itoa(tp->tm_yday+1,cp,3)
#define	WEEKDAY	itoa(tp->tm_wday,cp,1)
#define	MODHOUR	itoa(h,cp,2)

#define	dysize(A) (((A)%4)? 365: 366)

static	int	dmsize[12] =
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

char	month[12][3] = {
	"Jan","Feb","Mar","Apr",
	"May","Jun","Jul","Aug",
	"Sep","Oct","Nov","Dec"
};

char	days[7][3] = {
	"Sun","Mon","Tue","Wed",
	"Thu","Fri","Sat"
};

void	exit();
char	*itoa();

#ifdef S5EMUL
static char *usage = "usage: date [-u] [+format] [mmddhhmm[yy]] [-a sss.fff]\n";
#else
static char *usage = "usage: date [-u] [+format] [yymmddhhmm[.ss]] [-a sss.fff]\n";
#endif

struct utmp wtmp[2] = {
	{ "|", "", "", 0 },
	{ "{", "", "", 0 }
};

main(argc, argv)
	int argc;
	char *argv[];
{
	register char *aptr, *cp, c;
	int  h, hflag, i;
	long lseek();
	register struct tm *tp;
	char buf[200];
	register char *tzn;
	int wf, rc;

	rc = 0;
	gettimeofday(&tv, &tz);
#ifdef S5EMUL
	tzset();
#endif

	if (argc > 1) {
		if (*argv[1] == '+')	{
			hflag = 0;
			for (cp = buf; cp < &buf[200];)
				*cp++ = '\0';
			tp = localtime(&tv.tv_sec);
			aptr = argv[1];
			aptr++;
			cp = buf;
			while (c = *aptr++) {
				if (c == '%')
					switch(*aptr++) {
					case '%':
						*cp++ = '%';
						continue;
					case 'n':
						*cp++ = '\n';
						continue;
					case 't':
						*cp++ = '\t';
						continue;
					case 'm':
						cp = MONTH;
						continue;
					case 'd':
						cp = DAY;
						continue;
					case 'y':
						cp = YEAR;
						continue;
					case 'D':
						cp = MONTH;
						*cp++ = '/';
						cp = DAY;
						*cp++ = '/';
						cp = YEAR;
						continue;
					case 'H':
						cp = HOUR;
						continue;
					case 'M':
						cp = MINUTE;
						continue;
					case 'S':
						cp = SECOND;
						continue;
					case 'T':
						cp = HOUR;
						*cp++ = ':';
						cp = MINUTE;
						*cp++ = ':';
						cp = SECOND;
						continue;
					case 'j':
						cp = JULIAN;
						continue;
					case 'w':
						cp = WEEKDAY;
						continue;
					case 'r':
						if ((h = tp->tm_hour) >= 12)
							hflag++;
						if ((h %= 12) == 0)
							h = 12;
						cp = MODHOUR;
						*cp++ = ':';
						cp = MINUTE;
						*cp++ = ':';
						cp = SECOND;
						*cp++ = ' ';
						if (hflag)
							*cp++ = 'P';
						else
							*cp++ = 'A';
						*cp++ = 'M';
						continue;
					case 'h':
						for (i=0; i<3; i++)
							*cp++ = month[tp->tm_mon][i];
						continue;
					case 'a':
						for (i=0; i<3; i++)
							*cp++ = days[tp->tm_wday][i];
						continue;
					default:
						(void) fprintf(stderr, "date: bad format character - %c\n", *--aptr);
						exit(2);
					}
				*cp++ = c;
			}
			*cp = '\0';
			printf("%s\n", buf);
			exit(0);
		}

		if (strcmp(argv[1], "-u") == 0) {
			argc--;
			argv++;
			uflag++;
		} else if (strcmp(argv[1], "-a") == 0) {
			if (argc < 3) {
				printf(usage);
				exit(1);
			}
			get_adj(argv[2], &tv);
			if (adjtime(&tv, 0) < 0) {
				perror("date: Failed to adjust date");
			}
			exit(1);
		}
	}
	if (argc > 1) {
		ap = argv[1];
		if (gtime()) {
			printf(usage);
			exit(1);
		}
		if (uflag == 0) {
			/* convert to GMT assuming local time */
#ifdef S5EMUL
			tv.tv_sec += timezone;
#else
			tv.tv_sec += (long)tz.tz_minuteswest*60;
#endif
			/* now fix up local daylight time */
			if (localtime(&tv.tv_sec)->tm_isdst)
				tv.tv_sec -= 60*60;
		}
		wtmp[0].ut_time = tv.tv_sec;
		if (settimeofday(&tv, (struct timezone *)0) < 0) {
			rc++;
			perror("date: Failed to set date");
		} else if ((wf = open(WTMP, 1)) >= 0) {
			time(&wtmp[1].ut_time);
			lseek(wf, 0L, 2);
			write(wf, (char *)wtmp, sizeof(wtmp));
			close(wf);
		}
	}
	if (rc == 0)
		gettimeofday(&tv, (struct timezone *)0);
	if (uflag) {
		ap = asctime(gmtime(&tv.tv_sec));
		tzn = "GMT";
	} else {
		tp = localtime(&tv.tv_sec);
		ap = asctime(tp);
#ifdef S5EMUL
		tzn = tzname[tp->tm_isdst];
#else
		tzn = timezone(tz.tz_minuteswest, tp->tm_isdst);
#endif
	}
	printf("%.20s", ap);
	if (tzn)
		printf("%s", tzn);
	printf("%s", ap+19);
	exit(rc);
}

#ifdef S5EMUL
gtime()
{
	register int i, year, month;
	int day, hour, mins;
	long nt;

	month = gpair();
	if (month < 1 || month > 12)
		return (1);
	day = gpair();
	if (day < 1 || day > 31)
		return (1);
	hour = gpair();
	if (hour == 24) {
		hour = 0;
		day++;
	}
	mins = gpair();
	if (mins < 0 || mins > 59)
		return (1);
	year = gpair();
	if (year < 0) {
		(void) time(&nt);
		year = localtime(&nt)->tm_year;
	}
	if (*ap == 'p')
		hour += 12;
	if (hour < 0 || hour > 23)
		return (1);
	tv.tv_sec = 0;
	year += 1900;
	for (i = 1970; i < year; i++)
		tv.tv_sec += dysize(i);
	/* Leap year */
	if (dysize(year) == 366 && month >= 3)
		tv.tv_sec += 1;
	while (--month)
		tv.tv_sec += dmsize[month-1];
	tv.tv_sec += day-1;
	tv.tv_sec = 24*tv.tv_sec + hour;
	tv.tv_sec = 60*tv.tv_sec + mins;
	tv.tv_sec = 60*tv.tv_sec;
	return (0);
}

gpair()
{
	register int c, d;
	register char *cp;

	cp = ap;
	if (*cp == 0)
		return (-1);
	c = (*cp++ - '0') * 10;
	if (c < 0 || c > 100)
		return (-1);
	if (*cp == 0)
		return (-1);
	if ((d = *cp++ - '0') < 0 || d > 9)
		return (-1);
	ap = cp;
	return (c+d);
}

#else

gtime()
{
	register int i, year, month;
	int day, hour, mins, secs;
	struct tm *L;
	char x;

	ep = ap;
	while(*ep) ep++;
	sp = ap;
	while(sp < ep) {
		x = *sp;
		*sp++ = *--ep;
		*ep = x;
	}
	sp = ap;
	gettimeofday(&tv, (struct timezone *)0);
	L = localtime(&tv.tv_sec);
	secs = gp(-1);
	if (*sp != '.') {
		mins = secs;
		secs = 0;
	} else {
		sp++;
		mins = gp(-1);
	}
	hour = gp(-1);
	day = gp(L->tm_mday);
	month = gp(L->tm_mon+1);
	year = gp(L->tm_year);
	if (*sp)
		return (1);
	if (month < 1 || month > 12 ||
	    day < 1 || day > 31 ||
	    mins < 0 || mins > 59 ||
	    secs < 0 || secs > 59)
		return (1);
	if (hour == 24) {
		hour = 0;
		day++;
	}
	if (hour < 0 || hour > 23)
		return (1);
	tv.tv_sec = 0;
	year += 1900;
	for (i = 1970; i < year; i++)
		tv.tv_sec += dysize(i);
	/* Leap year */
	if (dysize(year) == 366 && month >= 3)
		tv.tv_sec++;
	while (--month)
		tv.tv_sec += dmsize[month-1];
	tv.tv_sec += day-1;
	tv.tv_sec = 24*tv.tv_sec + hour;
	tv.tv_sec = 60*tv.tv_sec + mins;
	tv.tv_sec = 60*tv.tv_sec + secs;
	return (0);
}

gp(dfault)
{
	register int c, d;

	if (*sp == 0)
		return (dfault);
	c = (*sp++) - '0';
	d = (*sp ? (*sp++) - '0' : 0);
	if (c < 0 || c > 9 || d < 0 || d > 9)
		return (-1);
	return (c+10*d);
}

#endif

char *
itoa(i,ptr,dig)
register  int	i;
register  int	dig;
register  char	*ptr;
{
	switch(dig)	{
		case 3:
			*ptr++ = i/100 + '0';
			i = i - i / 100 * 100;
		case 2:
			*ptr++ = i / 10 + '0';
		case 1:	
			*ptr++ = i % 10 + '0';
	}
	return (ptr);
}

get_adj(cp, tp)
	char *cp;
	struct timeval *tp;
{
	register int mult;
	int sign;

	tp->tv_sec = tp->tv_usec = 0;
	if (*cp == '-') {
		sign = -1;
		cp++;
	} else {
		sign = 1;
	}
	while (*cp >= '0' && *cp <= '9') {
		tp->tv_sec *= 10;
		tp->tv_sec += *cp++ - '0';
	}
	if (*cp == '.') {
		cp++;
		mult = 100000;
		while (*cp >= '0' && *cp <= '9') {
			tp->tv_usec += (*cp++ - '0') * mult;
			mult /= 10;
		}
	}
	if (*cp) {
		printf(usage);
		exit(1);
	}
	tp->tv_sec *= sign;
	tp->tv_usec *= sign;
}
