/*	@(#)param.c 1.1 86/09/25 SMI; from UCB 6.1 83/07/29	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/socket.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/text.h"
#include "../h/vnode.h"
#include "../h/file.h"
#include "../h/callout.h"
#include "../h/clist.h"
#include "../h/cmap.h"
#include "../h/mbuf.h"
#include "../h/stream.h"
#include "../h/var.h"
#include "../h/kernel.h"
#include <ufs/inode.h>
#include <ufs/quota.h>
#include <specfs/fifo.h>

#include "../h/map.h"
#include "../h/ipc.h"
#include "../h/sem.h"
#include "../h/msg.h"
#include "../h/shm.h"

/*
 * System parameter formulae.
 *
 * This file is copied into each directory where we compile
 * the kernel; it should be modified there to suit local taste
 * if necessary.
 *
 * Compiled with -DTIMEZONE=x -DDST=x -DMAXUSERS=xx
 */

#define	HZ 50
int	hz = HZ;
int	tick = 1000000 / HZ;
int	tickadj = 1000000 / HZ / 10;
struct	timezone tz = { TIMEZONE, DST };
#ifdef sun
#define	NPROC (10 + 16 * MAXUSERS)
#undef MAXUPRC
#define	MAXUPRC	(NPROC - 5)
#else
#define	NPROC (10 + 8 * MAXUSERS)
#endif
int	nproc = NPROC;
int	maxuprc = MAXUPRC;
int	ntext = 24 + MAXUSERS;
int	ninode = (NPROC + 16 + MAXUSERS) + 64;
int	ncsize = (NPROC + 16 + MAXUSERS) + 64;
#include "win.h"
#if NWIN > 0
/* if using the window system, will need more open files */
int	nfile = 16 * (NPROC + 16 + MAXUSERS) / 5 + 64;
#else
int	nfile = 16 * (NPROC + 16 + MAXUSERS) / 10 + 64;
#endif
int	ncallout = 16 + NPROC;
int	nclist = 100 + 16 * MAXUSERS;
#ifdef QUOTA
int	ndquot = (MAXUSERS*NMOUNT)/4 + NPROC;
#else
int	ndquot = 0;
#endif

/*
 * These are initialized at bootstrap time
 * to values dependent on memory size
 */
int	nbuf, nswbuf;

/*
 * These have to be allocated somewhere; allocating
 * them here forces loader errors if this file is omitted.
 */
struct	proc *proc, *procNPROC;
struct	text *text, *textNTEXT;
struct	inode *inode, *inodeNINODE;
struct	file *file, *fileNFILE;
struct 	callout *callout;
struct	cblock *cfree;
struct	buf *buf, *swbuf;
char	*buffers;
struct	cmap *cmap, *ecmap;
#ifdef QUOTA
struct	dquot *dquot, *dquotNDQUOT;
#endif

/* initialize SystemV named-pipe (and pipe()) information structure */
struct fifoinfo fifoinfo = {
	FIFOBUF,
	FIFOMAX,
	FIFOBSZ,
	FIFOMNB
};

/* initialize SystemV IPC information structures */
struct msginfo msginfo = {
#ifdef IPCMESSAGE
	MSGMAP,
	MSGMAX,
	MSGMNB,
	MSGMNI,
	MSGSSZ,
	MSGTQL,
	MSGSEG
#else
	0,0,0,0,0,0,0
#endif IPCMESSAGE
};

struct seminfo seminfo = {
#ifdef IPCSEMAPHORE
	SEMMAP,
	SEMMNI,
	SEMMNS,
	SEMMNU,
	SEMMSL,
	SEMOPM,
	SEMUME,
	SEMUSZ,
	SEMVMX,
	SEMAEM
#else
	0,0,0,0,0,0,0,0,0,0
#endif IPCSEMAPHORE
};

struct shminfo shminfo = {
#ifdef IPCSHMEM
	SHMMAX,
	SHMMIN,
	SHMMNI,
	SHMSEG,
	SHMALL
#else
	0,0,0,0,0
#endif IPCSHMEM
};

#ifdef STREAMS
/*
 * Stream data structures.
 * XXX - should be dynamically allocated.
 */
#define	NBLK4096	1
#define	NBLK2048	32
#define	NBLK1024	8
#define	NBLK512		12
#define	NBLK256		32
#define	NBLK128		64
#define	NBLK64		128
#define	NBLK16		128
#define	NBLK4		128
#define	NBLK (NBLK4096 + NBLK2048 + NBLK1024 + NBLK512 + NBLK256 + NBLK128 + \
	NBLK64 + NBLK16 + NBLK4)
#define	NQUEUE		196
#define	NSTREAM		32
#define	NMUXLINK	87
#define	NSTRPUSH	9
#define	NSTREVENT	256
#define	MAXSEPGCNT	1
#define	STRLOFRAC	80
#define	STRMEDFRAC	90
#define	STRMSGSZ	4096
#define	STRCTLSZ	1024

struct	stdata streams[NSTREAM];
struct	stdata *streamsNSTREAMS = &streams[NSTREAM];
queue_t	queue[NQUEUE];
mblk_t	mblock[NBLK];
dblk_t	dblock[NBLK];
struct	linkblk linkblk[NMUXLINK];
struct	strevent strevent[NSTREVENT];
int	strmsgsz = STRMSGSZ;
int	strctlsz = STRCTLSZ;
int	nmblock = NBLK;
int	nmuxlink = NMUXLINK;
int	nstrpush = NSTRPUSH;
int	nstrevent = NSTREVENT;
int	maxsepgcnt = MAXSEPGCNT;
char	strlofrac = STRLOFRAC;
char	strmedfrac = STRMEDFRAC;
struct  var v = {
	NQUEUE, NSTREAM, NBLK4096, NBLK2048, NBLK1024, NBLK512, NBLK256,
	NBLK128, NBLK64, NBLK16, NBLK4
};
#endif
