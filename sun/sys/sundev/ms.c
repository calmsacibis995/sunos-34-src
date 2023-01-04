#ifndef lint
static	char sccsid[] = "@(#)ms.c 1.1 86/09/25";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Mouse line discipline
 */

#include "ms.h"
#include "../h/param.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/ioctl.h"
#include "../h/buf.h"
#include "../h/systm.h"
#include "../h/mman.h"
#include "../h/uio.h"
#include "../h/tty.h"
#include "../h/vmmac.h"
#include "../h/kernel.h"

#include "../machine/pte.h"
#include "../machine/mmu.h"
#include "../machine/scb.h"
#include "../sundev/vuid_event.h"
#include "../sundev/msreg.h"
#include "../sundev/msio.h"


/*
 * Mouse select management is done by utilizing the tty mechanism.
 * We place a single character on the tty raw input queue whenever
 * there is some amount of mouse data available to be read.  Once,
 * all the data has been read, the tty raw input queue is flushed.
 *
 * Note: It is done in order to get around the fact that line
 * disiplines don't have select operations because they are always
 * expected to be ttys that stuff characters when they get them onto
 * a queue.
 *
 * Note: We use spl5 for the mouse because it is functionally the
 * same as spl6 and the tty mechanism is using spl5.  The original
 * code that was doing its own select processing was using spl6.
 */
#define	spl_ms	spl5

struct msdata {
    struct ms_softc msd_softc;
    struct tty     *msd_tp;
    short           msd_xerox;
    char            msd_oldbutt;
    short           msd_xnext;
    short           msd_jitter;
};
struct msdata   msdata[NMS];
struct msdata  *mstptomsd();

int             ms_overrun_msg;	/* Message when overrun circular buffer */
int             ms_overrun_cnt;	/* Increment when overrun circular buffer */

/*
 * Max pixel delta of jitter controlled. As this number increases the jumpiness
 * of the ms increases, i.e., the coarser the motion for medium speeds. 
 */
int             ms_jitter_thresh = 1;

/*
 * ms_jitter_thresh is the maximum number of jitters suppressed. Thus,
 * hz/ms_jitter_thresh is the maximum interval of jitters suppressed. As
 * ms_jitter_thresh increases, a wider range of jitter is suppressed. However,
 * the more inertia the mouse seems to have, i.e., the slower the mouse is to
 * react. 
 */

/*
 * Measure how many (ms_speed_count) ms deltas exceed threshold
 * (ms_speedlimit). If ms_speedlaw then throw away deltas over ms_speedlimit.
 * This is to keep really bad mice that jump around from getting too far. 
 */
int             ms_speedlimit = 48;
int             ms_speedlaw = 1;
int             ms_speed_count;
int             msjitterrate = 12;

#define	JITTER_TIMEOUT (hz/msjitterrate)

int             msjittertimeout;/* Timeout used when mstimeout in affect */
/*
 * Mouse buffer size in bytes.  Place here as variable so that one could
 * massage it using adb if it turns out to be too small. 
 */
int             MS_BUF_BYTES = 4096;

int             MS_DEBUG;

/*
 * Mouse line discipline 
 */

/*
 * Open a mouse. Calls sets mouse line characteristics 
 */
/* ARGSUSED */
msopen(dev, tp)
    dev_t           dev;
    struct tty     *tp;
{
    register int    err, i;
    struct sgttyb   sg;
    register struct mousebuf *b;
    register struct ms_softc *ms;
    register struct msdata *msd;
    caddr_t         zmemall();
    register struct cdevsw *dp;

    /*
     * See if tp is being used to drive ms already. 
     */
    for (i = 0; i < NMS; ++i)
	if (msdata[i].msd_tp == tp)
	    return (0);
    /*
     * Get next free msdata. 
     */
    for (i = 0; i < NMS; ++i)
	if (msdata[i].msd_tp == 0)
	    goto found;
    return (EBUSY);
found:
    /*
     * Open tty. 
     */
    if (err = ttyopen(dev, tp))
	return (err);
    /*
     * Setup tty flags 
     */
    dp = &cdevsw[major(dev)];
    if (err = (*dp->d_ioctl) (dev, TIOCGETP, (caddr_t) & sg, 0))
	goto error;
    sg.sg_flags = RAW + ANYP;
    sg.sg_ispeed = sg.sg_ospeed = B1200;
    if (err = (*dp->d_ioctl) (dev, TIOCSETP, (caddr_t) & sg, 0))
	goto error;
    /*
     * Set up private data. 
     */
    msd = &msdata[i];
    msd->msd_xnext = 1;
    msd->msd_tp = tp;
    ms = &msd->msd_softc;
    /*
     * Allocate buffer and initialize data. 
     */
    if (ms->ms_buf == 0) {
	ms->ms_bufbytes = MS_BUF_BYTES;
	b = (struct mousebuf *) zmemall(memall, ms->ms_bufbytes);
	if (b == 0) {
	    err = EINVAL;
	    goto error;
	}
	b->mb_size = 1 + (ms->ms_bufbytes - sizeof(struct mousebuf))
	    / sizeof(struct mouseinfo);
	ms->ms_buf = b;
	ms->ms_vuidaddr = VKEY_FIRST;
	msjittertimeout = JITTER_TIMEOUT;
	msflush(msd);
    }
    return (0);
error:
    bzero((caddr_t) msd, sizeof(*msd));
    bzero((caddr_t) ms, sizeof(*ms));
    return (err);
}

/*
 * Close the mouse 
 */
msclose(tp)
    struct tty     *tp;
{
    register struct msdata *msd = mstptomsd(tp);
    register struct ms_softc *ms;

    if (msd == 0)
	return;
    ms = &msd->msd_softc;
    /* Free mouse buffer */
    if (ms->ms_buf != NULL)
	wmemfree((caddr_t) ms->ms_buf, ms->ms_bufbytes);
    /* Close tty */
    ttyclose(tp);
    /* Zero structures */
    bzero((caddr_t) msd, sizeof(*msd));
    bzero((caddr_t) ms, sizeof(*ms));
}

/*
 * Read from the mouse buffer 
 */
msread(tp, uio)
    struct tty     *tp;
    struct uio     *uio;
{
    register struct msdata *msd = mstptomsd(tp);
    register struct ms_softc *ms;
    register struct mousebuf *b;
    register struct mouseinfo *mi;
    register int    error = 0, pri, send_event, hwbit;
    register char   c;
    Firm_event      fe;

    if (msd == 0)
	return (EINVAL);
    ms = &msd->msd_softc;
    b = ms->ms_buf;
    pri = spl_ms();
    /*
     * Wait on tty raw queue if this queue is empty since the tty is
     * controlling the select/wakeup/sleep stuff. 
     */
    while (tp->t_rawq.c_cc <= 0) {
	if (tp->t_state & TS_NBIO) {
	    (void) splx(pri);
	    return (EWOULDBLOCK);
	}
	(void) sleep((caddr_t) & tp->t_rawq, TTIPRI);
    }
    while (!error && (ms->ms_oldoff1 || ms->ms_oldoff != b->mb_off)) {
	mi = &b->mb_info[ms->ms_oldoff];
	switch (ms->ms_readformat) {

	  case MS_3BYTE_FORMAT:
	    if (uio->uio_resid == 0)
		goto done;
	    switch (ms->ms_oldoff1++) {

	      case 0:
		c = 0x80 | mi->mi_buttons;
		/* Update read buttons */
		ms->ms_readbuttons = mi->mi_buttons;
		break;

	      case 1:
		c = mi->mi_x;
		break;

	      case 2:
		c = -mi->mi_y;
		ms->ms_oldoff++;
		if (ms->ms_oldoff >= b->mb_size)
		    ms->ms_oldoff = 0;
		ms->ms_oldoff1 = 0;
		break;
	    }
	    /* lower pri to avoid mouse droppings */
	    (void) splx(pri);
	    error = ureadc(c, uio);
	    pri = spl_ms();
	    break;

	  case MS_VUID_FORMAT:
	    if (uio->uio_resid < sizeof(Firm_event))
		goto done;
	    send_event = 0;
	    switch (ms->ms_oldoff1++) {

	      case 0:		/* Send x if changed */
		if (mi->mi_x != 0) {
		    fe.id = vuid_id_addr(ms->ms_vuidaddr) | vuid_id_offset(LOC_X_DELTA);
		    fe.pair_type = FE_PAIR_ABSOLUTE;
		    fe.pair = LOC_X_ABSOLUTE;
		    fe.value = mi->mi_x;
		    send_event = 1;
		}
		break;

	      case 1:		/* Send y if changed */
		if (mi->mi_y != 0) {
		    fe.id = vuid_id_addr(ms->ms_vuidaddr) | vuid_id_offset(LOC_Y_DELTA);
		    fe.pair_type = FE_PAIR_ABSOLUTE;
		    fe.pair = LOC_Y_ABSOLUTE;
		    fe.value = -mi->mi_y;
		    send_event = 1;
		}
		break;

	      default:		/* Send buttons if changed */
		hwbit = MS_HW_BUT1 >> (ms->ms_oldoff1 - 3);
		if ((ms->ms_readbuttons & hwbit) != (mi->mi_buttons & hwbit)) {
		    fe.id = vuid_id_addr(ms->ms_vuidaddr) |
			vuid_id_offset(BUT(1) + (ms->ms_oldoff1 - 3));
		    fe.pair_type = FE_PAIR_NONE;
		    fe.pair = 0;
		    /* Update read buttons and set value */
		    if (mi->mi_buttons & hwbit) {
			fe.value = 0;
			ms->ms_readbuttons |= hwbit;
		    } else {
			fe.value = 1;
			ms->ms_readbuttons &= ~hwbit;
		    }
		    send_event = 1;
		}
		/* Increment mouse buffer pointer */
		if (ms->ms_oldoff1 == 5) {
		    ms->ms_oldoff++;
		    if (ms->ms_oldoff >= b->mb_size)
			ms->ms_oldoff = 0;
		    ms->ms_oldoff1 = 0;
		}
		break;

	    }
	    if (send_event) {
		fe.time = mi->mi_time;
		ms->ms_vuidcount--;
		/* lower pri to avoid mouse droppings */
		(void) splx(pri);
		error = uiomove((caddr_t) & fe, sizeof(fe),
				UIO_READ, uio);
		/* spl_ms should return same priority as pri */
		pri = spl_ms();
	    }
	    break;

	}
    }
done:
    /* Flush tty if no more to read */
    if ((ms->ms_oldoff1 == 0) && (ms->ms_oldoff == b->mb_off))
	ttyflush(tp, FREAD);
    /* Release protection AFTER ttyflush or will get out of sync with tty */
    (void) splx(pri);
    return (0);
}

/*
 * Mouse ioctl 
 */
msioctl(tp, cmd, data, flag)
    struct tty     *tp;
    int             cmd;
    caddr_t         data;
    int             flag;
{
    register struct msdata *msd = mstptomsd(tp);
    register struct ms_softc *ms;
    int             err = 0;
    Vuid_addr_probe *addr_probe;

    if (MS_DEBUG)
	printf("ms_ioctl(%X,%X,%X,%X)\n", tp, cmd, data, flag);
    if (msd == 0)
	return (EINVAL);
    ms = &msd->msd_softc;
    switch (cmd) {
      case FIONREAD:
	switch (ms->ms_readformat) {
	  case MS_3BYTE_FORMAT:
	    *(int *) data = ms->ms_samplecount;
	    break;

	  case MS_VUID_FORMAT:
	    *(int *) data = sizeof(Firm_event) * ms->ms_vuidcount;
	    break;
	}
	break;

      case VUIDSFORMAT:
	if (*(int *) data == ms->ms_readformat)
	    break;
	ms->ms_readformat = *(int *) data;
	/*
	 * Flush mouse buffer because otherwise ms_*counts get out of sync and
	 * some of the offsets can too. 
	 */
	msflush(msd);
	break;

      case VUIDGFORMAT:
	*(int *) data = ms->ms_readformat;
	break;

      case VUIDSADDR:
	addr_probe = (Vuid_addr_probe *) data;
	if (addr_probe->base != VKEY_FIRST) {
	    err = ENODEV;
	    break;
	}
	ms->ms_vuidaddr = addr_probe->data.next;
	break;

      case VUIDGADDR:
	addr_probe = (Vuid_addr_probe *) data;
	if (addr_probe->base != VKEY_FIRST) {
	    err = ENODEV;
	    break;
	}
	addr_probe->data.current = ms->ms_vuidaddr;
	break;

      case TIOCSETD:
	/*
	 * Don't let the line discipline change once it has been set to a
	 * mouse.  Changing the ldisc causes msclose to be called even if the
	 * ldisc of the tp is the same. We can't let this happen because the
	 * window system may have a handle on the mouse buffer. The basic
	 * problem is one of having anything depending on the continued
	 * existence of ldisc related data. The fix is to have: 1) a way of
	 * handing data to the dependent entity, and 2) notifying the dependent
	 * entity that the ldisc has been closed. 
	 */
	break;

      case MSIOGETBUF:
	*(int *) data = (int) ms->ms_buf;
	break;

      case MSIOGETPARMS:
	if (MS_DEBUG)
	    printf("ms_getparms\n");
    	err = ms_getparms((Ms_parms *)data);
	break;

      case MSIOSETPARMS:
	if (MS_DEBUG)
	    printf("ms_setparms\n");
	err = ms_setparms((Ms_parms *)data);
	break;

      default:
	err = ttioctl(tp, cmd, data, flag);
    }
    return (err);
}

ms_getparms(data)
    register Ms_parms	*data;
{
    data->jitter_thresh = ms_jitter_thresh;
    data->speed_law = ms_speedlaw;
    data->speed_limit = ms_speedlimit;
    return 0;
}

ms_setparms(data)
    register Ms_parms	*data;
{
    ms_jitter_thresh = data->jitter_thresh;
    ms_speedlaw = data->speed_law;
    ms_speedlimit = data->speed_limit;
    return 0;
}

msflush(msd)
    register struct msdata *msd;
{
    register struct ms_softc *ms = &msd->msd_softc;
    int             s = spl_ms();

    ms->ms_oldoff = 0;
    ms->ms_oldoff1 = 0;
    ms->ms_buf->mb_off = 0;
    ms->ms_vuidcount = 0;
    ms->ms_samplecount = 0;
    ms->ms_readbuttons = MS_HW_BUT1 | MS_HW_BUT2 | MS_HW_BUT3;
    msd->msd_oldbutt = ms->ms_readbuttons;
    ttyflush(msd->msd_tp, FREAD);
    (void) splx(s);
}

/*
 * Mouse input - called for each byte of mouse data Handles both XEROX mice and
 * Mouse Systems Mice. Xerox Mice have button byte 0x88; Mouse Systems Mice
 * 0x80, both with button values in low 3 bits. Conveniently, the MSC mouse (as
 * of serial #2327) never generates deltas which conflict with the Xerox button
 * byte 0x88; i.e., both mice restrict deltas to [-112:127]. We view the
 * protocol as a sequence of delta-x, delta-y pairs with interspersed button
 * bytes; a delta-y byte causes a new sample to be recorded with the
 * immediately preceeding delta-x and the most recent button byte.  This is
 * necessary because the MSC serial mouse sends two x,y samples between each
 * button sample, but the parallel mouse and the Xerox mouse have only one pair
 * per button sample. 
 */
/* ARGSUSED */
msinput(c, tp)
    register char   c;
    struct tty     *tp;
{
    register struct msdata *msd = mstptomsd(tp);
    register struct ms_softc *ms;
    register struct mousebuf *b;
    register struct mouseinfo *mi;
    register int    jitter_radius;
    register int    but_dif;
    int             msincr();

    if (msd == 0)
	return;
    ms = &msd->msd_softc;
    b = ms->ms_buf;
    if (b == NULL)
	return;
    mi = &b->mb_info[b->mb_off];
    /*
     * The byte protocol sends 5 bytes whenever something changes at the
     * mouse:  1: delta x  2: delta y  3: 10000LMR  4: delta x  5: delta y 
     */
    if ((c & 0xf0) == 0x80) {
	mi->mi_buttons = c&7;
#ifndef IGNORE_XEROX
	msd->msd_xerox = c & 8;
#endif
	msd->msd_xnext = 1;
	return;
    }
    if (msd->msd_xnext++ & 1) {
	mi->mi_x += c;
	uniqtime(&mi->mi_time);
	return;
    }
    mi->mi_y -= c;
    /* y completes the sample */

    if (msd->msd_jitter) {
	untimeout(msincr, (caddr_t) msd);
	msd->msd_jitter = 0;
    }
    if (mi->mi_buttons == msd->msd_oldbutt) {
	if (mi->mi_x == 0 && mi->mi_y == 0) {
	    return;
	}
	jitter_radius = ms_jitter_thresh;
#ifndef IGNORE_XEROX
	if (msd->msd_xerox)
	    /*
	     * Account for double resolution of xerox mouse. 
	     */
	    jitter_radius *= 2;
#endif
	if (mi->mi_x <= jitter_radius &&
	    mi->mi_x >= -jitter_radius &&
	    mi->mi_y <= jitter_radius &&
	    mi->mi_y >= -jitter_radius) {
	    msd->msd_jitter = 1;
	    timeout(msincr, (caddr_t) msd, msjittertimeout);
	    return;
	}
    } else {
	/* Update vuid event count for buttons */
	but_dif = msd->msd_oldbutt ^ mi->mi_buttons;
	if (but_dif & MS_HW_BUT1)
	    ms->ms_vuidcount++;
	if (but_dif & MS_HW_BUT2)
	    ms->ms_vuidcount++;
	if (but_dif & MS_HW_BUT3)
	    ms->ms_vuidcount++;
    }
    msd->msd_oldbutt = mi->mi_buttons;
    msincr(msd);
}

/*
 * Increment the mouse sample pointer Called either immediately after a sample
 * or after a jitter timeout 
 */
msincr(msd)
    struct msdata  *msd;
{
    register struct ms_softc *ms = &msd->msd_softc;
    register struct mousebuf *b = ms->ms_buf;
    register struct mouseinfo *mi;
    char            oldbutt;
    register short  xc, yc;
    register int    wake;
    register int    speedlimit = ms_speedlimit;
    int             s;

    if (b == NULL)
	return;
    s = spl_ms();
    mi = &b->mb_info[b->mb_off];
    if (ms_speedlaw) {
#ifndef IGNORE_XEROX
	if (msd->msd_xerox)
	    /*
	     * Account for double resolution of xerox mouse. 
	     */
	    speedlimit *= 2;
#endif
	if (mi->mi_x > speedlimit || mi->mi_x < -speedlimit ||
	    mi->mi_y > speedlimit || mi->mi_y < -speedlimit)
	    ms_speed_count++;
	if (mi->mi_x > speedlimit || mi->mi_x < -speedlimit)
	    mi->mi_x = 0;
	if (mi->mi_y > speedlimit || mi->mi_y < -speedlimit)
	    mi->mi_y = 0;
    }
    oldbutt = mi->mi_buttons;
#ifndef IGNORE_XEROX
    /*
     * XEROX mice are 200/inch; scale to 100/inch. 
     */
    if (msd->msd_xerox) {
	/*
	 * Xc and yc carry over fractional part. You might think that we have
	 * to worry about mi->mi_[xy] being negative here, but remember that
	 * using shift to divide always leaves a positive remainder! 
	 */
	xc = mi->mi_x & 1;
	yc = mi->mi_y & 1;
	mi->mi_x >>= 1;
	mi->mi_y >>= 1;
    } else
#endif
	xc = yc = 0;
    /* Update sample and event counts */
    ms->ms_samplecount++;
    if (mi->mi_x)
	ms->ms_vuidcount++;
    if (mi->mi_y)
	ms->ms_vuidcount++;
    /* See if need to wake up anyone waiting for input */
    wake = b->mb_off == ms->ms_oldoff;
    /* Adjust circular buffer pointer */
    if (++b->mb_off >= b->mb_size) {
	b->mb_off = 0;
	mi = b->mb_info;
    } else {
	mi++;
    }
    /*
     * If over-took read index then flush buffer so that mouse state (e.g.,
     * vuidcount) is consistent. 
     */
    if (b->mb_off == ms->ms_oldoff) {
	if (ms_overrun_msg)
	    printf("Mouse buffer flushed when overrun.\n");
	msflush(msd);
	ms_overrun_cnt++;
	mi = b->mb_info;
    }
    /* Remember current buttons and fractional part of x & y */
    mi->mi_buttons = oldbutt;
    mi->mi_x = xc;
    mi->mi_y = yc;
    if (wake)
	/* Place character on tty raw input queue to trigger select */
	ttyinput('\0', msd->msd_tp);
    (void) splx(s);
}

/*
 * Match tp to msdata. 
 */
struct msdata  *
mstptomsd(tp)
    struct tty     *tp;
{
    register        i;

    /*
     * Get next free msdata. 
     */
    for (i = 0; i < NMS; ++i)
	if (msdata[i].msd_tp == tp)
	    return (&msdata[i]);
    printf("mstptomsd called with unknown tp %X\n", tp);
    return (0);
}
