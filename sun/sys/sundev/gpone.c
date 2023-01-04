#ifndef lint
static  char sccsid[] = "@(#)gpone.c 1.6 87/01/09 SMI";
#endif

/*
 * Copyright 1986 by Sun Microsystems, Inc.
 */

/*
 * Sun Graphics Processor 1 Driver
 *
 * General notes:
 *
 * This driver was originally written to accomodate a GP/CGTWO combintation.
 * Theoretically, the GP can work in conjunction with another frame buffer
 * type.  This would involve new pixrect calls and switch constructs in this
 * driver to do the appropriate thing depending on the device.
 *
 * Extended to work with CG3 framebuffer, particularly double-buffering.
 * Included frame buffer interrupts and vertical retrace code.  Note that
 * CG3 is included in CG2 driver and is not a separate driver.
 *
 * This driver makes use of an open hack in ufs_vnodeops.c which allows the
 * minor device number to be altered from gponeopen().  This is used primarily
 * so that things can be cleaned up on the GP when a process goes away.
 * The altered minor device numbers are handed out on a per process basis.
 */

#include "gpone.h"
#if NGPONE > 0

#include "win.h"
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
#include "../machine/pte.h"
#include "../machine/psl.h"
#include "../sun/consdev.h"
#include "../sun/fbio.h"
#include "../sun/gpio.h"
#include "../sundev/mbvar.h"
#include "../pixrect/pixrect.h"
#include "../pixrect/pr_util.h"
#include "../pixrect/memreg.h"
#include "../pixrect/cg2reg.h"
#include "../pixrect/cg2var.h"
#include "../pixrect/gp1reg.h"
#include "../pixrect/gp1var.h"

#if NWIN > 0
#define	GP1_OPS	&gp1_ops
struct	pixrectops gp1_ops = {
	gp1_rop,
	gp1_putcolormap,
	gp1_putattributes,
};
#else
#define	GP1_OPS	((struct pixrectops *)0)
#endif


/* pixrect data structures */
struct	gp1pr gponeprdatadefault =
    {0/*cgpr_va set in ioctl*/, 0/*gp_shmem set in ioctl*/, 0, 255, 
	0, 0, 0, 0, 0, 0 };
struct	pixrect gponepixrectdefault =
    { &gp1_ops, { 0/* set in ioctl */, 0/* set in ioctl */ }, CG2_DEPTH,
	0/* set in ioctl */};


/* probe/attach related definitions */
#define GP1IDENTMASK	0xFE	/* mask for 0xEA(buffer) or 0xEB(no buffer) */
#define GPBUFPROBESIZE	52	/* size of microcode to stuff for gbuf probe */
#define GPBUFSTATUSMASK 0x0F	/* mask of status register for gbuf probe */
				/* only low 4 bits altered by GP after probe */
#define GPBUFPRESENT	1	/* value of status register if gbuf probe ok */

/* Bit definitions for gpx_flags in gponeextra. */
#define GP1X_GPBUF	1
    /* Asserted if GP Graphics buffer is present. */


/* GP size and count related definitions */
#define NFBGP		4		/* max num of fb bindings per GP */
#define NSTBLKGP	8		/* number of static blocks per GP */
#define GP1PROBESIZE	(NBPG)		/* bytes in a single page */
#define GP1SIZE		(VME_GP1SIZE)	/* size of entire GP board */
#define FBTOTALSIZE	(sizeof(struct cg2fb)+sizeof(struct cg2memfb))
#define GP1TOTALSIZE	(sizeof(struct cg2fb)+sizeof(struct cg2memfb)+	\
			VME_GP1SIZE)	/* total size of gp/fb (for pixrect) */


/* 
 * Minor device related definitions.
 *
 * The requested gp unit and fb unit are encoded in the minor device bits
 * The fb unit is encoded in the low order 2 bits.
 * The GP unit is encoded in the high order bits.
 *
 * Number of pseudo gpone devices is (256-16) (the first 16 minor device
 * numbers refer to the requested GP/fb encoded pair.  The remaining entries
 * are used to alter the minor device as part of a pseudo device hack to
 * allow us to notice when a process dies so we can clean things up on
 * the GP if neccessary).
 */
#define NGPREQMINDEV		16		/* requested minor devices */
#define NGPALTMINDEV		240 		/* altered minor devices */
#define GPUNIT(minordev)	(minordev >> 2) /* which GP */
#define FBUNIT(minordev)	(minordev & 3)  /* which fb */
#define NOREQMINDEV		0xFF		/* no requested minor device */

 
/*
 * Driver device specific defines and information structures.
 * Defines are similar to dev_specific[0] defines in cgtwo.c.
 * One softfb per fb.
 */

/* Bit definitions for dev_specific[0] in gp1_softfb. */
#define CG2X_CG3	1
    /* Asserted if Board is a CG3. */

struct gp1_softfb {
	int		fb_configured;	/* true if fb configured with this GP */
	unsigned char	*fb_ropaddr;	/* fb addr into kernelmap */
	int		fb_physaddr;	/* high byte of phys color board addr */
	int		fb_hwwidth;	/* fb width  (based on resolution) */
	int		fb_height;	/* fb height (based on resolution) */
	int		fb_off_low;	/* offset to start of fb board */
	int		fb_off_high;	/* offset to end of board */
	int		fb_flags;	/* CG3 buffer flags & vert. retrace */
	int		dev_specific[FB_ATTR_NDEVSPECIFIC];	/* catchall */
	    /* [0] is flag for cg2 or cg3. */
	    /* [1] is number of buffers. */
	    /* [0,1] same as cgtwo driver. */
};

struct stblkinfo {
	char	minordev;	     /* the minor device that owns the block */
	struct proc 	*process;    /* process that owns block */
};

struct	gponeextra {
	short		  gpx_flags;		    /* gbuf attached flag. */
	char		  gpx_gbunit;		    /* fb that owns the gb */
	caddr_t		  gpx_addr;		    /* addr of gp */
	caddr_t		  gpx_shmem;		    /* addr of gp shmem */
	int		  gpx_physaddr;		    /* phys addr of gp */
	struct gp1_softfb gpx_softfb[NFBGP]; 	    /* fb related information */
	char		  gpx_pixrect_owner[NFBGP]; /* mindev pixrect owner */
	struct stblkinfo  gpx_stblk_tbl[NSTBLKGP];  /* GP static block ledger */
	int		  gpx_restart_count; 	    /* rst cnt since power up */
};

struct gpone_mindevtbl {
	char minordev;		/* requested minor device (gp/fb pair) */
	int  close_pending	/* GP cleanup pending flag */
};

int	gp1_kern_sync();
int	gponeprobe();

/* For pr_dblbuf.c so kernel will build even if driver not configured in. */
int (*_pr_gp1_rop)() = gp1_rop;
int gpone_gattr(), (*_pr_gpone_gattr)() = gpone_gattr;
int gpone_vertical(), (*_pr_gpone_vertical)() = gpone_vertical;

struct	pixrect 	gponepixrect[NGPONE][NFBGP];	/* kernel pixrect*/
struct	gp1pr		gponeprdata[NGPONE][NFBGP];	/* pixrect priv data */
struct	mb_device	*gponeinfo[NGPONE];		/* mainbus info */
struct	gponeextra	gponeextra[NGPONE];		/* dev specific data */
struct  gpone_mindevtbl	gponedevtbl[NGPALTMINDEV]; 	/* true mindev tbl */


struct	mb_driver gponedriver = {
	gponeprobe, 0, 0, 0, 0, 0,
	GP1PROBESIZE, "gpone", gponeinfo, 0, 0, 0,
};


/* 
 * Microcode to probe for graphics buffer to see if it exists.
 * 
 * #define TestVal	0x5432
 * 
 * gbprobe: movw 0, y;		am->stlreg;	cjp, ~zbr .;			| 
 * 	mov2nw,s 3, acc;	0x6000->zbhiptr; ;				| 
 * pause1:	sub2nw,s 0, acc;	;		cjp, ~neg pause1;		| 
 * 	incw 0, y;		am->zbhiptr;	cjp, ~zbr .;			| 
 * 	movw d, r[1];		0x2345->am;	;				| 
 * 	movw r[1], y;		am->zbloptr;	cjp, ~zbr .;			| 
 * 	movw d, r[0];		TestVal->am;	;				| 
 * 	movw r[0], y;		am->zbwdreg;	cjp, ~zbr .;			| 
 * 	incw 0, y;		am->zbhiptr;	cjp, ~zbr .;			| 
 * 	movw r[1], y;		am->zbloptr;	cjp, ~zbr .;		zbrd	| 
 * 	subw,s d, r[0], y;	zbrdreg->am;	cjp, ~zbr .;			| 
 * 	;			;		cjp, ~zer .;			| 
 * 	incw 0, y;		am->stlreg;	cjp, go .;			| Set status register to 1.  Loop indefinitely.
 * 
 */
short gbufucode[GPBUFPROBESIZE] = {
	0x0028, 0x23e0, 0xf900, 0x0000,
	0x000d, 0x6e00, 0xe786, 0x6000,
	0x0008, 0x0390, 0xe185, 0x0002,
	0x0029, 0x63e0, 0xfd00, 0x0003,
	0x002c, 0x9e00, 0xd8c1, 0x2345,
	0x0029, 0x73e0, 0xd841, 0x0005,
	0x002c, 0x9e00, 0xd8c0, 0x5432,
	0x0028, 0xb3e0, 0xd840, 0x0007,
	0x0029, 0x63e0, 0xfd00, 0x0008,
	0x0029, 0x73e5, 0xd841, 0x0009, 
	0x000a, 0x13e0, 0x9600, 0x000a, 
	0x0028, 0x0380, 0x7140, 0x000b, 
	0x00a8, 0x2300, 0xfd00, 0x000c
};


/*
 * We determine that the thing we're addressing is a gpone
 * board by accessing the ident register.
 * Only GP1PROBESIZE is mapped when this is called.
 */
gponeprobe(reg, unit)
	caddr_t reg;
	int	unit;
{
        int a = 0;
	unsigned int page;
	short *gpptr, *gpucodeptr;
 
        /*
         * Get physical page number and type associated with first page of gp.
         */
        page = getkpgmap(reg) & PG_PFNUM;
        /*
         * Allocate virtual memory for part of device we will access.
         * (Following sequence taken from ../machine/autoconf.c doprobe).
         */
        if ((a = (int)rmalloc(kernelmap, (long)(btoc(GP1SIZE)))) == 0)
                panic("out of kernelmap for gp1");
        gponeextra[unit].gpx_addr = (caddr_t)kmxtob(a);
        gponeextra[unit].gpx_shmem = (caddr_t)kmxtob(a) + GP1_SHMEM_OFFSET;
	gpptr = (short *) kmxtob(a);

        /*
         * Mapin virtual memory that kernel will use.
         */
        mapin(&Usrptmap[a], btop(gponeextra[unit].gpx_addr),
            page, (int)btoc(GP1SIZE), PG_V|PG_KW);
        /*  
         * Actually do probe:
	 * Check the ident register on the GP board for correct pattern: 
	 * 0xEB or 0xEA which indicate that this is is a GP with or without a
	 * graphics buffer explicitly
         */
	if ((unsigned char)(peek((short *)gponeextra[unit].gpx_addr) & 
		GP1IDENTMASK) == 0xEA) { /* true if 0xEA or 0xEB */

		int i;

		/* determine if there is an attached gbuffer board */

		/* initialize with no gbuffer */
		gponeextra[unit].gpx_flags &= ~GP1X_GPBUF;
		gponeextra[unit].gpx_gbunit = -1;

		/* hardware reset */
		(void)poke(&gpptr[GP1_CONTROL_REG],
			GP1_CR_CLRIF | GP1_CR_INT_DISABLE | GP1_CR_RESET);
		(void)poke(&gpptr[GP1_CONTROL_REG], 0);

		/* load priming the pipe microcode*/
		/* org 0; ; cjp,go .; ; | jump to zero instruction */
		(void)poke(&gpptr[GP1_UCODE_ADDR_REG], 0);
		(void)poke(&gpptr[GP1_UCODE_DATA_REG], 0x0068);
		(void)poke(&gpptr[GP1_UCODE_DATA_REG], 0x0000);
		(void)poke(&gpptr[GP1_UCODE_DATA_REG], 0x7140);
		(void)poke(&gpptr[GP1_UCODE_DATA_REG], 0x0000);

		/* start the processors */
		(void)poke(&gpptr[GP1_CONTROL_REG], 0);
		(void)poke(&gpptr[GP1_CONTROL_REG], GP1_CR_VP_STRT0
		    | GP1_CR_VP_CONT | GP1_CR_PP_STRT0 | GP1_CR_PP_CONT);

		/* hardware reset */
		(void)poke(&gpptr[GP1_CONTROL_REG],
			GP1_CR_CLRIF | GP1_CR_INT_DISABLE | GP1_CR_RESET);
		(void)poke(&gpptr[GP1_CONTROL_REG], 
			0);

		/* load gbuffer probe microcode */
		(void)poke(&gpptr[GP1_UCODE_ADDR_REG],  0);
		gpucodeptr = gbufucode;
		for (i=0; i<GPBUFPROBESIZE; i++)
			(void)poke(&gpptr[GP1_UCODE_DATA_REG], *(gpucodeptr)++);
		
		/* start the processors */
		(void)poke(&gpptr[GP1_CONTROL_REG], 0);
		(void)poke(&gpptr[GP1_CONTROL_REG], GP1_CR_PP_STRT0
		    | GP1_CR_PP_CONT);

		/* let gp run for a hundreth of a second */
		DELAY(10000);

		/* check the status flag to see if gbuffer present */
		if ((peek(&gpptr[GP1_STATUS_REG]) & GPBUFSTATUSMASK) == 
			GPBUFPRESENT) {
			gponeextra[unit].gpx_flags |= GP1X_GPBUF;
		}

		/* save gp vme address and initially non-configure fb bds */
		gponeextra[unit].gpx_physaddr =
			(int)( (page << PGSHIFT)  & 0xFFFFFF);
		/* mark all the fb boards as unconfigured */
		/* initialize the owner of the pixrect data to 0 */
		for (i=0; i<NFBGP; i++) {
			gponeextra[unit].gpx_softfb[i].fb_configured = 0;
			gponeextra[unit].gpx_pixrect_owner[i] = 0;
			/* Clear out catchall array. */
			bzero((char *) gponeextra[unit].gpx_softfb[i].
			    dev_specific, 
			    sizeof gponeextra[unit].gpx_softfb[i].dev_specific);
		}

		/* initially no process owns any static blocks on the GP */
		for (i=0; i<NSTBLKGP; i++) {
			gponeextra[unit].gpx_stblk_tbl[i].minordev=NOREQMINDEV;
			gponeextra[unit].gpx_stblk_tbl[i].process = 0;
		}

		/* initialize the restart count */
		gponeextra[unit].gpx_restart_count = 0;

		/* initialize device table */
		/* this will initialize each time probe is invoked */
		/*  ... but who cares */
		for (i=0; i<NGPALTMINDEV; i++) {
			gponedevtbl[i].minordev = NOREQMINDEV;
			gponedevtbl[i].close_pending = 0;
		}
		return (GP1PROBESIZE);
	} else {
		/* probe failure drops through here */
		mapout(&Usrptmap[a], (int)btoc(GP1SIZE));
		rmfree(kernelmap, (long)(btoc(GP1SIZE)), (long)a);
		return( 0);
	}
} /* end of gponeprobe() */


/*
 * Only allow opens for writing or reading and writing
 * because reading is nonsensical.
 */
gponeopen(dev, flag, truedev)
	dev_t dev;
	dev_t *truedev;
{
	register int gpunit, fbunit;

	int fbopen_status;
	int i;
	dev_t fbdevice;

	if (minor(dev) >= NGPREQMINDEV) {
		/* this is a pseudo minor device already */
		gpunit=GPUNIT((gponedevtbl[minor(dev)-NGPREQMINDEV].minordev));
		fbunit=FBUNIT((gponedevtbl[minor(dev)-NGPREQMINDEV].minordev));
		fbdevice = makedev(major(dev),
		    gponedevtbl[minor(dev)-NGPREQMINDEV].minordev);
	} else {
		/* this is not a pseudo minor device already */
		gpunit = GPUNIT(minor(dev));
		fbunit = FBUNIT(minor(dev));
		fbdevice = dev;
	}

	/* check the gp itself */
	if (fbopen_status = fbopen(fbdevice & 0xFFFC, flag, NGPONE, gponeinfo))
		return (fbopen_status);

	/*
	 * Use the O_NDELAY flag if we don't care about the fb device (as
	 * is the case with gpconfig).
	 * We won't be using the altered minor device open hack for this case.
	 */
	if (!(flag & O_NDELAY))
		/* check the associated fb board */
		if (!(gponeextra[gpunit].gpx_softfb[fbunit].fb_configured))
			return (ENXIO);

	/* If we already have the altered minor device then return nicely */
	if (minor(dev) >= NGPREQMINDEV)
		return(0);

	/* Get a new table spot for this device */
	for (i=0; i<NGPALTMINDEV; i++) {
		if (gponedevtbl[i].minordev == (char) NOREQMINDEV)
			break;
	}
	if (i != NGPALTMINDEV)
		gponedevtbl[i].minordev = minor(dev); /* found a tbl entry */
	else
		return(ENXIO); /* did not find a tbl entry ... error return */

	/* Use the table index + NGPREQMINDEV as true minor device number */
	/* ( + NGPREQMINDEV) since we don't want to touch real minor devices */
	*truedev = makedev(major(dev), (i + NGPREQMINDEV));

	return (0);
} /* end of gponeopen() */

/*ARGSUSED*/
gponeclose(dev, flag)
	dev_t dev;
{
	register int gpunit =
	    GPUNIT(gponedevtbl[minor(dev)-NGPREQMINDEV].minordev);
	register int fbunit =
	    FBUNIT(gponedevtbl[minor(dev)-NGPREQMINDEV].minordev);
	register int i;

	/*
	 * Mark the altered minor device to be closed.
	 * Can't close immediately because of gp syncing problems.
	 */
	gponedevtbl[minor(dev)-NGPREQMINDEV].close_pending  = 1;

	/* If process used any static blocks ... free them */
	for (i=0; i<NSTBLKGP; i++) {
		if (gponeextra[gpunit].gpx_stblk_tbl[i].minordev == minor(dev)){
			gponeextra[gpunit].gpx_stblk_tbl[i].minordev = 
			    NOREQMINDEV;
			gponeextra[gpunit].gpx_stblk_tbl[i].process = 0;
		}
	}

	/* Check the gp allocation semaphore */
	if (!((*((char *)(gponeextra[gpunit].gpx_shmem)+5)) & 0x80)) {
		/* We can talk to the GP */
		if ( !gp1_kern_sync(gponeextra[gpunit].gpx_shmem) ) {
			/* Sync succeeded ... cleanup pending closes */
			(void) gpone_close_cleanup(gpunit);
		}
	}

	/* Remove the pixrect stuff if kernel brought us here */
        if (((caddr_t)&(gponeprdata[gpunit][fbunit]) == 
	            gponepixrect[gpunit][fbunit].pr_data) &&
		    gponeextra[gpunit].gpx_pixrect_owner[fbunit] == minor(dev)){
                bzero((caddr_t)&(gponeprdata[gpunit][fbunit]),
		    sizeof (struct gp1pr));
                bzero((caddr_t)&(gponepixrect[gpunit][fbunit]),
		    sizeof (struct pixrect));
	}
	return(0);
} /* end of gponeclose() */

/*ARGSUSED*/
gponeioctl(dev, cmd, data, flag)
	dev_t dev;
	caddr_t data;
{
	register int gpunit =
	    GPUNIT(gponedevtbl[minor(dev)-NGPREQMINDEV].minordev);
	register int fbunit =
	    FBUNIT(gponedevtbl[minor(dev)-NGPREQMINDEV].minordev);

	switch (cmd) {

	case FBIOGTYPE: {
		register struct fbtype *fb = (struct fbtype *)data;

		fb->fb_type   = FBTYPE_SUN2GP;
		fb->fb_height = gponeextra[gpunit].gpx_softfb[fbunit].fb_height;
		fb->fb_width = gponeextra[gpunit].gpx_softfb[fbunit].fb_hwwidth;
		fb->fb_depth  = CG2_DEPTH;
		fb->fb_cmsize = 256;
		fb->fb_size   = GP1TOTALSIZE;
		}
		break;

	case FBIOGATTR: {
		register struct fbgattr *gattr = (struct fbgattr *) data;

		(void) gpone_get_attr(gpunit, fbunit, gattr);
		}
		break;

	case FBIOSATTR: {
		register struct fbsattr *sattr = (struct fbsattr *) data;

		if (sattr->flags & FB_ATTR_DEVSPECIFIC) 
			bcopy((char *) sattr->dev_specific,
				(char *) gponeextra[gpunit].gpx_softfb[fbunit].
				    dev_specific,
				sizeof sattr->dev_specific);
		}
		break;

	case GP1IO_PUT_INFO: {
		register struct fbinfo *gp1fbinfo = (struct fbinfo *)data;

		/* Set up information that the GP needs of the color board */
		fbunit = gp1fbinfo->fb_unit;
		gponeextra[gpunit].gpx_softfb[fbunit].fb_physaddr = 
			gp1fbinfo->fb_physaddr;
		gponeextra[gpunit].gpx_softfb[fbunit].fb_hwwidth = 
			gp1fbinfo->fb_hwwidth;
		gponeextra[gpunit].gpx_softfb[fbunit].fb_height = 
			gp1fbinfo->fb_hwheight;
		gponeextra[gpunit].gpx_softfb[fbunit].fb_off_low = 
			gp1fbinfo->fb_physaddr -
			gponeextra[gpunit].gpx_physaddr;
		gponeextra[gpunit].gpx_softfb[fbunit].fb_off_high = 
			gponeextra[gpunit].gpx_softfb[fbunit].fb_off_low + 
			FBTOTALSIZE;
		gponeextra[gpunit].gpx_softfb[fbunit].fb_configured = 1;
		gponeextra[gpunit].gpx_softfb[fbunit].fb_ropaddr = 
			gp1fbinfo->fb_ropaddr;
	}
	break;

	case FBIOGINFO: {
		register struct fbinfo *fbinfo = (struct fbinfo *)data;

		/* Return phys addr, phys addr difference and fb unit number */
		fbinfo->fb_addrdelta = 
			gponeextra[gpunit].gpx_softfb[fbunit].fb_off_low;
		fbinfo->fb_physaddr = 
			gponeextra[gpunit].gpx_physaddr;
		fbinfo->fb_unit = fbunit;
	}
	break;

	case FBIOVERTICAL: {
		register struct cg2fb *cg2fb = (struct cg2fb *)
		    (gponeextra[gpunit].gpx_softfb [fbunit]).fb_ropaddr;

		(void) gpone_vertical_retrace(cg2fb, gpunit, fbunit);
	    }
	    break;

        case FBIOGPIXRECT: {
                register struct fbpixrect *fbpr = (struct fbpixrect *)data;

                /*
                 * "Allocate" and initialize pixrect data with defaults.
                 */
                fbpr->fbpr_pixrect = &(gponepixrect[gpunit][fbunit]);
                gponepixrect[gpunit][fbunit] = gponepixrectdefault;
                fbpr->fbpr_pixrect->pr_height = 
			(gponeextra[gpunit].gpx_softfb[fbunit]).fb_height;
                fbpr->fbpr_pixrect->pr_width = 
			(gponeextra[gpunit].gpx_softfb[fbunit]).fb_hwwidth;
                fbpr->fbpr_pixrect->pr_data = 
			(caddr_t) &(gponeprdata[gpunit][fbunit]);
                gponeprdata[gpunit][fbunit] = gponeprdatadefault;

		/*
		 * Note the minor device that grabbed the pixrect
		 */
		gponeextra[gpunit].gpx_pixrect_owner[fbunit] = minor(dev);

                /*
                 * Fixup pixrect data.
                 */
                gponeprdata[gpunit][fbunit].gp_shmem = 
			gponeextra[gpunit].gpx_addr + GP1_SHMEM_OFFSET;
                gponeprdata[gpunit][fbunit].cgpr_va = (struct cg2fb *)
			(gponeextra[gpunit].gpx_softfb[fbunit].fb_ropaddr);
                gponeprdata[gpunit][fbunit].cg2_index = fbunit;
	}
	break;

	/* set video flags */
	case FBIOSVIDEO: {
		register int *video = (int *) data;

		((struct cg2fb *) 
			gponeextra[gpunit].gpx_softfb[fbunit].fb_ropaddr)
			->status.reg.video_enab = *video & FBVIDEO_ON ? 1 : 0;
	}
	break;

	/* get video flags */
	case FBIOGVIDEO: {
		* (int *) data = ((struct cg2fb *) 
			gponeextra[gpunit].gpx_softfb[fbunit].fb_ropaddr)
			->status.reg.video_enab ? FBVIDEO_ON : FBVIDEO_OFF;
	}
	break;

	case GP1IO_GET_STATIC_BLOCK: {
		register int *static_blk = (int *)data;
		register int i;
		short *start, *end, *gpptr; 

		for (i=0; i<NSTBLKGP; i++) {
			if (gponeextra[gpunit].gpx_stblk_tbl[i].minordev == 
				(char) NOREQMINDEV)
				break;
		}
		if (i != NSTBLKGP) { /* found a free static block */
			*static_blk = i;
			gponeextra[gpunit].gpx_stblk_tbl[i].minordev=minor(dev);
			gponeextra[gpunit].gpx_stblk_tbl[i].process = u.u_procp;

			/* Zero out block before handing it out */
			start = (short *)(gponeextra[gpunit].gpx_shmem) +
				512*(24+i);
			end = start + 511;
			for (gpptr=start; gpptr<=end; gpptr++)
				*gpptr = 0;
		} else
			*static_blk = -1; /* no free static block */
	}
	break;

	case GP1IO_FREE_STATIC_BLOCK: {
		register int *static_block = (int *)data;

		/* Check for argument is in range */
		if (*static_block < 0 || *static_block > 7)
			return(EINVAL);

		/* Check for already freed */
		if (gponeextra[gpunit].gpx_stblk_tbl[*static_block].minordev == 
			(char) NOREQMINDEV)
			return(EINVAL);

		/* Check to see if this is the owner */
		if (gponeextra[gpunit].gpx_stblk_tbl[*static_block].minordev !=
		    minor(dev))
			return(EINVAL);

		/* Looks okay ... do it */
		gponeextra[gpunit].gpx_stblk_tbl[*static_block].minordev =
		    NOREQMINDEV;
		gponeextra[gpunit].gpx_stblk_tbl[*static_block].process = 0;

	}
	break;

	case GP1IO_CHK_FOR_GBUFFER: {
		register int *gbuffer_state = (int *)data;

		*gbuffer_state = gponeextra[gpunit].gpx_flags & GP1X_GPBUF;
	}
	break;

	case GP1IO_SET_USING_GBUFFER: {
		register int *using = (int *)data;

		if ((gponeextra[gpunit].gpx_flags & GP1X_GPBUF) != 0)
			gponeextra[gpunit].gpx_gbunit = *using;
		else
			return (EINVAL);
	}
	break;

	case GP1IO_GET_GBUFFER_STATE: {
		register int *gbuffer_state = (int *)data;

		/* Get state flag which was set in probe */
		if (gponeextra[gpunit].gpx_gbunit == fbunit)
			*gbuffer_state = 1;
		else
			*gbuffer_state = 0;
	}
	break;

	case GP1IO_CHK_GP: {
		register int *chk_pending_close_flag = (int *)data;

		/*
		 * Check for pending closes first.
		 * If we did free something then return without restarting
		 */
		if (*chk_pending_close_flag) {
			if (gpone_close_cleanup(gpunit))
				return(0);
		}

		/* restart the GP */
		gpone_restart(gpunit, gponeextra[gpunit].gpx_addr);
	}
	break;

	case GP1IO_GET_RESTART_COUNT: {
		register int *restart_count = (int *)data;

		*restart_count = gponeextra[gpunit].gpx_restart_count;

	}
	break;

	case GP1IO_REDIRECT_DEVFB: {
		register int *fb_unit = (int *)data;

		/* Redirect /dev/fb to vector to specified device*/
		fbdev = (dev_t)((major(dev) << 8) | (gpunit << 2) | *fb_unit);
	}
	break;

	case GP1IO_GET_REQDEV: {
		register dev_t *devtype = (dev_t *)data;

		/* Return the requested minor device */
		*devtype = major(dev)<<8 |
		    gponedevtbl[minor(dev)-NGPREQMINDEV].minordev;
	}
	break;

	case GP1IO_GET_TRUMINORDEV: {
		register char *minordev = (char *)data;

		/* Return the altered minor device */
		*minordev = minor(dev);
	}
	break;

	default:
		return (ENOTTY);

	} /* end of switch */

	return (0);
} /* end of gponeioctl() */


gponeintr(gpunit)
	register int gpunit;
{
register struct cg2fb *cg2fb;
register int fbunit;
	for (fbunit = 0; fbunit < NFBGP; fbunit++) {
		if ((gponeextra[gpunit].gpx_softfb[fbunit]).fb_ropaddr) {
			cg2fb = (struct cg2fb *)
			    (gponeextra[gpunit].gpx_softfb[fbunit]).fb_ropaddr;
			if (cg2fb->status.reg.inpend) {
				cg2fb->status.reg.inten = 0;
				    /* clear pending interrupt. */
				wakeup((caddr_t)(gponeextra[gpunit].gpx_softfb
				    + fbunit));
				break;
			} /* if  inpend */
		} /* if fb_ropaddr */
	} /* for */
#ifdef lint
	gponeintr(gpunit);
#endif
}


gpone_gattr(pr, fbgattr)
	struct pixrect *pr;
	struct fbgattr *fbgattr;
{
	register int dev = gp1_d(pr)->cgpr_fd;
	register int gpunit =
	    GPUNIT(gponedevtbl[minor(dev)-NGPREQMINDEV].minordev);
	register int fbunit =
	    FBUNIT(gponedevtbl[minor(dev)-NGPREQMINDEV].minordev);

	return gpone_get_attr(gpunit, fbunit, fbgattr);
}


gpone_get_attr(gpunit, fbunit, gattr)
	register int gpunit;
	register int fbunit;
	register struct fbgattr *gattr;
{
	gattr->real_type = FBTYPE_SUN2GP;
	gattr->owner = 0;
	gattr->fbtype.fb_type = FBTYPE_SUN2GP;
	gattr->fbtype.fb_height = 
		gponeextra[gpunit].gpx_softfb[fbunit].fb_height;
	gattr->fbtype.fb_width =
		gponeextra[gpunit].gpx_softfb[fbunit].fb_hwwidth;
	gattr->fbtype.fb_depth = CG2_DEPTH;
	gattr->fbtype.fb_cmsize = 256;
	gattr->fbtype.fb_size = GP1TOTALSIZE;
	gattr->sattr.flags = FB_ATTR_DEVSPECIFIC;
	gattr->sattr.emu_type = -1;
	bcopy((char *) gponeextra[gpunit].gpx_softfb[fbunit].dev_specific,
		(char *) gattr->sattr.dev_specific,
		sizeof gattr->sattr.dev_specific);
	gattr->emu_types[0] = -1;
	return 0;
}


gpone_vertical(pr)
	struct pixrect *pr;
{
	register int dev = gp1_d(pr)->cgpr_fd;
	register int gpunit =
	    GPUNIT(gponedevtbl[minor(dev)-NGPREQMINDEV].minordev);
	register int fbunit =
	    FBUNIT(gponedevtbl[minor(dev)-NGPREQMINDEV].minordev);
	register struct cg2fb *cg2fb = (struct cg2fb *)
	    (gponeextra[gpunit].gpx_softfb [fbunit]).fb_ropaddr;

	return gpone_vertical_retrace(cg2fb, gpunit, fbunit);
}


gpone_vertical_retrace(cg2fb, gpunit, fbunit)
	register struct cg2fb *cg2fb;
	register int gpunit;
	register int fbunit;
{
	int priority_level;

	priority_level = splx(pritospl(gponeinfo[gpunit]->md_intpri));
	    /* gpone will poll its fbunits for the given gpunit. */
	cg2fb->intrptvec.reg = gponeinfo[gpunit]->md_intr->v_vec;
	cg2fb->status.reg.inten = 1;
	(void) sleep((caddr_t) (gponeextra[gpunit].gpx_softfb+fbunit), PZERO-1);
	(void) splx(priority_level);
	return 0;
}


/*ARGSUSED*/
gponemmap(dev, off, prot)
	dev_t dev;
	off_t off;
	int prot;
{
	int page;
	register int gpunit =
	    GPUNIT(gponedevtbl[minor(dev)-NGPREQMINDEV].minordev);
	register int fbunit =
	    FBUNIT(gponedevtbl[minor(dev)-NGPREQMINDEV].minordev);

	/* Check to see if we are in GP board or fb board */
	if ( (off >= 0 && off < GP1SIZE) || 
		(off >= gponeextra[gpunit].gpx_softfb[fbunit].fb_off_low) && 
		(off <  gponeextra[gpunit].gpx_softfb[fbunit].fb_off_high) ) {
		/*
		 * Can't use fbmmap since gp and fb devices are encoded in
		 * the minor device portion of dev.
		 */
		page = getkpgmap(gponeinfo[gpunit]->md_addr);
		page += btop(off);
		return (page);
	} else {
		return (0);
	}
} /* end of gponemmap() */

gpone_restart(gpunit, gp_addr)
	register int gpunit;
	caddr_t gp_addr;
{
	int i;
	struct gp1reg *gpptr;
	short *shmptr;
	short savedgpucode[4];

	/* since the GP is reset clean up the device tbl */
	for (i=0; i<NGPALTMINDEV; i++) {
		if (gponedevtbl[i].close_pending == 1) {
			gponedevtbl[i].minordev = NOREQMINDEV;
			gponedevtbl[i].close_pending = 0;
		}
	}

	gpptr = (struct gp1reg *) gp_addr;

	/* hardware reset */
	gpptr->gpr_csr.control =
		GP1_CR_CLRIF | GP1_CR_INT_DISABLE | GP1_CR_RESET;
	gpptr->gpr_csr.control = 0;

	/* save the start of microcode */
        gpptr->gpr_ucode_addr = 0;
	for (i=0; i<4; i++)
		savedgpucode[i] = gpptr->gpr_ucode_data;

	/* load priming the pipe microcode*/
	gpptr->gpr_ucode_addr = 0;
	gpptr->gpr_ucode_data = 0x0068;
	gpptr->gpr_ucode_data = 0x0000;
	gpptr->gpr_ucode_data = 0x7140;
	gpptr->gpr_ucode_data = 0x0000;

	/* Start the processors */
	gpptr->gpr_csr.control = 0;
	gpptr->gpr_csr.control = GP1_CR_VP_STRT0 | GP1_CR_VP_CONT
		| GP1_CR_PP_STRT0 | GP1_CR_PP_CONT;

	/* Hardware reset */
	gpptr->gpr_csr.control = GP1_CR_CLRIF |GP1_CR_INT_DISABLE |GP1_CR_RESET;
	gpptr->gpr_csr.control = 0;

	/* Clear first block of shared memory */
	shmptr = (short *)(gp_addr + GP1_SHMEM_OFFSET);
	for (i=0; i<512; i++)
		shmptr[i] = 0;

	/* Reset the block allocation vectors on the GP side */
        shmptr[5] = 0x8000;
        shmptr[6] = 0x00FF;

	/* Restore the start of microcode */
        gpptr->gpr_ucode_addr = 0;
	for (i=0; i<4; i++)
		gpptr->gpr_ucode_data = savedgpucode[i];

	/* Start the processors */
	gpptr->gpr_csr.control = 0;
	gpptr->gpr_csr.control = GP1_CR_VP_STRT0 | GP1_CR_VP_CONT |
		GP1_CR_PP_STRT0 | GP1_CR_PP_CONT;

	/* Any process that had a static block needs to be signalled */
	for (i=0; i<NSTBLKGP; i++) {
		if (gponeextra[gpunit].gpx_stblk_tbl[i].minordev != 
			(char) NOREQMINDEV)
			psignal(gponeextra[gpunit].gpx_stblk_tbl[i].process,
			    SIGXCPU);
	}

	printf("The Graphics Processor has been restarted.");
	printf("  You may see display garbage as a result.\n");

	/* Bump the restart count */
	gponeextra[gpunit].gpx_restart_count++;
} /* end of gpone_restart() */


gpone_close_cleanup(gpunit)
	int gpunit;
{
    caddr_t gpshmem = gponeextra[gpunit].gpx_shmem;
    int bitmask1, bitmask2, tblptr, blkptr;
    int freed_a_block;
    
    bitmask1 = *(int *)(&(gpshmem[ 6]));	/* host alloc bit mask */
    bitmask2 = *(int *)(&(gpshmem[10]));	/* gp alloc bit mask */
    bitmask1 = bitmask1 ^ bitmask2;		/* allocated blk bit mask */
    bitmask1 = bitmask1 & 0x7FFFFF00;		/* host allocated blk bitmask */
    bitmask2 = 0; /* bitmask2 now represents the blks to deallocate */
	
    freed_a_block = 0;

    /* Find all blocks that need to be deallocated */
    for (tblptr=0; tblptr < NGPALTMINDEV; tblptr++) {
        if (gponedevtbl[tblptr].close_pending && 
	  		gpunit == GPUNIT(gponedevtbl[tblptr].minordev)) {
	    for (blkptr = 30; blkptr > 7; blkptr--) {
	        if ( (bitmask1 & (1<<blkptr)) &&
		    (((char *)gpshmem)[528+blkptr] == (tblptr+NGPREQMINDEV))) {
	            bitmask2 = bitmask2 | (1<<blkptr);
		    freed_a_block = 1;
		}
	    }
	    gponedevtbl[tblptr].close_pending = 0;
	    gponedevtbl[tblptr].minordev = NOREQMINDEV;
	}
    }

    /* 
     * Alter the host allocated block bitmask on the GP.
     * Do it manually ... since the kernel has exclusive access to the GP
     * at this point.
     */
    *(int *)&(gpshmem[6]) = ~( (*(int *)&(gpshmem[6])) ^ ~bitmask2);
    bitmask1 = *(int *)(&(gpshmem[ 6]));/* host alloc bit mask */
    bitmask2 = *(int *)(&(gpshmem[10]));/* gp alloc bit mask */

    return(freed_a_block);
} /* end of gpone_close_cleanup() */


/* 
 * Kernel pixrects don't have a fd to feed off of, so this routine finds
 * the gpunit with the specified shmem address and calls the real restart
 * routine.  This is safe enough since there is a on to one correspondence
 * between gpunits and shmem addresses
 */
kernsyncrestart(shmem) 
	caddr_t shmem;
{
	register int gpunit;

	for (gpunit=0; gpunit<NGPONE; gpunit++) {
		if (shmem == gponeextra[gpunit].gpx_shmem) {
			gpone_restart(gpunit, gponeextra[gpunit].gpx_addr);
		}
	}
} /* end of kernsyncrestart() */

#endif NGPONE > 0
