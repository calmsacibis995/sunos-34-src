#ifndef lint
static  char sccsid[] = "@(#)cgtwo.c 1.6 87/01/21 SMI";
#endif

/*
 * Copyright 1986 by Sun Microsystems, Inc.
 */

#include "cgtwo.h"
#if NCGTWO > 0

/*
 * Sun Two and Three Color Board(s) Driver.  The main difference is that
 * the CG3 board has double buffering instead of pan and zoom, which
 * was never supported on the CG2 board.  Changing which buffer is
 * displayed requires syncing with the vertical retrace by interrupt.
 */

#include "win.h"

#include "../machine/pte.h"
#include "../machine/psl.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/dk.h"
#include "../h/vm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/ioctl.h"
#include "../h/kernel.h"
#include "../machine/mmu.h"
#include "../sun3/scb.h"
#include "../sun/fbio.h"
#include "../sundev/mbvar.h"
#include "../pixrect/pixrect.h"
#include "../pixrect/pr_util.h"
#include "../pixrect/memreg.h"
#include "../pixrect/cg2reg.h"
#include "../pixrect/cg2var.h"


#if NWIN > 0
#define	CG2_OPS	&cg2_ops
struct	pixrectops cg2_ops = {
	cg2_rop,
	cg2_putcolormap,
	cg2_putattributes,
};
#else
#define	CG2_OPS	(struct pixrectops *)0
#endif

struct	cg2pr cgtwoprdatadefault =
    {0/*cgpr_va set in ioctl*/, 0, 255, 0, 0 };
struct	pixrect cgtwopixrectdefault =
    { &cg2_ops, { 0/* set in ioctl */, 0/* set in ioctl */ }, CG2_DEPTH,
	0/* set in ioctl */};

#define CG2PROBESIZE	(NBPG) /* Bytes - single page */
#define CG2TOTALSIZE	(sizeof(struct cg2fb)+sizeof(struct cg2memfb))
			/* Bytes - entire device */
#define CG2EXTRAOFF	(sizeof(struct cg2memfb))
			/* Bytes - what kernel need address */
#define CG2EXTRASIZE	(sizeof(struct cg2fb))
			/* Bytes - what kernel need address */
int	cg2extraclicks = btoc(CG2EXTRASIZE);

/*
 * Driver information for auto-configuration stuff.
 */

/* Bit definitions for dev_specific[0] in cgtwoextra. */
#define CG2X_CG3	1
    /* Asserted if Board is a CG3. */

struct	cgtwoextra {
	unsigned char	*cg2x_ropaddr;  /* addr of cg2fb (set in probe) */
	int		cg2x_hwwidth;   /* device width (from resolution) */
	int		cg2x_hwheight;  /* device height (from resolution) */
	unsigned int 	cg2x_physaddr;  /* color board vme address */
	int		dev_specific[FB_ATTR_NDEVSPECIFIC];	/* catchall */
	    /* [0] is flag for cg2 or cg3. */
	    /* [1] is number of buffers. */
	    /* [0,1] same as gpone driver. */
};


int	cgtwoprobe (), cgtwoattach ();

/* For pr_dblbuf.c so kernel will build even if driver not configured in. */
int (*_pr_cg2_rop)() = cg2_rop;
int cgtwo_gattr(), (*_pr_cgtwo_gattr)() = cgtwo_gattr;
int cgtwo_vertical(), (*_pr_cgtwo_vertical)() = cgtwo_vertical;


struct	pixrect cgtwopixrect[NCGTWO];
struct	cg2pr cgtwoprdata[NCGTWO];
struct	mb_device *cgtwoinfo[NCGTWO];
struct	cgtwoextra cgtwoextra[NCGTWO];

struct	mb_driver cgtwodriver = {
	cgtwoprobe, 0, cgtwoattach, 0, 0, 0,
	CG2PROBESIZE, "cgtwo", cgtwoinfo, 0, 0, 0,
};


/*
 * We determine that the thing we're addressing is a cgtwo
 * board by accessing the rop chip.
 * Only CG2PROBESIZE is mapped when this is called.  CG2EXTRAOFF is mapped now.
 * Since the kernel pixrect only uses 1.3Meg of the 3.3Meg address
 * space of this device, we save 2Megs of virtual address space in the kernel
 * by only mapping the first page of the address space and the last 1.3.
 */
cgtwoprobe(reg, unit)
	caddr_t reg;
	int	unit;
{
	int	a = 0, page;
	int	i;
	register struct cg2fb *cg2fb;
	struct	cg2statusreg *status;
	u_char	testpixel;
	short	shrt;
	int	opcount = 0;

	/*
	 * Get physical page number and type associated with first page of dev.
	 */
	page = getkpgmap(reg) & PG_PFNUM;
	/*
	 * Allocate virtual memory for part of device will access.
	 * (Following sequence taken from ../machine/autoconf.c doprobe).
	 */
	if ((a = (int)rmalloc(kernelmap, (long)cg2extraclicks)) == 0)
		panic("out of kernelmap for cg2");
	cg2fb = (struct cg2fb *)kmxtob(a);
	cgtwoextra[unit].cg2x_ropaddr = (unsigned char *)cg2fb;
	/*
	 * Mapin virtual memory that kernel will use.
	 */
	mapin(&Usrptmap[a], btop(cgtwoextra[unit].cg2x_ropaddr),
	    page+btop(CG2EXTRAOFF), cg2extraclicks, PG_V|PG_KW);
	/*
	 * Actually do probe:
	 * Enable writing to all rop chips.
	 * cg2fb->ppmask.reg = -1;
	 */
	if (poke((short *)&cg2fb->ppmask.reg, 0xFF))
		goto failure;
	opcount++;
	/*
	 * Set SRC op in all chips
	 * cg2_setfunction(cg2fb, CG2_ALLROP, 0xCC);
	 */
	if (poke((short *)&cg2fb->ropcontrol[CG2_ALLROP].ropregs.mrc_op, 0xCC))
		goto failure;
	opcount++;
	/*
	 * Set pixel mode access
	 * cg2fb->status.reg.ropmode = SWWPIX;
	 */
	if ((shrt = peek((short *)&cg2fb->status.reg)) == -1)
		goto failure;
	opcount++;
	status = (struct cg2statusreg *)&shrt;
	status->ropmode = SWWPIX;
	if (poke((short *)&cg2fb->status.reg, shrt))
		goto failure;
	opcount++;
	/*
	 * Zero the end masks.
	 * cg2fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask1 = 0;
	 * cg2fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask2 = 0;
	 */
	if (poke((short *)&cg2fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask1, 0))
		goto failure;
	opcount++;
	if (poke((short *)&cg2fb->ropcontrol[CG2_ALLROP].ropregs.mrc_mask2, 0))
		goto failure;
	opcount++;
	/*
	 * Zero the width and opcount.
	 * cg2fb->ropcontrol[CG2_ALLROP].ropregs.mrc_width = 0;
	 * cg2fb->ropcontrol[CG2_ALLROP].ropregs.mrc_opcount = 0;
	 */
	if (poke((short *)&cg2fb->ropcontrol[CG2_ALLROP].ropregs.mrc_width, 0))
		goto failure;
	opcount++;
	if (poke((short *)&cg2fb->ropcontrol[CG2_ALLROP].ropregs.mrc_opcount,0))
		goto failure;
	opcount++;
	/*
	 * Set source alignment shift to zero, set fifo direction to one,
	 * ie. bus -> src1 -> src2 -> ROPC function unit.
	 * cg2_setshift(cg2fb, CG2_ALLROP, 0, 1);
	 */
	if (poke((short *)&cg2fb->ropcontrol[CG2_ALLROP].ropregs.mrc_shift,
	    1<<8))
		goto failure;
	opcount++;
	/*
	 * Load the fifo src1.
	 * cg2fb->ropcontrol[CG2_ALLROP].prime.ropregs.mrc_source2 = 0;
	 */
	if (poke((short *)&cg2fb->ropcontrol[CG2_ALLROP].prime.ropregs.
		mrc_source2, 0))
		goto failure;
	opcount++;
	/*
	 * Pump src2 into a pixel and load src1 with 0xFFFF
	 * *cg2_roppixaddr(cg2fb, 0, 0) = 0xFF;
	 */
	if (pokec((char *)&cg2fb->ropio.roppixel.pixel[0][0], 0xFF))
		goto failure;
	opcount++;
	/*
	 * mask every other pixel bit
	 * cg2fb->ppmask.reg = 0xAA;
	 */
	if (poke((short *)&cg2fb->ppmask.reg, 0xAA))
		goto failure;
	opcount++;
	/*
	 * pump src1 (0xF) into the pixel through the ppmask 0xAA
	 * *cg2_roppixaddr(cg2fb, 0, 0) = 0;
	 */
	if (pokec((char *)&cg2fb->ropio.roppixel.pixel[0][0], 0))
		goto failure;
	opcount++;
	/*
	 * read it back
	 * testpixel = *cg2_roppixaddr(cg2fb, 0, 0);
	 */
	if ((testpixel = (u_char)
	    peekc((char *)&cg2fb->ropio.roppixel.pixel[0][0])) == 0xFF)
		goto failure;
	opcount++;
	if (testpixel == 0xAA) {
		struct cg3fb *cg3fb = (struct cg3fb *) cg2fb;

		switch (cg2fb->status.reg.resolution) {
		case 0:
			cgtwoextra[unit].cg2x_hwwidth = CG2_WIDTH;
			cgtwoextra[unit].cg2x_hwheight = CG2_HEIGHT;
			break;
		case 1:
			cgtwoextra[unit].cg2x_hwwidth = CG2_SQUARE;
			cgtwoextra[unit].cg2x_hwheight = CG2_SQUARE;
			break;
		default:
			printf("CGTWO unknown resolution (%D)\n",
			    cg2fb->status.reg.resolution);
			opcount++;
			goto failure;
		}
		/* save the physical address of the color board (for gp later)*/
		cgtwoextra[unit].cg2x_physaddr =
			(unsigned int)( (page << PGSHIFT)  & 0xFFFFFF);

		/* clear out catchall array */
		bzero((char *) cgtwoextra[unit].dev_specific, 
			sizeof cgtwoextra[unit].dev_specific);

		/* Always at least one buffer. */
		cgtwoextra[unit].dev_specific[1] = 1;

		/*
		 * Determine if board is CG2 or CG3.  Wait bit in CG3
		 * will clear itself after trailing edge of vert. retrace.
		 */
		for (i = 0; i < 10; i++) {
			cg3fb->dblbuf.word |= 0x200;  /* Wait bit */
			if (cg3fb->dblbuf.word & 0x200) break;
			    /* Make sure it got set. */
		}
		/* 
		 * Have to wait for trailing edge of vertical retrace.
		 * Make sure that you also first see leading edge.
		 */
		while (cg2fb->status.reg.retrace)
			;
		while (!cg2fb->status.reg.retrace)
			;
		while (cg2fb->status.reg.retrace)
			;
		if (cg3fb->dblbuf.word & 0x200) {
			cg3fb->dblbuf.word &= 0xfdff;
			    /* Restore state of bit if it was cg2. */
		} else {
			/*
			 * CG3
			 * Report single-buffered, regardless.
			 */
			cgtwoextra[unit].dev_specific [0] |= CG2X_CG3;
			cgtwoextra[unit].dev_specific [1] = 1;
		}
		return (CG2PROBESIZE);
	}
#ifdef DEBUG
	printf("CGTWO testpixel was %X instead of 0xAA\n", testpixel);
	memropc_print(&cg2fb->ropcontrol[CG2_ALLROP].ropregs);
#endif
failure:
#if defined(DEBUG) || defined(lint)
	printf("CGTWO probe failure (opcount == %D)\n", opcount);
#endif
	rmfree(kernelmap, (long)cg2extraclicks, (long)a);
	mapout(&Usrptmap[a], cg2extraclicks);
	return (0);
}


#ifdef DEBUG
memropc_print(rop)
	struct memropc *rop;
{

	printf("ROP CONTROL: dest=%x, src1=%x, src2=%x, pat=%x,\n",
	   rop->mrc_dest, rop->mrc_source1, rop->mrc_source2, rop->mrc_pattern);
	printf("mask1=%x, mask2=%x, shift=%x, op=%x\n",
	   rop->mrc_mask1, rop->mrc_mask2, rop->mrc_shift, rop->mrc_op);
	printf("width=%x, opcnt=%x, decout=%x, 11=%x,\n",
	   rop->mrc_width, rop->mrc_opcount, rop->mrc_decoderout, rop->mrc_x11);
	printf("12=%x, 13=%x, 14=%x, 15=%x\n",
	    rop->mrc_x12, rop->mrc_x13, rop->mrc_x14, rop->mrc_x15);
}
#endif


cgtwoattach (md)
	register struct mb_device *md;
{
	register struct cg2fb *cg2fb =
	    (struct cg2fb *) cgtwoextra [md->md_unit].cg2x_ropaddr;

	cg2fb->intrptvec.reg = md->md_intr->v_vec;
}


/*
 * Only allow opens for writing or reading and writing
 * because reading is nonsensical.
 */
cgtwoopen(dev, flag)
	dev_t dev;
	int flag;
{
	return (fbopen(dev, flag, NCGTWO, cgtwoinfo));
}


/*
 * When close driver destroy pixrect.
 */
/*ARGSUSED*/
cgtwoclose(dev, flag)
	dev_t dev;
{
	register int unit = minor(dev);

	if ((caddr_t)&cgtwoprdata[unit] == cgtwopixrect[unit].pr_data) {
		bzero((caddr_t)&cgtwoprdata[unit], sizeof (struct cg2pr));
		bzero((caddr_t)&cgtwopixrect[unit], sizeof (struct pixrect));
	}
}


/*ARGSUSED*/
cgtwoioctl(dev, cmd, data, flag)
	dev_t dev;
	caddr_t data;
{
	register int unit = minor(dev);
	register struct cg2fb *cg2fb =
	    (struct cg2fb *)cgtwoextra[unit].cg2x_ropaddr;

	switch (cmd) {

	case FBIOGTYPE: {
		register struct fbtype *fb = (struct fbtype *)data;

		fb->fb_type = FBTYPE_SUN2COLOR;
		fb->fb_height = cgtwoextra[unit].cg2x_hwheight;
		fb->fb_width = cgtwoextra[unit].cg2x_hwwidth;
		fb->fb_depth = CG2_DEPTH;
		fb->fb_cmsize = 256;
		fb->fb_size = CG2TOTALSIZE;
		}
		break;

	case FBIOGATTR: {
		register struct fbgattr *gattr = (struct fbgattr *) data;

		(void) cgtwo_get_attr(unit, gattr);
		}
		break;

	case FBIOSATTR: {
		register struct fbsattr *sattr = (struct fbsattr *) data;

		if (sattr->flags & FB_ATTR_DEVSPECIFIC) 
			bcopy((char *) sattr->dev_specific,
				(char *) cgtwoextra[unit].dev_specific,
				sizeof sattr->dev_specific);
		}
		break;

	case FBIOGPIXRECT: {
		register struct fbpixrect *fbpr = (struct fbpixrect *)data;

		/*
		 * "Allocate" and initialize pixrect data with default.
		 */
		fbpr->fbpr_pixrect = &cgtwopixrect[unit];
		cgtwopixrect[unit] = cgtwopixrectdefault;
		fbpr->fbpr_pixrect->pr_height = cgtwoextra[unit].cg2x_hwheight;
		fbpr->fbpr_pixrect->pr_width = cgtwoextra[unit].cg2x_hwwidth;
		fbpr->fbpr_pixrect->pr_data = (caddr_t) &cgtwoprdata[unit];
		cgtwoprdata[unit] = cgtwoprdatadefault;
		/*
		 * Fixup pixrect data.
		 */
		cgtwoprdata[unit].cgpr_va = cg2fb;
		/*
		 * Enable video
		 */
		cg2fb->status.reg.video_enab = 1;
		}
		break;

	case FBIOGINFO: {
		register struct fbinfo *fbinfo = (struct fbinfo *)data;

		/*
		 * hands down color board info to be passed to the GP
		 */
		fbinfo->fb_physaddr = cgtwoextra[unit].cg2x_physaddr;
		fbinfo->fb_hwwidth  = cgtwoextra[unit].cg2x_hwwidth;
		fbinfo->fb_hwheight = cgtwoextra[unit].cg2x_hwheight;
		fbinfo->fb_ropaddr  = cgtwoextra[unit].cg2x_ropaddr;
		}
		break;

	/* set video flags */
	case FBIOSVIDEO: {
		register int *video = (int *) data;

		cg2fb->status.reg.video_enab = *video & FBVIDEO_ON ? 1 : 0;
		}
		break;

	/* get video flags */
	case FBIOGVIDEO: {
		* (int *) data = cg2fb->status.reg.video_enab
			? FBVIDEO_ON : FBVIDEO_OFF;
		}
		break;

	case FBIOVERTICAL: {
		(void) cgtwo_vertical_retrace(cg2fb, unit);
		}
		break;

	default:
		return (ENOTTY);
	}
	return (0);
}


cgtwointr(unit)
	register int unit;
{
	register struct cg2fb *cg2fb;

	cg2fb = (struct cg2fb *) cgtwoextra[unit].cg2x_ropaddr;
	cg2fb->status.reg.inten = 0;	/* clear pending interrupt. */
	wakeup((caddr_t) &(cgtwoextra[unit]));
#ifdef lint
	cgtwointr(unit);
#endif
}


cgtwo_gattr(pr, fbgattr)
	struct pixrect *pr;
	struct fbgattr *fbgattr;
{
	register int unit = minor(cg2_d(pr)->cgpr_fd);
	
	return cgtwo_get_attr(unit, fbgattr);
}


cgtwo_get_attr(unit, gattr)
	register int unit;
	register struct fbgattr *gattr;
{
	gattr->real_type = FBTYPE_SUN2COLOR;
	gattr->owner = 0;
	gattr->fbtype.fb_type = FBTYPE_SUN2COLOR;
	gattr->fbtype.fb_height = cgtwoextra[unit].cg2x_hwheight;
	gattr->fbtype.fb_width = cgtwoextra[unit].cg2x_hwwidth;
	gattr->fbtype.fb_depth = CG2_DEPTH;
	gattr->fbtype.fb_cmsize = 256;
	gattr->fbtype.fb_size = CG2TOTALSIZE;
	gattr->sattr.flags = FB_ATTR_DEVSPECIFIC;
	gattr->sattr.emu_type = -1;
	bcopy((char *) cgtwoextra[unit].dev_specific,
		(char *) gattr->sattr.dev_specific,
		sizeof gattr->sattr.dev_specific);
	gattr->emu_types[0] = -1;
	return 0;
}


cgtwo_vertical(pr)
	struct pixrect *pr;
{
	register int unit = minor(cg2_d(pr)->cgpr_fd);
	register struct cg2fb *cg2fb =
	    (struct cg2fb *)cgtwoextra[unit].cg2x_ropaddr;

	return cgtwo_vertical_retrace(cg2fb, unit);
}


int cgtwo_sleep = 1;	/* For debugging purposes only in 3.4 Beta phase. */


cgtwo_vertical_retrace(cg2fb, unit)
	register struct cg2fb *cg2fb;
	register int unit;
{
	int priority_level;
	struct cg3fb *cg3fb = (struct cg3fb *) cg2fb;

	if (cgtwo_sleep) {
		priority_level = splx(pritospl(cgtwoinfo[unit]->md_intpri));
		cg2fb->status.reg.inten = 1;
		(void) sleep((caddr_t) &(cgtwoextra[unit]), PZERO - 1);
		(void) splx(priority_level);
	} else {	/* For debugging purposes only in 3.4 Beta phase. */
		cg3fb->dblbuf.word |= 0x200;
		while (cg3fb->dblbuf.word & 0x200)
			;
	}
	return 0;
}


/*ARGSUSED*/
cgtwommap(dev, off, prot)
	dev_t dev;
	off_t off;
	int prot;
{
	register int unit = minor(dev);

	if (off >= CG2EXTRAOFF) {
		caddr_t addrsaved = cgtwoinfo[unit]->md_addr;
		int	result;

		cgtwoinfo[unit]->md_addr=(caddr_t)cgtwoextra[unit].cg2x_ropaddr;
		result = fbmmap(dev, off-CG2EXTRAOFF, prot, NCGTWO, cgtwoinfo,
		    CG2EXTRASIZE);
		cgtwoinfo[unit]->md_addr = addrsaved;
		return (result);
	} else {
		int page;

		/*
		 * We can't call fbmmap here since only the 1st page of
	 	 * the frame buffer is in virtual memory
		 */
		page = getkpgmap(cgtwoinfo[unit]->md_addr) & PG_PFNUM;
		page += btop(off);
		return (page);
	}
}

#endif NCGTWO > 0
