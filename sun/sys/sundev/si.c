#ifndef lint
static	char sccsid[] = "@(#)si.c	1.19 87/04/14	Copyr 1986 Sun Micro";
#endif
/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "si.h"

#if NSI > 0
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

struct	scsi_ctlr sictlrs[NSI];			/* per controller structs */
#define SINUM(si)	(si - sictlrs)

int	siprobe(), sislave(), siattach(), sigo(), sidone(), sipoll();
int	siustart(), sistart(), si_getstatus();
int	si_off(), si_cmd(), si_cmdwait(), si_reset(), si_dmacnt();

struct	mb_ctlr *siinfo[NSI];
extern struct mb_device *sdinfo[];
struct	mb_driver sidriver = {
	siprobe, sislave, siattach, sigo, sidone, sipoll,
	sizeof (struct scsi_si_reg), "sd", sdinfo, "si", siinfo, MDR_BIODMA,
};

/* routines available to devices specific portion of scsi driver */
struct scsi_ctlr_subr sisubr = {
	siustart, sistart, sidone, si_cmd, si_getstatus, si_cmdwait,
	si_off, si_reset, si_dmacnt, sigo,
};

/*
 * Shorthand, to make the code look a bit cleaner.
 */

#define SBC_RD	sir->sbc_rreg	/* SBC read regs, sir points to SCSI ha regs */
#define SBC_WR	sir->sbc_wreg	/* SBC write regs, sir points to SCSI ha regs */

char *si_str_phase();

#define SCSI_DEBUG 1		/* turn on debugging code */

#ifdef SCSI_DEBUG
extern int scsi_debug;		/* generic debug information */
#endif SCSI_DEBUG

extern int scsi_ntype;
extern struct scsi_unit_subr scsi_unit_subr[];

/*
 * Software copy of ser state (debug only)
 */
u_char ser_state = 0;

u_char last_phase_was_data = 0;		/* true if last phase was data */
#define PHASE_READ	1		/* possible values ... */
#define PHASE_WRITE	2

/*
 * Patchable delays for debugging.
 */
int si_udc_wait = SI_UDC_WAIT;
int si_arbitration_delay = SI_ARBITRATION_DELAY;
int si_bus_clear_delay = SI_BUS_CLEAR_DELAY;
int si_bus_settle_delay = SI_BUS_SETTLE_DELAY;

/*
 * possible return values from si_arb_sel(), si_cmd()
 */
#define FAIL		0	/* failed */ 
#define OK		1	/* successful */ 
#define RESEL_FAIL	2	/* failed due to reselection */ 

/* possible return values from si_process_complete_msg() */
#define CMD_CMPLT_DONE	0	/* cmd processing done */ 
#define CMD_CMPLT_WAIT	1	/* cmd processing waiting on sense cmd cmplt */

extern int scsi_disre_enable;	/* enable disconnect/reconnect */

#ifdef SCSI_DEBUG
int scsi_dis_debug = 0;		/* disconnect debug info */
int scsi_reset_debug = 0;	/* scsi bus reset debug information */
u_int siintr_winner = 0;	/* # of times we had an intr at end of siintr */
u_int siintr_loser = 0;		/* # of times we didn't have an intr at end */
#endif SCSI_DEBUG

/*
 * trace buffer stuff.
 */


#ifdef SCSI_DEBUG
u_char scsi_trace_buf[(0x100 * 20)];	/* trace buffer */
u_char scsi_tape_trace_buf[(0x100 * 20)];	/* trace buffer */
int stbi = 0;
int sttbi = 0;

SCSI_TRACE(where, sir, un)
	char where;
	register struct scsi_si_reg *sir;
	struct scsi_unit *un;
{
	register u_char *r = &(SBC_RD.cdr);
	register u_int i;
	register struct tbuf {
		u_char wh[2];
		u_char r[6];
		u_short csr;
		u_short bcr;
		u_int dma_addr;
		u_int dma_count;
	} *tb;
	register u_char *x = &(scsi_trace_buf[stbi]);

	tb = (struct tbuf *) x;

	tb->wh[0] = tb->wh[1] = where;
	for (i = 0; i < 6; i++)
		tb->r[i] = *r++;
	tb->csr = sir->csr;
	tb->bcr = sir->bcr;
	tb->dma_addr = sir->dma_addr;
	tb->dma_count = sir->dma_count;

	(u_int)x += 20;
	if ((u_int)x >= ((int)scsi_trace_buf + (0x100 * 20)))
		stbi = 0;
	else
		stbi += 20;
	*x = '?';		/* mark end */

	if (un && un->un_target == 4)	{
		/* tape */
		x = &(scsi_tape_trace_buf[sttbi]);

		bcopy((char *)tb, (char *)x, sizeof(struct tbuf));

		(u_int)x += 20;
		if ((u_int)x >= ((int)scsi_tape_trace_buf + (0x100 * 20)))
			sttbi = 0;
		else
			sttbi += 20;
		*x = '?';		/* mark end */
	}
}
#else  SCSI_DEBUG

#define SCSI_TRACE(where, sir, un)	{}

#endif SCSI_DEBUG


#ifdef SCSI_DEBUG
u_char scsi_recon_trace_buf[(0x100 * 22)];	/* trace buffer */
int strbi = 0;
SCSI_RECON_TRACE(where, c, data1, data2, data3)
	char where;
	register struct scsi_ctlr *c;
	int data1, data2, data3;
{
	register u_char *x = &(scsi_recon_trace_buf[strbi]);
	register struct scsi_unit *un = c->c_un;
	register u_char *cdb;
	register int i;

	*x++ = where; *x++ = where;
	cdb = (u_char *)&c->c_cdb;

	for (i = 0; i < 6; i++)
		*x++ = *cdb++;
	*x++ = un->un_target;
	*x++ = un->un_lun;
	*(int *)x = data1;
	x += 4;
	*(int *)x = data2;
	x += 4;
	*(int *)x = data3;
	x += 4;

	if ((int)x >= ((int)scsi_recon_trace_buf + (0x100 * 22)))
		strbi = 0;
	else
		strbi += 22;
	*x = '?';		/* mark end */
}

#else  SCSI_DEBUG

#define SCSI_RECON_TRACE(where, c, data1, data2, data3)	{}

#endif SCSI_DEBUG

#ifdef SCSI_DEBUG
/*
 * Ends a DMA transfer from the point of view of the 5380.
 */
#define SI_VME_OK(c, sir, str)	{\
	if ((IS_VME(c)) && (sir->csr & SI_CSR_DMA_EN)) \
		printf("si: reg access during dma <%s>, csr %x\n", str, sir);\
}
#else  SCSI_DEBUG

#define SI_VME_OK(c, sir, str) {}

#endif SCSI_DEBUG

/*
 * Determine existence of SCSI host adapter.
 */
siprobe(reg, ctlr)
	register struct scsi_si_reg *reg;
	register int ctlr;
{
	register struct scsi_ctlr *c;

#ifdef SCSI_DEBUG
	if (scsi_debug)
		printf("siprobe: reg %x ctlr %d\n", (u_int) reg, ctlr);
#endif SCSI_DEBUG

	/* probe for different scsi host adaptor interfaces */
	c = &sictlrs[ctlr];

	/* 
	 * Check for sbc - NCR 5380 Scsi Bus Ctlr chip.
	 * sbc is common to sun3/50 onboard scsi and vme
	 * scsi board.
	 */
	if (peekc((char *)&reg->sbc_rreg.cbsr) == -1) {
		return (0);
	}

	/*
	 * Determine whether the host adaptor interface is onboard or vme.
	 */
	if (cpu == CPU_SUN3_50 || cpu == CPU_SUN3_60) {
		/* probe for sun3/50 dma interface */
		if (peek((short *)&reg->udc_rdata) == -1)
			return (0);
		c->c_flags = SCSI_ONBOARD;
	} else {
		/*
		 * Probe for vme scsi card but make sure it is not
		 * the SC host adaptor interface. SI vme scsi host
		 * adaptor occupies 2K bytes in the vme address space. 
		 * SC vme scsi host adaptor occupies 4K bytes in the 
		 * vme address space. So, peek past 2K bytes to 
		 * determine which host adaptor is there.
		 */
		if (peek((short *)&reg->dma_addr) == -1)
			return (0);
		if (peek((short *)((int)reg+0x800)) != -1)
			return (0);
		if (!(reg->csr & SI_CSR_ID)) {
			printf("\nUNMODIFIED SCSI-3 BOARD! PLEASE UPGRADE!\n");
			return (0);
		}
		c->c_flags = SCSI_VME;
	}

	/* allocate memory for sense information */
	c->c_sense = (struct scsi_sense *) rmalloc(iopbmap,
	    (long) (sizeof (struct scsi_sense) + 2));
	if (c->c_sense == NULL) {
		printf("siprobe: no iopb memory for sense.\n");
		return (0);
	}

	if ((int)c->c_sense & 0x01)
		((caddr_t)c->c_sense)++;

	/* init controller information */
	c->c_flags |= SCSI_PRESENT;
	if (scsi_disre_enable)
		c->c_flags |= SCSI_EN_DISCON;
	c->c_sir = reg;
	c->c_ss = &sisubr;
	c->c_num_sense_pending = 0;
	si_reset(c);

#ifdef SCSI_DEBUG
	if (scsi_debug)
		printf("si_probe: found\n");
#endif SCSI_DEBUG

	return (sizeof (struct scsi_si_reg));
}


/*
 * See if a slave exists.
 * Since it may exist but be powered off, we always say yes.
 */
/*ARGSUSED*/
sislave(md, reg)
	register struct mb_device *md;
	register struct scsi_si_reg *reg;
{
	register struct scsi_unit *un;
	register int type;

#ifdef lint
	reg = reg;
#endif lint

	/*
	 * This kludge allows autoconfig to print out "sd" for
	 * disks and "st" for tapes.  The problem is that there
	 * is only one md_driver for scsi devices.
	 */
	type = TYPE(md->md_flags);
	if (type >= scsi_ntype) {
		panic("sislave: unknown type in md_flags");
	}

	/* link unit to its controller */
	un = (struct scsi_unit *)(*scsi_unit_subr[type].ss_unit_ptr)(md);
	if (un == 0) {
		panic("sislave: md_flags scsi type not configured in\n");
	}
	un->un_c = &sictlrs[md->md_ctlr];
	md->md_driver->mdr_dname = scsi_unit_subr[type].ss_devname;
	return (1);
}

/*
 * Attach device (boot time).
 */
siattach(md)
	register struct mb_device *md;
{
	register int type = TYPE(md->md_flags);
	register struct mb_ctlr *mc = md->md_mc;
	register struct scsi_ctlr *c = &sictlrs[md->md_ctlr];

#ifdef SCSI_DEBUG
	if (scsi_debug)
		printf("siattach\n");
#endif SCSI_DEBUG

	if (type >= scsi_ntype) {
		panic("siattach: unknown type in md_flags");
	}
	(*scsi_unit_subr[type].ss_attach)(md);

	if (IS_ONBOARD(c))
		return;

	/* it's a vme... */

	/* 
	 * Make sure dma enable bit is off or 
	 * SI_CSR_DMA_CONFLICT will occur when 
	 * the iv_am register is accessed.
	 */
	c->c_sir->csr &= ~SI_CSR_DMA_EN;


	/* 
	 * Initialize interrupt vector and address modifier register.
	 * Address modifier specifies standard supervisor data access
	 * with 24 bit vme addresses. May want to change this in the
	 * future to handle 32 bit vme addresses.
	 */
	if (mc->mc_intr) {
		/* setup for vectored interrupts - we will pass ctlr ptr */
		c->c_sir->iv_am = (mc->mc_intr->v_vec & 0xff) | 
		    VME_SUPV_DATA_24;
		(*mc->mc_intr->v_vptr) = (int)c;
	}
}

/*
 * SCSI unit start routine.
 * Called by SCSI device drivers.
 * Does not actually start any I/O - just puts the device
 * on the ready queue for the bus.
 */
siustart(un)
	register struct scsi_unit *un;			/* our unit */
{
	register struct buf *dp;
	register struct mb_ctlr *mc;			/* unit's ctlr */

#ifdef SCSI_DEBUG
	if (scsi_debug)
		printf("siustart\n");
#endif SCSI_DEBUG

	mc = un->un_mc;
	dp = &un->un_utab;
	/* 
	 * Caller guarantees: dp->b_actf != NULL && dp->b_active == 0 
	 * Note: dp->b_active == 1 on a reconnect.
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
 * Set up a scsi operation.
 */
sistart(un)
	register struct scsi_unit *un;			/* our unit */
{
	register struct mb_ctlr *mc;			/* unit's ctlr */
	register struct buf *bp, *dp;

#ifdef SCSI_DEBUG
	if (scsi_debug)
		printf("sistart\n");
#endif SCSI_DEBUG

	mc = un->un_mc;
	dp = mc->mc_tab.b_actf;	     /* != NULL guaranteed by caller */
	un = (struct scsi_unit *) dp->b_un.b_addr;
	bp = dp->b_actf;
	for (;;) {
		if (bp == NULL) {	  /* no more blocks for this device */
			un->un_utab.b_active = 0;
			dp = mc->mc_tab.b_actf = dp->b_forw;
			if (dp == NULL) {  /* no more devices for this ctlr */
				si_idle(un->un_c);
				return;			/* done - out we go! */
			}
			un = (struct scsi_unit *) dp->b_un.b_addr;
			bp = dp->b_actf;
		} else {
			/*
			 * We have something to do - call unit start routine.
			 */
			if ((*un->un_ss->ss_start)(bp, un)) {
				mc->mc_tab.b_active = 1;
				un->un_c->c_un = un;
				if (bp == &un->un_sbuf  &&
				    ((un->un_flags & SC_UNF_DVMA) == 0)) {
					/*
					 * special command, 
					 * doesn't need DVMA.
					 */
					sigo(mc);
				} else {
					/* do DVMA setup first */
					(void) mbgo(mc);
				}
				return;			/* done - out we go! */
			}
			dp->b_actf = bp = bp->av_forw;
		}
	}
}

/*
 * Start up a scsi operation.
 * Called via mbgo after buffer is in memory.
 */
sigo(mc)
	register struct mb_ctlr *mc;		/* fire up this mb ctlr */
{
	register struct scsi_unit *un;		/* our scsi unit */
	register struct scsi_ctlr *c;		/* our scsi ctlr */
	register struct buf *bp, *dp;
	register int unit;			/* unit for stat purposes */

#ifdef SCSI_DEBUG
	if (scsi_debug)
		printf("sigo\n");
#endif SCSI_DEBUG

	c = &sictlrs[mc->mc_ctlr];
	dp = mc->mc_tab.b_actf;

	if (dp == NULL || dp->b_actf == NULL) {
		panic("sigo queueing error 1");
	}

	bp = dp->b_actf;
	un = c->c_un;

	if (dp != &un->un_utab) {
		panic("sigo queueing error 2");
	}

	un->un_baddr = MBI_ADDR(mc->mc_mbinfo);

	/*
	 * Diddle stats if necessary.
	 */
	if ((unit = un->un_md->md_dk) >= 0) {
		dk_busy |= 1<<unit;
		dk_xfer[unit]++;
		dk_wds[unit] += bp->b_bcount >> 6;
	}

	/*
	 * Make the command block and fire it up in interrupt mode.
	 * If it fails right off the bat, call the interrupt routine 
	 * to handle the failure.
	 */
	(*un->un_ss->ss_mkcdb)(c, un);
	if (si_cmd(c, un, 1) == 0) {
		(*un->un_ss->ss_intr)(c, 0, SE_RETRYABLE);
		si_off(un);
	}
}

/*
 * Handle a polling SCSI bus interrupt.
 */
sipoll()
{
	register struct scsi_ctlr *c;
	register int serviced = 0;

#ifdef SCSI_DEBUG
	if (scsi_debug)
		printf("sipoll\n");
#endif SCSI_DEBUG

	for (c = sictlrs; c < &sictlrs[NSI]; c++) {
		if ((c->c_flags & SCSI_PRESENT) == 0)
			continue;
		if ((c->c_sir->csr & 
		    (SI_CSR_SBC_IP | SI_CSR_DMA_IP | SI_CSR_DMA_CONFLICT)) 
		    == 0) {
			continue;
		}
		serviced = 1;
		siintr(c);
	}
	return (serviced);
}

/*
 * Clean up queues, free resources, and start next I/O
 * all done after I/O finishes
 * Called by mbdone after moving read data from Mainbus
 */
sidone(mc)
	register struct mb_ctlr *mc;
{
	register struct buf *bp, *dp;
	register struct scsi_unit *un;
	register struct scsi_ctlr *c;

#ifdef SCSI_DEBUG
	if (scsi_debug)
		printf("sidone\n");
#endif SCSI_DEBUG

	bp = mc->mc_tab.b_actf->b_actf;
	c = &sictlrs[mc->mc_ctlr];
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
		siustart(un);

	/* start next I/O on controller */
	if (mc->mc_tab.b_actf && mc->mc_tab.b_active == 0) {
		dp = mc->mc_tab.b_actf;
		un = (struct scsi_unit *)dp->b_un.b_addr;
		/* 
		 * If this activity was preempted due to a 
		 * reselection coming in, much of the setup has 
		 * already taken place and must not be redone.
		 */
		if (un->un_flags & SC_UNF_PREEMPT) {
			un->un_flags &= ~SC_UNF_PREEMPT;
			c->c_un = un;
			c->c_cdb = un->un_saved_cmd.saved_cdb;
			mc->mc_tab.b_active = 1;
			if (un->un_dma_curdir != SI_NO_DATA) {
				mc->mc_mbinfo = un->un_baddr;
			}
			if (si_cmd(c, un, 1) == 0) {
				(*un->un_ss->ss_intr)(c, 0, SE_FATAL);
				si_off(un);
			}
		} else if (un->un_flags & SC_UNF_GET_SENSE) {
			sigo(mc);
		} else {
			sistart(un);
		}
	} else {
		c->c_un = NULL;
		si_idle(c);
	}
}

/*
 * Bring a unit offline.
 */
/*ARGSUSED*/
si_off(un)
	register struct scsi_unit *un;
{
#ifdef lint
	un = un;
#endif lint

#ifdef SCSI_DEBUG
	if (scsi_debug)
		printf("si_off\n");
#endif SCSI_DEBUG

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
 * Pass a command to the SCSI bus.
 * Return FAIL on failure, OK if fully successful, 
 * RESEL_FAIL if we failed due to target being in process of reselecting us.
 */
si_cmd(c, un, intr)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
	register int intr;		/* if 1, allow disconnects
					 * if 2, run cmd to completion
					 * through interrupt path, disallowing
					 * disconnects.
					 * if 0, run cmd in polled mode 
					 */
{
	register struct scsi_si_reg *sir = c->c_sir;
	register int err;
	u_char id;

#ifdef SCSI_DEBUG
	if (scsi_debug) {
		printf("si%d: si_cmd: tgt %d want intr %d cmd ",
		     SINUM(c), un->un_target, intr);
		si_print_cdb(c);
	}
#endif SCSI_DEBUG

	/* disallow disconnects if waiting for command completion */
	if (intr == 0) {
		c->c_flags &= ~SCSI_EN_DISCON;
	} else {
		if (scsi_disre_enable)
			c->c_flags |= SCSI_EN_DISCON;
		else
			c->c_flags &= ~SCSI_EN_DISCON;
		un->un_wantint = 1;
	}

	/*
	 * For vme host adaptor interface, dma enable bit may
	 * be set to allow reconnect interrupts to come in.
	 * This must be disabled before arbitration/selection
	 * of target is done. Don't worry about re-enabling
	 * dma. If arb/sel fails, then si_idle() will re-enable.
	 * If arb/sel succeeds then handling of command will
	 * re-enable.
	 * Also, disallow sbc to accept reconnect attempts.
	 * Again, si_idle() will re-enable this if arb/sel fails.
	 * If arb/sel succeeds then we do not want to allow
	 * reconnects anyway.
	 */
	if (IS_VME(c))
		sir->csr &= ~SI_CSR_DMA_EN;

	SI_VME_OK(c, sir, "start of si_cmd");
	SCSI_TRACE('c', sir, un);

	SBC_WR.ser = ser_state = 0;

	un->un_flags &= ~SC_UNF_DMA_INITIALIZED;

	/* performing target selection */
	if ((err = si_arb_sel(c, un)) != OK) {
		/* 
		 * May not be able to execute this command at this time due
		 * to a target reselecting us. Indicate this in the unit
		 * structure for when we perform this command later.
		 */

		SI_VME_OK(c, sir, "si_cmd: arb_sel_fail");
		SBC_WR.ser = ser_state = SI_HOST_ID;

#ifdef SCSI_DEBUG
		if (scsi_debug)
			printf("si_arb_sel failed, err %d\n", err);
#endif SCSI_DEBUG

		if (err == RESEL_FAIL) {
			un->un_saved_cmd.saved_cdb = c->c_cdb;
			un->un_flags |= SC_UNF_PREEMPT;
		}
		un->un_wantint = 0;
		if (IS_VME(c))
			sir->csr |= SI_CSR_DMA_EN;
		return (err);
	}

	/*
	 * We need to send out an identify message to target.
	 */

	if (intr != 0 && scsi_disre_enable)
		id = SC_DR_IDENTIFY | c->c_cdb.lun;
	else
		id = SC_IDENTIFY | c->c_cdb.lun;

#ifdef SCSI_DEBUG
	if (scsi_debug) {
		printf("si_cmd: before MSG_OUT\n");
		si_print_state(sir);
	}
#endif SCSI_DEBUG

	SI_VME_OK(c, sir, "si_cmd: before msg out");
	SBC_WR.tcr = PHASE_MSG_OUT;

	/*++mjacob 4/14/87 per bug #1004516{
	 *
         * Hack to get around the fact that the sysgen ctlr does
         * support any message but command complete. We could
         * sit here a long time for each command waiting for
         * sysgen to respond to the ATN line (which it doesn't even see).
         * Hence, st.c will figure out that this is a sysgen controller
         * and from thence set this flag. 
         */

        if (!(un->un_flags&SC_UNF_NOMSGOUT)) {
                if (si_wait_phase(sir, PHASE_MSG_OUT)) {
                        if (si_putdata(c, PHASE_MSG_OUT, &id, 1) == 0) {
                                printf("si: synch: putdata of id msg failed\n");                                si_print_state(sir);
                                si_idle(c);
                                return(0);
                        }
                }
        } else {
                /*
                 * turn off this flag. Responsibility of top half
                 * to turn it on.
                 */
                un->un_flags ^= SC_UNF_NOMSGOUT;
        }
	/*}--mjacob */

	if (sir->csr & SI_CSR_DMA_BUS_ERR) {
		printf("si_cmd: dma bus err\n");
		si_reset(c);
		return (0);
	}

	/*
	 * Must split dma setup into 2 parts due to sun3/50
	 * which requires bcr to be set before target
	 * changes phase on scsi bus to data phase.
	 *
	 * Three fields in the per scsi unit structure
	 * hold information pertaining to the current dma
	 * operation: un_dma_curdir, un_dma_curaddr, and
	 * un_dma_curcnt. These fields are used to track
	 * the amount of data dma'd especially when disconnects 
	 * and reconnects occur.
	 * If the current command does not involve dma,
	 * these fields are set appropriately.
	 */

	if (un->un_dma_count > 0) {

		/* reset udc */
		if (IS_ONBOARD(c)) {
			DELAY(si_udc_wait);
			sir->udc_raddr = UDC_ADR_COMMAND;
			DELAY(si_udc_wait);
			sir->udc_rdata = UDC_CMD_RESET;;
			DELAY(si_udc_wait);
		}

		/* reset fifo */
		sir->csr &= ~SI_CSR_FIFO_RES;
		sir->csr |= SI_CSR_FIFO_RES;

		if (un->un_flags & SC_UNF_RECV_DATA) {
			un->un_dma_curdir = SI_RECV_DATA;
			sir->csr &= ~SI_CSR_SEND;
		} else {
			un->un_dma_curdir = SI_SEND_DATA;
			sir->csr |= SI_CSR_SEND;
		}
		/* save current dma info for disconnect */
		un->un_dma_curaddr = un->un_dma_addr;
		un->un_dma_curcnt = un->un_dma_count;
		un->un_dma_curbcr = 0;

#ifdef SCSI_DEBUG
		if (scsi_debug)
			printf("csr after resetting FIFO %x\n", sir->csr);
#endif SCSI_DEBUG

		if (IS_ONBOARD(c))
			/* must init bcr before tgt goes into data phase */
			sir->bcr = un->un_dma_curcnt;
		else {
			/* 
			 * Currently we don't use all 24 bits of the
			 * count register on the vme interface. To do
			 * this changes are required other places, e.g.
			 * in the scsi_unit structure the fields
			 * un_dma_curcnt and un_dma_count would need to
			 * be changed.
			 */
			sir->bcr = sir->bcrh = sir->dma_count = 0;
		}

#ifdef SCSI_DEBUG
		if (scsi_debug)
			printf("si_cmd: after bcr setup: bcr %x\n", sir->bcr);
#endif SCSI_DEBUG

		/*
		 * New: we set up everything we can here, rather
		 * than wait until data phase.
		 */
		
		if (IS_ONBOARD(c))
			si_ob_dma_setup(c, un);
		else {
			SI_VME_OK(c, sir, "si_cmd: before cmd phase");
			si_vme_dma_setup(c, un);
#ifdef SCSI_DEBUG
			if (scsi_debug)
				printf("si_cmd: after vme setup: bcr %x\n", 
					sir->bcr);
#endif SCSI_DEBUG

		}
	} else {
		un->un_dma_curdir = SI_NO_DATA;
		un->un_dma_curaddr = 0;
		un->un_dma_curcnt = 0;
		sir->bcr = 0;
	}

	if (sir->csr & SI_CSR_DMA_BUS_ERR) {
		printf("si_cmd: dma bus err1\n");
		SBC_WR.ser = ser_state = SI_HOST_ID;
		si_reset(c);
		return (0);
	}

	SI_VME_OK(c, sir, "si_cmd: before cmd phase");
	SCSI_TRACE('C', sir, un);

	if (! si_wait_phase(sir, PHASE_COMMAND)) {
		printf("si: synch: no COMMAND phase\n");
		si_print_state(sir);
		si_idle(c);
		return(0);
	}

	SCSI_RECON_TRACE('c', c, *(int *)(DVMA + un->un_dma_addr), 0, 0);

	if (! si_putcmd(c, (u_char *)&c->c_cdb, si_cdb_len(&c->c_cdb), intr)) {
		printf("si: synch: si_cmd_dma failed\n");
		si_print_state(sir);
		si_idle(c);
		return(0);
	}

	if (intr == 0) {
		/* 
		 * Synchronously transfer the data, if need be.
		 */

		if (un->un_dma_curdir != SI_NO_DATA) {
			register u_char phase;
			
			if (un->un_dma_curdir == SI_RECV_DATA)
				phase = PHASE_DATA_IN;
			else
				phase = PHASE_DATA_OUT;

			SI_VME_OK(c, sir, "si_cmd: before data xfer, sync");

			if (! si_wait_phase(sir, phase)) {
				/*
				 * We have a problem with the command
				 * - it has not gone into data phase.
				 * However, we will suppress any error
				 * information and let si_getstatus()
				 * handle the problem.
				 * Thus our return status is a lie, as
				 * si_getstatus will later learn the truth.
				 */
				return (OK);
			}

			/*
			 * Must actually start DMA xfer here - setup
			 * has already been done.
			 */

			/* put sbc in dma mode and start dma transfer */
			si_sbc_dma_setup(c, sir, (int)un->un_dma_curdir);

			/* not relevant for polled commands */
			last_phase_was_data = 0;
		}

		/* wait for DMA to finish */
		if (si_wait((u_short *)&sir->csr, SI_CSR_DMA_ACTIVE, 0)
								== 0) {
			printf("si: sync: DMA never completed\n");
			si_print_state(sir);
			si_idle(c);
			return(0);
		}
	}

	return (OK);
}

/*
 * Perform the SCSI arbitration and selection phases.
 * Returns FAIL if unsuccessful, 
 * returns RESEL_FAIL if unsuccessful due to target reselecting, 
 * returns OK if all was cool.
 */
si_arb_sel(c, un)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
{
	register struct scsi_si_reg *sir = c->c_sir;
	register u_char *icrp = &SBC_WR.icr;
	register u_char *mrp = &SBC_WR.mr;
	register int j;
	register u_char icr;
	int ret_val = OK;

#ifdef SCSI_DEBUG
	if (scsi_debug)
		printf("si_arb_sel\n");
#endif SCSI_DEBUG
	
	SI_VME_OK(c, sir, "si_arb_sel");

	/* 
	 * It seems that the tcr must be 0 for arbitration to work.
	 */

	SBC_WR.tcr = 0;
	*mrp &= ~SBC_MR_DMA;

#ifdef SCSI_DEBUG
	if (scsi_debug) {
		printf("state at beginning of si_arb_sel: ");
		si_print_state(sir);
	}
#endif SCSI_DEBUG

	/* wait for scsi bus to become free */
	if (si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_BSY, 0) == 0) {
		printf("si_arb_sel: continuously busy:\n");
		si_print_state(sir);
		si_reset(c);
		ret_val = FAIL;
		goto done;
	}

	/* check for target attempting a reselection */
	if ((SBC_RD.cbsr & SBC_CBSR_SEL) && 
	    (SBC_RD.cbsr & SBC_CBSR_IO) &&
	    (SBC_RD.cdr & SI_HOST_ID)) {
#ifdef SCSI_DEBUG
		if (scsi_debug)
			/*
			printf("si_arb_sel: REselect, cbsr 0x%x\n",
				SBC_RD.cbsr);
			*/
			printf("si_arb_sel: RE\n");
#endif SCSI_DEBUG

		SCSI_TRACE('r', sir, un);
		ret_val = RESEL_FAIL;
		goto done;
	}

	/* arbitrate for the scsi bus */
	SBC_WR.odr = SI_HOST_ID;
	for (j = 0; j < SI_NUM_RETRIES; j++) {
		*mrp |= SBC_MR_ARB;

		/* wait for sbc to begin arbitration */
		if (si_sbc_wait((caddr_t)icrp, SBC_ICR_AIP, 1) == 0) {
			/*
			 * sbc may never begin arbitration due to a
			 * target reselecting us, the initiator.
			 */
			*mrp &= ~SBC_MR_ARB;
			if ((SBC_RD.cbsr & SBC_CBSR_SEL) && 
			    (SBC_RD.cbsr & SBC_CBSR_IO) &&
			    (SBC_RD.cdr & SI_HOST_ID)) {
#ifdef SCSI_DEBUG
				if (scsi_debug)
					printf("si_arb_sel: REselect\n");
#endif SCSI_DEBUG
				ret_val = RESEL_FAIL;
				goto done;
			}
#ifdef SCSI_DEBUG
			if (scsi_debug)
				/*
				printf("si_arb_sel: AIP never set, cbsr 0x%x\n",
					SBC_RD.cbsr);
				*/
				printf("si_arb_sel: AIP never set\n");
#endif SCSI_DEBUG
			*icrp = 0;
			ret_val = FAIL;
			goto done;
		}

		/* check to see if we won arbitration */
		DELAY(si_arbitration_delay);
		if ( (*icrp & SBC_ICR_LA) == 0 && 
		    	((SBC_RD.cdr & ~SI_HOST_ID) < SI_HOST_ID) ) {
			/* won arbitration */
			icr = *icrp & ~SBC_ICR_AIP;
			*icrp = icr | SBC_ICR_ATN;
			icr = *icrp & ~SBC_ICR_AIP;
			*icrp = icr | SBC_ICR_SEL;
			DELAY(si_bus_clear_delay + si_bus_settle_delay);
			break;
		}

		/* lost arbitration, clear arbitration mode */
		*mrp &= ~SBC_MR_ARB;
	}

	/* couldn't win arbitration */
	if (j == SI_NUM_RETRIES) {
		/* should never happen since we have highest pri scsi id */
		*mrp &= ~SBC_MR_ARB;
		*icrp = 0;
		printf("si_arb_sel: couldn't win arbitration\n");
		ret_val = FAIL;
		goto done;
	}

	/* won arbitration, perform selection */
	SBC_WR.odr = (1 << un->un_target) | SI_HOST_ID;
	icr = *icrp & ~SBC_ICR_AIP;
	DELAY(1);
	*icrp = icr | SBC_ICR_DATA | SBC_ICR_BUSY;
	*mrp &= ~SBC_MR_ARB;

	/* wait for target to acknowledge selection */
	*icrp &= ~SBC_ICR_BUSY;
	if (si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_BSY, 1) == 0) {
		u_char junk;

#ifdef SCSI_DEBUG
		if (scsi_debug) {
			printf("si_arb_sel: cbsr bsy never set, cbsr 0x%x\n",
					SBC_RD.cbsr);
			si_print_state(sir);
		}
#endif SCSI_DEBUG

		*icrp = 0;

		junk = SBC_RD.clr;      /* clear intr due to this error state */
#ifdef lint
                junk = junk;
#endif lint
		ret_val = FAIL;
		goto done;
	}

	*icrp &= ~(SBC_ICR_SEL | SBC_ICR_DATA);

	return (OK);

done:
	return(ret_val);
}

/*
 * Set up the SCSI control logic for a dma transfer for vme host adaptor.
 */
si_vme_dma_setup(c, un)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
{
	register struct scsi_si_reg *sir = c->c_sir;

#ifdef SCSI_DEBUG
	if (scsi_debug)
		printf("si_vme_dma_setup: bcr %x\n",
			sir->bcr);
#endif SCSI_DEBUG

	SI_VME_OK(c, sir, "si_vme_dma_setup");

	/* reset fifo */
	sir->csr &= ~SI_CSR_FIFO_RES;
	sir->csr |= SI_CSR_FIFO_RES;

	SCSI_RECON_TRACE('v', c, un->un_dma_curaddr, (int)un->un_dma_curcnt,
				(int)un->un_dma_curbcr);

#ifdef SCSI_DEBUG
	if (scsi_debug)
		printf("si_vme_dma_setup: after FIFO res: csr %x bcr %x\n", 
			sir->csr, sir->bcr);
#endif SCSI_DEBUG

#ifdef SCSI_DEBUG
	if ((u_int)(un->un_dma_curaddr) & 0x1)
		printf("si_vme_dma_setup: dma_curraddr odd: %x\n",
			un->un_dma_curaddr);
#endif SCSI_DEBUG

	/* setup starting dma address and number bytes to dma */
	sir->dma_addr = un->un_dma_curaddr;
	sir->dma_count = 0;	/* don't start DMA hardware now - kick it 
				 * in si_sbc_dma_setup()
				 */

	/* set up byte packing control info */
	if (sir->dma_addr & 0x2) {
		/* setup word dma transfers across vme bus */
		sir->csr |= SI_CSR_BPCON;
	} else {
		/* setup longword dma transfers across vme bus */
		sir->csr &= ~SI_CSR_BPCON;
	}

#ifdef SCSI_DEBUG
	if (scsi_debug) {
		printf("si_vme_dma_setup exit: addr %x cnt %x csr %x bcr %x\n", 
		    sir->dma_addr, sir->dma_count, sir->csr, sir->bcr);
	}
#endif SCSI_DEBUG
}

/*
 * Set up the SCSI control logic for a dma transfer for onboard host
 * adaptor.
 */
si_ob_dma_setup(c, un)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
{
	register struct scsi_si_reg *sir = c->c_sir;
	register struct udc_table *udct = &c->c_udct;
	register int addr;

	/* reset udc */
	if (IS_ONBOARD(c)) {
		DELAY(si_udc_wait);
		sir->udc_raddr = UDC_ADR_COMMAND;
		DELAY(si_udc_wait);
		sir->udc_rdata = UDC_CMD_RESET;;
		DELAY(si_udc_wait);
	}

	/* reset fifo */
	sir->csr &= ~SI_CSR_FIFO_RES;
	sir->csr |= SI_CSR_FIFO_RES;

#ifdef SCSI_DEBUG
	if (scsi_debug)
		printf("si_ob_dma_setup: after FIFO res: csr %x\n", sir->csr);
#endif SCSI_DEBUG

	/* set up udc dma information */
	addr = un->un_dma_curaddr;
	if (addr < DVMA_OFFSET)
		addr += DVMA_OFFSET;
	udct->haddr = ((addr & 0xff0000) >> 8) | UDC_ADDR_INFO;
	udct->laddr = addr & 0xffff;
	udct->hcmr = UDC_CMR_HIGH;
	udct->count = un->un_dma_curcnt / 2; /* #bytes -> #words */
	if (un->un_dma_curdir == SI_RECV_DATA) {
		udct->rsel = UDC_RSEL_RECV;
		udct->lcmr = UDC_CMR_LRECV;
	} else {
		udct->rsel = UDC_RSEL_SEND;
		udct->lcmr = UDC_CMR_LSEND;
		if (un->un_dma_curcnt & 1) {
			udct->count++;
		}
	}

	/* initialize udc chain address register */
	sir->udc_raddr = UDC_ADR_CAR_HIGH;
	DELAY(si_udc_wait);
	sir->udc_rdata = ((int)udct & 0xff0000) >> 8;
	DELAY(si_udc_wait);
	sir->udc_raddr = UDC_ADR_CAR_LOW;
	DELAY(si_udc_wait);
	sir->udc_rdata = (int)udct & 0xffff;

#ifdef SCSI_DEBUG
	if (scsi_debug) {
		printf("si_ob_dma_setup: udct %x curaddr %x cnt %x -> %x\n", 
			udct, un->un_dma_curaddr, un->un_dma_curcnt, addr);
	}
#endif SCSI_DEBUG

	/* initialize udc master mode register */
	DELAY(si_udc_wait);
	sir->udc_raddr = UDC_ADR_MODE;
	DELAY(si_udc_wait);
	sir->udc_rdata = UDC_MODE;

	/* issue channel interrupt enable command, in case of error, to udc */
	DELAY(si_udc_wait);
	sir->udc_raddr = UDC_ADR_COMMAND;
	DELAY(si_udc_wait);
	sir->udc_rdata = UDC_CMD_CIE;

}

/*
 * Setup and start the sbc for a dma operation.
 */
si_sbc_dma_setup(c, sir, dir)
	register struct scsi_ctlr *c;
	register struct scsi_si_reg *sir;
	register int dir;
{
	register struct scsi_unit *un = c->c_un;

	SCSI_TRACE('G', sir, un);
	SI_VME_OK(c, sir, "si_sbc_dma_setup");

#ifdef SCSI_DEBUG
	if (scsi_debug) {
		printf("si_sbc_dma_setup dir %s\n", 
			(dir == SI_RECV_DATA) ? "IN" : "OUT");
		si_print_state(sir);
	}
#endif SCSI_DEBUG

	if (IS_ONBOARD(c)) {
		/* issue start chain command to udc */
		DELAY(si_udc_wait);
		sir->udc_rdata = UDC_CMD_STRT_CHN;
	} else {
		un->un_flags |= SC_UNF_DMA_INITIALIZED;
		sir->bcr = sir->dma_count = un->un_dma_curcnt;
#ifdef SCSI_DEBUG
		if (scsi_dis_debug)
			printf("si_sbc_dma_setup: dma_count %x (should be %x)\n", 
			sir->dma_count, un->un_dma_curcnt);
#endif SCSI_DEBUG
	}

	SBC_WR.mr |= SBC_MR_DMA;

	if (dir == SI_RECV_DATA) {
		SBC_WR.tcr = TCR_DATA_IN;
		SBC_WR.ircv = 0;
		last_phase_was_data = PHASE_READ;
	} else {
		SBC_WR.tcr = TCR_DATA_OUT;
		SBC_WR.icr = SBC_ICR_DATA;
		SBC_WR.send = 0;
		last_phase_was_data = PHASE_WRITE;
	}

	if (IS_VME(c)) {
		sir->csr |= SI_CSR_DMA_EN;
	}
}

/*
 * Cleanup up the SCSI control logic after a dma transfer.
 */
si_dma_cleanup(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_si_reg *sir = c->c_sir;

#ifdef SCSI_DEBUG
	if (scsi_debug)
		printf("si_dma_cleanup\n");
#endif SCSI_DEBUG

	/* disable dma controller */
	if (IS_ONBOARD(c)) {
		DELAY(si_udc_wait);
		sir->udc_raddr = UDC_ADR_COMMAND;
		DELAY(si_udc_wait);
		sir->udc_rdata = UDC_CMD_RESET;
		DELAY(si_udc_wait);
	} else {
		sir->dma_addr = 0;
		sir->dma_count = 0;
	}

	/* reset fifo */
	sir->csr &= ~SI_CSR_FIFO_RES;
	sir->csr |= SI_CSR_FIFO_RES;

	sir->bcr = 0;
}

/*
 * Handle special dma receive situations, e.g. an odd number of bytes 
 * in a dma transfer.
 * The Sun3/50 onboard interface has different situations which
 * must be handled than the vme interface.
 */
si_dma_recv(c) 
	register struct scsi_ctlr *c;
{
	register struct scsi_si_reg *sir = c->c_sir;
	register struct scsi_unit *un = c->c_un;
	register int offset;

#ifdef SCSI_DEBUG
	if (scsi_debug)
		printf("si_dma_recv\n");
#endif SCSI_DEBUG

	offset = un->un_dma_curaddr + (un->un_dma_curcnt - sir->bcr);

	SCSI_RECON_TRACE('R', c, un->un_dma_curaddr, (int)un->un_dma_curcnt,
				offset);
	SCSI_TRACE('u', sir, un);


	/* handle the onboard scsi situations */
	if (IS_ONBOARD(c)) {
		sir->udc_raddr = UDC_ADR_COUNT;

		/* wait for the fifo to empty */
		if (si_wait((u_short *)&sir->csr, SI_CSR_FIFO_EMPTY, 1) == 0) {
			printf("si_dma_recv: fifo never emptied\n");
			return (0);
		}

		if ((sir->bcr == un->un_dma_curcnt) ||
		    (sir->bcr + 1 == un->un_dma_curcnt))
			/*
			 * Didn't transfer any data.
			 * "Just say no" and leave, rather than
			 * erroneously executing left over byte code.
			 * The bcr + 1 above wards against 5380 prefetch.
			 */
			return (1);

		/* handle odd byte */
		if ((un->un_dma_curcnt - sir->bcr) & 1) {
			DVMA[offset - 1] = (sir->fifo_data & 0xff00) >> 8;

		/*
		 * The udc may not dma the last word from the fifo_data
		 * register into memory due to how the hardware turns
		 * off the udc at the end of the dma operation.
		 */
		} else if (((sir->udc_rdata*2) - sir->bcr) == 2) {
			DVMA[offset - 2] = (sir->fifo_data & 0xff00) >> 8;
			DVMA[offset - 1] = sir->fifo_data & 0x00ff;
		}

	/* handle the vme scsi situations */
	} else if ((sir->csr & SI_CSR_LOB) != 0) {
	    /*
	     * Grabs last few bytes which may not have been dma'd.
	     * Worst case is when longword dma transfers are being done
	     * and there are 3 bytes leftover.
	     * If BPCON bit is set then longword dmas were being done,
	     * otherwise word dmas were being done.
	     */
	    if ((sir->csr & SI_CSR_BPCON) == 0) {
		switch (sir->csr & SI_CSR_LOB) {
		case SI_CSR_LOB_THREE:
			DVMA[offset - 3] = (sir->bpr & 0xff000000) >> 24;
			DVMA[offset - 2] = (sir->bpr & 0x00ff0000) >> 16;
			DVMA[offset - 1] = (sir->bpr & 0x0000ff00) >> 8;
			break;

		case SI_CSR_LOB_TWO:
			DVMA[offset - 2] = (sir->bpr & 0xff000000) >> 24;
			DVMA[offset - 1] = (sir->bpr & 0x00ff0000) >> 16;
			break;

		case SI_CSR_LOB_ONE:
			DVMA[offset - 1] = (sir->bpr & 0xff000000) >> 24;
			break;
		}
	    } else {
		DVMA[offset - 1] = (sir->bpr & 0x0000ff00) >> 8;
	    }
	}
	return (1);
}

/*
 * Handle a scsi interrupt.
 */
siintr(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_si_reg *sir = c->c_sir;
	register struct scsi_unit *un;
	register int status;
	register int resid;
	int reset_occurred;
	u_char junk;
	u_char msg;
	u_short bcr;	/* get it for discon stuff BEFORE we clr int */
	u_int lun;

	/* 
	 * For vme host adaptor interface, must disable dma before
	 * accessing any registers other than the csr or the 
	 * SI_CSR_DMA_CONFLICT bit in the csr will be set.
	 */
head_siintr:
	un = c->c_un;

	if (IS_VME(c)) {
		sir->csr &= ~SI_CSR_DMA_EN;
		SI_VME_OK(c, sir, "top of siintr");
	}

	status = SE_NO_ERROR;
	resid = 0;
	reset_occurred = 0;

#ifdef SCSI_DEBUG
	if (scsi_debug) {
		printf("siintr: ");
		si_print_state(sir);
	}
#endif SCSI_DEBUG

	SCSI_TRACE('i', sir, un);

	/*
	 * We need to store the contents of the byte count register
	 * before we change the state of the 5380.  The 5380 has
	 * a habit of prefetching data before it knows whether it
	 * needs it or not, and this can throw off the bcr.
	 */

 	bcr = sir->bcr;

	SBC_WR.tcr = TCR_UNSPECIFIED;

	/*
	 * Determine source of interrupt.
	 */

	if (sir->csr & (SI_CSR_DMA_IP | SI_CSR_DMA_CONFLICT)) {
		/*
		 * DMA related error.
		 */

		if (sir->csr & SI_CSR_DMA_BUS_ERR) {
			printf("siintr: bus error during dma\n");
		} else if (sir->csr & SI_CSR_DMA_CONFLICT) {
			printf("siintr: invalid reg access during dma\n");
		} else {
			if (IS_ONBOARD(c))
				printf("siintr: dma ip, unknown reason\n");
			else
				printf("siintr: dma overrun\n");
		}

		/*
		 * Either we were waiting for an interrupt on a phase change 
		 * on the scsi bus, an interrupt on a reconnect attempt,
		 * or an interrupt upon completion of a real dma operation.
		 * Each of these situations must be handled appropriately.
		 */

		if (un == NULL) {
			/*
			 * Waiting for reconnect.
			 */
			goto reset_and_leave;
		} else if (un->un_flags & SC_UNF_DMA_ACTIVE) {
			/*
			 * Unit was DMAing, must clean up.
			 */

			resid = bcr;
			si_dma_cleanup(c);
			un->un_flags &= ~SC_UNF_DMA_ACTIVE;
		}

		status = SE_FATAL;
		goto handle_spurious_and_leave;
	}

	/*
	 * We have an SBC interrupt due to a phase change on the bus
	 * or a reconnection attempt.
	 */


	junk = SBC_RD.clr;		 /* acknowledge sbc interrupt */
#ifdef lint
	junk = junk;
#endif lint

	/* check for reconnect attempt */
	if ((SBC_RD.cbsr & SBC_CBSR_SEL) && 
	    (SBC_RD.cbsr & SBC_CBSR_IO) &&
	    (SBC_RD.cdr & SI_HOST_ID)) {
		register u_char cdr;
		register int i;
handle_reconnect:

#ifdef SCSI_DEBUG
		if (scsi_debug || scsi_dis_debug)
			printf("siintr: REconnect\n");
#endif SCSI_DEBUG

		/* get reselecting target scsi id */
		cdr = SBC_RD.cdr & ~SI_HOST_ID;

#ifdef SCSI_DEBUG
		if (scsi_debug)
			printf("si_recon: cdr %x\n", cdr);
#endif SCSI_DEBUG

		/* make sure there are only 2 scsi id's set */
		for (i=0; i < 8; i++) {
			if (cdr & (1<<i))
				break;
		}

		cdr &= ~(1<<i);
		if (cdr != 0) {
			printf("si_recon: > 2 scsi ids, cdr %x\n", SBC_RD.cdr);
			goto reset_and_leave;
		}
		
		c->c_recon_target = i;		/* save for reconnection code */

		SBC_WR.ser = ser_state = 0;	/* disable other reconnects */
		junk = SBC_RD.clr;

		 /* acknowledge reselection */
		SBC_WR.icr |= SBC_ICR_BUSY;

		if (! si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_SEL, 0)) {
			/* target did not remove SEL */
			printf("siintr: resel: target did not release SEL\n");
			goto reset_and_leave;
		}

		SBC_WR.icr &= ~SBC_ICR_BUSY;

#ifdef SCSI_DEBUG
		if (scsi_debug) {
			printf("after recon releases BSY: ");
			si_print_state(sir);
		}
#endif SCSI_DEBUG

		if (! si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ, 1)) {
			printf("si_recon: never saw MSG_IN req\n");
			goto reset_and_leave;
		}

		if ((SBC_RD.cbsr & CBSR_PHASE_BITS) != PHASE_MSG_IN) {
			printf("si_recon: had req, but no MSG_IN\n");
			junk = SBC_RD.clr;	/* clear int, if any */
			goto reset_and_leave;
		}

		junk = SBC_RD.clr;		/* clear int, if any */

		goto handle_message_in;
	}

	/*
	 * We know that we have a new phase we have to handle.
	 */
	
	switch (SBC_RD.cbsr & CBSR_PHASE_BITS) {

	case PHASE_DATA_IN:
	case PHASE_DATA_OUT:

#ifdef SCSI_DEBUG
		if (scsi_debug) {
			printf("siintr: PHASE_DATA_IN/OUT: state ");
			si_print_state(sir);
		}
#endif SCSI_DEBUG

		if (un == NULL) {
			printf("siintr: spurious DATA_IN/OUT cbsr %x\n", 
					SBC_RD.cbsr);
			goto reset_and_leave;
		}

		if (un->un_dma_curcnt <= 0 || un->un_dma_curdir == SI_NO_DATA) {
			printf("siintr: data phs no data curcnt %x curdir %x\n",
				un->un_dma_curcnt, un->un_dma_curdir);
			goto reset_and_leave;
		}

		si_sbc_dma_setup(c, sir, (int)un->un_dma_curdir);
		goto leave;

	case PHASE_MSG_IN:
handle_message_in:

#ifdef SCSI_DEBUG
		if (scsi_debug) {
			printf("before si_decode_msg: ");
			si_print_state(sir);
		}
#endif SCSI_DEBUG

		msg = SBC_RD.cdr;		 /* peek at message */
		lun = msg & 0x07;
		msg &= 0xf0;			/* mask off unit number */

#ifdef SCSI_DEBUG
		if (scsi_debug)
			printf("si_decode message: msg %x lun %d\n", msg, lun);
#endif SCSI_DEBUG

		if ((msg == SC_IDENTIFY) || (msg == SC_DR_IDENTIFY)) {
			/*
			 * If we have a reconnect, we want to do our
			 * DMA setup before we go into DATA phase.
			 * This is why we peek at the message via the cdr
			 * rather than doing an si_getdata() from the start.
			 * si_reconnect() acknowledges the reconnect message.
			 */

			if (si_reconnect(c, c->c_recon_target, lun) == 0) {
				goto reset_and_leave;
			} else {
				goto set_up_for_next_intr_and_leave;
			}
		}

		if (SBC_RD.cdr == SC_DISCONNECT && 
				un->un_dma_curdir == SI_RECV_DATA) {
			un->un_dma_curbcr = bcr;
			SCSI_RECON_TRACE('a', c, 
				(int)un->un_dma_curaddr, 
				(int)un->un_dma_curbcr, 
				(int)sir->bcr);
		}


		if (SBC_RD.cdr == SC_DISCONNECT) {
			SBC_WR.ser = ser_state = 0;	/* disable reconnects */
		}

		msg = si_getdata(c, PHASE_MSG_IN);

		switch (msg) {
		case SC_SAVE_DATA_PTR:
			/* save the bcr before the bastard pre-fetches again */
			un->un_dma_curbcr = bcr;

		case SC_RESTORE_PTRS:
			/* these messages are noise  - ignore them */
			goto set_up_for_next_intr_and_leave;
		
		case SC_DISCONNECT:
			if (! si_disconnect(c)) {
				printf("siintr: disconnect botch\n");
				goto reset_and_leave;
			}

			SBC_WR.ser = ser_state = SI_HOST_ID;
			if ((SBC_RD.cbsr & SBC_CBSR_SEL) &&
			    (SBC_RD.cbsr & SBC_CBSR_IO) &&
			    (SBC_RD.cdr & SI_HOST_ID)) {
				/*
				 * We have a reconnect pending already.
				 * This happens when stupid SCSI controllers
				 * automatically disconnect, even when
				 * they don't have to.
				 * Rather than field another interrupt,
				 * let's go handle it.
				 */
				junk = SBC_RD.clr;	/* clear interrupt */

				goto handle_reconnect;
			}

			goto start_next_command_and_leave;

		case SC_COMMAND_COMPLETE:
			goto hand_off_intr;

		default:
			printf("siintr: unknown message %x\n", msg);
			goto handle_spurious_and_leave;
		}

	case PHASE_STATUS:
		/*
		 * get status bytes
		 */

		if (un->un_dma_curdir == SI_RECV_DATA) {
			if (!si_dma_recv(c)) {
				status = SE_FATAL;
				goto hand_off_intr;
			}
		}

		if (! si_status_bytes(un)) {
			printf("siintr: could not get status\n");
			status = SE_FATAL;
			goto hand_off_intr;
		}

		SCSI_TRACE('s', sir, un);

		goto set_up_for_next_intr_and_leave;

	default:
		printf("siintr: spurious phase: ");
		si_print_state(sir);
		goto handle_spurious_and_leave;
	}
		
hand_off_intr:

	if (IS_VME(c))
		sir->csr &= ~SI_CSR_DMA_EN;

	/* pass interrupt info to unit */
	if (un && un->un_wantint) {
		un->un_wantint = 0;

		SCSI_RECON_TRACE('f', c, 0, 0, 0);

#ifdef SCSI_DEBUG
		if (scsi_debug)
			printf("siintr: passing int, stat %d rst %d\n",
				status, reset_occurred);
#endif SCSI_DEBUG

		if ((status == SE_FATAL) && (reset_occurred == 0))
			si_reset(c);

		if (un->un_dma_curdir != SI_NO_DATA) {
			if (bcr == 0xffff)	/* fix pre-fetch botch */
				bcr = 0;
			resid = bcr;	/* was sir->bcr */
		}

		(*un->un_ss->ss_intr)(c, resid, status);
	}

	if (IS_VME(c))
		sir->csr &= ~SI_CSR_DMA_EN;


	/* fall through to start_next_command_and_leave */

start_next_command_and_leave:

#ifdef SCSI_DEBUG
	if (scsi_debug)
		printf("siintr: start_next\n");
#endif SCSI_DEBUG

	/* start next I/O activity on controller */

	if ((un->un_mc->mc_tab.b_actf) && (un->un_mc->mc_tab.b_active == 0)) {
		/*
		 * We have things to do and the controller is not busy.
		 */

		register struct buf *dp = un->un_mc->mc_tab.b_actf;

		un = (struct scsi_unit *)dp->b_un.b_addr;
		/* 
		 * If this activity was preempted due to a 
		 * reselection coming in, much of the setup has 
		 * already taken place and must not be redone.
		 */
		if (un->un_flags & SC_UNF_PREEMPT) {

#ifdef SCSI_DEBUG
			if (scsi_debug)
				printf("siintr: preempted\n");
#endif SCSI_DEBUG

			un->un_flags &= ~SC_UNF_PREEMPT;
			c->c_cdb = un->un_saved_cmd.saved_cdb;
			c->c_un = un;
			un->un_mc->mc_tab.b_active = 1;
			if (un->un_dma_curdir != SI_NO_DATA) {
				un->un_mc->mc_mbinfo = un->un_baddr;
			}

			if (si_cmd(c, un, 1) == 0) {
				/*
				 * Command startup failed.
				 */

				(*un->un_ss->ss_intr)(c, 0, SE_FATAL);
				si_off(un);
			}
		} else {
#ifdef SCSI_DEBUG
			if (scsi_debug)
				printf("siintr: sistart, end\n");
#endif SCSI_DEBUG

			sistart(un);
		}
		goto set_up_for_next_intr_and_leave;	
	} else {
		/*
		 * No commands waiting to be started - reenable reconnects
		 * and get out.
		 */

#ifdef SCSI_DEBUG
		if (scsi_debug)
			printf("siintr: no cmds to start\n");
#endif SCSI_DEBUG

		SBC_WR.ser = ser_state = SI_HOST_ID;

		goto set_up_for_next_intr_and_leave;
	}

reset_and_leave:

#ifdef SCSI_DEBUG
	printf("siintr: resetting: state before reset (stbi %x): ", stbi);
	si_print_state(sir);
#endif SCSI_DEBUG
	si_reset(c);
	goto set_up_for_next_intr_and_leave;

handle_spurious_and_leave:
	printf("siintr: spurious interrupt: ");
	si_print_state(sir);
	/* FALL THROUGH */

set_up_for_next_intr_and_leave:

	if (cpu == CPU_SUN3_50 || cpu == CPU_SUN3_60) {
		int s = spl7();		/* make this as tight as possible */
#define S	SBC_RD
		if ( (!(S.bsr & SBC_BSR_PMTCH) && (S.cbsr & SBC_CBSR_REQ))
		   || ((S.cbsr & SBC_CBSR_SEL) && (S.cbsr & SBC_CBSR_IO) &&
		          (S.cdr & SI_HOST_ID)) ) {
#undef S
			/* 
			 * We've got a winner... 
			 * We either have a phase mismatch and a req
			 * or a reselection.
			 */
			(void) splx(s);
#ifdef SCSI_DEBUG
			siintr_winner++;
#endif SCSI_DEBUG
			goto head_siintr;
		} else {
			(void) splx(s);
#ifdef SCSI_DEBUG
			siintr_loser++;
#endif SCSI_DEBUG
		}
	}

	if (IS_VME(c))
		sir->csr |= SI_CSR_DMA_EN;
leave:
	return;
}

/*
 * Handle target disconnecting.
 * Returns true if all was OK, false otherwise.
 */
si_disconnect(c) 
	register struct scsi_ctlr *c;
{
	register struct scsi_unit *un = c->c_un;
	register struct mb_ctlr *mc = un->un_mc;
	register struct buf *dp;
	register struct scsi_si_reg *sir = c->c_sir;
	register u_short bcr;

#ifdef SCSI_DEBUG
	if (scsi_debug)
		printf("si_disconnect\n");
#endif SCSI_DEBUG

	bcr = sir->bcr;
	SCSI_RECON_TRACE('d', c, un->un_dma_curaddr, (int) un->un_dma_curbcr,
				(int) bcr);

	/* save dma info for reconnect */
	if (un->un_dma_curdir != SI_NO_DATA) {

		if (IS_VME(c)) {
			/* 
			 * bcr does not reflect how many bytes were actually
			 * transferred for VME.
			 */
			SCSI_RECON_TRACE('D', c, un->un_dma_curaddr, 
					(int)un->un_dma_curcnt, (int)bcr);
			if (un->un_flags & SC_UNF_DMA_INITIALIZED) {
				if ((un->un_dma_curdir == SI_SEND_DATA) &&
				    (bcr != un->un_dma_curcnt) &&
				    (bcr != 0)) {
				/*
				 * SCSI-3 VME interface is a little funny on 
				 * writes: if we have a disconnect, the dma 
				 * has overshot by one byte and needs to be 
				 * incremented.
				 * this is true if we have not transferred
				 * either all data or no data.
				 */
					if (un->un_dma_curbcr != 0) {
						bcr = un->un_dma_curbcr + 1;
						SCSI_RECON_TRACE('g', c, 
						un->un_dma_curaddr, 
						(int)un->un_dma_curcnt,
						(int) bcr);
					} else {
						bcr++;
						SCSI_RECON_TRACE('z', c, 
						un->un_dma_curaddr, 
						(int)un->un_dma_curcnt,
						(int) bcr);
					}
					SCSI_TRACE('g', sir, un);
				} else if (un->un_dma_curdir == SI_RECV_DATA) {
					/* 
					 * Use the bcr value we got before
					 * we pulled in the discon message.
					 */
					bcr = un->un_dma_curbcr;
					SCSI_RECON_TRACE('x', c, 
						un->un_dma_curaddr, 
						(int)un->un_dma_curcnt,
						(int)bcr);
				}
			} else {
				/* we haven't xferred any data yet */
				bcr = un->un_dma_count;
				SCSI_RECON_TRACE('t', c, 
						un->un_dma_curaddr, 
						(int)un->un_dma_curcnt,
						(int)bcr);
			}
		} else {
			bcr = sir->bcr;
			SCSI_RECON_TRACE('b', c, 
					un->un_dma_curaddr, 
					(int)un->un_dma_curcnt,
					(int)bcr);
		}

		if (un->un_dma_curdir == SI_RECV_DATA) {
			if (! si_dma_recv(c)) {
				printf("si_discon: si_dma_recv bogosity\n");
				return (0);
			}
		}

		/*
		 * Save dma information so dma can be restarted when
		 * a reconnect occurs.
		 */
		un->un_dma_curaddr += un->un_dma_curcnt - bcr;
		un->un_dma_curcnt = bcr;
		SCSI_RECON_TRACE('q', c, un->un_dma_curaddr, 
					(int)un->un_dma_curcnt, (int)bcr); 
#ifdef SCSI_DEBUG
		if (bcr == 1)
			printf("si_disconnect: bcr is 1\n");
		if (scsi_dis_debug) {
		    printf("si_discon: addr %x cnt %x bcr %x sr %x baddr %x\n",
			un->un_dma_curaddr, un->un_dma_curcnt, 
			bcr, sir->csr, un->un_baddr);
		}
#endif SCSI_DEBUG

	}

	/* 
	 * Remove this disconnected task from the ctlr ready queue and save 
	 * on disconnect queue until a reconnect is done.
	 * Advance controller queue. Remove mainbus resource alloc info.
	 */
	dp = mc->mc_tab.b_actf;
	mc->mc_tab.b_active = 0;
	mc->mc_tab.b_actf = dp->b_forw;
	mc->mc_mbinfo = 0;

	if (c->c_disqh == NULL) 
		c->c_disqh = dp;
	else
		c->c_disqt->b_forw = dp;
	dp->b_forw = NULL;
	c->c_disqt = dp;
	c->c_un = NULL;

	SI_VME_OK(c, sir, "si_disconnect");

	SBC_WR.mr &= ~SBC_MR_DMA;	/* off, as will get reselect next */

	/* reset udc */
	if (IS_ONBOARD(c)) {
		DELAY(si_udc_wait);
		sir->udc_raddr = UDC_ADR_COMMAND;
		DELAY(si_udc_wait);
		sir->udc_rdata = UDC_CMD_RESET;;
		DELAY(si_udc_wait);
	} else {
		sir->bcr = sir->bcrh = sir->dma_count = 0;
	}

	/* clear FIFO */
	sir->csr &= ~SI_CSR_FIFO_RES;
	sir->csr |= SI_CSR_FIFO_RES;

#ifdef SCSI_DEBUG
	if (scsi_debug) {
		printf("end of si_discon: ");
		si_print_state(sir);
	}
#endif SCSI_DEBUG

	un->un_saved_cmd.saved_cdb = c->c_cdb;
	un->un_saved_cmd.saved_scb = c->c_scb;

	return (1);
}


/*
 * Complete reselection phase and reconnect to target.
 *
 * Return true if sucessful, 0 if not.
 *
 * NOTE: TODO: think about whether we really want to do the resets w/in
 * 	this code, or let the return of 0 indicate to the caller that we
 *	need to reset.
 *
 * NOTE: this routine cannot use si_getdata() to get identify msg
 * from reconnecting target due to sun3/50 scsi interface. The bcr
 * must be setup before the target changes scsi bus to data phase
 * if the command being reconnected involves dma (which we do not
 * know until we get the identify msg). Thus we cannot acknowledge
 * the identify msg until some setup of the host adaptor registers 
 * is done.
 */
si_reconnect(c, target, lun)
	register struct scsi_ctlr *c;
	register u_int target;
	register u_int lun;
{
	register struct scsi_si_reg *sir = c->c_sir;
	register struct buf *dp;
	register struct buf *pdp;
	register struct scsi_unit *un;
	int s;

#ifdef SCSI_DEBUG
	if (scsi_debug)
		printf("si_reconnect: target %d lun %d\n", target, lun);
#endif SCSI_DEBUG

	/* search disconnect q for reconnecting task */
	for (dp = c->c_disqh, pdp = NULL; dp; pdp = dp, dp = dp->b_forw) {
		un = (struct scsi_unit *)dp->b_un.b_addr;
		if ((un->un_target == target) && (un->un_lun == lun))
			break;
	}

	if (dp == NULL) {
		printf("si: recon, never fnd dis unit: tgt %d lun %d\n",
			target, lun);
		/* dump out disconnnect queue */
		printf("discon queue:\n");
		for (dp = c->c_disqh,pdp = NULL; dp; pdp = dp,dp = dp->b_forw) {
			un = (struct scsi_unit *)dp->b_un.b_addr;
			printf("target %d lun %d\n",
					un->un_target, un->un_lun == lun);
		}

		SI_VME_OK(c, sir, "si_recon: dp == NULL");

#ifdef SCSI_DEBUG
		if (scsi_debug)
			si_print_state(sir);
		si_reset(c);
#endif SCSI_DEBUG

		return(0);
	}

	/* disable other reconnection attempts */

	SI_VME_OK(c, sir, "si_recon: ser = 0");
	SBC_WR.ser = ser_state = 0;

	/* remove entity from disconnect q */
	if (dp == c->c_disqh)
		c->c_disqh = dp->b_forw;
	else
		pdp->b_forw = dp->b_forw;
	if (dp == c->c_disqt)
		c->c_disqt = pdp;
	dp->b_forw = NULL;

	/* requeue at front of controller queue */
	if (un->un_mc->mc_tab.b_actf == NULL) {
		un->un_mc->mc_tab.b_actf = dp;
		un->un_mc->mc_tab.b_actl = dp;
	} else {
		dp->b_forw = un->un_mc->mc_tab.b_actf;
		un->un_mc->mc_tab.b_actf = dp;
	}
	un->un_mc->mc_tab.b_active = 1;
	c->c_un = un;

	c->c_cdb = un->un_saved_cmd.saved_cdb;
	c->c_scb = un->un_saved_cmd.saved_scb;

	SCSI_RECON_TRACE('r', c, 0, 0, 0); 
	/* restart disconnect activity */
	if (un->un_dma_curdir != SI_NO_DATA) {
		/* restore mainbus resource allocation info */
		un->un_mc->mc_mbinfo = un->un_baddr;

		/* reset udc */
		if (IS_ONBOARD(c)) {
			DELAY(si_udc_wait);
			sir->udc_raddr = UDC_ADR_COMMAND;
			DELAY(si_udc_wait);
			sir->udc_rdata = UDC_CMD_RESET;;
			DELAY(si_udc_wait);
		}

		/* do initial dma setup */
		sir->csr &= ~SI_CSR_FIFO_RES;
		sir->csr |= SI_CSR_FIFO_RES;

		if (un->un_dma_curdir == SI_RECV_DATA)
			sir->csr &= ~SI_CSR_SEND;
		else
			sir->csr |= SI_CSR_SEND;

		if (IS_ONBOARD(c))
			sir->bcr = un->un_dma_curcnt;
		else
			sir->bcr = sir->bcrh = sir->dma_count = 0;

		/*
		 * New: we set up everything we can here, rather
		 * than wait until data phase.
		 */
		
		if (IS_ONBOARD(c))
			si_ob_dma_setup(c, un);
		else {
			sir->csr &= ~SI_CSR_DMA_EN;
			si_vme_dma_setup(c, un);
		}

#ifdef SCSI_DEBUG
		if (scsi_dis_debug) {
		    printf("si_recon: addr %x cnt %x bcr %x sr %x baddr %x\n", 
			un->un_dma_curaddr, un->un_dma_curcnt, sir->bcr, 
			sir->csr, un->un_baddr);
		}
#endif SCSI_DEBUG
	}

	/* we can finally acknowledge identify message */
	SBC_WR.icr = SBC_ICR_ACK;

	if (si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ, 0) == 0) {
		printf("si_recon: REQ not INactive, cbsr 0x%x\n",
			SBC_RD.cbsr);
		si_reset(c);
		return(0);
	}

	s = spl7();
	SBC_WR.icr = 0;

	if (IS_VME(c)) {
		/* note: this is a little paranoid, and probably not required */

		SBC_WR.mr |= SBC_MR_DMA;	/* turn interrupts back on */
		sir->csr |= SI_CSR_DMA_EN;
	} else {
		SBC_WR.mr |= SBC_MR_DMA;	/* turn interrupts back on */
	}
	(void) splx(s);

	/* NOTE: possible race here - really need to see whether we got a
	 * phase change between the 2 preceeding statements.  We don't now
	 * because we are lazy.
	 */
	return(1);
}

/*
 * No current activity for the scsi bus. May need to flush some
 * disconnected tasks if a scsi bus reset occurred before the
 * target reconnected, since a scsi bus reset causes targets to 
 * "forget" about any disconnected activity.
 * Also, enable reconnect attempts.
 */
si_idle(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_si_reg *sir = c->c_sir;
	register struct buf *dp;
	register struct scsi_unit *un;
	register int resid;

#ifdef SCSI_DEBUG
	if (scsi_debug)
		printf("si_idle\n");
#endif SCSI_DEBUG

	if (c->c_flags & SCSI_FLUSHING) {
#ifdef SCSI_DEBUG
		if (scsi_reset_debug)
			printf("si_idle: flushing, flags %x\n", c->c_flags);
#endif SCSI_DEBUG
		return;
	}

	/* flush disconnect tasks if a reconnect will never occur */
	if (c->c_flags & SCSI_FLUSH_DISQ) {
#ifdef SCSI_DEBUG
		if (scsi_reset_debug) {
		    printf("si_idle: flush: flags %x disqh %x disqt %x\n", 
			c->c_flags, c->c_disqh, c->c_disqt);
		}
#endif SCSI_DEBUG

		/* now in process of flushing tasks */
		c->c_flags &= ~SCSI_FLUSH_DISQ;
		c->c_flags |= SCSI_FLUSHING;

		for (dp = c->c_disqh; dp && c->c_flush; dp = c->c_disqh) {
			/* keep track of last task to flush */
			if (c->c_flush == c->c_disqh) 
				c->c_flush = NULL;

			/* remove tasks from disconnect q */
			un = (struct scsi_unit *)dp->b_un.b_addr;
			c->c_disqh = dp->b_forw;
			dp->b_forw = NULL;

			/* requeue on controller q */
			siustart(un);
			un->un_mc->mc_tab.b_active = 1;
			c->c_un = un;

			/* inform device routines of error */
			if (un->un_dma_curdir != SI_NO_DATA) {
				un->un_mc->mc_mbinfo = un->un_baddr;
				resid = un->un_dma_curcnt;
			} else {
				resid = 0;
			}
			(*un->un_ss->ss_intr)(c, resid, SE_FATAL);
		}
		if (c->c_disqh == NULL) {
			c->c_disqt = NULL;
		}
		c->c_flags &= ~SCSI_FLUSHING;
	}

	/* enable reconnect attempts */
	if (IS_VME(c))
		sir->csr &= ~SI_CSR_DMA_EN;	/* turn off before SBC access */

	SBC_WR.ser = ser_state = SI_HOST_ID;
	if (IS_VME(c)) {
		sir->bcrh = sir->bcr = sir->dma_count = 0;
		sir->csr &= ~SI_CSR_SEND;
		sir->csr |= SI_CSR_DMA_EN;
	}
}

/*
 * Get status bytes from scsi bus.
 * Returns number of status bytes read.
 */
si_getstatus(un)
	struct scsi_unit *un;
{
	register struct scsi_ctlr *c = un->un_c;
	register struct scsi_si_reg *sir = c->c_sir;
	register int i;
	register u_char msg;

#ifdef SCSI_DEBUG
	if (scsi_debug)
		printf("si_getstatus\n");
#endif SCSI_DEBUG

	SI_VME_OK(c, sir, "si_getstatus: top");

	if (! si_wait_phase(sir, PHASE_STATUS)) {
		printf("si_getstatus: no STATUS phase\n");
		si_print_state(sir);
		return(0);
	}

	i = si_status_bytes(un);

#ifdef SCSI_DEBUG
	if (scsi_debug) {
		int x;
		register u_char *p = (u_char *)&c->c_scb;

		printf("si_getstatus: %d status bytes", i);
		for (x = 0; x < i; x++)
			printf(" %x", *p++);
		printf("\n");
	}
#endif SCSI_DEBUG

	if (! si_wait_phase(sir, PHASE_MSG_IN)) {
		printf("si_getstatus: no MSG_IN phase\n");
		si_print_state(sir);
		return(0);
	}
	
	SI_VME_OK(c, sir, "si_getstatus: msg_in");

	msg = si_getdata(c, PHASE_MSG_IN);

	if (msg != SC_COMMAND_COMPLETE) {
		printf("si_getstatus: bogus msg_in %x\n", msg);
		si_print_state(sir);
		return(0);
	}
	SBC_WR.tcr = TCR_UNSPECIFIED;

	return (i);
}

/*
 * Get status bytes from scsi bus.
 * Returns number of status bytes read.
 */
si_status_bytes(un)
	struct scsi_unit *un;
{
	register struct scsi_ctlr *c = un->un_c;
	register u_char *cp = (u_char *)&c->c_scb;
	register int i;
	register int b;

#ifdef SCSI_DEBUG
	if (scsi_debug)
		printf("si_status_bytes\n");
#endif SCSI_DEBUG

	/* get all the status bytes */
	for (i = 0;;) {
		b = si_getdata(c, PHASE_STATUS);

		if (b < 0)
			break;
		if (i < STATUS_LEN)
			cp[i++] = b;
	}

	return (i);
}

/* 
 * Wait for a scsi dma request to complete.
 * Disconnects were disabled in si_cmd() when polling for command completion.
 * Called by drivers in order to poll on command completion.
 */
si_cmdwait(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_si_reg *sir = c->c_sir;
	register struct scsi_unit *un = c->c_un;
	register u_char junk;
	int ret_val;

#ifdef SCSI_DEBUG
	if (scsi_debug)
		printf("si_cmdwait\n");
#endif SCSI_DEBUG

	/* wait for dma transfer to complete */
	if (si_wait((u_short *)&sir->csr, SI_CSR_DMA_ACTIVE, 0) == 0) {

#ifdef SCSI_DEBUG
		if (scsi_debug) {
			printf("si_cmdwait: DMA_ACTIVE still on\n");
			si_print_state(sir);
		}
#endif SCSI_DEBUG
		si_reset(c);
		return (0);
	}

	/* if command does not involve dma activity, then we are finished */
	if (un->un_dma_curdir == SI_NO_DATA) {
		return (1);
	}

	if (! si_wait((u_short *)&sir->csr,
	      (SI_CSR_SBC_IP|SI_CSR_DMA_IP|SI_CSR_DMA_CONFLICT), 1)) {
#ifdef SCSI_DEBUG
		if (scsi_debug)
			printf("si_cmdwait: dma never cmplt: csr %x cbsr %x\n",
				sir->csr, SBC_RD.cbsr);
#endif SCSI_DEBUG
		si_reset(c);
		ret_val = 0;
		goto done;
	}
			
	/* wait for indication of dma completion */
	if (si_wait((u_short *)&sir->csr, 
	    SI_CSR_SBC_IP|SI_CSR_DMA_IP|SI_CSR_DMA_CONFLICT, 1) == 0) {
#ifdef SCSI_DEBUG
		if (scsi_debug) {
			printf("si_cmdwait: dma never cmplt csr %x cbsr %x\n",
				sir->csr, SBC_RD.cbsr);
			si_print_state(sir);
		}
#endif SCSI_DEBUG
		si_reset(c);

		ret_val = 0;
		goto done;
	}

	/* 
	 * For vme host adaptor interface, must disable dma before
	 * accessing any registers other than the csr or a dma
	 * conflict error will occur.
	 */
	if (IS_VME(c))
		sir->csr &= ~SI_CSR_DMA_EN;

	/* make sure dma completely complete */
	if ((sir->csr & SI_CSR_SBC_IP) == 0) {
#ifdef SCSI_DEBUG
		if (scsi_debug) {
			if (sir->csr & SI_CSR_DMA_BUS_ERR) {
				printf("si_cmdwait: bus error during dma\n");
			} else if (sir->csr & SI_CSR_DMA_CONFLICT) {
				printf("si_cmdwait: reg acc during dma\n");
			} else if (IS_ONBOARD(c))
				printf("si_cmdwait: dma ip, unknown reason\n");
			else {
				printf("si_cmdwait: dma overrun\n");
			}

		si_print_state(sir);
		}
#endif SCSI_DEBUG
		si_reset(c);

		ret_val = 0;
		goto done;
	}

	/* handle special dma recv situations */
	if (un->un_dma_curdir == SI_RECV_DATA) {
#ifdef SCSI_DEBUG
		if (scsi_debug)
			printf("si_cmdwait: spec dma\n");

#endif SCSI_DEBUG
		if (si_dma_recv(c) == 0) {
			si_reset(c);
			ret_val = 0;
			goto done;
		}
	}

	/* ack sbc interrupt and cleanup */
	junk = SBC_RD.clr;
#ifdef lint
	junk = junk;
#endif lint
	si_dma_cleanup(c);
	ret_val = 1;

done:
	if (sir->csr & SI_CSR_SBC_IP) {
		/* clear SBC interrupt if still pending */
		junk = SBC_RD.clr;
#ifdef lint
		junk = junk;
#endif lint
	}

	if (IS_VME(c))
		sir->csr &= ~SI_CSR_DMA_EN;	/* turn it off to be sure */
	return (ret_val);
}

/*
 * Wait for a condition to be (de)asserted on the scsi bus.
 */
si_sbc_wait(reg, cond, set)
	register caddr_t reg;
	register u_char cond;
	register int set;
{
	register int i;
	register u_char regval;

	for (i = 0; i < SI_WAIT_COUNT; i++) {
		regval = *reg;
		if ((set == 1) && (regval & cond)) {
			return (1);
		}
		if ((set == 0) && !(regval & cond)) {
			return (1);
		} 
		DELAY(10);
	}
	return (0);
}

/*
 * Wait for a condition to be (de)asserted.
 */
si_wait(reg, cond, set)
	register u_short *reg;
	register u_short cond;
	register int set;
{
	register int i;
	register u_short regval;

	for (i = 0; i < SI_WAIT_COUNT; i++) {
		regval = *reg;
		if ((set == 1) && (regval & cond)) {
			return (1);
		}
		if ((set == 0) && !(regval & cond)) {
			return (1);
		} 
		DELAY(10);
	}
	return (0);
}

/*
 * Wait for a phase on the SCSI bus.
 */
si_wait_phase(sir, phase)
	register struct scsi_si_reg *sir;
	register u_char phase;
{
	register int i;

#ifdef SCSI_DEBUG
	if (scsi_debug)
		printf("si_wait_phase: phase %x (%s)\n", 
					phase, si_str_phase(phase));
#endif SCSI_DEBUG

	for (i = 0; i < SI_WAIT_COUNT; i++) {
		if ((SBC_RD.cbsr & CBSR_PHASE_BITS) == phase)
			return (1);
		DELAY(10);
	}
	return (0);
}

/*
 * Put data onto the scsi bus.
 * Returns 1 if successful, 0 otherwise.
 */
si_putdata(c, phase, data, num)
	register struct scsi_ctlr *c;
	register u_short phase;
	register u_char *data;
	register int num;
{
	register struct scsi_si_reg *sir = c->c_sir;
	register int i;

#ifdef SCSI_DEBUG
	if (scsi_debug)
		printf("si_putdata, phs %x (%s) num %d\n", 
					phase, si_str_phase(phase), num);
#endif SCSI_DEBUG

	SI_VME_OK(c, sir, "si_putdata");

	/* 
	 * Set up tcr so we can transmit data.
	 */
	switch (phase) {
	case PHASE_COMMAND:
		SBC_WR.tcr = TCR_COMMAND;
		break;

	case PHASE_MSG_OUT:
		SBC_WR.tcr = TCR_MSG_OUT;
		break;

	default:
		printf("si_putdata %d phase not supported\n", phase);
		return (0);
	}
 	
	/* put all desired bytes onto scsi bus */
	for (i = 0; i < num; i++ ) {

		/* wait for target to request a byte */
		if (si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ, 1) == 0) {
			printf("si: putdata, REQ not active\n");
			/* si_reset(c); */	/* what to do here? */
			return (0);
		}

#ifdef SCSI_DEBUG
		if (scsi_debug)
			printf("putting byte # %d, = %x\n", i, *data);
#endif SCSI_DEBUG

		/* load data */
		SBC_WR.odr = *data++;
		SBC_WR.icr = SBC_ICR_DATA;

		/* complete req/ack handshake */
		SBC_WR.icr |= SBC_ICR_ACK;
		if (si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ, 0) == 0) {
			printf("si: putdata, req not INactive\n");
			/* si_reset(c); */	/* what to do here? */
			return (0);
		}
		SBC_WR.icr = 0;
	}
	SBC_WR.tcr = TCR_UNSPECIFIED;

	return (1);
}

/*
 * Put command onto the scsi bus.
 * Returns 1 if successful, 0 otherwise.
 */
si_putcmd(c, data, num, want_intr)
	register struct scsi_ctlr *c;
	register u_char *data;
	register int num;
	int want_intr;
{
	register struct scsi_si_reg *sir = c->c_sir;
	register int i;

#ifdef SCSI_DEBUG
	if (scsi_debug)
		printf("si_putcmd\n"); 
#endif SCSI_DEBUG

	SI_VME_OK(c, sir, "si_putcmd");

	SBC_WR.tcr = TCR_COMMAND;

	/* put all desired bytes onto scsi bus */
	for (i = 0; i < num; i++ ) {

		/* wait for target to request a byte */
		if (si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ, 1) == 0) {
			printf("si: putdata, REQ not active\n");
			/* si_reset(c); */	/* what to do here? */
			return (0);
		}

#ifdef SCSI_DEBUG
		if (scsi_debug)
			printf("putting byte # %d, = %x\n", i, *data);
#endif SCSI_DEBUG

		/* load data */
		SBC_WR.odr = *data++;
		SBC_WR.icr = SBC_ICR_DATA;

		/* complete req/ack handshake */
		SBC_WR.icr |= SBC_ICR_ACK;
		if (si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ, 0) == 0) {
			printf("si: putdata, req not INactive\n");
			/* si_reset(c); */	/* what to do here? */
			return (0);
		}
		if (i < num - 1)	/* don't handshake last byte */
			SBC_WR.icr = 0;
	}

	if (want_intr) {
		int s = spl7();		/* lock out interrupts during race */

		SBC_WR.ser = ser_state =  SI_HOST_ID;
		SBC_WR.tcr = TCR_UNSPECIFIED;
		SBC_WR.icr = 0;		/* complete last byte handshake */
		SBC_WR.mr |= SBC_MR_DMA;

		if (IS_VME(c)) {
			sir->csr |= SI_CSR_DMA_EN;
		}

		(void) splx(s);
	} else {
		SBC_WR.icr = 0;
	}
		
	return (1);
}

/*
 * Get data from the scsi bus.
 * Returns a single byte of data, -1 if unsuccessful.
 */
si_getdata(c, phase)
	register struct scsi_ctlr *c;
	register u_short phase;
{
	register struct scsi_si_reg *sir = c->c_sir;
	register u_char data;
	int s;					/* spl level */

#ifdef SCSI_DEBUG
	if (scsi_debug)
		printf("si_getdata: phs %x (%s)\n", phase, si_str_phase(phase));
#endif SCSI_DEBUG

	SI_VME_OK(c, sir, "si_getdata");

	/* wait for target request */
	if (si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ, 1) == 0) {
		printf("si: getdata, REQ not active, cbsr 0x%x\n",
			SBC_RD.cbsr);
#ifdef SCSI_DEBUG
		if (scsi_debug)
			si_print_state(sir);
#endif SCSI_DEBUG

		return (-1);
	}

	if ((SBC_RD.cbsr & CBSR_PHASE_BITS) != phase) {
		/* not the phase we expected */
#ifdef SCSI_DEBUG
		if (scsi_debug)
			si_print_state(sir);
#endif SCSI_DEBUG
		return (-1);
	}


	/* grab data and complete req/ack handshake */
	data = SBC_RD.cdr;

	SBC_WR.icr = SBC_ICR_ACK;

	if (si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ, 0) == 0) {
		printf("si: getdata, REQ not inactive, cbsr 0x%x\n",
			SBC_RD.cbsr);
#ifdef SCSI_DEBUG
		if (scsi_debug)
			si_print_state(sir);
#endif SCSI_DEBUG

		si_reset(c);
		return (-1);
	}

#ifdef SCSI_DEBUG
	if (scsi_debug) {
		printf("si_getdata: before icr gets 0:\n");
		si_print_state(sir);
	}
#endif SCSI_DEBUG

	if ((phase == PHASE_MSG_IN) && 
		((data == SC_COMMAND_COMPLETE) || (data == SC_DISCONNECT)) ) {
		s = spl7();
		SBC_WR.icr = 0;
		SBC_WR.mr &= ~SBC_MR_DMA;
		(void) splx(s);
	} else {
		SBC_WR.icr = 0;
	}

#ifdef SCSI_DEBUG
	if (scsi_debug)
		printf("\tsi_getdata: data %x\n", data);
#endif SCSI_DEBUG

	return (data);
}

/*
 * Reset SCSI control logic and bus.
 */
si_reset(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_si_reg *sir = c->c_sir;
	register u_char junk;

#ifdef SCSI_DEBUG
	if (scsi_debug) {
		printf("scsi reset, sr= %x, bcr= %x\n", sir->csr, sir->bcr);
	}
#endif SCSI_DEBUG

	/* reset scsi control logic */
	sir->bcr = 0;
	sir->csr = 0;
	DELAY(10);
	sir->csr = SI_CSR_SCSI_RES|SI_CSR_FIFO_RES;
	if (IS_VME(c)) {
		sir->dma_addr = 0;
		sir->dma_count = 0;
	}

	/* issue scsi bus reset (make sure interrupts from sbc are disabled) */
	SBC_WR.icr = SBC_ICR_RST;
	DELAY(1000);
	SBC_WR.icr = 0;
	junk = SBC_RD.clr;
#ifdef lint
	junk = junk;
#endif lint

	/* enable sbc interrupts, but don't enable recon attempts */
	sir->csr |= SI_CSR_INTR_EN;
	SBC_WR.mr = 0;
	if (IS_VME(c))
		sir->csr |= SI_CSR_DMA_EN;

	/* disconnect queue needs to be flushed */
	if (c->c_disqh != NULL) {
		c->c_flags |= SCSI_FLUSH_DISQ;
		c->c_flush = c->c_disqt;
	}
}

/*
 * Return residual count for a dma.
 */
si_dmacnt(c)
	register struct scsi_ctlr *c;
{
	if (IS_ONBOARD(c)) {
		return (c->c_sir->bcr);
	} else {
		return ( ((c->c_sir->bcrh) << 16) | (c->c_sir->bcr) );
	}
}

/*
 * Return the proper length of the cdb (dependent on command)
 */
si_cdb_len(c)
	register struct scsi_cdb *c;
{
#ifdef lint
	c = c;
#endif lint
	return(6);		/* cheat for now... */
}

#ifdef SCSI_DEBUG
/*
 * Print out the cdb.
 */
si_print_cdb(c)
	struct scsi_ctlr *c;
{
	register u_char *cp;
	register int i;

	cp = (u_char *) &c->c_cdb;
	for (i = 0; i < sizeof (struct scsi_cdb); i++)
		printf("%x ", *cp++);

	printf("\n");
}
#endif SCSI_DEBUG

/*
 * returns string corresponding to the phase
 */
char *
si_str_phase(phase)
	register u_char phase;
{
	register int index = (phase & CBSR_PHASE_BITS) >> 2;
	static char *bogus_phase = "bogus phase";
	static char *phase_strings[] = {
		"DATA_OUT/FREE",
		"DATA_IN",
		"COMMAND",
		"STATUS",
		"UNSPEC",
		"?",
		"MSG_OUT",
		"MSG_IN"
	};

	if (index <= 7 && index >= 0)
		return(phase_strings[index]);
	else
		return(bogus_phase);
}

/*
 * print out the current hardware state
 */
si_print_state(sir)
	register struct scsi_si_reg *sir;
{
	printf("csr %x cbsr %x (%s) cdr %x mr %x bsr %x tcr %x ser %x\n",
			sir->csr, SBC_RD.cbsr, si_str_phase(SBC_RD.cbsr),
			SBC_RD.cdr, SBC_RD.mr, SBC_RD.bsr, SBC_RD.tcr, 
			ser_state);
}
#endif NSI > 0
