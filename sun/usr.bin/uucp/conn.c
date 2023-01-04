/*	conn.c	1.1	86/09/25	*/
	/*  conn 3.12  1/5/80  13:51:15  */
#define CONN
#include "uucp.h"
#include "dial.h"
#include <signal.h>
#include <sgtty.h>
#include <setjmp.h>
#include <ctype.h>
#include <sys/types.h>
#include <time.h>


#ifdef DATAKIT
#include <dk.h>
#endif

#define F_NAME 0
#define F_TIME 1
#define F_LINE 2
#define F_CLASS 3	/* an optional prefix and the speed */
#define F_PHONE 4
#define F_LOGIN 5

jmp_buf Sjbuf;
int alarmtr();

/*******
 *	conn(system)
 *	char *system;
 *
 *	conn - place a telephone call to system and
 *	login, etc.
 *
 *	return codes:
 *		CF_SYSTEM: don't know system
 *		CF_TIME: wrong time to call
 *		CF_DIAL: call failed
 *		CF_LOGIN: login/password dialog failed
 *
 *		>0  - file no.  -  connect ok
 *
 */

conn(system)
char *system;
{
	int ret, nf;
	int fn;
	char *flds[50];
	FILE *fsys;
	int fcode = 0;


	fsys = fopen(SYSFILE, "r");
	ASSERT(fsys != NULL, "CAN'T OPEN", SYSFILE, 0);
	DEBUG(4, "finds %s\n", "called");
	while((nf = finds(fsys, system, flds)) > 0) {
		DEBUG(4, "getto %s\n", "called");
		if ((fn = getto(flds)) > 0)
			break;
		fcode = CF_DIAL;
	}
	fclose(fsys);
	if (nf < 0)
		return(fcode ? fcode : nf);

	DEBUG(4, "login %s\n", "called");
	ret = login(nf, flds, fn);
	if (ret < 0) {
		close(fn);
		return(CF_LOGIN);
	}
	return(fn);
}


/***
 *	getto(flds)		connect to remote machine
 *	char *flds[];
 *
 *	return codes:
 *		>0  -  file number - ok
 *		FAIL  -  failed
 */

getto(flds)
char *flds[];
{
	DEBUG(4, "call: no. %s ", flds[F_PHONE]);
	DEBUG(4, "for sys %s ", flds[F_NAME]);

	if (prefix("ACU", flds[F_LINE]))
		return(call(flds));

#ifdef DATAKIT
	else if (prefix("DK", flds[F_LINE]))
		return(dkcall(flds));
#endif

	else
		return(direct(flds));
}



/***
 *	call(flds)		call remote machine
 *	char *flds[];
 *
 *	"flds" contains the call information (name, date, type, speed,
 *	phone no. ...
 *	Ndev has the device no.
 *
 *	return codes:
 *		>0  -  file number  -  ok
 *		FAIL  -  failed
 */

call(flds)
char *flds[];
{
	char phone[MAXPH+1];
	int dcr, i;

	exphone(flds[F_PHONE], phone);
	for (i = 0; i < TRYCALLS; i++) {
		DEBUG(4, "Dial %s\n", phone);
		dcr = dialup(phone, flds);
		DEBUG(4, "dcr returned as %d\n", dcr);
		if (dcr != FAIL)
			break;
	}
	return(dcr);

}


/***
 *	exphone - expand phone number for given prefix and number
 *
 *	return code - none
 */

exphone(in, out)
char *in, *out;
{
	FILE *fn;
	char pre[MAXPH], npart[MAXPH], tpre[MAXPH], p[MAXPH];
	char buf[BUFSIZ];
	char *s1;

	if (!isalpha(*in)) {
		strcpy(out, in);
		return;
	}

	s1=pre;
	while (isalpha(*in))
		*s1++ = *in++;
	*s1 = '\0';
	s1 = npart;
	while (*in != '\0')
		*s1++ = *in++;
	*s1 = '\0';

	tpre[0] = '\0';
	fn = fopen(DIALFILE, "r");
	if (fn != NULL) {
		while (cfgets(buf, BUFSIZ, fn)) {
			sscanf(buf, "%s%s", p, tpre);
			if (strcmp(p, pre) == SAME)
				break;
			tpre[0] = '\0';
		}
		fclose(fn);
	}

	strcpy(out, tpre);
	strcat(out, npart);
	return;
}

	/*  file descriptor for call unit  */
int Dnf = 0;

/* dialing procedures */
int dndial(), dfdial(), vendial(), hayesdial();

struct dialers {
	char	*type;		/* e.g. "ACU", "ACUDF03", "ACUVENTEL" */
	int	(*func)();	/* procedure to do the dialing */
} dialers[] = {
	"ACUDF02",	dfdial,
	"ACUDF03",	dfdial,
	"ACUVENTEL",	vendial,
	"ACUHAYES",	hayesdial,
	"ACUDN11",	dndial,
	"ACU",		dndial,		/* must be last */
	0,		0
};

/***
 *	dialup(ph, flds)	dial remote machine
 *	char *ph;
 *	char *flds[];
 *
 *	return codes:
 *		file descriptor  -  succeeded
 *		FAIL  -  failed
 *
 *	Modified 6/15/82 by Bill Shannon (Sun)
 *	Table driven dialer support added.
 */

dialup(ph, flds)
char *ph;
char *flds[];
{
#ifdef DIALOUT
	int dcf;
	if ((dcf = dialout(ph, flds[F_CLASS])) < 0)
		return(FAIL);
	return(dcf);
#endif

#ifndef DIALOUT
	register struct dialers *dp;
	char dnname[20];
	struct Devices dev;
	FILE *dfp;

	dfp = fopen(DEVFILE, "r");
	ASSERT(dfp != NULL, "CAN'T OPEN", DEVFILE, 0);
	while (rddev(dfp, &dev) != FAIL) {
		if (strcmp(flds[F_CLASS], dev.D_class) != SAME)
			continue;
		if (!prefix(flds[F_LINE], dev.D_type))
			continue;
		if (mlock(dev.D_line) == FAIL)
			continue;
		sprintf(dnname, "/dev/%s", dev.D_calldev);
		if ((Dnf = open(dnname, 2)) < 0) {
			delock(dev.D_line);
			continue;
		}
#ifdef TIOCSPGRP
		{
		int pgrp = getpgrp(0);
		ioctl(Dnf, TIOCSPGRP, &pgrp);
		}
#endif
		for (dp = dialers; dp->type; dp++)
			if (prefix(dp->type, dev.D_type)) {
				fclose(dfp);
				logent(dev.D_type, "DIALING");
				DEBUG(4, "Using dialer type %s\n", dev.D_type);
				return((*dp->func)(&dev, ph));
			}
		clsacu();
		logent(dev.D_type, "NO SUPPORT FOR DIALER");
		DEBUG(4, "NO SUPPORT FOR DIALER TYPE %s\n", dev.D_type);
		delock(dev.D_line);
	}
	fclose(dfp);
	logent("DEVICE", "NO");
	DEBUG(4, "NO DEVICE %s", "");
	return(FAIL);
#endif
}

/***
 *	rddev - read and decode a line from device file
 *
 *	return code - FAIL at end-of file; 0 otherwise
 */

rddev(fp, dev)
struct Devices *dev;
FILE *fp;
{
	char *fdig();
	char buf[BUFSIZ];
	int na;

	if (!cfgets(buf, BUFSIZ, fp))
		return(FAIL);

	na = sscanf(buf, "%s%s%s%s", dev->D_type, dev->D_line,
	  dev->D_calldev, dev->D_class);
	ASSERT(na >= 4, "BAD LINE", buf, 0);
	dev->D_speed = atoi(fdig(dev->D_class));
	return(0);
}

/***
 *	clsacu()	close call unit
 *
 *	return codes:  none
 */

clsacu()
{
	if (Dnf > 0) {
		close(Dnf);
		sleep(5);
		Dnf = 0;
	}
	return;
}


/***
 *	direct(flds)	connect to hardware line
 *	char *flds[];
 *
 *	return codes:
 *		>0  -  file number  -  ok
 *		FAIL  -  failed
 */

direct(flds)
char *flds[];
{
	int dcr, status;
	struct Devices dev;
	char dcname[20];
	FILE *dfp;
	dfp = fopen(DEVFILE, "r");
	ASSERT(dfp != NULL, "CAN'T OPEN", DEVFILE, 0);
	while ((status = rddev(dfp, &dev)) != FAIL) {
		if (strcmp(flds[F_CLASS], dev.D_class) != SAME)
			continue;
		if (strcmp(flds[F_LINE], dev.D_line) != SAME)
			continue;
		if (mlock(dev.D_line) != FAIL)
			break;
	}
	fclose(dfp);
	if (status == FAIL) {
		logent("DEVICE", "NO");
		DEBUG(4, "NO DEVICE %s", "");
		return(FAIL);
	}

	sprintf(dcname, "/dev/%s", dev.D_line);
	signal(SIGALRM, alarmtr);
	alarm(10);
	if (setjmp(Sjbuf)) {
		delock(dev.D_line);
		return(FAIL);
	}
	dcr = open(dcname, 2); /* read/write */
	alarm(0);
	if (dcr < 0) {
		delock(dev.D_line);
		return(FAIL);
	}
	fflush(stdout);
	fixline(dcr, dev.D_speed);
	return(dcr);
}

#ifdef DATAKIT

#define DKTRIES 2

/***
 *	dkcall(flds)	make datakit connection
 *
 *	return codes:
 *		>0 - file number - ok
 *		FAIL - failed
 */

dkcall(flds)
char *flds[];
{
	int dkphone;
	register char *cp;
	register ret, i;

	if (setjmp(Sjbuf))
		return(FAIL);
	signal(SIGALRM, alarmtr);
	dkphone = 0;
	cp = flds[F_PHONE];
	while(*cp)
		dkphone = 10 * dkphone + (*cp++ - '0');
	DEBUG(4, "dkphone (%d) ", dkphone);
	for (i = 0; i < DKTRIES; i++) {
		ret = dkdial(D_SH, dkphone, 0);
		DEBUG(4, "dkdial (%d)\n", ret);
		if (ret > -1)
			break;
	}
	return(ret);
}
#endif

#define MAXC 300

/***
 *	finds(fsys, sysnam, flds)	set system attribute vector
 *
 *	return codes:
 *		>0  -  number of arguments in vector - succeeded
 *		CF_SYSTEM  -  system name not found
 *		CF_TIME  -  wrong time to call
 */

finds(fsys, sysnam, flds)
char *sysnam, *flds[];
FILE *fsys;
{
	static char info[MAXC];
	char sysn[8];
	int na;
	int fcode = 0;

	/* format of fields
	 *	0 name;
	 *	1 time
	 *	2 acu/hardwired
	 *	3 speed
	 *	etc
	 */
	while (cfgets(info, MAXC, fsys) != NULL) {
		na = getargs(info, flds);
		sprintf(sysn, "%.7s", flds[F_NAME]);
		if (strcmp(sysnam, sysn) != SAME)
			continue;
		if (ifdate(flds[F_TIME]))
			/*  found a good entry  */
			return(na);
		logent(sysnam, "WRONG TIME TO CALL");
		fcode = CF_TIME;
	}
	return(fcode ? fcode : CF_SYSTEM);
}

/***
 *	login(nf, flds, dcr)		do log conversation
 *	char *flds[];
 *	int nf;
 *
 *	return codes:  0  |  FAIL
 */

login(nf, flds, fn)
char *flds[];
int nf, fn;
{
	char *want, *altern;
	extern char *index();
	int k, ok;

	ASSERT(nf > 4, "TOO FEW LOG FIELDS", "", nf);
	for (k = F_LOGIN; k < nf; k += 2) {
		want = flds[k];
		ok = FAIL;
		while (ok != 0) {
			if (want == NULL) {
				logent("LOGIN", "FAILED");
				return(FAIL);
			}
			altern = index(want, '-');
			if (altern != NULL)
				*altern++ = '\0';
			DEBUG(4, "wanted %s ", want);
			ok = expect(want, fn);
			DEBUG(4, "got %s\n", ok ? "?" : "that");
			if (ok == 0)
				break;
			if (altern == NULL) {
				logent("LOGIN", "FAILED");
				return(FAIL);
			}
			want = index(altern, '-');
			if (want != NULL)
				*want++ = '\0';
			sendthem(altern, fn);
		}
		sleep(2);
		sendthem(flds[k+1], fn);
	}
	return(0);
}


struct sg_spds {int sp_val, sp_name;} spds[] = {
	{ 300,  B300},
	{1200, B1200},
	{2400, B2400},
	{4800, B4800},
	{9600, B9600},
	{0, 0} };

int	linebaudrate;		/* for pk1/pkcget() to know whether to sleep */

/***
 *	fixline(tty, spwant)	set speed/echo/mode...
 *	int tty, spwant;
 *
 *	return codes:  none
 */

fixline(tty, spwant)
int tty, spwant;
{
	struct sgttyb ttbuf;
	struct sg_spds *ps;
	int speed = -1;
	int ret;

	for (ps = spds; ps->sp_val; ps++)
		if (ps->sp_val == spwant)
			speed = ps->sp_name;
	ASSERT(speed >= 0, "BAD SPEED", "", speed);
	ioctl(tty, TIOCGETP, &ttbuf);
	ttbuf.sg_flags =(ANYP|RAW);
	ttbuf.sg_ispeed = ttbuf.sg_ospeed = speed;
	ret = ioctl(tty, TIOCSETP, &ttbuf);
	ASSERT(ret >= 0, "RETURN FROM STTY", "", ret);
	ioctl(tty, TIOCHPCL, STBNULL);
	ioctl(tty, TIOCEXCL, STBNULL);
	linebaudrate = spwant;
	return;
}


#define MR 1000


/***
 *	expect(str, fn)	look for expected string
 *	char *str;
 *
 *	return codes:
 *		0  -  found
 *		FAIL  -  lost line or too many characters read
 *		some character  -  timed out
 */

expect(str, fn)
char *str;
int fn;
{
	static char rdvec[MR];
	char *rp = rdvec;
	int kr;
	char nextch;

	if (strcmp(str, "\"\"") == SAME)
		return(0);
	*rp = 0;
	if (setjmp(Sjbuf)) {
		return(FAIL);
	}
	signal(SIGALRM, alarmtr);
	alarm(MAXMSGTIME);
	while (notin(str, rdvec)) {
		kr = read(fn, &nextch, 1);
		if (kr <= 0) {
			alarm(0);
			DEBUG(4, "lost line kr - %d\n", kr);
			logent("LOGIN", "LOST LINE");
			return(FAIL);
		}
		{
		int c;
		c = nextch & 0177;
		DEBUG(4, c >= 040 ? "%c" : "\\%03o", c);
		}
		if ((*rp = nextch & 0177) != '\0')
			rp++;
		if (rp >= rdvec + MR) {
			alarm(0);
			return(FAIL);
		}
		*rp = '\0';
	}
	alarm(0);
	return(0);
}


/***
 *	expectline(fn)	get a line from fn with timeout
 *
 *	return codes:
 *		!0  -  found
 *		0  -  lost line or too many characters read or timed out
 */

char *
expectline(fn)
int fn;
{
	static char rdvec[MR];
	register char *rp = rdvec;
	int kr;
	char nextch;

	*rp = '\0';
	if (setjmp(Sjbuf)) {
		return(0);
	}
	signal(SIGALRM, alarmtr);
	alarm(2*MAXMSGTIME);
	for(;;) {
		kr = read(fn, &nextch, 1);
		if (kr <= 0) {
			alarm(0);
			DEBUG(4, "lost line kr - %d\n", kr);
			logent("expectline", "LOST LINE");
			return(0);
		}
		nextch &= 0177;
		DEBUG(4, nextch >= 040 ? "%c" : "\\%03o", nextch);
		if (nextch == '\r')
			break;
		if ((*rp = nextch) != '\0')
			rp++;
		if (rp >= rdvec + MR) {
			alarm(0);
			return(0);
		}
		*rp = '\0';
	}
	alarm(0);
	return(rdvec);
}


/***
 *	alarmtr()  -  catch alarm routine for "expect".
 */

alarmtr()
{
	longjmp(Sjbuf, 1);
}


/***
 *	sendthem(str, fn)	send line of login sequence
 *	char *str;
 *
 *	return codes:  none
 */

sendthem(str, fn)
char *str;
int fn;
{
	int nw, nulls, nlw = 1;
	char *strptr;

	DEBUG(5, "send %s\n", str);
	if (prefix("BREAK", str)) {
		sscanf(&str[5], "%1d", &nulls);
		if (nulls <= 0 || nulls > 10)
			nulls = 3;
		/* send break */
		genbrk(fn, nulls);
		return;
	}

	if (strcmp(str, "EOT") == SAME) {
		write(fn, EOTMSG, (unsigned) strlen(EOTMSG));
		return;
	}

	for (strptr = str; *strptr; strptr++) {
		if (*strptr == '\\') switch(*++strptr) {
		case 's':
			DEBUG(5, "BLANK\n", "");
			*strptr = ' ';
			break;
		case 'd':
			DEBUG(5, "DELAY\n", "");
			sleep(1);
			continue;
		case 'r':
			DEBUG(5, "RETURN\n", "");
			*strptr = '\r';
			break;
		case 'b':
			if (isdigit(*(strptr+1))) {
				nulls = (*++strptr - '0');
				if (nulls <= 0 || nulls > 10)
					nulls = 3;
			} else
				nulls = 3;
			/* send break */
			DEBUG(5, "BREAK (%d nulls)\n", nulls);
			genbrk(fn, nulls);
			continue;
		case 'c':
			if (*(strptr+1) == '\0') {
			DEBUG(5, "NO NL\n", "");
				nlw = 0;
				continue;
			}
			DEBUG(5, "NO NL - MIDDLE IGNORED\n", "");
		default:
			DEBUG(5, "BACKSLASH\n", "");
			strptr--;
		}
		nw = write(fn, strptr, 1);
		ASSERT(nw == 1, "BAD WRITE", str, 0);
	}
	if (nlw) write(fn, "\n", 1);
	return;
}

#define BSPEED B150

/***
 *	genbrk		send a break
 *
 *	return codes;  none
 */

genbrk(fn, bnulls)
{
	struct sgttyb ttbuf;
	int ret, sospeed;

#ifdef	VMUNIX
	ret = ioctl(fn, TIOCSBRK, (char *)0);
	sleep(1);
	ret = ioctl(fn, TIOCCBRK, (char *)0);
	DEBUG(4, "ioctl 1 second break\n", 0);
#else
	ret = ioctl(fn, TIOCGETP, &ttbuf);
	sospeed = ttbuf.sg_ospeed;
	ttbuf.sg_ospeed = BSPEED;
	ret = ioctl(fn, TIOCSETP, &ttbuf);
	ret = write(fn, "\0\0\0\0\0\0\0\0\0\0\0\0", bnulls);
	ASSERT(ret > 0, "BAD WRITE genbrk", "", ret);
	ttbuf.sg_ospeed = sospeed;
	ret = ioctl(fn, TIOCSETP, &ttbuf);
	ret = write(fn, "@", 1);
	ASSERT(ret > 0, "BAD WRITE genbrk", "", ret);
	DEBUG(4, "sent BREAK nulls - %d\n", bnulls);
#endif
	return;
}


/***
 *	notin(sh, lg)	check for occurrence of substring "sh"
 *	char *sh, *lg;
 *
 *	return codes:
 *		0  -  found the string
 *		1  -  not in the string
 */

notin(sh, lg)
char *sh, *lg;
{
	while (*lg != '\0') {
		if (prefix(sh, lg))
			return(0);
		else
			lg++;
	}
	return(1);
}


/*******
 *	ifdate(s)
 *	char *s;
 *
 *	ittvax!swatt
 *	Allow multiple date specifications separated by '|'.
 *	Calls ifadate, formerly "ifdate".
 *
 *	return codes:
 *		see ifadate
 */

ifdate(s)
char *s;
{
	register char *p;
	register int ret;

	for (p = s; p && (*p == '|' ? *++p : *p); p = index(p, '|'))
		if (ret = ifadate(p))
			return(ret);
	return(0);
}


/*******
 *	ifadate(s)
 *	char *s;
 *
 *	ifadate  -  this routine will check a string (s)
 *	like "MoTu0800-1730" to see if the present
 *	time is within the given limits.
 *	SIDE EFFECT - Retrytime is set
 *
 *	String alternatives:
 *		Wk - Mo thru Fr
 *		zero or one time means all day
 *		Any - any day
 *
 *	return codes:
 *		0  -  not within limits
 *		1  -  within limits
 */

ifadate(s)
char *s;
{
	static char *days[]={
		"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa", 0
	};
	long clock;
	int rtime;
	int i, tl, th, tn, flag, dayok=0;
	struct tm *localtime();
	struct tm *tp;
	char *index();
	char *p;

	/*  pick up retry time for failures  */
	/*  global variable Retrytime is set here  */
	if ((p = index(s, ',')) == NULL) {
		Retrytime = RETRYTIME;
	}
	else {
		i = sscanf(p+1, "%d", &rtime);
		if (i < 1 || rtime < 5)
			rtime = 5;
		Retrytime  = rtime * 60;
	}

	time(&clock);
	tp = localtime(&clock);
	while (isalpha(*s)) {
		for (i = 0; days[i]; i++) {
			if (prefix(days[i], s))
				if (tp->tm_wday == i)
					dayok = 1;
		}

		if (prefix("Wk", s))
			if (tp->tm_wday >= 1 && tp->tm_wday <= 5)
				dayok = 1;
		if (prefix("Any", s))
			dayok = 1;
		s++;
	}

	if (dayok == 0)
		return(0);
	i = sscanf(s, "%d-%d", &tl, &th);
	tn = tp->tm_hour * 100 + tp->tm_min;
	if (i < 2)
		return(1);
	if (th < tl)
		flag = 0;  /* set up for crossover 2400 test */
	else
		flag = 1;
	if ((tn >= tl && tn <= th)
	  || (tn >= th && tn <= tl)) /* test for crossover 2400 */
		return(flag);
	else
		return(!flag);
}


/***
 *	char *
 *	lastc(s)	return pointer to last character
 *	char *s;
 *
 */

char *
lastc(s)
char *s;
{
	while (*s != '\0') s++;
	return(s);
}



/***
 *	char *
 *	fdig(cp)	find first digit in string
 *
 *	return - pointer to first digit in string or end of string
 */

char *
fdig(cp)
char *cp;
{
	char *c;

	for (c = cp; *c; c++)
		if (*c >= '0' && *c <= '9')
			break;
	return(c);
}
