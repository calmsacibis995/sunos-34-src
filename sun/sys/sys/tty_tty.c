/*	@(#)tty_tty.c 1.1 86/09/25 SMI; from UCB 4.14 82/12/05	*/

/*
 * Indirect driver for controlling tty.
 *
 * THIS IS GARBAGE: MUST SOON BE DONE WITH struct vnode * IN PROC TABLE.
 */
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/conf.h"
#include "../h/user.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/proc.h"
#include "../h/uio.h"

/*ARGSUSED*/
syopen(dev, flag)
	dev_t dev;
	int flag;
{
	register struct tty *tp;
	register dev_t ttydev;

	if ((tp = u.u_ttyp) == NULL)
		return (ENXIO);
	ttydev = u.u_ttyd;
	/*
	 * If line is active for dial-out, we can only
	 * be running on the dial-out side.  We must make
	 * sure that open(/dev/tty) resolves to open
	 * on the dial-out side of the tty port; otherwise
	 * the open will hang waiting for carrier on the
	 * dial-in side of the line
	 */
#define	OUTLINE	0x80
	if (tp->t_state & TS_OUT)
		ttydev |= OUTLINE;
	return ((*cdevsw[major(u.u_ttyd)].d_open)(ttydev, flag));
}

/*ARGSUSED*/
syread(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	if (u.u_ttyp == NULL)
		return (ENXIO);
	return ((*cdevsw[major(u.u_ttyd)].d_read)(u.u_ttyd, uio));
}

/*ARGSUSED*/
sywrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	if (u.u_ttyp == NULL)
		return (ENXIO);
	return ((*cdevsw[major(u.u_ttyd)].d_write)(u.u_ttyd, uio));
}

/*ARGSUSED*/
syioctl(dev, cmd, addr, flag)
	dev_t dev;
	int cmd;
	caddr_t addr;
	int flag;
{

	if (cmd == TIOCNOTTY) {
		u.u_ttyp = 0;
		u.u_ttyd = 0;
		u.u_procp->p_pgrp = 0;
		return (0);
	}
	if (u.u_ttyp == NULL)
		return (ENXIO);
	return ((*cdevsw[major(u.u_ttyd)].d_ioctl)(u.u_ttyd, cmd, addr, flag));
}

/*ARGSUSED*/
syselect(dev, flag)
	dev_t dev;
	int flag;
{

	if (u.u_ttyp == NULL) {
		u.u_error = ENXIO;
		return (0);
	}
	return ((*cdevsw[major(u.u_ttyd)].d_select)(u.u_ttyd, flag));
}
