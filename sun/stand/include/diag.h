/*	@(#) diag.h 1.1 9/25/86 SMI	*/
/*	from diag.h 1.12 84/04/08 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 *----------------------------------------------------------------------------
 *      disk diagnostic globals
 *----------------------------------------------------------------------------
 */
#include "setjmp.h"
jmp_buf abort_jmp;

#include "s2addrs.h"

typedef unsigned char uchar;
#define NULL	0

#define MBMEM_VA ((int)MBMEM_BASE)
#define MBIO_VA  ((int)MBIO_BASE)

#define DBUF_PA	0x1000
#define DBUF_VA	(uchar *)(MBMEM_VA+DBUF_PA)

#define STATUS     0
#define INIT       1
#define FORMAT     2
#define VERIFY     3
#define MAP        4
#define SEEK       5
#define READ       6
#define WRITE      7
#define RESTORE    8
#define	ZAP	   9	/* write bad header */
#define INTERDIAG 10
#define RAMDIAG   11
#define DRIVEDIAG 12
#define DRIVECHAR 13

int infomsgs, errors;
int devaddr, controller, drive, unit, target, ncyl, acyl, nhead, nsect;
int gap1, gap2, interleave, groupsize;
int basehead, physpart;
int drivetype;
int scsi;
char *ascii_id;

int errno;	/* to shut up C library */
struct dkmaplist *currmap;

#define NORMAL_RETRIES	5
#define FORMAT_RETRIES	1
extern	max_retries;

#define min(a, b)	((a) < (b) ? (a) : (b))
#define max(a, b)	((a) > (b) ? (a) : (b))

#define	C_SMD2180	0
#define	C_XY440		1
#define	C_XY450		2
#define	C_ADAPTEC	3
#define	C_ADES		4
