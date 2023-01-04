/*
 * @(#)xxboot.c 1.6 84/08/05 Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * General boot code for routines which implement the "standalone
 * driver" boot interface.
 */

#include "../../sys/sunstand/saio.h"
#include "../../sys/sunstand/sasun.h"
#undef DEV_BSIZE
#undef BBSIZE
#undef DEF_MBMEM_VA
#undef DEF_MBIO_VA
#undef MAX
#undef SOUND_BASE
#include "../h/sasun.h"

int
xxboot(bp)
	register struct bootparam *bp;
{
	struct saioreq req;
	int blkno;
	char *addr;

	req.si_ctlr = bp->bp_ctlr;
	req.si_unit = bp->bp_unit;
	req.si_boff = (daddr_t)bp->bp_part;
	if ((*bp->bp_boottab->b_open)(&req))
		return -1;

	for (blkno = 1, addr = (char *)LOADADDR;
	     blkno <= BBSIZE/DEV_BSIZE;
	     blkno++, addr += DEV_BSIZE) {
		req.si_bn = blkno;
		req.si_cc = DEV_BSIZE;
		req.si_ma = addr;
		if (req.si_cc != (*bp->bp_boottab->b_strategy)(&req, READ) )
			return -1;
	}
	return LOADADDR;
}
