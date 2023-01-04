/*	@(#)time.h 1.1 86/09/24 SMI; not from UCB */

/*
 * Structure returned by gmtime and localtime calls (see ctime(3)).
 */
struct tm {
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

extern	struct tm *gmtime(), *localtime();
extern	char *asctime(), *ctime();
