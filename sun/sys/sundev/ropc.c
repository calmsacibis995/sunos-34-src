#ifndef lint
static	char sccsid[] = "@(#)ropc.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 *  Sun-2 Rasterop Chip Driver
 */
#include "ropc.h"
#if NROPC > 0
#include "../h/param.h"
#include "../h/buf.h"
#include "../h/errno.h"
#include "../machine/pte.h"
#include "../sundev/mbvar.h"
#include "../pixrect/memreg.h"

/*
 * Driver information for auto-configuration stuff.
 */
int	ropcprobe(), ropcattach();
struct	mb_device *ropcinfo[1];	/* XXX only supports 1 chip */
struct	mb_driver ropcdriver = {
	ropcprobe, 0, ropcattach, 0, 0, 0,
	2 * sizeof(struct memropc), "ropc", ropcinfo, 0, 0, 0,
};

extern struct	memropc *mem_ropc;

/*ARGSUSED*/
ropcprobe(reg, unit)
	caddr_t reg;
	int unit;
{
	register struct memropc *ropcaddr = (struct memropc *)reg;

	if (peek((short *)ropcaddr) == -1)
		return (0);
	if (poke((short *)&ropcaddr->mrc_x15, 0x13)) 
		return (0);
	if (peek((short *)&ropcaddr->mrc_x15) != 0xff13)
		return (0);
	mem_ropc = (struct memropc *)reg;
	return (sizeof (struct memropc));
}

/*ARGSUSED*/
ropcattach(md)
	struct mb_device *md;
{

}

/*ARGSUSED*/
ropcopen(dev, flag)
	dev_t dev;
	int flag;
{
 
	if (mem_ropc == 0)
		return (ENXIO);
	return (0);
}

/*ARGSUSED*/
ropcmmap(dev, off, prot)
	dev_t dev;
	off_t off;
	int prot;
{

	if (off)
		return (-1);
	return (getkpgmap((caddr_t)mem_ropc) & PG_PFNUM);
}
#endif NROPC > 0
