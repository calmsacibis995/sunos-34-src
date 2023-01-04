#ifndef lint
static	char sccsid[] = "@(#)consfb.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Console frame buffer driver for Sun.
 *
 * Indirects to fbdev found during autoconf.
 */

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/conf.h"
#include "../h/user.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/systm.h"
#include "../h/kernel.h"
#include "../h/buf.h"
#include "../h/vmmac.h"
#include "../h/uio.h"
#include "../h/proc.h"

dev_t	fbdev = -1;

/* 
 * In case /dev/fb is indirected to a gp device (or any device that
 * may use the pseudo-minor device opening hack), we have to deal
 * specially with the minor device number.
 */

/*ARGSUSED*/
consfbopen(dev, flag, trueminor)
	dev_t dev;
	int *trueminor;
{
        register struct cdevsw *dp;

	if (fbdev == -1)
		return (ENXIO);
	dp = &cdevsw[major(fbdev)];
        return ((*dp->d_open)(fbdev, flag, trueminor));
}

/*ARGSUSED*/
consfbclose(dev, flag)
	dev_t dev;
{

	if (fbdev != -1)
		return ((*cdevsw[major(fbdev)].d_close)
			( (major(fbdev)<<8) | minor(dev), flag));
	return (-1);
}

/*ARGSUSED*/
consfbioctl(dev, cmd, data, flag)
	dev_t dev;
	caddr_t data;
{

	if (fbdev != -1)
                return ((*cdevsw[major(fbdev)].d_ioctl)
			( (major(fbdev)<<8) | minor(dev), cmd, data, flag));
	return (ENOTTY);
}

/*ARGSUSED*/
consfbmmap(dev, off, prot)
	dev_t dev;
	off_t off;
	int prot;
{
	if (fbdev != -1)
                return ((*cdevsw[major(fbdev)].d_mmap)
			( (major(fbdev)<<8) | minor(dev), off, prot));
	return (-1);
}
