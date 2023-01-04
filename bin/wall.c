#ifndef lint
static	char sccsid[] = "@(#)wall.c 1.1 86/09/24 SMI"; /* from UCB 4.7 83/07/01 */
#endif
/*
 * wall - Broadcast a message to all users.
 *
 * This program is not related to David Wall, whose Stanford Ph.D. thesis
 * is entitled "Mechanisms for Broadcast and Selective Broadcast".
 */

#include <stdio.h>
#include <utmp.h>
#include <sys/time.h>
#include <signal.h>
#define	USERS	128
#define IGNOREUSER	"sleeper"

char	hostname[32];
char	mesg[3000];
int	msize,sline;
struct	utmp utmp[USERS];
char	*strcpy();
char	*strcat();
char who[9] = "???";
long	clock, time();
struct tm *localtime();
struct tm *localclock;
int aflag;			/* write to all ptys */
main(argc, argv)
char *argv[];
{
	register i;
	register char c;
	register struct utmp *p;
	FILE *f;
	FILE *mf;

	gethostname(hostname, sizeof (hostname));
	if((f = fopen("/etc/utmp", "r")) == NULL) {
		fprintf(stderr, "Cannot open /etc/utmp\n");
		exit(1);
	}
	clock = time( 0 );
	localclock = localtime( &clock );
	mf = stdin;
	if (argc >= 2 && strcmp(argv[1], "-a") == 0) {
		argc--;
		argv++;
		aflag++;
	}
	if(argc >= 2) {
		/* take message from unix file instead of standard input */
		if((mf = fopen(argv[1], "r")) == NULL) {
			fprintf(stderr,"Cannot open %s\n", argv[1]);
			exit(1);
		}
	}
	while((i = getc(mf)) != EOF) {
		if (msize >= sizeof mesg) {
			fprintf(stderr, "Message too long\n");
			exit(1);
		}
		mesg[msize++] = i;
	}
	fclose(mf);
	sline = ttyslot(2); /* 'utmp' slot no. of sender */
	fread((char *)utmp, sizeof(struct utmp), USERS, f);
	fclose(f);
	if (sline)
		strncpy(who, utmp[sline].ut_name, sizeof(utmp[sline].ut_name));
	for(i=0; i<USERS; i++) {
		p = &utmp[i];
		if ((p->ut_name[0] == 0) ||
		    (strncmp (p->ut_name, IGNOREUSER, sizeof(p->ut_name)) == 0))
			continue;
		 /*
		  * unless aflag is on, then if
		  * tty[pqr]? and not remote, then skip it
		  */
		if (!aflag && nonuser(*p))
			continue;
		sendmes(p->ut_line);
	}
	exit(0);
}

sendmes(tty)
char *tty;
{
	register i;
	char t[50], buf[BUFSIZ];
	register char *cp;
	register int c, ch;
	FILE *f;

	while ((i = fork()) == -1)
		if (wait((int *)0) == -1) {
			fprintf(stderr, "Try again\n");
			return;
		}
	if(i)
		return;
	strcpy(t, "/dev/");
	strcat(t, tty);

	signal(SIGALRM, SIG_DFL);	/* blow away if open hangs */
	alarm(10);

	if((f = fopen(t, "w")) == NULL) {
		fprintf(stderr,"cannot open %s\n", t);
		exit(1);
	}
	setbuf(f, buf);
	if (who[0] != '?')
		fprintf(f,
	    "\nBroadcast Message from %s!%s (%.*s) at %d:%02d ...\r\n\n"
		, hostname
		, who
		, sizeof(utmp[sline].ut_line)
		, utmp[sline].ut_line
		, localclock -> tm_hour
		, localclock -> tm_min
		);
	else
		fprintf(f,
	    "\nBroadcast Message at %d:%02d ...\r\n\n"
		, localclock -> tm_hour
		, localclock -> tm_min
		);
	/* fwrite(mesg, msize, 1, f); */
	for (cp = mesg, c = msize; c-- > 0; cp++) {
		ch = *cp;
		if (ch == '\n')
			putc('\r', f);
		putc(ch, f);
	}

	/*
	 * Bitchin'.
	 */

	exit(0);
}
