#ifndef lint
static  char sccsid[] = "@(#)perfmeter.c 1.4 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 *	Copyright (c) 1985 by Sun Microsystems Inc.
 */

/* 
 *	perfmeter [-v name] [-s sample] [-m minute] [-h hour]
 *		[ -M value minmax maxmax ] [ host ]
 *
 *		displays meter for value 'name' sampling
 *		every 'sample' seconds.  The minute hand is an
 *		average over 'minute' seconds, the hour hand over
 *		'hour' seconds
 */

#include <stdio.h>
#include <sys/time.h>
#include <sys/param.h>
#include <rpc/rpc.h>
#include <rpcsvc/rstat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/dk.h>
#include <sys/vmmeter.h>
#include <sys/wait.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <sunwindow/sun.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/win_input.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/win_environ.h>
#include <suntool/icon.h>
#include <suntool/tool.h>
#include <suntool/menu.h>
#include <suntool/walkmenu.h>
#include <suntool/wmgr.h>
#include "meter.h"

#define	MAXINT		0x7fffffff
#define	TRIES		8	/* number of bad rstat's before giving up */
#define TOP_KEY         KEY_LEFT(5)
#define OPEN_KEY        KEY_LEFT(7)

struct timeval TIMEOUT = {20,0};

struct	meter meters_init[] = {
	/* name		maxmax	minmax	curmax	scale	lastval */
	{ "cpu",	100,	100,	100,	1,	-1 },
#define	CPU	0
	{ "pkts",	MAXINT,	32,	32,	1,	-1 },
#define	PKTS	1
	{ "page",	MAXINT,	16,	16,	1,	-1 },
#define	PAGE	2
	{ "swap",	MAXINT,	4,	4,	1,	-1 },
#define	SWAP	3
	{ "intr",	MAXINT,	100,	100,	1,	-1 },
#define	INTR	4
	{ "disk",	MAXINT,	40,	40,	1,	-1 },
#define	DISK	5
	{ "cntxt",	MAXINT,	64,	64,	1,	-1 },
#define	CNTXT	6
	{ "load",	MAXINT,	4,	4,	FSCALE,	-1 },
#define	LOAD	7
	{ "colls",	MAXINT,	4,	4,	FSCALE,	-1 },
#define	COLL	8
	{ "errs",	MAXINT,	4,	4,	FSCALE,	-1 },
#define	ERR	9
};
#define	MAXMETERS	sizeof (meters_init) / sizeof (meters_init[0])

struct	meter *meters;	/* [MAXMETERS] */

int	*getdata();
int	killkids();
int	sigwinched();
int	meter_selected();
int	meter_sigwinhandler();
char 	*calloc();
Bool	defaults_get_boolean();
void	wmgr_changerect();

struct	tool *tool;
struct	pixwin *pw;		/* pixwin for strip chart */
struct	timeval	tv;
struct	pixfont *pfont;
struct	pixrect *ic_mpr;
struct	icon metericon;		/* Defaults to all zeros */

static	struct menuitem menu_items[MAXMETERS];
static	struct menu menu_body;
static	struct menu *menu_ptr;
static	Bool walking;		/* am I using walking menus? */
static	Menu wmenu, fmenu;	/* walking menus */

int	(*old_selected)();
int	(*old_sigwinch)();
int	wantredisplay;		/* flag set by interrupt handler */
int	*save;			/* saved values for redisplay; dynamically
				 * allocated to be [MAXMETERS][MAXSAVE] */
int	saveptr;		/* where I am in save+(visible*MAXMETERS) */
int	dead;			/* is remote machine dead? */
int	visible;		/* which quantity is visible*/
int	length = sizeof (meters_init) / sizeof (meters_init[0]);
int	remote;			/* is meter remote? */
int	rootfd;			/* file descriptor for the root window */
int	width;			/* current width of graph area */
int	height;			/* current height of graph area */
char	*hostname;		/* name of host being metered */
int	sampletime;		/* sample seconds */
int	minutehandintv;		/* average this second interval */
int	hourhandintv;		/* long average over this seconds */
int	shortexp, longexp;
int	designee;

static int oldsocket;

#ifdef STANDALONE
main(argc, argv)
#else
perfmeter_main(argc, argv)
#endif STANDALONE
	register int argc;
	register char **argv;
{
	static char buf[256];
	char **tool_attrs = NULL;
	struct inputmask im;
	register int i, j;
	register struct meter *mp;
	char *cmdname;
	char winname[WIN_NAMESIZE];

	/* Dynamically allocate save */
	save = (int *)(LINT_CAST(calloc(1, sizeof (int)*MAXMETERS*MAXSAVE)));
	/* Dynamically allocate and initialize meters */
	meters = (struct meter *)(LINT_CAST(calloc(1, sizeof (meters_init))));
	for (i = 0; i < MAXMETERS; i++) meters[i] = meters_init[i];
	/* Explicitely initialize data in code */
	menu_body.m_imagetype = MENU_IMAGESTRING;
	menu_body.m_imagedata = "Meter";
	menu_body.m_items = menu_items;
	menu_ptr = &menu_body;
	sampletime = 2;
	minutehandintv = 2;
	hourhandintv = 20;
	oldsocket = -1;

	cmdname = argv[0];
	argc--;
	argv++;
	while (argc > 0) {
		if (argv[0][0] == '-') {
			if (argv[0][2] != '\0')
				goto toolarg;
			switch (argv[0][1]) {
			case 's':
				if (argc < 2)
					usage(cmdname);
				sampletime = atoi(argv[1]);
				break;
			case 'h':
				if (argc < 2)
					usage(cmdname);
				hourhandintv = atoi(argv[1]);
				break;
			case 'm':
				if (argc < 2)
					usage(cmdname);
				minutehandintv = atoi(argv[1]);
				break;
			case 'v':
				if (argc < 2)
					usage(cmdname);
				for (i = 0, mp = meters; i < length; i++, mp++)
					if (strcmp(argv[1], mp->m_name) == 0)
						break;
				if (i >= length)
					usage(cmdname);
				visible = i;
				break;
			case 'M':
				if (argc < 4)
					usage(cmdname);
				for (i = 0, mp = meters; i < length; i++, mp++)
					if (strcmp(argv[1], mp->m_name) == 0)
						break;
				if (i >= length)
					usage(cmdname);
				mp->m_curmax = mp->m_minmax = atoi(argv[2]);
				mp->m_maxmax = atoi(argv[3]);
				argc -= 2;
				argv += 2;
				break;
			default:
			toolarg:
				/*
				 * Pick up generic tool arguments.
				 */
				if ((i = tool_parse_one(argc, argv,
				    &tool_attrs, cmdname)) == -1) {
					(void)tool_usage(cmdname);
					exit(1);
				} else if (i == 0)
					usage(cmdname);
				argc -= i;
				argv += i;
				continue;
			}
			argc--;
			argv++;
		} else {
			if (hostname != NULL)
				usage(cmdname);
			hostname = argv[0];
			remote = 1;
		}
		argc--;
		argv++;
	}

	if (sampletime <= 0 || hourhandintv < 0 || minutehandintv < 0)
		usage(cmdname);
	shortexp = (1 - ((double)sampletime/max(hourhandintv, sampletime))) *
	    FSCALE;
	longexp = (1 - ((double)sampletime/max(minutehandintv, sampletime))) *
	    FSCALE;
	tv.tv_usec = 0;
	tv.tv_sec = sampletime;

	/*
	 * Set up meter icon.
	 */
	metericon.ic_gfxrect.r_width = ICONWIDTH;
	metericon.ic_width = ICONWIDTH;
	metericon.ic_mpr =
	    mem_create(ICONWIDTH, remote ? RICONHEIGHT : ICONHEIGHT, 1);
	if (remote) {
		metericon.ic_gfxrect.r_height = RICONHEIGHT;
		metericon.ic_height = RICONHEIGHT;
	} else {
		metericon.ic_gfxrect.r_height = ICONHEIGHT;
		metericon.ic_height = ICONHEIGHT;
	}

	/*
	 * Open font for icon label.
	 */
	pfont = pf_open("/usr/lib/fonts/fixedwidthfonts/screen.r.7");
	if (pfont == NULL) {
		(void)fprintf(stderr, "%s: can't find screen.r.7\n", cmdname);
		exit(1);
	}

	/*
	 * Get a file descriptor for the root window, it is needed
	 * in many places.  Must get it from the name in the environment
	 * rather than follow links from the tool window because we
	 * haven't created the tool yet and we need it to create the
	 * tool.
	 */
	(void)we_getparentwindow(winname);
	if ((rootfd = open(winname, 0)) < 0) {
		(void)fprintf(stderr, "%s: can't open root window %s\n",
		    cmdname, winname);
		exit(1);
	}

	/*
	 * Create tool window and customize
	 */
	tool = tool_make(
	    WIN_ICON,		&metericon,
	    WIN_NAME_STRIPE,	0,
	    WIN_WIDTH,		ICONWIDTH,
	    WIN_HEIGHT,		remote ? RICONHEIGHT : ICONHEIGHT,
	    WIN_ATTR_LIST,	tool_attrs,
	    0);
	if (tool == (struct tool *)NULL) {
		(void)fprintf(stderr, "%s: can't create tool\n", cmdname);
		exit(1);
	}
	(void)tool_free_attribute_list(tool_attrs);

	/*
	 * Reach into tool to get icon image so don't have to do dynamic
	 * storage stuff with tool_set_attributes every second.
	 * XXX - This is a performance hack.
	 */
	ic_mpr = tool->tl_icon->ic_mpr;

	/*
	 * Save away standard handlers and interpose our own.
	 */
	old_sigwinch = tool->tl_io.tio_handlesigwinch;
	tool->tl_io.tio_handlesigwinch = meter_sigwinhandler;
	old_selected = tool->tl_io.tio_selected;
	tool->tl_io.tio_selected = meter_selected;
	tool->tl_io.tio_timer = &tv;
	pw = tool->tl_pixwin;

	/*
	 * Setup timer.
	 */
	if (setup() < 0) {
		dead = 1;
		keeptrying();
	}

	/* 
	 * Get first set of data, then initialize arrays.
	 */
	updatedata();
	for (i = 0; i < length; i++)
		for (j = 1; j < MAXSAVE; j++)
			save_access(i, j) = -1;

	/*
	 * Initialize menu.
	 */
	walking = defaults_get_boolean("/SunView/Walking_menus", (Bool)FALSE, (int *)0);
	if (walking) {
		char **av;

		av = (char **)(LINT_CAST(
			malloc((unsigned)(3*length*sizeof(char*) + sizeof(char *)))));
		for (i = 0; i < 3*length; i+=3) {
			av[i] = (char *)MENU_STRING_ITEM;
			av[i+1] = meters[i/3].m_name;
			av[i+2] = (char *)(i/3);
		}
		av[i] = 0;
		wmenu = menu_create(ATTR_LIST, av,
				    0);
		fmenu = tool->tl_menu;
		(void)menu_set(wmenu, MENU_INSERT, 0,
			 menu_create_item(MENU_PULLRIGHT_ITEM, "frame",
			    fmenu, 0), 0);
		tool->tl_menu = wmenu;
	} else {
		for (i = 0; i < length; i++) {
			menu_items[i].mi_imagetype = MENU_IMAGESTRING;
			menu_items[i].mi_imagedata = meters[i].m_name;
			menu_items[i].mi_data = (char *)i;
		}
		menu_body.m_itemcount = length;
		menu_body.m_next = wmgr_toolmenu;
		wmgr_setupmenu(tool->tl_windowfd);
	}
	
	(void)input_imnull(&im);
	(void)win_getinputmask(tool->tl_windowfd, &im, &designee);
	win_setinputcodebit(&im, MS_RIGHT);
	win_setinputcodebit(&im, MS_LEFT);
	win_setinputcodebit(&im, MS_MIDDLE);
	im.im_flags |= IM_ASCII;
	(void)win_setinputmask(tool->tl_windowfd, &im, (struct inputmask *)0,
	    designee);

	/*
	 * Install tool
	 */
	(void)signal(SIGWINCH, sigwinched);
	(void)signal(SIGTERM, killkids);
	(void)signal(SIGINT, killkids);
	(void)tool_install(tool);

	/*
	 * Main loop
	 */
	do {
		wantredisplay = 0;
		meter_paint();
		(void)tool_select(tool, 0);
	} while (wantredisplay);

	/*
	 * Cleanup
	 */
	(void)pf_close(pfont);
	(void)tool_destroy(tool);
	(void)pr_destroy(metericon.ic_mpr);
	killkids();
	exit(0);
}

usage(name)
	char *name;
{
	register int i;
	
	(void)fprintf(stderr, "Usage: %s [-m minutehandintv] [-h hourhandintv] \
[-s sampletime] [-v value] [-M value minmax maxmax] [hostname]\n", name);
	(void)fprintf(stderr, "value is one of: ");
	for (i = 0; i < length; i++)
		(void)fprintf(stderr, " %s", meters[i].m_name);
	(void)fprintf(stderr,"\n");
	exit(1);
}

/*
 * SIGWINCH signal catcher.
 */
sigwinched()
{

	(void)tool_sigwinch(tool);
}

/*
 * SIGWINCH handler for perfmeter tool window.
 */
meter_sigwinhandler()
{
	struct rect rect;

	(void)win_getsize(tool->tl_windowfd, &rect);
	width = rect.r_width - 2*BORDER;
	height = rect.r_height - 2*BORDER - NAMEHEIGHT - 1;
	if (remote)
		height -= NAMEHEIGHT;
	old_sigwinch(tool);
	meter_paint();
}

/*
 * New selection handler for perfmeter tool window.
 */
/*ARGSUSED*/
meter_selected(data, ibits, obits, ebits, timer)
	char *data;
	int *ibits, *obits, *ebits;
	register struct timeval **timer;
{
	struct	inputevent ie;
	struct	menuitem *mi;
	int	iconic = tool->tl_flags&TOOL_ICONIC;
	int	n;

	if (*timer && ((*timer)->tv_sec == 0) && ((*timer)->tv_usec == 0) &&
	    *ibits == 0) {
		updatedata();
		(*timer)->tv_sec = sampletime;
		meter_update();
		goto out;
	}
	if (input_readevent(tool->tl_windowfd, &ie) == -1) {
		perror("meter: input_readevent");
		exit(1);
	}

	switch (ie.ie_code) {
	case SELECT_BUT:
		if (event_ctrl_is_down(&ie))	  	/* Full */	
			wmgr_full(tool, rootfd);
		else if (event_shift_is_down(&ie)) 	/* Hide */
			wmgr_bottom(tool->tl_windowfd, rootfd);
		else if (iconic) 			/* Open */
			wmgr_open(tool->tl_windowfd, rootfd);
		else					/* Expose */
			wmgr_top(tool->tl_windowfd, rootfd);
		break;
	case MS_MIDDLE:
		if (!iconic && (tool_moveboundary(tool, &ie) != -1)) {
			/*
			 * Moved boundary while in boundary stripe
			 */
		 } else {
			/*
			 * Do move/stretch operation without prompt
			 * If the ctrl key is down, do an accelerated
			 * stretch.
			 */
			wmgr_changerect(tool->tl_windowfd, tool->tl_windowfd,
			    &ie, !event_ctrl_is_down(&ie), TRUE);
		}
		break;
	  case MENU_BUT: /* Do menus */
		if (walking) {
			n = (int) menu_show_using_fd(wmenu,
			    tool->tl_windowfd, &ie);
			if (menu_get(wmenu, MENU_VALID_RESULT)) {
				visible = n;
				meter_paint();
			}
		}
		else {
			wmgr_setupmenu(tool->tl_windowfd);
			mi = menu_display(&menu_ptr, &ie, tool->tl_windowfd);
			if (mi == NULL)
				break;
			if (menu_ptr == &menu_body) {
				visible = (int)mi->mi_data;
				meter_paint();
			} else if (menu_ptr == wmgr_toolmenu) {
				if (wmgr_handletoolmenuitem(wmgr_toolmenu, mi,
				    tool->tl_windowfd, rootfd) == -1) {
					tool->tl_io.tio_selected=old_selected;
					(void)tool_done(tool);
					goto out;
				}
			}
		}
		break;
	case OPEN_KEY:
                if (win_inputposevent(&ie))
 			break;
 		if (tool->tl_flags & TOOL_ICONIC)
                         wmgr_open(tool->tl_windowfd, rootfd);
 		else
                         wmgr_close(tool->tl_windowfd, rootfd);
 		break;
	case TOP_KEY:
                 if (win_inputposevent(&ie))
 			break;
                 if (event_shift_is_down(&ie) ||
                     win_getlink(tool->tl_windowfd, WL_COVERING) ==
                     WIN_NULLLINK)
                         wmgr_bottom(tool->tl_windowfd, rootfd);
                 else
                         wmgr_top(tool->tl_windowfd, rootfd);
                 break;
	case '1': 
		sampletime = 1;
		break;
	case '2': 
		sampletime = 2;
		break;
	case '3': 
		sampletime = 3;
		break;
	case '4': 
		sampletime = 4;
		break;
	case '5': 
		sampletime = 5;
		break;
	case '6': 
		sampletime = 6;
		break;
	case '7': 
		sampletime = 7;
		break;
	case '8': 
		sampletime = 8;
		break;
	case '9': 
		sampletime = 9;
		break;
	case 'h':	/* hour hand */
		if (hourhandintv > 0)
			hourhandintv--;
		break;
	case 'H':	/* hour hand */
		hourhandintv++;
		break;
	case 'm':	/* minute hand */
		if (minutehandintv > 0)
			minutehandintv--;
		break;
	case 'M':	/* minute hand */
		minutehandintv++;
		break;
	}
	shortexp = (1 - ((double)sampletime/
	    max(hourhandintv, sampletime))) * FSCALE;
	longexp = (1 - ((double)sampletime/
	    max(minutehandintv, sampletime))) * FSCALE;
out:
	*ibits = *obits = *ebits = 0;
}

/*
 * SIGCHLD signal catcher.
 * Harvest any child (from keeptrying).
 * If can now contact host, request redisplay.
 */
ondeath()
{
	union wait status;

	while (wait3(&status, WNOHANG, (struct rusage *)0) > 0)
		;
	if (setup() < 0)
		keeptrying();
	else {
		dead = 0;
		/*
		 * Can't do redisplay from interrupt level
		 * so set flag and do a tool_done.
		 */
		wantredisplay = 1;
		(void)tool_done(tool);
	}
}

/*
 * Convert raw data into properly scaled and averaged data
 * and save it for later redisplay.
 */
updatedata()
{
	register int i, *dp, old, tmp;
	register struct meter *mp;
	
	if (dead)
		return;
	dp = getdata();
	if (dp == NULL) {
		dead = 1;
		meter_paint();
		keeptrying();
		return;
	}

	/* 
	 * Don't have to worry about save[old] being -1 the
         * very first time thru, because we are called
	 * before save is initialized to -1.
	 */
	old = saveptr;
	if (++saveptr == MAXSAVE)
		saveptr = 0;
	for (i = 0, mp = meters; i < length; i++, mp++, dp++) {
		if (*dp < 0)	/* should print out warning if this happens */
			*dp = 0;
		tmp = (longexp * save_access(i, old) +
		    (*dp * FSCALE)/mp->m_scale * (FSCALE - longexp)) >> FSHIFT;
		if (tmp < 0)	/* check for wraparound */
			tmp = mp->m_curmax * FSCALE;
		save_access(i, saveptr) = tmp;
		tmp = (shortexp * mp->m_longave +
		    (*dp * FSCALE/mp->m_scale) * (FSCALE - shortexp)) >> FSHIFT;
		if (tmp < 0)	/* check for wraparound */
			tmp = mp->m_curmax * FSCALE;
		mp->m_longave = tmp;
	}
}

getminutedata()
{
	
	return (save_access(visible, saveptr));
}

gethourdata()
{
	
	return (meters[visible].m_longave);
}

/*
 * Initialize the connection to the metered host.
 */

static	CLIENT *client, *oldclient;

setup()
{
	struct hostent *hp;
	struct timeval timeout;
	struct sockaddr_in serveradr;
	int snum;

	snum = RPC_ANYSOCK;
	bzero((char *)&serveradr, sizeof (serveradr));
	if (hostname) {
		if ((hp = gethostbyname(hostname)) == NULL) {
			(void)fprintf(stderr,
			    "Sorry, host %s not in hosts database\n",
			     hostname);
			exit(1);
		}
		bcopy(hp->h_addr, (char *)&serveradr.sin_addr, hp->h_length);
	}
	else {
		if (hp = gethostbyname("localhost"))
			bcopy(hp->h_addr, (char *)&serveradr.sin_addr,
			    hp->h_length);
		else
			serveradr.sin_addr.s_addr = inet_addr("127.0.0.1");
	}
	serveradr.sin_family = AF_INET;
	serveradr.sin_port = 0;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	if ((client = clntudp_bufcreate(&serveradr, RSTATPROG,
	    RSTATVERS_SWTCH, timeout, &snum, sizeof(struct rpc_msg),
	    sizeof(struct rpc_msg) + sizeof(struct statsswtch))) == NULL)
		return (-1);
	if (oldsocket >= 0)
		(void)close(oldsocket);
	oldsocket = snum;
	if (oldclient)
		clnt_destroy(oldclient);
	oldclient = client;
	return (0);
}

/*
 * Fork a separate process to keep trying to contact the host
 * so that the main process can continue to service window
 * requests (repaint, move, stretch, etc.).
 */
keeptrying()
{	
	int pid;
	
	if ((int)signal(SIGCHLD, ondeath) == -1)
		perror("signal");
	pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(1);
	}
	if (pid == 0) {
		for (;;) {
			sleep(1);
			if (setup() < 0)
				continue;
			if (clnt_call(client, NULLPROC, xdr_void, 0,
			    xdr_void, 0, TIMEOUT) != RPC_SUCCESS)
				continue;
			exit(0);
		}
	}
}

/*
 * Kill any background processes we may've started.
 */
killkids()
{

	(void)signal(SIGINT, SIG_IGN);
	(void)signal(SIGCHLD, SIG_IGN);
	(void)killpg(getpgrp(0), SIGINT);	/* get rid of forked processes */
	exit(0);
}

/*
 * Get the metered data from the host via RPC
 * and process it to compute the actual values
 * (rates) that perfmeter wants to see.
 */

/* static data used only by getdata() */
static	struct statsswtch statsswtch;
static	int oldtime[CPUSTATES];
static	int total;			/* Default to zero */
static	int toterr;			/* Default to zero */
static	int totcoll;			/* Default to zero */
static	struct timeval tm, oldtm;
static	int xfer1[DK_NDRIVE];
static	int badcnt;			/* Default to zero */
static	int ans[MAXMETERS];
static	int oldi, olds, oldp, oldsp;
static	int getdata_init_done;		/* Default to zero */

int *
getdata()
{
	register int i, t;
	register int msecs;
	int maxtfer;
	enum clnt_stat clnt_stat;
	int intrs, swtchs, pag, spag;
	int sum, ppersec;

	clnt_stat = clnt_call(client, RSTATPROC_STATS, xdr_void, 0,
		xdr_statsswtch,  &statsswtch, TIMEOUT);
	if (clnt_stat == RPC_TIMEDOUT)
		return (NULL);		
	if (clnt_stat != RPC_SUCCESS) {
		clnt_perror(client, "cpugetdata");
		exit(1);
	}

	for (i = 0; i < CPUSTATES; i++) {
		t = statsswtch.cp_time[i];
		statsswtch.cp_time[i] -= oldtime[i];
		oldtime[i] = t;
	}
	t = 0;
	for (i = 0; i < CP_IDLE; i++)	/* assume IDLE is last */
		t += statsswtch.cp_time[i];
	if (statsswtch.cp_time[CP_IDLE] + t <= 0) {
		t++;
		badcnt++;
		if (badcnt >= TRIES) {
			(void)fprintf(stderr, 
					"perfmeter: rstatd on %s returned bad data\n",
					hostname);
			exit(1);
		}
	} else {
		badcnt = 0;
	}
	ans[CPU] = (100*t) / (statsswtch.cp_time[CP_IDLE] + t);

	(void)gettimeofday(&tm, (struct timezone *)0);
	msecs = (1000*(tm.tv_sec - oldtm.tv_sec) +
	    (tm.tv_usec - oldtm.tv_usec)/1000);

	sum = statsswtch.if_ipackets + statsswtch.if_opackets;
	ppersec = 1000*(sum - total) / msecs;
	total = sum;
	ans[PKTS] = ppersec;

	ans[COLL] = FSCALE*(statsswtch.if_collisions - totcoll)*1000 / msecs;
	totcoll = statsswtch.if_collisions;
	ans[ERR] = FSCALE*(statsswtch.if_ierrors - toterr)*1000 / msecs;
	toterr = statsswtch.if_ierrors;

	if (!getdata_init_done) {
		pag = 0;
		spag = 0;
		intrs = 0;
		swtchs = 0;
		getdata_init_done = 1;
	} else {
		pag = statsswtch.v_pgpgout + statsswtch.v_pgpgin - oldp;
		pag = 1000*pag / msecs;
		spag = statsswtch.v_pswpout + statsswtch.v_pswpin - oldsp;
		spag = 1000*spag / msecs;
		intrs = statsswtch.v_intr - oldi;
		intrs = 1000*intrs / msecs;
		swtchs = statsswtch.v_swtch - olds;
		swtchs = 1000*swtchs / msecs;
	}
	oldp = statsswtch.v_pgpgin + statsswtch.v_pgpgout;
	oldsp = statsswtch.v_pswpin + statsswtch.v_pswpout;
	oldi = statsswtch.v_intr;
	olds = statsswtch.v_swtch;
	ans[PAGE] = pag;
	ans[SWAP] = spag;
	ans[INTR] = intrs;
	ans[CNTXT] = swtchs;
	ans[LOAD] = statsswtch.avenrun[0];
	for (i = 0; i < DK_NDRIVE; i++) {
		t = statsswtch.dk_xfer[i];
		statsswtch.dk_xfer[i] -= xfer1[i];
		xfer1[i] = t;
	}
	maxtfer = statsswtch.dk_xfer[0];
	for (i = 1; i < DK_NDRIVE; i++)
		if (statsswtch.dk_xfer[i] > maxtfer)
			maxtfer = statsswtch.dk_xfer[i];
	maxtfer = (1000*maxtfer) / msecs;
	ans[DISK] = maxtfer;
	oldtm = tm;
	return (ans);
}
