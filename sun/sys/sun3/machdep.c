#ifndef lint
static	char sccsid[] = "@(#)machdep.c 1.16 87/04/06";
#endif lint

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
#include "../machine/eeprom.h"
#include "../machine/interreg.h"
#include "../machine/memerr.h"
#include "../machine/eccreg.h"
#ifdef SUN3_260
#include "../machine/enable.h"
#endif

/*
 * Declare these as initialized data so we can patch them.
 */
int nbuf = 0;
int nswbuf = 0;
int bufpages = 0;
int physmem = 0;	/* memory size in pages, patch if you want less */
int kernprot = 1;	/* write protect kernel text */
int msgbufinit = 1;	/* message buffer has been initialized, ok to printf */
int nopanicdebug = 0;

int (*exit_vector)() = (int (*)())0;	/* Where to go when halting UNIX */

#define TESTVAL	0xA55A	/* memory test value */

u_char	getsegmap(), pmegallocres();
long	getpgmap();

#ifdef SUN3_260
/*
 * Since there is no implied ordering of the memory cards, we store
 * a zero terminated list of pointers to eccreg's that are active so
 * that we only look at existent memory cards during softecc() handling.
 */
struct eccreg *ecc_alive[MAX_ECC+1];
#endif SUN3_260

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
	register int c;
	register struct pte *pte;
	register caddr_t v;
	u_int firstaddr;		/* next free physical page number */
	extern char start[], etext[], end[], CADDR1[], Syslimit[];
	u_char pm, u_pmeg;
	u_int mapaddr;
	caddr_t zmemall();
	void v_handler();
	int mon_mem;
	int range;

	bootflags();			/* get the boot options */

	initscb();			/* set trap vectors */
	*INTERREG |= IR_ENA_INT;	/* make sure interrupts can occur */
	firstaddr = btoc((int)end - KERNELBASE);

	setcputype();			/* sets cpu and dvmasize variables */

	/* 
	 * Initialize map of allocated page map groups.
 	 * Must be done before mapin of unallocated segments.
	 */
	pmeginit();			/* init list of pmeg data structures */
	ctxinit();			/* init context data structures */

	/*
	 * Reserve necessary pmegs and set segment mapping.
	 * It is assumed here that the pmegs for low
	 * memory have already been duplicated for the
	 * segments up in the kernel virtual address space.
	 */

	/*
	 * This KLUDGE is because earlier proms will blow up during
	 * reboot if the pmeg which maps the last 2 physical pages
	 * of memory is modified!  Because of this, we have to reserve
	 * this pmeg if we are on a system which potentially has a
	 * buggy prom.  This is thoroughly disgusting!
	 */
	if ((cpu == CPU_SUN3_160 || cpu == CPU_SUN3_50) &&
	    (pm = getsegmap(*romp->v_memoryavail >> SGSHIFT)) != SEGINV)
	        pmegreserve(pm);

	/*
	 * invalidate to start of high mapping
	 */
	for (i = 0; i < KERNELBASE >> SGSHIFT; i++)
		setsegmap(i, (u_char)SEGINV);

	/* reserve kernel pmegs */
	for (; i < ptos(NPAGSEG - 1 + btoc(end)); i++)
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

	for (; i < NSEGMAP - 1; i++)		/* invalid until last seg */
		setsegmap(i, (u_char)SEGINV);

	/*
	 * Last segment contains the u area itself, 
	 * the pmeg here is reserved for all contexts.
	 * We also reserve the invalid pmeg itself.
	 */
	u_pmeg = getsegmap(NSEGMAP - 1);
	pmegreserve(u_pmeg);
	pmegreserve((u_char)SEGINV);

#ifdef SUN3_260
	if (cpu == CPU_SUN3_260)
		vac_init();
#endif

	/*
	 * Make sure the memory error register is
	 * set up to generate interrupts on error.
	 */
#if defined(SUN3_160) || defined(SUN3_50) || defined(SUN3_110) || defined(SUN3_60)
	if (cpu == CPU_SUN3_160 || cpu == CPU_SUN3_50 ||
	    cpu == CPU_SUN3_110 || cpu == CPU_SUN3_60)
		MEMREG->mr_per = PER_INTENA | PER_CHECK;
#endif defined(SUN3_160) || defined(SUN3_50) || defined(SUN3_110) || defined(SUN3_60)

#ifdef SUN3_260
	if (cpu == CPU_SUN3_260) {
		register struct eccreg **ecc_nxt = ecc_alive;
		register struct eccreg *ecc;

		/*
		 * Go probe for all memory cards and perform initialization.
		 * The address of the cards found is stashed in ecc_alive[].
		 * We assume that the cards are already enabled and the
		 * base addresses have been set correctly by the monitor.
		 */
		for (ecc = ECCREG; ecc < &ECCREG[MAX_ECC]; ecc++) {
			if (peekc((char *)ecc) == -1)
				continue;
			MEMREG->mr_dvma = 1; /* clear intr from mem register */
			ecc->syndrome |= SY_CE_MASK; /* clear syndrom fields */
			ecc->eccena |= ENA_SCRUB_MASK;
			ecc->eccena |= ENA_BUSENA_MASK;
			*ecc_nxt++ = ecc;
		}
		*ecc_nxt = (struct eccreg *)0;		/* terminate list */
	}
#endif SUN3_260

	/*
	 * Allocate pmegs for DVMA space
	 */
	for (i = ptos(btop(DVMA)); i < ptos(btop(DVMA) + dvmasize); i++) {
		pm = pmegallocres();
		setsegmap(i, pm);
		for (v = (caddr_t)ctob(NPAGSEG * i);
		    v < (caddr_t)ctob(NPAGSEG * (i+1)); v += NBPG)
			setpgmap(v, (long)0);
	}

	/*
	 * Now go through all the other contexts and set up the segment
	 * maps so that all segments are mapped the same.
	 * We have to use a PROM routine to do this since we don't want
	 * to switch to a new (unmapped) context to call setsegmap()!
	 */
	for (c = 0; c < NCONTEXT; c++) {
		if (c == KCONTEXT)
			continue;

		for (v = (caddr_t)0, i = 0;
		    v < (caddr_t)ctob(NPAGSEG * NSEGMAP); v += NBSG, i++)
			(*romp->v_setcxsegmap)(c, v, getsegmap(i));
	}

	/*
	 * Initialize kernel page table entries.
	 */
	pte = &Sysmap[0];

	/* invalid until start except msgbuf/scb/pte page which is KW */
	for (v = (caddr_t)KERNELBASE; v < (caddr_t)start; v += NBPG) {
		if ((btoc(v) == btoc(&scb)) || btoc(v) == btoc(&msgbuf))
			*(int *)pte = PG_V | PG_KW | getpgmap(v) & PG_PFNUM;
		else
			*(int *)pte = 0;
		setpgmap(v, *(long *)pte++);
	}

	/* set up kernel text pages */
	for (; v < (caddr_t)etext; v += NBPG) {
		if (kernprot)		/* is kernel to be protected? */
			*(int *)pte = PG_V | PG_KR | getpgmap(v) & PG_PFNUM;
		else
			*(int *)pte = PG_V | PG_KW | getpgmap(v) & PG_PFNUM;
		setpgmap(v, *(long *)pte++);
	}

	/* set up kernel data/bss pages to be writeable */
	for (; v < (caddr_t)end; v += NBPG) {
		*(int *)pte = PG_V | PG_KW | getpgmap(v) & PG_PFNUM;
		setpgmap(v, *(long *)pte++);
	}

	/* invalid until end of this segment */
	i = ((u_int)end + SGOFSET) & ~SGOFSET;
	for (; v < (caddr_t)i; v += NBPG)
		setpgmap(v, (long)0);

	/*
	 * Remove user access to monitor-set-up maps.
	 */
	for (i = MONSTART>>SGSHIFT; i < MONEND>>SGSHIFT; i++) {
		if (getsegmap(i) == SEGINV)
			continue;
		for (v = (caddr_t)ctob(NPAGSEG * i); 
		    v < (caddr_t)ctob(NPAGSEG * (i+1)); v += NBPG)
			setpgmap(v, 
			    (long)(((getpgmap(v) & ~PG_PROT) | PG_KW | PG_NC)));
	}

	for (i = DEBUGSTART>>SGSHIFT; i < DEBUGEND>>SGSHIFT; i++) {
		if (getsegmap(i) == SEGINV)
			continue;
		for (v = (caddr_t)ctob(NPAGSEG * i); 
			v < (caddr_t)ctob(NPAGSEG * (i+1));
		    v += NBPG) {
			setpgmap(v, 
			    (long)(((getpgmap(v) & ~PG_PROT) | PG_KW | PG_NC)));
		}
	}

	/*
	 * Loop through the last segment and set page protections.
	 * We want to invalidate any other pages in last segment
	 * besides the u area, EEPROM_ADDR, CLKADDR, MEMREG, INTERREG,
	 * ECCREG and MONSHORTPAGE.  This sets up the kernel
	 * redzone below the u area (NOTE: we will be mapping in
	 * a `yellowzone' also).  We get the interrupt redzone
	 * for free when the kernel is write protected as the
	 * interrupt stack is the first thing in the data area.
	 * Since u and MONSHORTPAGE are defined as 32 bit virtual
	 * addresses (to get short references to work), we must
	 * mask to get only the 28 bits we really want to look at.
	 */
	for (v = (caddr_t)ctob(NPAGSEG * (NSEGMAP - 1));
	     v < (caddr_t)ctob(NPAGSEG * NSEGMAP); v += NBPG) {
		if ((u_int)v == ((u_int)MONSHORTPAGE & 0x0FFFFFFF))
			setpgmap(v,		/* remove any user access */
			    (long)(((getpgmap(v) & ~PG_PROT) | PG_KW | PG_NC)));
		else if (((u_int)v < ((u_int)&u & 0x0FFFFFFF) ||
		    (u_int)v >= (((u_int)&u & 0x0FFFFFFF) + UPAGES*NBPG)) &&
		    (u_int)v != (u_int)EEPROM_ADDR &&
		    (u_int)v != (u_int)CLKADDR &&
		    (u_int)v != (u_int)MEMREG &&
		    (u_int)v != (u_int)INTERREG &&
		    (u_int)v != (u_int)ECCREG)
			setpgmap(v, (long)0);
	}

	/*
	 * v_memorysize is the amount of physical memory while
	 * v_memoryavail is the amount of usable memory in versions
	 * equal or greater to 1.  Mon_mem is the difference which
	 * is the number of pages hidden by the monitor.
	 */
	if (romp->v_romvec_version >= 1)
		mon_mem = btop(*romp->v_memorysize - *romp->v_memoryavail);
	else
		mon_mem = 0;

	/*
	 * If physmem is patched to be non-zero, use it instead of
	 * the monitor value unless physmem is larger than the total
	 * amount of memory on hand.
	 */
	if (physmem == 0 || physmem > btop(*romp->v_memorysize))
		physmem = btop(*romp->v_memorysize);
	/*
	 * Adjust physmem down for the pages stolen by the monitor.
	 */
	physmem -= mon_mem;

	/*
	 * If debugger is in memory, subtract the pages it stole from physmem.
	 * By doing this here instead of adding to mon_mem, the physical
	 * memory message display by UNIX will show memory loss for debugger.
	 */
	if (boothowto & RB_DEBUG)
		physmem -= *dvec->dv_pages;
	maxmem = physmem;

	/*
	 * Now map in a global stack overflow page into a ``yellowzone''.
	 * This is a kludge so that we can extend the kernelstack in the
	 * u area some w/o having to change offsets in the u area from 3.0.
	 * For later releases, we will just extend KERNELSTACK and get rid
	 * of all of this crap.
	 */
	setpgmap((caddr_t)(UADDR - NBPG), (long)(PG_V | PG_KW | --maxmem));
	bzero((caddr_t)(UADDR - NBPG), NBPG);

	/*
	 * v_vector_cmd is the handler for new monitor vector
	 * command in versions equal or greater to 2.
	 * We install v_handler() there for Unix.
	 */
	if (romp->v_romvec_version >= 2)
		*romp->v_vector_cmd = v_handler;

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
	 * We're paranoid and go through both the 16 bit
	 * and 32 bit device types.
 	 */
	disable_dvma();
	/* Access VME addresses, don't worry about cache here. */
	for (dvmapage = 0; dvmapage < btoc(dvmasize); dvmapage++) {
		mapin(CMAP1, btop(CADDR1), (u_int)(dvmapage | PGT_VME_D16),
		    1, PG_V | PG_KW);
		if (poke((short *)CADDR1, TESTVAL) == 0)
			break;
		mapin(CMAP1, btop(CADDR1), (u_int)(dvmapage | PGT_VME_D32),
		    1, PG_V | PG_KW);
		if (poke((short *)CADDR1, TESTVAL) == 0)
			break;
	}
	enable_dvma();

	/*
	 * Allocate IOPB memory space just below the end of
	 * memory and map it to the first pages of DVMA space.
	 */
	maxmem -= IOPBMEM;
	for (v = (caddr_t)DVMA, i = maxmem; i < maxmem + IOPBMEM;
	    v += NBPG, i++) {
		struct pte tmp;			/* scratch pte */

		/* Map to IOPB permanently, no flush. */
		mapin(&tmp, btop(v), i, 1, PG_V | PG_KW);
	}

	/*
	 * Good {morning,afternoon,evening,night}.
	 * When printing memory, use the total including
	 * those hidden by the monitor (mon_mem).
	 */
	printf(version);
	printf("mem = %dK (0x%x)\n", ctob(physmem + mon_mem) / 1024,
	    ctob(physmem + mon_mem));

	if (dvmapage < btoc(dvmasize)) {
		printf("CAN'T HAVE PERIPHERALS IN RANGE 0 - %dKB\n",
		    ctob(dvmasize) / 1024);
		panic("dvma collision");
	}

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
	 * The first available real memory address is in "firstaddr".
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
	if (fbobmemavail)
		mapaddr = btoc(OBFBADDR + FBSIZE);
	else
		mapaddr = firstaddr;
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
	bzero(ptob(i), (u_int)range * NBPG);
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
		memialloc((int)(1 + mapaddr), maxmem);
	} else {
		meminit((int)mapaddr, maxmem);
		memialloc((int)mapaddr, maxmem);
	}
	maxmem = freemem;
	printf("avail mem = %d\n", ctob(maxmem));
	rminit(iopbmap, (long)ctob(IOPBMEM), (long)DVMA, 
	    "IOPB space", IOPBMAPSIZE);
	rminit(mb_hd.mh_map, (long)(dvmasize - IOPBMEM), (long)IOPBMEM,
	    "DVMA map space", DVMAMAPSIZE);

	uinit();		/* initialize the u area & kernelmap */
	kmem_init();		/* initialize kernel memory allocator */

	/*
	 * Configure the system.
	 */
	configure();		/* set up devices */
	if (fbobmemavail) {
		/*
		 * Onboard frame buffer memory still
		 * available, put back onto the free list.
		 */
		memialloc((int)btop(OBFBADDR), (int)btop(OBFBADDR + FBSIZE));
		fbobmemavail = 0;
	}
	bufmemall();
#ifdef SUN3_260
	if (cpu == CPU_SUN3_260) {
		on_enablereg((u_char)ENA_CACHE);  /* turn on cache */
		vac = 1; /* low level vac routine checks vac before flushing */
	}
#endif
	(void) spl0();		/* drop priority */
}

/*
 * Allocate physical memory for system buffers
 * In Ethernet memory if the right board exists &
 * the root device is ND & there are no block I/O DMA devices
 */
bufmemall()
{
	struct pte *pte;
	long a, va;
	int npages;
	int i, j, base, residual;

	printf("using %d buffers containing %d bytes of main memory\n",
		nbuf, bufpages * CLBYTES);
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
			if (memall(pte+j, CLSIZE, &proc[0], CSYS) == 0)
				panic("no mem for buffers");
			*(int *)(pte+j) |= PG_V|PG_KW;
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
		pte -= MAXBSIZE/CLBYTES;
		va = (int)kmxtob(a-1);
		pte[MAXBSIZE/CLBYTES - 1] = pte[0];
		vmaccess(&pte[MAXBSIZE/CLBYTES - 1], (caddr_t)va, 1);
		/* This page is not used yet, no flush. */
		/* now unmap without disturbing the pmeg */
		*(int *)&pte[MAXBSIZE/CLBYTES - 1] = 0;
		setpgmap((caddr_t)va, (long)0);
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
	register struct bootparam *bp = (*romp->v_bootparam);
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

	u.u_error = 0;				/* paranoid */
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
 * This is closely related to way the code in locore.s sets things up.
 *
 * We use physical page 0 for the u area, and a different page
 * for the proc 0 page table. The latter contains only one pte
 * at the top of the page which points to the proc 0 u area,
 * so we doublely map this page with msgbuf and the scb down
 * in low memory so that this pte is found in Usrptmap[]
 * (which unfortunately is needed by some user programs).
 */
uinit()
{
	register struct pte *pte;
	register int offset;

	/*
	 * main() will initialize proc[0].p_p0br to u.u_pcb.pcb_p0br
	 * and proc[0].p_szpt to 1.  All we have to do is set up
	 * the pcb_p{0,1}{b,l}r registers in the pcb for now.
	 */

	/*
	 * First find the page offset into the segment that msgbuf is on
	 * into `offset' and initialize pte into usrpt that many pages.
	 * This is where we can cache consistently double map the
	 * physical page.  Then we use mapin() to do the dirty work.
	 */
	offset = btop(&msgbuf) & (NPAGSEG - 1);
	pte = &usrpt[offset * NPTEPG];
	mapin(&Usrptmap[offset], btop(pte),
	    (u_int)(getpgmap((caddr_t)&msgbuf) & PG_PFNUM), 1, PG_V | PG_KW);

	/* initialize base and length of P0 region */
	u.u_pcb.pcb_p0br = pte;
	u.u_pcb.pcb_p0lr = 0;		/* no user text/data (P0) for proc 0 */

	/*
	 * Initialize base and length of P1 region,
	 * where the length here is for invalid pages.
	 */
	u.u_pcb.pcb_p1br = initp1br(pte + 1 * NPTEPG);
	u.u_pcb.pcb_p1lr = P1PAGES;	/* no user stack (P1) for proc 0 */

	/*
	 * Now build the software page maps to map virtual U to physical U.
	 * We assume that this page is physical page 0 and UPAGES == 1
	 * (locore.s enforces this and won't assemble if UPAGES != 1).
	 */
	*(int *)(pte + 1 * NPTEPG - 1) = PG_V | PG_KW | 0;

	offset++;			/* skip over doublely mapped page */
	rminit(kernelmap, (long)(USRPTSIZE - offset), (long)offset,
	    "usrpt", nproc);		/* initialize with what is left */
}

#ifdef PGINPROF
/*
 * Return the difference (in microseconds)
 * between the current time and a previous
 * time as represented by the arguments.
 */
vmtime(otime, olbolt)
	register int otime, olbolt;
{

	return (((time-otime)*HZ + lbolt-olbolt)*(1000000/HZ));
}
#endif PGINPROF

/*
 * Clear registers on exec
 */
setregs(entry)
	u_long entry;
{
	register int i;
	register struct regs *r = (struct regs *)u.u_ar0;
	extern struct proc *fpprocp;

	for (i = 0; i < 8; i++) {
		r->r_dreg[i] = 0;
		if (&r->r_areg[i] != &r->r_sp)
			r->r_areg[i] = 0;
	}
	r->r_ps = PSL_USERSET;
	r->r_pc = entry;
	u.u_eosys = REALLYRETURN;

	/*
	 * Clear any external and internal fpp state.
	 * If this process was the last one to load its
	 * external fpp state, erase that fact also.
	 */
	bzero((caddr_t)&u.u_fp_status, sizeof (u.u_fp_status));
	bzero((caddr_t)&u.u_fp_istate, sizeof (u.u_fp_istate));
	if (u.u_procp == fpprocp)
		fpprocp = (struct proc *)0;
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
	if (howto & RB_HALT) {
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
		(*romp->v_boot_me)(howto & RB_SINGLE ? "-s" : "");
		/*NOTREACHED*/
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
#endif notdef
	printf("\ndumping to dev %x, offset %d\n", dumpdev, dumplo);

	addr = &DVMA[ctob(dvmasize-1)];
	dumper = bdevsw[major(dumpdev)].d_dump;
	setcontext(KCONTEXT);
	vac_flushall(); /* make physical memory consistent with the cache */
	for (pg = 0; pg < physmem && !err; pg++) {
		setpgmap(addr, (long)(pg | PG_V | PG_KW));
		err = (*dumper)(dumpdev, addr, bn, ctod(1));
		bn += ctod(1);
		vac_pageflush(addr);
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
		printf("failed: unknown error\n");
		break;
	}
}

/*
 * Initialize UNIX's vector table:
 * Vectors are copied from protoscb unless
 * they are zero; zero means preserve whatever the
 * monitor put there.  If the protoscb is zero,
 * then the original contents are copied into
 * the scb we are setting up.
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
	for (n = sizeof (struct scb)/sizeof (int); n--; s++, p++, f++) {
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
	/* Mapping is invalid after copyin(), page flush. */
	vac_pageflush((caddr_t)CADDR1);
	if (lock)
		munlock(pte->pg_pfnum);
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
#ifdef SUN3_260
	if (vac) {
		/*
		 * The mapping from the "user" virtual addresses to
		 * physical pages is replaced by the kernel virtual
		 * addresses.  The context register must be set before
		 * a page flush.
		 */
		int	saved_ctx, c;
		register unsigned	vaddr = (bp->b_flags & B_UAREA) ? 
				    (unsigned)(&u) : (unsigned)bp->b_un.b_addr;

		saved_ctx = getcontext();
		if (rp && rp->p_ctx)
		        c = rp->p_ctx->ctx_context;
		else
			c = KCONTEXT;
		setcontext(c);
		for (n = npte; --n != 0; kpte++, pte++) {
			vac_pageflush((caddr_t)vaddr);
			vaddr += NBPG;
			*(int *)kpte = PG_V | PG_KW | (*(int *)pte & PG_PFNUM);
		}
		setcontext(saved_ctx);
	} else
		for (n = npte; --n != 0; kpte++, pte++)
			*(int *)kpte = PG_V | PG_KW | (*(int *)pte & PG_PFNUM);
#else
	for (n = npte; --n != 0; kpte++, pte++)
		*(int *)kpte = PG_V | PG_KW | (*(int *)pte & PG_PFNUM);
#endif
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
 * within the kernel or from the monitor by typing "g4" (for sun-2
 * compatibility) or "w trace".  This causes the monitor to call
 * the v_handler() routine which will call tracedump() for these cases.
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
 * setup is a valid busmem type (i.e. VMEbus).  Returns 1 if ok
 * busmem type, returns 0 if not busmem type.  This routine
 * make checks and panic's if an illegal busmem type request is detected.
 */
buscheck(pte, npf)
	register struct pte *pte;
	register int npf;
{
	register int i, pf;
	register int pt = *(int *)pte & PGT_MASK;

	if (pt == PGT_VME_D16 || pt == PGT_VME_D32) {
		pf = pte->pg_pfnum;
		if (pf < btoc(DVMASIZE))
			panic("buscheck: busmem in DVMA range");
		for (i = 0; i < npf; i++, pte++, pf++) {
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
	case PGT_VME_D16:
	case PGT_VME_D32:
		if (physaddr > VME16_BASE) {
			/* 16 bit VMEbus address */
			physaddr -= VME16_BASE;
		} else if (physaddr > VME24_BASE) {
			/* 24 bit VMEbus address */
			physaddr -= VME24_BASE;
		}
		/*
		 * else 32 bit VMEbus address,
		 * physaddr doesn't require adjustments
		 */
		break;

	case PGT_OBMEM:
	case PGT_OBIO:
		/* physaddr doesn't require adjustments */
		break;
	}

	return (physaddr + off);
}

static int (*mon_nmi)();		/* monitor's level 7 nmi routine */
static u_char mon_mem;			/* monitor memory register setting */
extern int level7();			/* Unix's level 7 nmi routine */

stopnmi()
{
	struct scb *vbr, *getvbr();

	vbr = getvbr();
	if (vbr->scb_autovec[7 - 1] != level7) {
#ifndef GPROF
		set_clk_mode(0, IR_ENA_CLK7);	/* disable level 7 clk intr */
#endif !GPROF
		mon_nmi = vbr->scb_autovec[7 - 1];	/* save mon vec */
		vbr->scb_autovec[7 - 1] = level7;	/* install Unix vec */
#ifdef SUN3_260
		if (cpu == CPU_SUN3_260) {
			mon_mem = MEMREG->mr_eer;
			MEMREG->mr_eer = EER_INTENA | EER_CE_ENA;
		}
#endif SUN3_260
#if defined(SUN3_160) || defined(SUN3_50) || defined(SUN3_110) || defined(SUN3_60)
		if (cpu == CPU_SUN3_160 || cpu == CPU_SUN3_50 ||
		    cpu == CPU_SUN3_110 || cpu == CPU_SUN3_60)
			mon_mem = MEMREG->mr_per;
#endif defined(SUN3_160) || defined(SUN3_50) || defined(SUN3_110) || defined(SUN3_60)
	}
}

startnmi()
{
	struct scb *getvbr();

	if (mon_nmi) {
		getvbr()->scb_autovec[7 - 1] = mon_nmi;	/* install mon vec */
#ifndef GPROF
		set_clk_mode(IR_ENA_CLK7, 0);	/* enable level 7 clk intr */
#endif !GPROF
#ifdef SUN3_260
		if (cpu == CPU_SUN3_260)
			MEMREG->mr_eer = mon_mem;
#endif SUN3_260
#if defined(SUN3_160) || defined(SUN3_50) || defined(SUN3_110) || defined(SUN3_60)
		if (cpu == CPU_SUN3_160 || cpu == CPU_SUN3_50 ||
		    cpu == CPU_SUN3_110 || cpu == CPU_SUN3_60)
			MEMREG->mr_per = mon_mem;
#endif defined(SUN3_160) || defined(SUN3_50) || defined(SUN3_110) || defined(SUN3_60)
	}
}

/*
 * Handler for monitor vector cmd -
 * For now we just implement the old "g0" and "g4"
 * commands and a printf hack.
 */
void
v_handler(addr, str)
	int addr;
	char *str;
{

	switch (*str) {
	case '\0':
		/*
		 * No (non-hex) letter was specified on
		 * command line, use only the number given
		 */
		switch (addr) {
		case 0:		/* old g0 */
		case 0xd:	/* 'd'ump short hand */
			panic("zero");
			/*NOTREACHED*/
		
		case 4:		/* old g4 */
			tracedump();
			break;

		default:
			goto err;
		}
		break;

	case 'p':		/* 'p'rint string command */
	case 'P':
		(*romp->v_printf)("%s\n", (char *)addr);
		break;

	case '%':		/* p'%'int anything a la printf */
		(*romp->v_printf)(str, addr);
		(*romp->v_printf)("\n");
		break;

	case 't':		/* 't'race kernel stack */
	case 'T':
		tracedump();
		break;

	case 'u':		/* d'u'mp hack ('d' look like hex) */
	case 'U':
		if (addr == 0xd) {
			panic("zero");
		} else
			goto err;
		break;

	default:
	err:
		(*romp->v_printf)("Don't understand 0x%x '%s'\n", addr, str);
	}
}

/*
 * Handle parity/ECC memory errors.  XXX - use something like
 * vax to only look for soft ecc errors periodically?
 */
memerr()
{
	u_char per, eer;
	char *mess = 0;
	int c;
	long pme;

	eer = per = MEMREG->mr_er;
#ifdef SUN3_260
	if (cpu == CPU_SUN3_260 && (eer & EER_ERR) == EER_CE) {
		MEMREG->mr_eer = ~EER_CE_ENA;
		softecc();
		MEMREG->mr_dvma = 1;	/* clear latching */
		MEMREG->mr_eer |= EER_CE_ENA;
		return;
	} 
#endif SUN3_260

	/*
	 * Since we are going down in flames, disable further
	 * memory error interrupts to prevent confusion.
	 */
	MEMREG->mr_er &= ~ER_INTENA;

#if defined(SUN3_160) || defined(SUN3_50) || defined(SUN3_110) || defined(SUN3_60)
	if ((cpu == CPU_SUN3_160 || cpu == CPU_SUN3_50 || 
	    cpu == CPU_SUN3_110 || cpu == CPU_SUN3_60) &&
	    (per & PER_ERR) != 0) {
		printf("Parity Error Register %b\n", per, PARERR_BITS);
		mess = "parity error";
	}
#endif defined(SUN3_160) || defined(SUN3_50) || defined(SUN3_110) || defined(SUN3_60)

#ifdef SUN3_260
	if ((cpu == CPU_SUN3_260) && (eer & EER_ERR) != 0) {
		printf("Memory Error Register %b\n", eer, ECCERR_BITS);
		if (eer & EER_TIMEOUT)
			mess = "memory timeout error";
		if (eer & EER_UE)
			mess = "uncorrectable ECC error";
		if (eer & EER_WBACKERR)
			mess = "writeback  error";
	}
#endif SUN3_260

	if (!mess) {
		printf("Memory Error Register %b %b\n",
		    per, PARERR_BITS, eer, ECCERR_BITS);
	}

	printf("DVMA = %x, context = %x, virtual address = %x\n",
		MEMREG->mr_dvma, MEMREG->mr_ctx, MEMREG->mr_vaddr);

	c = getcontext();
	setcontext((int)MEMREG->mr_ctx);
	pme = getpgmap((caddr_t)MEMREG->mr_vaddr);
	printf("pme = %x, physical address = %x\n", pme,
	    ptob(((struct pte *)&pme)->pg_pfnum) + (MEMREG->mr_vaddr&PGOFSET));
	setcontext(c);

	/*
	 * Clear the latching by writing to the top
	 * nibble of the memory address register
	 */
	MEMREG->mr_dvma = 1;

	panic(mess);
	/*NOTREACHED*/
}

#ifdef SUN3_260
int prtsoftecc = 1;
extern int noprintf;
int memintvl = MEMINTVL;
struct {
	u_char	m_syndrome;
	char	m_bit[3];
} memlogtab[] = {
0x01, "64", 0x02, "65", 0x04, "66", 0x08, "67", 0x0B, "30", 0x0E, "31", 
0x10, "68", 0x13, "29", 0x15, "28", 0x16, "27", 0x19, "26", 0x1A, "25", 
0x1C, "24", 0x20, "69", 0x23, "07", 0x25, "06", 0x26, "05", 0x29, "04", 
0x2A, "03", 0x2C, "02", 0x31, "01", 0x34, "00", 0x40, "70", 0x4A, "46", 
0x4F, "47", 0x52, "45", 0x54, "44", 0x57, "43", 0x58, "42", 0x5B, "41", 
0x5D, "40", 0x62, "55", 0x64, "54", 0x67, "53", 0x68, "52", 0x6B, "51", 
0x6D, "50", 0x70, "49", 0x75, "48", 0x80, "71", 0x8A, "62", 0x8F, "63", 
0x92, "61", 0x94, "60", 0x97, "59", 0x98, "58", 0x9B, "57", 0x9D, "56", 
0xA2, "39", 0xA4, "38", 0xA7, "37", 0xA8, "36", 0xAB, "35", 0xAD, "34", 
0xB0, "33", 0xB5, "32", 0xCB, "14", 0xCE, "15", 0xD3, "13", 0xD5, "12", 
0xD6, "11", 0xD9, "10", 0xDA, "09", 0xDC, "08", 0xE3, "23", 0xE5, "22", 
0xE6, "21", 0xE9, "20", 0xEA, "19", 0xEC, "18", 0xF1, "17", 0xF4, "16" };

/* 
 * Routine to turn on correctable error reporting.
 */
ce_enable(ecc)
register struct eccreg *ecc;
{       
	ecc->eccena |= ENA_BUSENA_MASK;
}

/*
 * Probe memory cards to find which one(s) had ecc error(s).
 * If prtsoftecc is non-zero, log messages regarding the failing
 * syndrome.  Then clear the latching on the memory card.
 */
softecc()
{
	register struct eccreg **ecc_nxt, *ecc;

	for (ecc_nxt = ecc_alive; *ecc_nxt != (struct eccreg *)0; ecc_nxt++) {
		ecc = *ecc_nxt;
		if (ecc->syndrome & SY_CE_MASK) {
			if (prtsoftecc) 
				noprintf = 1;	/* (not on the console) */
				memlog(ecc); 		/* log the error */
				noprintf = 0;
			ecc->syndrome |= SY_CE_MASK;	/* clear latching */
			ecc->eccena &= ~ENA_BUSENA_MASK;/* disable board */
			timeout(ce_enable, (caddr_t)ecc, memintvl*hz);
		}
	}
}

memlog(ecc)
register struct eccreg *ecc;
{       
	register int i;
	register u_char syn;
	register u_int err_addr;
	int unum;
	
	syn = (ecc->syndrome & SY_SYND_MASK) >>	SY_SYND_SHIFT;
	err_addr = ((ecc->syndrome & SY_ADDR_MASK) << SY_ADDR_SHIFT) & 
		PHYSADDR;
	printf("mem%d: soft ecc addr %x syn %b ",
		  ecc - ECCREG, err_addr, syn, SYNDERR_BITS);
	for (i = 0;i < (sizeof (memlogtab) / sizeof (memlogtab[0])); i++) 
		if (memlogtab[i].m_syndrome == syn)
			break;
	if (i < (sizeof (memlogtab) / sizeof (memlogtab[0]))) {
		printf("%s ", memlogtab[i].m_bit);
		/* compute U number on board */
		/* which half is it */
		if (atoi(memlogtab[i].m_bit) >= LOWER) {
			switch (err_addr & ECC_BITS) {
			      case U14XX:
				unum = 14;
				break;
			      case U16XX:
				unum = 16;
				break;
			      case U18XX:
				unum = 18;
				break;
			      case U20XX:
				unum = 20;
				break;
			}
		} else {
			switch (err_addr & ECC_BITS) {
			      case U15XX:
				unum = 15;
				break;
			      case U17XX:
				unum = 17;
				break;
			      case U19XX:
				unum = 19;
				break;
			      case U21XX:
				unum = 21;
				break;
			}
		}
		printf("U%d%s\n", unum, memlogtab[i].m_bit);
	} else
		printf("No bit information\n");
}	
atoi(num_str)
char *num_str;
{
	register int num;

	num = (*num_str++ & 0xf) * 10;
	num = num + (*num_str & 0xf);
	return(num);
}
#endif SUN3_260
