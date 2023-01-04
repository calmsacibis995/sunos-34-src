/*	@(#)diag.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 *----------------------------------------------------------------------------
 *      disk diagnostic globals
 *----------------------------------------------------------------------------
 */

#include <machine/param.h>
#include <mon/sunromvec.h>
#include <setjmp.h>

jmp_buf abort_jmp;

typedef unsigned char uchar;

#define NULL		0
#define FALSE		0
#define TRUE		(~FALSE)

/*
 * virtual address of DVMA which will work for
 * both sun2 (24 bit) and sun3 (28 bit)
 */
#define	DVMA		0x0FF00000

#define DBUF_PA		0x1000
#define DBUF_VA		(uchar *)(DVMA + DBUF_PA)
#define DBUF_PA1	0x1100		/* is XA1 really needed ? */
#define DBUF_VA1	(uchar *)(DVMA + DBUF_PA1)


#define STATUS		0
#define INIT		1
#define FORMAT		2
#define VERIFY		3
#define MAP		4
#define SEEK		5
#define READ		6
#define WRITE		7
#define RESTORE		8
#define	ZAP		9	/* write bad sector header */
#define	SLIP		10	/* slip bad sector */
#define	READ_SLIP	11	/* return slipped sector list */
#define	READ_DEFECT	12	/* read defect */

int infomsgs, errors, slipmsgs, formatmsgs;
int devaddr, controller, drive, unit, target, ncyl, acyl, nhead, nsect, nspare;
int gap1, gap2, interleave;
int basehead, physpart;
int drivetype;
int scsi;
int onboard;
int si_ha_type;
char *ascii_id;

int errno;			/* to shut up C library */
struct dkmaplist *currmap;

#define NORMAL_RETRIES	3
#define FORMAT_RETRIES	1
extern	int max_retries;

extern int passdata[];
extern int npassdata;

#define MIN(a, b)	((a) < (b) ? (a) : (b))
#define MAX(a, b)	((a) > (b) ? (a) : (b))

#define	C_SMD2180	0
#define	C_XY440		1
#define	C_XY450		2
#define	C_ADAPTEC	3
#define C_CCS_EMULEX	4
#define C_XY751		5

#define	SECSIZE		512	/* Size of a sector in bytes */

#define NBAD		126	/* No of bt_bad entries in dkbad structure */
#define LINEBUFSZ	128	/* Size of buffer used to read lines */

struct params {
	int s_nsect, s_gap1, s_gap2, s_interleave;
};

struct specs{                           /* disk drive specs */
	int s_ncyl, s_acyl, s_bhead, s_ppart, s_nhead;
	struct params *s_param;
	int special;			/* intended for anything */
	char s_id[32];
};
