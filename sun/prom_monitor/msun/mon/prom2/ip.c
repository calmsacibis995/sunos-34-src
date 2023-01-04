/*
 * @(#)ip.c 1.7 84/02/07 Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Interphase 2180 disk controller bootstrap code.
 */

#include <sys/types.h>
#include <sun/dklabel.h>
#include <sun/dkio.h>
#include <sundev/ipreg.h>
#include "../h/sasun.h"
#include "../h/bootparam.h"
#include "../h/msg.h"

#define	NSTD	4
int ipstd[NSTD] = { 0x40, 0x44, 0x48, 0x4c };

#define MAXHEAD		4	/* Max # heads to search for a label */

struct ipparam {
	int	ip_unit;
	unsigned short	ip_nsect;
	unsigned short	ip_ncyl;
	unsigned short	ip_nhead;
	unsigned short	ip_bhead;	/* Base head #, for CDC Lark support */
	struct ipdevice *ip_addr;
};

#define	IOPBADDR	0x100
#define IOPB		(DEF_MBMEM_VA + IOPBADDR)
#define	DMADDR		0x200
#define IPBUF		((char *) (DEF_MBMEM_VA + DMADDR))
#define label		((struct dk_label *) IPBUF)

ipprobe()
{
	register int i;

	for (i=0; i<NSTD; i++) {
		if (peek((short *)(DEF_MBIO_VA + ipstd[i])) != -1) {
			return (i);
		}
	}
	return (-1);
}

/*
 * This routine is passed to isspinning() as the spinup test.
 */
int
ipspinning(ipaddr, mask)
	struct ipdevice *ipaddr;
	int mask;
{
	return (ipaddr->ip_r0 & mask) != 0;
}


/*
 * This does the "real" work.
 */
ipboot(bp)
	register struct bootparam *bp;
{
	register struct ipdevice *ipaddr;
	register int i;
	register char *caddr;
	u_short ppart;
	struct ipparam ipparam;
#	define ipp   (&ipparam)
	int ctlr;

	bzero((char *)ipp, sizeof(*ipp));
	ipp->ip_unit = bp->bp_unit & 0x03;
	ppart = (bp->bp_unit >> 2) & 1;
	if ((ctlr = bp->bp_ctlr) < NSTD)
		ctlr = ipstd[ctlr];
	ipp->ip_addr = ipaddr = (struct ipdevice *)(DEF_MBIO_VA + ctlr);
	if (peek((short *)ipaddr) == -1) {
		printf(msg_noctlr, ctlr);
		return (-1);
	}

	ipp->ip_nsect = 2;		/* Read label */
	ipp->ip_ncyl  = 2;
	ipp->ip_nhead = MAXHEAD;
	ipp->ip_bhead = 0;		/* No base head yet, til label read */

	ipaddr->ip_r0 = 0;		/* Clear for power-up case */
	if (ipcmd(IP_RESET, ipp, 0, IPBUF)) return -1;
	ipaddr->ip_r0 = IP_CLRINT;	/* Get it out of idle loop */
	DELAY(30);
	i = ipaddr->ip_r0;
	i = ipaddr->ip_r0;

	/*
	 * Wait for disk to spin up, if necessary.
	 */
	if (!isspinning(ipspinning, (char *)ipaddr, 0x10 << ipp->ip_unit)) {
		DELAY(30);
		return -1;
	}
	if (ipcmd(IP_RESTORE, ipp, 0, IPBUF)) return -1;

	/*
	 * Read and check disk label.
	 */
	for (ipp->ip_bhead = 0; ipp->ip_bhead < MAXHEAD; ipp->ip_bhead++) {
		label->dkl_magic = 0;
		if (ipcmd(IP_READ, ipp, 0, IPBUF))
			continue;
		if (chklabel(label))
			continue;
		if (ipp->ip_bhead != label->dkl_bhead)
			continue;
		if (ppart != label->dkl_ppart)
			continue;
		/* reinitialize with proper number of sectors and heads */
		ipp->ip_nhead = label->dkl_nhead;
		ipp->ip_nsect = label->dkl_nsect;
		ipp->ip_ncyl  = label->dkl_ncyl;
		ipp->ip_bhead = label->dkl_bhead;	/* IMPORTANT for Lark */
		goto foundlabel;
	}
	printf(msg_nolabel);
foundlabel:

	/*
	 * Read blocks 1-15 of the disk -- the bootstrap block.
	 */
	caddr = (char *)LOADADDR;
	for (i = 1; i < BBSIZE/DEV_BSIZE; i++) {
		if (ipcmd(IP_READ, ipp, i, caddr))
			return (-1);
		caddr += DEV_BSIZE;
	}
	return (LOADADDR);
#	undef ipp
}

ipcmd(cmd, ipp, bno, buf)
	register int cmd;
	register struct ipparam *ipp;
	register int bno;
	char *buf;
{
	register unsigned short i;
	register struct iopb0 *ip0;
	register struct ipdevice *ipaddr = ipp->ip_addr;
	register char *bp;
	unsigned short cylno, sect, head;
	int status, error, errcnt = 0;

	ip0 = (struct iopb0 *)IOPB;
	bzero((char *)IOPB, IPIOPBSZ);
	bp = IPBUF;

	/*
	 * Calculate cylinder, head, and sector.
	 * Note that <i>, as well as the above, are unsigned short.
	 * Can't overflow -- we have spare factor of 4 on Eagle, won't
	 * be eaten for awhile and probably never on Interphase disks.
	 */
	i = bno / ipp->ip_nsect;
	sect = bno - i * ipp->ip_nsect;		/* Cheap % */
	cylno = i / ipp->ip_nhead;
	head = ipp->ip_bhead + i - cylno * ipp->ip_nhead;	/* Cheap % */

	if (cylno > ipp->ip_ncyl)
		return (-1);

retry:
	while (ipaddr->ip_r0 & IP_BUSY)
		DELAY(30);
	ip0->i0_cmd = cmd;
	ip0->i0_status = 0;
	ip0->i0_error = 0;
	ip0->i0_unit_cylhi = (1 << (4 + (ipp->ip_unit&3))) | ((cylno>>8)&0xF);
	ip0->i0_cylinder = cylno;
	ip0->i0_sector = sect;
	ip0->i0_secnt = 1;
	ip0->i0_buf_xmb = (long) DMADDR >> 16;
	ip0->i0_buf_msb = (long) DMADDR >> 08;
	ip0->i0_buf_lsb = (long) DMADDR;
	ip0->i0_head = head;
	ip0->i0_ioaddr = (int)ipaddr;
	ip0->i0_burstlen = IP0_BURSTLEN;

	/* point controller at iopb and start it up */

	ipaddr->ip_r1 = ((long) IOPBADDR >> 16) | IP_BUS;
	ipaddr->ip_r2 = (long) IOPBADDR >> 8;
	ipaddr->ip_r3 = (long) IOPBADDR;
	ipaddr->ip_r0 = IP_GO;

	while ((ip0->i0_status == 0) || (ip0->i0_status == IP_DBUSY))
		DELAY(30);
	status = ip0->i0_status;
	error = ip0->i0_error;

	while (ipaddr->ip_r0 & IP_BUSY)
		DELAY(30);
	ipaddr->ip_r0 = IP_CLRINT;
	if (status != IP_OK) {
		printf("ip: error %x\n", error);
		/* Attempt to reset the error condition */
		if (cmd != IP_RESTORE)
			if (ipcmd(IP_RESTORE, ipp, 0, (char *)0))
				return (-1);
		if (++errcnt < 10)
			goto retry;
		return (-1);			/* Error */
	}

	if (cmd == IP_READ && buf != bp)	/* Many just use common buf */
		for (i = 0; i < DEV_BSIZE; i++)
			*buf++ = *bp++;

	return (0);
}
