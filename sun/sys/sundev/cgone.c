#ifndef lint
static  char sccsid[] = "@(#)cgone.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include "cgone.h"
#include "win.h"
#if NCGONE > 0

/*
 * Sun One Color Graphics Board(s) Driver
 */

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

#include "../machine/mmu.h"
#include "../machine/pte.h"

#include "../sun/fbio.h"

#include "../sundev/mbvar.h"
#include "../pixrect/pixrect.h"
#include "../pixrect/pr_util.h"
#include "../pixrect/cg1reg.h"
#include "../pixrect/cg1var.h"

#if NWIN > 0
#define	CG1_OPS	&cg1_ops
struct	pixrectops cg1_ops = {
	cg1_rop,
	cg1_putcolormap,
	cg1_putattributes,
};
#else
#define	CG1_OPS	(struct pixrectops *)0
#endif

#define CG1SIZE	(sizeof (struct cg1fb))
struct	cg1pr cgoneprdatadefault =
    { 0, 0, 255, 0, 0 };
struct	pixrect cgonepixrectdefault =
    { CG1_OPS, { CG1_WIDTH, CG1_HEIGHT }, CG1_DEPTH, /* filled in later */ 0 };


/*
 * Driver information for auto-configuration stuff.
 */
int	cgoneprobe(), cgoneintr();
struct	pixrect cgonepixrect[NCGONE];
struct	cg1pr cgoneprdata[NCGONE];
struct	mb_device *cgoneinfo[NCGONE];
struct	mb_driver cgonedriver = {
	cgoneprobe, 0, 0, 0, 0, cgoneintr,
	CG1SIZE, "cgone", cgoneinfo, 0, 0, 0,
};

/*
 * Only allow opens for writing or reading and writing
 * because reading is nonsensical.
 */
cgoneopen(dev, flag)
	dev_t dev;
{
	return(fbopen(dev, flag, NCGONE, cgoneinfo));
}

/*
 * When close driver destroy pixrect.
 */
/*ARGSUSED*/
cgoneclose(dev, flag)
	dev_t dev;
{
	register int unit = minor(dev);

	if ((caddr_t)&cgoneprdata[unit] == cgonepixrect[unit].pr_data) {
		bzero((caddr_t)&cgoneprdata[unit], sizeof (struct cg1pr));
		bzero((caddr_t)&cgonepixrect[unit], sizeof (struct pixrect));
	}
}

/*ARGSUSED*/
cgoneioctl(dev, cmd, data, flag)
	dev_t dev;
	caddr_t data;
{
	register int unit = minor(dev);

	switch (cmd) {

	case FBIOGTYPE: {
		register struct fbtype *fb = (struct fbtype *)data;

		fb->fb_type = FBTYPE_SUN1COLOR;
		fb->fb_height = 480;
		fb->fb_width = 640;
		fb->fb_depth = 8;
		fb->fb_cmsize = 256;
		fb->fb_size = 512*640;
		break;
		}
	case FBIOGPIXRECT: {
		register struct fbpixrect *fbpr = (struct fbpixrect *)data;
		register struct cg1fb *cg1fb =
		    (struct cg1fb *)cgoneinfo[(unit)]->md_addr;

		/*
		 * "Allocate" and initialize pixrect data with default.
		 */
		fbpr->fbpr_pixrect = &cgonepixrect[unit];
		cgonepixrect[unit] = cgonepixrectdefault;
		fbpr->fbpr_pixrect->pr_data = (caddr_t) &cgoneprdata[unit];
		cgoneprdata[unit] = cgoneprdatadefault;
		/*
		 * Fixup pixrect data.
		 */
		cgoneprdata[unit].cgpr_va = cg1fb;
		/*
		 * Enable video
		 */
		cg1_setcontrol(cg1fb, CG_VIDEOENABLE);
		/*
		 * Clear interrupt
		 */
		cg1_intclear(cg1fb);
		break;
		}

	/* set video flags */
	case FBIOSVIDEO: {
		register int *video = (int *) data;

		if (*video & FBVIDEO_ON) 
			((struct cg1fb *) cgoneinfo[unit]->md_addr)
				->update[0].regsel.reg[CG_STATUS].dat[0] 
				|= CG_VIDEOENABLE;
		else
			((struct cg1fb *) cgoneinfo[unit]->md_addr)
				->update[0].regsel.reg[CG_STATUS].dat[0] 
				&= ~(CG_VIDEOENABLE);
	}
	break;

	/* get video flags */
	case FBIOGVIDEO: {
		* (int *) data = 
			((struct cg1fb *) cgoneinfo[unit]->md_addr)
				->update[0].regsel.reg[CG_STATUS].dat[0]
			& CG_VIDEOENABLE
			? FBVIDEO_ON : FBVIDEO_OFF;
	}
	break;

	default:
		return (ENOTTY);
	}
	return (0);
}

/*
 * We need to handle vertical retrace interrupts here.
 * The color map(s) can only be loaded during vertical 
 * retrace; we should put in ioctls for this to synchronize
 * with the interrupts.
 * FOR NOW, see comments in the code.
 */
cgoneintclear(cg1fb)
	struct	cg1fb *cg1fb;
{
	/*
	 * The Sun 1 color frame buffer doesn't indicate that an
	 * interrupt is pending on itself so we don't know if the interrupt
	 * is for our device.  So, just turn off interrupts on the cgone board.
	 * This routine can be called from any level.
	 */
	cg1_intclear(cg1fb);
	/*
	 * We return 0 so that if the interrupt is for some other device
	 * then that device will have a chance at it.
	 */
	return(0);
}

int
cgoneintr()
{
	return(fbintr(NCGONE, cgoneinfo, cgoneintclear));
}

/*ARGSUSED*/
cgonemmap(dev, off, prot)
	dev_t dev;
	off_t off;
	int prot;
{
	return(fbmmap(dev, off, prot, NCGONE, cgoneinfo, CG1SIZE));
}

#include "../sundev/cgreg.h"
	/*
	 * Note: using old cgreg.h to peek and poke for now.
	 */
/*
 * We determine that the thing we're addressing is a color
 * board by setting it up to invert the bits we write and then writing
 * and reading back DATA1, making sure to deal with FIFOs going and coming.
 */
#define DATA1 0x5C
#define DATA2 0x33
/*ARGSUSED*/
cgoneprobe(reg, unit)
	caddr_t reg;
	int	unit;
{
	register caddr_t CGXBase;
	register u_char *xaddr, *yaddr;

	CGXBase = reg;
	if (pokec((caddr_t)GR_freg, GR_copy_invert))
		return (0);
	if (pokec((caddr_t)GR_mask, 0))
		return (0);
	xaddr = (u_char *)(CGXBase + GR_x_select + GR_update + GR_set0);
	yaddr = (u_char *)(CGXBase + GR_y_select + GR_set0);
	if (pokec((caddr_t)yaddr, 0))
		return (0);
	if (pokec((caddr_t)xaddr, DATA1))
		return (0);
	(void) peekc((caddr_t)xaddr);
	(void) pokec((caddr_t)xaddr, DATA2);
	if (peekc((caddr_t)xaddr) == (~DATA1 & 0xFF)) {
		/*
		 * The Sun 1 color frame buffer doesn't indicate that an
		 * interrupt is pending on itself.
		 * Also, the interrupt level is user program changable.
		 * Thus, the kernel never knows what level to expect an
		 * interrupt on this device and doesn't know is an interrupt
		 * is pending.
		 * So, we add the cgoneintr routine to a list of interrupt
		 * handlers that are called if no one handles an interrupt.
		 * Add_default_intr screens out multiple calls with the same
		 * interrupt procedure.
		 */
		add_default_intr(cgoneintr);
		return (CG1SIZE);
	}
	return (0);
}

#endif
