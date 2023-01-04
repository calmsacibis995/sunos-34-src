#ifndef lint
static  char sccsid[] = "@(#)fbutil.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Frame buffer driver utilities.
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/ioctl.h"
#include "../machine/pte.h"
#include "../machine/mmu.h"
#include "../sundev/mbvar.h"

/* ARGSUSED */
int
fbopen(dev, flag, numdevs, mb_devs)
	dev_t	dev;
	int	flag, numdevs;
	struct	mb_device **mb_devs;
{
	register int unit = minor(dev);
	struct	mb_device *mb_dev = *(mb_devs+unit);

	if (unit >= numdevs || mb_dev == 0 || mb_dev->md_alive == 0)
		return (ENXIO);
	return (0);
}

/*
 * Call intclear to turn off interrupts on all alive devices.
 * If intclear returns non-zero value then know that found an interrupting
 * device.
 */
int
fbintr(numdevs, mb_devs, intclear)
	int	numdevs;
	struct	mb_device **mb_devs;
	int	(*intclear)();
{
	register int i;
	register struct mb_device *md;

	for (i = 0; i < numdevs; i++) {
		if ((md = *(mb_devs+i)) == NULL)
			continue;
		if (!md->md_alive)
			continue;
		if ((*intclear)(md->md_addr))
			return (1);
	}
	return (0);
}

/*ARGSUSED*/
int
fbmmap(dev, off, prot, numdevs, mb_devs, size)
	dev_t	dev;
	off_t	off;
	int	prot;
	int	numdevs;
	struct	mb_device **mb_devs;
	int	size;
{
	struct	mb_device *mb_dev = *(mb_devs+minor(dev));
	register int page;

	if (off >= size)
		return (-1);
	page = getkpgmap(mb_dev->md_addr + off) & PG_PFNUM;
	return (page);
}
