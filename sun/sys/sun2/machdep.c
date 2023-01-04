#ifndef lint
static	char sccsid[] = "@(#)machdep.c 1.1 86/09/25";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/reboot.h"
#include "../h/conf.h"
#include "../h/vnode.h"
#include "../h/file.h"
#include "../h/text.h"
#include "../h/clist.h"
#include "../h/callout.h"
#include "../h/cmap.h"
#include "../h/mbuf.h"
#include "../h/msgbuf.h"
#include "../h/socket.h"
#include "../h/kernel.h"
#include "../h/dnlc.h"
#include "../debug/debug.h"

#if defined(IPCSEMAPHORE) || defined(IPCMESSAGE) || defined(IPCSHMEM)
#include "../h/ipc.h"
#endif IPCSEMAPHORE || IPCMESSAGE || IPCSHMEM

#ifdef IPCSEMAPHORE
#include "../h/sem.h"
#endif IPCSEMAPHORE

#ifdef IPCMESSAGE
#include "../h/msg.h"
#endif IPCMESSAGE

#ifdef IPCSHMEM
#include "../h/shm.h"
#endif IPCSHMEM

#include "../net/if.h"
#include "../ufs/inode.h"
#ifdef QUOTA
#include "../ufs/quota.h"
#endif QUOTA
#include "../sun/consdev.h"
#include "../sun/frame.h"
#include "../sundev/mbvar.h"

#include "../machine/psl.h"
#include "../machine/reg.h"
#include "../machine/clock.h"
#include "../machine/pte.h"
#include "../machine/scb.h"
#include "../machine/mmu.h"
#include "../machine/cpu.h"

/*
 * Declare these as initialized data so we can patch them.
 */
int nbuf = 0;
int nswbuf = 0;
int bufpages = 0;
int physmem = 0;	/* memory size in pages, patch if you want less */
int kernprot = 1;	/* write protect kernel text */
int msgbufinit = 0;	/* message buffer has been initialized, ok to printf */
int nopanicdebug = 0;

int (*exit_vector)() = (int (*)())0;	/* Where to go when halting UNIX */

#define TESTVAL	0xA55A	/* memory test value */

u_char	getsegmap(), pmegallocres();
long	getpgmap();


/*
 * We make use of CMAPn (the pte address)
 * and CADDRn (the virtual address)
 * which are both temporaries defined in locore.s,
 * not preserved across context switches,
 * and not to be used in interrupt routines
 */

/*
 * Machine-dependent startup code
 */
startup()
{
	register int unixsize, dvmapage;
	register unsigned i;
	register struct pte *pte;
	register caddr_t v;
	extern char redzone[];
	u_int firstaddr;	/* first available real memory address */
	extern char start[], etext[], end[], CADDR1[], Syslimit[];
	u_int mapaddr;
	caddr_t zmemall();
	u_char pm;
	int range;

	bootflags();			/* get the boot options */

	firstaddr = btoc((int)end - KERNELBASE);
	initscb();

	/* 
	 * Initialize map of allocated page map groups.
 	 * Must be done before mapin of unallocated segments.
	 */
	pmeginit();
	ctxinit();

	/* reserve kernel pmegs */
	for (i = 0; i <= ptos(NPAGSEG - 1 + btoc(end)); i++)
		pmegreserve(getsegmap(i));

	for (; i < (DEBUGSTART >> SGSHIFT); i++)/* invalidate to debug start */
		setsegmap(i, (u_char)SEGINV);

	for (; i < (DEBUGEND >> SGSHIFT); i++) {
		if ((boothowto & RB_DEBUG) == 0)
			setsegmap(i, (u_char)SEGINV);
		else if ((pm = getsegmap(i)) != (u_char)SEGINV)
			pmegreserve(pm);	/* reserve debugger pmeg */
	}

	for (; i < (MONSTART >> SGSHIFT); i++)	/* invalidate to mon start */
		setsegmap(i, (u_char)SEGINV);

	for (; i < (MONEND >> SGSHIFT); i++) { 	/* reserve monitor pmegs */
		if ((pm = getsegmap(i)) != (u_char)SEGINV) {
			for (v = (caddr_t)ctob(NPAGSEG * i); 
			    v < (caddr_t)ctob(NPAGSEG * (i+1)); v += NBPG) {
				long pg = getpgmap(v);
				
				if ((pg & PG_V) && (pg & PG_PROT) >= PG_KR)
					break;
			}
			if (v < (caddr_t)ctob(NPAGSEG * (i+1))) {
				/*
				 * The pmeg had a valid page with
				 * some access allowed, reserve
				 * the pmeg for the monitor.
				 */
				pmegreserve(pm);
			} else {
				/*
				 * "steal" wasted pmeg back from the monitor
				 */
				setsegmap(i, (u_char)SEGINV);
			}
		}
	}

	for (; i < NSEGMAP; i++) {		/* invalidate rest of segs */
		/*
		 * kludge for bwone frame buffers which are mapped
		 * in here instead of in the [MONSTART..MONEND] range
		 */
		if ((i << SGSHIFT) >= *romp->v_fbaddr &&
		    (i << SGSHIFT) < *romp->v_fbaddr + FBSIZE) {
			if ((pm = getsegmap(i)) != (u_char)SEGINV)	
				pmegreserve(pm);
		} else
			setsegmap(i, (u_char)SEGINV);
	}

	pmegreserve((u_char)SEGINV);		/* reserve inval pmeg itself */

	/*
	 * Allocate pmegs for DVMA space
	 */
	setcputype();			/* sets cpu and dvmasize variables */
	for (i = ptos(btop(DVMA)); i < ptos(btop(DVMA) + dvmasize); i++) {
		pm = pmegallocres();
		setsegmap(i, pm);
		for (v = (caddr_t)ctob(NPAGSEG * i);
		    v < (caddr_t)ctob(NPAGSEG * (i+1)); v += NBPG)
			setpgmap(v, (long)0);
	}

	/*
	 * Initialize kernel page table entries.
	 */
	pte = &Sysmap[0];
	/* Low memory (below start) is writeable, except u area redzone */
	for (v = KERNELBASE, i = 0; v < (caddr_t)start; v += NBPG, i++) {
		if (v == (caddr_t)redzone)
			*(int *)pte = PG_V | PG_KR | i;
		else
			*(int *)pte = PG_V | PG_KW | i;
		setpgmap(v, *(long *)pte);
		/*
		 * Make the Sysmap entries for the u area
		 * invalid to catch any references here.
		 */
		if (v >= (caddr_t)&u && v < (caddr_t)((int)&u + UPAGES*NBPG))
			*(int *)pte = 0;
		pte++;
	}
	/* read-only until first data page */
	for (; v < etext; v += NBPG, i++) {
		if (kernprot)
			*(int *)pte = PG_V | PG_KR | i;
		else
			*(int *)pte = PG_V | PG_KW | i;
		setpgmap(v, *(long *)pte);
		pte++;
	}
	/* writeable until end */
	for (; v < (caddr_t)end; v += NBPG, i++) {
		*(int *)pte = PG_V | PG_KW | i;
		setpgmap(v, *(long *)pte);
		pte++;
	}
	/* invalid until end of last seg */
	i = ((u_int)end + SGOFSET) &~ SGOFSET;
	for (; v < (caddr_t)i; v += NBPG)
		setpgmap(v, (long)0);

	/*
	 * Remove user access to monitor-set-up maps.
	 */
	for (i = MONSTART >> SGSHIFT; i < MONEND >> SGSHIFT; i++) {
		if (getsegmap(i) == SEGINV)
			continue;
		for (v = (caddr_t)ctob(NPAGSEG * i); 
		    v < (caddr_t)ctob(NPAGSEG * (i+1)); v += NBPG)
			setpgmap(v, (long)(((getpgmap(v) & ~PG_PROT) | PG_KW)));
	}

	/*
	 * If physmem is patched to be non-zero, use it instead
	 * of the monitor value unless physmem is larger than
	 * the amount of memory on hand.
	 */
#ifdef notyet
	/* DOESN'T WORK WITH REV N PROTOTYPE PROMS!!! */
	/*
	 * v_memorysize is the amount of physical memory while
	 * v_memoryavail is the amount of usable memory in versions
	 * equal or greater to 1.
	 */
	if (romp->v_romvec_version >= 1) {
		if (physmem == 0 || physmem > btop(*romp->v_memoryavail))
			physmem = btop(*romp->v_memoryavail);
	} else {
		if (physmem == 0 || physmem > btop(*romp->v_memorysize))
			physmem = btop(*romp->v_memorysize);
	}
#else notyet
	if (physmem == 0 || physmem > btop(*romp->v_memorysize))
		physmem = btop(*romp->v_memorysize);
#endif notyet
	/*
	 * If debugger is in memory, subtract the pages it stole from physmem.
	 */
	if (boothowto & RB_DEBUG)
		physmem -= *dvec->dv_pages;
	maxmem = physmem;

	/*
	 * Allocate the memory for the video copy memory. If 
	 * the memory is not needed it will be returned when the
	 * device is configured later.  We need a 128Kb aligned
	 * 128Kb continguous chunk of physical memory.
	 */
#include "bwtwo.h"
#if NBWTWO > 0
	if (physmem > btop(OBFBADDR + FBSIZE))
		fbobmemavail = 1;
	else
		fbobmemavail = 0;
#else
	fbobmemavail = 0;
#endif
	
	/*
	 * Determine if anything lives in DVMA bus space.
 	 */
	disable_dvma();
	for (dvmapage = 0; dvmapage < btoc(dvmasize); dvmapage++) {
		mapin(CMAP1, btop(CADDR1), (u_int)(dvmapage | PGT_DVMABUS),
		    1, PG_V | PG_KW);
		if (poke((short *)CADDR1, TESTVAL) == 0)
			break;
	}
	enable_dvma();

	if (cpu == CPU_SUN2_50)
		uvecinit();		/* initialize user vectors */

	/*
	 * Initialize error message buffer (at end of core).
	 * Printf's which occur prior to this will not be captured.
	 */
	i = btoc(sizeof (struct msgbuf));
	maxmem -= i;
	mapin(msgbufmap, btop(&msgbuf), (u_int)maxmem, (int)i, PG_V | PG_KW);
	msgbufinit = 1;

	/*
	 * Allocate IOPB memory space just below the message
	 * buffer and map it to the first pages of DVMA space.
	 */
	maxmem -= IOPBMEM;
	for (v = (caddr_t)DVMA, i = maxmem; i < maxmem + IOPBMEM;
	    v += NBPG, i++) {
		struct pte tmp;			/* scratch pte */

		mapin(&tmp, btop(v), i, 1, PG_V | PG_KW);
	}

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	printf("mem = %dK (0x%x)\n", ctob(physmem) / 1024, ctob(physmem));

	if (dvmapage < btoc(dvmasize)) {
		printf("CAN'T HAVE PERIPHERALS IN RANGE 0 - %dKB\n",
		    ctob(dvmasize) / 1024);
		panic("dvma collision");
	}

#ifndef lint
	if (sizeof (struct user) > UPAGES * NBPG)
		panic("user area too large");
#endif

	if ((int)Syslimit > CSEG << SGSHIFT)
		panic("system map tables too large");

	/*
	 * Determine how many buffers to allocate.
	 * Use 10% of memory (not counting 512K for kernel), with min of 16.
	 * We allocate 1/4 as many swap buffer headers as file i/o buffers.
	 */
	if (bufpages == 0)
		bufpages = (physmem * NBPG - 512 * 1024) / 10 / CLBYTES;
	if (nbuf == 0) {
		nbuf = bufpages;
		if (nbuf < 16)
			nbuf = 16;
		if (nbuf > 100)
			nbuf = 100;
	}
	if (bufpages > nbuf * (MAXBSIZE / CLBYTES))
		bufpages = nbuf * (MAXBSIZE / CLBYTES);
	if (nswbuf == 0) {
		nswbuf = (nbuf / 4) &~ 1;	/* force even */
		if (nswbuf > 32)
			nswbuf = 32;		/* sanity */
	}

	/*
	 * Allocate space for system data structures.
	 * The first available real memory address is held in "firstaddr".
	 * The first available kernel virtual address is in "v".
	 * As pages of kernel virtual memory are allocated, "v" is incremented.
	 * "mapaddr" is the real memory address where the tables start.
	 * It is used when remapping the tables later.
	 * In order to support the frame buffer which might appear in 
	 * the middle of contiguous memory we adjust the map address to 
	 * start after the end of the frame buffer.  Later we will adjust
	 * the core map to take this hole into account.  The reason for
	 * this is to keep all the kernel tables contiguous in virtual space.
	 */
	if (fbobmemavail) {
		mapaddr = btoc(OBFBADDR + FBSIZE);
	} else {
		mapaddr = firstaddr;
	}
	v = (caddr_t)(ctob(firstaddr) + KERNELBASE);
#define	valloc(name, type, num) \
	    (name) = (type *)(v); (v) = (caddr_t)((name)+(num))
#define	valloclim(name, type, num, lim) \
	    (name) = (type *)(v); (v) = (caddr_t)((lim) = ((name)+(num)))
	valloc(swbuf, struct buf, nswbuf);
	valloclim(inode, struct inode, ninode, inodeNINODE);
	valloclim(file, struct file, nfile, fileNFILE);
	valloclim(proc, struct proc, nproc, procNPROC);
	valloclim(text, struct text, ntext, textNTEXT);
	valloc(cfree, struct cblock, nclist);
	valloc(callout, struct callout, ncallout);
	valloc(swapmap, struct map, nswapmap = nproc * 2);
	valloc(argmap, struct map, ARGMAPSIZE);
	valloc(kernelmap, struct map, nproc);
	valloc(iopbmap, struct map, IOPBMAPSIZE);
	valloc(mb_hd.mh_map, struct map, DVMAMAPSIZE);
	valloc(ncache, struct ncache, ncsize);
#ifdef QUOTA
	valloclim(dquot, struct dquot, ndquot, dquotNDQUOT);
#endif QUOTA

/* define macro to round up to integer size */
#define INTSZ(X)	howmany((X), sizeof (int))
/* define macro to round up to nearest int boundary */
#define INTRND(X)	roundup((X), sizeof (int))

#ifdef IPCSEMAPHORE
	valloc(sem, struct sem, seminfo.semmns);
	valloc(sema, struct semid_ds, seminfo.semmni);
	valloc(semmap, struct map, seminfo.semmap);
	valloc(sem_undo, struct sem_undo *, nproc);
	valloc(semu, int, INTSZ(seminfo.semusz * seminfo.semmnu));
#endif IPCSEMAPHORE

#ifdef IPCMESSAGE
	valloc(msg, char, INTRND(msginfo.msgseg * msginfo.msgssz));
	valloc(msgmap, struct map, msginfo.msgmap);
	valloc(msgh, struct msg, msginfo.msgtql);
	valloc(msgque, struct msqid_ds, msginfo.msgmni);
	valloc(msglock, char, INTRND(msginfo.msgmni));
#endif IPCMESSAGE

#ifdef IPCSHMEM
	valloc(shmem, struct shmid_ds, shminfo.shmmni);

	/* PRE-VM-REWRITE */
	valloc(shm_shmem, struct shmid_ds *, (nproc * shminfo.shmseg));
	/* END ... PRE-VM-REWRITE */
#endif IPCSHMEM

	/*
	 * Now allocate space for core map
	 * Allow space for all of physical memory minus the amount 
	 * dedicated to the system. The amount of physical memory
	 * dedicated to the system is the total virtual memory of
	 * the system minus the space in the buffers which is not
	 * allocated real memory.
	 */
	ncmap = physmem - firstaddr;
	valloclim(cmap, struct cmap, ncmap, ecmap);
	unixsize = btoc((int)(ecmap+1) - KERNELBASE);

	if ((int)unixsize > SYSPTSIZE)
		panic("sys pt too small");

	/*
	 * Clear allocated space, and make r/w entries
	 * for the space in the kernel map.
	 */
	if (unixsize >= physmem - 8*UPAGES)
		panic("no memory");
	pte = &Sysmap[firstaddr];
	range = btoc(v) - (firstaddr + btop(KERNELBASE));
	i = firstaddr + btop(KERNELBASE);
	mapin(pte, i, mapaddr,  range, PG_V | PG_KW);
	bzero(ptob(i), (u_int)(range * NBPG));
	mapaddr = mapaddr + range;

	/*
	 * Initialize callouts.
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];

	/*
	 * Initialize memory allocator and swap
	 * and user page table maps.
	 */
	if (fbobmemavail) {
		meminit((int)firstaddr, maxmem);
		memialloc((int)firstaddr, (int)btop(OBFBADDR));
		memialloc((int)mapaddr, maxmem);
	} else {
		meminit((int)mapaddr, maxmem);
		memialloc((int)mapaddr, maxmem);
	}
	maxmem = freemem;
	printf("avail mem = %d\n", ctob(maxmem));
	rminit(kernelmap, (long)(USRPTSIZE - CLSIZE), (long)CLSIZE,
	    "usrpt", nproc);
	rminit(iopbmap, (long)ctob(IOPBMEM), (long)DVMA, 
	    "IOPB space", IOPBMAPSIZE);
	rminit(mb_hd.mh_map, (long)(dvmasize - IOPBMEM), (long)IOPBMEM,
	    "DVMA map space", DVMAMAPSIZE);

	/*
	 * Initialize kernel memory allocator.
	 */
	kmem_init();

	/*
	 * Configure the system.
	 */
	configure();
	if (fbobmemavail) {
		/*
		 * Onboard frame buffer memory still
		 * available, put back onto the free list.
		 */
		memialloc((int)btop(OBFBADDR), (int)btop(OBFBADDR + FBSIZE));
		fbobmemavail = 0;
	}
	bufmemall();
	uinit();
	(void) spl0();
}

/*
 * Allocate physical memory for system buffers
 * In Ethernet memory if the right board exists &
 * the root device is ND & there are no block I/O DMA devices
 */
bufmemall()
{
	int mpages = 0;
	struct pte *pte;
	long a, va;
	int npages;
	int i, j, base, residual;
	struct ifnet *ifp;
#include "nd.h"
#if NND > 0
	int ndopen();
	struct mapent *mp;
	struct mb_device *md;
	struct mb_driver *mdr;

	if ((ifp = if_ifwithaf(AF_INET)) == NULL || ifp->if_memmap == NULL)
		goto mainmem;
	if (bdevsw[major(rootdev)].d_open != ndopen)
		goto mainmem;
	for (md = mbdinit; mdr = md->md_driver; md++)
		if (md->md_alive && (mdr->mdr_flags & MDR_BIODMA))
			goto mainmem;

	mp = (struct mapent *)(ifp->if_memmap + 1);
	for (mpages = 0; mp->m_size > 0; mp++)
		mpages += btop(mp->m_size);
	if (bufpages < mpages) {
		bufpages = mpages;
		if (nbuf < bufpages)
			nbuf = bufpages;
	}
mainmem:
#endif

	printf("using %d buffers containing %d bytes of main memory\n",
		nbuf, (bufpages - mpages) * CLBYTES);
	a = rmalloc(kernelmap, (long)(nbuf*MAXBSIZE/NBPG));
	if (a == 0)
		panic("no vmem for buffers");
	buffers = (caddr_t)kmxtob(a);
	pte = &Usrptmap[a];
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
		if (i < residual)
			npages = base+1;
		else
			npages = base;
		/* XXX - this loop only works if CLSIZE == 1 */
		for (j = 0; j < npages; j += CLSIZE) {
			if (mpages-- > 0) {
				va = rmalloc(ifp->if_memmap, (long)CLBYTES);
				if (va == 0)
					panic("bufmemall");
				pte[j] = Sysmap[btop(va - KERNELBASE)];
			} else {
				if (memall(pte+j, CLSIZE, &proc[0], CSYS) == 0)
					panic("no mem for buffers");
				*(int *)(pte+j) |= PG_V|PG_KW;
			}
			va = (int)kmxtob(a+j);
			vmaccess(pte+j, (caddr_t)va, 1);
			bzero((caddr_t)va, CLBYTES);
		}
		pte += MAXBSIZE/CLBYTES;
		a += MAXBSIZE/CLBYTES;
	}
	/*
	 * Double map and then unmap the last page of the last
 	 * buffer to insure the presence of a pmeg.
	 * AARRRGGGHHH. Kludge away.
	 */
	if (base < MAXBSIZE/CLBYTES) {
		int uc;

		pte -= MAXBSIZE/CLBYTES;
		va = (int)kmxtob(a-1);
		pte[MAXBSIZE/CLBYTES - 1] = pte[0];
		vmaccess(&pte[MAXBSIZE/CLBYTES - 1], (caddr_t)va, 1);
		/* now unmap without disturbing the pmeg */
		*(int *)&pte[MAXBSIZE/CLBYTES - 1] = 0;
		uc = getusercontext();
		setusercontext(KCONTEXT);
		setpgmap((caddr_t)va, (long)0);
		setusercontext(uc);
	}
	buf = (struct buf *)zmemall(memall, nbuf * sizeof(struct buf));
	if (buf == 0)
		panic("no mem for buf headers");
}

struct	bootf {
	char	let;
	short	bit;
} bootf[] = {
	'a',	RB_ASKNAME,
	's',	RB_SINGLE,
	'i',	RB_INITNAME,
	'h',	RB_HALT,
	'b',	RB_NOBOOTRC,
	'd',	RB_DEBUG,
	0,	0,
};
char *initname = "/etc/init";

/*
 * Parse the boot line to determine boot flags .
 */
bootflags()
{
	register struct bootparam *bp = *(romp->v_bootparam);
	register char *cp;
	register int i;

	cp = bp->bp_argv[1];
	if (cp && *cp++ == '-')
		do {
			for (i = 0; bootf[i].let; i++) {
				if (*cp == bootf[i].let) {
					boothowto |= bootf[i].bit;
					break;
				}
			}
			cp++;
		} while (bootf[i].let && *cp);
	if (boothowto & RB_INITNAME)
		initname = bp->bp_argv[2];
	if (boothowto & RB_HALT)
		halt("bootflags");
}

/*
 * Start the initial user process.
 * The program [initname] is invoked with one argument
 * containing the boot flags.
 */
icode()
{
	struct execa {
		char	*fname;
		char	**argp;
		char	**envp;
	} *ap;
	char *ucp, **uap, *arg0, *arg1;
	int i;

	u.u_error = 0;			/* paranoid */
	/* Make a user stack (1 page) */
	expand(1, 1);
	(void) swpexpand(0, 1, &u.u_dmap, &u.u_smap);

	/* Move out the boot flag argument */
	ucp = (char *)USRSTACK;
	(void) subyte(--ucp, 0);		/* trailing zero */
	for (i = 0; bootf[i].let; i++) {
		if (boothowto & bootf[i].bit)
			(void) subyte(--ucp, bootf[i].let);
	}
	(void) subyte(--ucp, '-');		/* leading hyphen */
	arg1 = ucp;

	/* Move out the file name (also arg 0) */
	for (i = 0; initname[i]; i++)
		;				/* size the name */
	for (; i >= 0; i--)
		(void) subyte(--ucp, initname[i]);
	arg0 = ucp;

	/* Move out the arg pointers */
	uap = (char **) ((int)ucp & ~(NBPW-1));
	(void) suword((caddr_t)--uap, 0);	/* terminator */
	(void) suword((caddr_t)--uap, (int)arg1);
	(void) suword((caddr_t)--uap, (int)arg0);

	/* Point at the arguments */
	u.u_ap = u.u_arg;
	ap = (struct execa *)u.u_ap;
	ap->fname = arg0;
	ap->argp = uap;
	ap->envp = 0;
	
	/* Now let exec do the hard work */
	execve();
	if (u.u_error) {
		printf("Can't invoke %s, error %d\n", initname, u.u_error);
		panic("icode");
	}
}

/*
 * Set up page tables for process 0 U pages.
 */
uinit()
{
	register struct pte *pte;
	int upage = btop((int)&u);
	register int i;
	extern char redzone[];

	/*
	 * main() will initialize proc[0].p_p0br to u.u_pcb.pcb_p0br
	 * and proc[0].p_szpt to 1.  All we have to do is set up
	 * the pcb_p{0,1}{b,l}r registers in the pcb for now.
	 */

	/* initialize base and length of P0 region */
	u.u_pcb.pcb_p0br = usrpt;
	u.u_pcb.pcb_p0lr = 0;		/* no user text/data (P0) for proc 0 */

	/*
	 * initialize base and length of P1 region,
	 * where the length here is for invalid pages
	 */
	u.u_pcb.pcb_p1br = initp1br(usrpt + 1 * NPTEPG);
	u.u_pcb.pcb_p1lr = P1PAGES;	/* no user stack (P1) for proc 0 */

	/*
	 * Doublely map the real page mapped read-only at the redzone
	 * to contain the ptes whose virtual address is usrpt. Got that?
 	 */
	mapin(&Usrptmap[0], btop(usrpt), (u_int)(getpgmap(redzone) & PG_PFNUM),
		1, PG_V | PG_KW);

	/*
	 * Now build the software page maps to map virtual U 
	 * to physical U.  The redzone page (our ptes) is below there.
	 */
	pte = usrpt + 1 * NPTEPG - UPAGES;
	for (i = 0; i < UPAGES; i++)
		*(int *)pte++ = PG_V | PG_KW | upage++;
}

#ifdef PGINPROF
/*
 * Return the difference (in microseconds)
 * between the current time and a previous
 * time as represented  by the arguments.
 */
vmtime(otime, olbolt)
	register int otime, olbolt;
{

	return (((time-otime)*HZ + lbolt-olbolt)*(1000000/HZ));
}
#endif

/*
 * Clear registers on exec
 */
setregs(entry)
	u_long entry;
{
	register int i;
	register struct regs *r = (struct regs *)u.u_ar0;

	for (i = 0; i < 8; i++) {
		r->r_dreg[i] = 0;
		if (&r->r_areg[i] != &r->r_sp)
			r->r_areg[i] = 0;
	}
	r->r_ps = PSL_USERSET;
	r->r_pc = entry;
	u.u_eosys = REALLYRETURN;
}

/*
 * Send an interrupt to process.
 *
 * When using new signals user code must do a
 * sys #139 to return from the signal, which
 * calls sigcleanup below, which resets the
 * signal mask and the notion of onsigstack,
 * and returns from the signal handler.
 */
sendsig(p, sig, mask)
	int (*p)(), sig, mask;
{
	register int usp, *regs, scp;
	struct nframe {
		int	sig;
		int	code;
		int	scp;
	} frame;
	struct sigcontext sc;
	int oonstack;

	regs = u.u_ar0;
	oonstack = u.u_onstack;
	/*
	 * Allocate and validate space for the signal handler
	 * context. Note that if the stack is in P0 space, the
	 * call to grow() is a nop, and the useracc() check
	 * will fail if the process has not already allocated
	 * the space with a `brk'.
	 */
	if (!u.u_onstack && (u.u_sigonstack & sigmask(sig))) {
		usp = (int)u.u_sigsp;
		u.u_onstack = 1;
	} else
		usp = regs[SP];
	usp -= sizeof (struct sigcontext);
	scp = usp;
	usp -= sizeof (frame);
	if (!u.u_onstack && usp <= USRSTACK - ctob(u.u_ssize))
		(void) grow((unsigned)usp);
	if (useracc((caddr_t)usp, sizeof (frame) + sizeof (sc), B_WRITE) == 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
printf("sendsig: bad user stack pid=%d, sig=%d\n", u.u_procp->p_pid, sig);
printf("usp is %x, action is %x, upc is %x\n", usp, p, regs[PC]);
		u.u_signal[SIGILL] = SIG_DFL;
		sig = sigmask(SIGILL);
		u.u_procp->p_sigignore &= ~sig;
		u.u_procp->p_sigcatch &= ~sig;
		u.u_procp->p_sigmask &= ~sig;
		psignal(u.u_procp, SIGILL);
		return;
	}
	/*
	 * push sigcontext structure.
	 */
	sc.sc_onstack = oonstack;
	sc.sc_mask = mask;
	sc.sc_sp = regs[SP];
	sc.sc_pc = regs[PC];
	sc.sc_ps = regs[PS];
	/*
	 * If trace mode was on for the user process
	 * when we came in here, it may have been because
	 * of an ast-induced trace on a trap instruction,
	 * in which case we do not want to restore the
	 * trace bit in the status register later on
	 * in sigcleanup().  If we were to restore it
	 * and another ast trap had been posted, we would
	 * end up marking the trace trap as a user-requested
	 * real trace trap and send a bogus "Trace/BPT" signal.
	 */
	if ((sc.sc_ps & PSL_T) && (u.u_pcb.pcb_p0lr & TRACE_AST))
		sc.sc_ps &= ~PSL_T;
	(void) copyout((caddr_t)&sc, (caddr_t)scp, sizeof (sc));
	/*
	 * push call frame.
	 */
	frame.sig = sig;
	if (sig == SIGILL || sig == SIGFPE || sig == SIGEMT) {
		frame.code = u.u_code;
		u.u_code = 0;
	} else
		frame.code = 0;
	frame.scp = scp;
	(void) copyout((caddr_t)&frame, (caddr_t)usp, sizeof (frame));
	regs[SP] = usp;
	regs[PC] = (int)p;
}

/*
 * Routine to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * notion of on signal stack from context
 * left there by sendsig (above).  Pop these
 * values and perform rti.
 */
sigcleanup()
{
	struct sigcontext *scp, sc;

	scp = (struct sigcontext *)fuword((caddr_t)u.u_ar0[SP] + sizeof(int));
	if ((int)scp == -1)
		return;
	if (copyin((caddr_t)scp, (caddr_t)&sc, sizeof (sc)))
		return;
	u.u_onstack = sc.sc_onstack & 01;
	u.u_procp->p_sigmask =
	    sc.sc_mask &~ (sigmask(SIGKILL)|sigmask(SIGCONT)|sigmask(SIGSTOP));
	u.u_ar0[SP] = sc.sc_sp;
	u.u_ar0[PC] = sc.sc_pc;
	u.u_ar0[PS] = sc.sc_ps;
	u.u_ar0[PS] &= ~PSL_USERCLR;
	u.u_ar0[PS] |= PSL_USERSET;
	u.u_eosys = REALLYRETURN;
}

int	waittime = -1;

boot(paniced, howto)
	int paniced, howto;
{
	static short prevflag = 0;
	static short bufprevflag = 0;
	register struct buf *bp;
	int iter, nbusy;
	int s;

	consdev = 0;
	startnmi();
	if ((howto&RB_NOSYNC) == 0 && waittime < 0 && bfreelist[0].b_forw &&
	    bufprevflag == 0) {
		bufprevflag = 1;		/* prevent panic recursion */
		waittime = 0;
		printf("syncing disks... ");
		s = spl0();
		sync();
		for (iter = 0; iter < 20; iter++) {
			nbusy = 0;
			for (bp = &buf[nbuf]; --bp >= buf; )
				if ((bp->b_flags & (B_BUSY|B_INVAL)) == B_BUSY)
					nbusy++;
			if (nbusy == 0)
				break;
			printf("%d ", nbusy);
			DELAY(40000 * iter);
		}
		(void) splx(s);
		printf("done\n");
	}
	s = spl7();				/* extreme priority */
	if (howto&RB_HALT) {
		halt((char *)NULL);
		/* MAYBE REACHED */
	} else {
		if (paniced == RB_PANIC && prevflag == 0) {
			if ((boothowto & RB_DEBUG) != 0 && nopanicdebug == 0) {
				CALL_DEBUG();
			}
			prevflag = 1;		/* prevent panic recursion */
			dumpsys();
		}
		printf("Rebooting Unix...\n");
		disable_all_interrupts();	/* kludge around bug in PROMs */
		(*romp->v_boot_me)(howto & RB_SINGLE ? "-s" : "");
		/* NOTREACHED */
	}
	(void) splx(s);
}

long	dumpmag = 0x8FCA0101;	/* magic number for savecore(8) */
long	dumpsize = 0;		/* also for savecore */

/*
 * Dump the system:
 * Map in memory page by page and call the dump device driver
 * to write it out.
 */
dumpsys()
{
	caddr_t addr;
	int pg;
	int bn = dumplo;
	int err = 0;
	int (*dumper)();

#ifdef notdef
	if ((minor(dumpdev)&07) != 1)
		return;
#endif
	printf("\ndumping to dev %x, offset %d\n", dumpdev, dumplo);

	addr = &DVMA[ctob(dvmasize-1)];
	dumper = bdevsw[major(dumpdev)].d_dump;
	setusercontext(KCONTEXT);
	for (pg = 0; pg < physmem && !err; pg++) {
		setpgmap(addr, (long)(pg | PG_V | PG_KW));
		err = (*dumper)(dumpdev, addr, bn, ctod(1));
		bn += ctod(1);
	}

	printf("dump ");
	switch (err) {

	case ENXIO:
		printf("device bad\n");
		break;

	case EFAULT:
		printf("device not ready\n");
		break;

	case EINVAL:
		printf("area improper\n");
		break;

	case EIO:
		printf("i/o error\n");
		break;

	case 0:
		printf("succeeded\n");
		break;

	default:
		printf("failed: error %d\n", err);
		break;
	}
}

/*
 * Initialize UNIX's vector table:
 * Vectors are copied from protoscb unless
 * they are zero; zero means preserve whatever the
 * monitor put there.  If the protoscb is zero,
 * then the original contents are copied into
 * the scb we are setting up.  We initialize up to
 * the VEC_MIN vector only since we don't know
 * whether or not the user interrupts vectors
 * are to be initialized at this time.
 */
initscb()
{
	register int *s, *p, *f;
	register int n;
	struct scb *orig, *getvbr();

	orig = getvbr();
	exit_vector = orig->scb_trap[14];
	s = (int *)&scb;
	p = (int *)&protoscb;
	f = (int *)orig;
	for (n = 0; n < VEC_MIN; s++, p++, f++, n++) {
		if (*p) 
			*s = *p;
		else
			*s = *f;
	}
	setvbr(&scb);
	/*
	 * If the boot flags say that the debugger is there,
	 * test and see if it really is by peeking at DVEC.
	 * If is isn't, we turn off the RB_DEBUG flag else
	 * we call the debugger scbsync() routine.
	 */
	if ((boothowto & RB_DEBUG) != 0) {
		if (peek((short *)DVEC) == -1)
			boothowto &= ~RB_DEBUG;
		else
			(*dvec->dv_scbsync)();
	}
}

/*
 * Finish scb initialization for user interrupt vectors
 */
uvecinit()
{
	register int *s, *p;
	register int n;

	s = (int *)&scb + VEC_MIN;
	p = (int *)&protoscb + VEC_MIN;
	for (n = VEC_MIN; n <= VEC_MAX; s++, p++, n++) {
		if (*p) 
			*s = *p;
	}
}

/*
 * Copy a segment (page) from a user virtual address
 * to a physical page number.
 */
copyseg(vaddr, pgno)
	caddr_t vaddr;
	int pgno;
{
	register struct pte *pte;
	register int lock;
	extern char CADDR1[];

	/*
	 * Make sure the user's page is valid and locked.
	 */
	pte = vtopte(u.u_procp, btop(vaddr));
	if (lock = !pte->pg_v) {
		pagein((u_int)vaddr, 1);		/* return it locked */
		pte = vtopte(u.u_procp, btop(vaddr));	/* pte may move */
	}
	/*
	 * Map the destination page into kernel address space.
	 */
	mapin(CMAP1, btop(CADDR1), (u_int)pgno, 1, PG_V | PG_KW);
	(void) copyin(vaddr, CADDR1, CLBYTES);
	if (lock)
		munlock(pte->pg_pfnum);
}

/*
 * Copy to/from user by mapping pages and using bcopy.
 * Called from copyin/out for blocks > 512 bytes.
 * SHOULD REPLACE mapin WITH SOMETHING SIMPLER.
 */
bcopyout(from, to, count)
	caddr_t from, to;
	int count;
{
	register int len, off, lock;
	struct pte *pte, *addrtopte();
	extern char CADDR2[];

	pte = addrtopte(to, (u_int)count);
	if (pte == NULL)
		return (EFAULT);

	while (count > 0) {
		off = ((int)to) & PGOFSET;
		len = count;
		if (off + len > NBPG)
			len = NBPG - off;
		if ((*(int *)pte & PG_PROT) != PG_UW)
			return (EFAULT);
		if (lock = !pte->pg_v) {
			pagein((u_int)to, 1);		/* return it locked */
			pte = vtopte(u.u_procp, btop(to)); /* pte may move */
		}
		mapin(CMAP2, btop(CADDR2), *(u_int *)pte & PG_PFNUM, 1,
			PG_V | PG_KW);
		bcopy(from, CADDR2+off, (u_int)len);
		pte->pg_m = 1;
		pte->pg_r = 1;
		if (lock)
			munlock(pte->pg_pfnum);
		from += len;
		to += len;
		count -= len;
		pte++;
	}
	return (0);
}

bcopyin(from, to, count)
	caddr_t from, to;
	int count;
{
	register int len, off, lock;
	struct pte *pte, *addrtopte();
	extern char CADDR2[];

	pte = addrtopte(from, (u_int)count);
	if (pte == NULL)
		return (EFAULT);

	while (count > 0) {
		off = ((int)from) & PGOFSET;
		len = count;
		if (off + len > NBPG)
			len = NBPG - off;
		if ((*(int *)pte & PG_PROT) != PG_UW &&
		    (*(int *)pte & PG_PROT) != PG_URKR)
			return (EFAULT);
		if (lock = !pte->pg_v) {
			pagein((u_int)from, 1);		/* return it locked */
			pte = vtopte(u.u_procp, btop(from)); /* pte may move */
		}
		mapin(CMAP2, btop(CADDR2), *(u_int *)pte & PG_PFNUM, 1,
			PG_V | PG_KR);
		bcopy(CADDR2+off, to, (u_int)len);
		pte->pg_r = 1;
		if (lock)
			munlock(pte->pg_pfnum);
		from += len;
		to += len;
		count -= len;
		pte++;
	}
	return (0);
}

/*
 * Handle "physical" block transfers.
 */
physstrat(bp, strat, pri)
	register struct buf *bp;
	int (*strat)();
	int pri;
{
	register int npte, n;
	register long a;
	unsigned v;
	register struct pte *pte, *kpte;
	struct proc *rp;
	int va, s, o;

	v = btop(bp->b_un.b_addr);
	o = (int)bp->b_un.b_addr & PGOFSET;
	npte = btoc(bp->b_bcount + o) + 1;
	while ((a = rmalloc(kernelmap, (long)npte)) == NULL) {
		mapwant(kernelmap)++;
		(void) sleep((caddr_t)kernelmap, PSWP+4);
	}
	kpte = &Usrptmap[a];
	rp = bp->b_flags&B_DIRTY ? &proc[2] : bp->b_proc;
	if ((bp->b_flags & B_PHYS) == 0)
		pte = &Sysmap[btop((int)bp->b_un.b_addr - KERNELBASE)];
	else if (bp->b_flags & B_UAREA)
		pte = &rp->p_addr[v];
	else if (bp->b_flags & B_PAGET)
		pte = &Usrptmap[btokmx((struct pte *)bp->b_un.b_addr)];
	else
		pte = vtopte(rp, v);
	for (n = npte; --n != 0; kpte++, pte++)
		*(int *)kpte = PG_V | PG_KW | (*(int *)pte & PG_PFNUM);
	*(int *)kpte = 0;
	va = (int)kmxtob(a);
	vmaccess(&Usrptmap[a], (caddr_t)va, npte);
	bp->b_saddr = bp->b_un.b_addr;
	bp->b_un.b_addr = (caddr_t)(va | o);
	bp->b_kmx = a;
	bp->b_npte = npte;
	(*strat)(bp);
	if (bp->b_flags & B_DIRTY)
		return;
	s = spl6();
	while ((bp->b_flags & B_DONE) == 0)
		(void) sleep((caddr_t)bp, pri);
	(void) splx(s);
	bp->b_un.b_addr = bp->b_saddr;
	bp->b_kmx = 0;
	bp->b_npte = 0;
	mapout(&Usrptmap[a], npte);
	rmfree(kernelmap, (long)npte, a);
}

/*
 * Halt the machine and return to the monitor
 */
halt(s)
	char *s;
{
	extern struct scb *getvbr();

	if (s)
		(*romp->v_printf)("(%s) ", s);
	(*romp->v_printf)("Unix Halted\n\n");
	startnmi();
	if (exit_vector)
		getvbr()->scb_trap[14] = exit_vector;
	asm("trap #14");
	if (exit_vector)
		getvbr()->scb_trap[14] = protoscb.scb_trap[14];
	stopnmi();
}

/*
 * Print out a traceback for the caller - can be called anywhere
 * within the kernel or from the monitor by typing g4.  This
 * causes a jmp to dotrace which simply calls tracedump().
 */
/*VARARGS0*/
tracedump(x1)
	caddr_t x1;
{
	struct frame *fp = (struct frame *)(&x1 - 2);
	u_int tospage = btoc(fp);

	(*romp->v_printf)("Begin traceback...fp = %x\n", fp);
	while (btoc(fp) == tospage) {
		if (fp == fp->fr_savfp) {
			(*romp->v_printf)("FP loop at %x", fp);
			break;
		}
		(*romp->v_printf)("Called from %x, fp=%x, args=%x %x %x %x\n",
		    fp->fr_savpc, fp->fr_savfp,
		    fp->fr_arg[0], fp->fr_arg[1], fp->fr_arg[2], fp->fr_arg[3]);
		fp = fp->fr_savfp;
	}
	(*romp->v_printf)("End traceback...\n");
}

/*
 * if a() calls b() calls caller(),
 * caller() returns return address in a().
 */
int
(*caller())()
{

#ifdef lint
	return ((int (*)())0);
#else
	asm("   movl    a6@,a0");
	asm("   movl    a0@(4),d0");
#endif
}

/*
 * if a() calls callee(), callee() returns the
 * return address in a();
 */
int
(*callee())()
{

#ifdef lint
	return ((int (*)())0);
#else
	asm("   movl    a6@(4),d0");
#endif
}

/*
 * Buscheck is called by mbsetup to check to see it the requested
 * setup is a valid busmem type (i.e. Multibus or VMEbus).  Returns
 * 1 if ok busmem type, returns 0 if not busmem type.  This routine
 * make checks and panic's if an illegal busmem type request is detected.
 */
buscheck(pte, npf)
	register struct pte *pte;
	register int npf;
{
	register int i, pf;
	register int pt = *(int *)pte & PGT_MASK;

	if ((cpu == CPU_SUN2_120 && pt == PGT_MBMEM) ||
	    (cpu == CPU_SUN2_50 && (pt == PGT_VME0 || pt == PGT_VME8))) {
		pf = pte->pg_pfnum;
		if (pt != PGT_VME8 && pf < btoc((cpu == CPU_SUN2_120)?
		    SUN2_120_DVMASIZE : SUN2_50_DVMASIZE)) {
			/*
			 * Remember: DVMA space has two different
			 * addresses: 1) the F Meg of the cpu kernel
			 * context; and 2) a portion of the IO address
			 * space.  We are checking the IO space here.
			 */
			panic("buscheck: busmem in DVMA range");
		}
		for (i = 0; i < npf; i++, pte++, pf++) {
			if (cpu == CPU_SUN2_50 && pt == PGT_VME0 &&
			    pf >= btoc(VME0_SIZE)) {
				pf -= btoc(VME0_SIZE);
				pt = PGT_VME8;
			}
			if ((*(int *)pte & PGT_MASK) != pt ||
			    pte->pg_pfnum != pf)
				panic("buscheck: request not contiguous");
		}
		return (1);
	}
	return (0);
}

/* 
 * Compute the address of an I/O device within standard address
 * ranges and return the result.  This is used by DKIOCINFO
 * ioctl to get the best guess possible for the actual address
 * set on the card.
 */
getdevaddr(addr)
	caddr_t addr;
{
	int off = (int)addr & PGOFSET;
	int pte = getkpgmap(addr);
	int physaddr = ((pte & PG_PFNUM) & ~PGT_MASK) * NBPG;

	switch (pte & PGT_MASK) {
	case PGT_MBIO_VME8:
		if (cpu == CPU_SUN2_50) {
			if (physaddr > VME8_SIZE - (1 << 16)) {
				/* 16 bit VMEbus address */
				physaddr -= (VME8_SIZE - (1 << 16));
			} else {
				/* upper half of 24 bit VMEbus address */
				physaddr += VME0_SIZE;
			}
		}
		/* else PGT_MBIO, physaddr doesn't require adjustments */
		break;

	case PGT_MBMEM_VME0:
	case PGT_OBMEM:
	case PGT_OBIO:
		/* physaddr doesn't require adjustments */
		break;
	}

	return (physaddr + off);
}
