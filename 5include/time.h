/*	@(#)time.h 1.1 86/09/24 SMI; from S5R2 1.1	*/

struct	tm {	/* see ctime(3) */
	int	tm_sec;
	int	tm_min;
	int	tm_hour;
	int	tm_mday;
	int	tm_mon;
	int	tm_year;
	int	tm_wday;
	int	tm_yday;
	int	tm_isdst;
};
extern struct tm *gmtime(), *localtime();
extern char *ctime(), *asctime();
extern void tzset();
extern long timezone;
extern int daylight;
extern char *tzname[];
