/*
 * @(#)ttboot.c 1.5 84/08/05 Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * General tape boot code for routines which implement the "standalone
 * driver" device interface.
 */

#include "../../sys/sunstand/saio.h"
#include "../../sys/sunstand/sasun.h"
#undef DEV_BSIZE
#undef BBSIZE
#undef DEF_MBMEM_VA
#undef DEF_MBIO_VA
#undef MAX
#include "../h/sasun.h"

/*
 * Values higher than 64K will give problems for st driver since
 * dma count register is only 16 bits and driver doesn't deal properly
 * with longer transfers.
 */
#define	MAX_TAPE_REC_SIZE	32768


int
ttboot(bp)
	register struct bootparam *bp;
{
	struct saioreq req;
	int blkno;
	register int len;
	char *addr;
	register int result = -1;			/* Assume error */

	req.si_ctlr = bp->bp_ctlr;
	req.si_unit = bp->bp_unit;
	req.si_boff = (daddr_t)bp->bp_part;
	if ((*bp->bp_boottab->b_open)(&req))
		return result;				/* Never opened */

	for (blkno = 1, addr = (char *)LOADADDR;
	     ;
	     blkno++, addr += len) {
		req.si_bn = blkno;
		req.si_cc = MAX_TAPE_REC_SIZE;		/* Largest possible */
		req.si_ma = addr;
		len = (*bp->bp_boottab->b_strategy)(&req, READ);
		if (len < 0) break;			/* Tape read error */
		if (len == 0) {
			/* EOF on first read is error; otherwise is OK */
			if (blkno != 1) result = LOADADDR;
			break;
		}
	}
	/*
	 * Tapes get closed before exiting, since called program can't
	 * tell where in the tape they are.  (Plus, if they use the
	 * standalone library to read from tape, it will rewind and
	 * forward space to the file anyway.)  It also makes it easier
	 * for the user to remove the tape.
	 */
	(*bp->bp_boottab->b_close)(&req);
	return result;
}
