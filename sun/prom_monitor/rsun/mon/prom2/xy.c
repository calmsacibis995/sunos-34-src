/*
 * @(#)xy.c 1.10 84/09/27 Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Xylogics 440/450 disk controller bootstrap code.
 */

#include "../h/sasun.h"
#include "../h/bootparam.h"
#include <sys/types.h>
#include <sun/dklabel.h>
#include <sun/dkio.h>
#include <sundev/xyreg.h>
#include <sundev/xycreg.h>
#include "../h/msg.h"

#define NSTD 	2
int xystd[NSTD] = { 0xee40, 0xee48 };

#define	MAXHEAD	4

struct xyparam {
	unsigned char	xy_unit;
	unsigned short	xy_nsect;
	unsigned short	xy_ncyl;
	unsigned short	xy_nhead;
	unsigned short	xy_bhead;
	unsigned short	xy_drive;
	struct xydevice *xy_addr;
};

#define	IOPBADDR	0x100
#define IOPB		(DEF_MBMEM_VA + IOPBADDR)
#define	DMADDR		0x200
#define XYBUF		((char *) (DEF_MBMEM_VA + DMADDR))
#define label		((struct dk_label *) XYBUF)


xyprobe()
{
	register int i;
	register u_char *xyaddr;

	for (i=0; i<NSTD; i++) {
		xyaddr = (u_char *)(DEF_MBIO_VA + xystd[i]);
		if (pokec(xyaddr, 0x67) || pokec(xyaddr+1, 0x89))
			continue;
		if (xyaddr[0] == 0x67 && xyaddr[1] == 0x89)  /*!2180*/
			return (i);
	}
	return (-1);
}

xyboot(bp)
	register struct bootparam *bp;
{
	register struct xydevice *xyaddr;
	register struct xyiopb *xy = (struct xyiopb *)IOPB;
	register int i, t;
	register char *caddr;
	u_short ppart;
	struct xyparam xyparam;
	register struct xyparam *xyp = &xyparam;
	int ctlr;
	int xyspinning();

	xyp->xy_unit = bp->bp_unit & 0x03;
	ppart = (bp->bp_unit >> 2) & 1;
	if ((ctlr = bp->bp_ctlr) < NSTD)
		ctlr = xystd[ctlr];
	xyp->xy_addr = xyaddr = (struct xydevice *)(DEF_MBIO_VA + ctlr);

	/* Make sure that, even if it exists, it's not a 2180 */
	if (pokec(&xyaddr->xy_iopbrel[0], 0x67)	||
	    pokec(&xyaddr->xy_iopbrel[1], 0x89)	||
	    xyaddr->xy_iopbrel[0] != 0x67	||
	    xyaddr->xy_iopbrel[1] != 0x89) {
		printf(msg_noctlr, ctlr);
		return (-1);
	}

	xyp->xy_nhead = MAXHEAD;
	xyp->xy_nsect = 32;
	xyp->xy_ncyl  = 2;

	i = xyaddr->xy_resupd;
	xycmd(XY_RESET, xyp, 0, (char *)0);
	/*
	 * Wait for disk to spin up, if necessary.
	 */
	if (!isspinning(xyspinning, (char *)xyaddr, (int)xyp))
		return (-1);

	for (xyp->xy_bhead = 0; xyp->xy_bhead < MAXHEAD; xyp->xy_bhead++) {
	for (xyp->xy_drive = 0; xyp->xy_drive < NXYDRIVE; xyp->xy_drive++) {
		label->dkl_magic = 0;
		if (xycmd(XY_READ, xyp, xyp->xy_bhead*xyp->xy_nsect, XYBUF))
			continue;
		if (chklabel(label))
			continue;
		if (ppart != label->dkl_ppart)
			continue;
		if (xyp->xy_bhead != label->dkl_bhead)
			continue;
		xyp->xy_nhead = label->dkl_nhead;
		xyp->xy_nsect = label->dkl_nsect;
		xyp->xy_ncyl = label->dkl_ncyl + label->dkl_acyl;
		if (xy->xy_ctype == XYC_450) {
			bzero((char *)IOPB, sizeof (struct xyiopb));
			xy->xy_reloc = 1;
			xy->xy_autoup = 1;
			xy->xy_cmd = XY_INIT;
			xy->xy_unit = xyp->xy_unit & 3;
			xy->xy_cylinder = xyp->xy_ncyl - 1;
			xy->xy_head = xyp->xy_nhead - 1;
			xy->xy_sector = xyp->xy_nsect - 1;
			xy->xy_bhead = xyp->xy_bhead;
			t = XYREL(xyaddr, IOPBADDR);
			xyaddr->xy_iopbrel[0] = t >> 8;
			xyaddr->xy_iopbrel[1] = t;
			t = XYOFF(IOPBADDR);
			xyaddr->xy_iopboff[0] = t >> 8;
			xyaddr->xy_iopboff[1] = t;
			xyaddr->xy_csr = XY_GO;
			do {
				DELAY(30);
			} while (xyaddr->xy_csr & XY_BUSY);
		}
		goto foundlabel;
	}
	}
	printf(msg_nolabel);

foundlabel:
	caddr = (char *)LOADADDR;
	for (i = 1; i < BBSIZE/DEV_BSIZE; i++) {
		if (xycmd(XY_READ, xyp, i, caddr))
			return (-1);
		caddr += DEV_BSIZE;
	}
	return (LOADADDR);
}

/* This routine is called from  isspinning()  as the test condition. */
int
xyspinning(xyaddr, xyp)
	struct xydevice *xyaddr;
	register struct xyparam *xyp;
{
	register struct xyiopb *xy = (struct xyiopb *)IOPB;

	xycmd(XY_STATUS, xyp, 0, (char *)0);
	return ((xy->xy_status & XY_READY) ? 0 : 1);
}

xycmd(cmd, xyp, bno, buf)
	register int cmd;
	register struct xyparam *xyp;
	register int bno;
	char *buf;
{
	register struct xyiopb *xy = (struct xyiopb *)IOPB;
	register struct xydevice *xyaddr = xyp->xy_addr;
	register char *bp;
	register unsigned short i;
	int t, error, errcnt = 0;
	unsigned short cylno, sect, head;

	bp = XYBUF;

	/*
	 * Calculate cylinder, head, and sector.
	 * Note that <i>, as well as the above, are unsigned short.
	 * Can't overflow -- we have spare factor of 4 on Eagle -- won't
	 * be eaten for awhile.
	 */
	i = bno / xyp->xy_nsect;
	sect = bno - i * xyp->xy_nsect;		/* Cheap % */
	cylno = i / xyp->xy_nhead;
	head = i - cylno * xyp->xy_nhead;	/* Cheap % */
	

retry:
	bzero((char *)IOPB, sizeof (struct xyiopb));
	xy->xy_reloc = 1;
	xy->xy_autoup = 1;
	xy->xy_eccmode = 2;
	xy->xy_cmd = cmd;
	xy->xy_drive = xyp->xy_drive;
	xy->xy_unit = xyp->xy_unit & 3;
	xy->xy_throttle = XY_THROTTLE;
	xy->xy_sector = sect;
	xy->xy_head = head;
	xy->xy_cylinder = cylno;
	xy->xy_nsect = 1;
	xy->xy_bufoff = XYOFF(DMADDR);
	xy->xy_bufrel = XYREL(xyaddr, DMADDR);

	t = XYREL(xyaddr, IOPBADDR);
	xyaddr->xy_iopbrel[0] = t >> 8;
	xyaddr->xy_iopbrel[1] = t;
	t = XYOFF(IOPBADDR);
	xyaddr->xy_iopboff[0] = t >> 8;
	xyaddr->xy_iopboff[1] = t;
	xyaddr->xy_csr = XY_GO;

	do {
		DELAY(30);
	} while (xyaddr->xy_csr & XY_BUSY);

	if (cmd == XY_STATUS)
		if ((xyaddr->xy_csr & XY_DREADY) == 0)
			xy->xy_status |= XY_READY;
	i = xyaddr->xy_resupd;
	if (xy->xy_iserr) {
		error = xy->xy_errno;
		/* Attempt to reset the error condition */
		if (cmd != XY_RESET)
			xycmd(XY_RESET, xyp, 0, (char *)0);
		if (++errcnt < 10)
			goto retry;
		if (bno != 0 || error != 5)	/* drive type probe */
			printf("xy: error %x cmd %x\n", error, cmd);
		return (-1);			/* Error */
	}

	if (cmd == XY_READ && buf != bp)	/* Many just use common buf */
		for (i = 0; i < DEV_BSIZE; i++)
			*buf++ = *bp++;

	return (0);
}
