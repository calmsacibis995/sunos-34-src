/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static	char sccsid[] = "@(#)w.c 1.1 86/09/25 SMI"; /* from UCB 5.3 2/23/86 */
#endif not lint
/*
 * w - print system status (who and what)
 *
 * This program is similar to the systat command on Tenex/Tops 10/20
 * It needs read permission on /dev/mem, /dev/kmem, and /dev/drum.
 */
#include <sys/param.h>
#include <nlist.h>
#include <stdio.h>
#include <ctype.h>
#include <utmp.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <machine/pte.h>
#include <sys/vm.h>

#define NMAX sizeof(utmp.ut_name)
#define LMAX sizeof(utmp.ut_line)

#define ARGWIDTH	33	/* # chars left on 80 col crt for args */

struct pr {
	short	w_pid;			/* proc.p_pid */
	char	w_flag;			/* proc.p_flag */
	short	w_size;			/* proc.p_size */
	long	w_seekaddr;		/* where to find args */
	long	w_lastpg;		/* disk address of stack */
	int	w_igintr;		/* INTR+3*QUIT, 0=die, 1=ign, 2=catch */
	time_t	w_time;			/* CPU time used by this process */
	time_t	w_ctime;		/* CPU time used by children */
	dev_t	w_tty;			/* tty device of process */
	int	w_uid;			/* uid of process */
	char	w_comm[15];		/* user.u_comm, null terminated */
	char	w_args[ARGWIDTH+1];	/* args if interesting process */
} *pr;
int	nproc;

struct	nlist nl[] = {
	{ "_proc" },
#define	X_PROC		0
	{ "_swapdev" },
#define	X_SWAPDEV	1
	{ "_Usrptmap" },
#define	X_USRPTMA	2
	{ "_usrpt" },
#define	X_USRPT		3
	{ "_nswap" },
#define	X_NSWAP		4
	{ "_avenrun" },
#define	X_AVENRUN	5
	{ "_boottime" },
#define	X_BOOTTIME	6
	{ "_nproc" },
#define	X_NPROC		7
	{ "_dmmin" },
#define	X_DMMIN		8
	{ "_dmmax" },
#define	X_DMMAX		9
	{ "" },
};

FILE	*ps;
FILE	*ut;
FILE	*bootfd;
int	kmem;
int	mem;
int	swap;			/* /dev/kmem, mem, and swap */
int	nswap;
int	dmmin, dmmax;
dev_t	tty;
int	uid;
char	doing[520];		/* process attached to terminal */
time_t	proctime;		/* cpu time of process in doing */
long	avenrun[3];
struct	proc *aproc;

#define	DIV60(t)	((t+30)/60)    /* x/60 rounded */ 
#define	TTYEQ		(tty == pr[i].w_tty && (uid == 0 || uid == pr[i].w_uid))
			/* match if same tty and tty owner same as process or */
			/*   tty owned by root (for ptys used for windows) */
#define IGINT		(1+3*1)		/* ignoring both SIGINT & SIGQUIT */

char	*getargs();
char	*fread();
char	*ctime();
char	*rindex();
FILE	*popen();
struct	tm *localtime();
time_t	findidle();

int	debug;			/* true if -d flag: debugging output */
int	header = 1;		/* true if -h flag: don't print heading */
int	lflag = 1;		/* true if -l flag: long style output */
int	login;			/* true if invoked as login shell */
time_t	idle;			/* number of minutes user is idle */
int	nusers;			/* number of users logged in now */
char *	sel_user;		/* login of particular user selected */
char firstchar;			/* first char of name of prog invoked as */
time_t	jobtime;		/* total cpu time visible */
time_t	now;			/* the current time of day */
struct	timeval boottime;
time_t	uptime;			/* time of last reboot & elapsed time since */
int	np;			/* number of processes currently active */
struct	utmp utmp;
struct	proc mproc;
union {
	struct user U_up;
	char	pad[NBPG][UPAGES];
} Up;
#define	up	Up.U_up
#define	upsize	roundup(sizeof(struct user), DEV_BSIZE)

main(argc, argv)
	char **argv;
{
	int days, hrs, mins;
	register int i, j;
	char *cp;
	register int curpid, empty;

	login = (argv[0][0] == '-');
	cp = rindex(argv[0], '/');
	firstchar = login ? argv[0][1] : (cp==0) ? argv[0][0] : cp[1];
	cp = argv[0];	/* for Usage */

	while (argc > 1) {
		if (argv[1][0] == '-') {
			for (i=1; argv[1][i]; i++) {
				switch(argv[1][i]) {

				case 'd':
					debug++;
					break;

				case 'h':
					header = 0;
					break;

				case 'l':
					lflag++;
					break;

				case 's':
					lflag = 0;
					break;

				case 'u':
				case 'w':
					firstchar = argv[1][i];
					break;

				default:
					printf("Bad flag %s\n", argv[1]);
					exit(1);
				}
			}
		} else {
			if (!isalnum(argv[1][0]) || argc > 2) {
				printf("Usage: %s [ -hlsuw ] [ user ]\n", cp);
				exit(1);
			} else
				sel_user = argv[1];
		}
		argc--; argv++;
	}

	if ((kmem = open("/dev/kmem", 0)) < 0) {
		fprintf(stderr, "No kmem\n");
		exit(1);
	}
	nlist("/vmunix", nl);
	if (nl[0].n_type==0) {
		fprintf(stderr, "No namelist\n");
		exit(1);
	}

	if (firstchar != 'u')
		readpr();

	ut = fopen("/etc/utmp","r");
	time(&now);
	if (header) {
		/* Print time of day */
		prtat(&now);

		/*
		 * Print how long system has been up.
		 * (Found by looking for "boottime" in kernel)
		 */
		lseek(kmem, (long)nl[X_BOOTTIME].n_value, 0);
		read(kmem, &boottime, sizeof (boottime));

		uptime = now - boottime.tv_sec;
		uptime += 30;
		days = uptime / (60*60*24);
		uptime %= (60*60*24);
		hrs = uptime / (60*60);
		uptime %= (60*60);
		mins = uptime / 60;

		printf("  up");
		if (days > 0)
			printf(" %d day%s,", days, days>1?"s":"");
		if (hrs > 0 && mins > 0) {
			printf(" %2d:%02d,", hrs, mins);
		} else {
			if (hrs > 0)
				printf(" %d hr%s,", hrs, hrs>1?"s":"");
			if (mins > 0)
				printf(" %d min%s,", mins, mins>1?"s":"");
		}

		/* Print number of users logged in to system */
		while (fread(&utmp, sizeof(utmp), 1, ut)) {
			if (utmp.ut_name[0] != '\0'&& !nonuser(utmp))
				nusers++;
		}
		rewind(ut);
		printf("  %d user%s", nusers, nusers>1?"s":"");

		/*
		 * Print 1, 5, and 15 minute load averages.
		 * (Found by looking in kernel for avenrun).
		 */
		printf(",  load average:");
		lseek(kmem, (long)nl[X_AVENRUN].n_value, 0);
		read(kmem, avenrun, sizeof(avenrun));
		for (i = 0; i < (sizeof(avenrun)/sizeof(avenrun[0])); i++) {
			if (i > 0)
				printf(",");
			printf(" %.2f", (double)avenrun[i]/FSCALE);
		}
		printf("\n");
		if (firstchar == 'u')
			exit(0);

		/* Headers for rest of output */
		if (lflag)
			printf("User     tty       login@  idle   JCPU   PCPU  what\n");
		else
			printf("User    tty  idle  what\n");
		fflush(stdout);
	}


	for (;;) {	/* for each entry in utmp */
		if (fread(&utmp, sizeof(utmp), 1, ut) == NULL) {
			fclose(ut);
			exit(0);
		}
		if (utmp.ut_name[0] == '\0')
			continue;	/* that tty is free */
		if (sel_user && strncmp(utmp.ut_name, sel_user, NMAX) != 0)
			continue;	/* we wanted only somebody else */

		gettty();
		jobtime = 0;
		proctime = 0;
		strcpy(doing, "-");	/* default act: normally never prints */
		empty = 1;
		curpid = -1;
		idle = findidle();
		for (i=0; i<np; i++) {	/* for each process on this tty */
			if (!(TTYEQ))
				continue;
			jobtime += pr[i].w_time + pr[i].w_ctime;
			proctime += pr[i].w_time;
			/* 
			 * Meaning of debug fields following proc name is:
			 * & by itself: ignoring both SIGINT and QUIT.
			 *		(==> this proc is not a candidate.)
			 * & <i> <q>:   i is SIGINT status, q is quit.
			 *		0 == DFL, 1 == IGN, 2 == caught.
			 * *:		proc pgrp == tty pgrp.
			 */
			if (debug) {
				printf("\t\t%d\t%s", pr[i].w_pid, pr[i].w_args);
				if ((j=pr[i].w_igintr) > 0)
					if (j==IGINT)
						printf(" &");
					else
						printf(" & %d %d", j%3, j/3);
				printf("\n");
			}
			if (empty && pr[i].w_igintr!=IGINT) {
				empty = 0;
				curpid = -1;
			}
			if(pr[i].w_pid>curpid && (pr[i].w_igintr!=IGINT || empty)){
				curpid = pr[i].w_pid;
				strcpy(doing, lflag ? pr[i].w_args : pr[i].w_comm);
#ifdef notdef
				if (doing[0]==0 || doing[0]=='-' && doing[1]<=' ' || doing[0] == '?') {
					strcat(doing, " (");
					strcat(doing, pr[i].w_comm);
					strcat(doing, ")");
				}
#endif
			}
		}
		putline();
	}
}

/* figure out the major/minor device # pair for this tty */
gettty()
{
	char ttybuf[20];
	struct stat statbuf;

	ttybuf[0] = 0;
	strcpy(ttybuf, "/dev/");
	strcat(ttybuf, utmp.ut_line);
	stat(ttybuf, &statbuf);
	tty = statbuf.st_rdev;
	uid = statbuf.st_uid;
}

/*
 * putline: print out the accumulated line of info about one user.
 */
putline()
{
	register int tm;

	/* print login name of the user */
	printf("%-*.*s ", NMAX, NMAX, utmp.ut_name);

	/* print tty user is on */
	if (lflag)
		/* long form: all (up to) LMAX chars */
		printf("%-*.*s", LMAX, LMAX, utmp.ut_line);
	else {
		/* short form: 2 chars, skipping 'tty' if there */
		if (utmp.ut_line[0]=='t' && utmp.ut_line[1]=='t' && utmp.ut_line[2]=='y')
			printf("%-2.2s", &utmp.ut_line[3]);
		else
			printf("%-2.2s", utmp.ut_line);
	}

	if (lflag)
		/* print when the user logged in */
		prtat(&utmp.ut_time);

	/* print idle time */
	if (idle >= 36 * 60)
		printf("%2ddays ", (idle + 12 * 60) / (24 * 60));
	else
		prttime(idle," ");

	if (lflag) {
		/* print CPU time for all processes & children */
		prttime(jobtime," ");
		/* print cpu time for interesting process */
		prttime(proctime," ");
	}

	/* what user is doing, either command tail or args */
	printf(" %-.32s\n",doing);
	fflush(stdout);
}

/* find & return number of minutes current tty has been idle */
time_t
findidle()
{
	struct stat stbuf;
	time_t lastaction, diff;
	char ttyname[20];

	strcpy(ttyname, "/dev/");
	strncat(ttyname, utmp.ut_line, LMAX);
	stat(ttyname, &stbuf);
	time(&now);
	lastaction = stbuf.st_atime;
	diff = now - lastaction;
	diff = DIV60(diff);
	if (diff < 0) diff = 0;
	return(diff);
}

#define	HR	(60 * 60)
#define	DAY	(24 * HR)
#define	MON	(30 * DAY)

/*
 * prttime prints a time in hours and minutes or minutes and seconds.
 * The character string tail is printed at the end, obvious
 * strings to pass are "", " ", or "am".
 */
prttime(tim, tail)
	time_t tim;
	char *tail;
{

	if (tim >= 60) {
		printf("%3d:", tim/60);
		tim %= 60;
		printf("%02d", tim);
	} else if (tim > 0)
		printf("    %2d", tim);
	else
		printf("      ");
	printf("%s", tail);
}

char *weekday[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
char *month[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

/* prtat prints a 12 hour time given a pointer to a time of day */
prtat(time)
	long *time;
{
	struct tm *p;
	register int hr, pm;

	p = localtime(time);
	hr = p->tm_hour;
	pm = (hr > 11);
	if (hr > 11)
		hr -= 12;
	if (hr == 0)
		hr = 12;
	if (now - *time <= 18 * HR)
		prttime(hr * 60 + p->tm_min, pm ? "pm" : "am");
	else if (now - *time <= 7 * DAY)
		printf(" %s%2d%s", weekday[p->tm_wday], hr, pm ? "pm" : "am");
	else
		printf(" %2d%s%2d", p->tm_mday, month[p->tm_mon], p->tm_year);
}

/*
 * readpr finds and reads in the array pr, containing the interesting
 * parts of the proc and user tables for each live process.
 */
readpr()
{
	int pn, mf, addr, c;
	int szpt, pfnum, i;
	struct pte *Usrptma, *usrpt, *pte, apte;
	struct dblock db;

	Usrptma = (struct pte *) nl[X_USRPTMA].n_value;
	usrpt = (struct pte *) nl[X_USRPT].n_value;
	if((mem = open("/dev/mem", 0)) < 0) {
		fprintf(stderr, "No mem\n");
		exit(1);
	}
	if ((swap = open("/dev/drum", 0)) < 0) {
		fprintf(stderr, "No drum\n");
		exit(1);
	}
	/*
	 * read mem to find swap dev.
	 */
	lseek(kmem, (long)nl[X_SWAPDEV].n_value, 0);
	read(kmem, &nl[X_SWAPDEV].n_value, sizeof(nl[X_SWAPDEV].n_value));
	/*
	 * Find base of and parameters of swap
	 */
	lseek(kmem, (long)nl[X_NSWAP].n_value, 0);
	read(kmem, &nswap, sizeof(nswap));
	lseek(kmem, (long)nl[X_DMMIN].n_value, 0);
	read(kmem, &dmmin, sizeof(dmmin));
	lseek(kmem, (long)nl[X_DMMAX].n_value, 0);
	read(kmem, &dmmax, sizeof(dmmax));
	/*
	 * Locate proc table
	 */
	lseek(kmem, (long)nl[X_NPROC].n_value, 0);
	read(kmem, &nproc, sizeof(nproc));
	pr = (struct pr *)calloc(nproc, sizeof (struct pr));
	np = 0;
	lseek(kmem, (long)nl[X_PROC].n_value, 0);
	read(kmem, &aproc, sizeof(aproc));
	for (pn=0; pn<nproc; pn++) {
		lseek(kmem, (int)(aproc + pn), 0);
		read(kmem, &mproc, sizeof mproc);
		/* decide if it's an interesting process */
		if (mproc.p_stat==0 || mproc.p_stat==SZOMB || mproc.p_pgrp==0)
			continue;
		/* find & read in the user structure */
		if ((mproc.p_flag & SLOAD) == 0) {
			/* not in memory - get from swap device */
			addr = dtob(mproc.p_swaddr);
			lseek(swap, (long)addr, 0);
			if (read(swap, &up, upsize) != upsize) {
				continue;
			}
		} else {
			int cc;
#define INTPPG (NBPG / sizeof (int))
			struct pte pagetbl[UPAGES];
			/* loaded, get each page from memory separately */
			szpt = mproc.p_szpt;
			lseek(kmem, mproc.p_addr, 0);
			if (read(kmem,pagetbl,sizeof(pagetbl)) != sizeof(pagetbl))   
cont:
				continue;
			for(cc=0; cc<UPAGES; cc++) {	/* get u area */
				int upage = pagetbl[cc].pg_pfnum;
				lseek(mem,ctob(upage),0);
				if (read(mem,((int *)&up)+INTPPG*cc,NBPG) != NBPG)
					goto cont;
			}
			szpt = up.u_pcb.pcb_szpt;
			pr[np].w_seekaddr = (int)sptopte(&mproc, CLSIZE-1);
		}
		vstodb(0, CLSIZE, &up.u_smap, &db, 1);
		pr[np].w_lastpg = dtob(db.db_base);
		if (up.u_ttyp == NULL)
			continue;

		/* save the interesting parts */
		pr[np].w_pid = mproc.p_pid;
		pr[np].w_flag = mproc.p_flag;
		pr[np].w_size = mproc.p_dsize + mproc.p_ssize;
		pr[np].w_igintr = (((int)up.u_signal[2]==1) +
		    2*((int)up.u_signal[2]>1) + 3*((int)up.u_signal[3]==1)) +
		    6*((int)up.u_signal[3]>1);
		pr[np].w_time =
		    up.u_ru.ru_utime.tv_sec + up.u_ru.ru_stime.tv_sec;
		pr[np].w_ctime =
		    up.u_cru.ru_utime.tv_sec + up.u_cru.ru_stime.tv_sec;
		pr[np].w_tty = up.u_ttyd;
		pr[np].w_uid = mproc.p_uid;
		up.u_comm[14] = 0;	/* Bug: This bombs next field. */
		strcpy(pr[np].w_comm, up.u_comm);
		/*
		 * Get args if there's a chance we'll print it.
		 * Cant just save pointer: getargs returns static place.
		 * Cant use strcpyn: that crock blank pads.
		 */
		pr[np].w_args[0] = 0;
		strncat(pr[np].w_args,getargs(&pr[np]),ARGWIDTH);
		if (pr[np].w_args[0]==0 || pr[np].w_args[0]=='-' && pr[np].w_args[1]<=' ' || pr[np].w_args[0] == '?') {
			strcat(pr[np].w_args, " (");
			strcat(pr[np].w_args, pr[np].w_comm);
			strcat(pr[np].w_args, ")");
		}
		np++;
	}
}

/*
 * getargs: given a pointer to a proc structure, this looks at the swap area
 * and tries to reconstruct the arguments. This is straight out of ps.
 */
char *
getargs(p)
	struct pr *p;
{
	int c, addr, nbad;
	static int abuf[CLSIZE*NBPG/sizeof(int)];
	struct pte apte;
	register int *ip;
	register char *cp, *cp1;

	if ((p->w_flag & SLOAD) == 0) {
		lseek(swap, p->w_lastpg, 0);
		if (read(swap, abuf, sizeof(abuf)) != sizeof(abuf))
			return(p->w_comm);
	} else {
		lseek(kmem,p->w_seekaddr,0);
		if (read(kmem,&apte,sizeof (apte)) != sizeof (apte))
			return(p->w_comm);
		if (apte.pg_fod==0 && apte.pg_pfnum) {
			lseek(mem,ctob(apte.pg_pfnum),0);
			if (read(mem,abuf,sizeof(abuf)) != sizeof(abuf))
				return(p->w_comm);
		} else {
			lseek(swap, p->w_lastpg, 0);
			if (read(swap, abuf, sizeof(abuf)) != sizeof(abuf))
				return(p->w_comm);
		}
	}
	abuf[sizeof(abuf)/sizeof(abuf[0])-1] = 0;
	for (ip = &abuf[sizeof(abuf)/sizeof(abuf[0])-2]; ip > abuf;) {
		/* Look from top for -1 or 0 as terminator flag. */
		if (*--ip == -1 || *ip == 0) {
			cp = (char *)(ip+1);
			if (*cp==0)
				cp++;
			nbad = 0;	/* up to 5 funny chars as ?'s */
			for (cp1 = cp; cp1 < (char *)&abuf[sizeof(abuf)/sizeof(abuf[0])]; cp1++) {
				c = *cp1&0177;
				if (c==0)  /* nulls between args => spaces */
					*cp1 = ' ';
				else if (c < ' ' || c > 0176) {
					if (++nbad >= 5) {
						*cp1++ = ' ';
						break;
					}
					*cp1 = '?';
				} else if (c=='=') {	/* Oops - found an
							 * environment var, back
							 * over & erase it. */
					*cp1 = 0;
					while (cp1>cp && *--cp1!=' ')
						*cp1 = 0;
					break;
				}
			}
			while (*--cp1==' ')	/* strip trailing spaces */
				*cp1 = 0;
			return(cp);
		}
	}
	return (p->w_comm);
}

/*
 * Given a base/size pair in virtual swap area,
 * return a physical base/size pair which is the
 * (largest) initial, physically contiguous block.
 */
vstodb(vsbase, vssize, dmp, dbp, rev)
	register int vsbase;
	int vssize;
	struct dmap *dmp;
	register struct dblock *dbp;
{
	register int blk = dmmin;
	register swblk_t *ip = dmp->dm_map;

	vsbase = ctod(vsbase);
	vssize = ctod(vssize);
	if (vsbase < 0 || vsbase + vssize > dmp->dm_size)
		panic("vstodb");
	while (vsbase >= blk) {
		vsbase -= blk;
		if (blk < dmmax)
			blk *= 2;
		ip++;
	}
	if (*ip <= 0 || *ip + blk > nswap)
		panic("vstodb *ip");
	dbp->db_size = min(vssize, blk - vsbase);
	dbp->db_base = *ip + (rev ? blk - (vsbase + dbp->db_size) : vsbase);
}

panic(cp)
	char *cp;
{

	/* printf("%s\n", cp); */
}

min(a, b)
{

	return (a < b ? a : b);
}
