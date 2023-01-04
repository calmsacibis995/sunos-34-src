#ifndef lint
static	char sccsid[] = "@(#)find.c 1.1 86/09/25 SMI"; /* from S5R2 4.7 */
#endif

/*	find	COMPILE:	cc -o find -s -O -i find.c -lS	*/
#include	<stdio.h>
#include	<pwd.h>
#include	<grp.h>

#define	UID	1
#define	GID	2

#ifdef RT
#include <rt/types.h>
#include <rt/dir.h>
#include <rt/stat.h>
#else
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <ufs/fs.h>
#include <mntent.h>
#include <ctype.h>
#endif
#define A_DAY	86400L /* a day full of seconds */
#define EQ(x, y)	(strcmp(x, y)==0)
#define BUFSIZE	512	/* In u370 I can't use BUFSIZ nor BSIZE */
#define CPIOBSZ	4096
#define Bufsize	5120

int	Randlast;
char	Pathname[MAXPATHLEN+1];

#define	MAXNODES	400

struct anode {
	int (*F)();
	struct anode *L, *R;
} Node[MAXNODES];
int Nn;  /* number of nodes */
char	*Fname;
char	Needfs;		/* don't compute Fstype unless this is true */
char	*Fstype;
dev_t	Fsnum;
long	Now;
int	Argc,
	Ai,
	Pi;
char	**Argv;
/* cpio stuff */
int	Cpio;
short	*SBuf, *Dbuf, *Wp;
char	*Buf, *Cbuf, *Cp;
char	Strhdr[500],
	*Chdr = Strhdr;
int	Wct = Bufsize / 2, Cct = Bufsize;
int	Cflag;
int	depthf = 0;

long	Newer;
int	giveup = 0;	/* abort search in this directory */

int	Xdev = 1;	/* true if SHOULD cross devices (file systems) */
struct	stat Devstat;	/* stats of each argument path's file system */

struct stat Statb;

struct	anode	*exp(),
		*e1(),
		*e2(),
		*e3(),
		*mk();
char	*nxtarg();
char	Home[MAXPATHLEN + 1];
long	Blocks;
char *strcpy(), *strrchr();
char *sbrk();
extern long	lseek(), time();
char *gettype();
	struct header {
		short	h_magic,
			h_dev;
		ushort	h_ino,
			h_mode,
			h_uid,
			h_gid;
		short	h_nlink,
			h_rdev,
			h_mtime[2],
			h_namesize,
			h_filesize[2];
		char	h_name[256];
	} hdr;
#ifdef RT
short	Extend;
#endif
char	Symlbuf[MAXPATHLEN + 1];	/* target of symbolic link */

/*
 * SEE ALSO:	updatedb, bigram.c, code.c
 *		Usenix ;login:, February/March, 1983, p. 8.
 *
 * REVISIONS: 	James A. Woods, Informatics General Corporation,
 *		NASA Ames Research Center, 6/81.
 *
 *		The second form searches a pre-computed filelist
 *		(constructed nightly by /usr/lib/crontab) which is
 *		compressed by updatedb (v.i.z.)  The effect of
 *			find <name>
 *		is similar to
 *			find / +0 -name "*<name>*" -print
 *		but much faster.
 *
 *		8/82 faster yet + incorporation of bigram coding -- jaw
 *
 *		1/83 incorporate glob-style matching -- jaw
 */
#define	AMES	1

main(argc, argv)
	int argc;
	char *argv[];
{
	struct anode *exlist;
	int paths;
	register char *cp, *sp;
#ifdef	SUID_PWD
	FILE *pwd, *popen();
#endif

#ifdef  AMES
	if (argc < 2) {
		fprintf(stderr,
			"Usage: find name, or find path-list predicate-list\n");
		exit(1);
	}
	if (argc == 2) {
		fastfind(argv[1]);
		exit(0);
	}
#endif
	time(&Now);
#ifdef	SUID_PWD
	pwd = popen("pwd", "r");
	fgets(Home, sizeof Home, pwd);
	pclose(pwd);
	Home[strlen(Home) - 1] = '\0';
#else
	if (getwd(Home) == NULL) {
		fprintf(stderr, "find: %s\n", Home);
		exit(1);
	}
#endif
	Argc = argc; Argv = argv;
	if(argc<3) {
usage:		fprintf(stderr,"Usage: find path-list predicate-list\n");
		exit(1);
	}
	for(Ai = paths = 1; Ai < (argc-1); ++Ai, ++paths)
		if(*Argv[Ai] == '-' || EQ(Argv[Ai], "(") || EQ(Argv[Ai], "!"))
			break;
	if(paths == 1) /* no path-list */
		goto usage;
	if(!(exlist = exp())) { /* parse and compile the arguments */
		fprintf(stderr,"find: parsing error\n");
		exit(1);
	}
	if(Ai<argc) {
		fprintf(stderr,"find: missing conjunction\n");
		exit(1);
	}
	for(Pi = 1; Pi < paths; ++Pi) {
		sp = "\0";
		strcpy(Pathname, Argv[Pi]);
		if(*Pathname != '/') {
			if (chdir(Home) < 0) {
				fprintf(stderr,"find: can't chdir to %s: ");
				perror("");
				exit(1);
			}
		}
		if(cp = strrchr(Pathname, '/')) {
			sp = cp + 1;
			*cp = '\0';
			if(chdir(*Pathname? Pathname: "/") == -1) {
				fprintf(stderr,"find: bad starting directory %s: ");
				perror("");
				exit(2);
			}
			*cp = '/';
		}
		Fname = *sp? sp: Pathname;
		if (Needfs)
			Fstype = gettype(Fname);
		if (!Xdev)
			stat(Pathname, &Devstat);
		/* to find files that match  */
		descend(Pathname, Fname, Fstype, 0, exlist);
	}
	if(Cpio) {
		strcpy(Pathname, "TRAILER!!!");
		Statb.st_size = 0;
		cpio();
		printf("%ld blocks\n", Blocks*10);
	}
	exit(0);
}

/* compile time functions:  priority is  exp()<e1()<e2()<e3()  */

struct anode *exp() { /* parse ALTERNATION (-o)  */
	int or();
	register struct anode * p1;

	p1 = e1() /* get left operand */ ;
	if(EQ(nxtarg(), "-o")) {
		Randlast--;
		return(mk(or, p1, exp()));
	}
	else if(Ai <= Argc) --Ai;
	return(p1);
}
struct anode *e1() { /* parse CONCATENATION (formerly -a) */
	int and();
	register struct anode * p1;
	register char *a;

	p1 = e2();
	a = nxtarg();
	if(EQ(a, "-a")) {
And:
		Randlast--;
		return(mk(and, p1, e1()));
	} else if(EQ(a, "(") || EQ(a, "!") || (*a=='-' && !EQ(a, "-o"))) {
		--Ai;
		goto And;
	} else if(Ai <= Argc) --Ai;
	return(p1);
}
struct anode *e2() { /* parse NOT (!) */
	int not();

	if(Randlast) {
		fprintf(stderr,"find: operand follows operand\n");
		exit(1);
	}
	Randlast++;
	if(EQ(nxtarg(), "!"))
		return(mk(not, e3(), (struct anode *)0));
	else if(Ai <= Argc) --Ai;
	return(e3());
}
struct anode *e3() { /* parse parens and predicates */
	int exeq(), ok(), glob(),  mtime(), atime(), Ctime(), user(),
		group(), size(), csize(), perm(), links(), print(),
		type(), fstype(), ino(), cpio(), newer(), prune(),
		nouser(), nogroup(), ls(), dummy();
	struct anode *p1;
	struct anode *mkret;
	int i;
	register char *a, *b, s;

	a = nxtarg();
	if(EQ(a, "(")) {
		Randlast--;
		p1 = exp();
		a = nxtarg();
		if(!EQ(a, ")")) goto err;
		return(p1);
	}
	else if(EQ(a, "-print")) {
		return(mk(print, (struct anode *)0, (struct anode *)0));
	}
	else if(EQ(a, "-depth")) {
		depthf = 1;
		return(mk(dummy, (struct anode *)0, (struct anode *)0));
	}
	else if(EQ(a, "-prune")) {
		return(mk(prune, (struct anode *)0, (struct anode *)0));
	}
	else if (EQ(a, "-nouser")) {
		return (mk(nouser, (struct anode *)0, (struct anode *)0));
	}
	else if (EQ(a, "-nogroup")) {
		return (mk(nogroup, (struct anode *)0, (struct anode *)0));
	}
	else if (EQ(a, "-ls")) {
		return (mk(ls, (struct anode *)0, (struct anode *)0));
	}
	else if (EQ(a, "-xdev")) {
		Xdev = 0;
		return (mk(dummy, (struct anode *)0, (struct anode *)0));
	}
	b = nxtarg();
	s = *b;
	if(s=='+') b++;
	if(EQ(a, "-name"))
		return(mk(glob, (struct anode *)b, (struct anode *)0));
	else if(EQ(a, "-mtime"))
		return(mk(mtime, (struct anode *)atoi(b), (struct anode *)s));
	else if(EQ(a, "-atime"))
		return(mk(atime, (struct anode *)atoi(b), (struct anode *)s));
	else if(EQ(a, "-ctime"))
		return(mk(Ctime, (struct anode *)atoi(b), (struct anode *)s));
	else if(EQ(a, "-user")) {
		if((i=getunum(UID, b)) == -1) {
			if(gmatch(b, "[0-9][0-9]*"))
				return mk(user, (struct anode *)atoi(b), (struct anode *)s);
			fprintf(stderr,"find: cannot find -user name %s\n", b);
			exit(1);
		}
		return(mk(user, (struct anode *)i, (struct anode *)s));
	}
	else if(EQ(a, "-inum"))
		return(mk(ino, (struct anode *)atoi(b), (struct anode *)s));
	else if(EQ(a, "-group")) {
		if((i=getunum(GID, b)) == -1) {
			if(gmatch(b, "[0-9][0-9]*"))
				return mk(group, (struct anode *)atoi(b), (struct anode *)s);
			fprintf(stderr,"find: cannot find -group name %s\n", b);
			exit(1);
		}
		return(mk(group, (struct anode *)i, (struct anode *)s));
	} else if(EQ(a, "-size")) {
		mkret = mk(size, (struct anode *)atoi(b), (struct anode *)s);
		if(*b == '+' || *b == '-')b++;
		while(isdigit(*b))b++;
		if(*b == 'c') Node[Nn-1].F = csize;
		return(mkret);
	} else if(EQ(a, "-links"))
		return(mk(links, (struct anode *)atoi(b), (struct anode *)s));
	else if(EQ(a, "-perm")) {
		for(i=0; *b ; ++b) {
			if(*b=='-') continue;
			i <<= 3;
			i = i + (*b - '0');
		}
		return(mk(perm, (struct anode *)i, (struct anode *)s));
	}
	else if(EQ(a, "-type")) {
		i = s=='d' ? S_IFDIR :
		    s=='b' ? S_IFBLK :
		    s=='c' ? S_IFCHR :
		    s=='p' ? S_IFIFO :
		    s=='f' ? S_IFREG :
		    s=='l' ? S_IFLNK :
		    s=='s' ? S_IFSOCK :
#ifdef RT
		    s=='r' ? S_IFREC :
		    s=='m' ? S_IFEXT :
		    s=='1' ? S_IF1EXT:
#endif
		    0;
		return(mk(type, (struct anode *)i, (struct anode *)0));
	}
	else if(EQ(a, "-fstype")) {
		Needfs = 1;
		return(mk(fstype, (struct anode *)b, (struct anode *)0));
	}
	else if (EQ(a, "-exec")) {
		i = Ai - 1;
		while(!EQ(nxtarg(), ";"));
		return(mk(exeq, (struct anode *)i, (struct anode *)0));
	}
	else if (EQ(a, "-ok")) {
		i = Ai - 1;
		while(!EQ(nxtarg(), ";"));
		return(mk(ok, (struct anode *)i, (struct anode *)0));
	}
	else if(EQ(a, "-cpio")) {
		if((Cpio = creat(b, 0666)) < 0) {
			fprintf(stderr,"find: cannot create %s: ", b);
			perror("");
			exit(1);
		}
		SBuf = (short *)sbrk(CPIOBSZ);
		Wp = Dbuf = (short *)sbrk(Bufsize);
		if (SBuf == NULL || Dbuf == NULL) {
			perror("find");
			exit(1);
		}
#ifdef RT
		setio(-1,1);	/* turn on physio */
#endif
		depthf = 1;
		return(mk(cpio, (struct anode *)0, (struct anode *)0));
	}
	else if(EQ(a, "-ncpio")) {
		if((Cpio = creat(b, 0666)) < 0) {
			fprintf(stderr,"find: cannot create %s: ", b);
			perror("");
			exit(1);
		}
		Buf = (char*)sbrk(CPIOBSZ);
		Cp = Cbuf = (char *)sbrk(Bufsize);
		if (Buf == NULL || Cbuf == NULL) {
			perror("find");
			exit(1);
		}
#ifdef RT
		setio(-1,1);	/* turn on physio */
#endif
		Cflag++;
		depthf = 1;
		return(mk(cpio, (struct anode *)0, (struct anode *)0));
	}
	else if(EQ(a, "-newer")) {
		if(stat(b, &Statb) < 0) {
			fprintf(stderr,"find: cannot access %s: ", b);
			perror("");
			exit(1);
		}
		Newer = Statb.st_mtime;
		return mk(newer, (struct anode *)0, (struct anode *)0);
	}
err:	fprintf(stderr,"find: bad option %s\n", a);
	exit(1);
	/*NOTREACHED*/
}
struct anode *mk(f, l, r)
int (*f)();
struct anode *l, *r;
{
	if (Nn >= MAXNODES) {
		fprintf(stderr, "find: Too many options\n");
		exit(1);
	}

	Node[Nn].F = f;
	Node[Nn].L = l;
	Node[Nn].R = r;
	return(&(Node[Nn++]));
}

char *nxtarg() { /* get next arg from command line */
	static strikes = 0;

	if(strikes==3) {
		fprintf(stderr,"find: incomplete statement\n");
		exit(1);
	}
	if(Ai>=Argc) {
		strikes++;
		Ai = Argc + 1;
		return("");
	}
	return(Argv[Ai++]);
}

/* execution time functions */
and(p)
register struct anode *p;
{
	return(((*p->L->F)(p->L)) && ((*p->R->F)(p->R))?1:0);
}
or(p)
register struct anode *p;
{
	 return(((*p->L->F)(p->L)) || ((*p->R->F)(p->R))?1:0);
}
not(p)
register struct anode *p;
{
	return( !((*p->L->F)(p->L)));
}
glob(p)
register struct { int f; char *pat; } *p; 
{
	return(gmatch(Fname, p->pat));
}
/*ARGSUSED*/
print(p)
struct anode *p;
{
	puts(Pathname);
	return(1);
}
mtime(p)
register struct { int f, t, s; } *p; 
{
	return(scomp((int)((Now - Statb.st_mtime) / A_DAY), p->t, p->s));
}
atime(p)
register struct { int f, t, s; } *p; 
{
	return(scomp((int)((Now - Statb.st_atime) / A_DAY), p->t, p->s));
}
Ctime(p)
register struct { int f, t, s; } *p; 
{
	return(scomp((int)((Now - Statb.st_ctime) / A_DAY), p->t, p->s));
}
user(p)
register struct { int f, u, s; } *p; 
{
	return(scomp(Statb.st_uid, p->u, p->s));
}
/*ARGSUSED*/
nouser(p)
struct anode *p;
{
	char *getname();

	return (getname(Statb.st_uid) == NULL);
}
ino(p)
register struct { int f, u, s; } *p;
{
	return(scomp((int)Statb.st_ino, p->u, p->s));
}
group(p)
register struct { int f, u; } *p; 
{
	return(p->u == Statb.st_gid);
}
/*ARGSUSED*/
nogroup(p)
struct anode *p;
{
	char *getgroup();

	return (getgroup(Statb.st_gid) == NULL);
}
links(p)
register struct { int f, link, s; } *p; 
{
	return(scomp(Statb.st_nlink, p->link, p->s));
}
size(p)
register struct { int f, sz, s; } *p; 
{
	return(scomp((int)((Statb.st_size+(BUFSIZE - 1))/BUFSIZE), p->sz, p->s));
}
csize(p)
register struct { int f, sz, s; } *p; 
{
	return(scomp((int)Statb.st_size, p->sz, p->s));
}
perm(p)
register struct { int f, per, s; } *p; 
{
	register i;
	i = (p->s=='-') ? p->per : 07777; /* '-' means only arg bits */
	return((Statb.st_mode & i & 07777) == p->per);
}
type(p)
register struct { int f, per, s; } *p;
{
	return((Statb.st_mode&S_IFMT)==p->per);
}
fstype(p)
register struct { int f; char *typename } *p;
{
	return(Fstype && !strcmp(Fstype, p->typename));
}
prune(p)
register struct { int f, per, s; } *p;
{
	giveup = 1;
	return(1);
}
exeq(p)
register struct { int f, com; } *p;
{
	fflush(stdout); /* to flush possible `-print' */
	return(doex(p->com));
}
ok(p)
struct { int f, com; } *p;
{
	int c, yes=0;

	fflush(stdout); /* to flush possible `-print' */
	fprintf(stderr,"< %s ... %s >?   ", Argv[p->com], Pathname);
	fflush(stderr);
	if((c=getchar())=='y') yes = 1;
	while(c!='\n')
		if(c==EOF)
			exit(2);
		else
			c = getchar();
	return(yes? doex(p->com): 0);
}

#define MKSHORT(v, lv) {U.l=1L;if(U.c[0]) U.l=lv, v[0]=U.s[1], v[1]=U.s[0]; else U.l=lv, v[0]=U.s[0], v[1]=U.s[1];}
union { long l; short s[2]; char c[4]; } U;
long mklong(v)
short v[];
{
	U.l = 1;
	if(U.c[0] /* VAX */)
		U.s[0] = v[1], U.s[1] = v[0];
	else
		U.s[0] = v[0], U.s[1] = v[1];
	return U.l;
}

/*ARGSUSED*/
cpio(p)
struct anode *p;
{
#define MAGIC 070707
#define HDRSIZE	(sizeof hdr - 256)
#define CHARS	76
	register ifile, ct;
	static long fsz;
	register i;

	strcpy(hdr.h_name, !strncmp(Pathname, "./", 2)? Pathname+2: Pathname);
	hdr.h_magic = MAGIC;
	hdr.h_namesize = strlen(hdr.h_name) + 1;
	hdr.h_uid = Statb.st_uid;
	hdr.h_gid = Statb.st_gid;
	hdr.h_dev = Statb.st_dev;
	hdr.h_ino = Statb.st_ino;
	hdr.h_mode = Statb.st_mode;
	hdr.h_nlink = Statb.st_nlink;
	hdr.h_rdev = Statb.st_rdev;
	MKSHORT(hdr.h_mtime, Statb.st_mtime);
	fsz = ((hdr.h_mode & S_IFMT) == S_IFREG ||
		(hdr.h_mode & S_IFMT) == S_IFLNK)? Statb.st_size: 0L;
#ifdef RT
	if ((hdr.h_mode & S_IFMT) == S_IFEXT
		|| (hdr.h_mode & S_IFMT) == S_IF1EXT) {
		Extend = 1;
		fsz = Statb.st_size;
	} else Extend = 0;
#endif
	MKSHORT(hdr.h_filesize, fsz);

	if (Cflag)
		bintochar(fsz);

	if(EQ(hdr.h_name, "TRAILER!!!")) {
		Cflag? writehdr(Chdr, CHARS + hdr.h_namesize):
			bwrite((short *)&hdr, HDRSIZE + hdr.h_namesize);
		for (i = 0; i < 10; ++i)
			Cflag? writehdr(Buf, BUFSIZE): bwrite(SBuf, BUFSIZE);
		return 0;
	}
	if(!mklong(hdr.h_filesize)) {
		Cflag? writehdr(Chdr, CHARS + hdr.h_namesize):
			bwrite((short *)&hdr, HDRSIZE + hdr.h_namesize);
#ifdef RT
		if (Extend)
			Cflag? writehdr(Chdr, CHARS + hdr.h_namesize):
				bwrite((short *)&hdr, HDRSIZE + hdr.h_namesize);
#endif
		return 0;
	} else if((hdr.h_mode & S_IFMT) == S_IFLNK) {
		if (readlink(Fname, Symlbuf, (int)fsz) < 0) {
			fprintf(stderr,"Cannot read symbolic link <%s>: ", hdr.h_name);
			perror("");
			return 0;
		}
		Symlbuf[(int)fsz] = '\0';
		Cflag? writehdr(Chdr, CHARS + hdr.h_namesize):
			bwrite((short *)&hdr, HDRSIZE + hdr.h_namesize);
		Cflag? writehdr(Symlbuf,(int)fsz):
			bwrite((short *)Symlbuf,(int)fsz);
		return 0;
	}
	if((ifile = open(Fname, 0)) < 0) {
cerror:
		fprintf(stderr,"find: cannot copy %s: ", hdr.h_name);
		perror("");
		return 0;
	}
	Cflag? writehdr(Chdr, CHARS + hdr.h_namesize):
		bwrite((short *)&hdr, HDRSIZE+hdr.h_namesize);
#ifdef RT
	if (Extend)
		Cflag? writehdr(Chdr, CHARS + hdr.h_namesize):
			bwrite((short *)&hdr, HDRSIZE + hdr.h_namesize);
#endif
	for(fsz = mklong(hdr.h_filesize); fsz > 0; fsz -= CPIOBSZ) {
		ct = fsz>CPIOBSZ? CPIOBSZ: fsz;
		if(read(ifile, Cflag? Buf: (char *)SBuf, ct) < 0)  {
			fprintf(stderr,"find: Cannot read %s: ", hdr.h_name);
			perror("");
			continue;
		}
		Cflag? writehdr(Buf, ct): bwrite(SBuf, ct);
	}
	close(ifile);
	return 1;
}

bintochar(t)
long t;
{
#ifdef u370
	sprintf(Chdr,"%06ho%06ho%06ho%06ho%06ho%06ho%06ho%06ho%011lo%06ho%011lo%s",
#else
	sprintf(Chdr, "%.6ho%.6ho%.6ho%.6ho%.6ho%.6ho%.6ho%.6ho%.11lo%.6ho%.11lo%s",
#endif
		MAGIC,Statb.st_dev,Statb.st_ino,Statb.st_mode,Statb.st_uid,
		Statb.st_gid,Statb.st_nlink,Statb.st_rdev & 00000177777,
		Statb.st_mtime,(short)strlen(hdr.h_name)+1,t,hdr.h_name);
}

/*ARGSUSED*/
newer(p)
struct anode *p;
{
	return Statb.st_mtime > Newer;
}
/*ARGSUSED*/
ls(p)
struct anode *p;
{
	list(Pathname, &Statb);
	return (1);
}
/*ARGSUSED*/
dummy(p)
struct anode *p;
{
	/* dummy */
	return (1);
}

/* support functions */
scomp(a, b, s) /* funny signed compare */
register a, b;
register int s;
{
	if(s == '+')
		return(a > b);
	if(s == '-')
		return(a < -(b));
	return(a == b);
}

doex(com)
{
	register np;
	register char *na;
	static char *nargv[50];
	static ccode;
	register int w, pid, omask;

	ccode = np = 0;
	while (na=Argv[com++]) {
		if(np >= sizeof nargv / sizeof *nargv - 1) break;
		if(strcmp(na, ";")==0) break;
		if(strcmp(na, "{}")==0) nargv[np++] = Pathname;
		else nargv[np++] = na;
	}
	nargv[np] = 0;
	if (np==0) return(9);
	switch (pid = vfork()) {
	case -1:
		perror("find: Can't fork");
		exit(1);
		break;

	case 0:
		chdir(Home);
		execvp(nargv[0], nargv);
		write(2, "find: Can't execute ", 20);
		_perror(nargv[0]);
		/*
		 * Kill ourselves; our exit status will be a suicide
		 * note indicating we couldn't do the "exec".
		 */
		kill(getpid(), SIGUSR1);
		_exit(1);
		break;

	default:
		omask = sigblock(sigmask(SIGINT)|sigmask(SIGQUIT));
		while ((w = wait(&ccode)) != pid && w != -1)
			;
		(void) sigsetmask(omask);
		if ((ccode&0177) == SIGUSR1)
			exit(1);	/* "exec" failed in child */
		return (ccode != 0 ? 0 : 1);
	}
	/*NOTREACHED*/
}

getunum(t, s)
int	t;
char	*s;
{
	register i;
	struct	passwd	*getpwnam(), *pw;
	struct	group	*getgrnam(), *gr;

	i = -1;
	if( t == UID ){
		if( ((pw = getpwnam( s )) != (struct passwd *)NULL) && pw != (struct passwd *)EOF )
			i = pw->pw_uid;
	} else {
		if( ((gr = getgrnam( s )) != (struct group *)NULL) && gr != (struct group *)EOF )
			i = gr->gr_gid;
	}
	return(i);
}

descend(name, fname, pfstype, pfsnum, exlist)
struct anode *exlist;
dev_t pfsnum;		/* parent's dev */
char *name, *fname;
char *pfstype;		/* parent's fstype */
{
	DIR	*dir = NULL; /* open directory */
	int	cdval = 0;
	register struct direct	*dp;
	register char *c1, *c2;
	register long	offset;
	char *endofname;
	dev_t cfsnum;		/* current dev */
	char *cfstype;		/* current fstype */

	if(lstat(fname, &Statb)<0) {
		fprintf(stderr,"find: cannot stat %s: ", name);
		perror("");
		return(0);
	}
	cfsnum = Statb.st_dev;
	if (Needfs) {
		if (cfsnum != pfsnum) {
			if ((Fstype = gettype(Fname)) == NULL)
				return(0);	/* can't get type */
		} else
			Fstype = pfstype;
		cfstype = Fstype;	/* squirrel this away on stack */
	}
	if(!depthf) {
		(*exlist->F)(exlist);
		if (giveup) {
			giveup = 0;
			return(1);
		}
	}
	if((Statb.st_mode&S_IFMT)!=S_IFDIR ||
	   !Xdev && Devstat.st_dev != Statb.st_dev) {
		if(depthf) {
			(*exlist->F)(exlist);
			if (giveup) {
				giveup = 0;
				return(1);
			}
		}
		return(1);
	}

	for(c1 = name; *c1; ++c1);
	if((int)(c1-name) >= MAXPATHLEN-sizeof dp->d_name) {
		fprintf(stderr,"Pathname too long");
		exit(2);
	}
	if(*(c1-1) == '/')
		--c1;
	endofname = c1;
	if((cdval=chdir(fname)) == -1) {
		fprintf(stderr,"find: cannot chdir to %s: ", name);
		perror("");
	} else {
		for(offset=0L; ;) { /* each directory entry */
			if(dir==NULL) {
				if((dir=opendir("."))==NULL) {
					fprintf(stderr,"find: cannot open %s: ", name);
					perror("");
					break;
				}
				if(offset) seekdir(dir, offset);
			}
			if((dp = readdir(dir)) == NULL)
				break;
			if((dp->d_name[0]=='.' && dp->d_name[1]=='\0')
			|| (dp->d_name[0]=='.' && dp->d_name[1]=='.' && dp->d_name[2]=='\0'))
				continue;
			c1 = endofname;
			*c1++ = '/';
			c2 = dp->d_name;
			while((*c1++ = *c2++)!='\0')
				;
			if(c1 == (endofname + 2)) {
				break;
			}
			Fname = endofname+1;
			if(dir->dd_fd > 10) {
				offset = telldir(dir);
				closedir(dir);
				dir = NULL;
			}
			descend(name, Fname, cfstype, cfsnum, exlist);
		}
	}
	if(dir!=NULL)
		closedir(dir);
	c1 = endofname;
	*c1 = '\0';
	if(cdval == -1 || chdir("..") == -1) {
		if((endofname=strrchr(Pathname,'/')) == Pathname)
			if(chdir("/") == -1) {
				fprintf(stderr,"find: cannot change directory to /: ");
				perror("");
				exit(1);
			}
		else {
			if(endofname != NULL)
				*endofname = '\0';
			if(chdir(Home) == -1) {
				fprintf(stderr,"find: cannot change directory to %s: ", Home);
				perror("");
				exit(1);
			}
			if(chdir(Pathname) == -1) {
				fprintf(stderr,"find: bad directory tree (cannot change to %s): ", Pathname);
				perror("");
				exit(1);
			}
			if(endofname != NULL)
				*endofname = '/';
		}
	}
	if(depthf) {
		if(stat(fname, &Statb) < 0) {
			fprintf(stderr,"find: cannot stat %s: ", fname);
			perror("");
		}
		(*exlist->F)(exlist);
	}
/*	*c1 = '/';	*/
	return(0);
}

gmatch(s, p) /* string match as in glob */
register char *s, *p;
{
	if (*s=='.' && *p!='.') return(0);
	return amatch(s, p);
}

amatch(s, p)
register char	*s, *p;
{
	register int	scc;
	char		c;

	if(scc = *s++) {
		if((scc &= 0177) == 0) {
			scc = 0200;
		}
	}
	switch(c = *p++) {

	case '[':
		{
			int ok; 
			int lc; 
			int notflag = 0;

			ok = 0; 
			lc = 077777;
			if( *p == '!' ) {
				notflag = 1; 
				p++; 
			}
			while( c = *p++ ) {
				if(c == ']') {
					return(ok?amatch(s,p):0);
				} else if (c == '-') {
					if(notflag) {
						if(lc > scc || scc > *(p++)) {
							ok++;
						} else { 
							return(0);
						}
					} else { 
						if( lc <= scc && scc <= (*p++)) {
							ok++;
						}
					}
				} else {
					if(notflag) {
						if(scc != (lc = (c&0177))) {
							ok++;
						} else {
							return(0);
						}
					} else { 
						if(scc == (lc = (c&0177))) { 
							ok++;
						}
					}
				}
			}
			return(0);
		}
	case '?':
		return(scc?amatch(s,p):0);

	case '*':
		if(*p == 0) {
			return(1);
		}
		--s;
		while(*s) {
			if(amatch(s++,p)) {
				return(1);
			} 
		}
		return(0);

	case 0:
		return(scc == 0);

	default:
		if((c&0177) != scc) {
			return(0);
		}

	}
	return(amatch(s,p)?1:0);
}

bwrite(rp, c)
register short *rp;
register c;
{
	register short *wp = Wp;

	c = (c+1) >> 1;
	while(c--) {
		if(!Wct) {
again:
			if(write(Cpio, (char *)Dbuf, Bufsize)<0) {
				Cpio = chgreel(1, Cpio);
				goto again;
			}
			Wct = Bufsize >> 1;
			wp = Dbuf;
			++Blocks;
		}
		*wp++ = *rp++;
		--Wct;
	}
	Wp = wp;
}

writehdr(rp, c)
register char *rp;
register c;
{
	register char *cp = Cp;

	while (c--)  {
		if (!Cct)  {
again:
			if(write(Cpio, Cbuf, Bufsize) < 0)  {
				Cpio = chgreel(1, Cpio);
				goto again;
			}
			Cct = Bufsize;
			cp = Cbuf;
			++Blocks;
		}
		*cp++ = *rp++;
		--Cct;
	}
	Cp = cp;
}

chgreel(x, fl)
{
	register f;
	char str[22];
	FILE *devtty;
	struct stat statb;

	fprintf(stderr,"find: can't %s: ", (x? "write output": "read input"));
	perror("");
	fstat(fl, &statb);
	if((statb.st_mode&S_IFMT) != S_IFCHR)
		exit(1);
again:
	fprintf(stderr,"If you want to go on, type device/file name when ready\n");
	devtty = fopen("/dev/tty", "r");
	fgets(str, 20, devtty);
	str[strlen(str) - 1] = '\0';
	if(!*str)
		exit(1);
	close(fl);
	if((f = open(str, x? 1: 0)) < 0) {
		fprintf(stderr,"Can't open <%s>: ", str);
		perror("");
		fclose(devtty);
		goto again;
	}
	return f;
}

/*
 * Given a name like /usr/src/etc/foo.c returns the file system type
 * for the file system it lives in.
 */
char *
gettype(file)
	char *file;
{
	FILE *mntp;
	struct mntent *mnt;
	struct stat filestat, dirstat;
	char *mnttype;

	if (stat(file, &filestat) < 0) {
		fprintf(stderr, "find: can't stat %s: ", file);
		perror("");
		return(NULL);
	}

	if ((mntp = setmntent(MOUNTED, "r")) == 0) {
		fprintf(stderr, "find: can't open %s: ", MOUNTED);
		perror("");
		exit(1);
	}

	while ((mnt = getmntent(mntp)) != 0) {
		if (strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0 ||
		    strcmp(mnt->mnt_type, MNTTYPE_SWAP) == 0)
			continue;
		if (strcmp(mnt->mnt_fsname, file) == 0) {
			endmntent(mntp);
			return(mnt->mnt_type);
		}
		if (stat(mnt->mnt_dir, &dirstat) < 0) {
			fprintf("find: can't stat %s: ", mnt->mnt_dir);
			perror("");
			endmntent(mntp);
			return(NULL);
		}
		if (filestat.st_dev == dirstat.st_dev) {
			endmntent(mntp);
			if ((mnttype = (char *)malloc(strlen(mnt->mnt_type+1))) == NULL) {
				perror("find");
				exit(1);
			}
			strcpy(mnttype, mnt->mnt_type);
			return(mnttype);
		}
	}
	fprintf(stderr, "Couldn't find mount point for %s\n", file);
	exit(1);
	/*NOTREACHED*/
}

#ifdef	AMES
/*
 * 'fastfind' scans a file list for the full pathname of a file
 * given only a piece of the name.  The list has been processed with
 * with "front-compression" and bigram coding.  Front compression reduces
 * space by a factor of 4-5, bigram coding by a further 20-25%.
 * The codes are:
 *
 *	0-28	likeliest differential counts + offset to make nonnegative 
 *	30	escape code for out-of-range count to follow in next word
 *	128-255 bigram codes, (128 most common, as determined by 'updatedb')
 *	32-127  single character (printable) ascii residue
 *
 * A novel two-tiered string search technique is employed: 
 *
 * First, a metacharacter-free subpattern and partial pathname is
 * matched BACKWARDS to avoid full expansion of the pathname list.
 * The time savings is 40-50% over forward matching, which cannot efficiently
 * handle overlapped search patterns and compressed path residue.
 *
 * Then, the actual shell glob-style regular expression (if in this form)
 * is matched against the candidate pathnames using the slower routines
 * provided in the standard 'find'.
 */

#define	FCODES 	"/usr/lib/find/find.codes"
#define	YES	1
#define	NO	0
#define	OFFSET	14
#define	ESCCODE	30

fastfind ( pathpart )	
	char pathpart[];
{
	register char *p, *s;
	register int c; 
	char *q, *strchr(), *patprep();
	int i, count = 0, globflag;
	FILE *fp, *fopen();
	char *patend, *cutoff;
	char path[MAXPATHLEN];
	char bigram1[128], bigram2[128];
	int found = NO;

	if ( (fp = fopen ( FCODES, "r" )) == NULL ) {
		fprintf ( stderr, "find: " );
		perror ( FCODES );
		exit ( 1 );
	}
	for ( i = 0; i < 128; i++ ) 
		bigram1[i] = getc ( fp ),  bigram2[i] = getc ( fp );
	
	if ( strchr ( pathpart, '*' ) || strchr ( pathpart, '?' ) || strchr ( pathpart, '[' ) )
		globflag = YES;
	patend = patprep ( pathpart );

	c = getc ( fp );
	for ( ; ; ) {

		count += ( (c == ESCCODE) ? getw ( fp ) : c ) - OFFSET;

		for ( p = path + count; (c = getc ( fp )) > ESCCODE; )	/* overlay old path */
			if ( c < 0200 )	
				*p++ = c;
			else		/* bigrams are parity-marked */
				*p++ = bigram1[c & 0177],  *p++ = bigram2[c & 0177];
		if ( c == EOF )
			break;
		*p-- = NULL;
		cutoff = ( found ? path : path + count);

		for ( found = NO, s = p; s >= cutoff; s-- ) 
			if ( *s == *patend ) {		/* fast first char check */
				for ( p = patend - 1, q = s - 1; *p != NULL; p--, q-- )
					if ( *q != *p )
						break;
				if ( *p == NULL ) {	/* success on fast match */
					found = YES;
					if ( globflag == NO || amatch ( path, pathpart ) )
						puts ( path );
					break;
				}
			}
	}
}

/*
    extract last glob-free subpattern in name for fast pre-match;
    prepend '\0' for backwards match; return end of new pattern
*/
static char globfree[100];

char *
patprep ( name )
	char *name;
{
	register char *p, *endmark;
	register char *subp = globfree;

	*subp++ = '\0';
	p = name + strlen ( name ) - 1;
	/*
	   skip trailing metacharacters (and [] ranges)
	*/
	for ( ; p >= name; p-- )
		if ( strchr ( "*?", *p ) == 0 )
			break;
	if ( p < name )
		p = name;
	if ( *p == ']' )
		for ( p--; p >= name; p-- )
			if ( *p == '[' ) {
				p--;
				break;
			}
	if ( p < name )
		p = name;
	/*
	   if pattern has only metacharacters,
	   check every path (force '/' search)
	*/
	if ( (p == name) && strchr ( "?*[]", *p ) != 0 )
		*subp++ = '/';					
	else {				
		for ( endmark = p; p >= name; p-- )
			if ( strchr ( "]*?", *p ) != 0 )
				break;
		for ( ++p; (p <= endmark) && subp < (globfree + sizeof ( globfree )); )
			*subp++ = *p++;
	}
	*subp = '\0';
	return ( --subp );
}
#endif

/* rest should be done with nameserver or database */

#include <utmp.h>

struct	utmp utmp;
#define	NMAX	(sizeof (utmp.ut_name))
#define SCPYN(a, b)	strncpy(a, b, NMAX)

#define NUID	64
#define NGID	64

struct ncache {
	int	id;
	char	name[NMAX+1];
} nc[NUID], gc[NGID];

/*
 * This function assumes that the password file is hashed
 * (or some such) to allow fast access based on a name key.
 */
char *
getname(uid)
{
	register struct passwd *pw;
	struct passwd *getpwent();
	register int cp;

#if	(((NUID) & ((NUID) - 1)) != 0)
	cp = uid % (NUID);
#else
	cp = uid & ((NUID) - 1);
#endif
	if (uid >= 0 && nc[cp].id == uid && nc[cp].name[0])
		return (nc[cp].name);
	pw = getpwuid(uid);
	if (!pw)
		return (0);
	nc[cp].id = uid;
	SCPYN(nc[cp].name, pw->pw_name);
	return (nc[cp].name);
}

/*
 * This function assumes that the group file is hashed
 * (or some such) to allow fast access based on a name key.
 */
char *
getgroup(gid)
{
	register struct group *gr;
	struct group *getgrent();
	register int cp;

#if	(((NGID) & ((NGID) - 1)) != 0)
	cp = gid % (NGID);
#else
	cp = gid & ((NGID) - 1);
#endif
	if (gid >= 0 && gc[cp].id == gid && gc[cp].name[0])
		return (gc[cp].name);
	gr = getgrgid(gid);
	if (!gr)
		return (0);
	gc[cp].id = gid;
	SCPYN(gc[cp].name, gr->gr_name);
	return (gc[cp].name);
}

#define permoffset(who)		((who) * 3)
#define permission(who, type)	((type) >> permoffset(who))
#define kbytes(bytes)		(((bytes) + 1023) / 1024)

list(file, stp)
	char *file;
	register struct stat *stp;
{
	char pmode[32], uname[32], gname[32], fsize[32], ftime[32];
	char *getname(), *getgroup(), *ctime();
	static long special[] = { S_ISUID, 's', S_ISGID, 's', S_ISVTX, 't' };
	static time_t sixmonthsago = -1;
#ifdef	S_IFLNK
	char flink[MAXPATHLEN + 1];
#endif
	register int who;
	register char *cp;
	time_t now;

	if (file == NULL || stp == NULL)
		return (-1);

	time(&now);
	if (sixmonthsago == -1)
		sixmonthsago = now - 6L*30L*24L*60L*60L;

	switch (stp->st_mode & S_IFMT) {
#ifdef	S_IFDIR
	case S_IFDIR:	/* directory */
		pmode[0] = 'd';
		break;
#endif
#ifdef	S_IFCHR
	case S_IFCHR:	/* character special */
		pmode[0] = 'c';
		break;
#endif
#ifdef	S_IFBLK
	case S_IFBLK:	/* block special */
		pmode[0] = 'b';
		break;
#endif
#ifdef	S_IFIFO
	case S_IFIFO:	/* fifo special */
		pmode[0] = 'p';
		break;
#endif
#ifdef	S_IFLNK
	case S_IFLNK:	/* symbolic link */
		pmode[0] = 'l';
		break;
#endif
#ifdef	S_IFSOCK
	case S_IFSOCK:	/* socket */
		pmode[0] = 's';
		break;
#endif
#ifdef	S_IFREG
	case S_IFREG:	/* regular */
#endif
	default:
		pmode[0] = '-';
		break;
	}

	for (who = 0; who < 3; who++) {
		if (stp->st_mode & permission(who, S_IREAD))
			pmode[permoffset(who) + 1] = 'r';
		else
			pmode[permoffset(who) + 1] = '-';

		if (stp->st_mode & permission(who, S_IWRITE))
			pmode[permoffset(who) + 2] = 'w';
		else
			pmode[permoffset(who) + 2] = '-';

		if (stp->st_mode & special[who * 2])
			pmode[permoffset(who) + 3] = special[who * 2 + 1];
		else if (stp->st_mode & permission(who, S_IEXEC))
			pmode[permoffset(who) + 3] = 'x';
		else
			pmode[permoffset(who) + 3] = '-';
	}
	pmode[permoffset(who) + 1] = '\0';

	cp = getname(stp->st_uid);
	if (cp != NULL)
		sprintf(uname, "%-9.9s", cp);
	else
		sprintf(uname, "%-9d", stp->st_uid);

	cp = getgroup(stp->st_gid);
	if (cp != NULL)
		sprintf(gname, "%-9.9s", cp);
	else
		sprintf(gname, "%-9d", stp->st_gid);

	if (pmode[0] == 'b' || pmode[0] == 'c')
		sprintf(fsize, "%3d,%4d",
			major(stp->st_rdev), minor(stp->st_rdev));
	else {
		sprintf(fsize, "%8ld", stp->st_size);
#ifdef	S_IFLNK
		if (pmode[0] == 'l') {
			/*
			 * Need to get the tail of the file name, since we have
			 * already chdir()ed into the directory of the file
			 */
			cp = strrchr(file, '/');
			if (cp == NULL)
				cp = file;
			else
				cp++;
			who = readlink(cp, flink, sizeof flink - 1);
			if (who >= 0)
				flink[who] = '\0';
			else
				flink[0] = '\0';
		}
#endif
	}

	cp = ctime(&stp->st_mtime);
	if (stp->st_mtime < sixmonthsago || stp->st_mtime > now)
		sprintf(ftime, "%-7.7s %-4.4s", cp + 4, cp + 20);
	else
		sprintf(ftime, "%-12.12s", cp + 4);

	printf("%5lu %4ld %s %2d %s%s%s %s %s%s%s\n",
		stp->st_ino,				/* inode #	*/
#ifdef	S_IFSOCK
		(long) kbytes(dbtob(stp->st_blocks)),	/* kbytes       */
#else
		(long) kbytes(stp->st_size),		/* kbytes       */
#endif
		pmode,					/* protection	*/
		stp->st_nlink,				/* # of links	*/
		uname,					/* owner	*/
		gname,					/* group	*/
		fsize,					/* # of bytes	*/
		ftime,					/* modify time	*/
		file,					/* name		*/
#ifdef	S_IFLNK
		(pmode[0] == 'l') ? " -> " : "",
		(pmode[0] == 'l') ? flink  : ""		/* symlink	*/
#else
		"",
		""
#endif
	);

	return (0);
}
