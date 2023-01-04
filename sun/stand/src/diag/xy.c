#ifndef lint
static	char sccsid[] = "@(#)xy.c 1.1 86/09/25";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "diag.h"
#include <sys/types.h>
#include <sundev/xyreg.h>
#include <sundev/xycreg.h>
#include "xyxd.h"


#define THROTTLE        5	/* 64 words/transfer */

#define DIOPB_PA        0x100
#define DIOPB_VA        (DVMA + DIOPB_PA)

/* command list */
char *XY_cmdlist[] = {
	"nop",
	"write",
	"read",
	"write track headers",
	"read track headers",
	"seek",
	"reset",
	"format",
	"read all",
	"status",
	"write all",
	"set drive size",
	"self test",
	"bad cmd",
	"maint buf load",
	"maint buf dump",
};

/* Special message for when a 450 doesn't update the done bit in the iopb */
char XY_nocomplete[] = "[no return status]";

/* error list */
char *XY_errlist[] = {
	"no error",			/* 00 */
	"interrrupt pending",		/* 01 */
	"error pending",		/* 02 */
	"busy conflict",		/* 03 */
	"operation timeout",		/* 04 */
	"read err - header 2",		/* 05 */
	"CRC/hard ECC error",		/* 06 */
	"cyl addr error",		/* 07 */
	"unknown",			/* 08 */
	"unknown",			/* 09 */
	"sector addr error",		/* 0a */
	"unknown",			/* 0b */
	"unknown",			/* 0c */
	"sector switches wrong",	/* 0d */
	"memory addr error",		/* 0e */
	"unknown",			/* 0f */
	"unknown",			/* 10 */
	"unknown",			/* 11 */
	"seek err - header 1",		/* 12 */
#define	XYE_SEEK_RETRY 0x13
	"seek retry",			/* 13 */
	"write protect",		/* 14 */
	"bad command",			/* 15 */
	"drive not ready",		/* 16 */
	"zero sector count",		/* 17 */
	"drive fault",			/* 18 */
	"sector switches wrong",	/* 19 */
	"self test error a",		/* 1a */
	"self test error b",		/* 1b */
	"self test error c",		/* 1c */
	"unknown",			/* 1d */
	"soft ECC error",		/* 1e */
	"fixed ECC error",		/* 1f */
	"head error",			/* 20 */
	"disk sequencer error",		/* 21 */
	"unknown",			/* 22 */
	"unknown",			/* 23 */
	"unknown",			/* 24 */
	"drive seek error",		/* 25 */
	"hard error",			/* 26 */
	"double hard error",		/* 27 */
};

/*
 * Rom revision number - we might need to use this since Xylogics keeps
 * changing the way certain things work with different revisions.
 */
u_char xyrev;

char abortdma = 1;		/* Default to abort after non-compare */
extern int hsect;

C_abortdma()
{

	abortdma = !abortdma;
	printf("Now %saborting on non-compares\n", abortdma? "" : "not ");
}

C_dmatest()
{
	register int e, i, n, oldi = 0;
	register u_char *cp1, *cp2;
	
	if (controller != C_XY450) {
		printf("dmatest only allowed on Xylogics 450/451\n");
		return;
	}
	e = XYexec(XY_BUFLOAD, 0, 0, 0, 0, DBUF_PA+1024);
	for (n = 0; ; n++) {
		printf("\r%d ", n);
		i = rand();
		for (cp1 = DBUF_VA+1024; cp1 < DBUF_VA+2048; cp1++)
			*cp1 = i;
		e = XYexec(XY_BUFDUMP, 0, 0, 0, 0, DBUF_PA);
		
		cp1 = DBUF_VA;
		cp2 = DBUF_VA+1024;
		e = 0;
		for (i = 0; i < SECSIZE; i++) {
			if (*cp1++ != *cp2++) {
				if (!e++)
					printf("  Wrote %x  Read %x  Prev %x ",
						*cp1, *cp2, oldi);
			}
		}
		if (e) {
			printf(":%d differ\n", e);
			if (abortdma)
				return;
		}
		oldi = *(char *)(DBUF_VA+1024);
		tryabort();
	}
}

/*
 * Shifts an array of numbers from pseudo index form
 * to physical index form.  Assumes dp array has extra room.
 */
to_physical(dp, head)
	register int dp[], head;
{
	register i;

	if (head == 0)
		return;
	for (i = hsect-1; i >= 0; i--)
		dp[i + head] = dp[i];
	for (i = 0; i < head; i++)
		dp[i] = dp[hsect + i];
}

/*
 * Shifts an array of numbers from physical index form
 * to pseudo index form.  Assumes dp array has extra room.
 */
to_pseudo(dp, head)
	register int dp[], head;
{
	register i;

	if (head == 0)
		return;
	for (i = 0; i < head; i++)
		dp[hsect + i] = dp[i];
	for (i = 0; i < hsect; i++)
		dp[i] = dp[i + head];
}

C_whdr()
{
	register int cyl, head, sec, bn;
	register int *dp = (int *)DBUF_VA;

	printf("write track header\nCAN DESTROY TRACK DATA!\n");
	bn = do_rhdr(0);
	if (bn < 0)
		return;

	cyl = bn / (nhead * nsect);
	head = (bn % (nhead * nsect)) / nsect;

	for (;;) {
		printf("ok? ");
		if (confirm())
			break;
		for (;;) {
			sec = pgetn("physical sector number to modify (dec): ");
			if (sec >= 0 && sec < hsect)
				break;
			printf("%d is not within 0 and %d\n", sec, hsect-1);
		}
		printf("old value = %x, new value (hex): ", dp[sec]);
		dp[sec] = getx();
		hdr_print(dp, cyl, head);
	}

	printf("Ok to write this track header information at %d/%d? ",
		cyl, head);

	if (confirm())
		(void) XYexec(XY_WRITEHDR, cyl, head, 0, hsect, DBUF_PA);
}

xyhdrfill(hp, cyl, head, nsec)
    struct xyhdr *hp;
{
    hp->xyh_type = drivetype;
    hp->xyh_head = head;
    hp->xyh_sec_lo = XY_SEC_LO(nsec);
    hp->xyh_sec_hi = XY_SEC_HI(nsec);
    hp->xyh_cyl_lo = XY_CYL_LO(cyl);
    hp->xyh_cyl_hi = XY_CYL_HI(cyl);
}

/*
 * This routine is a hack to cover the out-of-spec eagles.
 * Occasionally, the format command screws up the header
 * of logical sector 0.  We read it back and correct it now if
 * it is garbage.  This prevents the user from ever knowing
 * what is happening.
 */

static int
fixformat(p1, p2, p3, p4)
{
	register int *dp = (int *)DBUF_VA;
	register struct xyhdr *hp;
	int i;

	i = do_readhdr(p1, p2);
	if (i == 0) {
		hp = (struct xyhdr *)&dp[p2];
		if ((XY_GET_SEC(hp) != 0) || (XY_GET_CYL(hp) != p1)) {
			if (formatmsgs) {
				printf("format failed.\n");
				printf("original track headers for ");
				hdr_print(dp, p1, p2);
			}

			/*
			 * clear out the bogus header
			 */

			dp[p2] = 0;
			hp->xyh_type = drivetype;
			hp->xyh_head = p2;
			hp->xyh_sec_lo = XY_SEC_LO(0);
			hp->xyh_sec_hi = XY_SEC_HI(0);
			hp->xyh_cyl_lo = XY_CYL_LO(p1);
			hp->xyh_cyl_hi = XY_CYL_HI(p1);
			if (formatmsgs) {
				printf("new track headers for ");
				hdr_print(dp, p1, p2);
			}
			i |= XYexec(XY_WRITEHDR, p1, p2, p3, p4, DBUF_PA);
		}
	}
	return i;
}

/*
 * Check for obsolete PROMs.  If we have one warn the user and give
 * them a chance to continue on if they really want to.
 */
static
checkrev()
{
	if (xyrev != 'A')
		return;

	printf("\nRevision A Xylogics PROMs are no longer supported.\n");
	printf("They may fail some operations during standalone\n");
	printf("and UNIX usage.  Contact your Sun Field Service\n");
	printf("representative for more information.\n");

	printf("\nDo you wish to continue? ");
	if (confirm())
		return;

	doexit();
	/*NOTREACHED*/
}

struct xystat {
	u_char	xs_junk1[6];
	u_char	xs_nhead;
	u_char	xs_nsect;
	u_short	xs_ncyl;
	u_char	xs_rev;
	u_char	xs_dstat;
	u_short	xs_secsize;
	u_char	xs_junk2;
	u_char	xs_hsect;
	u_char	xs_junk3;
	u_char	xs_bhead;
};

/*ARGSUSED*/
XYcmd(cmd, p1, p2, p3, p4, p5, p6, p7, p8)
{
	register struct xydevice *xy = (struct xydevice *)devaddr;
	register struct xystat *st = (struct xystat *)DIOPB_VA;
	register int i;

	switch (cmd) {

	case STATUS:
		printf("status: ");
		i = xy->xy_csr;
		if (i & XY_BUSY) printf("busy ");
		if (i & XY_ERROR) printf("error ");
		if (i & XY_DBLERR) printf("double-error ");
		if (i & XY_DREADY) printf("ready ");
		printf("\n");

		(void) XYexec(XY_STATUS, 0, 0, 0, 0, 0);

		/*
		 * Retry if we got make a hardware sector
		 * count of zero back, this sometimes comes from
		 * a Rev A PROM not being ready after a reset.
		 */
		if (st->xs_hsect == 0) {
			DELAY(50000);
			(void) XYexec(XY_STATUS, 0, 0, 0, 0, 0);
		}

		i = st->xs_dstat;
		printf("drive status: %s", (i & 0x40) ? "not ready" : "ready");
		if (i & 0x80) printf(" off-cyl");
		if (i & 0x20) printf(" write-protected");
		if (i & 0x10) printf(" dual-port-busy");
		if (i & 0x08) printf(" seek-error");
		if (i & 0x04) printf(" drive-fault");
		printf("\n");

		/*
		 * Save and display revision number as a letter.
		 */
		xyrev = st->xs_rev + 'A' - 1;
		printf("Xylogics PROM Rev `%c'\n", xyrev);

		hsect = st->xs_hsect;
		if (hsect == 0) {
			printf("DRIVE NOT READY\nRE-ISSUE ");
			printf("status COMMAND AFTER DRIVE IS READY!\n");
			i = -1;
		} else if (hsect < nsect) {
			printf("DISK CONFIGURED FOR ONLY %d SECTORS.\n", hsect);
			printf("%s %d DATA SECTORS!\n",
				"IT MUST BE ABLE TO HANDLE AT LEAST", nsect);
			printf("%s status COMMAND BEFORE PROCEEDING!!!\n",
				"CORRECT AND RE-ISSUE");
			i = -1;
		} else
			i = 0;

		checkrev();
		if (i == 0)
			runtsect(0, 0);
		return i;

	case INIT:
		i = xy->xy_resupd;	/* reading resets */
		(void) XYexec(XY_RESTORE, 0, 0, 0, 0, 0);
		if (controller == C_XY440)
			return 0;
		return XYexec(XY_INIT, ncyl+acyl-1, nhead-1, nsect-1, 0, 0);

	case FORMAT:
		if (hsect < nsect && controller == C_XY450) {
			printf("\nSorry, cannot format without first\n");
			printf("successfully executing a status command.\n");
			_longjmp(abort_jmp, 1);
			/*NOTREACHED*/
		}
		i = XYexec(XY_FORMAT, p1, p2, p3, p4, 0);
		if (controller == C_XY440)
			return i;
		i |= fixformat(p1, p2, p3, p4);
		i |= XYexec(XY_WRITE, p1, p2, p3, p4, DBUF_PA);
		return i;

	case VERIFY:
		return XYexec(XY_READ, p1, p2, p3, p4, DBUF_PA);

	case MAP:
		printf("Xylogics mapping not supported!\n");
		return -1;

	case SEEK:
		return XYexec(XY_SEEK, p1, 0, 0, 0, 0);

	case READ:
		return XYexec(XY_READ, p1, p2, p3, p4, p5);

	case WRITE:
		return XYexec(XY_WRITE, p1, p2, p3, p4, p5);

	case RESTORE:
		return XYexec(XY_RESTORE, 0, 0, 0, 0, 0);

	case ZAP:
		/* write bad header */
		if (controller == C_XY440) {
			*(int *)DBUF_VA = HDR_ZAP;
			return XYexec(XY_WRITEALL, p1, p2, p3, 1, DBUF_PA);
		}

		return slipzap(p1, p2, p3, HDR_ZAP);

	case SLIP:
		/*
		 * Attempt to slip a sector to a spare sector
		 * by remapping the headers for the track.
		 * This cannot be done on a 440 controller
		 * or if the hard sector/track (hsect) is not
		 * larger than the soft sector/track (nsect).
		 */
		if (controller == C_XY440 || hsect <= nsect)
			return -1;

		return (slipzap(p1, p2, p3, HDR_SLIP));

	case READ_SLIP:
		/*
		 * Return a list of previously slipped logical sectors
		 */
		if (controller == C_XY440 || hsect <= nsect)
			return -1;

		return read_slip(p1, p2, (int *)p3);

	case READ_DEFECT:
		/*
		 * read the defect info of a track 
		 */
		if (controller != C_XY450)
	 	    return -1;
		if (xyrev < 'a')
		    return -1;		/* only the 451 support read defect */
		(void) XYexec(XY_READALL|XY_DEFLST, p1, p2, p3, p4, p5);
		reversed(); 		/* reversed the bits */
		return 0;

	default:
		return -1;
	}
}

int xydelay = 0x1ffff;
int xythrottle = THROTTLE;
extern int silent;

int
XYexec(cmd, cyl, head, sec, cnt, buf)
{
	struct xyiopb *p = (struct xyiopb *)DIOPB_VA;
	register int retry = 0;
	register struct xydevice *xy = (struct xydevice *)devaddr;
	int i, t;
	int err;
	int cmdcode = cmd >> 8;

	do {
		bzero((char *)p, sizeof *p);
		p->xy_autoup = 1;
		p->xy_reloc = 1;
		p->xy_cmd = cmdcode;
		p->xy_drive= drivetype;
		p->xy_unit = unit;
		p->xy_throttle = xythrottle;
		p->xy_sector = sec;
		p->xy_head = head;
		if (controller == C_XY450) {
			p->xy_bhead = basehead;	/* 450 only */
			p->xy_enabext = 1;	/* enable extensions */
			p->xy_eccmode = 2;	/* correct ECC */
			p->xy_recal = 1;	/* auto recal */
			p->xy_subfunc = cmd & 0xff;
		}
		p->xy_cylinder = cyl;
		p->xy_nsect = cnt;
		p->xy_bufoff = XYOFF(buf);
		p->xy_bufrel = XYREL(xy, buf);

		i = xy->xy_resupd;		/* reset previous errors */
		err = 0;

		while (xy->xy_csr & XY_BUSY) {
			for (i = 0; i < 30; i++)
				;
			tryabort();
		}
		t = XYREL(xy, DIOPB_PA);
		xy->xy_iopbrel[0] = t >> 8;
		xy->xy_iopbrel[1] = t;
		xy->xy_iopboff[0] = DIOPB_PA >> 8;
		xy->xy_iopboff[1] = DIOPB_PA;
		xy->xy_csr = XY_GO;
		if (controller == C_XY440)
			for (i = 0; i < 30; i++)
				;
		i = 0;
		while (xy->xy_csr & XY_BUSY) {
			if (i++ == xydelay) {
			    if (!silent)
			        printf("cmd %s cyl %d head %d sect %d time out!\n",
					XY_cmdlist[cmdcode], cyl, head, sec);
			    return -1;
			}
			tryabort();
		}

		if (xy->xy_csr & XY_ERROR)
			err = XYE_ERR;
		if (xy->xy_csr & XY_DBLERR)
			err = XYE_DERR;
		if (p->xy_iserr)
			err = p->xy_errno;

		if ((p->xy_complete || controller == C_XY440) && err == 0)
			break;

		if (errors && (p->xy_complete || controller == C_XY440))
			printf(
			"%s error #%x, %s, cyl=%d head=%d sector=%d retry=%d\n",
			    XY_cmdlist[cmdcode], err,
			    XY_errlist[err], cyl, head, sec, retry);
		else if (errors)
			printf(
			"%s error %s, cyl=%d head=%d sector=%d retry=%d\n",
			    XY_cmdlist[cmdcode], XY_nocomplete,
			    cyl, head, sec, retry);

		if (err == XYE_SEEK_RETRY && p->xy_complete &&
		    controller == C_XY450) {
			err = 0;
			break;
		}

	} while (retry++ < max_retries);

	if (retry > max_retries)
		retry = max_retries;

	if ((p->xy_complete || controller == C_XY440) && err == 0) {
		if (infomsgs)
			printf("%s worked, cyl=%d head=%d sector=%d retry=%d\n",
			    XY_cmdlist[cmdcode], cyl, head, sec, retry);
		return 0;
	}

	if (!silent)
		if (p->xy_complete || controller == C_XY440)
			printf("%s failed #%x, %s, cyl=%d head=%d sector=%d\n",
			    XY_cmdlist[cmdcode], err, XY_errlist[err],
			    cyl, head, sec);
		else {
			printf("%s failed %s, cyl=%d head=%d sector=%d\n",
			    XY_cmdlist[cmdcode], XY_nocomplete,
			    cyl, head, sec);
		}

	return -1;
}
