

 /*	@(#)wintree.h 1.3 87/01/09 SMI	*/

/*
 * This header file defines the SunWindows related kernel data structures,
 * including the Workstation (input control), Desktop (screen managment),
 * Window (rectangular region on a screen).
 */


#include "win.h"
#include "dtop.h"
#include "../h/param.h" /* has machine/param h/types h/signal */
#include "../h/dir.h"
#include "../h/user.h"	/* has machine/pcb h/dmap h/time h/resource */
#include "../h/proc.h"
#include "../h/conf.h"
#include "../h/ioctl.h"	/* h/ttychars h/ttydev sgtty */
#include "../h/tty.h"	/* h/ttychars h/ttydev */
#include "../h/map.h"
#include "../h/clist.h"
#include "../pixrect/pixrect.h"
#include "../pixrect/memvar.h"
#include "../sunwindow/rect.h"
#include "../sunwindow/rectlist.h"
#include "../sunwindow/win_screen.h"
#include "../sunwindow/win_cursor.h"
#include "../sunwindow/win_input.h"
#include "../sunwindow/cms.h"
#include "../sunwindow/win_lock.h"
#include "../sundev/vuid_state.h"
#include "../sundev/vuid_queue.h"

/*
 * Flag used during open of window device when it is supposed to be the
 * first time opened (exclusive open).  O_EXCL in file.h currently has wrong
 * mapping to FEXLOCK.  Need a FEXCLOPEN in file.h.
 */
#define	WIN_EXCLOPEN		0x10000

/*
 * The following structure is used to provide exclusive locks to
 * workstation (input & display lock access) and desktop (screen
 * access and clipping data change) resources [see below].  It
 * works on a process granularity although clients often think
 * that it is on a window file descriptor granularity.
 */
typedef	struct	windowlock {	/* "winlock" name collides in win_ioctl.h */
	int	lok_options;		/* lock control options */
#define	WLOK_SILENT	0x01		/* don't print messages about lock */
	int	lok_pid;		/* pid that holds lock */
	int	*lok_count;		/* reference count on lock */
	int	lok_count_storage;	/* default lock count storage */
	int	lok_time;		/* timeout on lock */
	int	lok_max_time;		/* max. time for lok_time */
	int	lok_nice;		/* remembered nice before boosted */
	struct	proc *lok_proc;		/* proc struct holding lock */
	struct	user *lok_user;		/* user struct holding lock */
					/* May be NULL even while lock held */
	long	lok_user_addr;		/* mem man handle on lok_user */
					/* May be 0 even while lock held */
	struct	timeval lok_utime;	/* user time when acquired lock */
	struct	timeval lok_stime;	/* system time when acquired lock */
	struct	timeval lok_limit;	/* elapsed time when will break lock */
	int	lok_bit;		/* mask bit in lok_flags of interest */
	int	*lok_flags;		/* flags associate with passed around */
					/* lockbit */
	/* The following fields are set after calling wlok_setlock (each */
	/* pointer could be null, except lok_string): */
	caddr_t	lok_client;		/* interpreted by actions */
	void	(*lok_unlock_action)();	/* called on unlock */
	void	(*lok_timeout_action)();/* called when lock timedout, no */
					/* timeout detection if null */
	void	(*lok_force_action)();	/* called on force unlock */
	char	*lok_string;		/* describes lock type for error msgs */
	int	(*lok_other_check)();	/* called to check for other locks */
	caddr_t	lok_wakeup;		/* who to wakeup when unlocked */
	int	lok_id;			/* lock id from shared lock */
} Winlock;

/*
 * The workstation input device structure is used to describe a
 * physical input device that the kernel is polling for input.
 * The input device is opened in a user process and reopened by
 * the kernel so as to have the device's reference count bumped.
 * Input device drivers should talk in vuid (virtual user input
 * device format) although ascii will be accepted.
 */
typedef	struct	wsindev {
	int	wsid_flags;
#define	WSID_TREAT_AS_ASCII	0x01	/* Should expect ascii from this dev */
#define	WSID_PREV_NBIO		0x02	/* Previous non-blocking status */
#define	WSID_USE_CODE_VALID	0x04	/* wsid_usecode is valid */
#define	WSID_DIRECT_VALID	0x08	/* wsid_usecode is "direct" flag */
	short	wsid_stty_flags;	/* Remembered struct sgttyb.sg_flags */
	struct	file *wsid_fp;		/* File ptr to open input device */
	dev_t	wsid_dev;		/* Input device */
	struct	tty *wsid_tp;		/* Tty ptr of open input device */
	int	(*wsid_usecode)();	/* Use code function saved (/dev/kbd) */
	char	wsid_name[SCR_NAMESIZE];/* Device name */
	struct	wsindev *wsid_next;	/* Next node (when put as list) */
	int	wsid_previous_mode;	/* Result of VUIDGFORMAT */
} Wsindev;
#define	WSINDEV_NULL	((Wsindev *)0)
#define	WSID_DEVNONE	-1

/*
 * Ws_usr_async is used to help detect real time user specified
 * interrupt actions.
 */
typedef	struct	ws_usr_async {
	short	flags;
#define	WUA_1ST_HAPPENED	0x01	/* 1st event happened */
#define	WUA_IGNORE_2ND		0x02	/* don't bother waiting for 2nd event */
	short	first_id;		/* id of the first event */
	int	first_value;		/* value of the first event */
	short	second_id;		/* id of the second event */
	int	second_value;		/* value of the second event */
	void	(*action)();		/* called to handle interrupt */
} Ws_usr_async;

/*
 * States of focus transition state machine.
 */
typedef	enum win_focus_state {
	SEND_Q_TOP=0,		/* Try pull event off top of input q */
	SEND_EXIT=1,		/* Send LOC_WINEXIT */
	SEND_ENTER=2,		/* Send LOC_WINENTER */
	SEND_REQUEST=3,		/* Request kbd focus change veto */
	REQUEST_WAIT=4,		/* Wait for kbd focus change veto reply */
	SEND_DONE=5,		/* Send KBD_DONE */
	SEND_USE=6,		/* Send KBD_USE */
	SEND_FOCUS_EVENT=7,	/* Send event that provoked kbd focus change */
	SEND_DIRECT_REQUEST=8,	/* Explicit kbd focus change request */
} Win_focus_state;

/*
 * Ws_focus_match is used to describe a focus change event.
 */
typedef	struct ws_focus_set {
	short	id;		/* id of event that changes ws_kbdfocus */
	int	value;		/* value of event that changes ws_kbdfocus */
	int	shifts;		/* ws_shiftmask required to change ws_kbdfocus*/
#define	WS_FOCUS_ANY_SHIFT	-1	/* don't care value for shifts */
} Ws_focus_set;

/*
 * The workstation is a defined to be some number of screens and some
 * number of input devices with which a single user interacts.  A single
 * workstation shares a single extant cursor among all its screens.
 * One can acquire an i/o lock that applies to all input devices and all
 * screens of the workstation.
 *
 * Here is how the input focus mechanism works.  The pick focus follows
 * the primary locator position around and the kbd focus is set either
 * programmatically or via some explicit user action (could be locator
 * motion).  Setting the kbd focus via some explicit user action
 * generates a KBD_REQUEST event for windows whose pick mask asks for it,
 * otherwise the kernel silently sets the kbd focus to the current pick
 * focus.  A receiver of a KBD_REQUEST can refuse the kbd focus via an ioctl.
 *
 * Any event is first compared with the kbd focus' kbd mask
 * and sent to the kbd focus if there is a match.  If unsend, the event
 * is then compared with the pick focus' pick mask and sent to the pick
 * focus if there is a match.  Unmatched events travel up the pick focus'
 * input next link being compared with pick masks on the way.
 */
typedef	struct	workstation {
	int	ws_flags;
#define	WSF_PRESENT		0x01	/* the screen exists */
#define	WSF_LOCKED_IO		0x02	/* i/o access rights locked */
#define	WSF_LOCKED_EVENT	0x04	/* wait for ws_event_consumer to finish
					   (set when read event, cleared when
					   read/select/input_release) */
#define	WSF_ALL_LOCKS	(WSF_LOCKED_IO | WSF_LOCKED_EVENT )
#define	WSF_UNUSED		0x08	/* unused bit */
#define	WSF_LOC_UPDATED		0x10	/* changed locator position when read */
#define	WSF_BREAK_PUSHED	0x20	/* event lock break event detected */
#define	WSF_STOP_PUSHED		0x40	/* stop event detected */
#define	WSF_LOC_IN_TRANSIT	0x80	/* avoiding waking up win with motion */
#define WSF_KBD_REQUEST_PENDING 0x100	/* waiting for answer about kbd focus */
#define	WSF_SEND_FOCUS_EVENT	0x200	/* send ws_focus_event soon */
#define	WSF_EXITING		0x400	/* in middle of ws_close */
#define	WSF_LATEST_WAS_MOTION	0x800	/* user time level motion vs still */
#define	WSF_SWALLOW_FOCUS_EVENT	0x1000	/* no send focus event if request OKed*/
	/*
	 * Device management:
	 */
	struct	desktop *ws_dtop;	/* list of desktops for workstation */
	struct	wsindev *ws_indev;	/* input devices */
	/*
	 * Input queue management (linked list):
	 */
	Vuid_queue ws_q;		/* window input q */
	caddr_t	ws_qdata;		/* address of ws_q data block */
	u_int	ws_qbytes;		/* number of bytes used for ws_qdata */
	/*
	 * Input queue synchronized management:
	 */
	struct	window *ws_event_consumer;/* window that is processing event */
	struct	windowlock ws_eventlock;/* event access lock */
	struct	timeval ws_eventtimeout;/* time when will break ws_eventlock */
	/*
	 * Real time user specified asynchronous interrupt management:
	 */
	struct	ws_usr_async ws_break;	/* break ws_eventlock */
	struct	ws_usr_async ws_stop;	/* send stop to real time pick focus */
	/*
	 * Virtual user input device state management:
	 */
	Vuid_state ws_instate;		/* virtual input device state that */
					/* user processes see (locator pos */
					/* data relative to ws_pickfocus's */
					/* dtop) */
	int	ws_shiftmask;		/* usertime shiftmask */
	struct	inputmask ws_surpress_mask;
					/* when send escape sequence, turn on */
					/* func key bit here so don't send up */
	/*
	 * Input consumer management:
	 */
	struct	window *ws_kbdfocus;	/* win to which w_kbdmask events go */
	struct	window *ws_pickfocus;	/* win to which w_pickmask events go */
	struct	window *ws_kbdfocus_next; /* win to be next kbd focus */
	struct	window *ws_pickfocus_next; /* win to be next pick focus */
					/* Only state machine moves values */
					/* between * and *_next fields.  Only */
					/* ws_set_focus sets values to fields.*/
					/* winclose may clear the * field */
	Win_focus_state ws_focus_state;	/* state of focus transitions */
	struct	desktop *ws_pick_dtop;	/* desktop pick focus currently on */
	Firm_event ws_focus_event;	/* event that prompted focus change */
	/*
	 * Focus control parameters.  Can have separate events for both
	 * pass thru (_pt) the focus event and swallow (_sw) the focus
	 * event.  These affect ws_kbdfocus.
	 */
	Ws_focus_set ws_kbd_focus_pt;	/* pass thru focus event */
	Ws_focus_set ws_kbd_focus_sw;	/* swallowed focus event */
	/*
	 * Grabio management:
	 */
	struct	window *ws_inputgrabber;/* window that is hogging events */
	struct	window *ws_pre_grab_kbd_focus; /* kbd focus pre-iolock */
	struct	window *ws_pre_grab_kbd_focus_next;
					/* next kbd focus pre-iolock */
	struct	window *ws_pre_grab_pick_focus; /* pick focus pre-iolock */
	struct	window *ws_pre_grab_pick_focus_next;
					/* next pick focus pre-iolock */
	struct	windowlock ws_iolock;	/* input & display lock access lock */
	/*
	 * Cursor management:
	 */
	Vuid_state ws_rtstate;		/* real time vuid state (locator pos */
					/* data relative to ws_loc_	dtop) */
	struct	desktop *ws_loc_dtop;	/* desktop locator currently on */
	int	ws_quietticks;		/* # of ticks since no input received */
	int	ws_loc_stillticks;	/* # of ticks locator has been still */
	int	ws_favor_pid;		/* current interactive pid */
} Workstation;
#define	WORKSTATION_NULL	((Workstation *)0)

/*
 * The desktop cursor structure describes the current cursor and/or
 * xhairs displayed on a screen.  It includes the image under the cursor/
 * cross hairs as well as clipping data for the xhairs.
 */
#define	DTOP_PRMAXDEPTH	8

#define	cursor_x(dtop)		((dtop)->shared_info->cursor_info.x)
#define	cursor_y(dtop)		((dtop)->shared_info->cursor_info.y)

#define	cursor_screen_width(dtop)	\
	((dtop)->shared_info->cursor_info.screen_pr.pr_width)
#define	cursor_screen_height(dtop)	\
	((dtop)->shared_info->cursor_info.screen_pr.pr_height)

#define	cursor_up(dtop)		((dtop)->shared_info->cursor_info.cursor_is_up)
#define	horiz_hair_up(dtop)	((dtop)->dt_cursor.horiz_hair_is_up)
#define	vert_hair_up(dtop)	((dtop)->dt_cursor.vert_hair_is_up)
#define	enable_plane_cursor_up(dtop)	((dtop)->dt_cursor.enable_cursor_is_up)

#define	cursor_set_up(dtop, is_up)	cursor_up(dtop) = (is_up);
#define	horiz_hair_set_up(dtop, is_up)	horiz_hair_up(dtop) = (is_up);
#define	vert_hair_set_up(dtop, is_up)	vert_hair_up(dtop) = (is_up);
#define	enable_plane_cursor_set_up(dtop, is_up) \
	    enable_plane_cursor_up(dtop) = (is_up);

#define	show_enable_plane_cursor(dtop)	((dtop)->dt_cursor.enable_cursor_active)
#define	enable_plane_cursor_set_active(dtop, is_active) \
		((dtop)->dt_cursor.enable_cursor_active) = (is_active);

struct	dtopcursor {
	int	hair_x;		/* x-coordinate of vertical hair */
	int	hair_y;		/* y-coordinate of horizontal hair */
	int	horiz_hair_size;/* horizontal hair kmem_alloc size */
	int	vert_hair_size;	/* vertical hair kmem_alloc size */
	int	enable_color;	/* enable cursor color (0 or 1) */

	/* crosshair flags */
	unsigned int	horiz_hair_is_up : 1;	/* hair has been drawn */
	unsigned int	vert_hair_is_up	 : 1;	/* hair has been drawn */
	unsigned int	enable_cursor_is_up : 1;/* enable cursor drawn */
	unsigned int	enable_cursor_active :1;/* enable cursor active */

	struct	rectlist horiz_hair_rectlist;	/* list of clobbered area */
	struct	rectlist vert_hair_rectlist;	/* list of clobbered area */
	struct	pixrect horiz_hair_mpr;		/* horizontal x-hair */
	struct	mpr_data horiz_hair_data;	/* data underneath it */
	struct	pixrect vert_hair_mpr;		/* vertical x-hair */
	struct	mpr_data vert_hair_data;	/* data underneath it */
	struct	pixrect enable_mpr;		/* enable cursor */
	struct	mpr_data enable_data;		/* data underneath it */
	short	enable_image[CUR_MAXIMAGEBYTES/2];/* enable plane image */
#define	dtopcursorfixup(dc) /* Make screen mpr fields reference each other*/\
	{ extern struct pixrectops mem_ops;\
	  (dc)->horiz_hair_mpr.pr_ops = &mem_ops;\
	  (dc)->horiz_hair_mpr.pr_depth = DTOP_PRMAXDEPTH;\
	  (dc)->horiz_hair_mpr.pr_data = (caddr_t) &(dc)->horiz_hair_data;\
	  (dc)->vert_hair_mpr.pr_ops = &mem_ops;\
	  (dc)->vert_hair_mpr.pr_depth = DTOP_PRMAXDEPTH;\
	  (dc)->vert_hair_mpr.pr_data = (caddr_t) &(dc)->vert_hair_data;\
	  (dc)->enable_mpr.pr_ops = &mem_ops;\
	  (dc)->enable_mpr.pr_depth = 1;\
	  (dc)->enable_mpr.pr_width = 16;\
	  (dc)->enable_mpr.pr_height = 16;\
	  (dc)->enable_mpr.pr_data = (caddr_t) &(dc)->enable_data;\
	  (dc)->enable_data.md_image = (dc)->enable_image;\
	  (dc)->enable_data.md_linebytes = mpr_linebytes( \
		(dc)->enable_mpr.pr_width, (dc)->enable_mpr.pr_depth);\
	}
};

/*
 * The desktop is a per-screen structure.
 *
 * This structure records physical attributes: whether the device
 * for the surface is present, its type (color, b/w, etc),
 * the physical addresses of the device, and the location of the
 * other screens in the system relative to this one (for use by its
 * workstation for multiplexing a single mouse among several screens.)
 *
 * Each screen has an associated window tree for clipping, attached
 * to the screen starting at a root window.  Colormap multiplexing
 * is handled thru the desktop.
 *
 * Locator x/y deltas are held in ws_rtstate and ws_utstate
 * There are a variety of places that locator x/y positions are held.
 * They are alway relative to a particular desktop since we don't support
 * adjacent screen locator access.  The ws_instate values are not defined.
 * A window that wants the user time locator position goes to its desktop
 * and uses the dt_ut_* values, these are the latest locator positions from
 * the last time that the locator was on that desktop.  The ws_rtstate
 * values are defined to be those that are associate with the desktop
 * that currently has the cursor.  A desktop that wants the latest real
 * time locator position for itself goes to its dt_rt_* values.
 * A desktop's current cursor image position is in dt_cursor.
 */

typedef	struct	desktop {
	int	dt_flags;
#define	DTF_PRESENT		0x01	/* the screen exists */
#define	DTF_NEWCURSOR		0x08	/* set if should change current cursor*/
#define	DTF_NEWCMAP		0x10	/* set if should change cur color map*/
#define	DTF_EXITING		0x20	/* process of destroying desktop */
#define	DTF_MULTI_PLANE_GROUPS	0x40	/* pixrect has mulitple plane groups */
#define DTF_DBLBUFFER		0x80	/* Double buffer capablity 	     */
#define DTF_DBL_FOUND		0x100	/* Found the double bufferer yet */


	struct	workstation *dt_ws;	/* workstation tyed to */
	struct	desktop *dt_next;	/* desktop list starting at dt_ws */
	struct	screen dt_screen;	/* external screen description */
	struct	pixrect *dt_pixrect;	/* pixel device used for screen access*/
	struct	file *dt_fbfp;		/* file ptr to framebuffer device */
	struct	window *dt_displaygrabber;/* only window that can lock display*/
	struct	window *dt_rootwin;	/* root window for screen */
	struct	window *dt_parentinvalidwin; /* rlInvalid is relative to this */

	/* Note: both the display and mutex locks are shared locks.
	 * the contents of these structs should not be considered
	 * valid unless dtop_validate_shared_lock() is called first.
	 */
	struct	windowlock dt_mutexlock; /* shared memory mutual exclusion */
	struct	windowlock dt_displaylock; /* display lock */

	struct	windowlock dt_datalock;	/* data lock */
	struct	rectlist dt_rlinvalid;	/* Accumulated damaged rl */
	struct	desktop *dt_neighbors[SCR_POSITIONS]; /* where are other dts? */
	struct	dtopcursor dt_cursor;	/* cursor/xhairs for desktop */
	struct	window *dt_cursorwin;	/* window from with next cursor gotten*/
	int	dt_rt_x;		/* realtime x for this desktop */
	int	dt_rt_y;		/* realtime y for this desktop */
	int	dt_ut_x;		/* usertime x for this desktop */
	int	dt_ut_y;		/* usertime y for this desktop */
	struct	cms_map dt_cmap;	/* colormap data ptrs into cmapdata */
	int	dt_cmsize;		/* number of entries in colormap */
	caddr_t dt_cmapdata;		/* & of colormap data block */
	struct	mapent *dt_rmp;		/* colormap resource allocator data */
	int	dt_exit_seconds;	/* count of secs blocked on destroy */
	Win_lock_block	*shared_info;	/* shared memory for faster locking */
	int	display_waiting;	/* count of waiters for display lock */
#define	DTOP_MAX_PLANE_GROUPS	12	/* 12 is arbitrary */
	char	dt_plane_groups_available[DTOP_MAX_PLANE_GROUPS];
					/* framebuffer capabilities */
	struct	singlecolor dt_bkgnd_cache; /* Fullscreen access color cache */
	struct	singlecolor dt_frgnd_cache; /* Fullscreen access color cache */
	int	dt_dblcount;		/* Number of double bufferes */
	char	dt_dbl_bkgnd;		/* Background for double buffer */
	char 	dt_dbl_frgnd;		/* Foreground for double buffer */
	struct	window	*dt_curdbl;	/* Current double bufferer   */
} Desktop;
#define	DESKTOP_NULL	((Desktop *)0)

/*
 * Window data structure defines the characteristics of a window psuedo
 * device that are not shared among all the windows on a desktop or
 * workstation.  A window is the smallest granularity about which the
 * kernel manages input distribution or display image mangement.
 * (See Workstation comment about input mask usage).
 */
typedef	struct	window {
	int	w_flags;
#define	WF_OPEN			0x01	/* device is open */
#define	WF_INSTALLED		0x02	/* installed in a tree */
#define	WF_SIZECHANGED		0x08	/* size changed */
#define	WF_DTOPPOSCHANGED	0x10	/* dtop relative position changed */
#define	WF_RCOLL		0x20	/* select collided */
#define	WF_NBIO			0x40	/* non-blocking i/o wanted */
#define	WF_ROOTWINDOW		0x80	/* root window of screen */
#define	WF_PREVIOUSDAMAGED	0x100	/* non-null rlfixup last time clipped*/
#define	WF_WANTINPUT		0x200	/* currently waiting on input */
#define	WF_CMSHOG		0x400	/* override std color map when active */
#define	WF_ASYNC		0x800	/* use asyncronous input */
#define	WF_KBD_MASK_SET		0x1000	/* kbd mask has been set explicitly */
#define	WF_UNSYNCHRONIZED	0x2000	/* Broke event lock so run this guy
					 * unsynchronized (don't give event
					 * lock) until his input queue empty */
#define	WF_PLANE_GROUP_SET	0x4000	/* plane group set explicitly */
#define WF_DBLBUF_ACCESS	0x8000	/* Double buffer Accessed */

#define	WIN_LINKS		5
	struct	window *w_link[WIN_LINKS];
#define	WL_PARENT		0
#define	WL_OLDERSIB		1
#define	WL_YOUNGERSIB		2
#define	WL_OLDESTCHILD		3
#define	WL_YOUNGESTCHILD	4

#define	WL_ENCLOSING		w_link[WL_PARENT]
#define	WL_COVERED		w_link[WL_OLDERSIB]
#define	WL_COVERING		w_link[WL_YOUNGERSIB]
#define	WL_BOTTOMCHILD		w_link[WL_OLDESTCHILD]
#define	WL_TOPCHILD		w_link[WL_YOUNGESTCHILD]

#define	WIN_NULLLINK		-1

#define	WT_TOPTOBOTTOM		1
#define	WT_BOTTOMTOTOP		2

	struct	desktop *w_desktop;	/* desktop on which window lies */
	struct	workstation *w_ws;	/* workstation that drives window */
	struct	rect w_rect;		/* parent relative...
					   used when computing rlExposed */
	struct	rect w_rectsaved;	/* used for icon/normal toggle */
	coord	w_screenx;		/* screen relative (used when paint) */
	coord	w_screeny;		/* screen relative (used when paint) */
	int	w_pid;			/* pid that will be signalled */
	int	w_userflags;		/* flags available to user process */
	int	w_clippingid;		/* version of clipping information */
	/* All rectlist's are self relative; use screenOffset's when paint */
	struct	rectlist w_rlexposed;	/* for normal clipping */
	struct	rectlist w_rlexposedold;/* for saving bits (wmgr) */
	struct	rectlist w_rlfixup;	/* for fixing damage (user) */
	struct	inputmask w_kbdmask;	/* kbd focus input mask */
	struct	inputmask w_pickmask;	/* pick focus input mask */
	struct	window *w_inputnext;	/* if !in *mask (above), passed here */
	struct	clist w_input;		/* input queue for this window */
	struct	proc *w_rsel;		/* process selecting */
	struct	cursor w_cursor;	/* per-window cursor */
	struct	pixrect w_cursormpr;	/* w_cursor.cur_shape */
	struct	mpr_data w_cursordata;	/* w_cursormpr.pr_data */
	short	w_cursorimage[CUR_MAXIMAGEBYTES/2];/* w_cursordata.md_image */
	struct	colormapseg w_cms;	/* colormap segment */
	struct	cms_map w_cmap;		/* colormap data ptrs into w_cmapdata */
	caddr_t w_cmapdata;		/* address of colormap data block */
	int	w_plane_group;		/* Plane group that window is using */
	struct	proc *w_aproc;		/* proc for FIOASYNC */
	int	w_apid;			/* pid for FIOASYNC */
#define	winfixupcursor(w) /* Make cursor components in w reference each other*/\
	{ extern struct pixrectops mem_ops;\
	  (w)->w_cursor.cur_shape = &(w)->w_cursormpr;\
	  (w)->w_cursormpr.pr_ops = &mem_ops;\
	  (w)->w_cursormpr.pr_data = (caddr_t)&(w)->w_cursordata;\
	  (w)->w_cursordata.md_image = (w)->w_cursorimage;\
	}
	char	w_dbl_rdstate;	/*Read control bit foregr/backgr state*/
	char	w_dbl_wrstate;	/*Write control bit foregr/backgr st*/
} Window;
#define	WINDOW_NULL	((Window *)0)

Desktop desktops[NDTOP];
#define	NWORKSTATION	NDTOP
Workstation workstations[NWORKSTATION];

#define	WINALLOC_INC	((8192)/(sizeof(struct window)))
#define	WINBUFS		((NWIN/WINALLOC_INC)+1)

struct	window *winbufs[WINBUFS];

struct	window	*winfromdev(),
		*winfromopendev();

/*
 * Colormap Resource Map macros/constants
 */
#define	CRM_ADDROFFSET		1
#define	CRM_SLOTSIZE		8
#define	CRM_NSLOTS(size)	((size)/CRM_SLOTSIZE)
#define	CRM_SIZE(size)		(CRM_NSLOTS((size))+2)

extern	win_errno;

int	ws_close_indev();
Window	*wt_intersected();

#define	LOCKPRI	28

#define	spl_timeout()	spl1()

/*
 * Flags to the ws_set_focus call.
 * FF_PICK_CHANGE means that the locator (or windows) may have changed,
 * set pick focus from locator.  If LOC_WINENTER then change kbd focus too.
 * FF_KBD_EVENT mean to use ws_focus_event to change the kbd focus.  Will
 * set WSF_KBD_REQUEST_PENDING.
 */
void	ws_set_focus();
#define	FF_PICK_CHANGE	0x1
#define	FF_KBD_EVENT	0x2

void win_shared_update_cursor();
void win_shared_update_cursor_active();
void win_shared_update_mouse_xy();
void win_shared_update();


