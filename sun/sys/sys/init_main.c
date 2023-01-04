/*	@(#)init_main.c 1.1 86/09/25 SMI; from UCB 4.53 83/07/01	*/

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/vfs.h"
#include "../h/map.h"
#include "../h/proc.h"
#include "../h/seg.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/cmap.h"
#include "../h/text.h"
#include "../h/clist.h"
#include "../h/vnode.h"
#ifdef QUOTA
#include "../ufs/quota.h"
#endif
#include "../h/systm.h"
#include "../machine/reg.h"

int	cmask = CMASK;
/*
 * Initialization code.
 * Called from cold start routine as
 * soon as a stack and segmentation
 * have been established.
 * Functions:
 *	clear and free user core
 *	turn on clock
 *	hand craft 0th process
 *	call all initialization routines
 *	fork - process 0 to schedule
 *	     - process 1 execute bootstrap
 *	     - process 2 to page out
 */
#ifdef vax
main(firstaddr)
	int firstaddr;
#endif
#ifdef sun
main(regs)
	struct regs regs;
#endif
{
	register int i;
	register struct proc *p;
	int s;

	rqinit();
#include "loop.h"
#ifdef vax
	startup(firstaddr);
#endif
#ifdef sun
	startup();
#endif

	/*
	 * set up system process 0 (swapper)
	 */
	p = &proc[0];
	p->p_p0br = u.u_pcb.pcb_p0br;
	p->p_p1br = u.u_pcb.pcb_p1br;
	p->p_szpt = 1;
	p->p_addr = uaddr(p);
	p->p_stat = SRUN;
	p->p_flag |= SLOAD|SSYS;
	p->p_nice = NZERO;
	setredzone(p->p_addr, (caddr_t)&u);
	u.u_procp = p;
#ifdef sun
	u.u_ar0 = &regs.r_r0;
#endif
	u.u_cmask = cmask;
	for (i = 0; i < sizeof(u.u_rlimit)/sizeof(u.u_rlimit[0]); i++)
		u.u_rlimit[i].rlim_cur = u.u_rlimit[i].rlim_max = 
		    RLIM_INFINITY;
	u.u_rlimit[RLIMIT_STACK].rlim_cur = 512*1024;
	u.u_rlimit[RLIMIT_STACK].rlim_max = ctob(MAXDSIZ);
	u.u_rlimit[RLIMIT_DATA].rlim_max =
	    u.u_rlimit[RLIMIT_DATA].rlim_cur = ctob(MAXDSIZ);
	p->p_maxrss = RLIM_INFINITY/NBPG;

	/*
	 * get vnodes for swapdev and argdev
	 */
	swapdev_vp = bdevvp(swapdev);
	argdev_vp = bdevvp(argdev);
	rootvp = bdevvp(rootdev);

	/*
	 * Setup credentials
	 */
	u.u_cred = crget();
	for (i = 1; i < NGROUPS; i++)
		u.u_groups[i] = NOGROUP;

#ifdef IPCMESSAGE
	msginit();		/* init SysV IPC message facility */
#endif
 
#ifdef IPCSEMAPHORE
	seminit();		/* init SysV IPC semaphore facility */
#endif

#ifdef QUOTA
	qtinit();
#endif
	startrtclock();
#ifdef vax
#include "kg.h"
#if NKG > 0
	startkgclock();
#endif
#endif

	/*
	 * Initialize tables, protocols
	 */
	mbinit();
	cinit();			/* needed by dmc-11 driver */
#ifdef INET
#if NLOOP > 0
	loattach();			/* XXX */
#endif
	/*
	 * Block reception of incoming packets
	 * until protocols have been initialized.
	 */
	s = splimp();
	ifinit();
#endif
	domaininit();
#ifdef INET
	(void) splx(s);
#endif
	ihinit();
	bhinit();
	dnlc_init();
	binit();
	bswinit();
#ifdef STREAMS
	strinit();
#endif
#ifdef GPROF
	kmstartup();
#endif

#ifdef sun
	consconfig();
#endif

/* kick off timeout driven events by calling first time */
	roundrobin();
	schedcpu();
	/*
	 * mount the root, gets rootdir
	 */
	vfs_mountroot();
	boottime = time;

	schedpaging();

	u.u_dmap = zdmap;
	u.u_smap = zdmap;

	/*
	 * Set the scan rate and other parameters of the paging subsystem.
	 */
	setupclock();

	/*
	 * make page-out daemon (process 2)
	 * the daemon has ctopt(nswbuf*CLSIZE*KLMAX) pages of page
	 * table so that it can map dirty pages into
	 * its address space during asychronous pushes.
	 */
	mpid = 1;
	proc[0].p_szpt = clrnd(ctopt(nswbuf*CLSIZE*KLMAX + UPAGES));
	proc[1].p_stat = SZOMB;		/* force it to be in proc slot 2 */
	if (newproc(0)) {
		proc[2].p_flag |= SLOAD|SSYS;
		proc[2].p_dsize = u.u_dsize = nswbuf*CLSIZE*KLMAX; 
		pageout();
		/*NOTREACHED*/
	}

	/*
	 * make init process and
	 * enter scheduling loop
	 */

	mpid = 0;
	proc[1].p_stat = 0;
	proc[0].p_szpt = CLSIZE;
	if (newproc(0)) {
#ifdef vax
		expand(clrnd((int)btoc(szicode)), 0);
		(void) swpexpand(u.u_dsize, 0, &u.u_dmap, &u.u_smap);
		(void) copyout((caddr_t)icode, (caddr_t)0, (unsigned)szicode);
#endif
#ifdef sun
		icode();
#endif
		/*
		 * Return goes to start up of user
		 * init code just copied out.
		 */
		return (0);
	}
	proc[0].p_szpt = 1;
	sched();
	/*NOTREACHED*/
}

/*
 * Initialize hash links for buffers.
 */
bhinit()
{
	register int i;
	register struct bufhd *bp;

	for (bp = bufhash, i = 0; i < BUFHSZ; i++, bp++)
		bp->b_forw = bp->b_back = (struct buf *)bp;
}

/*
 * Initialize the buffer I/O system by freeing
 * all buffers and setting all device buffer lists to empty.
 */
binit()
{
	register struct buf *bp, *dp;
	register int i;
	struct swdevt *swp;
	int base, residual;

	for (dp = bfreelist; dp < &bfreelist[BQUEUES]; dp++) {
		dp->b_forw = dp->b_back = dp->av_forw = dp->av_back = dp;
		dp->b_flags = B_HEAD;
	}
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
		bp = &buf[i];
		bp->b_dev = NODEV;
		bp->b_bcount = 0;
		bp->b_un.b_addr = buffers + i * MAXBSIZE;
		if (i < residual)
			bp->b_bufsize = (base + 1) * CLBYTES;
		else
			bp->b_bufsize = base * CLBYTES;
		binshash(bp, &bfreelist[BQ_AGE]);
		bp->b_flags = B_BUSY|B_INVAL;
		brelse(bp);
	}
	/*
	 * Count swap devices, and adjust total swap space available.
	 * Some of this space will not be available until a vswapon()
	 * system call is issued, usually when the system goes multi-user.
	 */
	swapconf();
	nswdev = 0;
	nswap = 0;
	for (swp = swdevt; swp->sw_dev; swp++) {
		nswdev++;
		if (swp->sw_nblks > nswap)
			nswap = swp->sw_nblks;
	}
	if (nswdev == 0)
		panic("binit");
	if (nswdev > 1)
		nswap = ((nswap + dmmax - 1) / dmmax) * dmmax;
	nswap *= nswdev;
	maxpgio *= nswdev;
	swfree(0);
}

/*
 * Initialize linked list of free swap
 * headers. These do not actually point
 * to buffers, but rather to pages that
 * are being swapped in and out.
 */
bswinit()
{
	register int i;
	register struct buf *sp = swbuf;

	bswlist.av_forw = sp;
	for (i=0; i<nswbuf-1; i++, sp++)
		sp->av_forw = sp+1;
	sp->av_forw = NULL;
}

/*
 * Initialize clist by freeing all character blocks, then count
 * number of character devices. (Once-only routine)
 */
cinit()
{
	register int ccp;
	register struct cblock *cp;

	ccp = (int)cfree;
	ccp = (ccp+CROUND) & ~CROUND;
	for(cp=(struct cblock *)ccp; cp < &cfree[nclist-1]; cp++) {
		cp->c_next = cfreelist;
		cfreelist = cp;
		cfreecount += CBSIZE;
	}
}
