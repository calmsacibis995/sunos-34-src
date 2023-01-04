#ifndef lint
static	char sccsid[] = "@(#)conskbd.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Console kbd driver for Sun.
 *
 * Indirects to kbddev found during autoconf.
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

dev_t	kbddev = -1;
int	kbddevopen = 0;

/*ARGSUSED*/
conskbdopen(dev, flag)
	dev_t dev;
{
        int err;
        int ldisc = KBDLDISC;
        register struct cdevsw *dp;

	if (kbddev == -1)
		return (ENXIO);
	dp = &cdevsw[major(kbddev)];
        if (err = (*dp->d_open)(kbddev, flag))
                return (err);
	if (err = (*dp->d_ioctl) (kbddev, TIOCSETD, (caddr_t)&ldisc, flag))
                return (err);
	cdevsw[major(dev)].d_ttys = cdevsw[major(kbddev)].d_ttys+minor(kbddev);
	return (0);
}

/*ARGSUSED*/
conskbdclose(dev, flag)
	dev_t dev;
{

	if (kbddev != -1) {
		(*cdevsw[major(kbddev)].d_close)(kbddev, flag);
		cdevsw[major(dev)].d_ttys = 0;
	}
}

/*ARGSUSED*/
conskbdread(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	if (kbddev != -1)
		return ((*cdevsw[major(kbddev)].d_read)(kbddev, uio));
	return (ENXIO);
}

/*ARGSUSED*/
conskbdselect(dev, rw)
	dev_t dev;
	int rw;
{

	if (kbddev != -1)
		return ((*cdevsw[major(kbddev)].d_select)(kbddev, rw));
	return (ENXIO);
}

/*ARGSUSED*/
conskbdioctl(dev, cmd, data, flag)
	dev_t dev;
	caddr_t data;
{

	if (kbddev != -1)
		return ((*cdevsw[major(kbddev)].d_ioctl)
		    (kbddev, cmd, data, flag));
	return (ENOTTY);
}
