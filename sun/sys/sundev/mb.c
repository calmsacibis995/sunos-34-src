#ifndef lint
static	char sccsid[] = "@(#)mb.c 1.1 86/09/25";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Mainbus support routines.
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/vmmac.h"
#include "../h/vmmeter.h"
#include "../h/vmparam.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/conf.h"
#include "../h/kernel.h"

#include "../machine/mmu.h"
#include "../machine/cpu.h"
#include "../machine/psl.h"
#include "../machine/pte.h"
#include "../machine/reg.h"

#include "../sundev/mbvar.h"

/*
 * Do transfer on controller argument.
 * We queue for resource wait in the Mainbus code if necessary.
 * We return 1 if the transfer was started, 0 if it was not.
 * If you call this routine with the head of the queue for a
 * Mainbus, it will automatically remove the controller from the Mainbus
 * queue before it returns.  If some other controller is given
 * as argument, it will be added to the request queue if the
 * request cannot be started immediately.  This means that
 * passing a controller which is on the queue but not at the head
 * of the request queue is likely to be a disaster.
 */
mbgo(mc)
	register struct mb_ctlr *mc;
{
	register struct mb_hd *mh = mc->mc_mh;
	register struct buf *bp = mc->mc_tab.b_actf->b_actf;
	register int s;

	s = splx(pritospl(SPLMB));
	if ((mc->mc_driver->mdr_flags & MDR_XCLU) &&
	    mh->mh_users > 0 || mh->mh_xclu)
		goto rwait;
	mc->mc_mbinfo = mbsetup(mh, bp, MB_CANTWAIT);
	if (mc->mc_mbinfo == 0)
		goto rwait;
	mh->mh_users++;
	if (mc->mc_driver->mdr_flags & MDR_XCLU)
		mh->mh_xclu = 1;
	(void) splx(s);
	if (mh->mh_actf == mc)
		mh->mh_actf = mc->mc_forw;
	if ((mc->mc_driver->mdr_flags & MDR_SWAB) && (bp->b_flags&B_READ)==0) {
		register int reg = MBI_MR(mc->mc_mbinfo);
		register int offset = MBI_OFFSET(mc->mc_mbinfo);

		if (reg < dvmasize)
			/* DVMA memory, access through DVMA address */
			swab(&DVMA[ctob(reg)] + offset,
			    &DVMA[ctob(reg)] + offset, (int)bp->b_bcount);
		else
			swab(bp->b_un.b_addr, bp->b_un.b_addr,
			    (int)bp->b_bcount);
	}
	(*mc->mc_driver->mdr_go)(mc);
	return (1);
rwait:
	if (mh->mh_actf != mc) {
		mc->mc_forw = NULL;
		if (mh->mh_actf == NULL)
			mh->mh_actf = mc;
		else
			mh->mh_actl->mc_forw = mc;
		mh->mh_actl = mc;
	}
	(void) splx(s);
	return (0);
}

/*
 * Do a transfer on device argument.  This is used when a controller
 * can have multiple devices active at once.
 * We queue for resource wait in the Mainbus code if necessary.
 * We return 1 if the transfer was started, 0 if it was not.
 * If you call this routine with the head of the queue for its
 * controller, it will automatically remove the device from the
 * queue before it returns.  If some other device is given as argument,
 * it (and its controller if not already present) will be added to the
 * request queue if the request cannot be started immediately.  This means
 * that passing a device that is on the queue but not at the head is
 * likely to be a disaster.
 */
mbugo(md)
	register struct mb_device *md;
{
	register struct mb_hd *mh = md->md_hd;
	register struct buf *bp = md->md_utab.b_forw;
	register struct mb_ctlr *mc = md->md_mc;
	struct mb_ctlr *nmc;
	register int s;

	s = splx(pritospl(SPLMB));
	if ((md->md_driver->mdr_flags & MDR_XCLU) &&
	    mh->mh_users || mh->mh_xclu)
		goto rwait;
	md->md_mbinfo = mbsetup(mh, bp, MB_CANTWAIT);
	if (md->md_mbinfo == 0)
		goto rwait;
	mh->mh_users++;
	if (md->md_driver->mdr_flags & MDR_XCLU)
		mh->mh_xclu = 1;
	(void) splx(s);
	if (mc->mc_tab.b_forw == (struct buf *)md)
		mc->mc_tab.b_forw = (struct buf *)md->md_forw;
	if ((md->md_driver->mdr_flags & MDR_SWAB) && (bp->b_flags &
	    B_READ) == 0) {
		register int reg = MBI_MR(mc->mc_mbinfo);
		register int offset = MBI_OFFSET(mc->mc_mbinfo);

		if (reg < dvmasize)
			/* DVMA memory, access through DVMA address */
			swab(&DVMA[ctob(reg)] + offset,
			    &DVMA[ctob(reg)] + offset, (int)bp->b_bcount);
		else
			swab(bp->b_un.b_addr, bp->b_un.b_addr,
			    (int)bp->b_bcount);
	}
	(*md->md_driver->mdr_go)(md);
	return (1);

rwait:
	if (mc->mc_tab.b_forw != (struct buf *)md) {
		md->md_forw = NULL;
		if (mc->mc_tab.b_forw == NULL)
			mc->mc_tab.b_forw = (struct buf *)md;
		else
			((struct mb_device *)mc->mc_tab.b_back)->md_forw = md;
		mc->mc_tab.b_back = (struct buf *)md;
		/*
		 * If controller isn't in queue, must add it too.
		 */
		for (nmc = mh->mh_actf; nmc != NULL; nmc = nmc->mc_forw)
			if (nmc == mc)
				break;
		if (nmc == NULL) {
			mc->mc_forw = NULL;
			mc->mc_tab.b_actf = NULL;
			if (mh->mh_actf == NULL)
				mh->mh_actf = mc;
			else
				mh->mh_actl->mc_forw = mc;
			mh->mh_actl = mc;
		}
	}
	(void) splx(s);
	return (0);
}

mbdone(mc)
	register struct mb_ctlr *mc;
{
	register struct mb_hd *mh = mc->mc_mh;
	register struct buf *bp = mc->mc_tab.b_actf->b_actf;

	if (mc->mc_driver->mdr_flags & MDR_XCLU)
		mh->mh_xclu = 0;
	mh->mh_users--;
	mbrelse(mh, &mc->mc_mbinfo);
	if (mc->mc_driver->mdr_flags & MDR_SWAB)
		swab(bp->b_un.b_addr, bp->b_un.b_addr, (int)bp->b_bcount);
	(*mc->mc_driver->mdr_done)(mc);
}

mbudone(md)
	register struct mb_device *md;
{
	register struct mb_hd *mh = md->md_hd;
	register struct buf *bp = md->md_utab.b_forw;

	if (md->md_driver->mdr_flags & MDR_XCLU)
		mh->mh_xclu = 0;
	mh->mh_users--;
	mbrelse(mh, &md->md_mbinfo);
	if (md->md_driver->mdr_flags & MDR_SWAB)
		swab(bp->b_un.b_addr, bp->b_un.b_addr, (int)bp->b_bcount);
	(*md->md_driver->mdr_done)(md);
}

/*
 * This is a wrapper for mbugo, since each controller may have
 * multiple devices on the queue.  Only returns 1 if all devices
 * on the controller fit into the newly freed Mainbus space.
 */
mbuwrap(mc)
	register struct mb_ctlr *mc;
{

	while (mc->mc_tab.b_forw &&
	    mbugo((struct mb_device *)mc->mc_tab.b_forw))
		;
	if (mc->mc_tab.b_forw)
		return (0);
	else {
		/*
		 * Since all devices are dequeued, remove controller from
		 * queue too.
		 */
		mc->mc_mh->mh_actf = mc->mc_forw;
		return (1);
	}
}

/*
 * Allocate and setup Mainbus map registers.
 * Flags says whether the caller can't
 * wait (e.g. if the caller is at interrupt level).
 *
 * We also allow DMA to memory already mapped at the Mainbus
 * (e.g., for Sun Ethernet board memory) and denote this with
 * a zero in the MBI_NMR field.
 */
mbsetup(mh, bp, flags)
	register struct mb_hd *mh;
	struct buf *bp;
	int flags;
{
	int npf, reg;
	register struct pte *pte;
	register char *addr;
	int s, o;
	struct mbcookie mbcookie;
#ifdef sun3
	register caddr_t usr_addr;
#endif sun3
#ifdef sun2
	int uc;
#endif

	o = (int)bp->b_un.b_addr & PGOFSET;
	if ((bp->b_flags & B_PHYS) == 0) {
		pte = &Sysmap[btop((int)bp->b_un.b_addr - KERNELBASE)];
#ifdef sun3
		usr_addr = (caddr_t)bp->b_un.b_addr;
#endif sun3
	} else {
		if (bp->b_kmx == 0)
			panic("mbsetup: zero kmx");
		pte = &Usrptmap[bp->b_kmx];
#ifdef sun3
		usr_addr = (caddr_t)kmxtob((int)(bp->b_kmx));
#endif sun3
	}

	npf = btoc(bp->b_bcount + o);
	if (buscheck(pte, npf)) {
		mbcookie.mbi_mapreg = pte->pg_pfnum;
		mbcookie.mbi_offset = o;
		return (*(int *)&mbcookie);
	}

	/* Defensively invalidate the page following the allocation */
	npf++;
	s = splx(pritospl(SPLMB));
	while ((reg = (int)rmalloc(mh->mh_map, (long)npf)) == 0) {
		if (flags & MB_CANTWAIT) {
			(void) splx(s);
			return (0);
		}
		mh->mh_mrwant++;
		(void) sleep((caddr_t)&mh->mh_mrwant, PSWP);
	}
	(void) splx(s);
	mbcookie.mbi_mapreg = reg;
	mbcookie.mbi_offset = o;
	addr = &DVMA[ctob(reg)];
#ifdef sun2
	uc = getusercontext();
	setusercontext(KCONTEXT);
#endif
	while (--npf > 0) {
		register int pfnum;

		switch (*(int *)pte & PGT_MASK) {
		default:
			/* may not go from dvma back out to the bus */
			panic("mbsetup: bad PGT");
		case PGT_OBMEM:
#ifdef sun3
			vac_pageflush(usr_addr);
			usr_addr += NBPG;
#endif sun3
			/* fall through */
		case PGT_OBIO:
			if ((pfnum = (*(int *)pte & PG_PFNUM)) == 0)
				panic("mbsetup: zero pfnum");
			setpgmap(addr, (long)(PG_V | PG_KW | pfnum));
			addr += NBPG;
			pte++;
		}
	}
	setpgmap(addr, (long)0);
#ifdef sun2
	setusercontext(uc);
#endif
	return (*(int *)&mbcookie);
}

/*
 * Non buffer setup interface... set up a buffer and call mbsetup.
 */
mballoc(mh, addr, bcnt, flags)
	struct mb_hd *mh;
	caddr_t addr;
	int bcnt, flags;
{
	struct buf mbbuf;

	mbbuf.b_un.b_addr = addr;
	mbbuf.b_flags = B_BUSY;
	mbbuf.b_bcount = bcnt;
	/* that's all the fields mbsetup() needs */
	return (mbsetup(mh, &mbbuf, flags));
}

/*
 * Release resources on Mainbus, and then unblock resource waiters.
 * The map register parameter is by value since we need to block
 * against Mainbus resets.
 */
mbrelse(mh, amr)
	register struct mb_hd *mh;
	int *amr;
{
	register int reg, s;
	register char *addr;
	int mr;
#ifdef sun2
	int uc;
#endif
 
	/*
	 * Carefully see if we should release the space, since
	 * it may be released asynchronously at Mainbus reset time.
	 */
	s = splx(pritospl(SPLMB));
	mr = *amr;
	if (mr == 0) {
		printf("mbrelse: MR == 0!!!\n");
		(void) splx(s);
		return;
	}
	*amr = 0;
	(void) splx(s);		/* let interrupts in, we're safe for a while */

	if ((reg = MBI_MR(mr)) < dvmasize) {	/* DVMA memory */
		long getpgmap();
		register int npf = 1;		/* plus one for last entry */

#ifdef sun2
		uc = getusercontext();
		setusercontext(KCONTEXT);
#endif
		for (addr = &DVMA[ctob(reg)]; getpgmap(addr) != (long)0;
		    addr += NBPG, npf++) {
			/*
			 * Flush outgoing DVMA addresses.  The DVMA pages
			 * are kernel pages, so don't worry about context no.
			 */
			vac_pageflush(addr);
			setpgmap(addr, (long)0);
		}
#ifdef sun2
		setusercontext(uc);
#endif
		/*
		 * Put back the registers in the resource map.
		 * The map code must not be reentered, so we do this
		 * at high spl.
		 */
		s = splx(pritospl(SPLMB));
		rmfree(mh->mh_map, (long)npf, (long)reg);
		(void) splx(s);

		/*
		 * Wakeup sleepers for map registers,
		 * and also, if there are processes blocked in mbgo(),
		 * give them a chance at the Mainbus.
		 */
		if (mh->mh_mrwant) {
			mh->mh_mrwant = 0;
			wakeup((caddr_t)&mh->mh_mrwant);
		}
	}
	/*
	 * Try to map waiting devices, using the appropriate routine.
	 */
	while (mh->mh_actf && (mh->mh_actf->mc_tab.b_actf == NULL ?
	    mbuwrap(mh->mh_actf) : mbgo(mh->mh_actf)))
		;
}

/*
 * Swap bytes in 16-bit [half-]words
 * for going between the 11 and the interdata
 */
swab(pf, pt, n)
	register caddr_t pf, pt;
	register int n;
{
	register char temp;

	n = (n+1)>>1;

	while (--n >= 0) {
		temp = *pf++;
		*pt++ = *pf++;
		*pt++ = temp;
	}
}
