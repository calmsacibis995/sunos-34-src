#ifndef lint
static	char sccsid[] = "@(#)ps.c 1.1 86/09/24 SMI"; /* from UCB 4.24 83/05/22 */
#endif
/*
 * ps
 */
#include <stdio.h>
#include <ctype.h>
#include <nlist.h>
#include <pwd.h>
#include <sys/param.h>
#include <sys/tty.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <machine/pte.h>
#include <sys/vm.h>
#include <sys/text.h>
#include <sys/stat.h>
#include <sys/mbuf.h>
#include <math.h>

struct nlist nl[] = {
	{ "_proc" },
#define	X_PROC		0
	{ "_Usrptmap" },
#define	X_USRPTMA	1
	{ "_usrpt" },
#define X_USRPT         2
	{ "_text" },
#define	X_TEXT		3
	{ "_nswap" },
#define	X_NSWAP		4
	{ "_maxslp" },
#define	X_MAXSLP	5
	{ "_ccpu" },
#define	X_CCPU		6
	{ "_ecmx" },
#define	X_ECMX		7
	{ "_nproc" },
#define	X_NPROC		8
	{ "_ntext" },
#define	X_NTEXT		9
	{ "_dmmin" },
#define	X_DMMIN		10
	{ "_dmmax" },
#define	X_DMMAX		11
	{ "_Sysmap" },
#define	X_SYSMAP	12
	{ "_Sysbase" },
#define	X_SYSBASE	13
	{ "_Syslimit" },
#define	X_SYSLIMIT	14
	{ "" },
};

struct	savcom {
	union {
		struct	lsav *lp;
		float	u_pctcpu;
		struct	vsav *vp;
		int	s_ssiz;
	} s_un;
	struct	asav *ap;
} *savcom;

struct	asav {
	char	*a_cmdp;
	int	a_flag;
	short	a_stat, a_uid, a_pid, a_nice, a_pri, a_slptime, a_time;
	size_t	a_size, a_rss, a_tsiz, a_txtrss;
	short	a_xccount;
	char	a_tty[MAXNAMLEN+1];
	dev_t	a_ttyd;
	time_t	a_cpu;
	size_t	a_maxrss;
};

char	*lhdr;
struct	lsav {
	short	l_ppid;
	char	l_cpu;
	int	l_addr;
	caddr_t	l_wchan;
};

char	*uhdr;
char	*shdr;

char	*vhdr;
struct	vsav {
	u_int	v_majflt;
	size_t	v_swrss, v_txtswrss;
	float	v_pctcpu;
};

struct	proc proc[8];		/* 8 = a few, for less syscalls */
struct	proc *mproc;
struct	text *text;

union {
	struct	user user;
	char	upages[UPAGES][NBPG];
} user;
#define u	user.user

#define clear(x) 	((int)x - KERNELBASE)

int	chkpid;
int	aflg, cflg, eflg, gflg, kflg, lflg, sflg,
	uflg, vflg, xflg;
char	*tptr;
char	*gettty(), *getcmd(), *getname(), *savestr(), *alloc(), *state();
char	*rindex(), *calloc(), *sbrk(), *strcpy(), *strcat(), *strncat();
char	*index(), *ttyname(), mytty[16];
long	lseek();
double	pcpu(), pmem();
int	pscomp();
int	nswap, maxslp;
struct	text *atext;
double	ccpu;
long	kccpu;
int	ecmx;
struct	pte *Usrptma, *usrpt;
struct	pte *Sysmap;
int	Sysbase, Syslimit;
int	nproc, ntext;
int	dmmin, dmmax;

struct	ttys {
	char	name[MAXNAMLEN+1];
	dev_t	ttyd;
	struct	ttys *next;
	struct	ttys *cand;
} *allttys, *cand[16];

int	npr;

int	cmdstart;
int	twidth;
char	*kmemf, *memf, *swapf, *nlistf;
int	kmem, mem, swap = -1;
int	rawcpu, sumcpu;

int	pcbpf;
int	argaddr;
int	swaddr;

#define	pgtok(a)	((a)*CLBYTES/1024)

main(argc, argv)
	char **argv;
{
	register int i, j;
	register char *ap;
	int uid;
	off_t procp;

	twidth = 80;
	argc--, argv++;
	if (argc > 0) {
		ap = argv[0];
		while (*ap) switch (*ap++) {

		case 'C':
			rawcpu++;
			break;
		case 'S':
			sumcpu++;
			break;
		case 'a':
			aflg++;
			break;
		case 'c':
			cflg = !cflg;
			break;
		case 'e':
			eflg++;
			break;
		case 'g':
			gflg++;
			break;
		case 'k':
			kflg++;
			break;
		case 'l':
			lflg++;
			break;
		case 's':
			sflg++;
			break;
		case 't':
			if (*ap)
				tptr = ap;
			else if ((tptr = ttyname(2)) != 0) {
				strcpy(mytty, tptr);
				if ((tptr = index(mytty,'y')) != 0)
					tptr++;
			}
			aflg++;
			gflg++;
			if (tptr && *tptr == '?')
				xflg++;
			while (*ap)
				ap++;
			break;
		case 'u': 
			uflg++;
			break;
		case 'v':
			cflg = 1;
			vflg++;
			break;
		case 'w':
			if (twidth == 80)
				twidth = 132;
			else
				twidth = BUFSIZ;
			break;
		case 'x':
			xflg++;
			break;
		default:
			if (!isdigit(ap[-1]))
				break;
			chkpid = atoi(--ap);
			*ap = 0;
			aflg++;
			xflg++;
			break;
		}
	}
	openfiles(argc, argv);
	getkvars(argc, argv);
	if (chdir("/dev") < 0) {
		perror("/dev");
		exit(1);
	}
	getdev();
	uid = getuid();
	printhdr();
	procp = getw(nl[X_PROC].n_value);
	nproc = getw(nl[X_NPROC].n_value);
	savcom = (struct savcom *)calloc(nproc, sizeof (*savcom));
	for (i=0; i<nproc; i += 8) {
		vlseek(kmem, (long)procp, 0);
		j = nproc - i;
		if (j > 8)
			j = 8;
		j *= sizeof (struct proc);
		if (read(kmem, (char *)proc, j) != j) {
			cantread("proc table", kmemf);
			exit(1);
		}
		procp += j;
		for (j = j / sizeof (struct proc) - 1; j >= 0; j--) {
			mproc = &proc[j];
			if (mproc->p_stat == 0 ||
			    mproc->p_pgrp == 0 && xflg == 0)
				continue;
			if (tptr == 0 && gflg == 0 && xflg == 0 &&
			    mproc->p_ppid == 1)
				continue;
			if (uid != mproc->p_uid && aflg==0 ||
			    chkpid != 0 && chkpid != mproc->p_pid)
				continue;
			if (vflg && gflg == 0 && xflg == 0) {
				if (mproc->p_stat == SZOMB ||
				    mproc->p_flag&SWEXIT)
					continue;
				if (mproc->p_slptime > MAXSLP &&
				    (mproc->p_stat == SSLEEP ||
				     mproc->p_stat == SSTOP))
				continue;
			}
			save();
		}
	}
	qsort(savcom, npr, sizeof(savcom[0]), pscomp);
	for (i=0; i<npr; i++) {
		register struct savcom *sp = &savcom[i];
		if (lflg)
			lpr(sp);
		else if (vflg)
			vpr(sp);
		else if (uflg)
			upr(sp);
		else
			spr(sp);
		if (sp->ap->a_flag & SWEXIT)
			printf(" <exiting>");
		else if (sp->ap->a_stat == SZOMB)
			printf(" <defunct>");
		else if (sp->ap->a_pid == 0)
			printf(" swapper");
		else if (sp->ap->a_pid == 2)
			printf(" pagedaemon");
		else if (sp->ap->a_pid == 3 && sp->ap->a_flag & SSYS)
			printf(" ip input");
		else
			printf(" %.*s", twidth - cmdstart - 2, sp->ap->a_cmdp);
		printf("\n");
	}
	exit(npr == 0);
}

/*
 * Returns the physical address of location "addr" in kmem.
 * It is assumed that addr, Sysbase, Syslimit, and Sysmap
 * have already been "clear()ed" in the kflg case.
 */
physaddr(addr)
	int addr;
{
	int v;
	struct pte pte;

	if (kflg) {
		if (addr < Sysbase || addr > Syslimit) {
			return (addr);
		}
		v = btop(addr);
		lseek(kmem, (long)(Sysmap+v), 0);
		if (read(kmem, &pte, sizeof(pte)) != sizeof(pte)) {
			printf("error reading %s to get pte for %x\n",
			    kmemf, addr);
			return (addr);
		}
		if (pte.pg_v == 0 && (pte.pg_fod || pte.pg_pfnum == 0)) {
			printf("page not valid for address %x\n", addr);
			return (addr);
		}
		addr &= PGOFSET;
		addr += (int)ptob(pte.pg_pfnum);
	}
	return (addr);
}


getw(loc)
	unsigned long loc;
{
	long word;

	vlseek(kmem, (long)loc, 0);
	if (read(kmem, (char *)&word, sizeof (word)) != sizeof (word))
		printf("error reading %s at %x\n", kmemf, loc);
	if (kflg && word >= KERNELBASE)
		word -= KERNELBASE;
	return (word);
}

/*
 * Seek to a kernel virtual address
 */
vlseek(fd, loc, off)
	int fd;
	long loc;
	int off;
{
	loc = physaddr(loc);
	if (lseek(fd, (long)loc, off) == -1) {
		perror("seek");
		exit(1);
	}
}

klseek(fd, loc, off)
	int fd;
	long loc;
	int off;
{

	if (kflg && loc >= KERNELBASE)
		loc -= KERNELBASE;
	if (lseek(fd, (long)loc, off) == -1) {
		perror("seek");
		exit(1);
	}
}


openfiles(argc, argv)
	char **argv;
{

	kmemf = "/dev/kmem";
	if (kflg)
		kmemf = argc > 2 ? argv[2] : "/vmcore";
	kmem = open(kmemf, 0);
	if (kmem < 0) {
		perror(kmemf);
		exit(1);
	}
	if (kflg)  {
		mem = kmem;
		memf = kmemf;
	} else {
		memf = "/dev/mem";
		mem = open(memf, 0);
		if (mem < 0) {
			perror(memf);
			exit(1);
		}
	}
	if (kflg == 0 || argc > 3) {
		swapf = argc>3 ? argv[3]: "/dev/drum";
		swap = open(swapf, 0);
		if (swap < 0) {
			perror(swapf);
			exit(1);
		}
	}
}

getkvars(argc, argv)
	char **argv;
{
	register struct nlist *nlp;

	nlistf = argc > 1 ? argv[1] : "/vmunix";
	nlist(nlistf, nl);
	if (nl[0].n_type == 0) {
		fprintf(stderr, "%s: No namelist\n", nlistf);
		exit(1);
	}
	usrpt = (struct pte *)nl[X_USRPT].n_value;      /* don't clear!! */
	Usrptma = (struct pte *)nl[X_USRPTMA].n_value;
	if (kflg)
		for (nlp = nl; nlp < &nl[sizeof (nl)/sizeof (nl[0])]; nlp++)
			if (nlp->n_value >= KERNELBASE)
				nlp->n_value = clear(nlp->n_value);
	Sysmap = (struct pte *)nl[X_SYSMAP].n_value;
	Sysbase = nl[X_SYSBASE].n_value;
	Syslimit = nl[X_SYSLIMIT].n_value;
	vlseek(kmem, (long)nl[X_NSWAP].n_value, 0);
	if (read(kmem, (char *)&nswap, sizeof (nswap)) != sizeof (nswap)) {
		cantread("nswap", kmemf);
		exit(1);
	}
	vlseek(kmem, (long)nl[X_MAXSLP].n_value, 0);
	if (read(kmem, (char *)&maxslp, sizeof (maxslp)) != sizeof (maxslp)) {
		cantread("maxslp", kmemf);
		exit(1);
	}
	vlseek(kmem, (long)nl[X_CCPU].n_value, 0);
	if (read(kmem, (char *)&kccpu, sizeof (kccpu)) != sizeof (kccpu)) {
		cantread("ccpu", kmemf);
		exit(1);
	}
	ccpu = (double)kccpu / FSCALE;
	vlseek(kmem, (long)nl[X_ECMX].n_value, 0);
	if (read(kmem, (char *)&ecmx, sizeof (ecmx)) != sizeof (ecmx)) {
		cantread("ecmx", kmemf);
		exit(1);
	}
	if (uflg || vflg) {
		ntext = getw(nl[X_NTEXT].n_value);
		text = (struct text *)alloc(ntext * sizeof (struct text));
		if (text == 0) {
			fprintf(stderr, "no room for text table\n");
			exit(1);
		}
		atext = (struct text *)getw(nl[X_TEXT].n_value);
		vlseek(kmem, (long)atext, 0);
		if (read(kmem, (char *)text, ntext * sizeof (struct text))
		    != ntext * sizeof (struct text)) {
			cantread("text table", kmemf);
			exit(1);
		}
	}
	dmmin = getw(nl[X_DMMIN].n_value);
	dmmax = getw(nl[X_DMMAX].n_value);
}

printhdr()
{
	char *hdr;

	if (sflg+lflg+vflg+uflg > 1) {
		fprintf(stderr, "ps: specify only one of s,l,v and u\n");
		exit(1);
	}
	hdr = lflg ? lhdr : 
			(vflg ? vhdr : 
				(uflg ? uhdr : shdr));
	if (lflg+vflg+uflg+sflg == 0)
		hdr += strlen("SSIZ ");
	cmdstart = strlen(hdr);
	printf("%s COMMAND\n", hdr);
	(void) fflush(stdout);
}

cantread(what, fromwhat)
	char *what, *fromwhat;
{

	fprintf(stderr, "ps: error reading %s from %s\n", what, fromwhat);
}

struct	direct *dbuf;
int	dialbase;

getdev()
{
	register DIR *df;

	dialbase = -1;
	if ((df = opendir(".")) == NULL) {
		fprintf(stderr, "Can't open . in /dev\n");
		exit(1);
	}
	while ((dbuf = readdir(df)) != NULL) 
		maybetty();
	closedir(df);
}

/*
 * Attempt to avoid stats by guessing minor device
 * numbers from tty names.  Console is known,
 * know that r(hp|up|mt) are unlikely as are different mem's,
 * floppy, null, tty, etc.
 */
maybetty()
{
	register char *cp = dbuf->d_name;
	register struct ttys *dp;
	int x;
	struct stat stb;

	switch (cp[0]) {

	case 'c':
		if (!strcmp(cp, "console")) {
			x = 0;
			goto donecand;
		}
		/* cu[la]? are possible!?! don't rule them out */
		break;

	case 'd':
		if (!strcmp(cp, "drum"))
			return;
		break;

	case 'f':
		if (!strcmp(cp, "floppy"))
			return;
		break;

	case 'k':
		cp++;
		if (*cp == 'U')
			cp++;
		goto trymem;

	case 'r':
		cp++;
		if (*cp == 'r' || *cp == 'u' || *cp == 'h')
			cp++;
#define is(a,b) cp[0] == 'a' && cp[1] == 'b'
		if (is(r,p) || is(u,p) || is(r,k) || is(r,m) || is(m,t)) {
			cp += 2;
			if (isdigit(*cp) && cp[2] == 0)
				return;
		}
		break;

	case 'm':
trymem:
		if (cp[0] == 'm' && cp[1] == 'e' && cp[2] == 'm' && cp[3] == 0)
			return;
		if (cp[0] == 'm' && cp[1] == 't')
			return;
		break;

	case 'n':
		if (!strcmp(cp, "null"))
			return;
		break;

	case 'v':
		if ((cp[1] == 'a' || cp[1] == 'p') && isdigit(cp[2]) &&
		    cp[3] == 0)
			return;
		break;
	}
	cp = dbuf->d_name + dbuf->d_namlen - 1;
	x = 0;
	if (cp[-1] == 'd') {
		if (dialbase == -1) {
			if (stat("ttyd0", &stb) == 0)
				dialbase = stb.st_rdev & 017;
			else
				dialbase = -2;
		}
		if (dialbase == -2)
			x = 0;
		else
			x = 11;
	}
	if (cp > dbuf->d_name && isdigit(cp[-1]) && isdigit(*cp))
		x += 10 * (cp[-1] - ' ') + cp[0] - '0';
	else if (*cp >= 'a' && *cp <= 'f')
		x += 10 + *cp - 'a';
	else if (isdigit(*cp))
		x += *cp - '0';
	else
		x = -1;
donecand:
	dp = (struct ttys *)alloc(sizeof (struct ttys));
	(void) strcpy(dp->name, dbuf->d_name);
	dp->next = allttys;
	dp->ttyd = -1;
	allttys = dp;
	if (x == -1)
		return;
	x &= 017;
	dp->cand = cand[x];
	cand[x] = dp;
}

char *
gettty()
{
	register char *p;
	register struct ttys *dp;
	struct stat stb;
	int x;

	if (u.u_ttyp == 0)
		return("?");
	x = u.u_ttyd & 017;
	for (dp = cand[x]; dp; dp = dp->cand) {
		if (dp->ttyd == -1) {
			if (stat(dp->name, &stb) == 0 &&
			   (stb.st_mode&S_IFMT)==S_IFCHR)
				dp->ttyd = stb.st_rdev;
			else
				dp->ttyd = -2;
		}
		if (dp->ttyd == u.u_ttyd)
			goto found;
	}
	/* ick */
	for (dp = allttys; dp; dp = dp->next) {
		if (dp->ttyd == -1) {
			if (stat(dp->name, &stb) == 0 &&
			   (stb.st_mode&S_IFMT)==S_IFCHR)
				dp->ttyd = stb.st_rdev;
			else
				dp->ttyd = -2;
		}
		if (dp->ttyd == u.u_ttyd)
			goto found;
	}
	return ("?");
found:
	p = dp->name;
	if (p[0]=='t' && p[1]=='t' && p[2]=='y')
		p += 3;
	return (p);
}

save()
{
	register struct savcom *sp;
	register struct asav *ap;
	register char *cp;
	register struct text *xp;
	char *ttyp, *cmdp;

	if (mproc->p_stat != SZOMB && getu() == 0)
		return;
	ttyp = gettty();
	if (xflg == 0 && ttyp[0] == '?' || tptr && strncmp(tptr, ttyp, 2))
		return;
	sp = &savcom[npr];
	cmdp = getcmd();
	if (cmdp == 0)
		return;
	sp->ap = ap = (struct asav *)alloc(sizeof (struct asav));
	sp->ap->a_cmdp = cmdp;
#define e(a,b) ap->a = mproc->b
	e(a_flag, p_flag); e(a_stat, p_stat); e(a_nice, p_nice);
	e(a_uid, p_uid); e(a_pid, p_pid); e(a_pri, p_pri);
	e(a_slptime, p_slptime); e(a_time, p_time);
	ap->a_tty[0] = ttyp[0];
	ap->a_tty[1] = ttyp[1] ? ttyp[1] : ' ';
	if (ap->a_stat == SZOMB) {
		ap->a_cpu = 0;
	} else {
		ap->a_size = mproc->p_dsize + mproc->p_ssize;
		e(a_rss, p_rssize); 
		ap->a_ttyd = u.u_ttyd;
		ap->a_cpu = u.u_ru.ru_utime.tv_sec + u.u_ru.ru_stime.tv_sec;
		if (sumcpu)
			ap->a_cpu += u.u_cru.ru_utime.tv_sec + u.u_cru.ru_stime.tv_sec;
		if (mproc->p_textp && text) {
			xp = &text[mproc->p_textp - atext];
			ap->a_tsiz = xp->x_size;
			ap->a_txtrss = xp->x_rssize;
			ap->a_xccount = xp->x_ccount;
		}
	}
#undef e
	ap->a_maxrss = mproc->p_maxrss;
	if (lflg) {
		register struct lsav *lp;

		sp->s_un.lp = lp = (struct lsav *)alloc(sizeof (struct lsav));
#define e(a,b) lp->a = mproc->b
		e(l_ppid, p_ppid); e(l_cpu, p_cpu);
		if (ap->a_stat != SZOMB)
			e(l_wchan, p_wchan);
#undef e
		if (ap->a_flag & SLOAD)
			lp->l_addr = pcbpf;
		else
			lp->l_addr = swaddr;
	} else if (vflg) {
		register struct vsav *vp;

		sp->s_un.vp = vp = (struct vsav *)alloc(sizeof (struct vsav));
#define e(a,b) vp->a = mproc->b
		if (ap->a_stat != SZOMB) {
			e(v_swrss, p_swrss);
			vp->v_majflt = u.u_ru.ru_majflt;
			if (mproc->p_textp)
				vp->v_txtswrss = xp->x_swrss;
		}
		vp->v_pctcpu = pcpu();
#undef e
	} else if (uflg)
		sp->s_un.u_pctcpu = pcpu();
	else if (sflg) {
		if (ap->a_stat != SZOMB) {
			for (cp = (char *)u.u_stack;
			    cp < &user.upages[UPAGES][0]; )
				if (*cp++)
					break;
			sp->s_un.s_ssiz = (&user.upages[UPAGES][0] - cp);
		}
	}

	npr++;
}

double
pmem(ap)
	register struct asav *ap;
{
	double fracmem;
	int szptudot;

	if ((ap->a_flag&SLOAD) == 0)
		fracmem = 0.0;
	else {
		szptudot = UPAGES + clrnd(ctopt(ap->a_size+ap->a_tsiz));
		fracmem = ((float)ap->a_rss+szptudot)/CLSIZE/ecmx;
		if (ap->a_xccount)
			fracmem += ((float)ap->a_txtrss)/CLSIZE/
			    ap->a_xccount/ecmx;
	}
	return (100.0 * fracmem);
}

double
pcpu()
{
	time_t time;
	double p;

	time = mproc->p_time;
	if (time == 0 || (mproc->p_flag&SLOAD) == 0)
		return (0.0);
	p = (double)mproc->p_pctcpu / FSCALE;
	if (rawcpu)
		return (100.0 * p);
	return (100.0 * p / (1.0 - exp(time * log(ccpu))));
}

/*
 * Get npte ptes from kernel address ptep into array kpte.
 * XXX - ptes must not cross a kernel page boundary.
 */
getkpte(ptep, npte, kpte)
	struct pte *ptep;
	int npte;
	struct pte kpte[];
{
	struct pte *pteaddr, apte;

	pteaddr = &Usrptma[btokmx(ptep)];
	if (kflg)
		pteaddr = (struct pte *)clear(pteaddr);
	klseek(kmem, (long)pteaddr, 0);
	if (read(kmem, (char *)&apte, sizeof (apte)) != sizeof (apte)) {
		printf("ps: can't read indir pte to get u for pid %d from %s\n",
		    mproc->p_pid, swapf);
		return (0);
	}
	klseek(mem, (long)ctob(apte.pg_pfnum) + (((int)ptep)&PGOFSET), 0);
	if (read(mem, (char *)kpte, npte * sizeof (struct pte)) !=
	    npte * sizeof (struct pte)) {
		printf("ps: can't read page table for u of pid %d from %s\n",
		    mproc->p_pid, kmemf);
		return (0);
	}
	return (1);
}

getu()
{
	struct pte apte;
	struct pte uutl[UPAGES];
	register int i;
	int ncl, size;

	size = sflg ? ctob(UPAGES) : roundup(sizeof (struct user), DEV_BSIZE);
	if ((mproc->p_flag & SLOAD) == 0) {
		if (swap < 0)
			return (0);
		(void) lseek(swap, (long)dtob(mproc->p_swaddr), 0);
		if (read(swap, (char *)&user.user, size) != size) {
			fprintf(stderr, "ps: can't read u for pid %d from %s\n",
			    mproc->p_pid, swapf);
			return (0);
		}
		pcbpf = 0;
		argaddr = 0;
		swaddr = mproc->p_swaddr;
		return (1);
	}
	swaddr = 0;
	if (getkpte(sptopte(mproc, CLSIZE-1), 1, &apte) == 0)
		return (0);
	if (apte.pg_fod == 0 && apte.pg_pfnum)
		argaddr = ctob(apte.pg_pfnum);
	else
		argaddr = 0;
	if (getkpte(mproc->p_addr, UPAGES, uutl) == 0)
		return (0);
	pcbpf = uutl[((char *)&u.u_pcb - (char *)&u) / CLBYTES].pg_pfnum;
	ncl = (size + CLBYTES - 1) / (CLBYTES);
	while (--ncl >= 0) {
		i = ncl * CLSIZE;
		klseek(mem, (long)ctob(uutl[i].pg_pfnum), 0);
		if (read(mem, user.upages[i], CLBYTES) != CLBYTES) {
			printf("ps: can't read page %d of u of pid %d from %s\n",
			    uutl[i].pg_pfnum, mproc->p_pid, memf);
			return (0);
		}
	}
	return (1);
}

char *
getcmd()
{
	char cmdbuf[CLBYTES];
	union {
		char	argc[CLBYTES];
		int	argi[CLBYTES/sizeof (int)];
	} argspac;
	register char *cp;
	register int *ip;
	char c;
	int nbad;
	struct dblock db;
	char *file;

	if (mproc->p_stat == SZOMB || mproc->p_flag&(SSYS|SWEXIT))
		return ("");
	if (cflg) {
		(void) strncpy(cmdbuf, u.u_comm, sizeof (u.u_comm));
		return (savestr(cmdbuf));
	}
	if (u.u_ssize == 0)
		goto retucomm;
	if ((mproc->p_flag & SLOAD) == 0 || argaddr == 0) {
		if (swap < 0)
			goto retucomm;
		vstodb(0, CLSIZE, &u.u_smap, &db, 1);
		(void) lseek(swap, (long)dtob(db.db_base), 0);
		if (read(swap, (char *)&argspac, sizeof(argspac))
		    != sizeof(argspac))
			goto bad;
		file = swapf;
	} else {
		klseek(mem, (long)argaddr, 0);
		if (read(mem, (char *)&argspac, sizeof (argspac))
		    != sizeof (argspac))
			goto bad;
		file = memf;
	}
	ip = &argspac.argi[CLBYTES/sizeof (int)];
	ip -= 2;		/* last arg word and .long 0 */
	while (*--ip)
		if (ip == argspac.argi)
			goto retucomm;
	*(char *)ip = ' ';
	ip++;
	nbad = 0;
	for (cp = (char *)ip; cp < &argspac.argc[CLBYTES]; cp++) {
		c = *cp & 0177;
		if (c == 0)
			*cp = ' ';
		else if (c < ' ' || c > 0176) {
			if (++nbad >= 5*(eflg+1)) {
				*cp++ = ' ';
				break;
			}
			*cp = '?';
		} else if (eflg == 0 && c == '=') {
			while (*--cp != ' ')
				if (cp <= (char *)ip)
					break;
			break;
		}
	}
	*cp = 0;
	while (*--cp == ' ')
		*cp = 0;
	cp = (char *)ip;
	(void) strncpy(cmdbuf, cp, &argspac.argc[CLBYTES] - cp);
	if (cp[0] == '-' || cp[0] == '?' || cp[0] <= ' ') {
		(void) strcat(cmdbuf, " (");
		(void) strncat(cmdbuf, u.u_comm, sizeof(u.u_comm));
		(void) strcat(cmdbuf, ")");
	}
/*
	if (xflg == 0 && gflg == 0 && tptr == 0 && cp[0] == '-')
		return (0);
*/
	return (savestr(cmdbuf));

bad:
	fprintf(stderr, "ps: error locating command name for pid %d from %s\n",
	    mproc->p_pid, file);
retucomm:
	(void) strcpy(cmdbuf, " (");
	(void) strncat(cmdbuf, u.u_comm, sizeof (u.u_comm));
	(void) strcat(cmdbuf, ")");
	return (savestr(cmdbuf));
}

char	*lhdr =
"      F UID   PID  PPID CP PRI NI ADDR  SZ RSS   WCHAN STAT TT  TIME";
lpr(sp)
	struct savcom *sp;
{
	register struct asav *ap = sp->ap;
	register struct lsav *lp = sp->s_un.lp;

	printf("%7x%4d%6u%6u%3d%4d%3d%5x%4d%4d",
	    ap->a_flag, ap->a_uid,
	    ap->a_pid, lp->l_ppid, lp->l_cpu&0377, ap->a_pri-PZERO,
	    ap->a_nice-NZERO, lp->l_addr, pgtok(ap->a_size), pgtok(ap->a_rss));
	printf(lp->l_wchan ? " %7x" : "        ", (int)lp->l_wchan&0xfffffff);
	printf(" %4.4s ", state(ap));
	ptty(ap->a_tty);
	ptime(ap);
}

ptty(tp)
	char *tp;
{

	printf("%-2.2s", tp);
}

ptime(ap)
	struct asav *ap;
{

	printf("%3ld:%02ld", ap->a_cpu / 60, ap->a_cpu % 60);
}

char	*uhdr =
"USER       PID %CPU %MEM   SZ  RSS TT STAT  TIME";
upr(sp)
	struct savcom *sp;
{
	register struct asav *ap = sp->ap;
	int vmsize, rmsize;

	vmsize = pgtok((ap->a_size + ap->a_tsiz));
	rmsize = pgtok(ap->a_rss);
	if (ap->a_xccount)
		rmsize += pgtok(ap->a_txtrss/ap->a_xccount);
	printf("%-8.8s %5d%5.1f%5.1f%5d%5d",
	    getname(ap->a_uid), ap->a_pid, sp->s_un.u_pctcpu, pmem(ap),
	    vmsize, rmsize);
	putchar(' ');
	ptty(ap->a_tty);
	printf(" %4.4s", state(ap));
	ptime(ap);
}

char *vhdr =
" SIZE  PID TT STAT  TIME SL RE PAGEIN SIZE  RSS  LIM TSIZ TRS %CPU %MEM"+5;
vpr(sp)
	struct savcom *sp;
{
	register struct vsav *vp = sp->s_un.vp;
	register struct asav *ap = sp->ap;

	printf("%5u ", ap->a_pid);
	ptty(ap->a_tty);
	printf(" %4.4s", state(ap));
	ptime(ap);
	printf("%3d%3d%7d%5d%5d",
	   ap->a_slptime > 99 ? 99 : ap-> a_slptime,
	   ap->a_time > 99 ? 99 : ap->a_time, vp->v_majflt,
	   pgtok(ap->a_size), pgtok(ap->a_rss));
	if (ap->a_maxrss == (RLIM_INFINITY/NBPG))
		printf("   xx");
	else
		printf("%5d", pgtok(ap->a_maxrss));
	printf("%5d%4d%5.1f%5.1f",
	   pgtok(ap->a_tsiz), pgtok(ap->a_txtrss), vp->v_pctcpu, pmem(ap));
}

char	*shdr =
"SSIZ   PID TT STAT  TIME";
spr(sp)
	struct savcom *sp;
{
	register struct asav *ap = sp->ap;

	if (sflg)
		printf("%4d ", sp->s_un.s_ssiz);
	printf("%5u", ap->a_pid);
	putchar(' ');
	ptty(ap->a_tty);
	printf(" %4.4s", state(ap));
	ptime(ap);
}

char *
state(ap)
	register struct asav *ap;
{
	char stat, load, nice, anom;
	static char res[5];

	switch (ap->a_stat) {

	case SSTOP:
		stat = 'T';
		break;

	case SSLEEP:
		if (ap->a_pri >= PZERO)
			if (ap->a_slptime >= MAXSLP)
				stat = 'I';
			else
				stat = 'S';
		else if (ap->a_flag & SPAGE)
			stat = 'P';
		else
			stat = 'D';
		break;

	case SWAIT:
	case SRUN:
	case SIDL:
		stat = 'R';
		break;

	case SZOMB:
		stat = 'Z';
		break;

	default:
		stat = '?';
	}
	load = ap->a_flag & SLOAD ? (ap->a_rss>ap->a_maxrss ? '>' : ' ') : 'W';
	if (ap->a_nice < NZERO)
		nice = '<';
	else if (ap->a_nice > NZERO)
		nice = 'N';
	else
		nice = ' ';
	anom = (ap->a_flag&SUANOM) ? 'A' : ((ap->a_flag&SSEQL) ? 'S' : ' ');
	res[0] = stat; res[1] = load; res[2] = nice; res[3] = anom;
	return (res);
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

/*ARGSUSED*/
panic(cp)
	char *cp;
{

#ifdef DEBUG
	printf("%s\n", cp);
#endif
}

min(a, b)
{

	return (a < b ? a : b);
}

pscomp(s1, s2)
	struct savcom *s1, *s2;
{
	register int i;

	if (uflg)
		return (s2->s_un.u_pctcpu > s1->s_un.u_pctcpu ? 1 : -1);
	if (vflg)
		return (vsize(s2) - vsize(s1));
	i = s1->ap->a_ttyd - s2->ap->a_ttyd;
	if (i == 0)
		i = s1->ap->a_pid - s2->ap->a_pid;
	return (i);
}

vsize(sp)
	struct savcom *sp;
{
	register struct asav *ap = sp->ap;
	register struct vsav *vp = sp->s_un.vp;
	
	if (ap->a_flag & SLOAD)
		return (ap->a_rss +
		    ap->a_txtrss / (ap->a_xccount ? ap->a_xccount : 1));
	return (vp->v_swrss + (ap->a_xccount ? 0 : vp->v_txtswrss));
}

#define	NMAX	8	/* sizeof loginname (should be sizeof (utmp.ut_name)) */
#define NUID	2048	/* must not be a multiple of 5 */

struct nametable {
	char	nt_name[NMAX+1];
	int	nt_uid;
} nametable[NUID];

struct nametable *
findslot(uid)
unsigned short	uid;
{
	register struct nametable	*n, *start;

	/*
	 * find the uid or an empty slot.
	 * return NULL if neither found.
	 */

	n = start = nametable + (uid % (NUID - 20));
	while (n->nt_name[0] && n->nt_uid != uid) {
		if ((n += 5) >= &nametable[NUID])
			n -= NUID;
		if (n == start)
			return((struct nametable *)NULL);
	}
	return(n);
}

char *
getname(uid)
{
	register struct passwd		*pw;
	static				init = 0;
	struct passwd			*getpwent();
	register struct nametable	*n;

	/*
	 * find uid in hashed table; add it if not found.
	 * return pointer to name.
	 */

	if ((n = findslot(uid)) == NULL)
		return((char *)NULL);

	if (n->nt_name[0])	/* occupied? */
		return(n->nt_name);

	switch (init) {
		case 0:
			setpwent();
			init = 1;
			/* intentional fall-thru */
		case 1:
			while (pw = getpwent()) {
				if (pw->pw_uid < 0)
					continue;
				if ((n = findslot(pw->pw_uid)) == NULL) {
					endpwent();
					init = 2;
					return((char *)NULL);
				}
				if (n->nt_name[0])
					continue;	/* duplicate, not uid */
				strncpy(n->nt_name, pw->pw_name, NMAX);
				n->nt_uid = pw->pw_uid;
				if (pw->pw_uid == uid)
					return (n->nt_name);
			}
			endpwent();
			init = 2;
			/* intentional fall-thru */
		case 2:
			return ((char *)NULL);
	}
}

char	*freebase;
int	nleft;

char *
alloc(size)
	int size;
{
	register char *cp;
	register int i;

#ifdef sun
	size = (size+1)&~1;
#endif
	if (size > nleft) {
		freebase = (char *)sbrk((int)(i = size > 2048 ? size : 2048));
		if (freebase == (char *)-1) {
			fprintf(stderr, "ps: ran out of memory\n");
			exit(1);
		}
		nleft = i - size;
	} else
		nleft -= size;
	cp = freebase;
	for (i = size; --i >= 0; )
		*cp++ = 0;
	freebase = cp;
	return (cp - size);
}

char *
savestr(cp)
	char *cp;
{
	register int len;
	register char *dp;

	len = strlen(cp);
	dp = (char *)alloc(len+1);
	(void) strcpy(dp, cp);
	return (dp);
}
