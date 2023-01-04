#ifndef lint
static char sccsid[] = "@(#)sc.c	1.6\t1/16/87 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "sc.h"

#if NSC > 0
/*
 * Generic scsi routines.
 */
 
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
#include "../machine/scb.h"

#include "../sun/dklabel.h"
#include "../sun/dkio.h"

#include "../sundev/mbvar.h"
#include "../sundev/screg.h"
#include "../sundev/sireg.h"
#include "../sundev/scsi.h"

struct	scsi_ctlr scctlrs[NSC];
#define SCNUM(sc)	(sc - scctlrs)

int	scprobe(), scslave(), scattach(), scgo(), scdone(), scpoll();
int	scustart(), scstart(), sc_cmd(), sc_getstatus(), sc_cmdwait();
int	sc_off(), sc_reset(), sc_dmacnt();

struct	mb_ctlr *scinfo[NSC];
extern struct mb_device *sdinfo[];
struct	mb_driver scdriver = {
	scprobe, scslave, scattach, scgo, scdone, scpoll,
	sizeof (struct scsi_ha_reg), "sd", sdinfo, "sc", scinfo, MDR_BIODMA,
};

/* routines available to devices for mainbus scsi host adaptor */
struct scsi_ctlr_subr scsubr = {
	scustart, scstart, scdone, sc_cmd, sc_getstatus, sc_cmdwait,
	sc_off, sc_reset, sc_dmacnt, scgo,
};

extern int scsi_debug;
extern int scsi_disre_enable;	/* enable disconnect/reconnect */
extern int scsi_ntype;
extern struct scsi_unit_subr scsi_unit_subr[];

/*
 * Determine existence of SCSI host adapter.
 */
scprobe(reg, ctlr)
	register struct scsi_ha_reg *reg;
	register int ctlr;
{
	register struct scsi_ctlr *c;

	/* probe for different scsi host adaptor interfaces */
	c = &scctlrs[ctlr];
	if (peekc((char *)&reg->dma_count) == -1) {
		return (0);
	}
	reg->dma_count = 0x6789;
	if (reg->dma_count != 0x6789) {
		return (0);
	}

	/* allocate memory for sense information */
	c->c_sense = (struct scsi_sense *) rmalloc(iopbmap,
	    (long) sizeof (struct scsi_sense));
	if (c->c_sense == NULL) {
		printf("scprobe: no iopb memory for sense.\n");
		return (0);
	}

	/* init controller information */
	c->c_flags = SCSI_PRESENT;
	c->c_har = reg;
	c->c_ss = &scsubr;
	sc_reset(c);
	return (sizeof (struct scsi_ha_reg));
}

/*
 * See if a slave exists.
 * Since it may exist but be powered off, we always say yes.
 */
/*ARGSUSED*/
scslave(md, reg)
	struct mb_device *md;
	register struct scsi_ha_reg *reg;
{
	register struct scsi_unit *un;
	register int type;

#ifdef lint
	reg = reg;				/* use it or lose it */
#endif lint

	/*
	 * This kludge allows autoconfig to print out "sd" for
	 * disks and "st" for tapes.  The problem is that there
	 * is only one md_driver for scsi devices.
	 */
	type = TYPE(md->md_flags);
	if (type >= scsi_ntype) {
		panic("scslave: unknown type in md_flags");
	}

	/* link unit to its controller */
	un = (struct scsi_unit *)(*scsi_unit_subr[type].ss_unit_ptr)(md);
	if (un == 0) {
		panic("scslave: md_flags scsi type not configured in\n");
	}
	un->un_c = &scctlrs[md->md_ctlr];
	md->md_driver->mdr_dname = scsi_unit_subr[type].ss_devname;
	return (1);
}

/*
 * Attach device (boot time).
 */
scattach(md)
	register struct mb_device *md;
{
	register struct mb_ctlr *mc = md->md_mc;
	register struct scsi_ctlr *c = &scctlrs[md->md_ctlr];
	register int type = TYPE(md->md_flags);

	if (type >= scsi_ntype) {
		panic("scattach: unknown type in md_flags");
	}
	(*scsi_unit_subr[type].ss_attach)(md);

	/*
	 * Initialize interrupt register
	 */
	if (mc->mc_intr) {
		/* set up for vectored interrupts - we will pass ctlr ptr */
		c->c_har->intvec = mc->mc_intr->v_vec;
		*(mc->mc_intr->v_vptr) = (int)c;
	} else {
		/* use auto-vectoring */
		c->c_har->intvec = AUTOBASE + mc->mc_intpri;
	}
}

/*
 * SCSI unit start routine.
 * Called by SCSI device drivers.
 */
scustart(un)
	register struct scsi_unit *un;
{
	register struct buf *dp;
	register struct mb_ctlr *mc;

	mc = un->un_mc;
	dp = &un->un_utab;
	/* 
	 * Caller guarantees: dp->b_actf != NULL && dp->b_active == 0 
	 */
	/*
	 * Put device on ready queue for bus.
	 */
	if (mc->mc_tab.b_actf == NULL) {
		mc->mc_tab.b_actf = dp;
	} else {
		mc->mc_tab.b_actl->b_forw = dp;
	}
	dp->b_forw = NULL;
	mc->mc_tab.b_actl = dp;
	dp->b_active = 1;
	dp->b_un.b_addr = (caddr_t) un;
}

/*
 * Set up a transfer for the bus.
 */
scstart(un)
	register struct scsi_unit *un;
{
	register struct mb_ctlr *mc;
	register struct buf *bp, *dp;

	mc = un->un_mc;
	dp = mc->mc_tab.b_actf;	     /* != NULL guaranteed by caller */
	un = (struct scsi_unit *) dp->b_un.b_addr;
	bp = dp->b_actf;
	for (;;) {
		if (bp == NULL) {	  /* no more blocks for this device */
			un->un_utab.b_active = 0;
			dp = mc->mc_tab.b_actf = dp->b_forw;
			if (dp == NULL) {  /* no more devices for this ctlr */
				return;
			}
			un = (struct scsi_unit *) dp->b_un.b_addr;
			bp = dp->b_actf;
		} else {
			if ((*un->un_ss->ss_start)(bp, un)) {
				mc->mc_tab.b_active = 1;
				un->un_c->c_un = un;
				if (bp == &un->un_sbuf  &&
				    ((un->un_flags & SC_UNF_DVMA) == 0)) {
					scgo(mc);
				} else {
					(void) mbgo(mc);
				}
				return;
			}
			dp->b_actf = bp = bp->av_forw;
		}
	}
}

/*
 * Start up a transfer
 * Called via mbgo after buffer is in memory
 */
scgo(mc)
	register struct mb_ctlr *mc;
{
	register struct scsi_unit *un;
	register struct scsi_ctlr *c;
	register struct buf *bp, *dp;
	register int unit;

	c = &scctlrs[mc->mc_ctlr];
	dp = mc->mc_tab.b_actf;
	if (dp == NULL || dp->b_actf == NULL) {
		panic("scgo queueing error 1");
	}
	bp = dp->b_actf;
	un = c->c_un;
	if (dp != &un->un_utab) {
		panic("scgo queueing error 2");
	}
	un->un_baddr = MBI_ADDR(mc->mc_mbinfo);
	if ((unit = un->un_md->md_dk) >= 0) {
		dk_busy |= 1<<unit;
		dk_xfer[unit]++;
		dk_wds[unit] += bp->b_bcount >> 6;
	}
	(*un->un_ss->ss_mkcdb)(c, un);
	if (sc_cmd(c, un, 1) == 0) {
		(*un->un_ss->ss_intr)(c, 0, SE_RETRYABLE);
		sc_off(un);
	}
}

/*
 * Pass a command to the SCSI bus.
 */
sc_cmd(c, un, intr)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
	register int intr;
{
	register u_char *cp;
	register int i, errct;
	register u_short icr_mode;

	errct = 0;
	do {
		/* make sure scsi bus is not continuously busy */
		for (i = WAIT_COUNT; i > 0; i--) {
		        if (!(c->c_har->icr & ICR_BUSY))
		                break;
		        DELAY(10);
		}
		if (i == 0) {
			sc_reset(c);
			return (0);
		}

		/* select target and wait for response */
		c->c_har->data = (1 << un->un_target) | HOST_ADDR;
		c->c_har->icr = ICR_SELECT;

		/* target never responded to selection */
		if (sc_wait(c, ICR_BUSY) == 0) {
			c->c_har->data = 0;
			c->c_har->icr = 0;
			return (0);
		}
		/*
		 * may need to map between the CPU's kernel context address
		 * and the device's DVMA bus address
		 */
		c->c_har->dma_addr = un->un_dma_addr;
		c->c_har->dma_count = ~un->un_dma_count; /* hardware is funny */
		icr_mode = ICR_DMA_ENABLE;
		if (intr) {
			icr_mode |= ICR_INTERRUPT_ENABLE;
			un->un_wantint = 1;
		}
		if  (! (un->un_dma_count & 1)) {
			icr_mode |= ICR_WORD_MODE;
		}
		c->c_har->icr = icr_mode;
		cp = (u_char *) &c->c_cdb;
		if (scsi_debug) {
			printf("sc%d: sc_cmd: target %d issuing command ",
			    SCNUM(c), un->un_target);
			for (i = 0; i < sizeof (struct scsi_cdb); i++) {
				printf("%x ", *cp++);
			}
			printf("\n");
			cp = (u_char *) &c->c_cdb;
		}
		for (i = 0; i < sizeof (struct scsi_cdb); i++) {
			if (sc_putbyte(c, ICR_COMMAND, *cp++) == 0) {
				errct++;
				break;
			}
		}
		if (i == sizeof (struct scsi_cdb)) {
			return (1);
		}
	} while (errct < 5);
	printf("sc_cmd: unrecoverable errors\n");
	return (0);
}

/*
 * Handle a scsi bus interrupt for the mainbus scsi.
 */
scintr(c)
	register struct scsi_ctlr *c;
{
	register int resid;
	register struct scsi_unit *un;

	un = c->c_un;
	if (un->un_wantint == 0) {
		if (c->c_har->icr & ICR_BUS_ERROR) {
			printf("scsi bus error (unwanted interrupt)\n");
		} else {
			printf("scsi: unwanted interrupt\n");
		}
		sc_reset(c);
		return;
	}
	un->un_wantint = 0;
	resid = (u_short) (~c->c_har->dma_count);
	if (c->c_har->icr & ICR_BUS_ERROR) {
		printf("scsi bus error. icr %x resid %d\n",
		    c->c_har->icr, resid);
		sc_reset(c);
		(*c->c_un->un_ss->ss_intr)(c, resid, SE_FATAL);
		return;
	}
	if (c->c_har->icr & ICR_ODD_LENGTH) {
		if ((c->c_cdb.cmd == SC_READ) || 
		    (c->c_cdb.cmd == SC_REQUEST_SENSE) ||
		    (c->c_cdb.cmd == SC_INQUIRY)) {
			DVMA[un->un_baddr + un->un_dma_count - resid] =
			    c->c_har->data;
			resid--;
		} else if (c->c_cdb.cmd == SC_WRITE) {
			resid++;
		} else {
			printf("scsi: odd length without xfer\n");
		}
	}
	if (resid < 0) {
		panic("scsi resid");
	}
	if (sc_getstatus(c->c_un) == 0) {
		(*c->c_un->un_ss->ss_intr)(c, resid, SE_RETRYABLE);
	} else {
		(*c->c_un->un_ss->ss_intr)(c, resid, SE_NO_ERROR);
	}
}

/*
 * Handle a polling SCSI bus interrupt.
 */
scpoll()
{
	register struct scsi_ctlr *c;
	register int serviced = 0;

	for (c = scctlrs; c < &scctlrs[NSC]; c++) {
		if ((c->c_flags & SCSI_PRESENT) == 0)
			continue;
		if ((c->c_har->icr & (ICR_INTERRUPT_REQUEST | ICR_BUS_ERROR)) 
		    == 0) {
			continue;
		}
		serviced = 1;
		scintr(c);
	}
	return (serviced);
}

/*
 * Clean up queues, free resources, and start next I/O
 * all done after I/O finishes
 * Called by mbdone after moving read data from Mainbus
 */
scdone(mc)
	register struct mb_ctlr *mc;
{
	register struct buf *bp, *dp;
	register struct scsi_unit *un;
	register struct scsi_ctlr *c;

	bp = mc->mc_tab.b_actf->b_actf;
	c = &scctlrs[mc->mc_ctlr];
	un = c->c_un;

	/* advance controller queue */
	dp = mc->mc_tab.b_actf;
	mc->mc_tab.b_active = 0;
	mc->mc_tab.b_actf = dp->b_forw;

	/* advance unit queue */
	dp->b_active = 0;
	dp->b_actf = bp->av_forw;

	iodone(bp);

	/* start next I/O on unit */
	if (dp->b_actf)
		scustart(un);

	/* start next I/O on controller */
	if (mc->mc_tab.b_actf && mc->mc_tab.b_active == 0)
		scstart(un);
}

/*ARGSUSED*/
sc_off(un)
	struct scsi_unit *un;
{

#ifdef notdef
	/* if done to root real bad things happen... */
	un->un_present = 0;
	printf("scsi unit %d/%d offline\n", un->un_target, un->un_lun);
	if (un->un_md->md_dk > 0) {
		dk_mspw[un->un_md->md_dk]=0;
	}
#endif
}

/*
 * Wait for a condition on the scsi bus.
 */
sc_wait(c, cond)
	register struct scsi_ctlr *c;
{
	register struct scsi_ha_reg *har = c->c_har;
	register int i, icr;

	for (i = 0; i < WAIT_COUNT; i++) {
		icr = har->icr;
		if ((icr & cond) == cond) {
			return (1);
		}
		if (icr & ICR_BUS_ERROR) {
			break;
		}
		DELAY(10);
	}
	return (0);
}

/*
 * Wait for the completion of a scsi command.
 */
sc_cmdwait(c)
	register struct scsi_ctlr *c;
{
	if (sc_wait(c, ICR_INTERRUPT_REQUEST) == 0) {
		sc_reset(c);
		return(0);
	} else {
		return(1);
	}
}

/*
 * Put a byte into the scsi command register.
 */
sc_putbyte(c, bits, data)
	register struct scsi_ctlr *c;
	register u_short bits;
	register u_char data;
{
	register struct scsi_ha_reg *har = c->c_har;
	register int icr;

	if (sc_wait(c, ICR_REQUEST) == 0) {
		sc_reset(c);
		return (0);
	}
	icr = har->icr;
	if ((icr & ICR_BITS) != bits) {
#ifdef SCSI_DEBUG
		printf("sc_putbyte error.\n");
		sc_pr_icr("icr is     ", icr);
		sc_pr_icr("waiting for", bits);
#endif SCSI_DEBUG
		sc_reset(c);
		return (0);
	}
	har->cmd_stat = data;
	return (1);
}

/*
 * Get a byte from the scsi command/status register.
 */
sc_getbyte(c, bits)
	register struct scsi_ctlr *c;
{
	register struct scsi_ha_reg *har = c->c_har;
	register int icr;

	if (sc_wait(c, ICR_REQUEST) == 0) {
		sc_reset(c);
		return (-1);
	}
	icr = har->icr;
	if ((icr & ICR_BITS) != bits) {
		if (bits == ICR_STATUS) {
			return (-1);	/* no more status */
		}
#ifdef SCSI_DEBUG
		printf("sc_getbyte error.\n");
		sc_pr_icr("icr is     ", icr);
		sc_pr_icr("waiting for", bits);
#endif SCSI_DEBUG
		sc_reset(c);
		return (-1);
	}
	return (har->cmd_stat);
}

sc_dmacnt(c)
	register struct scsi_ctlr *c;
{
	return ((u_short)(~c->c_har->dma_count));
}

sc_reset(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_ha_reg *har = c->c_har;

	har->icr = ICR_RESET;
	DELAY(50);
	har->icr = 0;
#ifdef SCSI_DEBUG
	printf("scsi host adapter reset\n");
#endif SCSI_DEBUG
}

/*
 * Get status bytes from scsi bus.
 */
sc_getstatus(un)
	register struct scsi_unit *un;
{
	register struct scsi_ctlr *c = un->un_c;
	register u_char *cp = (u_char *)&c->c_scb;
	struct scsi_cdb save_cdb;
	struct scsi_scb save_scb;
	register int i;
	register int b;
	register int save_dma_addr;
	register int save_dma_count;
	register int save_reg_dma_count;
	int resid;
	short retval = 1;
	int s;

	/* get all the status bytes */
	for (i = 0;;) {
		b = sc_getbyte(c, ICR_STATUS);
		if (b < 0) {
			break;
		}
		if (i < STATUS_LEN) {
			cp[i++] = b;
		}
	}

	/* get command complete message */
	b = sc_getbyte(c, ICR_MESSAGE_IN);
	if (b != SC_COMMAND_COMPLETE) {
		if (scsi_debug) {
			printf("Invalid SCSI message: %x\n", b);
			printf("Status bytes:");
			for (b = 0; b < i; b++) {
				printf(" %x", cp[b]);
			}
			printf("\n");
		}
		return (0);
	}
	if (scsi_debug) {
		printf("sc%d: sc_getstatus: Got status ", SCNUM(c));
		for (b = 0; b < i; b++) {
			printf(" %x", cp[b]);
		}
		printf("\n");
	}
	if (c->c_scb.busy) {
		return (0);
	}

	/* check for sense data */
	if (c->c_scb.chk) {

		/* save information while we get sense */
		save_cdb = c->c_cdb;
		save_scb = c->c_scb;
		save_dma_addr = un->un_dma_addr;
		save_dma_count = un->un_dma_count;
		save_reg_dma_count = c->c_har->dma_count;

		/* set up for getting sense */
		c->c_cdb.cmd = SC_REQUEST_SENSE;
		c->c_cdb.lun = un->un_lun;
		cdbaddr(&c->c_cdb, 0);
		c->c_cdb.count = sizeof(struct scsi_sense);
		un->un_dma_addr = (int)c->c_sense - (int)DVMA;
		un->un_dma_count = sizeof(struct scsi_sense);

		/* get sense */
		if ((s=0),sc_cmd(c, un, 0) == 0 ||
		    (s=1),sc_cmdwait(c) == 0 ||
		    (s=2),sc_getstatus(un) == 0) {
			printf("scsi: cannot get sense %d\n", s);
			sc_off(un);
			retval = 0;
		} 
		resid = (u_short) (~c->c_har->dma_count);
		if (resid < 0) {
			printf("scsi: dma_count %x resid %d",
			    c->c_har->dma_count, resid);
			panic("scsi sense count too big.");
		}

		/* restore pre sense information */
		c->c_cdb = save_cdb;
		c->c_scb = save_scb;
		un->un_dma_addr = save_dma_addr;
		un->un_dma_count = save_dma_count;
		c->c_har->dma_count = save_reg_dma_count;
	}
	return (retval);
}

#ifdef SCSI_DEBUG
/*
 * Print out the scsi host adapter interface control register.
 */
sc_pr_icr(cp, i)
	register char *cp;
{
	printf("\t%s: %x ", cp, i);
	if (i & ICR_PARITY_ERROR)
		printf("Parity err ");
	if (i & ICR_BUS_ERROR)
		printf("Bus err ");
	if (i & ICR_ODD_LENGTH)
		printf("Odd len ");
	if (i & ICR_INTERRUPT_REQUEST)
		printf("Int req ");
	if (i & ICR_REQUEST) {
		printf("Req ");
		switch (i & ICR_BITS) {
		case 0:
			printf("Data out ");
			break;
		case ICR_INPUT_OUTPUT:
			printf("Data in ");
			break;
		case ICR_COMMAND_DATA:
			printf("Command ");
			break;
		case ICR_COMMAND_DATA | ICR_INPUT_OUTPUT:
			printf("Status ");
			break;
		case ICR_MESSAGE | ICR_COMMAND_DATA:
			printf("Msg out ");
			break;
		case ICR_MESSAGE | ICR_COMMAND_DATA | ICR_INPUT_OUTPUT:
			printf("Msg in ");
			break;
		default:
			printf("DCM: %x ", i & ICR_BITS);
			break;
		}
	}
	if (i & ICR_PARITY)
		printf("Parity ");
	if (i & ICR_BUSY)
		printf("Busy ");
	if (i & ICR_SELECT)
		printf("Sel ");
	if (i & ICR_RESET)
		printf("Reset ");
	if (i & ICR_PARITY_ENABLE)
		printf("Par ena ");
	if (i & ICR_WORD_MODE)
		printf("Word mode ");
	if (i & ICR_DMA_ENABLE)
		printf("Dma ena ");
	if (i & ICR_INTERRUPT_ENABLE)
		printf("Int ena ");
	printf("\n");
}

/*
 * Print out status completion block.
 */
sc_pr_scb(scbp)
	register struct scsi_scb *scbp;
{
	register u_char *cp;

	cp = (u_char *) scbp;
	printf("scb: %x", cp[0]);
	if (scbp->ext_st1) {
		printf(" %x", cp[1]);
		if (scbp->ext_st2) {
			printf(" %x", cp[2]);
		}
	}
	if (scbp->is) {
		printf(" intermediate status");
	}
	if (scbp->busy) {
		printf(" busy");
	}
	if (scbp->cm) {
		printf(" condition met");
	}
	if (scbp->chk) {
		printf(" check");
	}
	if (scbp->ext_st1 && scbp->ha_er) {
		printf(" host adapter detected error");
	}
	printf("\n");
}
#endif SCSI_DEBUG
#endif NSC > 0
