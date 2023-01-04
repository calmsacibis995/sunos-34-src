#ifndef lint
static	char sccsid[] = "@(#)st.c	1.8 87/04/14	Copyr 1986 Sun Micro";
#endif
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "st.h"
#if NST > 0

/*
 * Driver for Sysgen SC400 and Emulex MT02 SCSI tape controller.
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

#include "../sun/dklabel.h"
#include "../sun/dkio.h"
#include "../machine/psl.h"
#include "../machine/mmu.h"
#include "../machine/cpu.h"
#include "../sundev/mbvar.h"
#include "../sundev/screg.h"
#include "../sundev/sireg.h"
#include "../sundev/scsi.h"
#include "../sundev/streg.h"

#define	INF		1000000000
/*
 * Max # of buffers outstanding per unit.
 */
#define	MAXSTBUF	3

/*
 * Bits in minor device.
 */
#define	STUNIT(dev)	(minor(dev) & 03)
#define	T_NOREWIND	04
#define QIC_24		8

/*
 * Archive sense data from Sysgen sense data.
 */
#define	AR_SENSE(p)	(((short *)(p))[2] & 0xffff)

/*
 * Archive sense bits definition.
 */
#define SENSE_BITS \
"\020\17NoCart\16NoDrive\15WriteProt\14EndMedium\13HardErr\12WrongBlock\
\11FileMark\7InvCmd\6NoData\5Flaking\4BOT\0034\0022\1GotReset"

/*
 * Commands which transfer data in to the host.
 */
#define ST_RECV_DATA_CMD(cmd)   ((cmd == SC_READ) || (cmd == SC_REQUEST_SENSE)                || (cmd == SC_INQUIRY) || (cmd == SC_READ_XSTATUS_CIPHER))

extern struct scsi_unit stunits[];
extern struct scsi_unit_subr scsi_unit_subr[];
extern struct scsi_tape stape[];
extern int nstape;
extern int scsi_disre_enable;

#ifdef STDEBUG
int st_debug = 0;
#endif STDEBUG

/*
 * Return a pointer to this unit's unit structure.
 */
stunitptr(md)
	register struct mb_device *md;
{
	return ((int)&stunits[md->md_unit]);
}

/*
 * Attach device (boot time).
 */
stattach(md)
	register struct mb_device *md;
{
	register struct scsi_unit *un;
	register struct scsi_tape *dsi;

	un = &stunits[md->md_unit];
	dsi = &stape[md->md_unit];
	un->un_md = md;
	un->un_mc = md->md_mc;
	un->un_unit = md->md_unit;
	un->un_target = TARGET(md->md_slave);
	un->un_lun = LUN(md->md_slave);
	un->un_ss = &scsi_unit_subr[TYPE(md->md_flags)];
	dsi->un_ctype = ST_TYPE_INVALID;
	dsi->un_openf = CLOSED;
}

stinit(dev)
	dev_t dev;
{
	register int unit;
	register struct scsi_tape *dsi;

	unit = STUNIT(dev);
	dsi = &stape[unit];

	/*
	 * Determine type of tape controller assuming initially that
	 * it is an emulex. Type is determined by requesting sense info
	 * and seeing how many bytes are returned. Is there a better way?
	 */
	dsi->un_ctype = ST_TYPE_EMULEX;
	dsi->un_qic = SC_QIC24;	/* emulex defaults to qic 24 format mode */
	dsi->un_openf = OPENING;
	(void) stcmd(dev, SC_TEST_UNIT_READY, 0);
	/*
	 * If it's not open, try 3 times to open it up.
	 * We expect the side effect of dsi->un_openf being set by
	 * stcmd() here (actually, it happens in stintr() as a result
	 * of calling stcmd()).
	 */
#define STNOTOPEN (stcmd(dev, SC_REQUEST_SENSE, 0)==0 || dsi->un_openf != OPEN)

	if (STNOTOPEN) {
		dsi->un_openf = OPENING;
		if (STNOTOPEN && STNOTOPEN) {
			if (dsi->un_openf == OPEN_FAILED) {
				uprintf("st%d: no cartridge loaded\n", unit);
			} else {
				uprintf("st%d: tape not online \n", unit);
			}
			dsi->un_openf = CLOSED;
			dsi->un_ctype = ST_TYPE_INVALID;
			return (0);
		}
	}
#undef STNOTOPEN

	/* allocate memory for mode select information */
	if (IS_EMULEX(dsi)) {
		u_int i;

		/* make sure that mode select stuff is word aligned */
		i = (u_int)rmalloc(iopbmap, 
				(long)(sizeof(struct st_emulex_mspl) + 2));
		if (i & 0x1)
			i++;

		dsi->un_mspl = (int) i;

		if (dsi->un_mspl != NULL) {
			bzero((caddr_t)dsi->un_mspl, 
			    sizeof(struct st_emulex_mspl));
		} else {
			dsi->un_openf = CLOSED;
			dsi->un_ctype = ST_TYPE_INVALID;
			printf("stinit: no iopb memory for mode select\n");
			return (0);
		}
		dsi->un_reset_occurred = 1;
	}
	dsi->un_openf = CLOSED;
	return (1);
}

stopen(dev, flag)
	dev_t dev;
	int flag;
{
	register struct scsi_unit *un;
	register int unit, s;
	register struct scsi_tape *dsi;

	unit = STUNIT(dev);
	if (unit > nstape) {
		return (ENXIO);
	}
	un = &stunits[unit];
	dsi = &stape[unit];
	if (un->un_mc == 0) {	/* never attached */
		return (ENXIO);
	}

	/* determine type of tape controller */
	if (dsi->un_ctype == ST_TYPE_INVALID) {
		if (stinit(dev) == 0) {
			return (EIO);
		}
	}

	s = spl6();
	if (dsi->un_openf != CLOSED) {	/* already open */
		(void) splx(s);
		return (EBUSY);
	}
	dsi->un_openf = OPEN;
	(void) splx(s);

	/* handle special open conditions for emulex */
	if (IS_EMULEX(dsi)) {
		/*
		 * Emulex will return busy indication until auto-load
		 * procedure is complete. Auto-load involves rewinding
		 * the tape so this could take a couple of minutes.
		 */
		if (stcmd(dev, SC_TEST_UNIT_READY, 0) == 0) {
			if (un->un_c->c_scb.chk && dsi->un_reset_occurred &&
			    (ST_NO_CART(dsi, un->un_c->c_sense) == 0)) {
				(void) stcmd(dev, SC_TEST_UNIT_READY, 0);
			}
			if (un->un_c->c_scb.busy) {
				uprintf("st%d: loading tape, try again... \n", 
				    unit);
				dsi->un_openf = CLOSED;
				return (EIO);
			} else if (un->un_c->c_scb.chk) {
				dsi->un_openf = CLOSED;
				return (EIO);
			}
		}

		/* 
		 * If a reset has occurred, the mode select information
		 * must be given to the controller again.
		 */
		if (dsi->un_reset_occurred) {
			dsi->un_reset_occurred = 0;
			if (stcmd(dev, SC_MODE_SELECT, 0) == 0) {
				uprintf("st%d: cannot initialize\n", unit);
				dsi->un_openf = CLOSED;
				return (EIO);
			}
		}

	} else {
		/* 
		 * If sysgen was not left in read mode, get sense to catch
		 * possible "no cartridge" indication if new tape has been
		 * installed since last close.
		 */
		if (dsi->un_readmode == 0) {
			dsi->un_openf = OPENING;
			(void) stcmd(dev, SC_REQUEST_SENSE, 0);
			dsi->un_openf = OPEN;
		}
		if (dsi->un_reset_occurred) {
			dsi->un_reset_occurred = 0;
			dsi->un_qic = SC_QIC11;
			dsi->un_readmode = 0;
			dsi->un_err_pending = 0;
		}
	}
	dsi->un_read_only = 0;

	/* check for need to convert to qic 24 format */
	if (minor(dev) & QIC_24) {
		if (dsi->un_qic == SC_QIC11) {
			/* 
			 * We cannot issue a QIC02 command to a cipher tape
			 * drive due to problems in the cipher firmware.
			 * Cipher drives only appear on sun2 multibus
			 * machines. So, if we have a sun2 multibus machine
			 * we must check for a cipher drive. We do this by
			 * issuing a read extended status QIC02 command.
			 * This is a cipher vendor unique command, so if
			 * we receive good status back then we assume the
			 * drive is a cipher and disallow the format change
			 * command. Another thing we must do if we determine
			 * that we have a cipher drive, is to reset the
			 * QIC02 bus. This is done by issuing a scsi bus
			 * reset.
			 * CAVEAT: since we are using a cipher vendor unique
			 * command, there is no guarantee that another vendor
			 * won't use this command for something else and
			 * screw us up. Ugh.....
			 */
#ifdef sun2
			if ((cpu == CPU_SUN2_120) && !IS_EMULEX(dsi)) {
				dsi->un_cipher_test = 1;
				if (stcmd(dev, SC_READ_XSTATUS_CIPHER, 0) !=
				    0) {
					uprintf("st%d: format change failed \n",
					    unit);
					dsi->un_openf = CLOSED;
					dsi->un_cipher_test = 0;
					(*un->un_c->c_ss->scs_reset)(un->un_c);
					return (EIO);
				}
				dsi->un_cipher_test = 0;
			}
#endif
			if (IS_EMULEX(dsi) && (dsi->un_mspl == NULL)) {
				uprintf("st%d: format change failed \n", unit);
				dsi->un_openf = CLOSED;
				return (EIO);
			}
			if (stcmd(dev, SC_QIC24, 0) == 0) {
				uprintf("st%d: format change failed \n", unit);
				dsi->un_openf = CLOSED;
				return (EIO);
			}
			dsi->un_qic = SC_QIC24;
		}
	} else if (dsi->un_qic == SC_QIC24) {
		if (IS_EMULEX(dsi) && (dsi->un_mspl == NULL)) {
			uprintf("st%d: format change failed \n", unit);
			dsi->un_openf = CLOSED;
			return (EIO);
		}
		if (stcmd(dev, SC_QIC11, 0) == 0) {
			uprintf("st%d: format change failed \n", unit);
			dsi->un_openf = CLOSED;
			return (EIO);
		}
		dsi->un_qic = SC_QIC11;
	}
	if ((flag & FWRITE) && dsi->un_read_only) {
		uprintf("st%d: cartridge is write protected \n", unit);
		dsi->un_openf = CLOSED;
		return (EIO);
	}
	dsi->un_lastiow = 0;
	dsi->un_lastior = 0;
	dsi->un_next_block = 0;
	dsi->un_last_block = INF;
	/* if we reset these here they will never be preserved for "mt status"
	dsi->un_retry_ct = 0;
	dsi->un_underruns = 0;
	*/
	return (0);
}

/*ARGSUSED*/
stclose(dev, flag)
	register dev_t dev;
	int flag;
{
	register struct scsi_unit *un;
	register struct scsi_tape *dsi;

	un = &stunits[STUNIT(dev)];
	dsi = &stape[STUNIT(dev)];
	dsi->un_openf = CLOSING;
	if (dsi->un_lastiow) {
		if (stcmd(dev, SC_WRITE_FILE_MARK, 0) == 0) {
			printf("st%d: stclose failed to write file mark\n",
				un - stunits);
		}
	}
	if (IS_SYSGEN(dsi)) {
		/* 
		 * If a read command was issued and we have not hit a 
		 * file mark, the tape is still in read mode. This could
		 * cause problems if the next command issued is not a read
		 * related command. Make note of this condition.
		 */
		if (dsi->un_lastior && (dsi->un_eof == 0)) {
			dsi->un_readmode = 1;

		/* 
		 * Tape was in read mode when last command was issued.
		 * This causes the tape to be rewound and an illegal command
		 * indication to be returned. 
		 */
		} else if (dsi->un_readmode && dsi->un_err_pending) {
			if ((dsi->un_last_cmd != SC_REWIND) &&
			    (minor(dev) & T_NOREWIND)) {
				uprintf("st%d: tape may have rewound\n",
				    un - stunits);
			}
			if (stcmd(dev, SC_REQUEST_SENSE, 0) == 0) {
				printf("st%d: stclose failed to get sense\n",
				    un - stunits);
			}
			dsi->un_readmode = 0;
			dsi->un_err_pending = 0;
		}
	}
	if ((minor(dev) & T_NOREWIND) == 0) {
		(void) stcmd(dev, SC_REWIND, -1);
	} 
	dsi->un_openf = CLOSED;
}

stcmd(dev, cmd, count)
	dev_t dev;
	int cmd, count;
{
	register struct buf *bp;
	register int s, error;
	register struct scsi_unit *un;

	un = &stunits[STUNIT(dev)];
	bp = &un->un_sbuf;
	s = splx(pritospl(un->un_mc->mc_intpri));
	while (bp->b_flags & B_BUSY) {
		/*
		 * special test because B_BUSY never gets cleared in
		 * the non-waiting rewind case.
		 */
		if (bp->b_bcount == -1 && (bp->b_flags & B_DONE)) {
			break;
		}
		bp->b_flags |= B_WANTED;
		(void) sleep((caddr_t) bp, PRIBIO);
	}
	bp->b_flags = B_BUSY | B_READ;
	(void) splx(s);
	bp->b_dev = dev;
	bp->b_bcount = count;
	un->un_scmd = cmd;
	ststrategy(bp);
	/*
	 * In case of rewind on close, don't wait.
	 */
	if (cmd == SC_REWIND && count == -1) {
		return (1);
	}
	s = splx(pritospl(un->un_mc->mc_intpri));
	while ((bp->b_flags & B_DONE) == 0) {
		(void) sleep((caddr_t) bp, PRIBIO);
	}
	(void) splx(s);
	error = geterror(bp);
	if (bp->b_flags & B_WANTED) {
		wakeup((caddr_t) bp);
	}
	bp->b_flags &= B_ERROR;		/* clears B_BUSY */
	return (error == 0);
}

ststrategy(bp)
	register struct buf *bp;
{
	register struct scsi_unit *un;
	register int unit, s;
	register struct buf *ap;
	register struct scsi_tape *dsi;

	unit = STUNIT(bp->b_dev);
	if (unit > nstape) {
		printf("st%d: ststrategy: invalid unit %x\n", unit, unit);
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	un = &stunits[unit];
	dsi = &stape[unit];
	if (dsi->un_openf != OPEN && bp != &un->un_sbuf) {
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	s = splx(pritospl(un->un_mc->mc_intpri));
	while (dsi->un_bufcnt >= MAXSTBUF) {
		(void) sleep((caddr_t) &dsi->un_bufcnt, PRIBIO);
	}
	dsi->un_bufcnt++;

	/*
	 * Put the block at the end of the queue.
	 * Should probably have a pointer to the end of
	 * the queue, but the queue can't get too long,
	 * so the added code complexity probably isn't
	 * worth it.
	 */
	ap = &un->un_utab;
	while (ap->b_actf != NULL) {
		ap = ap->b_actf;
	}
	ap->b_actf = bp;
	bp->b_actf = NULL;
	if (un->un_utab.b_active == 0) {
		(*un->un_c->c_ss->scs_ustart)(un);
		bp = &un->un_mc->mc_tab;
		if (bp->b_actf && bp->b_active == 0) {
			(*un->un_c->c_ss->scs_start)(un);
		}
	}

	/*
	 * Handle sysgen being issued a non-read related command while it
	 * is in read mode.
	 */
	if (IS_SYSGEN(dsi) && dsi->un_readmode) { 
		if ((dsi->un_last_cmd != SC_READ) && 
		    (dsi->un_last_cmd != SC_SPACE)) {
			dsi->un_err_pending = 1;
		} else {
			dsi->un_readmode = 0;
			dsi->un_err_pending = 0;
		}
	}
	(void) splx(s);
}

/*
 * Start the operation.
 */
/*ARGSUSED*/
ststart(bp, un)
	register struct buf *bp;
	register struct scsi_unit *un;
{
	register int bno;
	register struct scsi_tape *dsi;

	dsi = &stape[STUNIT(bp->b_dev)];
	/*
	 * Default is that last command was NOT a read/write command;
	 * if we issue a read/write command we will notice this in stintr().
	 */
	dsi->un_lastiow = 0;
	dsi->un_lastior = 0;
	if (bp == &un->un_sbuf) {
		un->un_cmd = un->un_scmd;
		un->un_count = bp->b_bcount;
	} else if (bp == &un->un_rbuf) {
		if (bp->b_flags & B_READ) {
			if (dsi->un_eof) {
				bp->b_resid = bp->b_bcount;
				iodone(bp);
				if (dsi->un_bufcnt-- >= MAXSTBUF) {
					wakeup((caddr_t) &dsi->un_bufcnt);
				}
				return (0);
			}
			un->un_cmd = SC_READ;
		} else {
			un->un_cmd = SC_WRITE;
		}
		un->un_count = howmany(bp->b_bcount, DEV_BSIZE);
		un->un_flags |= SC_UNF_DVMA;
	} else {
		bno = bdbtofsb(bp->b_blkno);
		if (bno > dsi->un_last_block && bp->b_flags & B_READ) {
			/* 
			 * Can't read past EOF.
			 */
			bp->b_flags |= B_ERROR;
			bp->b_error = EIO;
			iodone(bp);
			if (dsi->un_bufcnt-- >= MAXSTBUF) {
				wakeup((caddr_t) &dsi->un_bufcnt);
			}
			return (0);
		}
		if (bno == dsi->un_last_block && bp->b_flags & B_READ) {
			/*
			 * Reading at EOF returns 0 bytes.
			 */
			bp->b_resid = bp->b_bcount;
			iodone(bp);
			if (dsi->un_bufcnt-- >= MAXSTBUF) {
				wakeup((caddr_t) &dsi->un_bufcnt);
			}
			return (0);
		}
		if ((bp->b_flags & B_READ) == 0) {
			/*
			 * Writing sets EOF.
			 */
			dsi->un_last_block = bno + 1;
		}
		if (bno != dsi->un_next_block) {
			/*
			 * Not the next record.
			 * In theory we could space forward, or even rewind
			 * and space forward, and maybe someday we will.
			 * For now, no one really needs this capability.
			 */
			bp->b_flags |= B_ERROR;
			bp->b_error = ENXIO;
			iodone(bp);
			if (dsi->un_bufcnt-- >= MAXSTBUF) {
				wakeup((caddr_t) &dsi->un_bufcnt);
			}
			return (0);
		}
		/*
		 * Position OK, we can do the read or write.
		 */
		if (bp->b_flags & B_READ) {
			un->un_cmd = SC_READ;
		} else {
			un->un_cmd = SC_WRITE;
		}
		un->un_count = howmany(bp->b_bcount, DEV_BSIZE);
		un->un_flags |= SC_UNF_DVMA;
	}
	bp->b_resid = 0;
	return (1);
}

/*
 * Make a command description block.
 */
stmkcdb(c, un)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
{
	register struct scsi_cdb *cdb;
	register struct scsi_tape *dsi;
	int density = 0;

	cdb = &c->c_cdb;
	bzero((caddr_t)cdb, sizeof (*cdb));
	dsi = &stape[un->un_unit];

	/*++mjacob 4/14/87 per bug #1004516{*/
        if (IS_SYSGEN(dsi)) /* If sysgen, don't attempt a message out phase */
                un->un_flags |= SC_UNF_NOMSGOUT;
	/*}--mjacob                         */

	if (un->un_flags & SC_UNF_GET_SENSE) {
		cdb->cmd = SC_REQUEST_SENSE;
		cdb->lun = un->un_lun;
		un->un_flags |= SC_UNF_RECV_DATA;
		un->un_dma_addr = (int)c->c_sense - (int)DVMA;
		if (IS_EMULEX(dsi)) {
			un->un_dma_count = cdb->count = 
			    ST_EMULEX_SENSE_LEN;		
		} else if (IS_SYSGEN(dsi)) {
			un->un_dma_count = cdb->count = 
			    ST_SYSGEN_SENSE_LEN;		
		} else 
			un->un_dma_count = cdb->count = 
				sizeof (struct scsi_sense);
		return;
	}

	if (un->un_flags & SC_UNF_TAPE_APPEND_TEST) {
		/*
		 * Need to run special read command.
		 */
		if (IS_EMULEX(dsi)) {
			cdb->t_code = 1;
		}
		cdb->cmd = SC_READ;
		cdb->lun = un->un_lun;
		un->un_flags |= SC_UNF_RECV_DATA;
		un->un_dma_addr = (int)un->un_sbuf.b_un.b_addr - (int)DVMA;
		cdb->high_count = 0;
		cdb->mid_count = 0;
		cdb->low_count = 1;
		un->un_dma_count = DEV_BSIZE;		
		return;
	}

	cdb->cmd = un->un_cmd;
	cdb->lun = un->un_lun;
	un->un_dma_addr = un->un_dma_count = 0;

	if (ST_RECV_DATA_CMD(un->un_cmd))
		un->un_flags |= SC_UNF_RECV_DATA;
	else
		un->un_flags &= ~SC_UNF_RECV_DATA;

	switch (un->un_cmd) {

	case SC_WRITE_FILE_MARK:
	case SC_LOAD:
		cdb->count = 1;
		break;

	case SC_TEST_UNIT_READY:
		break;
		
	case SC_REWIND:
		if (dsi->un_reten_rewind) {	/* retension */
			dsi->un_reten_rewind = 0;
			if (IS_EMULEX(dsi)) {
				cdb->cmd = un->un_cmd = SC_LOAD;
				cdb->low_count = 0x03;
			} else if (IS_SYSGEN(dsi)) {
				cdb->vu_57 = 1;
			}
		}
		break;

	case SC_REQUEST_SENSE:
		un->un_dma_addr = (int)c->c_sense - (int)DVMA;
		if (IS_EMULEX(dsi)) {
			un->un_dma_count = cdb->count = 
			    ST_EMULEX_SENSE_LEN;		
		} else if (IS_SYSGEN(dsi)) {
			un->un_dma_count = cdb->count = 
			    ST_SYSGEN_SENSE_LEN;		
		}
		break;

	case SC_ERASE_CARTRIDGE:
		if (IS_EMULEX(dsi)) {
			cdb->t_code = 1;
		}
		break;

	case SC_READ:
	case SC_WRITE:
		if (IS_EMULEX(dsi)) {
			cdb->t_code = 1;
		}
		cdb->high_count = un->un_count >> 16;
		cdb->mid_count = (un->un_count >> 8) & 0xFF;
		cdb->low_count = un->un_count & 0xFF;
		un->un_dma_addr = un->un_baddr;
		un->un_dma_count = un->un_count * DEV_BSIZE;
		break;

	case SC_QIC11:
		if (IS_EMULEX(dsi)) {
			density = ST_EMULEX_QIC11;
			un->un_cmd = cdb->cmd = SC_MODE_SELECT;
			goto MODE;
		} else if (IS_SYSGEN(dsi)) {
			cdb->cmd = un->un_cmd = SC_QIC02;
			cdb->high_count = ST_SYSGEN_QIC11;
			un->un_dma_addr = un->un_dma_count = 0;
		}
		break;

	case SC_QIC24:
		if (IS_EMULEX(dsi)) {
			density = ST_EMULEX_QIC24;
			un->un_cmd = cdb->cmd = SC_MODE_SELECT;
			goto MODE;
		} else if (IS_SYSGEN(dsi)) {
			cdb->cmd = un->un_cmd = SC_QIC02;
			cdb->high_count = ST_SYSGEN_QIC24;
			un->un_dma_addr = un->un_dma_count = 0;
		}
		break;
	
#ifdef sun2
	case SC_READ_XSTATUS_CIPHER:
		cdb->cmd = un->un_cmd = SC_QIC02;
		cdb->high_count = SC_READ_XSTATUS_CIPHER;
		un->un_dma_addr = un->un_dma_count = 0;
		break;
#endif

MODE:
	case SC_MODE_SELECT:
		if (dsi->un_mspl == NULL)
			break;
		un->un_dma_addr = (int)dsi->un_mspl - (int)DVMA;
		if (IS_EMULEX(dsi)) {
			register struct st_emulex_mspl *em;

			em = (struct st_emulex_mspl *)dsi->un_mspl;
			em->hdr.bufm = 1;
			em->hdr.bd_len = EM_MS_BD_LEN;
			if (density != 0) {
				em->bd.density = density;
			} else if (dsi->un_qic == SC_QIC24) {
				em->bd.density = ST_EMULEX_QIC24;
			} else {
				em->bd.density = ST_EMULEX_QIC11;
			}
			un->un_dma_count = cdb->count = EM_MS_PL_LEN;
		}
		break;

	case SC_SPACE_FILE:
		cdb->t_code = 1;	/* space files, not records */
		/* fall through ... */

	case SC_SPACE_REC:
		cdb->cmd = SC_SPACE;
		cdb->high_count = un->un_count >> 16;
		cdb->mid_count = (un->un_count >> 8) & 0xFF;
		cdb->low_count = un->un_count & 0xFF;
		un->un_dma_addr = un->un_dma_count = 0;
		break;

	default:
		printf("st%d: stmkcdb: invalid command %x\n", un - stunits,
			un->un_cmd);
		break;	
	}
	/* save last command for handling state of sysgen */
	dsi->un_last_cmd = cdb->cmd;
}

stintr(c, resid, error)
	register struct scsi_ctlr *c;
	register int error;
	int resid;
{
	register struct scsi_unit *un;
	register struct buf *bp;
	register struct scsi_tape *dsi;
	struct st_emulex_sense *ems;
	struct st_sysgen_sense *sgs;

	un = c->c_un;
	bp = un->un_mc->mc_tab.b_actf->b_actf;
	dsi = &stape[STUNIT(bp->b_dev)];
	if (IS_EMULEX(dsi)) {
		ems = (struct st_emulex_sense *)c->c_sense;
	} else if (IS_SYSGEN(dsi)) {
		sgs = (struct st_sysgen_sense *)c->c_sense;
	}
		
#ifdef STDEBUG
	if (st_debug) {
		int x;
		u_char *y = (u_char *)&(c->c_cdb);

		if (!(c->c_scb.chk) && !(un->un_flags & SC_UNF_GET_SENSE)) {
			printf("stcmd worked: ");
			for (x = 0; x < 6; x++)
				printf("%x ", *y++);
			printf("\n");
		} else if (!(c->c_scb.chk) && 
				(un->un_flags & SC_UNF_GET_SENSE)) {
			printf("stcmd failed: ");
			for (x = 0; x < 6; x++)
				printf("%x ", *y++);
			st_err_message(c, dsi);
			printf("\n");
		} else if ((c->c_scb.chk && 
				un->un_flags & SC_UNF_TAPE_APPEND_TEST)) {
			printf("stcmd failed (no sense): ");
			for (x = 0; x < 6; x++)
				printf("%x ", *y++);
			printf("\n");
		}
	}
#endif STDEBUG

	if (c->c_scb.chk && IS_SCSI3(c) &&
	    !(un->un_flags & (SC_UNF_GET_SENSE|SC_UNF_TAPE_APPEND_TEST))) {
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

#ifdef STDEBUG
		if (st_debug)
			printf("running req sense\n");
#endif STDEBUG

		(*un->un_c->c_ss->scs_go)(un->un_mc);

		return;
	}

	if ((un->un_flags & SC_UNF_GET_SENSE) && IS_SCSI3(c)) {

		if (c->c_scb.chk) {
			printf("stintr: request sense failed\n");
		}

		/* were running a request sense cmd - restore old cmd */
 
		st_restore_cmd_state(c, un, &resid);
		un->un_flags &= ~SC_UNF_GET_SENSE;
	}
 
	if ((un->un_flags & SC_UNF_TAPE_APPEND_TEST)) {
		/*
		 * Just ran the special read command to see whether
		 * we were legitiately trying to append to the end
		 * of tape.  Must restore old cmd.
		 */

		un->un_flags &= ~SC_UNF_TAPE_APPEND_TEST;
		rmfree(iopbmap, (long) (DEV_BSIZE+2),
			 (long) (un->un_scratch_addr));
		
		if (c->c_scb.chk) {
			/* 
			 * Read failed, so we are at EOM, 
			 * so allow writing - in other words,
			 * retry the command.
			 */
#ifdef STDEBUG
			if (st_debug)
				printf("special read failed\n");
#endif STDEBUG
			st_restore_cmd_state(c, un, &resid);
			(*un->un_c->c_ss->scs_go)(un->un_mc);
			return;
		} else {
			/*
			 * Read worked, which means we are not
			 * at EOM.  Thus the controller 
			 * legitimately reported the append error.
			 */
#ifdef STDEBUG
			if (st_debug)
				printf("special read worked\n");
#endif STDEBUG
			st_restore_cmd_state(c, un, &resid);
			goto other_error;
		}
	}

	/* 
	 * We determine which tape controller we have by the number of
	 * sense bytes we get back.
	 */
	if (dsi->un_openf == OPENING && un->un_cmd == SC_REQUEST_SENSE) {
		if (resid) {
			dsi->un_ctype = ST_TYPE_SYSGEN;
			dsi->un_qic = SC_QIC11;
			dsi->un_openf = OPEN_FAILED;
			sgs = (struct st_sysgen_sense *)c->c_sense;
			ems = (struct st_emulex_sense *)0;
		} else {
			if (ST_NO_CART(dsi, c->c_sense))
				dsi->un_openf = OPEN_FAILED;
			else
				dsi->un_openf = OPEN;
			if (ST_WRITE_PROT(dsi, c->c_sense))
				dsi->un_read_only = 1;
		}
	}

	/* may get busy indication if emulex is auto-loading */
	if (c->c_scb.busy) {
		bp->b_flags |= B_ERROR;

	} else if (error || c->c_scb.chk || resid > 0) {
		if (c->c_scb.chk && ST_RESET(dsi, c->c_sense)) {
			/*
			 * Power on or reset occurred
			 */
			dsi->un_reset_occurred = 1;
			bp->b_flags |= B_ERROR;
		} else if (c->c_scb.chk && ST_NO_CART(dsi, c->c_sense)) {
			/*
			 * No cartridge loaded.
			 */
			if (IS_SYSGEN(dsi)) {
				if ((dsi->un_last_cmd == SC_REWIND) &&
				    dsi->un_readmode) {
				    dsi->un_readmode = 0;
				    dsi->un_err_pending = 0;
				    goto success;
				} else {
					printf("st%d: no cartridge loaded\n", 
					    un - stunits);
					bp->b_flags |= B_ERROR;
				}
			} else {
				printf("st%d: no cartridge loaded \n", 
				    un - stunits);
				bp->b_flags |= B_ERROR;
			}
		} else if (c->c_scb.chk && ST_FILE_MARK(dsi, c->c_sense)) {
			/*
			 * File mark detected.
			 */
			dsi->un_eof = 1;
			switch (un->un_cmd) {
			case SC_READ:
				dsi->un_next_block += un->un_count -
					resid / DEV_BSIZE;
				dsi->un_last_block = dsi->un_next_block;
				break;
			case SC_SPACE_REC:
				dsi->un_last_block = dsi->un_next_block +=
					un->un_count;	/* a little high */
				break;
			default:
				printf("st%d: scsi tape hit eof on cmd %x\n",
					un - stunits, un->un_cmd);
				break;
			}
			bp->b_resid = resid;
		} else if (c->c_scb.chk && ST_WRITE_PROT(dsi, c->c_sense) &&
		    	SC_IS_WRITE_COMMAND(un->un_cmd)) {
			/*
			 * Write protected tape.
			 */
			printf("st%d: tape is write protected\n", un - stunits);
			dsi->un_read_only = 1;
			bp->b_flags |= B_ERROR;
		} else if (c->c_scb.chk && ST_EOT(dsi, c->c_sense)) {
			/*
			 * End of tape.
			 */
			bp->b_resid = bp->b_bcount;
			if (un->un_cmd == SC_WRITE) {
				/*
				 * Setting this flag makes stclose()
				 * write a file mark before closing.
				 * Until a file mark is written, the 
				 * tape will return invalid command
				 * indications and not respond to 
				 * rewinds.
				 */
				dsi->un_lastiow = 1;
			}
#ifdef sun2
		} else if (dsi->un_cipher_test && c->c_scb.chk &&
		    ST_ILLEGAL(dsi, c->c_sense)) {
			/* 
			 * Read Extended Status command was issued to a
			 * non-cipher tape drive. See comments in stopen()
			 * for more details on cipher drive checking.
			 */
			bp->b_flags |= B_ERROR;
#endif
		} else if (c->c_scb.chk && ST_ILLEGAL(dsi, c->c_sense)) {
			/*
			 * Illegal command was issued.
			 */
			if (IS_SYSGEN(dsi)) {
				if ((dsi->un_last_cmd == SC_REWIND) &&
				    dsi->un_readmode) {
				    dsi->un_readmode = 0;
				    dsi->un_err_pending = 0;
				    goto success;
				} else {
					printf("st%d: try again\n", 
					    un - stunits);
					bp->b_flags |= B_ERROR;
				}
			} else {
				printf("st%d: illegal command\n", un-stunits);
				st_err_message(c, dsi);
				bp->b_flags |= B_ERROR;
			}
		} else if (ST_CORRECTABLE(dsi, c->c_sense)) {
			/*
			 * A block had to be read/written more than
			 * once but was successfully read/written.
			 * Just bump stats and consider the operation a
			 * success.
			 */
			dsi->un_retry_ct += (ems->retries_msb << 8) +
			    ems->retries_lsb;
			goto success;
		} else if (IS_EMULEX(dsi) && ems->error == EM_APPEND_ERR) {
			/*
			 * Append errors are fun!
			 * They can result from either being positioned
			 * in the middle of the tape (in which case the
			 * request to write is illegal) or from being positioned
			 * at the end of tape (which is legal, but the 
			 * controller doesn't know it).
			 *
			 * The way to differentiate these two cases is by
			 * trying to read.  If the read works, we must have
			 * been in the middle of the tape, so we report the
			 * append error.  If the read fails, we were at the
			 * end of the tape, so we retry the write.  Easy, huh?
			 */ 

			un->un_flags |= SC_UNF_TAPE_APPEND_TEST;

			/*
			 * Allocate a scratch buffer for the read.
			 * We appropriate the sbuf for this.
			 */
			un->un_sbuf.b_un.b_addr = (caddr_t)rmalloc(iopbmap, 
							(long) (DEV_BSIZE + 2));

			if (un->un_sbuf.b_un.b_addr == NULL) {
				printf("st%d: can't alloc scratch buf\n", 
					un - stunits);
				st_restore_cmd_state(c, un, &resid);
				un->un_flags &= ~SC_UNF_TAPE_APPEND_TEST;
				goto other_error;
			}

			/* save the original address for rmfree() */
			un->un_scratch_addr = un->un_sbuf.b_un.b_addr;

			if ((int)un->un_sbuf.b_un.b_addr & 0x1)   /* align it */
				((caddr_t)un->un_sbuf.b_un.b_addr)++;

			/*
			 * sistart() will call mkcdb(), which will notice
			 * that the flag is set and not do the copy of the cdb,
			 * doing a special read rather than the write command.
			 * 
			 * Don't need to store away saved command because
			 * we already did it when we ran the request
			 * sense command which told us that this is an
			 * append error.
			 */

			(*un->un_c->c_ss->scs_go)(un->un_mc);

			return;
		} else {
			/*
			 * Some other error which we can't handle.
			 */
other_error:
			/*
			 * Suppress printing error messages on failed
			 * FSFs if we are at EOM for MT02.
			 * This allows the user to use FSF to get
			 * to the EOM by doing mt fsf <bignum>
			 */

			if ((c->c_cdb.cmd != SC_QIC02) &&
			    (! ((c->c_cdb.cmd == SC_SPACE_FILE) &&
			       IS_EMULEX(dsi) &&
			       (ems->error == EM_READ_END_OF_MEDIA))) &&
			    (c->c_scb.chk && dsi->un_openf == OPEN)) {
				st_err_message(c, dsi);
			}
			bp->b_flags |= B_ERROR;
		}
	} else {
success:
		switch (un->un_cmd) {

		case SC_QIC02:
		case SC_REWIND:
		case SC_ERASE_CARTRIDGE:
		case SC_MODE_SELECT:
		case SC_LOAD:
			dsi->un_next_block = 0;
			dsi->un_eof = 0;
			break;

		case SC_WRITE:
			dsi->un_lastiow = 1;
			dsi->un_next_block += un->un_count;
			break;

		case SC_READ:
			dsi->un_lastior = 1;
			dsi->un_next_block += un->un_count;
			break;

		case SC_SPACE_FILE:
			dsi->un_next_block = 0;
			dsi->un_last_block = INF;
			dsi->un_eof = 0;
			break;

		case SC_SPACE_REC:
			dsi->un_next_block += un->un_count;
			break;

		case SC_REQUEST_SENSE:
			if (IS_EMULEX(dsi)) {
				dsi->un_status = ems->error;
				if (ems->ext_sense.add_len >= EM_ES_ADD_LEN) {
					dsi->un_retry_ct += 
					    (ems->retries_msb << 8) +
					    ems->retries_lsb;
				}
			} else if (IS_SYSGEN(dsi)) {
				dsi->un_status = AR_SENSE(sgs);
				dsi->un_retry_ct += sgs->qic_sense.retry_ct;
				dsi->un_underruns += sgs->qic_sense.underruns;
			}
			break;

		case SC_TEST_UNIT_READY:
			break;

		case SC_WRITE_FILE_MARK:
			dsi->un_next_block = 0;
			dsi->un_last_block = 0;	/* i.e. no reads allowed */
			break;

		default:
			printf("st%d: stintr: invalid command %x\n",
			    un - stunits, un->un_cmd);
			break;
		}
	}

	if (bp == &un->un_sbuf && 
	    ((un->un_flags & SC_UNF_DVMA) == 0)) {
		(*c->c_ss->scs_done)(un->un_mc);
	} else {
		mbdone(un->un_mc);
		un->un_flags &= ~SC_UNF_DVMA;
	}
	if (dsi->un_bufcnt-- >= MAXSTBUF) {
		wakeup((caddr_t) &dsi->un_bufcnt);
	}
}

/*
 * Restore old command state after running request sense command.
 */
st_restore_cmd_state(c, un, resid)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
	register int *resid;
{
	c->c_scb = un->un_saved_cmd.saved_scb;
	c->c_cdb = un->un_saved_cmd.saved_cdb;
	*resid = un->un_saved_cmd.saved_resid;
	un->un_dma_addr = un->un_saved_cmd.saved_dma_addr;
	un->un_dma_count = un->un_saved_cmd.saved_dma_count;
}
 

char *st_emulex_error_str[] = {
	"no sense",			/* 0x00 */
	"",				/* 0x01 */
	"",				/* 0x02 */
	"",				/* 0x03 */
	"drive not ready",		/* 0x04 */
	"",				/* 0x05 */
	"",				/* 0x06 */
	"",				/* 0x07 */
	"",				/* 0x08 */
	"tape not loaded",		/* 0x09 */
	"tape too short",		/* 0x0a */
	"drive timeout",		/* 0x0b */
	"",				/* 0x0c */
	"",				/* 0x0d */
	"",				/* 0x0e */
	"",				/* 0x0f */
	"",				/* 0x10 */
	"uncorrectable data error",	/* 0x11 */
	"",				/* 0x12 */
	"",				/* 0x13 */
	"block not found",		/* 0x14 */
	"",				/* 0x15 */
	"dma timeout",			/* 0x16 */
	"tape is write protected",	/* 0x17 */
	"correctable data error",	/* 0x18 */
	"bad block",			/* 0x19 */
	"",				/* 0x1a */
	"",				/* 0x1b */
	"file mark detected",		/* 0x1c */
	"compare error",		/* 0x1d */
	"",				/* 0x1e */
	"",				/* 0x1f */
	"invalid command",		/* 0x20 */
	"",				/* 0x21 */
	"",				/* 0x22 */
	"",				/* 0x23 */
	"",				/* 0x24 */
	"",				/* 0x25 */
	"",				/* 0x26 */
	"",				/* 0x27 */
	"",				/* 0x28 */
	"",				/* 0x29 */
	"",				/* 0x2a */
	"",				/* 0x2b */
	"",				/* 0x2c */
	"",				/* 0x2d */
	"",				/* 0x2e */
	"",				/* 0x2f */
	"unit attention",		/* 0x30 */
	"command timeout",		/* 0x31 */
	"",				/* 0x32 */
	"append error",			/* 0x33 */
	"read end-of-media",		/* 0x34 */
	0
};

#define N_ST_EMULEX_ERROR_STR \
	(sizeof(st_emulex_error_str)/sizeof(st_emulex_error_str[0]))

char *
st_pr_emulex_error(error_code)
	u_char error_code;
{
	static char *unknown_error = "unknown error";

	if ((error_code > N_ST_EMULEX_ERROR_STR - 1) || 
	    st_emulex_error_str[error_code] == NULL)
		return (unknown_error);
	else
		return (st_emulex_error_str[error_code]);
}


st_err_message(c, dsi)
	register struct scsi_ctlr *c;
	register struct scsi_tape *dsi;
{
	register struct scsi_unit *un = c->c_un;

	if (IS_SYSGEN(dsi)) {
		printf("st%d error: sense %b\n", un - stunits, 
		    AR_SENSE(c->c_sense), SENSE_BITS);
	} else if (IS_EMULEX(dsi)) {
		register u_char *cp = (u_char *)c->c_sense;
		register u_char error = 
				((struct st_emulex_sense *)c->c_sense)->error;
		register int i;

		printf("st%d error: %s (code 0x%x), sense ", 
			un - stunits, st_pr_emulex_error(error), error);

		for (i=0; i < ST_EMULEX_SENSE_LEN; i++)
			printf("%x ", *cp++);
		printf("\n");
	}
}

stread(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register struct scsi_unit *un;
	register int unit, r, resid;
	register struct scsi_tape *dsi;

	unit = STUNIT(dev);
	if (unit > nstape) {
		return (ENXIO);
	}
	un = &stunits[unit];
	dsi = &stape[unit];
	if (uio->uio_iov->iov_len % DEV_BSIZE) {
		return (EIO);	/* drive can't do it */
	}
	resid = uio->uio_resid;
	dsi->un_next_block = bdbtofsb(uio->uio_offset / DEV_BSIZE);
	dsi->un_last_block = INF;
	r = physio(ststrategy, &un->un_rbuf, dev, B_READ, minphys, uio);
	if (dsi->un_eof && uio->uio_resid == resid) {
		dsi->un_eof = 0;	/* the user is really getting it */
	}
	return (r);
}

stwrite(dev, uio)
	register dev_t dev;
	register struct uio *uio;
{
	register struct scsi_unit *un;
	register int unit;
	register struct scsi_tape *dsi;

	unit = STUNIT(dev);
	if (unit > nstape) {
		return (ENXIO);
	}
	un = &stunits[unit];
	dsi = &stape[unit];
	if (uio->uio_iov->iov_len % DEV_BSIZE) {
		return (EIO);	/* drive can't do it */
	}
	dsi->un_next_block = bdbtofsb(uio->uio_offset / DEV_BSIZE);
	dsi->un_last_block = INF;
	return (physio(ststrategy, &un->un_rbuf, dev, B_WRITE, minphys, uio));
}

/*ARGSUSED*/
stioctl(dev, cmd, data, flag)
	register dev_t dev;
	register int cmd;
	register caddr_t data;
	int flag;
{
	register struct mtop *mtop;
	register int callcount, fcount;
	register struct mtget *mtget;
	register int unit;
	register struct scsi_tape *dsi;
	static int ops[] = {
		SC_WRITE_FILE_MARK,	/* write tape mark */
		SC_SPACE_FILE,		/* forward space file */
		0,			/* backspace file - can't */
		SC_SPACE_REC,		/* forward space record */
		0,			/* backspace record - can't */
		SC_REWIND,		/* rewind tape */
		SC_REWIND,		/* unload - we just rewind */
		SC_REQUEST_SENSE,	/* get status */
		SC_REWIND,		/* retension - rewind + vu_57 */
		SC_ERASE_CARTRIDGE,	/* erase entire tape */
	};

	unit = STUNIT(dev);
	if (unit > nstape) {
		return (ENXIO);
	}
	dsi = &stape[unit];
	switch (cmd) {
	case MTIOCTOP:	/* tape operation */
		mtop = (struct mtop *) data;
		switch (mtop->mt_op) {
		case MTWEOF:
		case MTERASE:
			if (dsi->un_read_only)
				return (ENXIO);
		}

		switch (mtop->mt_op) {
		case MTWEOF:
		case MTFSF:
			callcount = mtop->mt_count;
			fcount = 1;
			break;

		case MTRETEN:
			dsi->un_reten_rewind = 1;
			/* FALL THRU */
		case MTREW:
		case MTOFFL:
		case MTNOP:
		case MTERASE:
			callcount = 1;
			fcount = mtop->mt_count;
			break;
		default:
			return (ENXIO);
		}
		if (callcount <= 0 || fcount <= 0) {
			return (ENXIO);
		}
		/*
		 * If eof_flag then we hit eof but didn't tell the user yet.
		 */
		if (ops[mtop->mt_op] == SC_SPACE_FILE && dsi->un_eof) {
			dsi->un_eof = 0;
			callcount--;
		}
		while (--callcount >= 0) {
			if (stcmd(dev, ops[mtop->mt_op], fcount) == 0) {
				return (EIO);
			}
		}
		return (0);

	case MTIOCGET:
		if (stcmd(dev, SC_REQUEST_SENSE, 0) == 0) {
			return (EIO);
		}
		/* 
		 * Tape was in read mode when request sense was issued.
		 * This causes the tape to be rewound and an illegal command
		 * indication to be returned. 
		 */
		if (IS_SYSGEN(dsi) && dsi->un_readmode && dsi->un_err_pending) {
			/* get sense with illegal cmd indication */
			if (stcmd(dev, SC_REQUEST_SENSE, 0) == 0) {
				return (EIO);
			}
			/* now get real sense information */
			if (stcmd(dev, SC_REQUEST_SENSE, 0) == 0) {
				return (EIO);
			}
			if (minor(dev) & T_NOREWIND)
				uprintf("st%d: tape may have rewound\n", unit);
			dsi->un_readmode = 0;
			dsi->un_err_pending = 0;
		}
		mtget = (struct mtget *) data;
		mtget->mt_type = MT_ISSC;
		mtget->mt_erreg = dsi->un_status;
		mtget->mt_fileno = dsi->un_retry_ct;
		mtget->mt_blkno = dsi->un_underruns;
		dsi->un_retry_ct = dsi->un_underruns = 0;
		return (0);

	default:
		return (ENXIO);
	}
}
#endif NST > 0
