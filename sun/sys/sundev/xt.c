#ifndef lint
static        char sccsid[] = "@(#)xt.c 1.4 86/10/29 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include "xt.h"
#if NXT > 0
/*
 * Driver for Xylogics 472 Tape controller
 * Controller names are xtc?
 * Device names are xt?
 * This driver lifted from the TapeMaster driver
 *
 * TODO:
 *	test driver with more than one slave
 */
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/dir.h"
#include "../h/conf.h"
#include "../h/user.h"
#include "../h/file.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/mtio.h"
#include "../h/ioctl.h"
#include "../h/cmap.h"
#include "../h/uio.h"
#include "../h/kernel.h"

#include "../machine/mmu.h"
#include "../machine/cpu.h"

#include "../sundev/mbvar.h"
#include "../sundev/xycreg.h"
#include "../sundev/xtreg.h"

#define SPLXT()	spl3()

/*
 * There is a cxtbuf per tape drive.
 * It is used as the token to pass to the internal routines
 * to execute tape ioctls.
 * When the tape is rewinding on close we release
 * the user process but any further attempts to use the tape drive
 * before the rewind completes will hang waiting for cxtbuf.
 */
struct	buf	cxtbuf[NXT];

#define	b_repcnt  b_bcount
#define	b_command b_resid

/*
 * Raw tape operations use rxtbuf.  The driver
 * notices when rxtbuf is being used and allows the user
 * program to continue after errors and read records
 * not of the standard length (BSIZE).
 */
struct	buf	rxtbuf[NXT];

/*
 * Driver Multibus interface routines and variables.
 */
int	xtprobe(), xtslave(), xtattach(), xtgo(), xtdone(), xtpoll();

struct	mb_ctlr *xtcinfo[NXTC];
struct	mb_device *xtdinfo[NXT];
struct	mb_driver xtcdriver = {
	xtprobe, xtslave, xtattach, xtgo, xtdone, xtpoll, 
	sizeof (struct xydevice), "xt", xtdinfo, "xtc", xtcinfo, MDR_BIODMA,
};
struct	buf xtutab[NXT];
short	xttoxtc[NXT];

/* bits in minor device */
#define NUNIT		4
#define	XTUNIT(dev)	(minor(dev)&03)
#define	XTCTLR(dev)	(xttoxtc[XTUNIT(dev)])
#define	T_NOREWIND	4
#define	T_HIDENS	8	/* select high density */

#define	INF	(daddr_t)1000000L

/*
 * Max # of buffers outstanding per unit
 */
#define MAXMTBUF	3

/*
 * Software state per tape transport.
 *
 * 1. A tape drive is a unique-open device; we refuse opens when it is already.
 * 2. We keep track of the current position on a block tape and seek
 *    before operations by forward/back spacing if necessary.
 * 3. We remember if the last operation was a write on a tape, so if a tape
 *    is open read write and the last thing done is a write we can
 *    write a standard end of tape mark (two eofs).
 * 4. We remember the status registers after the last command, using
 *    then internally and returning them to the SENSE ioctl.
 */
struct	xt_softc {
	char	sc_openf;	/* lock against multiple opens */
	char	sc_lastiow;	/* last op was a write */
	char	sc_stream;	/* tape is a streamer */
	char	sc_bufcnt;	/* queued system buffer count */
	daddr_t	sc_blkno;	/* block number, for block device tape */
	daddr_t	sc_nxrec;	/* position of end of tape, if known */
	u_short	sc_xtstat;	/* status bits from xt_status */
	u_short	sc_error;	/* copy of last erreg */
	u_short	sc_lastcomm;	/* last command executed */
	long	sc_resid;	/* copy of last bc */
	daddr_t	sc_timo;	/* time until timeout expires */
	short	sc_tact;	/* timeout is active */
} xt_softc[NXT];

/*
 * Data per controller
 */
struct	xtctlr {
	struct xt_softc	*c_units[NUNIT];/* units on controller */
	struct xydevice	*c_io;		/* ptr to I/O space data */
	struct xtiopb	*c_iopb;	/* ptr to IOPB */
	char		c_present;	/* controller is present */
	char		c_wantint;	/* expecting interrupt */
} xtctlrs[NXTC];

/*
 * States for mc->mc_tab.b_active, the per controller state flag.
 * This is used to sequence control in the driver.
 */
#define	SSEEK	1		/* seeking */
#define	SIO	2		/* doing seq i/o */
#define	SCOM	3		/* sending control command */
#define	SREW	4		/* sending a drive rewind */
#define	SERR	5		/* doing erase gap (error on write) */

/*
 * Give a simple command to a controller
 * and spin until done.
 * Returns the error number or zero
 */
static
simple(c, unit, cmd)
	struct xtctlr *c;
{
	register struct xydevice *xyio = c->c_io;
	register struct xtiopb *xt = c->c_iopb;
	int piopb, t;

	t = xyio->xy_resupd;		/* reset */
	DELAY(100);
	while (xyio->xy_csr & XY_BUSY)
		DELAY(10);
	bzero((caddr_t)xt, sizeof *xt);
	xt->xt_autoup = 1;
	xt->xt_reloc = 1;
	xt->xt_cmd = cmd;
	xt->xt_throttle = 4;
	xt->xt_unit = unit;
	piopb = ((char *)xt) - DVMA;
	t = XYREL(xyio, piopb);
	xyio->xy_iopbrel[0] = t >> 8;
	xyio->xy_iopbrel[1] = t;
	t = XYOFF(piopb);
	xyio->xy_iopboff[0] = t >> 8;
	xyio->xy_iopboff[1] = t;
	xyio->xy_csr = XY_GO;
	DELAY(100);
	while (xyio->xy_csr & XY_BUSY)
		DELAY(10);
	return (xt->xt_errno);
}

/*
 * Determine existence of controller
 */
xtprobe(reg, ctlr)
	caddr_t reg;
{
	register struct xtctlr *c = &xtctlrs[ctlr];
	int err;

	c->c_io = (struct xydevice *)reg;
	if (peekc((char *)&c->c_io->xy_resupd) == -1)
		return (0);

	c->c_iopb = (struct xtiopb *)rmalloc(iopbmap,
		(long)sizeof (struct xtiopb));
	if (c->c_iopb == NULL) {
		printf("xtprobe: no iopb space\n");
		return (0);
	}
	c->c_present = 1;
	(void) simple(c, 0, XT_NOP);
	if (c->c_iopb->xt_ctype != XYC_472) {
		printf("xtc%d: unknown controller type\n", ctlr);
		return (0);
	}
	if (err = simple(c, 0, XT_TEST))
		printf("xtc%d: self test error %x\n", ctlr, err);
	if (err)
		return (0);
	return (sizeof (struct xydevice));
}

/*
 * Always say that a unit is there.
 * We can't tell for sure anyway, and this lets
 * a guy plug one in without taking down the system
 * (These are micros, after all!)
 */
/*ARGSUSED*/
xtslave(md, reg)
	struct mb_device *md;
	caddr_t reg;
{

	return (1);
}

/*
 * Record attachment of the unit to the controller.
 */
/*ARGSUSED*/
xtattach(md)
	struct mb_device *md;
{
	register struct xtctlr *c = &xtctlrs[md->md_ctlr];
	int unit = md->md_slave;

	/* set up for any vectored interrupts to pass ctlr pointer */
	if (md->md_mc->mc_intr) {
	        if ((c->c_io->xy_csr & XY_ADDR24) == 0)
		        printf("xtc%d: WARNING: 20 bit addresses\n",
			    md->md_mc->mc_ctlr);
		*(md->md_mc->mc_intr->v_vptr) = (int)c;
	}

	c->c_units[unit] = &xt_softc[md->md_unit];
	/*
	 * xttoxtc is used in XTCTLR to index the controller
	 * arrays given a xt unit number.
	 */
	xttoxtc[md->md_unit] = md->md_mc->mc_ctlr;
}

int	xttimer();
int	xtdefdens = 0;
int	xtdefspeed = 0;
int	recurse_flag = 0;
/*
 * Open the device.  Tapes are unique open
 * devices, so we refuse if it is already open.
 * We also check that a tape is available, and
 * don't block waiting here; if you want to wait
 * for a tape you should timeout in user code.
 */
xtopen(dev, flag)
	dev_t dev;
	int flag;
{
	register int xtunit;
	register struct mb_device *md;
	register struct xt_softc *sc;
	int t;
	struct xydevice *xyio;

	xtunit = XTUNIT(dev);
	if (xtunit>=NXT || (sc = &xt_softc[xtunit])->sc_openf ||
	    (md = xtdinfo[xtunit]) == 0 || md->md_alive == 0)
		return (ENXIO);
	sc->sc_openf = 1;
	if (xtcommand(dev, XT_DSTAT, 0, 1)) 	/* timed out */
		goto retry;
	if ((sc->sc_xtstat & (XTS_ONL|XTS_RDY)) != (XTS_ONL|XTS_RDY)) {
		uprintf("xt%d: not online\n", xtunit);
		sc->sc_openf = 0;
		return (EIO);
	}
	if ((flag&FWRITE) && (sc->sc_xtstat & XTS_FPT)) {
		uprintf("xt%d: no write ring\n", xtunit);
		sc->sc_openf = 0;
		return (EIO);
	}
	if (sc->sc_tact == 0) {
		sc->sc_timo = INF;
		sc->sc_tact = 1;
		timeout(xttimer, (caddr_t)dev, 2*hz);
	}
	if (sc->sc_xtstat & XTS_BOT) {
		switch (md->md_flags) {
		default:
		case 0:		/* unknown drive type */
			if (xtdefdens) {
				if (xtdefdens > 0) {
					if (xtcommand(dev, XT_PARAM,
					    XT_HIDENS, 1))
						goto retry;
				} else
					if (xtcommand(dev, XT_PARAM,
					    XT_LODENS, 1))
						goto retry;
			}
			if (xtdefspeed) {
				if (xtdefspeed > 0) {
					if (xtcommand(dev, XT_PARAM,
					    XT_HIGH, 1))
						goto retry;
				} else
					if (xtcommand(dev, XT_PARAM,
					    XT_LOW, 1))
						goto retry;
			}
			break;
		case 1:		/* CDC Keystone III and Telex 9250 */
			if (minor(dev) & T_HIDENS) {
				if (xtcommand(dev, XT_PARAM, XT_HIDENS, 1))
					goto retry;
			} else
				if (xtcommand(dev, XT_PARAM, XT_LODENS, 1))
					goto retry;
			break;
		case 2:		/* Kennedy triple density */
			if (minor(dev) & T_HIDENS) {
				if (xtcommand(dev, XT_PARAM, XT_HIGH, 1))
					goto retry;
			} else
				if (xtcommand(dev, XT_PARAM, XT_LOW, 1))
					goto retry;
			break;
		}
	}
	sc->sc_blkno = (daddr_t)0;
	sc->sc_nxrec = INF;
	sc->sc_lastiow = 0;
	return (0);
retry:	sc->sc_openf = 0;
	if (!recurse_flag) {
		printf("xt: attempting reset\n");
		recurse_flag = 1;
		xyio = xtctlrs[md->md_ctlr].c_io;
		t = xyio->xy_resupd;			/* reset */
		DELAY(100);
		while (xyio->xy_csr & XY_BUSY)
			DELAY(10);
		t = xtopen(dev, flag);
		recurse_flag = 0;
		return(t);
	} else
		return (EIO);
}

/*
 * Close tape device.
 *
 * If tape was open for writing or last operation was
 * a write, then write two EOF's and backspace over the last one.
 * Unless this is a non-rewinding special file, rewind the tape.
 * Make the tape available to others.
 */
xtclose(dev, flag)
	register dev_t dev;
	register flag;
{
	register struct xt_softc *sc = &xt_softc[XTUNIT(dev)];

	if (flag == FWRITE || (flag&FWRITE) && sc->sc_lastiow) {
		if (xtcommand(dev, XT_FMARK, 0, 1) ||
		    xtcommand(dev, XT_FMARK, 0, 1) ||
		    xtcommand(dev, XT_SEEK, XT_REVERSE, 1)) { /* interrupted */
			sc->sc_openf = 0;
		}
	}
	if ((minor(dev)&T_NOREWIND) == 0 &&
	    sc->sc_lastcomm != ((XT_SEEK<<8)+XT_UNLOAD))
		/*
		 * 0 count means don't hang waiting for rewind complete
		 * rather cxtbuf stays busy until the operation completes
		 * preventing further opens from completing by
		 * preventing a TM_SENSE from completing.
		 */
		(void) xtcommand(dev, XT_SEEK, XT_REW, 0);
	sc->sc_openf = 0;
}

/*
 * Execute a command on the tape drive
 * a specified number of times.
 * Allow signals to occur (with tsleep) so the
 * poor user can use his terminal if something goes wrong
 */
xtcommand(dev, com, subfunc, count)
	dev_t dev;
	int com, subfunc, count;
{
	register struct buf *bp;
	register int s;
	int error;

	bp = &cxtbuf[XTUNIT(dev)];
	s = SPLXT();
	while (bp->b_flags&B_BUSY) {
		/*
		 * This special check is because B_BUSY never
		 * gets cleared in the non-waiting rewind case.
		 */
		if (bp->b_repcnt == 0 && (bp->b_flags&B_DONE))
			break;
		bp->b_flags |= B_WANTED;
#ifdef TSLEEP
		/* XXX - tsleep can be replaced by new PCATCH option to sleep */
		if (tsleep((caddr_t)bp, PRITM, 0) == TS_SIG)
			return (1);
#else
		(void) sleep((caddr_t)bp, PRIBIO);
#endif
	}
	bp->b_flags = B_BUSY|B_READ;
	(void) splx(s);

	bp->b_dev = dev;
	bp->b_repcnt = count;
	bp->b_command = (com << 8) + subfunc;
	bp->b_blkno = 0;
	xtstrategy(bp);
	/*
	 * In case of rewind from close, don't wait.
	 * This is the only case where count can be 0.
	 */
	if (count == 0)
		return (0);

	s = SPLXT();
	while ((bp->b_flags&B_DONE) == 0) {
#ifdef TSLEEP
		/* XXX - tsleep can be replaced by new PCATCH option to sleep */
		if (tsleep((caddr_t)bp, PRITM, 0) == TS_SIG)
			return (1);
#else
		(void) sleep((caddr_t)bp, PRIBIO);
#endif
	}
	(void) splx(s);

	error = geterror(bp);
	if (com == XT_SEEK && bp->b_resid != 0 && !error) {
		switch (subfunc) {
		case XT_REW:
		case XT_UNLOAD:
			break;
		default:
			error = EIO; 
			break;
		}
	}
	if (bp->b_flags&B_WANTED)
		wakeup((caddr_t)bp);
	bp->b_flags &= B_ERROR;		/* note: clears B_BUSY */
	return (error);
}

/*
 * Queue a tape operation.
 */
xtstrategy(bp)
	register struct buf *bp;
{
	int xtunit = XTUNIT(bp->b_dev);
	register struct mb_ctlr *mc;
	register struct buf *dp;
	register struct xt_softc *sc = &xt_softc[xtunit];
	int s;

	/*
	 * Put transfer at end of unit queue
	 */
	dp = &xtutab[xtunit];
	bp->av_forw = NULL;
	s = SPLXT();
	while (sc->sc_bufcnt >= MAXMTBUF)
		(void) sleep((caddr_t)&sc->sc_bufcnt, PRIBIO);
	sc->sc_bufcnt++;
	mc = xtdinfo[xtunit]->md_mc;
	if (dp->b_actf == NULL) {
		dp->b_actf = bp;
		/*
		 * Transport not already active...
		 * put at end of controller queue.
		 */
		dp->b_forw = NULL;
		if (mc->mc_tab.b_actf == NULL)
			mc->mc_tab.b_actf = dp;
		else
			mc->mc_tab.b_actl->b_forw = dp;
		mc->mc_tab.b_actl = dp;
	} else
		dp->b_actl->av_forw = bp;
	dp->b_actl = bp;
	/*
	 * If the controller is not busy, get
	 * it going.
	 */
	if (mc->mc_tab.b_active == 0)
		xtstart(mc);
	(void) splx(s);
}

/*
 * Start activity on a xt controller.
 */
xtstart(mc)
	register struct mb_ctlr *mc;
{
	register struct buf *bp, *dp;
	register struct xt_softc *sc;
	register struct mb_device *md;
	register struct xtiopb *xt;
	register struct xydevice *xyio;
	int xtunit, cmd, subfunc, count, size, fast;
	daddr_t blkno;

	/*
	 * Look for an idle transport on the controller.
	 */
loop:
	if ((dp = mc->mc_tab.b_actf) == NULL)
		return;
	if ((bp = dp->b_actf) == NULL) {
		mc->mc_tab.b_actf = dp->b_forw;
		goto loop;
	}
	xtunit = XTUNIT(bp->b_dev);
	md = xtdinfo[xtunit];
	sc = &xt_softc[xtunit];
	xyio = xtctlrs[md->md_ctlr].c_io;

	/* if commands are overlapped, the controller must be hung */

	if (xyio->xy_csr & XY_BUSY) {
		printf("xt: bad command synchronization\n");
		sc->sc_openf = -1;
	}
	/*
	 * Default is that last command was NOT a write command;
	 * if we do a write command we will notice this in xtintr().
	 */
	sc->sc_lastiow = 0;

	if ((int)sc->sc_openf < 0) {
		/*
		 * Have had a hard error on a non-raw tape
		 * or the tape unit is now unavailable
		 * (e.g. taken off line).
		 */
		bp->b_flags |= B_ERROR;
		goto next;
	}
	
	fast = 0;
	count = 1;
	subfunc = 0;
	if (bp == &cxtbuf[xtunit]) {
		/*
		 * Execute control operation with the specified count.
		 */
		/*
		 * Set next state; give 5 minutes to complete
		 * rewind, or 10 seconds per iteration (minimum 60
		 * seconds and max 5 minutes) to complete other ops.
		 */
	switch (bp->b_command & ~XT_REVERSE) {
		case (XT_SEEK<<8)+XT_REW:
		case (XT_SEEK<<8)+XT_UNLOAD:
			mc->mc_tab.b_active = SREW;
			sc->sc_timo = 5 * 60;
			fast = 1;
			break;

		case (XT_SEEK<<8)+XT_FILE:
			mc->mc_tab.b_active = SCOM;
			sc->sc_timo = 20 * 60;
			fast = 1;
			break;

		case (XT_NOP<<8):
		case (XT_DSTAT<<8):
		case (XT_PARAM<<8)+XT_LODENS:
		case (XT_PARAM<<8)+XT_HIDENS:
		case (XT_PARAM<<8)+XT_LOW:
		case (XT_PARAM<<8)+XT_HIGH:
			mc->mc_tab.b_active = SCOM;
			sc->sc_timo = imin(4*(int)bp->b_repcnt,60);
			break;
		default:
			mc->mc_tab.b_active = SCOM;
			sc->sc_timo =
			    imin(imax(20*(int)bp->b_repcnt,30),5*60);
			break;
		}
		count = bp->b_repcnt;
		cmd = bp->b_command >> 8;
		subfunc = bp->b_command & 0xFF;
		size = 0;
		goto dobpcmd;
	}
	/*
	 * The following checks handle boundary cases for operation
	 * on non-raw tapes.  On raw tapes the initialization of
	 * sc->sc_nxrec by xtphys causes them to be skipped normally
	 * (except in the case of retries).
	 */
	if (bdbtofsb(bp->b_blkno) > sc->sc_nxrec) {
		/*
		 * Can't read past known end-of-file.
		 */
		bp->b_flags |= B_ERROR;
		bp->b_error = ENXIO;
		goto next;
	}
	if (bdbtofsb(bp->b_blkno) == sc->sc_nxrec &&
	    bp->b_flags&B_READ) {
		/*
		 * Reading at end of file returns 0 bytes.
		 */
		clrbuf(bp);
		bp->b_resid = bp->b_bcount;
		goto next;
	}
	if ((bp->b_flags&B_READ) == 0) {
		/*
		 * Writing sets EOF
		 */
		sc->sc_nxrec = bdbtofsb(bp->b_blkno) + 1;
	}
	/*
	 * If the data transfer command is in the correct place,
	 * fire the operation up.
	 */
	if ((blkno = sc->sc_blkno) == bdbtofsb(bp->b_blkno)) {
		size = bp->b_bcount;
		if ((bp->b_flags&B_READ) == 0) {
			if (mc->mc_tab.b_active!=SERR && mc->mc_tab.b_errcnt) {
				cmd = XT_FMARK, subfunc = XT_ERASE;
				mc->mc_tab.b_active = SERR;
				size = 0;
				count = 1;
			} else {
				cmd = XT_WRITE;
				mc->mc_tab.b_active = SIO;
			}
		} else {
			cmd = XT_READ;
			mc->mc_tab.b_active = SIO;
		}
		sc->sc_timo = imin(imax(20*(size/65536)+20,30),5 * 60);
	} else {
		/*
		 * Tape positioned incorrectly;
		 * set to seek forwards or backwards to the correct spot.
		 * This happens for raw tapes only on error retries.
		 */
		mc->mc_tab.b_active = SSEEK;
		if (blkno < bdbtofsb(bp->b_blkno)) {
			cmd = XT_SEEK, subfunc = XT_REC;
			count = bdbtofsb(bp->b_blkno) - blkno;
		} else {
			cmd = XT_SEEK, subfunc = XT_REC+XT_REVERSE;
			count = blkno - bdbtofsb(bp->b_blkno);
		}
		sc->sc_timo = imin(imax(20 * count, 30), 5 * 60);
		fast = count > 1;
		size = 0;
	}
dobpcmd:
	/*
	 * Do the command in bp.
	 */
	xt = xtctlrs[md->md_ctlr].c_iopb;
	bzero((caddr_t)xt, sizeof *xt);
	xt->xt_autoup = 1;
	xt->xt_reloc = 1;
	xt->xt_ie = 1;
	xt->xt_cmd = cmd;
	xt->xt_throttle = 4;
	xt->xt_subfunc = subfunc;
	xt->xt_unit = xtdinfo[xtunit]->md_slave;
#ifdef notdef
	if (sc->sc_stream) {
		tpb->tm_ctl.tmc_speed = fast;
	} else {
		tpb->tm_ctl.tmc_speed = (minor(bp->b_dev) & T_HIDENS) ? 0 : 1;
	}
#else
#ifdef lint
	fast = fast;
#endif
#endif
	if (size) {
		xt->xt_swab = 1;
		xt->xt_retry = 1;
		xt->xt_cnt = size;
		(void) mbgo(mc);
	} else {
		xt->xt_cnt = count;
		xtgo(mc);
	}
	sc->sc_lastcomm = (cmd << 8) + subfunc;
	return;

next:
	/*
	 * Done with this operation due to error or
	 * the fact that it doesn't do anything.
	 */
	mc->mc_tab.b_errcnt = 0;
	mc->mc_tab.b_active = 0;
	sc->sc_timo = INF;
	dp->b_actf = bp->av_forw;
	iodone(bp);
	if (sc->sc_bufcnt-- >= MAXMTBUF)
		wakeup((caddr_t)&sc->sc_bufcnt);
	goto loop;
}

/*
 * The Multibus resources we needed have been
 * allocated to us; start the device.
 */
xtgo(mc)
	struct mb_ctlr *mc;
{
	register struct xtctlr *c = &xtctlrs[mc->mc_ctlr];
	register struct xydevice *xyio = c->c_io;
	register struct xtiopb *xt = c->c_iopb;
	register int dmaddr, piopb, t;

	dmaddr = MBI_ADDR(mc->mc_mbinfo);
	if ((dmaddr + xt->xt_cnt) > 0x100000 && (xyio->xy_csr & XY_ADDR24) == 0)
		panic("xt: exceeded 20 bit address");
	xt->xt_bufoff = XYOFF(dmaddr);
	xt->xt_bufrel = XYREL(xyio, dmaddr);
	/* stuff IOPB info into I/O registers */
	piopb = ((char *)xt) - DVMA;
	t = XYREL(xyio, piopb);
	xyio->xy_iopbrel[0] = t >> 8;
	xyio->xy_iopbrel[1] = t;
	t = XYOFF(piopb);
	xyio->xy_iopboff[0] = t >> 8;
	xyio->xy_iopboff[1] = t;
	xyio->xy_csr = XY_GO;
	mc->mc_tab.b_flags &=~ B_DONE;
	mc->mc_tab.b_flags |= B_BUSY;
}

/*
 * interrupt routine.
 */
xtintr(c)
	register struct xtctlr *c;
{
	register struct mb_ctlr *mc;
	register struct xt_softc *sc;
	register struct xtiopb *xt;
	register struct buf *dp;
	register struct buf *bp;
	register int xtunit, state;
	int err;

	mc = xtcinfo[c - xtctlrs];
	/* clear the interrupt */
	if (c->c_io->xy_csr & (XY_ERROR|XY_DBLERR)) {
		c->c_io->xy_csr = XY_ERROR;
		DELAY(50);
	}
	c->c_io->xy_csr = XY_INTR;
	mc->mc_tab.b_flags &= ~B_BUSY;
	if ((state = mc->mc_tab.b_active) == 0) {
		printf("xt%d: stray interrupt\n", mc->mc_ctlr);
		return;
	}
	dp = mc->mc_tab.b_actf;
	bp = dp->b_actf;
	if (bp == NULL)
		panic("xtintr: queuing error");
	xtunit = XTUNIT(bp->b_dev);
	if (xtunit >= NXT)
		panic("xtintr: queueing error 2");
	xt = c->c_iopb;
	sc = &xt_softc[xtunit];
	sc->sc_xtstat = xt->xt_status;
	if ((sc->sc_xtstat & XTS_EOT) && (bp->b_flags & B_READ) == B_WRITE)
		sc->sc_resid = bp->b_bcount;
	else
		sc->sc_resid = bp->b_bcount - xt->xt_acnt;

	/*
	 * If last command was a rewind, and tape is still
	 * rewinding, wait for another interrupt, triggered 
	 * by xttimer
	 */
	if (state == SREW) {
		if ((sc->sc_xtstat & XTS_ONL) &&
		    (sc->sc_xtstat & (XTS_RDY|XTS_BOT)) != (XTS_RDY|XTS_BOT))
			return;
		state = SCOM;
	}

	/*
	 * An operation completed... record status
	 */
	sc->sc_timo = INF;
	err = xt->xt_errno;
	if ((bp->b_flags & B_READ) == 0)
		sc->sc_lastiow = 1;
	if (err == XTE_BOT && (sc->sc_xtstat & XTS_BOT))
		err = XTE_NOERROR;
	if (err == XTE_SOFTERR) {
		printf("xt%d: soft error bn=%d\n", xtunit, bp->b_blkno);
		err = XTE_NOERROR;
	}
	sc->sc_error = err;
	/*
	 * Check for errors.
	 */
	if (err != XTE_NOERROR && err != XTE_SHORTREC) {
		/*
		 * If we hit the end of the tape file, update our position.
		 */
		if (err == XTE_EOF || err == XTE_EOT) {
			xtseteof(bp);		/* set blkno and nxrec */
			state = SCOM;		/* force completion */
			sc->sc_resid = bp->b_bcount;
			goto opdone;
		}
		if (err == XTE_LONGREC) {
			bp->b_flags |= B_ERROR;
			goto opdone;
		}
		/*
		 * If error is not hard, and this was an i/o operation
		 * retry up to 8 times.
		 */
		if (state == SIO) {
			if (++mc->mc_tab.b_errcnt < 7) {
				sc->sc_blkno++;	/* force repositioning */
				goto opcont;
			}
		} else {
			/*
			 * Hard or non-i/o errors on non-raw tape
			 * cause it to close.
			 */
			if ((int)sc->sc_openf > 0 && bp != &rxtbuf[xtunit])
				sc->sc_openf = -1;
		}
		/*
		 * Couldn't recover error
		 */
		printf("xt%d: hard error bn=%d er=%x\n", xtunit,
		    bp->b_blkno, err);
		bp->b_flags |= B_ERROR;
		goto opdone;
	}
	/*
	 * Advance tape control FSM.
	 */
	switch (state) {

	case SIO:
		/*
		 * Read/write increments tape block number
		 */
		sc->sc_blkno++;
		break;

	case SCOM:
		/*
		 * For forward/backward space record update current position.
		 */
		if (bp == &cxtbuf[xtunit])
		switch (bp->b_command) {

		case (XT_SEEK<<8)+XT_REC:
			sc->sc_blkno += bp->b_repcnt;
			break;

		case (XT_SEEK<<8)+XT_REVERSE+XT_REC:
			sc->sc_blkno -= bp->b_repcnt;
			break;

		case (XT_SEEK<<8)+XT_REW:
		case (XT_SEEK<<8)+XT_UNLOAD:
			sc->sc_blkno = 0;
			break;
		}
		break;

	case SSEEK:
		sc->sc_blkno = bdbtofsb(bp->b_blkno);
		goto opcont;

	case SERR:
		goto opcont;

	default:
		panic("xtintr");
	}
opdone:
	mc->mc_tab.b_active = 0;
	mc->mc_tab.b_flags |= B_DONE;
opcont:
	if (mc->mc_mbinfo != 0)
		mbdone(mc);
	else
		xtdone(mc);
}

/*
 * polling interrupt routine.
 */
xtpoll()
{
	register struct xtctlr *c;

	for (c = xtctlrs; c < &xtctlrs[NXTC]; c++) {
		if (!c->c_present || (c->c_io->xy_csr & XY_INTR) == 0)
			continue;
		xtintr(c);
		return (1);
	}
	return (0);
}

xtdone(mc)
	register struct mb_ctlr *mc;
{
	register struct buf *dp;
	register struct buf *bp;
	register struct xt_softc *sc;
	register struct mb_device *md;
	register struct xydevice *xyio;
	int xtunit;

	dp = mc->mc_tab.b_actf;
	bp = dp->b_actf;
	xtunit = XTUNIT(bp->b_dev);
	sc = &xt_softc[xtunit];
	if (mc->mc_tab.b_flags & B_DONE) {
		/*
		 * Reset error count and remove
		 * from device queue.
		 */
		mc->mc_tab.b_errcnt = 0;
		dp->b_actf = bp->av_forw;
		bp->b_resid = sc->sc_resid;
		iodone(bp);
		if (sc->sc_bufcnt-- >= MAXMTBUF)
			wakeup((caddr_t)&sc->sc_bufcnt);
		/*
		 * Advance controller queue and put this
		 * unit back on the controller queue if
		 * the unit queue is not empty
		 */
		mc->mc_tab.b_actf = dp->b_forw;
		if (dp->b_actf) {
			dp->b_forw = NULL;
			if (mc->mc_tab.b_actf == NULL)
				mc->mc_tab.b_actf = dp;
			else
				mc->mc_tab.b_actl->b_forw = dp;
			mc->mc_tab.b_actl = dp;
		}
	} else {
		/*
		 * Circulate slave to end of controller
		 * queue to give other slaves a chance.
		 * No need to look at unit queue since operation
		 * is still in progress
		 */
		if (dp->b_forw) {
			mc->mc_tab.b_actf = dp->b_forw;
			dp->b_forw = NULL;
			mc->mc_tab.b_actl->b_forw = dp;
			mc->mc_tab.b_actl = dp;
		}
	}
	md = xtdinfo[xtunit];
	xyio = xtctlrs[md->md_ctlr].c_io;
	CDELAY (((xyio->xy_csr & XY_BUSY) == 0), 100);
	if (mc->mc_tab.b_actf)
		xtstart(mc);
}

xttimer(dev)
	int dev;
{
	register struct mb_ctlr *mc = xtcinfo[XTCTLR(dev)];
	register struct xt_softc *sc = &xt_softc[XTUNIT(dev)];
	register struct xtctlr *c = &xtctlrs[XTCTLR(dev)];
	register struct xtiopb *xt = c->c_iopb;
	register struct xydevice *xyio;
	register int s;
	int piopb, t;

	if (sc->sc_timo != INF && (sc->sc_timo -= 2) < 0) {
		printf("xt%d: lost interrupt\n", XTUNIT(dev));
		sc->sc_timo = INF;
		s = SPLXT();
		xt->xt_errno = XTE_TIMEOUT;
		xtintr(c);
		(void) splx(s);
	}
	if (mc->mc_tab.b_active == SREW) {	/* check rewind status */
		s = SPLXT();
		if ((mc->mc_tab.b_flags & B_BUSY) == 0 &&
		    (c->c_io->xy_csr & XY_BUSY) == 0) {
			xyio = c->c_io;
			bzero((caddr_t)xt, sizeof *xt);
			xt->xt_autoup = 1;
			xt->xt_reloc = 1;
			xt->xt_ie = 1;
			xt->xt_cmd = XT_DSTAT;
			xt->xt_throttle = 4;
			xt->xt_unit = xtdinfo[XTUNIT(dev)]->md_slave;
			piopb = ((char *)xt) - DVMA;
			t = XYREL(xyio, piopb);
			xyio->xy_iopbrel[0] = t >> 8;
			xyio->xy_iopbrel[1] = t;
			t = XYOFF(piopb);
			xyio->xy_iopboff[0] = t >> 8;
			xyio->xy_iopboff[1] = t;
			xyio->xy_csr = XY_GO;
			mc->mc_tab.b_flags |= B_BUSY;
		}
		(void) splx(s);
	}
	timeout(xttimer, (caddr_t)dev, 2*hz);
}

xtseteof(bp)
	register struct buf *bp;
{
	register int xtunit = XTUNIT(bp->b_dev);
	register struct xt_softc *sc = &xt_softc[xtunit];
	register struct xtiopb *xt = xtctlrs[XTCTLR(bp->b_dev)].c_iopb;

	if (bp == &cxtbuf[xtunit]) {
		if (sc->sc_blkno > bdbtofsb(bp->b_blkno)) {
			/* reversing */
			sc->sc_nxrec = bdbtofsb(bp->b_blkno) - xt->xt_acnt;
			sc->sc_blkno = sc->sc_nxrec;
		} else {
			/* spacing forward */
			sc->sc_blkno = bdbtofsb(bp->b_blkno) + xt->xt_acnt;
			sc->sc_nxrec = sc->sc_blkno - 1;
		}
		return;
	} 
	/* eof on read */
	sc->sc_nxrec = bdbtofsb(bp->b_blkno);
}

void
xtminphys(bp)
	struct buf *bp;
{
	if (bp->b_bcount > 65535)
		bp->b_bcount = 65535;
}

xtread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	int err;

	if (err = xtphys(dev, uio))
		return (err);
	return (physio(xtstrategy, &rxtbuf[XTUNIT(dev)], dev, B_READ,
	    xtminphys, uio));
}

xtwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	int err;

	if (err = xtphys(dev, uio))
		return (err);
	if (uio->uio_iov->iov_len > 65535)
		return (EINVAL);
	return (physio(xtstrategy, &rxtbuf[XTUNIT(dev)], dev, B_WRITE,
	    xtminphys, uio));
}

/*
 * Check that a raw device exists.
 * If it does, set up sc_blkno and sc_nxrec
 * so that the tape will appear positioned correctly.
 */
xtphys(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int xtunit = XTUNIT(dev);
	register daddr_t a;
	register struct xt_softc *sc;
	register struct mb_device *md;

	if (xtunit >= NXT || (md=xtdinfo[xtunit]) == 0 || md->md_alive == 0)
		return (ENXIO);
	sc = &xt_softc[xtunit];
	a = bdbtofsb(uio->uio_offset / DEV_BSIZE);
	sc->sc_blkno = a;
	sc->sc_nxrec = a + 1;
	return (0);
}

/*ARGSUSED*/
xtioctl(dev, cmd, data, flag)
	dev_t dev;
	caddr_t data;
{
	int xtunit = XTUNIT(dev);
	register struct xt_softc *sc = &xt_softc[xtunit];
	register callcount;
	int fcount, op;
	struct mtop *mtop;
	struct mtget *mtget;
	/* we depend of the values and order of the MT codes here */
	static tmops[] = {
		(XT_FMARK<<8),
		(XT_SEEK<<8)+XT_FILE,
		(XT_SEEK<<8)+XT_REVERSE+XT_FILE,
		(XT_SEEK<<8)+XT_REC,
		(XT_SEEK<<8)+XT_REVERSE+XT_REC,
		(XT_SEEK<<8)+XT_REW,
		(XT_SEEK<<8)+XT_UNLOAD,
		(XT_DSTAT<<8),
	};

	switch (cmd) {

	case MTIOCTOP:	/* tape operation */
		mtop = (struct mtop *)data;
		switch (mtop->mt_op) {
		case MTWEOF:
		case MTFSF: case MTBSF:
			callcount = mtop->mt_count;
			fcount = 1;
			break;
		case MTFSR: case MTBSR:
			callcount = 1;
			fcount = mtop->mt_count;
			break;
		case MTREW: case MTOFFL: case MTNOP:
			callcount = 1;
			fcount = mtop->mt_count;
			break;
		default:
			return (ENXIO);
		}
		if (callcount <= 0 || fcount <= 0)
			return (ENXIO);
		while (--callcount >= 0) {
			op = tmops[mtop->mt_op];
			if (xtcommand(dev, op >> 8, op & 0xFF, fcount))
				return (EIO);
			if ((mtop->mt_op == MTFSR || mtop->mt_op == MTBSR) &&
			    sc->sc_error == XTE_EOF)
				return (EIO);
			if (sc->sc_xtstat & XTS_EOT)
				break;
		}
		break;

	case MTIOCGET:
		mtget = (struct mtget *)data;
		mtget->mt_dsreg = sc->sc_xtstat;
		mtget->mt_erreg = sc->sc_error;
		mtget->mt_resid = sc->sc_resid;
		mtget->mt_type = MT_ISXY;
		break;

	default:
		return (ENXIO);
	}
	return (0);
}
#endif
