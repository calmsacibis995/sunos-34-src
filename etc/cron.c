#ifndef lint
static	char sccsid[] = "@(#)cron.c 1.1 86/09/24 Copyr 1983 Sun Micro"; /* from UCB 4.7 */
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * cron - periodic event processor
 *
 * Cron itself runs at least every maxsleep but usually before then to run
 * a program.  If the user changes crontab (or the date) and wants cron to
 * immediately take another look at crontab he can send cron's process a
 * SIGHUP.
 */
#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/wait.h>

#define	LISTS	(2*BUFSIZ)
#define	MAXLIN	BUFSIZ

#define	EXACT	123
#define	ANY	124
#define	LIST	125
#define	RANGE	126
#define	EOS	127

char	crontab[]	= "/usr/lib/crontab";
char	cronlog[]	= "/usr/adm/cronlog";

time_t	itime;		/* cron's internal idea of the time */
char	*list;
unsigned listsize;
int	maxsleep = 3600;	/* max seconds that will sleep */
char	lorange[] = { 0,0,1,1,1 }, hirange[] = { 59,23,31,12,7 };

struct	tm *cronlocaltime();
char	*cmp();
char	*malloc();
char	*realloc();

main()
{
	time_t filetime = 0;
	int sleeptime = 0, pausemask, oldmask;

	initialize();
	/*
	 * Only allow SIGHUP while paused.
	 */
	pausemask = sigblock(1<<(SIGHUP-1));
	time(&itime);
	for (;;) {
		struct	stat cstat;

#ifndef DEBUG
		if (access(cronlog, W_OK) == 0) {
			freopen(cronlog, "a", stdout);
			setbuf(stdout, NULL);
			close(fileno(stderr));
			dup(1);
		} else {
			freopen("/dev/null", "w", stdout);
			freopen("/dev/null", "w", stderr);
		}
#endif
		if (sleeptime > 0) {
			int left;

#ifdef	DEBUG
			printf("CRON SLEEP %d\n", sleeptime);
#endif	DEBUG
			/*
			 * Block SIGALRM to prevent interrupt between
			 * call to alarm() and call to sigpause().
			 */
			oldmask = sigblock(1<<(SIGALRM-1));
			alarm(sleeptime);	/* set alarm */
			sigpause(pausemask);	/* Wait for ALRM or HUP. */
			left = alarm(0);	/* turn off alarm */
			sigsetmask(oldmask);	/* allow ALRM, but not HUP */
			itime += sleeptime - left;
		}
#ifdef	DEBUG
		prtime(time(0));
		printf(": CRON AWAKE sleeptime=%d\n", sleeptime);
#endif	DEBUG
		/*
		 * Get statistics on crontab
		 */
		if (stat(crontab, &cstat) == -1) {
			perror("cron (stat)");
			sleeptime = maxsleep;
			continue;
		}
		/*
		 * If crontab has changed, reread it.
		 */
		if (cstat.st_mtime > filetime) {
			filetime = cstat.st_mtime;
			readcrontab();
		}
		/*
		 * See if anything to run.
		 */
		runmatches(itime);
		/*
		 * Determine when next event should be run.
		 */
		sleeptime = figureeventwait(itime);
	}
}

runmatches(curtime)
	time_t	curtime;
{
	char	*cp;
	struct	tm *loct = cronlocaltime(curtime);

	for (cp = list; *cp != EOS;) {
		if (match(&cp, loct))
			ex(cp);
		while (*cp++ != 0)
			;
	}
}

figureeventwait(curtime)
	time_t	curtime;
{
	char *cp;
	register time_t eventtime;
	register struct tm *eventloct;
	time_t t;
	int i;

	/*
	 * Very computationally brute force approach to finding next event;
	 * forgivable because will do only every maxsleep seconds.
	 * Simulate every minute between next minute and curtime+maxsleep
	 * to find minimum interval.
	 */
	for (eventtime = ((curtime+60)/60)*60;
	    eventtime < curtime+maxsleep; eventtime += 60) {
		eventloct = cronlocaltime(eventtime);
		for (cp = list; *cp != EOS;) {
			if (match(&cp, eventloct))
				goto out;
			while (*cp++ != 0)
				;
		}
	}
out:
	time(&t);
	i = eventtime - t;
	/*
	 * If our internal idea of the time is very far
	 * off then assume someone has reset the date
	 * and resync our idea of the time.
	 */
	if (i < -maxsleep || i > 2*maxsleep) {
		itime = t;
		i = 60 - t%60;
		return (i);
	}
	/*
	 * If time we're waiting for has already passed
	 * then just move time forward and don't sleep.
	 */
	if (i <= 0) {
#ifdef DEBUG
t = eventtime;
printf("CATCH UP eventtime %-24.24s i %d\n", ctime(&t), i);
#endif
		i = 0;
	}
	/*
	 * Move internal time forward to account for any
	 * processing time and so internal time plus sleep
	 * time puts us at destination time.
	 */
	itime = eventtime - i;
	return (i);
}

ex(s)
char *s;
{
	int rfork, st;

	prtime(time(0)); fputs(": ", stdout); fputs(s, stdout);
#ifndef DEBUG
again:
	if (rfork = fork()) {
		if (rfork == -1) {
			sleep(20);
			goto again;
		}
		wait(&st);
		return;
	}
again2:
	if (rfork = fork()) {
		if (rfork == -1) {
			sleep(20);
			goto again2;
		}
		exit(0);
	}
	freopen("/dev/null", "r", stdin);
	freopen("/dev/null", "w", stdout);	/* until "force appends to */
	freopen("/dev/null", "w", stderr);	/* end of file" works */
	execl("/bin/sh", "sh", "-c", s, 0);
	exit(0);
#endif
}

onhup()
{
}

onalrm()
{
}

int
match(cp, loct)
	register char **cp;
	register struct	tm *loct;
{
	int	cancel_ex = 0;

	*cp = cmp(*cp, loct->tm_min, &cancel_ex);
	*cp = cmp(*cp, loct->tm_hour, &cancel_ex);
	*cp = cmp(*cp, loct->tm_mday, &cancel_ex);
	*cp = cmp(*cp, loct->tm_mon, &cancel_ex);
	*cp = cmp(*cp, loct->tm_wday, &cancel_ex);
	return(!cancel_ex);
}

char *
cmp(p, v, cancel_ex)
char *p;
int *cancel_ex;
{
	register char *cp;

	cp = p;
	switch(*cp++) {

	case EXACT:
		if (*cp++ != v)
			(*cancel_ex)++;
		return(cp);

	case ANY:
		return(cp);

	case LIST:
		while(*cp != LIST)
			if(*cp++ == v) {
				while(*cp++ != LIST)
					;
				return(cp);
			}
		(*cancel_ex)++;
		return(cp+1);

	case RANGE:
		if(cp[0] < cp[1]) {
			if(!(cp[0]<=v && cp[1]>=v))
				(*cancel_ex)++;
		} else if(!(v>=cp[0] || v<=cp[1]))
			(*cancel_ex)++;
		return(cp+2);
	}
	if(cp[-1] != v)
		(*cancel_ex)++;
	return(cp);
}

/*
 * The list is made up of separate entries, each of which has for the
 * first 5 fields (in order: minute, hour, month-day, month, weekday) 
 * an encoding of the following 4 possiblities (# is a number < 100):
 *	ANY|LIST##...##LIST|RANGE##|EXACT#
 * The sixth field is:
 *	<text>EOS
 * and the entire list is terminated by:
 *	EOS
 */
int	line;	/* line number in crontab, for error messages */

readcrontab()
{
	register i, c;
	register char *cp;
	register char *ocp, *cp2;
	register int n;

	prtime(time(0)); fputs(": read crontab\n", stdout);
	freopen(crontab, "r", stdin);
	if (list) {
		free(list);
		list = realloc(list, LISTS);
	} else
		list = malloc(LISTS);
	listsize = LISTS;
	cp = list;
	line = 0;

loop:
	line++;
	if(cp > list+listsize-MAXLIN) {
		char *olist;
		listsize += LISTS;
		olist = list;
		free(list);
		list = realloc(list, listsize);
		cp = list + (cp - olist);
	}
	ocp = cp;
	for(i=0;; i++) {
		do
			c = getchar();
		while(c == ' ' || c == '\t')
			;
		if(c == EOF || c == '\n')
			goto ignore;
		if(i == 5)
			break;
		if(c == '*') {
			*cp++ = ANY;
			continue;
		}
		if ((n = number(c, lorange[i], hirange[i])) < 0)
			goto ignore;
		c = getchar();
		if(c == ',')
			goto mlist;
		if(c == '-')
			goto mrange;
		if(c != '\t' && c != ' ')
			goto ignore;
		*cp++ = EXACT;
		*cp++ = n;
		continue;

	mlist:
		*cp++ = LIST;
		*cp++ = n;
		do {
			if ((n = number(getchar(), lorange[i], hirange[i])) < 0)
				goto ignore;
			*cp++ = n;
			c = getchar();
		} while (c==',');
		if(c != '\t' && c != ' ')
			goto ignore;
		*cp++ = LIST;
		continue;

	mrange:
		*cp++ = RANGE;
		*cp++ = n;
		if ((n = number(getchar(), lorange[i], hirange[i])) < 0)
			goto ignore;
		c = getchar();
		if(c != '\t' && c != ' ')
			goto ignore;
		*cp++ = n;
	}

	i = 0;
	while(c != '\n') {
		if(c == EOF)
			goto ignore;
		if(c == '%') {
			if (i == 0) {
				i++;
				for (cp2 = "<< __GNOME__"; *cp++ = *cp2++;)
						;
				cp--;
			}
			c = '\n';
		}
		*cp++ = c;
		c = getchar();
	}
	*cp++ = '\n';
	if (i)
		for (cp2 = "__GNOME__\n"; *cp++ = *cp2++;)
			;
	else
		*cp++ = 0;
	goto loop;

ignore:
	cp = ocp;
	while(c != '\n') {
		if(c == EOF) {
			*cp++ = EOS;
			*cp++ = EOS;
			fflush(stdin);
			return;
		}
		c = getchar();
	}
	printf("ignoring line %d\n", line);
	goto loop;
}

number(c, lowv, highv)
register c;
{
	register n = 0;

	while (isdigit(c)) {
		n = n*10 + c - '0';
		c = getchar();
	}
	ungetc(c, stdin);
	if (n<lowv || n>highv) {
		printf("line %d, value '%d' out of range (%d, %d)\n",
		    line, n, lowv, highv);
		return (-1);
	}
	return(n);
}

struct tm *
cronlocaltime(time)
	time_t	time;
{
	extern struct tm *localtime();
	register struct tm *loct;
	static struct tm clt;

	/*
	 * Make a static copy of the tm struct
	 * so it isn't destroyed later when we
	 * call ctime.
	 */
	loct = localtime(&time);
	clt = *loct;
	loct = &clt;

	/*
	 * Adjust the current local time to correspond
	 * to user's model of month and day-of-the-week indices
	 * (1-12 for month & sunday is 7, not 0).
	 */
	loct->tm_mon++;
	if (loct->tm_wday == 0)
		loct->tm_wday = 7;
	return (loct);
}

prtime(t)
	time_t t;
{
	char *p;

	p = ctime(&t);
	p[24] = '\0';
	fputs(p, stdout);
}

initialize()
{
	int onhup(), onalrm();
	int rfork, s;

	setuid(0);
#ifndef DEBUG
	if (rfork = fork())
		if (rfork == -1) {
			write(2, "cron: cannot fork\n", 18);
			exit(2);
		} else
			exit(0);
	for (s = 3; s < getdtablesize(); s++)
		close(s);
	if ((s = open("/dev/tty", O_RDWR)) > 0) {
		ioctl(s, TIOCNOTTY, 0);
		close(s);
	}
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
#else
	setbuf(stdout, NULL);
#endif
	/*
	 * Setup signal handlers.
	 */
	signal(SIGHUP, onhup);
	signal(SIGALRM, onalrm);
	chdir("/");
}
