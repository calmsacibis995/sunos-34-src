/*	@(#)sf.c 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "sf.h"
#if NSF > 0

/*
 * SCSI driver for SCSI floppy disks.
 */
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dk.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/user.h"
#include "../h/map.h"
#include "../h/vmmac.h"
#include "../h/ioctl.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/dkbad.h"
#include "../h/vfs.h"
#include "../h/vnode.h"

#include "../machine/pte.h"
#include "../machine/psl.h"
#include "../machine/mmu.h"
#include "../machine/cpu.h"
#include "../sun/dklabel.h"
#include "../sun/dkio.h"
#include "../pcfs/pc_label.h"
#include "../sundev/mbvar.h"
#include "../sundev/screg.h"
#include "../sundev/sireg.h"
#include "../sundev/scsi.h"

#define	MAX_RETRIES	0
#define	MAX_RESTORES	1

#define	SFUNIT(dev)	(minor(dev))
#define	SFNUM(un)	(un - sfunits)

#define b_cylin b_resid

#define BUSY_RETRY 1000

/*
 * Error message control.
 */
#define	EL_RETRY	3
#define EL_REST		2
#define	EL_FAIL		1
int sferrlvl = EL_REST;

/*
 * sf specific SCSI commands
 */
#define SC_INIT_CHARACTERISTICS 0x0c	/* initialize drive characteristics */

extern struct scsi_unit sfunits[];
extern struct scsi_unit_subr scsi_unit_subr[];
extern struct scsi_floppy sfloppy[];
extern int nsfloppy;

#define SF_LABELGET	0x01		/* getting device label */
#define SF_LABELWANT	0x02		/* waiting to get label */

struct sf_characteristics {
	u_char	sfc_ncyl;		/* number of cylinders */
	u_int	sfc_trkswtch	: 4;	/* track switching rate code */
	u_int	sfc_mstart	: 12;	/* motor start time (msec) */
	u_char	sfc_drive	: 4;	/* drive type (5 or 8 inch) */
	u_char	sfc_nhead	: 4;	/* number of heads */
	u_char	sfc_secsize;		/* sector size / 256 */
	u_char	sfc_uload;		/* head unload time in 1/10 sec */
	u_char	sfc_spt;		/* sectors per track */
	u_char	sfc_record;		/* recording density code */
};

/*
 * track switching codes
 */
#define TS_6MSEC	0	/* 6 msec */
#define TS_12MSEC	1	/* 12 msec */
#define TS_20MSEC	2	/* 20 msec */
#define TS_30MSEC	3	/* 30 msec */

/*
 * recording density codes
 */
#define	SDENSITY	0x00	/* single denisity */
#define DDENSITY	0xC0	/* double density */

static struct sf_characteristics default_characteristics = {
	40,
	TS_6MSEC,
	500,
	5,
	2,
	PC_SECSIZE/256,
	10,
	9,
	DDENSITY
};

static struct sf_characteristics current_characteristics;

/*
 * Return a pointer to this unit's unit structure.
 */
sfunitptr(md)
	register struct mb_device *md;
{
	return ((int)&sfunits[md->md_unit]);
}

/*
 * Attach device (boot time).
 */
sfattach(md)
	register struct mb_device *md;
{
	register struct scsi_unit *un;

	un = &sfunits[md->md_unit];
	un->un_md = md;
	un->un_mc = md->md_mc;
	un->un_unit = md->md_unit;
	un->un_target = TARGET(md->md_slave);
	un->un_lun = LUN(md->md_slave);
	un->un_ss = &scsi_unit_subr[TYPE(md->md_flags)];
}

static int
getlabel(dev)
	register dev_t dev;
{
	register struct scsi_unit *un;
	register char *bufp;
	register struct scsi_floppy *dsi;
	char c;

	un = &sfunits[SFUNIT(dev)];
	dsi = &sfloppy[SFUNIT(dev)];
	/*
	 * We don't want multiple opens going on at the same time.
	 */
	while (dsi->sf_flags & SF_LABELGET) {
		dsi->sf_flags |= SF_LABELWANT;
		(void) sleep((caddr_t)dsi, PRIBIO);
		if (un->un_present)
			return (1);
	}
	dsi->sf_flags |= SF_LABELGET;
	dsi->sf_nblk = 8;
	dsi->sf_spt = 8;
	dsi->sf_nhead = 1;
	if (sfcmd(dev, SC_TEST_UNIT_READY, 0, 0, (caddr_t)0)) {
		uprintf("sf%d: not online\n", SFNUM(un));
		goto out;
	}
	if (sfcmd(dev, SC_INIT_CHARACTERISTICS, 0,
	    sizeof(default_characteristics),
	    (caddr_t)&default_characteristics) ) {
		uprintf("sf%d: cannot init drive\n", SFNUM(un));
		goto out;
	}
	bufp = kmem_alloc(PC_SECSIZE);
	if (sfcmd(dev, SC_READ, PC_FATBLOCK, PC_SECSIZE, bufp)) {
		uprintf("sf%d: error reading label\n", SFNUM(un));
		kmem_free(bufp, PC_SECSIZE);
		goto out;
	}
	dsi->sf_mdb = bufp[0];
	c = bufp[1] & bufp[2];
	kmem_free(bufp, PC_SECSIZE);
	if (c != 0xFF) {
		uprintf("sf%d: bad label\n", SFNUM(un));
		goto out;
	}
	switch (dsi->sf_mdb) {
	case SS8SPT:
		dsi->sf_spt = 8;
		dsi->sf_nhead = 1;
		dsi->sf_nblk = 320;
		break;

	case DS8SPT:
		dsi->sf_spt = 8;
		dsi->sf_nhead = 2;
		dsi->sf_nblk = 640;
		break;

	case SS9SPT:
		dsi->sf_spt = 9;
		dsi->sf_nhead = 1;
		dsi->sf_nblk = 360;
		break;

	case DS9SPT:
		dsi->sf_spt = 9;
		dsi->sf_nhead = 2;
		dsi->sf_nblk = 720;
		break;

	default:
		uprintf("sf%d: bad label\n", SFNUM(un));
		goto out;
	}
	current_characteristics = default_characteristics;
	current_characteristics.sfc_spt = dsi->sf_spt;
	current_characteristics.sfc_nhead = dsi->sf_nhead;
	if (sfcmd(dev, SC_INIT_CHARACTERISTICS, 0,
	    sizeof(current_characteristics),
	    (caddr_t)&current_characteristics)) {
		uprintf("sf%d: cannot reinit drive\n", SFNUM(un));
	} else {
		un->un_present = 1;
	}
out:
	dsi->sf_flags &= ~SF_LABELGET;
	if (dsi->sf_flags & SF_LABELWANT) {
		dsi->sf_flags &= ~SF_LABELWANT;
		wakeup((caddr_t)dsi);
	}
	return (un->un_present);
}

/*ARGSUSED*/
sfopen(dev, flag)
	dev_t dev;
	int flag;
{
	register struct scsi_unit *un;
	register int unit;

	unit = SFUNIT(dev);
	if (unit >= nsfloppy) {
		return (ENXIO);
	}
	un = &sfunits[unit];
	if (un->un_mc == 0) {	/* never attached */
		return (ENXIO);
	}
	if (!un->un_present) {
		if (!getlabel(dev)) {
			return (EIO);
		}
	}
	return (0);
}

/*ARGSUSED*/
sfclose(dev, flag)
	dev_t dev;
	int flag;
{
	register struct scsi_unit *un;

	un = &sfunits[SFUNIT(dev)];
	/*
	 * make the driver get the label again on the next open
	 */
	un->un_present = 0;
}

sfsize(dev)
	register dev_t dev;
{
	register struct scsi_unit *un;

	un = &sfunits[SFUNIT(dev)];
	if (!un->un_present) {
		return (-1);
	}
	return (sfloppy[SFUNIT(dev)].sf_nblk);
}

sfstrategy(bp)
	register struct buf *bp;
{
	register struct scsi_unit *un;
	register daddr_t bn;
	register int unit, s;
	register struct buf *dp;
	register struct scsi_floppy *dsi;

	unit = SFUNIT(bp->b_dev);
	if (unit >= nsfloppy) {
		printf("sf%d: sfstrategy: invalid unit\n", unit);
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	un = &sfunits[unit];
	dsi = &sfloppy[unit];
	bn = dkblock(bp);
	if ((!un->un_present && bp != &un->un_sbuf) || (bn > dsi->sf_nblk)) {
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	if (un->un_present) {
		if (bn == dsi->sf_nblk) {	/* EOF */
			bp->b_resid = bp->b_bcount;
			iodone(bp);
			return;
		}
		bp->b_cylin = bn / (dsi->sf_spt * dsi->sf_nhead);
	} else {
		bp->b_cylin = 0;
	}
	dp = &un->un_utab;
	s = splx(pritospl(un->un_mc->mc_intpri));
	disksort((struct diskhd *)dp, bp);
	if (dp->b_active == 0) {
		(*un->un_c->c_ss->scs_ustart)(un);
		bp = &un->un_mc->mc_tab;
		if (bp->b_actf && bp->b_active == 0) {
			(*un->un_c->c_ss->scs_start)(un);
		}
	}
	(void) splx(s);
}

/*
 * Do a special command.
 */
sfcmd(dev, cmd, sector, len, addr)
	register dev_t dev;
	register int cmd, sector, len;
	register caddr_t addr;
{
	register struct scsi_unit *un;
	register struct buf *bp;
	register int s;

	un = &sfunits[SFUNIT(dev)];
	bp = &un->un_sbuf;
	s = splx(pritospl(un->un_mc->mc_intpri));
	while (bp->b_flags&B_BUSY) {
		bp->b_flags |= B_WANTED;
		(void) sleep((caddr_t)bp, PRIBIO);
	}
	bp->b_flags = B_BUSY|B_READ;
	(void) splx(s);
	un->un_scmd = cmd;
	bp->b_dev = dev;
	bp->b_blkno = sector;
	bp->b_un.b_addr = addr;
	bp->b_bcount = len;
	sfstrategy(bp);
	iowait(bp);
	bp->b_flags &= ~B_BUSY;
	if (bp->b_flags&B_WANTED)
		wakeup((caddr_t)bp);
	return (bp->b_flags & B_ERROR);
}

/*
 * Set up a transfer for the controller
 */
sfstart(bp, un)
	register struct buf *bp;
	register struct scsi_unit *un;
{
	register int nblk;
	register struct scsi_floppy *dsi;

	dsi = &sfloppy[SFUNIT(bp->b_dev)];
	un->un_blkno = dkblock(bp);
	if (bp == &un->un_sbuf) {
		un->un_cmd = un->un_scmd;
	} else if (bp->b_flags & B_READ) {
		un->un_cmd = SC_READ;
	} else {
		un->un_cmd = SC_WRITE;
	}
	if (un->un_cmd == SC_READ || un->un_cmd == SC_WRITE) {
		nblk = howmany(bp->b_bcount, PC_SECSIZE);
		un->un_count = MIN(nblk, dsi->sf_nblk - bp->b_blkno);
		bp->b_resid = bp->b_bcount - un->un_count * PC_SECSIZE;
		un->un_flags |= SC_UNF_DVMA;
	} else {
		if (un->un_cmd == SC_INIT_CHARACTERISTICS)
			un->un_flags |= SC_UNF_DVMA;
		un->un_count = bp->b_bcount;
	}
	return (1);
}

/*
 * Make a cdb for disk i/o.
 */
sfmkcdb(c, un)
	register struct scsi_ctlr *c;
	struct scsi_unit *un;
{
	register struct scsi_cdb *cdb;

	cdb = &c->c_cdb;
	bzero((caddr_t)cdb, sizeof (*cdb));
	un->un_dma_addr = 0;
	un->un_dma_count = 0;
	cdb->cmd = un->un_cmd;
	cdb->lun = un->un_lun;
	switch (un->un_cmd) {
	case SC_TEST_UNIT_READY:
	case SC_REZERO_UNIT:
	case SC_REQUEST_SENSE:
		break;
	case SC_SEEK:
		cdbaddr(cdb, un->un_blkno);
		break;
	case SC_READ:
	case SC_WRITE:
		cdbaddr(cdb, un->un_blkno);
		cdb->count = un->un_count;
		un->un_dma_addr = un->un_baddr;
		un->un_dma_count = un->un_count * PC_SECSIZE;
		break;
	case SC_INIT_CHARACTERISTICS:
		un->un_dma_addr = un->un_baddr;
		un->un_dma_count = un->un_count;
		break;
	default:
		panic("sfmkcdb");
		break;
	}
}

/*
 * Interrupt processing.
 */
sfintr(c, resid, error)
	register struct scsi_ctlr *c;
	register int resid, error;
{
	register struct scsi_unit *un;
	register struct buf *bp;
	register struct mb_device *md;

	un = c->c_un;
	bp = un->un_mc->mc_tab.b_actf->b_actf;
	md = un->un_md;
	if (md->md_dk >= 0) {
		dk_busy &= ~(1 << md->md_dk);
	}
	if (error == SE_FATAL) {
		if (bp == &un->un_sbuf &&
		    ((un->un_flags & SC_UNF_DVMA) == 0)) {
			(*c->c_ss->scs_done)(un->un_mc);
		} else {
			mbdone(un->un_mc);
			un->un_flags &= ~SC_UNF_DVMA;
		}
		bp->b_flags |= B_ERROR;
		printf("sf%d: SCSI FAILURE\n", SFNUM(un));
		(*c->c_ss->scs_off)(un);
		return;
	}
	if (error == SE_RETRYABLE || c->c_scb.chk || resid > 0) {
		sferror(c, un, bp);
		return;
	}
	if (c->c_cdb.cmd == SC_REZERO_UNIT &&
	    !(bp == &un->un_sbuf &&
	    un->un_scmd == SC_REZERO_UNIT)) {
		/* error recovery */
		sfmkcdb(c, un);
		if ((*c->c_ss->scs_cmd)(c, un, 1) == 0) {
			printf("sf%d sfintr: scsi cmd failed 1\n", SFNUM(un));
			(*c->c_ss->scs_off)(un);
		}
		return;
	}
	/* transfer worked */
	un->un_retries = un->un_restores = 0;
	if (un->un_sec_left) {	/* single sector stuff */
		un->un_sec_left--;
		un->un_baddr += PC_SECSIZE;
		un->un_blkno++;
		sfmkcdb(c, un);
		if ((*c->c_ss->scs_cmd)(c, un, 1) == 0) {
			printf("sf%d: sfintr: scsi cmd failed 2\n", SFNUM(un));
			(*c->c_ss->scs_off)(un);
		}
	} else if (bp == &un->un_sbuf &&
	    ((un->un_flags & SC_UNF_DVMA) == 0)) {
		(*c->c_ss->scs_done)(un->un_mc);
	} else {
		mbdone(un->un_mc);
		un->un_flags &= ~SC_UNF_DVMA;
	}
}

/*
 * Error handling.
 */
sferror(c, un, bp)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
	register struct buf *bp;
{

	if (un->un_present == 0) {	/* error trying to open */
		bp->b_flags |= B_ERROR;
		if (bp == &un->un_sbuf &&
		    ((un->un_flags & SC_UNF_DVMA) == 0)) {
			(*c->c_ss->scs_done)(un->un_mc);
		} else {
			mbdone(un->un_mc);
			un->un_flags &= ~SC_UNF_DVMA;
		}
	} else if (un->un_retries++ < MAX_RETRIES) {
		/* retry */
		if (sferrlvl >= EL_RETRY) {
			sferrmsg(c, un, bp, "retry");
		}
		if ((*c->c_ss->scs_cmd)(c, un, 1) == 0) {
			printf("sf%d: sferror: scsi cmd failed 2\n", SFNUM(un));
			(*c->c_ss->scs_off)(un);
		}
	} else if (un->un_restores++ < MAX_RESTORES) {
		/* retries exhausted, try restore */
		un->un_retries = 0;
		if (sferrlvl >= EL_REST) {
			sferrmsg(c, un, bp, "restore");
		}
		c->c_cdb.cmd = SC_REZERO_UNIT;
		cdbaddr(&c->c_cdb, 0);
		c->c_cdb.count = 0;
		un->un_dma_addr = un->un_dma_count = 0;
		if ((*c->c_ss->scs_cmd)(c, un, 1) == 0) {
			printf("sf%d: sferror: scsi cmd failed 3\n", SFNUM(un));
			(*c->c_ss->scs_off)(un);
		}
	} else {
		/* complete failure */
		if (sferrlvl >= EL_FAIL) {
			sferrmsg(c, un, bp, "failed");
		}
		(*c->c_ss->scs_off)(un);
		bp->b_flags |= B_ERROR;
		if (bp == &un->un_sbuf &&
		    ((un->un_flags & SC_UNF_DVMA) == 0)) {
			(*c->c_ss->scs_done)(un->un_mc);
		} else {
			mbdone(un->un_mc);
			un->un_flags &= ~SC_UNF_DVMA;
		}
	}
}

sfread(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return (sfrw(dev, uio, B_READ));
}

sfrw(dev, uio, direction)
	dev_t dev;
	struct uio *uio;
	int direction;
{
	register struct scsi_unit *un;
	register int unit;

	unit = SFUNIT(dev);
	if (unit >= nsfloppy) {
		return (ENXIO);
	}
	un = &sfunits[unit];
	if ((uio->uio_offset % DEV_BSIZE) != 0) {
		return (EINVAL);
	}
	if ((uio->uio_iov->iov_len % DEV_BSIZE) != 0) {
		return (EINVAL);
	}
	return (physio(sfstrategy, &un->un_rbuf, dev, direction, minphys,uio));
}

sfwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return (sfrw(dev, uio, B_WRITE));
}

/*ARGSUSED*/
sfioctl(dev, cmd, data, flag)
	register dev_t dev;
	register caddr_t data;
{
	register struct scsi_unit *un;
	register struct dk_info *inf;
	register int unit;

	unit = SFUNIT(dev);
	if (unit >= nsfloppy) {
		return (ENXIO);
	}
	un = &sfunits[unit];
	switch (cmd) {

	case DKIOCINFO:
		inf = (struct dk_info *)data;
		inf->dki_ctlr = getdevaddr(un->un_mc->mc_addr);
		inf->dki_unit = un->un_md->md_slave;
		inf->dki_ctype = DKC_XB1401;
		inf->dki_flags = DKI_FMTVOL;
		break;

	default:
		return (ENOTTY);
	}
	return (0);
}

static char *sf_class_00_errors[] = {
	"invalid sense code",
	"invalid sense code",
	"no seek complete",
	"invalid sense code",
	"drive not ready",
	"invalid sense code",
	"no track 00",
	"door open",
	"media not loaded",
};

static char *sf_class_01_errors[] = {
	"I.D. CRC error",
	"write fault",
	"write protected",
	"invalid sense code",
	"sector not found",
	"seek error",
	"(0x16) unformatted or bad format on drive",
	"(0x17) unformatted or bad format on drive",
	"invalid sense code",
	"two sided error",
	"wrong data mark found",
	"pad error",
	"invalid sense code",
	"lost data in FDC",
	"CRC error",
	"FDC failure",
};

static char *sf_class_02_errors[] = {
	"invalid command",
	"illegal block address",
	"invalid cdb",
	"invalid interleave",
};

static char *sf_class_03_errors[] = {
	"RAM error",
	"controller program memory checksum error",
};

static char **sf_errors[] = {
	sf_class_00_errors,
	sf_class_01_errors,
	sf_class_02_errors,
	sf_class_03_errors,
};

int sf_errct[] = {
	sizeof sf_class_00_errors / sizeof (char *),
	sizeof sf_class_01_errors / sizeof (char *),
	sizeof sf_class_02_errors / sizeof (char *),
	sizeof sf_class_03_errors / sizeof (char *),
};

char *sf_cmds[] = {
	"test unit ready",
	"rezero unit",
	"bad cmd",
	"request sense",
	"bad cmd",
	"bad cmd",
	"bad cmd",
	"bad cmd",
	"read",
	"bad cmd",
	"write",
	"seek",
	"initialize characteristics",
};

/*ARGSUSED*/
sferrmsg(c, un, bp, action)
	register struct scsi_ctlr *c;
	struct scsi_unit *un;
	struct buf *bp;
	char *action;
{
	char *sensemsg, *cmdname;
	register struct scsi_sense *sense;
	register int blkno;

	sense = c->c_sense;
	if (c->c_scb.chk == 0) {
		sensemsg = "invalid sense code";
	} else if (sense->class <= 3) {
		if (sense->code < sf_errct[sense->class]) {
			sensemsg = sf_errors[sense->class][sense->code];
		} else {
			sensemsg = "invalid sense code";
		}
	} else {
		sensemsg = "invalid sense class";
	}
	if (un->un_cmd < sizeof(sf_cmds)) {
		cmdname = sf_cmds[un->un_cmd];
	} else {
		cmdname = "bad cmd";
	}
	blkno = (sense->high_addr << 16) | (sense->mid_addr << 8) |
	    sense->low_addr;
	printf("sf%d: %s %s (%s) blk %d\n", SFNUM(un),
	    cmdname, action, sensemsg, blkno);
}
#endif NSF > 0
