#ifndef lint
static	char sccsid[] = "@(#)sd.c	1.9 87/03/02	Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * SCSI driver for SCSI disks.
 */

#include "sd.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dk.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/map.h"
#include "../h/vmmac.h"
#include "../h/ioctl.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/dkbad.h"

#include "../machine/pte.h"
#include "../machine/psl.h"
#include "../machine/mmu.h"
#include "../machine/cpu.h"

#include "../sun/dklabel.h"
#include "../sun/dkio.h"

#include "../sundev/mbvar.h"
#include "../sundev/screg.h"
#include "../sundev/sireg.h"
#include "../sundev/scsi.h"
#include "../sundev/sdreg.h"

#define	MAX_RETRIES 		3
#define	MAX_LABEL_RETRIES  	3
#define	MAX_RESTORES 		1 
#define	MAX_LABEL_RESTORES 	1

#define	LPART(dev)	(dev % NLPART)
#define	SDUNIT(dev)	((dev >> 3) % NUNIT)
#define	SDNUM(un)	(un - sdunits)

#define	SD_RECV_DATA_CMD(cmd)	((cmd == SC_READ) || (cmd == SC_REQUEST_SENSE) \
				|| (cmd == SC_INQUIRY))

#define b_cylin b_resid
#define	SECSIZE	512

#define BUSY_RETRY 1000

/*
 * Error message control.
 */
#define	EL_RETRY	3
#define EL_REST		2
#define	EL_FAIL		1
int sderrlvl = EL_FAIL;

extern struct scsi_unit sdunits[];
extern struct scsi_unit_subr scsi_unit_subr[];
extern struct scsi_disk sdisk[];
extern int nsdisk;

/*
 * Return a pointer to this unit's unit structure.
 */
sdunitptr(md)
	register struct mb_device *md;
{
	return ((int)&sdunits[md->md_unit]);
}

int sd_delay_amt = 0xf0000;		/* so we can patch the value */
u_char sdattach_blab = 0;		/* blabbermouth mode */
u_char sd_recov_error_flag = 0;		/* report recoverable errors */

/*
 * Attach device (boot time).
 */
sdattach(md)
	register struct mb_device *md;
{
	register struct scsi_unit *un;
	register struct dk_label *l;
	register int nbusy;
	struct scsi_inquiry_data *sid;
	struct scsi_disk *dsi;

	/*
	 * link in, fill in unit struct.
	 */
	un = &sdunits[md->md_unit];
	un->un_md = md;
	un->un_mc = md->md_mc;
	un->un_unit = md->md_unit;
	un->un_target = TARGET(md->md_slave);
	un->un_lun = LUN(md->md_slave);
	un->un_ss = &scsi_unit_subr[TYPE(md->md_flags)];

	nbusy = 0;

	if (sdattach_blab)
		printf("sdattach: looking for lun %d on target %d: ",
			LUN(md->md_slave), TARGET(md->md_slave));

	/*
	 * Test for unit ready.
	 */
	for (;;) {
		/*
		 * This command seems to take a long time
		 * on the ACB 4520, so we wait a while.
		 */
		DELAY(sd_delay_amt);
		if (simple(un, SC_TEST_UNIT_READY, 0, 0, 0)) {
			break;
		} else if (un->un_c->c_scb.busy && !un->un_c->c_scb.is 
			&&  nbusy++ < BUSY_RETRY) {
			continue;
		} else {
			if (nbusy >= BUSY_RETRY) {
				printf("sd%d: scsi continuously busy\n",
				    SDNUM(un));
			} else {
				/*
				 * some ESDI controllers only wake up
				 * on the second test.
				 */
				if (simple(un, SC_TEST_UNIT_READY, 0, 0, 0))
					break;
			}
			if (sdattach_blab)
				printf(" not found\n");
			return;
		}
	}

	if (sdattach_blab)
		printf(" found\n");

	/*
	 * Now let's see what kind of controller it is.
	 */

	dsi = &sdisk[un->un_unit];

	/* Allocate space for inquiry data in Multibus memory */
	sid = (struct scsi_inquiry_data *)rmalloc(iopbmap, 
		(long)(sizeof(struct scsi_inquiry_data) + 2));
	if (sid == NULL) {
		printf("sd%d: sdattach: no space for inquiry data\n",
		    SDNUM(un));
		return;
	}

	if ((int)sid & 0x01)
		((caddr_t)sid)++;

	bzero((caddr_t)sid, (u_int)sizeof(struct scsi_inquiry_data));

	if (simple(un, SC_INQUIRY, (char *) sid - DVMA, 0, 
			(int)sizeof(struct scsi_inquiry_data))) {
		/* Only CCS controllers support Inquiry command */
		if (bcmp(sid->pid, "MD21", 4) == 0)
			dsi->dk_ctype = CTYPE_MD21;
		else
			dsi->dk_ctype = CTYPE_CCS;
	} else {
		/* non-CCS, assume Adaptec */
		dsi->dk_ctype = CTYPE_ACB4000;
	}
	rmfree(iopbmap, (long)sizeof(struct scsi_inquiry_data), (long)sid);

	/*
	 * Ok, it's ready - try to read and use the label.
	 */

	/* Allocate space for label in Multibus memory */
	l = (struct dk_label *)rmalloc(iopbmap, (long)(SECSIZE +2));
	if (l == NULL) {
		printf("sd%d: sdattach: no space for disk label\n",
		    SDNUM(un));
		return;
	}

	if ((int)l & 0x01)
		((caddr_t)l)++;

	l->dkl_magic = 0;

	if (getlabel(un, l) && islabel(un, l)) {
		/*
		 * Note that uselabel() marks the unit as present...
		 */
		uselabel(un, l);
	}

	rmfree(iopbmap, (long)SECSIZE, (long)l);
}

/*
 * Read the label from the disk.
 * Return true if read was ok, false otherwise.
 */
static
getlabel(un, l)
	register struct scsi_unit *un;
	register struct dk_label *l;
{
	register int retries, restores;

	for (restores = 0; restores < MAX_LABEL_RESTORES; restores++) {
		for (retries = 0; retries < MAX_LABEL_RETRIES; retries++) {
			if (simple(un, SC_READ, (char *) l - DVMA, 0, 1)) {
				return (1);
			}
		}
		(void) simple(un, SC_REZERO_UNIT, 0, 0, 0);
	}
	if (sdattach_blab)
		printf("getlabel: could not read label\n");
	return (0);
}

/*
 * Check the label for righteousity.
 */
static
islabel(un, l)
	register struct scsi_unit *un;
	register struct dk_label *l;
{
	if (l->dkl_magic != DKL_MAGIC) {
		return (0);
	}
	if (!ck_cksum(l)) {
		printf("sd%d: corrupt label\n", SDNUM(un));
		return (0);
	}
	return (1);
}

/*
 * Check the checksum of the label
 */
static
ck_cksum(l)
	register struct dk_label *l;
{
	short *sp, sum = 0;
	short count = sizeof(struct dk_label)/sizeof(short);

	sp = (short *)l;
	while (count--)  {
		sum ^= *sp++;
	}
	return (sum ? 0 : 1);
}

/*
 * Snarf yummos from validated label.
 * Responsible for autoconfig message, interestingly enough.
 * Also marks the unit as present.
 */
static
uselabel(un, l)
	register struct scsi_unit *un;
	register struct dk_label *l;
{
	register int i, intrlv;
	register struct scsi_disk *dsi;

	printf("sd%d: <%s>\n", SDNUM(un), l->dkl_asciilabel);

	un->un_present = 1;			/* "they're here..." */

	/*
	 * Fill in disk geometry from label.
	 */
	dsi = &sdisk[un->un_unit];
	dsi->un_g.dkg_ncyl = l->dkl_ncyl;
	dsi->un_g.dkg_bcyl = 0;
	dsi->un_g.dkg_nhead = l->dkl_nhead;
	dsi->un_g.dkg_bhead = l->dkl_bhead;
	dsi->un_g.dkg_nsect = l->dkl_nsect;
	dsi->un_g.dkg_gap1 = l->dkl_gap1;
	dsi->un_g.dkg_gap2 = l->dkl_gap2;
	dsi->un_g.dkg_intrlv = l->dkl_intrlv;

	/*
	 * Fill in partition table.
	 */
	for (i = 0; i < NLPART; i++)
		dsi->un_map[i] = l->dkl_map[i];
	/*
	 * Diddle stats if neccessary.
	 */
	if (un->un_md->md_dk >= 0) {
		intrlv = dsi->un_g.dkg_intrlv;
		if (intrlv <= 0 || intrlv >= dsi->un_g.dkg_nsect) {
			intrlv = 1;
		}
		dk_bps[un->un_md->md_dk] = 
			(SECSIZE * 60 * dsi->un_g.dkg_nsect) / intrlv;
	}
}

/*
 * Run a command in polled mode.
 * Return true if successful, false otherwise.
 */
static
simple(un, cmd, dma_addr, secno, nsect)
	register struct scsi_unit *un;
	register int cmd, dma_addr, secno, nsect;
{
	register struct scsi_cdb *cdb;
	register struct scsi_ctlr *c;
	register int count;

	c = un->un_c;
	/*
	 * Grab and clear the command block.
	 */
	cdb = &c->c_cdb;
	bzero((caddr_t)cdb, sizeof(struct scsi_cdb));

	/*
	 * Plug in command block values.
	 */
	cdb->cmd = cmd;
	if (SD_RECV_DATA_CMD(cmd))
		un->un_flags |= SC_UNF_RECV_DATA;
	else
		un->un_flags &= ~SC_UNF_RECV_DATA;

	c->c_un = un;
	cdb->lun = un->un_lun;
	cdbaddr(cdb, secno);
	cdb->count = nsect;
	un->un_dma_addr = dma_addr;
	if (cmd == SC_INQUIRY)
		un->un_dma_count = nsect;
	else
		un->un_dma_count = nsect * SECSIZE;

	/*
	 * Fire up the pup.
	 */
	if ((*c->c_ss->scs_cmd)(c, un, 0)) {
		if ((*c->c_ss->scs_cmd_wait)(c)) {
			count = (*c->c_ss->scs_dmacount)(c);
			* (char *) &c->c_scb = 0;
			if ((*c->c_ss->scs_getstat)(un)) {
				if (count == 0) {
					if (c->c_scb.chk == 0 && 
					    c->c_scb.is == 0 &&
					    c->c_scb.busy == 0) {
						return (1);
					}
				}
			}
		}
	}
	return (0);
}

/*ARGSUSED*/
/*
 * This routine opens a disk.
 * Note that we can handle disks that make an appearance after boot time.
 */
sdopen(dev, flag)
	dev_t dev;
	int flag;
{
	register struct scsi_unit *un;
	register struct dk_label *l;
	register int unit;
	register struct scsi_disk *dsi;
	int memall();

	unit = SDUNIT(dev);
	if (unit >= nsdisk) {
		return (ENXIO);
	}
	un = &sdunits[unit];
	dsi = &sdisk[unit];
	if (un->un_mc == 0) {	/* never attached */
		return (ENXIO);
	}
	if (!un->un_present) {
		/*
		 * Didn't see it at autoconfig time? 
		 * Let's look again..
		 */
		dsi->un_g.dkg_nsect = dsi->un_g.dkg_nhead = 1;   /* for strat */
		if (sdcmd(dev, SC_TEST_UNIT_READY, 0, 0, (caddr_t)0)) {
			return (EIO);
		}
		/* Allocate space for label */
		l = (struct dk_label *)wmemall(memall, SECSIZE);
		if (l == NULL) {
			printf("sd%d: no space for disk label\n",
			    SDNUM(un));
			return (EIO);
		}
		if (sdcmd(dev, SC_READ, 0, SECSIZE, (caddr_t)l)) {
			uprintf("sd%d: error reading label\n", SDNUM(un));
			wmemfree((caddr_t)l, SECSIZE);
			return (EIO);
		}
		if (islabel(un, l)) {
			uselabel(un, l);
		} else {
			wmemfree((caddr_t)l, SECSIZE);
			return (EIO);
		}
		wmemfree((caddr_t)l, SECSIZE);
	}
	return (0);
}

sdsize(dev)
	register dev_t dev;
{
	register struct scsi_unit *un;
	register struct dk_map *lp;
	register struct scsi_disk *dsi;

	un = &sdunits[SDUNIT(dev)];
	if (!un->un_present) {
		return (-1);
	}
	dsi = &sdisk[SDUNIT(dev)];
	lp = &dsi->un_map[LPART(dev)];
	return (lp->dkl_nblk);
}

sdstrategy(bp)
	register struct buf *bp;
{
	register struct scsi_unit *un;
	register struct dk_map *lp;
	register daddr_t bn;
	register int unit, s;
	register struct buf *dp;
	register struct scsi_disk *dsi;

	unit = dkunit(bp);
	if (unit >= nsdisk) {
		printf("sd%d: sdstrategy: invalid unit\n", unit);
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	un = &sdunits[unit];
	dsi = &sdisk[unit];
	lp = &dsi->un_map[LPART(bp->b_dev)];
	bn = dkblock(bp);

	if ((!un->un_present && bp != &un->un_sbuf)
	    || bn > lp->dkl_nblk || (un->un_present && lp->dkl_nblk == 0)) {
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	if (un->un_present) {
		if (bn == lp->dkl_nblk) {	/* EOF */
			bp->b_resid = bp->b_bcount;
			iodone(bp);
			return;
		}
	}
	bp->b_cylin = bn / (dsi->un_g.dkg_nsect * dsi->un_g.dkg_nhead);
	bp->b_cylin += lp->dkl_cylno + dsi->un_g.dkg_bcyl;
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
sdcmd(dev, cmd, sector, len, addr)
	register dev_t dev;
	register int cmd, sector, len;
	register caddr_t addr;
{
	register struct scsi_unit *un;
	register struct buf *bp;
	register int s;

	un = &sdunits[SDUNIT(dev)];
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
	sdstrategy(bp);
	iowait(bp);
	bp->b_flags &= ~B_BUSY;
	if (bp->b_flags&B_WANTED)
		wakeup((caddr_t)bp);
	return (bp->b_flags & B_ERROR);
}

/*
 * Set up a transfer for the controller
 */
sdstart(bp, un)
	register struct buf *bp;
	register struct scsi_unit *un;
{
	register struct dk_map *lp;
	register int nblk;
	register struct scsi_disk *dsi;

	dsi = &sdisk[dkunit(bp)];
	lp = &dsi->un_map[LPART(bp->b_dev)];
	un->un_blkno = dkblock(bp) + 
		lp->dkl_cylno * dsi->un_g.dkg_nhead * dsi->un_g.dkg_nsect;
	if (bp == &un->un_sbuf) {
		un->un_cmd = un->un_scmd;
	} else if (bp->b_flags & B_READ) {
		un->un_cmd = SC_READ;
	} else {
		un->un_cmd = SC_WRITE;
	}
	if (un->un_cmd == SC_READ || un->un_cmd == SC_WRITE) {
		nblk = howmany(bp->b_bcount, SECSIZE);
		un->un_count = MIN(nblk, lp->dkl_nblk - bp->b_blkno);
		bp->b_resid = bp->b_bcount - un->un_count * SECSIZE;
		un->un_flags |= SC_UNF_DVMA;
	} else {
		un->un_count = 0;
	}
	return (1);
}

/*
 * Make a cdb for disk i/o.
 */
sdmkcdb(c, un)
	register struct scsi_ctlr *c;
	struct scsi_unit *un;
{
	register struct scsi_cdb *cdb;

	cdb = &c->c_cdb;
	bzero((caddr_t)cdb, sizeof (*cdb));

	if (un->un_flags & SC_UNF_GET_SENSE) {
		cdb->cmd = SC_REQUEST_SENSE;
		cdb->lun = un->un_lun;
		cdb->count = sizeof (struct scsi_sense);
		un->un_flags |= SC_UNF_RECV_DATA;
		un->un_dma_addr = (int)c->c_sense - (int)DVMA;
		un->un_dma_count = sizeof (struct scsi_sense);
		return;
	}

	cdb->cmd = un->un_cmd;
	cdb->lun = un->un_lun;
	cdbaddr(cdb, un->un_blkno);
	cdb->count = un->un_count;
	un->un_dma_addr = un->un_baddr;
	if (SD_RECV_DATA_CMD(un->un_cmd))
		un->un_flags |= SC_UNF_RECV_DATA;
	else
		un->un_flags &= ~SC_UNF_RECV_DATA;
	un->un_dma_count = un->un_count * SECSIZE;

	un->un_saved_cmd.saved_cdb = *cdb;	/* for later reference... */
}

/*
 * Interrupt processing.
 */
sdintr(c, resid, error)
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
		if (bp == &un->un_sbuf  &&
		    ((un->un_flags & SC_UNF_DVMA) == 0)) {
			(*c->c_ss->scs_done)(un->un_mc);
		} else {
			mbdone(un->un_mc);
			un->un_flags &= ~SC_UNF_DVMA;
		}
		bp->b_flags |= B_ERROR;
		printf("sd%d: SCSI FAILURE\n", SDNUM(un));
		(*c->c_ss->scs_off)(un);
		return;
	}

	if ((c->c_scb.chk) && !(un->un_flags & SC_UNF_GET_SENSE) &&
	    IS_SCSI3(c)) {
		/*
		 * Command failed, need to run request sense command.
		 */

		un->un_flags |= SC_UNF_GET_SENSE;
		un->un_saved_cmd.saved_scb = c->c_scb;
		un->un_saved_cmd.saved_cdb = c->c_cdb;
		un->un_saved_cmd.saved_resid = resid;
		un->un_saved_cmd.saved_dma_addr = un->un_dma_addr;
		un->un_saved_cmd.saved_dma_count = un->un_dma_count;

		/* note that sistart() will call mkcdb(), which will notice
		* that the flag is set and not do the copy of the cdb,
		* doing a request sense rather than the normal command.
		*/

		(*un->un_c->c_ss->scs_go)(un->un_mc);

		return;
	}

	if ((un->un_flags & SC_UNF_GET_SENSE) && IS_SCSI3(c)) {

		if (c->c_scb.chk) {
			/* should never happen... */
			printf("sdintr: request sense failed\n");
		}

		/* were running a request sense cmd - restore old cmd */

		c->c_scb = un->un_saved_cmd.saved_scb;
		c->c_cdb = un->un_saved_cmd.saved_cdb;
		resid = un->un_saved_cmd.saved_resid;
		un->un_dma_addr = un->un_saved_cmd.saved_dma_addr;
		un->un_dma_count = un->un_saved_cmd.saved_dma_count;

		un->un_flags &= ~SC_UNF_GET_SENSE;
	}

	if (error == SE_RETRYABLE || c->c_scb.chk || resid > 0) {
		if (sderror(c, un, bp)) {
			/* real error requiring error recovery */
			return;
		} else {
			/* a psuedo-error: soft error reported by ctlr */
			goto done;
		}
	} 

	if (c->c_cdb.cmd == SC_REZERO_UNIT && 
	    !(bp == &un->un_sbuf && 
	    un->un_scmd == SC_REZERO_UNIT)) {
		/* error recovery */
		sdmkcdb(c, un);
		if ((*c->c_ss->scs_cmd)(c, un, 1) == 0) {
			printf("sd%d sdintr: scsi cmd failed 1\n", SDNUM(un));
			(*c->c_ss->scs_off)(un);
		}

		return;
	}

done:
	/* transfer worked */
	un->un_retries = un->un_restores = 0;
	if (un->un_sec_left) {	/* single sector stuff */
		un->un_sec_left--;
		un->un_baddr += SECSIZE;
		un->un_blkno++;
		sdmkcdb(c, un);
		if ((*c->c_ss->scs_cmd)(c, un, 1) == 0) {
			printf("sd%d: sdintr: scsi cmd failed 2\n", SDNUM(un));
			(*c->c_ss->scs_off)(un);
		}
	} else if (bp == &un->un_sbuf  &&
	    ((un->un_flags & SC_UNF_DVMA) == 0)) {
		(*c->c_ss->scs_done)(un->un_mc);
	} else {
		mbdone(un->un_mc);
		un->un_flags &= ~SC_UNF_DVMA;
	}
}


/*
 * Error handling.
 * returns 0 for psuedo-error, 1 for real error.
 */
sderror(c, un, bp)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
	register struct buf *bp;
{
	register struct scsi_disk *dsi = &sdisk[dkunit(bp)];

	if (un->un_present == 0) {	
		/* error trying to open */
		bp->b_flags |= B_ERROR;
		if (bp == &un->un_sbuf &&
		    ((un->un_flags & SC_UNF_DVMA) == 0)) {
			(*c->c_ss->scs_done)(un->un_mc);
		} else {
			mbdone(un->un_mc);
			un->un_flags &= ~SC_UNF_DVMA;
		}
	} else if (c->c_scb.chk && c->c_sense->class == 1 &&
	    c->c_sense->code == 5 && un->un_count > 1) {
		/* Seek errors - try single sectors (Adaptec bug) */
		un->un_sec_left = un->un_count - 1;
		un->un_count = 1;
		sdmkcdb(c, un);
		if ((*c->c_ss->scs_cmd)(c, un, 1) == 0) {
			printf("sd%d: sderror: scsi cmd failed 1\n", SDNUM(un));
			(*c->c_ss->scs_off)(un);
		}
	} else if (c->c_scb.chk && 
		c->c_sense->class == SC_CLASS_EXTENDED_SENSE &&
		(dsi->dk_ctype == CTYPE_MD21 || dsi->dk_ctype == CTYPE_CCS)) {

		register struct scsi_ext_sense *s 
					= (struct scsi_ext_sense *)c->c_sense;
		switch (s->key) { 

		case SC_RECOVERABLE_ERROR:
			if (sd_recov_error_flag) {
				switch (s->error_code) {

				case SC_ERR_READ_RECOV_RETRIES:
					sderrmsg(c, un, bp, 
					"ctlr corrected soft error (retried)");
					break;

				case SC_ERR_READ_RECOV_ECC:
					sderrmsg(c, un, bp,
					"ctlr corrected soft error (ECC)");
					break;
				
				default:
					sderrmsg(c, un, bp, 
					"ctlr corrected soft error (unknown)");
				}
			}

			return (0);             /* a psuedo-error */

		case SC_UNIT_ATTENTION:
			/*
			 * Need to retry the command.
			 * Note that we may retry forever in 
			 * the not very likely event that we keep
			 * getting a unit attention.
			 */
			if ((*c->c_ss->scs_cmd)(c, un, 1) == 0) {
				printf("sd%d: sderror: scsi cmd failed 2\n", 
								SDNUM(un));
				(*c->c_ss->scs_off)(un);
			}
			break;
		}
	} else if (un->un_retries++ < MAX_RETRIES) {
		/* retry */
		if (sderrlvl >= EL_RETRY) {
			sderrmsg(c, un, bp, "retry");
		}
		if ((*c->c_ss->scs_cmd)(c, un, 1) == 0) {
			printf("sd%d: sderror: scsi cmd failed 3\n", SDNUM(un));
			(*c->c_ss->scs_off)(un);
		}
	} else if (un->un_restores++ < MAX_RESTORES) {
		/* retries exhausted, try restore */
		un->un_retries = 0;
		if (sderrlvl >= EL_REST) {
			sderrmsg(c, un, bp, "restore");
		}
		c->c_cdb.cmd = SC_REZERO_UNIT;
		cdbaddr(&c->c_cdb, 0);
		c->c_cdb.count = 0;
		un->un_dma_addr = un->un_dma_count = 0;
		if ((*c->c_ss->scs_cmd)(c, un, 1) == 0) {
			printf("sd%d: sderror: scsi cmd failed 4\n", SDNUM(un));
			(*c->c_ss->scs_off)(un);
		}
	} else {
		/* complete failure */
		if (sderrlvl >= EL_FAIL) {
			sderrmsg(c, un, bp, "failed");
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
	return (1);		/* a real error */
}

sdread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	return (sdrw(dev, uio, B_READ));
}

sdrw(dev, uio, direction)
	dev_t dev;
	struct uio *uio;
	int direction;
{
	register struct scsi_unit *un;
	register int unit;
	
	unit = SDUNIT(dev);
	if (unit >= nsdisk) {
	    	return (ENXIO);
	}
	un = &sdunits[unit];
	if ((uio->uio_offset % DEV_BSIZE) != 0) {
		return (EINVAL);
	}
	if ((uio->uio_iov->iov_len % DEV_BSIZE) != 0) {
		return (EINVAL);
	}
	return (physio(sdstrategy, &un->un_rbuf, dev, direction, minphys,uio));
}

sdwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	return (sdrw(dev, uio, B_WRITE));
}

/*ARGSUSED*/
sdioctl(dev, cmd, data, flag)
	register dev_t dev;
	register caddr_t data;
{
	register struct scsi_unit *un;
	register struct dk_map *lp;
	register struct dk_info *inf;
	register int unit;
	register struct scsi_disk *dsi;

	unit = SDUNIT(dev);
	if (unit >= nsdisk) {
	    	return (ENXIO);
	}
	un = &sdunits[unit];
	dsi = &sdisk[unit];
	lp = &dsi->un_map[LPART(dev)];
	switch (cmd) {

	case DKIOCINFO:
		inf = (struct dk_info *)data;
		inf->dki_ctlr = getdevaddr(un->un_mc->mc_addr);
		inf->dki_unit = un->un_md->md_slave;

		switch (dsi->dk_ctype) {
		case CTYPE_MD21:
			inf->dki_ctype = DKC_MD21;
			break;
		case CTYPE_ACB4000:
		default:
			inf->dki_ctype = DKC_ACB4000;
			break;
		}

		inf->dki_flags = DKI_FMTVOL;
		break;

	case DKIOCGGEOM:
		*(struct dk_geom *)data = dsi->un_g;
		break;

	case DKIOCSGEOM:
		if (!suser())
			return (u.u_error);
		dsi->un_g = *(struct dk_geom *)data;
		break;

	case DKIOCGPART:
		*(struct dk_map *)data = *lp;
		break;

	case DKIOCSPART:
		if (!suser())
			return (u.u_error);
		*lp = *(struct dk_map *)data;
		break;

	default:
		return (ENOTTY);
	}
	return (0);
}

/*
 * For dumping core.
 */
sddump(dev, addr, blkno, nblk)
	register dev_t dev;
	register caddr_t addr;
	register daddr_t blkno, nblk;
{
	register struct scsi_unit *un;
	register struct dk_map *lp;
	static int first_time = 1;
	register struct scsi_disk *dsi;

	un = &sdunits[SDUNIT(dev)];
	if (un->un_present == 0) {
		return (ENXIO);
	}
	dsi = &sdisk[SDUNIT(dev)];
	lp = &dsi->un_map[LPART(dev)];
	if (blkno >= lp->dkl_nblk || (blkno + nblk) > lp->dkl_nblk) {
		return (EINVAL);
	}
	blkno += lp->dkl_cylno * dsi->un_g.dkg_nhead * dsi->un_g.dkg_nsect;
	if (first_time) {
		(*un->un_c->c_ss->scs_reset)(un->un_c); /* clr state - prevent err msg */
		first_time = 0;
	}
	if (simple(un, SC_WRITE, (int)(addr-DVMA), (int) blkno, (int) nblk)) {
		return (0);
	} else {
		return (EIO);
	}
}

char	*class_00_errors[] = {
	"no sense",
	"no index signal",
	"no seek complete",
	"write fault",
	"drive not ready",
	"drive not selected",
	"no track 00",
	"multiple drives selected",
	"no address acknowledged",
	"media not loaded",
	"insufficient capacity",
};

char	*class_01_errors[] = {
	"I.D. CRC error",
	"unrecoverable data error",
	"I.D. address mark not found",
	"data address mark not found",
	"record not found",
	"seek error",
	"DMA timeout error",
	"write protected",
	"correctable data check",
	"bad block found",
	"interleave error",
	"data transfer incomplete",
	"unformatted or bad format on drive",
	"self test failed",
	"defective track (media errors)",
};

char	*class_02_errors[] = {
	"invalid command",
	"illegal block address",
	"aborted",
	"volume overflow",
};

char	**sc_errors[] = {
	class_00_errors,
	class_01_errors,
	class_02_errors,
	0, 0, 0, 0,
};

int	sc_errct[] = {
	sizeof class_00_errors / sizeof (char *),
	sizeof class_01_errors / sizeof (char *),
	sizeof class_02_errors / sizeof (char *),
	0, 0, 0, 0,
};

char	*sc_ext_sense_keys [] = {
	"no sense",
	"recoverable error",
	"not ready",
	"media error",
	"hardware error",
	"illegal request",
	"media change",
	"write protect",
	"diagnostic unique",
	"vendor unique",
	"power up failed",
	"aborted command",
	"equal",
	"volume overflow",
};

#define N_EXT_SENSE_KEYS \
	(sizeof(sc_ext_sense_keys)/sizeof(sc_ext_sense_keys[0]))

char *sd_cmds[] = {
	"test unit ready",
	"rezero unit",
	"<bad cmd>",
	"request sense",
	"<bad cmd>",
	"<bad cmd>",
	"<bad cmd>",
	"<bad cmd>",
	"read",
	"<bad cmd>",
	"write",
	"seek",
};

sderrmsg(c, un, bp, action)
	register struct scsi_ctlr *c;
	struct scsi_unit *un;
	struct buf *bp;
	char *action;
{
	char *sensemsg, *cmdname;
	register struct scsi_sense *sense;
#define	ext_sense	((struct scsi_ext_sense* ) sense)
	register struct dk_map *lp;
	register int blkno;
	register struct scsi_disk *dsi = &sdisk[dkunit(bp)];
	int abs_blkno;			/* abs blk # (from beginning of disk) */
	int badblkno_flag = 0;		/* if true, blkno is meaningless */

	sense = c->c_sense;
	if (c->c_scb.chk == 0) {
		sensemsg = "no sense";
		badblkno_flag = 1;
	} else if (sense->class <= 6) {
		if (sense->code < sc_errct[sense->class]) {
			sensemsg = sc_errors[sense->class][sense->code];
		} else {
			sensemsg = "invalid sense code";
			badblkno_flag = 1;
		}
	} else if (sense->class == 7) {
		if (ext_sense->key < N_EXT_SENSE_KEYS) {
			sensemsg = sc_ext_sense_keys[ext_sense->key];
		} else {
			sensemsg = "invalid sense code";
			badblkno_flag = 1;
		}
	} else {
		sensemsg = "invalid sense class";
		badblkno_flag = 1;
	}
	if (un->un_cmd < sizeof(sd_cmds)) {
		cmdname = sd_cmds[un->un_cmd];
	} else {
		cmdname = "unknown cmd";
	}

	/*
	 * The position of the offending block number information 
	 * in the sense structure is controller-dependent.
	 */

	if (dsi->dk_ctype == CTYPE_ACB4000)
		blkno = (sense->high_addr << 16) | (sense->mid_addr << 8) |
			    sense->low_addr;
	else 
		/* assume CCS */
		blkno = (ext_sense->info_1 << 24) | (ext_sense->info_2 << 16) |
			(ext_sense->info_3 << 8) | ext_sense->info_4;

	lp = &dsi->un_map[LPART(bp->b_dev)];

	abs_blkno = blkno;
	blkno -= lp->dkl_cylno * dsi->un_g.dkg_nhead * dsi->un_g.dkg_nsect;

	printf("sd%d%c: %s %s (%s - class %x code %x) ", 
		SDNUM(un), LPART(bp->b_dev) + 'a', cmdname, action, 
		sensemsg, sense->class, sense->code);
	if (badblkno_flag)
		printf("unknown blk\n");
	else
		printf("blk %d (abs blk %d)\n", blkno, abs_blkno);
}
