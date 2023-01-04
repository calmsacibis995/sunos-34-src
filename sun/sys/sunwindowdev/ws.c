#ifndef lint
static	char sccsid[] = "@(#)ws.c 1.4 87/01/09";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * SunWindows Workstation/Wsindev open and close code.  Includes tuning
 * parameters.
 */

#include "../sunwindowdev/wintree.h"
#include "../h/errno.h"
#include "../h/file.h"		/* For FREAD f_data */
#include "../h/vnode.h"		/* For v_rdev */
#include "../sundev/kbd.h"	/* For struct keyboard */
#include "../sundev/kbdreg.h"	/* For struct keyboardstate */
#include "../sundev/kbio.h"	/* For KIOCS/GUSCODE */

#define	WS_VQ_NODE_BYTES	8192
u_int	ws_vq_node_bytes = WS_VQ_NODE_BYTES;
	    /* Number of bytes to use for input q nodes */
int	ws_vq_expand_times = 6;
	    /* Number of times can expand q by ws_vq_node_bytes */
#define	WS_QUEUE_COLLAPSE_FACTOR 0	/* Try to collapse q to 1/nth */
					/* Zero mean collapse all the time */
int	ws_q_collapse_factor = WS_QUEUE_COLLAPSE_FACTOR;

extern	void ws_break_handler();
extern	void ws_stop_handler();
extern	void ws_flush_handler();
Ws_usr_async ws_break_default =
	    {0, SHIFT_TOP, 1, TOP_FIRST + 'i', 1, ws_break_handler};
Ws_usr_async ws_stop_default =
	    {0, SHIFT_TOP, 1, SHIFT_TOP, 0, ws_stop_handler};
Ws_usr_async ws_flush_default =
	    {0, SHIFT_TOP, 1, TOP_FIRST + 'f', 1, ws_flush_handler};

Ws_focus_set ws_kbd_focus_pt_default = {LOC_WINENTER, 1, 0};
Ws_focus_set ws_kbd_focus_sw_default = {LOC_WINENTER, 1, 0};
struct	timeval ws_event_timeout_default = {2, 0};
	/* 2 secs give micro synchronization, thus collapsing when tracking */

/*
 * Tuning parameters that are set if ws_tuning_done is 0 when start SunWindows.
 */
int	ws_tuning_done;		/* Controls whether the following should be
				   initialized. */
int	ws_fast_timeout;	/* Fast polling rate in hz */
int	ws_slow_timeout;	/* Slow polling rate in hz */
int	ws_fast_poll_duration;	/* Stop fast polling after this # hz */
int	ws_loc_still;		/* Locator is still after this number of hz */
int	ws_check_lock;		/* Check locks every this # of hz if pid */
int	ws_no_pid_check_lock;	/* Check locks every this # of hz if no pid */
struct	timeval ws_check_time;	/* Check locks every this # usec */
int	winclistcharsmax;	/* Limit the amount of clist space can use */
int	ws_q_collapse_trigger;	/* # q items that trigger collapse */

ws_init_tuning()
{
#define	WS_FAST_TIMEOUT		(hz/40)	/* Fast polling rate */
#define WS_SLOW_TIMEOUT		(hz/8)	/* Slow polling rate */
#define WS_FAST_POLL_DURATION	(hz/5)	/* Stop fast polling after this # hz */
#define WS_MOUSE_STILL		(hz/5)	/* Locator is still after this # of hz*/
#define WS_CHECK_LOCK		(hz/2)	/* Check locks every #of hz if pid */
#define WS_NO_PID_CHECK_LOCK	(hz*20)	/* Check locks every #of hz if no pid */

	if (!ws_tuning_done) {
		extern int hz;

		/* Init things can't do statically */
		if (!ws_fast_timeout)
			ws_fast_timeout = WS_FAST_TIMEOUT;
		if (!ws_slow_timeout)
			ws_slow_timeout = WS_SLOW_TIMEOUT;
		if (!ws_fast_poll_duration)
			ws_fast_poll_duration = WS_FAST_POLL_DURATION;
		if (!ws_loc_still)
			ws_loc_still = WS_MOUSE_STILL;
		if (!ws_check_lock)
			ws_check_lock = WS_CHECK_LOCK;
		if (!ws_no_pid_check_lock)
			ws_no_pid_check_lock = WS_NO_PID_CHECK_LOCK;
		if (!ws_check_time.tv_usec)
			ws_check_time.tv_usec = 1000000 / hz * ws_check_lock;
		if (!winclistcharsmax)
			/* Use half of all clist space */
			winclistcharsmax = nclist * CBSIZE / 2;
		if ((!ws_q_collapse_trigger) && ws_q_collapse_factor)
			ws_q_collapse_trigger =
			    (ws_vq_node_bytes / sizeof (Vuid_q_node)) /
			    ws_q_collapse_factor;
		ws_tuning_done = 1;
	}
}
	
/* Find workstation using input_device_name */
Workstation *
ws_indev_match_name(input_device_name, indev_ptr)
	char *input_device_name;
	Wsindev **indev_ptr;
{
	register Workstation *ws;
	register Wsindev *indev;

	if (input_device_name[0] == '\0')
		return (WORKSTATION_NULL);
	for (ws = workstations; ws < &workstations[NWORKSTATION]; ws++) {
		for (indev = ws->ws_indev; indev != WSINDEV_NULL;
		    indev = indev->wsid_next) {
			if (indev->wsid_name[0] != '\0' &&
			    (strncmp(input_device_name,
			    indev->wsid_name, SCR_NAMESIZE) == 0)) {
				if (indev_ptr)
					*indev_ptr = indev;
				return (ws);
			}
		}
	}
	return (WORKSTATION_NULL);
}
	
/* Find workstation using dev */
Workstation *
ws_indev_match_dev(dev)
	dev_t dev;
{
	register Workstation *ws;
	register Wsindev *indev;

	for (ws = workstations; ws < &workstations[NWORKSTATION]; ws++) {
		for (indev = ws->ws_indev; indev != WSINDEV_NULL;
		    indev = indev->wsid_next) {
			if (indev->wsid_dev == dev)
				return (ws);
		}
	}
	return (WORKSTATION_NULL);
}

/*
 * A workstation is opened by a window that trying to establish itself
 * as the root window of a first desktop at a workstation.  The list of
 * desktops is searched for a conflict of names of framebuffers to avoid
 * blitzing and existing screen.  The list of workstations is searched for
 * a conflict of names of input devices that the window has opened for
 * itself.
 */
Workstation *
ws_open()
{
	register Workstation *ws = WORKSTATION_NULL;
	register Workstation *ws_probe = WORKSTATION_NULL;
	caddr_t zmemall();
	int memall();
	extern	int ws_poll_rate;

	/* Initialize tuning parameters */
	ws_init_tuning();
	/* Allocate an unused workstation */
	for (ws_probe = workstations; ws_probe < &workstations[NWORKSTATION];
	    ws_probe++) {
		if (!(ws_probe->ws_flags & WSF_PRESENT)) {
			ws = ws_probe;
			break;
		}
	}
	if ((ws == WORKSTATION_NULL) || (ws->ws_dtop != DESKTOP_NULL)) {
		win_errno = EBUSY;
		return (WORKSTATION_NULL);
	}
	/* Allocate the input q */
	ws->ws_qbytes = ws_vq_node_bytes;
	ws->ws_qdata = zmemall(memall, (int)ws->ws_qbytes);
	if (ws->ws_qdata == NULL) {
		printf("Couldn't allocate %D byte event buffer bytes\n",
		    ws->ws_qbytes);
		win_errno = ENOMEM;
		return (WORKSTATION_NULL);
	}
	vq_initialize(&ws->ws_q, ws->ws_qdata, ws->ws_qbytes);
	/* Allocate lock static data */
	wlok_init(&ws->ws_eventlock);
	wlok_init(&ws->ws_iolock);
	/* Make present so that need ws_close to clean up after this point */
	ws->ws_flags |= WSF_PRESENT;
	/* Initialize input queue synchronization parameters */
	ws->ws_break = ws_break_default;
	ws->ws_stop = ws_stop_default;
	ws->ws_eventtimeout = ws_event_timeout_default;
	/* Initialize kbd focus changing events */
	ws->ws_kbd_focus_pt = ws_kbd_focus_pt_default;
	ws->ws_kbd_focus_sw = ws_kbd_focus_sw_default;
	/*
	 * Start device driver looking for input and doing deadlock
	 * resolution if haven't started yet.
	 */
	if (ws_poll_rate == 0) {
		int ws_interrupt();

		ws_poll_rate = ws_fast_timeout;
		timeout(ws_interrupt, (caddr_t)0, ws_poll_rate);
	}
	return (ws);
}

/*
 * A workstation is told by its last desktop to close, as is a desktop
 * told by its last window to close.
 */
ws_close(ws)
	register Workstation *ws;
{
	/*
	 * ws_close_indev can sleep (see ws_close_indev) which can
	 * allow ws_interrupt to be called as a timeout.  Make this
	 * workstation invisible to ws_interrupt now.
	 */
	ws->ws_flags |= WSF_EXITING;
	/* Close input devices used by this workstation */
	while (ws->ws_indev != WSINDEV_NULL)
		/* ws_close_indev deletes ws_indev from list */
		if (ws_close_indev(ws, ws->ws_indev) == -1)
			break;
	/* All desktops should be removed by now */
	if (ws->ws_dtop != DESKTOP_NULL)
		printf("ws_close: desktop still opened\n");
	/* Deallocate input queue */
	if (ws->ws_qdata != NULL)
		wmemfree(ws->ws_qdata, (int)ws->ws_qbytes);
	/* Deallocate input state descriptions */
	vuid_destroy_state(ws->ws_instate);
	vuid_destroy_state(ws->ws_rtstate);
	/* Release lock static data */
	wlok_done(&ws->ws_eventlock);
	wlok_done(&ws->ws_iolock);
	bzero((caddr_t)ws, sizeof (*ws));
	/* ws_interrupt turns self off after last workstation goes away */
}

Wsindev *
ws_open_indev(ws, name, fd)
	Workstation *ws;
	char *name;
	int fd;
{
	register Wsindev *wsid;
	register int	i;
	register int	err = 0;
	int	mode, block;
	struct	file *fp = 0;
	register struct	tty *tp;
	int	(*tp_io)();
	Wsindev *wsid_tmp;
	dev_t	dev;
	struct	sgttyb sg;
	extern	caddr_t win_kmem_alloc();

	/* Open device */
	if ((err = kern_openfd(fd, &fp, FREAD)))
		goto SilentError;
	dev = ((struct vnode *)fp->f_data)->v_rdev;
	/* See if can find this device open already */
	if (ws_indev_match_dev(dev) != WORKSTATION_NULL)
		goto SilentError;
	/* Allocate new input record */
	wsid = (Wsindev *) win_kmem_alloc(sizeof (*wsid));
	if (wsid == WSINDEV_NULL) {
		printf("ws_open_indev: heap alloc failed\n");
		err = ENOMEM;
		goto SilentError;
	}
	bzero((caddr_t)wsid, sizeof (*wsid));
	/* Initialize record */
	wsid->wsid_fp = fp;
	wsid->wsid_dev = dev;
	tp = wsid->wsid_tp = cdevsw[major(dev)].d_ttys+minor(dev);
	tp_io = linesw[tp->t_line].l_ioctl;
	wsid->wsid_flags = 0;
	/* Copy name */
	for (i = 0;i < SCR_NAMESIZE; i++)
		wsid->wsid_name[i] = name[i];
	/* Determine current byte stream mode */
	if ((err = (*tp_io)(tp, VUIDGFORMAT, &wsid->wsid_previous_mode, 0))) {
		printf("ioctl err VUIDGFORMAT err = %D\n", err);
		wsid->wsid_flags |= WSID_TREAT_AS_ASCII;
		wsid->wsid_previous_mode = VUID_NATIVE;
	}
	/* Set up to send firm events */
	mode = VUID_FIRM_EVENT;
	if ((err = (*tp_io)(tp, VUIDSFORMAT, &mode, 0))) {
		printf("ioctl err VUIDSFORMAT err = %D\n", err);
		wsid->wsid_flags |= WSID_TREAT_AS_ASCII;
		wsid->wsid_previous_mode = VUID_NATIVE;
	}
	/* Get blocking status (cheating by going into tty struct) */
	if (tp->t_state & TS_NBIO)
		wsid->wsid_flags |= WSID_PREV_NBIO;
	/* Set non-blocking */
	block = 1;
	if ((err = (*tp_io)(tp, FIONBIO, (caddr_t)&block, 0))) {
		printf("ioctl err FIONBIO ");
		goto Error;
	}
	/* Get sgttyb flags */
	if ((err = (*tp_io)(tp, TIOCGETP, (caddr_t)&sg, 0))) {
		printf("ioctl err TIOCGETP ");
		goto Error;
	}
	wsid->wsid_stty_flags = sg.sg_flags;
	/* Set raw */
	sg.sg_flags = RAW;
	if ((err = (*tp_io)(tp, TIOCSETP, (caddr_t)&sg, 0))) {
		printf("ioctl err TIOCSETP ");
		goto Error;
	}
	/*
 	 * Get/set direct I/O flag.  This is a hack to avoid /dev/console
	 * interference when using /dev/kbd.  Silent about any error
	 * on KIOCGDIRECT because only /dev/kbd will respond.
	 */
 	if ((*tp_io)(tp, KIOCGDIRECT, &wsid->wsid_usecode, 0) == 0) {
 		int directio = 1;

 		if ((err = (*tp_io)(tp, KIOCSDIRECT, &directio, 0))) {
 			printf("ioctl err KIOCSDIRECT ");
			goto Error;
		}
		wsid->wsid_flags |= WSID_DIRECT_VALID;
	} else if ((*tp_io)(tp, KIOCGUSECODE, &wsid->wsid_usecode) == 0) {
		/*
		 * Get/set use code.  This is a hack to avoid /dev/console
		 * interference when using /dev/kbd.  Silent about any error
		 * on KIOCGUSECODE because only /dev/kbd will respond.
		 * This duplicate stuff is for backwards compatibility with
		 * pre-3.2 stuff.
		 */
		int ws_usecode();

		if ((err = (*tp_io)(tp, KIOCSUSECODE, ws_usecode))) {
			printf("ioctl err TIOCSETP ");
			goto Error;
		}
		wsid->wsid_flags |= WSID_USE_CODE_VALID;
	}
	/* Put new record in list */
	wsid_tmp = ws->ws_indev;
	ws->ws_indev = wsid;
	wsid->wsid_next = wsid_tmp;
	return (wsid);
	/* Error handling is dirty...doesn't release resources */
Error:
	printf("err = %D\n", err);
SilentError:
	win_errno = err;
	return (WSINDEV_NULL);
}

int
ws_close_indev(ws, wsid)
	Workstation *ws;
	Wsindev *wsid;
{
	register int err;
	struct sgttyb sg;
	int block;
	register struct tty *tp = wsid->wsid_tp;
	int (*tp_io)() = linesw[tp->t_line].l_ioctl;
	register Wsindev *indev, **indev_ptr;

	/*
	 * Remove wsid from list associated with workstation.
	 * This will avoid any further read of this device because zsclose
	 * (called from closef below) might sleep.
	 * In other words, when zsclose sleeps, the ws_interrupt timeout
	 * can occur.  By closef the device's original blocking state
	 * has been restored.  Thus, when ws_interrupt does a read of this
	 * device, it might block (sleep).  Blocking from a timeout notification
	 * will lead to a panic: sleep.
	 */
	indev_ptr = &ws->ws_indev;
	for (indev = ws->ws_indev; indev != WSINDEV_NULL;
	    indev = indev->wsid_next) {
		if (indev == wsid) {
			*indev_ptr = indev->wsid_next;
			indev->wsid_next = WSINDEV_NULL;
			goto Found;
		}
		indev_ptr = &indev->wsid_next;
	}
	printf("ws_close_indev: device not found\n");
	return (-1);
Found:
	/* Reset non-blocking state */
	if (wsid->wsid_flags & WSID_PREV_NBIO)
		block = 1;
	else
		block = 0;
	if ((err = (*tp_io)(tp, FIONBIO, (caddr_t)&block, 0)))
		printf("ioctl err %D FIONBIO\n", err);
	/* Reset sgttyb flags */
	if ((err = (*tp_io)(tp, TIOCGETP, (caddr_t)&sg, 0)))
		printf("ioctl err %D TIOCGETP\n", err);
	sg.sg_flags = wsid->wsid_stty_flags;
	if ((err = (*tp_io)(tp, TIOCSETP, (caddr_t)&sg, 0)))
		printf("ioctl err %D TIOCSETP\n", err);
	/* Reset vuid format */
	if ((err = (*tp_io)(tp, VUIDSFORMAT, &wsid->wsid_previous_mode, 0)))
		printf("ioctl err %D VUIDSFORMAT\n", err);
	/* Reset use code or direct mode */
	if (wsid->wsid_flags & WSID_DIRECT_VALID) {
 		if ((err = (*tp_io)(tp, KIOCSDIRECT, &wsid->wsid_usecode, 0)))
 			printf("ioctl err %d KIOCSDIRECT\n", err);
	} else if (wsid->wsid_flags & WSID_USE_CODE_VALID) {
		if ((err = (*tp_io)(tp, KIOCSUSECODE, wsid->wsid_usecode, 0)))
			printf("ioctl err %D KIOCSUSECODE\n", err);
	}
	/* Cleanup file pointer */
	if (wsid->wsid_fp)
		closef(wsid->wsid_fp);
	/* Release memory */
	kmem_free((caddr_t)wsid, sizeof (*wsid));
	return (0);
}

/*
 * Called at interrupt level 5 or 6.  Send kbd input to own tty.
 * This is a hack to avoid interference from the /dev/console on /dev/kbd.
 */
/* ARGSUSED */
ws_usecode(tp, keycode, k)
	struct	tty *tp;
	unsigned char keycode;
	struct	keyboardstate *k;
{
	extern ttyinput();

	kbdtranslate(tp, keycode, ttyinput);
}

void
ws_shrink_queue(ws)
	register Workstation *ws;
{
	Vuid_queue q;
	int qbytes;
	caddr_t qdata;

	/* Allocate new input q */
	qbytes = ws_vq_node_bytes;
	qdata = zmemall(memall, qbytes);
	if (qdata == NULL)
		return;
	vq_initialize(&q, qdata, (u_int)qbytes);
	/* Deallocate old input queue */
	if (ws->ws_qdata != NULL)
		wmemfree(ws->ws_qdata, (int)ws->ws_qbytes);
	/* Update ws */
	ws->ws_qbytes = qbytes;
	ws->ws_qdata = qdata;
	ws->ws_q = q;
}

/* Deal with input queue overflow */
void
ws_handle_overflow(ws)
	register Workstation *ws;
{
	Vuid_queue q;
	int qbytes;
	caddr_t qdata;
	Firm_event firm_event;

	if (ws->ws_qbytes >= ws_vq_node_bytes * ws_vq_expand_times)
		goto Flush;
	/* Allocate new input q */
	qbytes = ws->ws_qbytes + ws_vq_node_bytes;
	qdata = zmemall(memall, qbytes);
	if (qdata == NULL)
		goto Flush;
	vq_initialize(&q, qdata, (u_int)qbytes);
	/* Copy contents of old q onto new one */
	while (vq_get(&ws->ws_q, &firm_event) != VUID_Q_EMPTY)
		if (vq_put(&q, &firm_event) == VUID_Q_OVERFLOW)
			printf("Window input queue unexpectedly full!\n");
	/* Deallocate old input queue */
	if (ws->ws_qdata != NULL)
		wmemfree(ws->ws_qdata, (int)ws->ws_qbytes);
	/* Update ws */
	ws->ws_qbytes = qbytes;
	ws->ws_qdata = qdata;
	ws->ws_q = q;
	return;
Flush:
	/* Punt and flush queue */
	printf("Window input queue overflow!\n");
	ws_flush_input(ws);
	printf("Window input queue flushed!\n");
}

