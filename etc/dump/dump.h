/*	@(#)dump.h 1.1 86/09/24 SMI; from UCB 1.12 83/07/01	*/

#define	NI		16
#define MAXINOPB	(MAXBSIZE / sizeof(struct dinode))
#define MAXNINDIR	(MAXBSIZE / sizeof(daddr_t))

#include <stdio.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <ufs/inode.h>
#include <ufs/fs.h>
#include <dumprestor.h>
#include <sys/dir.h>
#include <utmp.h>
#include <signal.h>
#include <fstab.h>

#define	MWORD(m,i)	(m[(unsigned)(i-1)/NBBY])
#define	MBIT(i)		(1<<((unsigned)(i-1)%NBBY))
#define	BIS(i,w)	(MWORD(w,i) |=  MBIT(i))
#define	BIC(i,w)	(MWORD(w,i) &= ~MBIT(i))
#define	BIT(i,w)	(MWORD(w,i) & MBIT(i))

int	msiz;
char	*clrmap;
char	*dirmap;
char	*nodmap;

/*
 *	All calculations done in 0.1" units!
 */

char	*disk;		/* name of the disk file */
char	*tape;		/* name of the tape file */
char	*increm;	/* name of the file containing incremental information*/
char	*temp;		/* name of the file for doing rewrite of increm */
char	lastincno;	/* increment number of previous dump */
char	incno;		/* increment number */
int	uflag;		/* update flag */
int	fi;		/* disk file descriptor */
int	to;		/* tape file descriptor */
int	pipeout;	/* true => output to standard output */
ino_t	ino;		/* current inumber; used globally */
int	nsubdir;
int	newtape;	/* new tape flag */
int	nadded;		/* number of added sub directories */
int	dadded;		/* directory added flag */
int	density;	/* density in 0.1" units */
long	tsize;		/* tape size in 0.1" units */
long	esize;		/* estimated tape size, blocks */
long	asize;		/* number of 0.1" units written on current tape */
int	etapes;		/* estimated number of tapes */

int	notify;		/* notify operator flag */
int	blockswritten;	/* number of blocks written on current tape */
int	tapeno;		/* current tape number */
time_t	tstart_writing;	/* when started writing the first tape block */
char	*processname;
struct fs *sblock;	/* the file system super block */
char	buf[MAXBSIZE];

char	*ctime();
char	*prdate();
long	atol();
int	mark();
int	add();
int	dump();
int	tapsrec();
int	dmpspc();
int	dsrch();
int	nullf();
char	*getsuffix();
char	*rawname();
struct dinode *getino();

int	interrupt();		/* in case operator bangs on console */

#define	HOUR	(60L*60L)
#define	DAY	(24L*HOUR)
#define	YEAR	(365L*DAY)

/*
 *	Exit status codes
 */
#define	X_FINOK		1	/* normal exit */
#define	X_REWRITE	2	/* restart writing from the check point */
#define	X_ABORT		3	/* abort all of dump; don't attempt checkpointing*/

#define	NINCREM	"/etc/dumpdates"	/*new format incremental info*/
#define	TEMP	"/etc/dtmp"		/*output temp file*/

#define	TAPE	"/dev/rmt8"		/* default tape device */
#define	RTAPE	"dumphost:/dev/rmt8"	/* default tape device */
#define	OPGRENT	"operator"		/* group entry to notify */
#define DIALUP	"ttyd"			/* prefix for dialups */

struct	fstab	*fstabsearch();	/* search in fs_file and fs_spec */

/*
 *	The contents of the file NINCREM is maintained both on
 *	a linked list, and then (eventually) arrayified.
 */
struct	idates {
	char	id_name[MAXNAMLEN+3];
	char	id_incno;
	time_t	id_ddate;
};
struct	itime{
	struct	idates	it_value;
	struct	itime	*it_next;
};
struct	itime	*ithead;	/* head of the list version */
int	nidates;		/* number of records (might be zero) */
int	idates_in;		/* we have read the increment file */
struct	idates	**idatev;	/* the arrayfied version */
#define	ITITERATE(i, ip) for (i = 0,ip = idatev[0]; i < nidates; i++, ip = idatev[i])

/*
 *	We catch these interrupts
 */
int	sighup();
int	sigquit();
int	sigill();
int	sigtrap();
int	sigfpe();
int	sigkill();
int	sigbus();
int	sigsegv();
int	sigsys();
int	sigalrm();
int	sigterm();

struct tapfunc {
	int	(*tf_alloctape)();
	int	(*tf_taprec)();
	int	(*tf_dmpblk)();
	int	(*tf_flusht)();
	int	(*tf_rewind)();
	int	(*tf_close_rewind)();
	int	(*tf_otape)();
} *tf, localtape, remotetape;
#define	alloctape()		(*tf->tf_alloctape)()
#define	taprec(dp)		(*tf->tf_taprec)(dp)
#define	dmpblk(blkno,size)	(*tf->tf_dmpblk)(blkno,size)
#define	flusht()		(*tf->tf_flusht)()
#define	rewind()		(*tf->tf_rewind)()
#define	close_rewind()		(*tf->tf_close_rewind)()
#define	otape()			(*tf->tf_otape)()
