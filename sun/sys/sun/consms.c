#ifndef lint
static	char sccsid[] = "@(#)consms.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Console mouse driver for Sun.
 *
 * Indirects to mousedev found during autoconf.
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

dev_t	mousedev = -1;

/*ARGSUSED*/
consmsopen(dev, flag)
	dev_t dev;
{
        int err;
        int ldisc = MOUSELDISC;
        register struct cdevsw *dp;

	if (mousedev == -1)
		return (ENXIO);
	dp = &cdevsw[major(mousedev)];
        if (err = (*dp->d_open)(mousedev, flag))
                return (err);
	if (err = (*dp->d_ioctl) (mousedev, TIOCSETD, (caddr_t)&ldisc, flag))
                return (err);
	cdevsw[major(dev)].d_ttys =
	    cdevsw[major(mousedev)].d_ttys+minor(mousedev);
	return(0);
}

/*ARGSUSED*/
consmsclose(dev, flag)
	dev_t dev;
{
	if (mousedev != -1) {
		(*cdevsw[major(mousedev)].d_close)(mousedev, flag);
		cdevsw[major(dev)].d_ttys = 0;
	}
}

/*ARGSUSED*/
consmsread(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	if (mousedev != -1)
		return ((*cdevsw[major(mousedev)].d_read)(mousedev, uio));
	return (ENXIO);
}

/*ARGSUSED*/
consmsselect(dev, rw)
	dev_t dev;
	int rw;
{

	if (mousedev != -1)
		return ((*cdevsw[major(mousedev)].d_select)(mousedev, rw));
	return (ENXIO);
}

/*ARGSUSED*/
consmsioctl(dev, cmd, data, flag)
	dev_t dev;
	caddr_t data;
{

	if (mousedev != -1)
		return ((*cdevsw[major(mousedev)].d_ioctl)
		    (mousedev, cmd, data, flag));
	return (ENOTTY);
}
