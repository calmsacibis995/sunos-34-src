#ifndef lint
static        char sccsid[] = "@(#)xd.c 1.1 86/09/25";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "diag.h"
#include <sys/types.h>
#include <sundev/xdreg.h>
#include <sundev/xdcreg.h>
#include "xyxd.h"


#define DIOPB_PA        0x100
#define DIOPB_VA        (DVMA + DIOPB_PA)

#ifdef DEBUG
int f751;
#endif

#define XDTIMEOUT 0xfffff

#define	XD_CYL_LO(n)	((int)(n)&0xff)
#define	XD_CYL_HI(n)	(((int)(n)&0xff00)>>8)
#define	XD_GET_SEC(h)	(h->xdh_sec)
#define	XD_GET_CYL(h)	(((h)->xdh_cyl_hi<<8)|((h)->xdh_cyl_lo))


#define XD_err(x) xderrors[xdgeterr(x)].e_name

/* command list */
char *XD_cmdlist[] = {
	"nop",
	"write",
	"read",
	"seek",
	"drive reset",
	"write parameters",
	"read parameters",
	"extended write",
	"extended read",
	"diagnostics",
	"abort",
};

char XD_nocomplete[] = "[no return status]";
struct	error {
	char	*e_name;	/* error message */
	char	e_retry;	/* # of times to retry */
	char 	e_restore;	/* # of times to restore after retries */
	u_char 	e_err_code	/* error code */
} xderrors[] = {
	"no error",			3,	1,	0x00,

	/* non retryable programming errors */
	"illegal cylinder",		0,	0,	0x10,
	"illegal head",			0,	0,	0x11,
	"illegal sector",		0,	0,	0x12,
	"count zero",			0,	0,	0x13,
	"unimplemented cmd",		0,	0,	0x14,
	"ill field length 1",		0,	0,	0x15,
	"ill field length 2",		0,	0,	0x16,
	"ill field length 3",		0,	0,	0x17,
	"ill field length 4",		0,	0,	0x18,
	"ill field length 5",		0,	0,	0x19,
	"ill field length 6",		0,	0,	0x1a,
	"ill field length 7",		0,	0,	0x1b,
	"ill scatter/gather length",	0,	0,	0x1c,
	"not enough sectors per trk",	0,	0,	0x1d,
	"next IOPB alignement error",	0,	0,	0x1e,

	/* sucessful recovered soft errors */
#define XDE_OKECC		0x12
	"soft ECC corrected",		3,	1,	0x30,
	"ECC ignored",			3,	1,	0x31,
#define XDE_SEEK_RETRY	0x32
	"auto seek retry recovered",	3,	1,	0x32,
	"soft retry recovered",		3,	1,	0x33,

	/* hard errors / retry */
	"Hard data ECC",		3,	1,	0x40,
	"header not found",		3,	1,	0x41,
#define DP_RETRY 4
#define XDE_NOTREADY	0x34
	"drive not ready",     3*DP_RETRY,	3,	0x42,
	"operation timeout",		3,	1,	0x43,
	"DMAC timeout",			3,	1,	0x44,
	"disk sequencer error",		3,	1,	0x45,
	"buffer parity",		3,	1,	0x46,
	"dual port busy timeout",	3,	1,	0x47,
	"header ecc error",		3,	1,	0x48,
	"read verify",			3,	1,	0x49,
	"fatal DMAC error",		3,	1,	0x4a,
	"VMEbus error",			3,	1,	0x4b,

	/* hard errors / reset / retry */
	"drive faulted",		3,	1,	0x60,
	"header error/cylinder",	3,	1,	0x61,
	"header error/head",		3,	1,	0x62,
	"drive not on-cylinder",	3,	1,	0x63,
	"seek error",			3,	1,	0x64,

	/* fatal hardware errors */
	"illegal sector size",		0,	0,	0x70,
	"firmware failure",		0,	0,	0x71,

	/* miscellaneous errors */
#define XDE_SOFTECC	0x1a
	"soft ecc",			3,	1,	0x80,
	"IRAM checksum failure",	3,	1,	0x81,
	"Abort by command",		3,	1,	0x82,

	"write protect error",		3,	1,	0x90,
#define	XDE_UNKNOWN	0xf1
#define	XDE_FTERR		0xf2
#define	XDE_FORWARD	0xf3
#define	XDE_FORFAIL	0xf4
#define	XDE_LOSTINT       0xf5
	"unknown error",		0,	3,	0xf1,
	"fatal error",			0,	3,	0xf2,
	"forwarding bad block",		0,	0,	0xf3,
	"bad block forward failed",	0,	0,	0xf4,
	"lost interrupt",		0,	0,	0xf5,
	0
};

extern int hsect;

/*
 * this function return the index to the errors array of
 * an error code.
 */
int
xdgeterr(error)
    u_char error;
{
    return(error);
}

/* autoupdate; longword xfer; aior = 100 us; disable ac fail handling */
#define CTLRPAR1 0xe000	
#define CTLRPAR2 0x5e00 /* eccm = mode 2; retry before correction */
			/* zero latency read; automatic seek retry */
			/* throttle weight = 256 */
#define P1AND2 0x020a	/* disk format parameters */
#define P3AND4 0x1B14
#define P6AND7 0x0a030200

/*ARGSUSED*/
XDcmd(cmd, p1, p2, p3, p4, p5, p6, p7, p8)
{
    return (-1);
}

extern int silent;

int
XDexec(cmd, cyl, head, sec, cnt, buf, subcmd)
{
    return -1;
}

xdhdrfill(hp, cyl, head, nsec)
    struct xdhdr *hp;
{
    hp->xdh_head =  head;
    hp->xdh_sec =  nsec;
    hp->xdh_cyl_lo =  XD_CYL_LO(cyl);
    hp->xdh_cyl_hi =  XD_CYL_HI(cyl);
}

#ifdef DEBUG
printiopb()
{
    struct iopb {
	u_char byt0;
	u_char byt1;
	u_char byt2;
	u_char byt3;
	u_char byt4;
	u_char byt5;
	u_char byt6;
	u_char byt7;
	u_char byt8;
	u_char byt9;
	u_char byt10;
	u_char byt11;
	u_char byt12;
	u_char byt13;
	u_char byt14;
	u_char byt15;
	u_char byt16;
	u_char byt17;
	u_char byt18;
	u_char byt19;
	u_char byt20;
	u_char byt21;
	u_char byt22;
	u_char byt23;
    } *p = (struct iopb *)DIOPB_VA;

    printf("iopbyte 0, 1 = %x %x\n", p->byt0, p->byt1);
    printf("iopbyte 2, 3 = %x %x\n", p->byt2, p->byt3);
    printf("iopbyte 4, 5 = %x %x\n", p->byt4, p->byt5);
    printf("iopbyte 6, 7 = %x %x\n", p->byt6, p->byt7);
    printf("iopbyte 8, 9 = %x %x\n", p->byt8, p->byt9);
    printf("iopbyte a, b = %x %x\n", p->byt10, p->byt11);
    printf("iopbyte c, d = %x %x\n", p->byt12, p->byt13);
    printf("iopbyte e, f = %x %x\n", p->byt14, p->byt15);
    printf("iopbyte 10, 11 = %x %x\n", p->byt16, p->byt17);
    printf("iopbyte 12, 13 = %x %x\n", p->byt18, p->byt19);
    printf("iopbyte 14, 15 = %x %x\n", p->byt20, p->byt21);
    printf("iopbyte 16, 17 = %x %x\n", p->byt22, p->byt23);
}

stop()
{
    char buf[80];

    gets(buf);
}

#endif
