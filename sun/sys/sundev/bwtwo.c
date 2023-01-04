#ifndef lint
static  char sccsid[] = "@(#)bwtwo.c 1.6 87/04/14 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Sun Two Black & White Board(s) Driver
 */

#include "bwtwo.h"
#include "win.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/ioctl.h"
#include "../h/map.h"
#include "../h/vmmac.h"
#include "../h/vmmeter.h"

#include "../sun/fbio.h"

#include "../machine/pte.h"
#include "../machine/mmu.h"
#include "../machine/cpu.h"

#include "../sundev/mbvar.h"
#include "../sundev/bw2reg.h"

#include "../pixrect/pixrect.h"
#include "../pixrect/pr_util.h"
#include "../pixrect/memvar.h"
#include "../pixrect/bw2var.h"

#ifndef sun2
#include "../sun3/eeprom.h"
#include "../sun3/enable.h"

extern u_char enablereg;
#endif !sun2

/*
 * Driver information for auto-configuration stuff.
 */
int	bwtwoprobe(), bwtwoattach(), bwtwointr();
struct	mb_device *bwtwoinfo[NBWTWO];
struct	mb_driver bwtwodriver = {
	bwtwoprobe, 0, bwtwoattach, 0, 0, bwtwointr,
	sizeof (struct bw2dev), "bwtwo", bwtwoinfo, 0, 0, 0,
};

#if NWIN > 0

/* SunWindows specific stuff */

struct	pixrectops bw2_ops = {
	mem_rop,
	mem_putcolormap,
	mem_putattributes
};

#endif NWIN > 0

struct bw2_softc {
	caddr_t	image;	/* pointer to frame buffer */
	int w, h;	/* resolution */
	int size;	/* size of frame buffer, bytes */
	int flags;	/* misc. flags */
#define	BW2_ISCONSOLE	1	/* I am the console */

#if NWIN > 0
	Pixrect pr;
	struct mpr_data prd;
#endif
} bw2_softc[NBWTWO];

bwtwoopen(dev, flag)
	dev_t dev;
	int flag;
{
	return (fbopen(dev, flag, NBWTWO, bwtwoinfo));
}

/*ARGSUSED*/
bwtwoclose(dev, flag)
	dev_t dev;
	int flag;
{
}

/*ARGSUSED*/
bwtwommap(dev, off, prot)
	dev_t dev;
	off_t off;
	int prot;
{
	register struct bw2_softc *softc = &bw2_softc[minor(dev)];

	if (off >= (off_t) softc->size)
		return -1;

	return getkpgmap(softc->image + off) & PG_PFNUM;
}

/*
 * Determine if a bwtwo exists at the given address.
 * If it exists, determine its size.
 */
/*ARGSUSED*/
bwtwoprobe(reg, unit)
	caddr_t reg;
	int	unit;
{
	register struct bw2_softc *softc = &bw2_softc[unit];

	struct size_tab {
#ifndef sun2
		char sizecode;
#endif !sun2
		int w, h;
		int size;
	};

#ifdef lint
#define	SIZE_TAB_ENT(w,h)	{ 0 }
#else lint
#ifdef sun2 
#define	SIZE_TAB_ENT(w,h) \
	{ w, h, mpr_linebytes(w, 1) * h }
#else sun2
#define	SIZE_TAB_ENT(w,h) \
	{ EED_SCR_/**/w/**/X/**/h, w, h, mpr_linebytes(w, 1) * h }
#endif sun2
#endif lint

	/* first table entry is default size */
	static struct size_tab size_tab[] = {
		SIZE_TAB_ENT(1152,900),
		SIZE_TAB_ENT(1024,1024),
#ifndef sun2
		SIZE_TAB_ENT(1600,1280),
		SIZE_TAB_ENT(1440,1440)
#endif !sun2
	};
#undef	SIZE_TAB_ENT

	register struct size_tab *sizep = size_tab;

	softc->flags = 0;

	/* figure out if this is the console frame buffer */
	if (unit == 0 &&
		*romp->v_outsink == OUTSCREEN &&
		*romp->v_fbtype == FBTYPE_SUN2BW)
		softc->flags |= BW2_ISCONSOLE;
 
#ifndef sun2
	{
		register char sizecode;

		/*
		 * If this is the console frame buffer use the size code
		 * from the EEPROM, otherwise just assume it is 1152x900.
		 *
		 * Maybe we should probe for the end of the frame buffer
		 * and guess the resolution from that?
		 */
		if (softc->flags & BW2_ISCONSOLE)
			sizecode = EEPROM->ee_diag.eed_scrsize;
		else
			sizecode = EED_SCR_1152X900;

		for (sizep = size_tab; sizecode != sizep->sizecode; sizep++)
			if (sizep >= &size_tab[sizeof(size_tab) /
				sizeof(size_tab[0])]) {
				sizep = size_tab;
				break;
			}
	}
#else !sun2
	{
		struct	bw2dev *bw2dev = (struct bw2dev *)(reg);
		register struct	bw2cr *alias1, *alias2;
		short w1, w2, wrestore;

		bw2crmapin(bw2dev);
		alias1 = &bw2dev->bw2cr;
		alias2 = alias1 + 1;

		/*
	 	 * Two adjacent shorts should be the same because
	 	 * the control register is replicated every 2 bytes
	 	 * throughout the control page.
	 	 */
		if ((w1 = peek((short *)alias1)) == -1)
			return 0;
		wrestore = w1;

		((struct bw2cr *)&w1)->vc_copybase = 0xAA & BW2_COPYBASEMASK;

		if (poke((short *)alias1, w1) ||
			(w2 = peek((short *)alias2)) == -1 ||
			w1 != w2) {
				(void) poke((short *)alias1, wrestore);
				return 0;
		}

		((struct bw2cr *)&w1)->vc_copybase = ~0xAA & BW2_COPYBASEMASK;

		if (poke((short *)alias1, w1) ||
			(w2 = peek((short *)alias2)) == -1 ||
			w1 != w2) {
				(void) poke((short *)alias1, wrestore);
				return 0;
		}

		if (poke((short *)alias1, wrestore))
			panic("bwtwoprobe");

		if (cpu != CPU_SUN2_120 &&
			((struct bw2cr *)&wrestore)->vc_1024_jumper)
			sizep++;
	}
#endif sun2

	softc->w = sizep->w;
	softc->h = sizep->h;

	return softc->size = sizep->size;
}

/*
 * Set up the softc structure
 */
bwtwoattach(md)
register struct mb_device *md;
{
	register struct bw2_softc *softc = &bw2_softc[md->md_unit];

	softc->image = md->md_addr;

#if defined(sun2) || defined(sun3)
	{
		/* pfnum before shadow memory mapped in */
		static int copyenpfnum;	

		/* virtual address mapped to shadow memory */
		static caddr_t copyenvirt = 0;

		int	pfnum;
		caddr_t	fbvirtaddr;
		caddr_t	v;
		int	i;
		extern char CADDR1[];

		pfnum = getkpgmap(md->md_addr) & PG_PFNUM;

#ifdef sun3
		/*
	 	 * If we are on a SUN3_50 (Model 25), then we must
	 	 * reserve the on board memory for the frame buffer.
	 	 */
		if (cpu == CPU_SUN3_50) {
			if (fbobmemavail == 0)
				panic("No video memory");
			else
				fbobmemavail = 0;
		}
#endif sun3

		/*
	 	 * Have we passed this way before? 
	 	 */
		if (fbobmemavail == 0) {
			if (copyenvirt == 0) {
				copyenvirt = (caddr_t)(*romp->v_fbaddr);
				if (pfnum == copyenpfnum)
					softc->image = copyenvirt;
			}
			return;
		}

		/* 
	 	 * We know that the copy memory can be used.  Use the
	 	 * shadow memory if the config flags say to use it.
	 	 */
#ifdef sun3
		if (md->md_flags & BW2_USECOPYMEM && 
			cpu != CPU_SUN3_50 && 
			cpu != CPU_SUN3_160) {
				printf(
			"WARNING: copy memory not available on this CPU\n"
					);
			md->md_flags &= ~BW2_USECOPYMEM;
		}
#endif

		if (!(md->md_flags & BW2_USECOPYMEM)) {
			/* don't bother using reserved shadow memory */
			copyenvirt  = md->md_addr;
			return;
		}

		/*
	 	 * Mark the onboard frame buffer memory as not available
	 	 * anymore as we are going to use it for copy memory.
	 	 */
		fbobmemavail = 0;

		/*
		 * If this frame buffer is the console, throw away
		 * config's mapping and use the boot PROM's.
		 */
		if (softc->flags & BW2_ISCONSOLE)
			fbvirtaddr = (caddr_t)md->md_addr;
		else {
			rmfree(kernelmap, (long)btoc(softc->size), 
			  	  (long)btokmx((struct pte *)(md->md_addr)));
			mapout(&Usrptmap[btokmx((struct pte *)(md->md_addr))], 
			  	  (int)btop(softc->size));
			fbvirtaddr = (caddr_t)(*romp->v_fbaddr);
		} 
		copyenvirt = fbvirtaddr;
		copyenpfnum = getkpgmap(fbvirtaddr) & PG_PFNUM;

		/*
	 	 * Copy the current frame buffer memory
	 	 * to the copy memory as we map it in.
	 	 */
		for (v = (caddr_t)fbvirtaddr, i = btop(OBFBADDR);
	    	    i < btop(OBFBADDR + softc->size); v += NBPG, i++) {
			mapin(CMAP1, btop(CADDR1),
		    	    (u_int)(i | PGT_OBMEM), 1, PG_V | PG_KW);
			bcopy(v, CADDR1, NBPG);
			setpgmap(v, (long)(PG_V|PG_KW|PGT_OBMEM|i));
		}

#ifdef sun2
		(void) bwtwosetcr(&((struct bw2dev *) md->md_addr)->bw2cr, 
			(OBFBADDR>>BW2_COPYSHIFT) | BW2_COPYENABLEMASK, 1);
#else sun2
		(void) setcopyenable(1);
#endif sun2

		if (pfnum == copyenpfnum)
			softc->image = copyenvirt;
	}
#endif sun2 || sun3
}

/*ARGSUSED*/
bwtwoioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	register int unit = minor(dev);
	register struct bw2_softc *softc = &bw2_softc[unit];

	switch (cmd) {

	case FBIOGTYPE: {
		register struct fbtype *fb = (struct fbtype *) data;

		fb->fb_type = FBTYPE_SUN2BW;
		fb->fb_height = softc->h;
		fb->fb_width = softc->w;
		fb->fb_depth = 1;
		fb->fb_cmsize = 2;
		fb->fb_size = softc->size;
	}
	break;

#if NWIN > 0

	case FBIOGPIXRECT: {
		struct fbpixrect *fbpr = (struct fbpixrect *) data;

		fbpr->fbpr_pixrect = &softc->pr;

		softc->pr.pr_ops = &bw2_ops;
		softc->pr.pr_size.x = softc->w;
		softc->pr.pr_size.y = softc->h;
		softc->pr.pr_depth = 1;
		softc->pr.pr_data = (caddr_t) &softc->prd;

		softc->prd.md_linebytes = mpr_linebytes(softc->w, 1);
		softc->prd.md_image = (short *) softc->image;
		softc->prd.md_offset.x = 0;
		softc->prd.md_offset.y = 0;
		softc->prd.md_primary = 0;
		softc->prd.md_flags = MP_DISPLAY;

		/*
		 * Enable video and clear interrupt
		 */
#ifdef sun2
		bwtwosetcr(
			&((struct bw2dev *) bwtwoinfo[unit]->md_addr)->bw2cr,
			BW2_VIDEOENABLEMASK, 1);
		bwtwosetcr(
			&((struct bw2dev *) bwtwoinfo[unit]->md_addr)->bw2cr,
			BW2_INTENABLEMASK, 0);
#else sun2
		(void) setvideoenable(1);
		(void) setintrenable(0);
#endif sun2
	}
	break;

#endif NWIN > 0

	case FBIOSVIDEO: {
		register int *video = (int *) data;

#ifdef sun2
		bwtwosetcr(
			&((struct bw2dev *) bwtwoinfo[unit]->md_addr)->bw2cr,
			BW2_VIDEOENABLEMASK, *video & FBVIDEO_ON);
#else sun2
		setvideoenable((u_int) (*video & FBVIDEO_ON));
#endif sun2
	}
	break;

	/* get video flags */
	case FBIOGVIDEO: {
		* (int *) data = 
#ifdef sun2
			((struct bw2dev *) bwtwoinfo[unit]->md_addr)
				->bw2cr.vc_video_en
#else sun2
			enablereg & ENA_VIDEO 
#endif sun2
			? FBVIDEO_ON : FBVIDEO_OFF;
	}
	break;

	default:
		return ENOTTY;

	} /* switch(cmd) */

	return 0;
}

bwtwointr()
{
#ifdef sun2
	int bwtwointclear();

	return fbintr(NBWTWO, bwtwoinfo, bwtwointclear);
#else sun2
	(void) setintrenable(0);
	return 0;
#endif sun2
}


#ifdef sun2

/* Sun-2 support routines */

/*
 * Turn off interrupts on bwtwo board.
 */
static
bwtwointclear(bw2dev)
	struct	bw2dev *bw2dev;
{
	int active;

	active = bw2dev->bw2cr.vc_int;
	bwtwosetcr(&bw2dev->bw2cr, BW2_INTENABLEMASK, 0);
	return active;
}

/*
 * Special access approach to video ctrl register needed because byte writes,
 * generated when do bit writes, replicates itself on the subsequent byte as
 * well (this is a hardware bug).  Thus, we need to access the register as a
 * word.  Also these routines assume that only one bit changes at a time.
 */
static
bwtwosetcr(bw2cr, mask, value)
	struct	bw2cr *bw2cr;
	short	mask;
	int	value;
{
	register short	w;

	/*
	 * Read word from video control register.
	 */
	w = *((short *)bw2cr);
	/*
	 * Modify bit as requested.
	 */
	if (value)
		w |= mask;
	else
		w &= ~mask;
	/*
	 * Write word back to video control register.
	 */
	*((short *)bw2cr) = w;
}

/*
 * Given the video base virtual address,
 * map in the control register address.
 * This lets us handle minor implementation differences.
 */
static
bw2crmapin(bw2dev)
	struct bw2dev *bw2dev;
{
	struct bw2cr *bw2cr = &bw2dev->bw2cr;
	int pte = getkpgmap((caddr_t)bw2dev);
	int page, delta;

	page = pte & PGT_MASK;

	if (page == PGT_OBMEM)
		delta = (int)BW2MB_CR - (int)BW2MB_FB;
	else if (page == PGT_OBIO)
		delta = (int)BW2VME_CR - (int)BW2VME_FB;
	else
		panic("bwtwocraddr");

	mapin(&Sysmap[btoc((u_int)bw2cr - KERNELBASE)], btoc(bw2cr),
	    pte + btoc(delta), 1, PG_V | (pte & PG_PROT));
}
#endif sun2
