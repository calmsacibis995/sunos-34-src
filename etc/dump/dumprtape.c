#ifndef lint
static	char sccsid[] = "@(#)dumprtape.c 1.1 86/09/24 SMI"; /* from UCB 1.5 83/08/11 */
#endif

#include "dump.h"

int	r_alloctape();
int	r_taprec();
int	r_dmpblk();
int	r_flusht();
int	r_rewind();
int	r_close_rewind();
int	r_otape();
struct	tapfunc remotetape =
   { r_alloctape, r_taprec, r_dmpblk, r_flusht,
     r_rewind, r_close_rewind, r_otape } ;

/*
 * tape buffering routines double buffer for remote dump.
 * tblock[1-rotor] is written to remote in tape order
 * as tblock[rotor] is filled in in seek order.
 */

struct	atblock {
	char	tblock[TP_BSIZE];
};
struct	atblock *tblock[2]; 	/* Pointers to malloc()ed buffers for tape */
int	writesize;		/* Size of single malloc()ed buffer for tape */
int	trotor;
daddr_t *tdaddr;		/* Pointer to array of disk addrs */
int	toldindex, tcurindex, trecno;
extern	int ntrec;		/* blocking factor on tape */

/*
 * Allocate the buffer for tape operations.
 *
 * Depends on global variable ntrec, set from 'b' option in command line.
 * Returns 1 if successful, 0 if failed.
 *
 * For later kernel performance improvement, this buffer should be allocated
 * on a page boundary.
 */
r_alloctape()
{

	writesize = ntrec * TP_BSIZE;
	tblock[0] = (struct atblock *)malloc(2 * writesize);
	if (tblock[0] == 0)
		return (0);
	tblock[1] = tblock[0]+ntrec;	/* Point to second bigbuffer */
	tdaddr = (daddr_t *)malloc(ntrec * sizeof(daddr_t));
	return (tdaddr != NULL);
}

r_taprec(dp)
	char *dp;
{
	register i;
	register struct atblock *bp = tblock[tcurindex];

	r_tadvance();
	tdaddr[tcurindex] = 0;
	*(&tblock[trotor][tcurindex++]) = *(struct atblock *)dp;
	spcl.c_tapea++;
	if (tcurindex >= ntrec)
		flusht();
}

r_dmpblk(blkno, size)
	daddr_t blkno;
	int size;
{
	int tpblks, dblkno;

	if (size % TP_BSIZE != 0)
		msg("bad size to dmpblk: %d\n", size);
	dblkno = fsbtodb(sblock, blkno);
	for (tpblks = size / TP_BSIZE; tpblks > 0; tpblks--) {
		r_tapsrec(dblkno);
		dblkno += TP_BSIZE / DEV_BSIZE;
	}
}

int	nogripe;

r_tadvance()
{

	if (trecno == 0)
		return;
	if (toldindex == 0)
		rmtwrite0(TP_BSIZE * ntrec);
	rmtwrite1((char *)(&tblock[1 - trotor][toldindex++]), TP_BSIZE);
	if (toldindex != ntrec)
		return;
	toldindex = 0;
	if (rmtwrite2() != writesize) {
		msg("Write error on tape %d\n", tapeno);
		broadcast("TAPE ERROR!\n");
		if (query("Restart this tape?") == 0)
			dumpabort();
		msg("After this tape rewinds, replace the reel\n");
		msg("and the dump volume will be rewritten.\n");
		close_rewind();
		exit(X_REWRITE);
	}
}

r_close_rewind()
{

	rewind();
	tnexttape();
}

r_tapsrec(d)
	daddr_t d;
{

	if (d == 0)
		return;
	tdaddr[tcurindex] = d;
	tcurindex++;
	spcl.c_tapea++;
	if (tcurindex >= ntrec)
		flusht();
}

r_flusht()
{
	register i, si;
	daddr_t d;
	extern int tenthsperirg;	/* from dumpmain.c */

	while (tcurindex < ntrec)
		tdaddr[tcurindex++] = 1;
loop:
	d = 0;
	for (i=0; i<ntrec; i++)
		if (tdaddr[i] != 0)
			if (d == 0 || tdaddr[i] < d) {
				si = i;
				d = tdaddr[i];
			}
	if (d != 0) {
		r_tadvance();
		bread(d, (char *)&tblock[trotor][si], TP_BSIZE);
		tdaddr[si] = 0;
		goto loop;
	}
	tcurindex = 0;
	trecno++;
	trotor = 1 - trotor;
	asize += writesize/density;
	asize += tenthsperirg;
	blockswritten += ntrec;
	if (asize > tsize) {
		tflush(0);
		rewind();
		msg("Change Tapes: Mount tape #%d\n", tapeno+1);
		broadcast("CHANGE TAPES!\7\7\n");
		tnexttape();
		otape();
		/* returns in child */
	}
	timeest();
}

tflush(eof)
	int eof;
{
	int i;

	if (eof) {
		do {
			spclrec();
		} while (tcurindex);
	}
	for (i = 0; i < ntrec; i++)
		r_tadvance();
}

tnexttape()
{

again:
	if (query("Next tape ready?") == 1)
		return;
	if (query("Want to abort?") == 1)
		dumpabort();
	goto again;
}

r_rewind()
{

	msg("Tape rewinding\n");
	rmtclose();
	while (rmtopen(tape, 0, 0) < 0)
		sleep(10);
	rmtclose();
}

r_otape()
{
	int ppid, child, status;
	int w, interrupt();

	rmtclose();
	ppid = getpid();
again:
	signal(SIGINT, interrupt);
	child = fork();
	if (child < 0) {
		msg("Context save fork fails in parent %d\n", ppid);
		exit(X_ABORT);
	}
	if (child != 0) {
		signal(SIGINT, SIG_IGN);
		for (;;) {
			w = wait(&status);
			if (w == child)
				break;
msg("Parent %d waiting for %d has another child %d return\n", ppid, child, w);
		}
		if (status & 0xff)
msg("Child %d returns LOB status %o\n", child, status & 0xff);
		switch ((status >> 8) & 0xff) {

		case X_FINOK:
			exit(X_FINOK);

		case X_ABORT:
			exit(X_ABORT);

		case X_REWRITE:
			rmtclose();
#ifdef notdef
			do {
				if (!query("Retry conection to remote host?"))
					exit(X_ABORT);
				rmtgetconn();
			} while (rmtape < 0);
#endif
			goto again;

		default:
			msg("Bad return code from dump: %d\n", status);
			exit(X_ABORT);
		}
		/*NOTREACHED*/
	}
	for (;;) {
		if (rmtopen(tape, 2) >= 0)
			break;
		if (query("Tape open failed, try again?") == 0)
			dumpabort();
	}
	asize = 0;
	tapeno++, newtape++;
	trecno = 0;
	spcl.c_volume++;
	spcl.c_type = TS_TAPE;
	spclrec();
	if (tapeno > 1)
		msg("Tape %d begins with blocks from ino %d\n", tapeno, ino);
}
