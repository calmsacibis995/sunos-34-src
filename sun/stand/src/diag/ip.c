#ifndef lint
static	char sccsid[] = "@(#)ip.c 1.1 86/09/25";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "diag.h"

int bus;

#define R0              *(uchar*)(devaddr + 1)
#define R1              *(uchar*)(devaddr + 0)
#define R2              *(uchar*)(devaddr + 3)
#define R3              *(uchar*)(devaddr + 2)
#define GO              0x1
#define CLR             0x2
#define REL             0       /*  0 => absolute addressing */
#define BURSTLEN        16
#define DIOPB_PA        (struct iopb*)0x100
#define DIOPB_VA        (struct iopb*)(DVMA + (int)DIOPB_PA)
#define DUIB_PA		(struct uib *)0x140
#define DUIB_VA		(struct uib *)(DVMA + (int)DUIB_PA)

/* command codes */

#define D_READ          0x81
#define D_WRITE         0x82
#define D_VERIFY        0x83
#define D_FORMAT        0x84
#define D_MAP           0x85
#define D_SWITCH        0x86
#define D_INIT          0x87
#define D_RESTORE       0x89
#define D_SEEK          0x8a
#define D_ZERO          0x8b
#define D_SPINDWN       0x8c
#define D_RESET         0x8f

/* command list */

char *IP_cmdlist[] = {
	"read", "write", "verify", "format", "map", "switch",
	"initialize", "?", "restore", "seek", "zero", "spin down",
	"?", "?", "reset"
};

/* error list */

char *IP_errlist[] = {
	"disk not ready", "invalid disk address", "seek error",
	"ECC code error -- data field", "invalid command code",
	"invalid track in IOBP", "invalid sector in command",
	"(spare #17)", "bus timeout or drive powered down", "write error",
	"disk write protected",
	"unit not selected", "no address mark -- header field",
	"no data mark -- data field", "unit fault", "data overrun timeout",
	"surface overrun", "id field error -- wrong sector read",
	"id field ECC error", "(spare #23)", "(spare #24)", "(spare #25)",
	"no sector pulse", "data overrun", "no index pulse on write format",
	"sector not found", "id field error -- wrong head",
	"invalid sync in data field", "invalid sync in header field",
	"seek timeout error", "busy timeout",
	"no normal complete at beginning of a seek", "rtz timeout",
	"format overrun on data", "?", "?", "?", "?", "?", "?", "?", "?",
	"?", "?", "?", "?", "?", "?", "unit not initialized",
	"disk busy executing", "(spare #42)", "ANSI bus timeout -- type 1",
	"ANSI bus timeout -- type 2", "ANSI bus timeout -- type 3",
	"ANSI bus error", "illegal command", "illegal parameter",
	"time dependent command error", "command reject", "seek error",
	"(spare #4C)", "unspecified seek error", "read/write fault"
};

/* i/o parameter block */

struct iopb {
	uchar b_status;
	uchar b_cmd;
	uchar b_unit_cylhi;
	uchar b_error;
	uchar b_sector;
	uchar b_cylinder;
	uchar b_buf_xmb;
	uchar b_secnt;
	uchar b_buf_lsb;
	uchar b_buf_msb;
	uchar b_ioaddr;
	uchar b_head;
	uchar b_nxt_xmb;
	uchar b_burstlen;
	uchar b_nxt_lsb;
	uchar b_nxt_msb;
	uchar b_seg_lsb;
	uchar b_seg_msb;
};

/* unit initialization block */

struct uib {
	uchar u_nsect, u_nhead;
	uchar u_secsize_hi;
	uchar u_secsize_lo;
	uchar u_gap2, u_gap1;
	uchar u_retry, u_interleave;
	uchar u_reseek, u_ecc;
	uchar u_inchead, u_movebad;
};

/*ARGSUSED*/
IPcmd(cmd, p1, p2, p3, p4, p5, p6, p7, p8)
{
	register int i;

	switch (cmd) {

	case STATUS:
		printf("status: ");
		i = R0;

		if (i & 0x80) printf("unit-3-ready ");
		if (i & 0x40) printf("unit-2-ready ");
		if (i & 0x20) printf("unit-1-ready ");
		if (i & 0x10) printf("unit-0-ready ");

		if (i & 0x02) printf("done ");
		if (i & 0x01) printf("busy");
		printf("\n");
		return 0;

	case INIT:
		bus = 0x20;
		return IPexec(D_RESET, 0, 0, 0, 0, 0);

	case FORMAT:
		return IPexec(D_FORMAT, p1, p2, 0, nsect, p3);

	case VERIFY:
		return IPexec(D_VERIFY, p1, p2, 0, nsect, 0);

	case MAP:
		return IPexec(D_MAP, p1, p2, 0, p3, p4);

	case SEEK:
		return IPexec(D_SEEK, p1, 0, 0, 0, 0);

	case READ:
		return IPexec(D_READ, p1, p2, p3, p4, p5);

	case WRITE:
		return IPexec(D_WRITE, p1, p2, p3, p4, p5);

	case RESTORE:
		return IPexec(D_RESTORE, 0, 0, 0, 0, 0);

	default:
		return -1;
	}
}

IPexec(cmd, cyl, head, sec, cnt, buf)
{
	struct iopb *p = DIOPB_VA;
	struct iopb *pp = DIOPB_PA;
	register int retry = 0;
	register int physunit;
	int i;

	p->b_cmd = cmd;
	p->b_status = 0;
	p->b_error = 0;
	p->b_unit_cylhi = (cyl >> 8);
	physunit = unit & 03;

	/*
	 * The unit # is a bit mask in which exactly one bit must be set.
	 * 0x80 selects unit 4, 0x40 for 3, 0x20 for 2, 0x10 for 1.
	 */
	p->b_unit_cylhi |= (1<<physunit) << 4;

	p->b_cylinder = cyl & 0xff;
	p->b_sector = sec;

	if (cmd == D_MAP) {
		/* MAP command has funny parameters */
		p->b_secnt = cnt;
		p->b_buf_xmb = buf >> 8;
		p->b_buf_msb = buf & 0xff;
		p->b_buf_lsb = (cnt+buf) & 0xff; /* Data byte for format */
	} else {
		p->b_secnt = cnt;
		p->b_buf_xmb = (buf >> 16) | bus | REL;
		p->b_buf_msb = (buf >> 08) & 0xff;
		p->b_buf_lsb = buf & 0xff;
	}
	p->b_head = head + basehead;
	p->b_ioaddr = devaddr & 0xff;
	p->b_burstlen = BURSTLEN;
	p->b_nxt_xmb = bus | REL;
	p->b_nxt_msb = 0;
	p->b_nxt_lsb = 0;
	p->b_seg_msb = 0;
	p->b_seg_lsb = 0;

	do {
		p->b_status = 0;
		p->b_error = 0;
		R1 = ((long)pp >> 16) | bus;
		R2 = ((long)pp >> 8) & 0xff;
		R3 = (long)pp & 0xff;
		R0 = CLR | GO;
		while ((p->b_status == 0) || (p->b_status == 0x81))
			;
		R0 = CLR;
		for (i = 0; i < 30; i++)
			;

		if ((p->b_error != 0) && (p->b_error != 0x41) && errors)
		     printf("%s error #%x, %s, cyl=%d, head=%d, sector=%d retry=%d\n",
				IP_cmdlist[p->b_cmd - 0x81],
				p->b_error, IP_errlist[p->b_error - 0x10],
				((p->b_unit_cylhi&0xf)<<8)+p->b_cylinder,
				p->b_head, p->b_sector, retry);

	} while ((p->b_error == 0x41) ||
		((p->b_status != 0x80) && (retry++ < max_retries)));

	if (retry > max_retries)
		retry = max_retries;

	switch (p->b_status) {

	case (uchar) 0x80:
		if (infomsgs || (p->b_error != 0))
		   printf("%s worked, cyl=%d head=%d sector=%d%s%s retry=%d hard retry=%d\n",
			IP_cmdlist[p->b_cmd - 0x81],
			((p->b_unit_cylhi&0xf)<<8)+p->b_cylinder,
			p->b_head, p->b_sector,
			(p->b_error & 0x80)? " error-corrected" : "",
			(p->b_error & 0x10)? " reseek-performed" : "",
			retry, p->b_error & 0xf);

		return 0;

	case (uchar) 0x82:
		printf("%s failed #%x, %s, cyl=%d head=%d sector=%d\n",
			IP_cmdlist[p->b_cmd - 0x81],
			p->b_error, IP_errlist[p->b_error - 0x10],
			((p->b_unit_cylhi&0xf)<<8)+p->b_cylinder,
			p->b_head, p->b_sector);
		return -1;

	default:
		printf("unknown status %x\n", p->b_status);
		return -1;
	}
}
