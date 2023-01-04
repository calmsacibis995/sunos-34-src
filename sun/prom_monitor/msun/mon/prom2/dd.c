/*
 * @(#)dd.c 1.6 83/09/19 Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Data Systems Design ST-506 disk controller bootstrap code.
 */

#include "../h/sasun.h"
#include "../h/bootparam.h"
#include "../h/msg.h"
#include <sys/types.h>
#include <sun/dklabel.h>
#include <sun/dkio.h>
#include <sundev/dsdreg.h>

#define NSTD 	2
int ddstd[NSTD] = { 0xf0, 0xf2 };

struct ddparam {
	int	dd_unit;
	int	dd_nsect;
	int	dd_ncyl;
	int	dd_acyl;
	int	dd_nhead;
	int	dd_ctlr;
};

/*
 * Put all the MBMEM crud together since we don't have a real allocator 
 */
struct dsdcrud {
	struct dsdwub	dw;
	struct dsdccb	dc;
	struct dsdiopb	dp;
	struct dsdinit	dx;
};

#define	DMADDR		0x1000
#define DDBUF		((char *) (DEF_MBMEM_VA + DMADDR))
#define label		((struct dk_label *) DDBUF)

#define	CYL(p)	(p * ddp->dd_nsect * ddp->dd_nhead)	/* block # at cylinder location */

ddprobe()
{
	register int i;
	register struct dsddevice *ddaddr;

	for (i=0; i<NSTD; i++) {
		ddaddr = (struct dsddevice *)(DEF_MBIO_VA + ddstd[i]);
		if (!pokec((char *)&ddaddr->dsd_r0, DSD_RESET)) 
			return (i);
	}
	return (-1);
}

pswlong(p, n)
	register swlong_t *p;
{
	p->swl_lo = n & 0xFFFF;
	p->swl_hi = n >> 16;
}


/* address in multibus address space */
#define swadd(p, n)	(pswlong(p, (((int)(n)) - DEF_MBMEM_VA) & 0xFFFFF)) 


/* This routine contains the test condition for  isspinning() */
int
ddspinning(ddp, d)
	char *ddp;
	int d;
{
	return 0x38 != ddcmd(DSD_INIT, (struct ddparam *)ddp, 0,
	    (char *) &((struct dsdcrud *) d)->dx,
	    sizeof ((struct dsdcrud *) d)->dx);
}


ddboot(bp)
	register struct bootparam *bp;
{
	register struct dsddevice *dsdio;
	register struct dsdcrud *d;
	register int i;
	register char *caddr;
	struct ddparam ddparam;
#define ddp (&ddparam)
	int ctlr;

	ddp->dd_unit = bp->bp_unit & 0x03;
	if ((ctlr = bp->bp_ctlr) < NSTD)
		ctlr = ddstd[ctlr];
	dsdio = (struct dsddevice *)(DEF_MBIO_VA + ctlr);
	if (pokec((char *)&dsdio->dsd_r0, DSD_RESET)) {
		printf(msg_noctlr, ctlr);
		return (-1);
	}
	ddp->dd_ctlr = ctlr;
	d = (struct dsdcrud *)(DEF_MBMEM_VA + (ctlr << 4));

	ddp->dd_nsect = 2;
	ddp->dd_nhead = 2;
	ddp->dd_ncyl = 2;

	dsdio->dsd_r0 = DSD_RESET;
	DELAY(1000);			/* let reset take effect */
	bzero((char *)d, sizeof *d);
	d->dw.dsw_extdiag = 1;		/* extended diagnostics */
	d->dw.dsw_linaddr = 1;		/* linear addressing */
	d->dw.dsw_emul    = 1;		/* isbx215 emulation, must be 1 */
	swadd(&d->dw.dsw_ccbp, &d->dc);
	d->dc.dsd_busy = DSD_BUSY;
	d->dc.dsd_01h = 0x01;
	d->dc.dsd_01h1 = 0x01;
	d->dc.dsd_04h = 0x04;
	swadd(&d->dc.dsd_cibp, &d->dc.dsd_handle);
	swadd(&d->dc.dsd_iopbp, &d->dp);
	dsdio->dsd_r0 = DSD_CLEAR;	/* clear reset condition */
DELAY(1000);
	dsdio->dsd_r0 = DSD_START;
DELAY(1000);
	while (d->dc.dsd_busy)
		DELAY(1000);
	dsdio->dsd_r0 = DSD_CLEAR;
DELAY(1000);
	/* now initialize drive */
	d->dx.dsx_ncyl = 2;
	d->dx.dsx_acyl = 0;
	d->dx.dsx_rheads = 0;	/* XXX */
	d->dx.dsx_fheads = 2;	/* XXX */
	d->dx.dsx_nsect = 2;
	d->dx.dsx_bpslo = DEV_BSIZE & 0xFF;
	d->dx.dsx_bpshi = DEV_BSIZE >> 8;

	switch (isspinning(ddspinning, (char *)ddp, (int)d)) {
	case 0:			/* Not spinning */
		return -1;
	case 1:			/* Is spinning */
		break;
	default:		/* Spun up while we waited */
		DELAY(5000000);	/* Ctlr sez ready before really ready */
		break;
	}

	label->dkl_magic = 0;
	if (ddcmd(DSD_READ, ddp, 0, DDBUF, DEV_BSIZE) == 0 &&
	    chklabel(label) == 0) {
		ddp->dd_nhead = label->dkl_nhead;
		ddp->dd_nsect = label->dkl_nsect;
		ddp->dd_ncyl  = label->dkl_ncyl;
		ddp->dd_acyl  = label->dkl_acyl;
		d->dx.dsx_ncyl = ddp->dd_ncyl+ddp->dd_acyl;
		d->dx.dsx_acyl = ddp->dd_acyl;;
		d->dx.dsx_rheads = 0;	/* XXX */
		d->dx.dsx_fheads = ddp->dd_nhead;	/* XXX */
		d->dx.dsx_nsect = ddp->dd_nsect;
		d->dx.dsx_bpslo = DEV_BSIZE & 0xFF;
		d->dx.dsx_bpshi = DEV_BSIZE >> 8;
		ddcmd(DSD_INIT, ddp, 0, (char *)&d->dx, sizeof d->dx);
	} else 
		printf(msg_nolabel);

	caddr = (char *)LOADADDR;
	for (i = 1; i < BBSIZE/DEV_BSIZE; i++) {
		if (ddcmd(DSD_READ, ddp, i, caddr, DEV_BSIZE))
			return (-1);
		caddr += DEV_BSIZE;
	}
	return (LOADADDR);
#undef ddp
}

ddcmd(cmd, ddp, bno, buf, cnt)
	register int cmd;
	register struct ddparam *ddp;
	register int bno;
	char *buf;
	int cnt;
{
	register struct dsddevice *dsdio =
	    (struct dsddevice *)(DEF_MBIO_VA + ddp->dd_ctlr);
	register struct dsdcrud *d =
	    (struct dsdcrud *)(DEF_MBMEM_VA + (ddp->dd_ctlr << 4));
	register struct dsdiopb *p = &d->dp;
	register char *bp;
	int cylno, sect, error, errcnt = 0;
	int head;

	if (cmd == DSD_READ)
		bp = DDBUF;
	else
		bp = buf;

	cylno = bno / CYL(1);
	sect = bno % ddp->dd_nsect;
	head = (bno / ddp->dd_nsect) % ddp->dd_nhead;

retry:
	bzero((char *)p, sizeof *p);
	p->dsi_device = DSD_WINCH;
	p->dsi_cmd = cmd;
	p->dsi_unit = ddp->dd_unit;
	p->dsi_nointr = 1;
	p->dsi_deldata = 0;
	p->dsi_cylinder = cylno;
	p->dsi_sector = sect;
	p->dsi_head = head;
	swadd(&p->dsi_bufp, bp);
	pswlong(&p->dsi_count, cnt);

	d->dc.dsd_busy = DSD_BUSY;
	d->dc.dsd_stsema = 0;
	dsdio->dsd_r0 = DSD_CLEAR;
DELAY(1000);
	dsdio->dsd_r0 = DSD_START;
DELAY(1000);

	error = 0;
	while (d->dc.dsd_busy == DSD_BUSY) {
		DELAY(1000);
		if (d->dc.dsd_stsema) {
			error |= d->dc.dsd_harderr;
			d->dc.dsd_stsema = 0;
		}
		if (error)
			break;
	}
	dsdio->dsd_r0 = DSD_CLEAR;
DELAY(1000);
	if (cmd == DSD_STATUS)
		return (0);

	if (error) {
		ddcmd(DSD_STATUS, ddp, 0, (char *)d->dc.dsd_error, 13);
		if (cmd == DSD_INIT && d->dc.dsd_exterr == 0x38)
			return (0x38);
		printf("dd: err %x cmd %x\n", d->dc.dsd_exterr, cmd);
		if (++errcnt < 16)
			goto retry;
		return (-1);
	}

	if (cmd == DSD_READ && buf != bp)	/* Many just use common buf */
		bcopy(bp, buf, DEV_BSIZE);
	return (0);
}
