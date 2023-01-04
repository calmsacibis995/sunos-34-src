#ifndef lint
static	char sccsid[] = "@(#)mon.c 1.1 86/09/24 SMI"; /* from UCB 4.8 83/04/20 */
#endif

#define ARCDENSITY	5	/* density of routines */
#define MINARCS		50	/* minimum number of counters */
#define	HISTFRACTION	2	/* fraction of text space for histograms */


struct phdr {
	int *lpc;
	int *hpc;
	int ncnt;
};

struct cnt {
	int *pc;
	long ncall;
} *countbase;

static int cntrs = 0;
static int profiling = 3;
static char *s_sbuf;
static int s_bufsiz;
static int s_scale;
static char *s_lowpc;

int numctrs;

static char		*minsbrk = 0;

#define	MSG "No space for monitor buffer(s)\n"

monstartup(lowpc, highpc)
	char *lowpc;
	char *highpc;
{
	int monsize;
	char *buffer;
	int cntsiz;
	char *sbrk();

	cntsiz = (highpc - lowpc) * ARCDENSITY / 100;
	if (cntsiz < MINARCS)
		cntsiz = MINARCS;
	monsize = (highpc - lowpc + HISTFRACTION - 1) / HISTFRACTION
		+ sizeof(struct phdr) + cntsiz * sizeof(struct cnt);
	monsize = (monsize + 1) & ~1;
	buffer = sbrk(monsize);
	if (buffer == (char *)-1) {
		write(2, MSG, sizeof(MSG));
		return;
	}
	minsbrk = sbrk(0);
	monitor(lowpc, highpc, buffer, monsize, cntsiz);
}

/*
 *	This routine is massaged so that it may be jsb'ed to
 */
asm(".text");
asm("#the beginning of mcount()");
asm(".data");
mcount()
{
	register int *selfpc;	/* r11 */
	register long **cntp;   /* r10 */

#ifdef lint
	selfpc = (int *) 0;
#else not lint
	/*
	 * find the return address for mcount,
	 * and address of counter pointer
	 */
	asm("	movl	(sp),r11");	/* selfpc = ... (jsb frame) */
	asm("	movl	r0,r10");	/* address of count local */
#endif not lint
	/*
	 * check that we aren't recursively invoked.
	 */
	if (profiling)
		goto out;
	profiling++;
	/*
	 * check that counter is allocated
	 */
	if (*cntp == 0) {
		/*
		 * check that a counter is available
		 */
		if (cntrs++ == numctrs)
			goto overflow;
		countbase->pc = selfpc;
		*cntp = &countbase->ncall;
		countbase++;
	}
	(**cntp)++;
	profiling--;
out:
	asm( "	rsb" );

overflow:
#   define	TOLIMIT	"mcount: counter overflow\n"
	write( 2 , TOLIMIT , sizeof( TOLIMIT ) );
	goto out;
}
asm(".text");
asm("#the end of mcount()");
asm(".data");

monitor(lowpc, highpc, buf, bufsiz, cntsiz)
	char *lowpc, *highpc;
	char *buf;
	int bufsiz, cntsiz;
{
	register int o;
	struct phdr *php;
	static int ssiz;
	static char *sbuf;

	if (lowpc == 0) {
		moncontrol(0);
		o = creat("mon.out", 0666);
		write(o, sbuf, ssiz);
		close(o);
		return;
	}
	sbuf = buf;
	ssiz = bufsiz;
	php = (struct phdr *)&buf[0];
	php->lpc = (int *)lowpc;
	php->hpc = (int *)highpc;
	php->ncnt = cntsiz;
	numctrs = cntsiz;
	countbase = (struct cnt *)(buf + sizeof(struct phdr));
	o = sizeof(struct phdr) + cntsiz * sizeof(struct cnt);
	buf += o;
	bufsiz -= o;
	if (bufsiz <= 0)
		return;
	o = (highpc - lowpc);
	if(bufsiz < o)
		o = ((float) bufsiz / o) * 65536;
	else
		o = 65536;
	s_scale = o;
	s_sbuf = buf;
	s_bufsiz = bufsiz;
	s_lowpc = lowpc;
	moncontrol(1);
}

/*
 * Control profiling
 *	profiling is what mcount checks to see if
 *	all the data structures are ready.
 */
moncontrol(mode)
    int mode;
{
    if (mode) {
	/* start */
	profil(s_sbuf, s_bufsiz, s_lowpc, s_scale);
	profiling = 0;
    } else {
	/* stop */
	profil((char *)0, 0, 0, 0);
	profiling = 3;
    }
}

/*
 * This is a stub for the "brk" system call, which we want to
 * catch so that it will not deallocate our data space.
 * (of which the program is not aware)
 */
extern char *curbrk;

brk(addr)
	char *addr;
{

	if (addr < minsbrk)
		addr = minsbrk;
	asm("	chmk	$17");
	asm("	jcc	1f");
	asm("	jmp	cerror");
asm("1:");
	curbrk = addr;
	return (0);
}
