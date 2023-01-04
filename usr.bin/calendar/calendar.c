#ifndef lint
static	char sccsid[] = "@(#)calendar.c 1.1 86/09/25 SMI"; /* from UCB 4.5 84/05/07 */
#endif

/* /usr/lib/calendar produces an egrep -f file
   that will select today's and tomorrow's
   calendar entries, with special weekend provisions

   used by calendar command
*/
#include <sys/time.h>

#define DAY (3600*24L)

char *month[] = {
	"[Jj]an",
	"[Ff]eb",
	"[Mm]ar",
	"[Aa]pr",
	"[Mm]ay",
	"[Jj]un",
	"[Jj]ul",
	"[Aa]ug",
	"[Ss]ep",
	"[Oo]ct",
	"[Nn]ov",
	"[Dd]ec"
};
struct tm *localtime();

tprint(t)
long t;
{
	struct tm *tm;
	tm = localtime(&t);
	printf("(^|[ \t(,;])(((%s[^ \t]*|\\*)[ \t]*|(0%d|%d|\\*)/)0*%d)([^0123456789]|$)\n",
		month[tm->tm_mon],
		tm->tm_mon + 1, tm->tm_mon + 1, tm->tm_mday);
}

main()
{
	long t;
	time(&t);
	tprint(t);
	switch(localtime(&t)->tm_wday) {
	case 5:
		t += DAY;
		tprint(t);
	case 6:
		t += DAY;
		tprint(t);
	default:
		t += DAY;
		tprint(t);
	}
}
