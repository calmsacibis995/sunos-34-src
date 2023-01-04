#ifndef lint
static  char sccsid[] = "@(#)suntools.c 1.7 87/05/04 SMI";
#endif

/*
 * Sun Microsystems, Inc.
 */

/*
 * Root window: Provides the background window for a screen.
 *	Put up environment manager menu.
 */

#include <suntool/tool_hs.h>
#include <sys/ioctl.h>
#include <sys/dir.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <pwd.h>
#include <ctype.h>
#include <sunwindow/defaults.h>
#include <suntool/menu.h>
#include <suntool/wmgr.h>
#include <suntool/selection.h>
#include <suntool/selection_svc.h>
#include <suntool/walkmenu.h>
#include <suntool/icon.h>
#include <suntool/icon_load.h>

#define Pkg_private

Pkg_private int walk_getrootmenu(), walk_handlerootmenuitem();
static int wmgr_getrootmenu(), wmgr_handlerootmenuitem();
void	expand_path();
int	wmgr_forktool();

#define GET_MENU (walk ? walk_getrootmenu : wmgr_getrootmenu)
#define MENU_ITEM (walk ? walk_handlerootmenuitem : wmgr_handlerootmenuitem)

#define KEY_PUT		KEY_LEFT(6)
#define KEY_CLOSE	KEY_LEFT(7)
#define KEY_GET		KEY_LEFT(8)
#define KEY_FIND	KEY_LEFT(9)
#define KEY_DELETE	KEY_LEFT(10)

#define MAXPATHLEN	1024

extern int            errno;

extern char          *malloc(),
		     *calloc(),
		     *getenv(),
		     *strcpy(),
		     *strncat(),
		     *strncpy(), 
		     *index();

extern struct pixrect *pr_load();

static void	      root_winmgr(),
		      root_sigchldhandler(),
		      root_sigwinchhandler(),
		      root_sigchldcatcher(),
		      root_sigwinchcatcher(),
		      root_initialsetup(),
		      root_set_background(),
		      root_set_pattern(),
		      root_start_service();

Pkg_private char *wmgr_savestr();

static int            dummy_proc();
static struct selection empty_selection = {
	SELTYPE_NULL, 0, 0, 0, 0
};

static Seln_client    root_seln_handle;
static char	     *seln_svc_file;

static int            rootfd,
		      rootnumber,
		      root_SIGCHLD,
		      root_SIGWINCH;

static int walk;

static struct screen  screen;

static struct pixwin *pixwin;

#define	ROOTMENUITEMS	20
#define	ROOTMENUFILE	"/usr/lib/rootmenu"
#define ROOTMENUNAME	"Suntools"
#define ARG_CHARS       1024

static struct menuitem       root_items[ROOTMENUITEMS];

struct menuitemstrings {
	char         *mis_prog;    /* program to call */
	char         *mis_args;    /* args to program */
} root_itemstrings[ROOTMENUITEMS];

char                 *rootmenufile;

struct menu           wmgr_rootmenubody;
Pkg_private Menu      wmgr_rootmenu;

struct stat_rec {
    char             *name;	   /* Dynamically allocated menu file name */
    time_t            mftime;      /* Modified file time */
};

#define	MAX_FILES	40
Pkg_private struct stat_rec wmgr_stat_array[MAX_FILES];
Pkg_private int            wmgr_nextfile;

static	enum root_image_type {
	ROOT_IMAGE_PATTERN,
	ROOT_IMAGE_SOLID,
} root_image_type;
static struct pixrect *root_image_pixrect;
static root_color;

#ifdef STANDALONE
main(argc, argv)
#else
suntools_main(argc, argv)
#endif
	int argc;
	char **argv;
{
	char	name[WIN_NAMESIZE], setupfile[MAXNAMLEN];
	int	placeholderfd;
	int	donosetup = 0, printname = 0;
	char   *root_background_file = 0;
	unsigned char red[256], green[256], blue[256];
	register struct	pixrect *fb_pixrect;
#define DONT_CARE_SHIFT         -1
        Firm_event fe_focus;
        int fe_focus_shift;
        Firm_event fe_restore_focus;
        int fe_restore_focus_shift;
        int set_focus = 0, set_restore_focus = 0, set_sync_tv = 0;
        struct timeval sync_tv;
	char *font_name;
	char    **args;
	struct singlecolor single_color;
	register int i;

	seln_svc_file = "selection_svc";
	root_image_type = ROOT_IMAGE_PATTERN;
	root_image_pixrect = tool_bkgrd;
	root_color = -1;
	root_set_pattern(defaults_get_string("/SunView/Root_Pattern",
	    "on", (int *)NULL), &single_color);
	if (defaults_get_boolean("/SunView/Click_to_Type", (Bool)FALSE, (int *)NULL)) {
		/* 's'plit focus (See -S below if change) */
		fe_focus.id = MS_LEFT;
		fe_focus.value = 1;
		fe_focus_shift = DONT_CARE_SHIFT;
		set_focus = 1;
		fe_restore_focus.id = MS_MIDDLE;
		fe_restore_focus.value = 1;
		fe_restore_focus_shift = DONT_CARE_SHIFT;
		set_restore_focus = 1;
	}
	
	font_name = defaults_get_string("/SunView/Font", "", (int *)NULL);
	if (*font_name != '\0') (void)setenv("DEFAULT_FONT", font_name);
	
	/*
	 * Parse cmd line.
	 */
	setupfile[0] = NULL;
	(void)win_initscreenfromargv(&screen, argv);
	if (argv) {
                for (args = ++argv;*args;args++) {
                        if ((strcmp(*args, "-s") == 0) && *(args+1)) {
				(void) strcpy(setupfile, *(args+1));
                                args++;
                        } else if (strcmp(*args, "-F") == 0) {
                                root_color = -1;
                                root_image_type = ROOT_IMAGE_SOLID;
                        } else if (strcmp(*args, "-B") == 0) {
                                root_color = 0;
                                root_image_type = ROOT_IMAGE_SOLID;
                        } else if (strcmp(*args, "-P") == 0)
				root_set_pattern("on", &single_color);
                        else if (strcmp(*args, "-n") == 0)
                                donosetup = 1;
                        else if (strcmp(*args, "-p") == 0)
                                printname = 1;
                        else if (strcmp(*args, "-color") == 0) {
				if (args_remaining(args) < 4)
					goto Arg_Count_Error;
				++args;
                                if (win_getsinglecolor(&args, &single_color))
                                	continue;
                                root_color = 1;
                        } else if (strcmp(*args, "-background") == 0) {
				if (args_remaining(args) < 2)
					goto Arg_Count_Error;
                                root_background_file = *++args;
                        } else if (strcmp(*args, "-pattern") == 0) {
				if (args_remaining(args) < 2)
					goto Arg_Count_Error;
				++args;
				root_set_pattern(*args, &single_color);
                        } else if (strcmp(*args, "-svc") == 0) {
				if (args_remaining(args) < 2)
					goto Arg_Count_Error;
                                seln_svc_file = *++args;
                                (void)fprintf(stderr,
                                	"Starting selection service \"%s\"\n",
                                	seln_svc_file);
			} else if (strcmp(*args, "-S") == 0) {
				/*
				 * 's'plit focus (See Click_to_type above
				 * if change)
				 */
				fe_focus.id = MS_LEFT;
				fe_focus.value = 1;
				fe_focus_shift = DONT_CARE_SHIFT;
				set_focus = 1;
				fe_restore_focus.id = MS_MIDDLE;
				fe_restore_focus.value = 1;
				fe_restore_focus_shift = DONT_CARE_SHIFT;
				set_restore_focus = 1;
			} else if (strcmp(*args, "-c") == 0) {
				/* set 'c'aret */
				get_focus_from_args(&args, (short *)&fe_focus.id,
				    &fe_focus.value, &fe_focus_shift);
				set_focus = 1;
			} else if (strcmp(*args, "-r") == 0) {
				/* 'r'estore caret */
				get_focus_from_args(&args, (short *)&fe_restore_focus.id,
				    &fe_restore_focus.value,
				    &fe_restore_focus_shift);
				set_restore_focus = 1;
			} else if (strcmp(*args, "-t") == 0) {
				/* set 't'imeout */
				if (args_remaining(args) < 2)
					goto Arg_Count_Error;
				args++;
				sync_tv.tv_usec = 0;
				sync_tv.tv_sec = atoi(*args);
				set_sync_tv = 1;
			} else if (argc == 2 && *args[0] != '-')
				/*
				 * If only arg and not a flag then treat as
				 * setupfile (backward compatibility with 1.0).
				 */
				(void) strcpy(setupfile, *args);
                }
        }

	/*
	 * Initialize root menu from menu file.
	 */
	rootmenufile = defaults_get_string("/SunView/Rootmenu_filename", "",
					   (int *)NULL);
	if (*rootmenufile == '\0'
	    && (rootmenufile = getenv("ROOTMENU")) == NULL)
	    rootmenufile = ROOTMENUFILE;
	if (defaults_get_boolean("/SunView/Walking_Menus", (Bool)FALSE, (int *)NULL))
	    walk = 1;
	wmgr_rootmenu = walk ? NULL : (Menu)&wmgr_rootmenubody;
	if (GET_MENU(ROOTMENUNAME, wmgr_rootmenu, rootmenufile,
		     root_items, root_itemstrings, ROOTMENUITEMS) <= 0) {
	    (void)fprintf(stderr, "suntools: invalid root menu\n");
	    exit(1);
	}
	/*
	 * Set up signal catchers.
	 */
	(void) signal(SIGCHLD, (int (*)())(LINT_CAST(root_sigchldcatcher)));
	(void) signal(SIGWINCH, (int (*)())(LINT_CAST(root_sigwinchcatcher)));
	/*
	 * Find out what colormap is so can restore later.
	 * Do now before call win_screennew which changes colormap.
	 */
	if (screen.scr_fbname[0] == NULL)
		(void)strcpy(screen.scr_fbname, "/dev/fb");
	if ((fb_pixrect = pr_open(screen.scr_fbname)) == (struct pixrect *)0) {
		(void)fprintf(stderr, "suntools: invalid frame buffer %s\n",
		    screen.scr_fbname);
		exit(1);
	}
	(void)pr_getcolormap(fb_pixrect, 0, 256, red, green, blue);
	/*
	 * Create root window
	 */
	if ((rootfd = win_screennew(&screen)) == -1) {
		perror("suntools");
		exit(1);
	}
        if (root_background_file != (char *) NULL)
        	root_set_background(root_background_file);
	if (root_color != 1 && root_image_type == ROOT_IMAGE_SOLID) {
		Cursor	cursor = cursor_create((char *)0);

		(void)win_getcursor(rootfd, cursor);
		(void)cursor_set(cursor, CURSOR_OP, PIX_SRC^PIX_DST, 0);
		(void)win_setcursor(rootfd, cursor);
		cursor_destroy(cursor);
	}
	/* Set input parameters */
	if (set_focus)
		if (win_set_focus_event(rootfd, &fe_focus, fe_focus_shift)) {
			perror("win_set_focus_event");
			exit(1);
		}
	if (set_restore_focus)
		if (win_set_swallow_event(rootfd,
		    &fe_restore_focus, fe_restore_focus_shift)){
			perror("win_set_swallow_event");
			exit(1);
		}
	if (set_sync_tv)
		if (win_set_event_timeout(rootfd, &sync_tv)) {
			perror("win_set_event_timeout");
			exit(1);
		}
	(void)win_screenget(rootfd, &screen);
	/*
	 * Open pixwin.
	 */
	if ((pixwin = pw_open_monochrome(rootfd)) == 0) {
                (void)fprintf(stderr, "%s not available for window system usage\n",
		    screen.scr_fbname);
                perror("suntools");
                exit(1);
        }
        /* Set up own color map if have own color */
        if (root_color == 1) {
        	char cmsname[CMS_NAMESIZE];
        	int color;
#define	ROOT_CMS_SIZE 4
        	u_char root_red[ROOT_CMS_SIZE], root_green[ROOT_CMS_SIZE],
        	    root_blue[ROOT_CMS_SIZE];
        	
        	root_red[0] = screen.scr_background.red;
        	root_green[0] = screen.scr_background.green;
        	root_blue[0] = screen.scr_background.blue;
		for (color = 1; color < ROOT_CMS_SIZE-1; color++) {
        		root_red[color] = single_color.red;
        		root_green[color] = single_color.green;
        		root_blue[color] = single_color.blue;
		}
        	root_red[ROOT_CMS_SIZE-1] = screen.scr_foreground.red;
        	root_green[ROOT_CMS_SIZE-1] = screen.scr_foreground.green;
        	root_blue[ROOT_CMS_SIZE-1] = screen.scr_foreground.blue;
        	(void)sprintf(cmsname, "rootcolor%D", getpid());
        	(void)pw_setcmsname(pixwin, cmsname);
		(void)pw_putcolormap(pixwin, 0, ROOT_CMS_SIZE,
		    root_red, root_green, root_blue);
        }
       	/*
	 * Set up root''s name in environment
	 */
	(void)win_fdtoname(rootfd, name);
	rootnumber = win_nametonumber(name);
	(void)we_setparentwindow(name);
	if (printname)
		(void)fprintf(stderr, "suntools window name is %s\n", name);
	/*
	 * Steal a window for the tool slot allocator
	 * & stash its name in the environment
	 */
	if ((placeholderfd = win_getnewwindow()) == -1)  {
		(void)fprintf(stderr,
			"No window available for placing open windows\n");
		perror("suntools");
	} else {
		(void)win_fdtoname(placeholderfd, name);
		(void)setenv("WMGR_ENV_PLACEHOLDER", name);
	}
	/*
	 * Set up tool slot allocator
	 */
	(void)wmgr_init_icon_position(rootfd);
	(void)wmgr_init_tool_position(rootfd);
	/*
	 * Setup tty parameters for all terminal emulators that will start.
	 */
	{int tty_fd;
	tty_fd = open("/dev/tty", O_RDWR, 0);
	if (tty_fd < 0)
		(void)ttysw_saveparms(2);	/* Try stderr */
	else {
		(void)ttysw_saveparms(tty_fd);
		(void) close(tty_fd);
	}
	}
	/*	try to make sure there is a selection service
	 */
#define	SEL_SVC_SEC_WAIT	5
#define	SEL_CLIENT_NULL		(Seln_client) NULL
	if ((root_seln_handle = seln_create((void (*)())(LINT_CAST(dummy_proc)), 
		(Seln_result (*)())(LINT_CAST(dummy_proc)), 
	    (char *)(LINT_CAST(&root_seln_handle)))) == SEL_CLIENT_NULL) {
		root_start_service();
		for (i = 0;
		   i < SEL_SVC_SEC_WAIT && root_seln_handle == SEL_CLIENT_NULL;
		   i++) {
			sleep(1);
			root_seln_handle = seln_create(
				(void (*)())(LINT_CAST(dummy_proc)),
				(Seln_result (*)())(LINT_CAST(dummy_proc)),
				(char *)(LINT_CAST(&root_seln_handle))
			);
		}
		if (root_seln_handle == SEL_CLIENT_NULL)
			(void)fprintf(stderr,
		        "Can't find old, or start new, selection service\n");
	}
	/*
	 * Draw background.
	 */
	root_sigwinchhandler();
	/*
	 * Do initial window setup.
	 */
	if (!donosetup)
		root_initialsetup(setupfile);
	/*
	 * Do window management loop.
	 */
	root_winmgr();
	/*
	 * Destroy screen sends SIGTERMs to all existing windows and
	 * wouldn''t let any windows install themselves in the window tree.
	 * Calling process of win_screedestroy is spared SIGTERM.
	 */
	(void)win_screendestroy(rootfd);
	(void)close(placeholderfd);
	/*
	 * Lock screen before clear so don''t clobber frame buffer while
	 * cursor moving.
	 */
	(void)pw_lock(pixwin, &screen.scr_rect);
	/*
	 * Clear available plane groups
	 */
	for (i = 0; i < PIX_MAX_PLANE_GROUPS; i++) {
		if (pixwin->pw_clipdata->pwcd_plane_groups_available[i]) {
			/* Write to all plane groups */
			pr_set_planes(pixwin->pw_pixrect, i, PIX_ALL_PLANES);
			/* Clear screen */
			(void)pr_rop(pixwin->pw_pixrect,
			    screen.scr_rect.r_left, screen.scr_rect.r_top,
			    screen.scr_rect.r_width, screen.scr_rect.r_height,
			    PIX_CLR, (Pixrect *)0, 0, 0);
			/* Reset previous colormap */
			(void)pr_putcolormap(pixwin->pw_pixrect, 0, 256,
			    red, green, blue);
		}
	}
	/*
	 * Unlock screen.
	 */
	(void)pw_unlock(pixwin);
	exit(0);
Arg_Count_Error:
	(void)fprintf(stderr, "%s arg count error\n", args);
	exit (-1);
}

static
args_remaining(args)
		char **args;
{
	register i;

	for (i = 0; *(args+i); i++) {}
	return (i);
}

static
get_focus_from_args(argv_ptr, event, value, shift)
	char ***argv_ptr;
	register short *event;
	register int *value;
	register int *shift;
{
#define	SHIFT_MASK(bit) (1 << (bit))
	char str[200];
	register char *arg;

	if (args_remaining(*argv_ptr) < 4) {
		(void)fprintf(stderr, "%s arg count error\n", *argv_ptr);
		exit (-1);
	}
	(*argv_ptr)++;
	arg = **argv_ptr;
	if (strcmp(arg, "LOC_WINENTER") == 0)
		*event = LOC_WINENTER;
	else if (strcmp(arg, "MS_LEFT") == 0)
		*event = MS_LEFT;
	else if (strcmp(arg, "MS_MIDDLE") == 0)
		*event = MS_MIDDLE;
	else if (strcmp(arg, "MS_RIGHT") == 0)
		*event = MS_RIGHT;
	else if (sscanf(arg, "BUT%s", str) == 1)
		*event = atoi(str)+BUT_FIRST;
	else if (sscanf(arg, "KEY_LEFT%s", str) == 1)
		*event = atoi(str)+KEY_LEFTFIRST-1;
	else if (sscanf(arg, "KEY_RIGHT%s", str) == 1)
		*event = atoi(str)+KEY_RIGHTFIRST-1;
	else if (sscanf(arg, "KEY_TOP%s", str) == 1)
		*event = atoi(str)+KEY_TOPFIRST-1;
	else if (strcmp(arg, "KEY_BOTTOMLEFT") == 0)
		*event = KEY_BOTTOMLEFT;
	else if (strcmp(arg, "KEY_BOTTOMRIGHT") == 0)
		*event = KEY_BOTTOMRIGHT;
	else
		*event = atoi(arg);
	(*argv_ptr)++;
	arg = **argv_ptr;
	if (strcmp(arg, "DOWN") == 0 || strcmp(arg, "Down") == 0 ||
	    strcmp(arg, "down") == 0)
		*value = 1;
	else if (strcmp(arg, "ENTER") == 0 || strcmp(arg, "Enter") == 0 ||
	    strcmp(arg, "enter") == 0)
		*value = 1;
	else if (strcmp(arg, "UP") == 0 || strcmp(arg, "Up") == 0 ||
	    strcmp(arg, "up") == 0)
		*value = 0;
	else
		*value = atoi(arg);
	(*argv_ptr)++;
	arg = **argv_ptr;
	if (strcmp(arg, "SHIFT_LEFT") == 0)
		*shift = SHIFT_MASK(LEFTSHIFT);
	else if (strcmp(arg, "SHIFT_RIGHT") == 0)
		*shift = SHIFT_MASK(RIGHTSHIFT);
	else if (strcmp(arg, "SHIFT_LEFTCTRL") == 0)
		*shift = SHIFT_MASK(LEFTCTRL);
	else if (strcmp(arg, "SHIFT_RIGHTCTRL") == 0)
		*shift = SHIFT_MASK(RIGHTCTRL);
	else if (strcmp(arg, "SHIFT_DONT_CARE") == 0)
		*shift = DONT_CARE_SHIFT;
	else if (strcmp(arg, "SHIFT_ALL_UP") == 0)
		*shift = 0;
	else
		*shift = atoi(arg);

}

static void
root_winmgr()
{
    struct inputmask      im;
    struct inputevent     event;
    int                   keyexit = 0;

    /*
     * Set up input mask so can do menu stuff 
     */
    (void)input_imnull(&im);
    im.im_flags |= IM_NEGEVENT;
    im.im_flags |= IM_ASCII;
    win_setinputcodebit(&im, SELECT_BUT);
    win_setinputcodebit(&im, MENU_BUT);
    win_setinputcodebit(&im, WIN_STOP);
    win_setinputcodebit(&im, KBD_REQUEST);
    win_setinputcodebit(&im, KEY_PUT);
    win_setinputcodebit(&im, KEY_GET);
    win_setinputcodebit(&im, KEY_FIND);
    win_setinputcodebit(&im, KEY_DELETE);
    (void)win_setinputmask(rootfd, &im, (struct inputmask *) 0, WIN_NULLLINK);
    /*
     * Read and invoke menu items 
     */
    for (;;) {
	int                   ibits, nfds;

	/*
	 * Use select (to see if have input) so will return on SIGWINCH or
	 * SIGCHLD. 
	 */
	ibits = 1 << rootfd;
	do {
	    if (root_SIGCHLD)
		root_sigchldhandler();
	    if (root_SIGWINCH)
		root_sigwinchhandler();
	} while (root_SIGCHLD || root_SIGWINCH);
	nfds = select(NOFILE, &ibits, (int *) 0, (int *) 0,
		      (struct timeval *) 0);
	if (nfds == -1) {
	    if (errno == EINTR)
		/*
		 * Go around again so that signals can be handled.  ibits may
		 * be non-zero but should be ignored in this case and they will
		 * be selected again. 
		 */
		continue;
	    else {
		perror("suntools");
		break;
	    }
	}
	if (ibits & (1 << rootfd)) {
	    /*
	     * Read will not block. 
	     */
	    if (input_readevent(rootfd, &event) < 0) {
		if (errno != EWOULDBLOCK) {
		    perror("suntools");
		    break;
		}
	    }
	} else
	    continue;

	switch (event.ie_code) {
	  case CTRL(q):	       /* Escape for getting out	 */
	    if (keyexit) {	       /* when no mouse around	 */
		return;
	    }
	    continue;
	  case CTRL(d):
	    keyexit = 1;
	    continue;
	  case WIN_STOP:
	    if (root_seln_handle != (Seln_client) NULL) {
		seln_clear_functions();
	    }
	    break;
	  case KBD_REQUEST:	/* Always refuse keyboard focus request */
	    (void)win_refuse_kbd_focus(rootfd);
	    break;
	  case KEY_PUT:
	  case KEY_GET:
	  case KEY_FIND:
	  case KEY_DELETE:
	    if (root_seln_handle != (Seln_client) NULL) {
		seln_report_event((Seln_client)(LINT_CAST(
			root_seln_handle)), &event);
	    }
	    break;
	  case KEY_CLOSE:
	    break;
	  case MS_LEFT:
	    if (win_inputposevent(&event)) {
		/*  the left button went down; clear the selection.  */
		(void)selection_set(&empty_selection, dummy_proc, dummy_proc, rootfd);
		if (root_seln_handle != (Seln_client) NULL) {
		    Seln_rank             rank;

		    rank = seln_acquire((Seln_client)(LINT_CAST(
		    	root_seln_handle)), SELN_UNSPECIFIED);
		    (void)seln_done((Seln_client)(LINT_CAST(root_seln_handle)),
		    	 rank);
		}
	    }
	    break;
	  case MENU_BUT:
	    if (win_inputposevent(&event)) {
		if (!root_menu_mgr(&event)) {
		    return;
		}
	    }
	    break;
	  default:
	    break;
	}
	keyexit = 0;
    }
}

extern struct menuitem *menu_display();

static int
root_menu_mgr(event)
	struct inputevent    *event;
{
	Menu_item     mi; /* Old and new menu item */
	int	      exit_local;

	if (GET_MENU(ROOTMENUNAME, wmgr_rootmenu, rootmenufile,
		     root_items, root_itemstrings, ROOTMENUITEMS) <= 0) {
	    (void)fprintf(stderr, "suntools: invalid root menu\n");
	    return 1;
	}
	for (;;) {
	    struct inputevent tevent;

	    exit_local = 0;
	    tevent = *event;
	    if (walk)
		mi = menu_show_using_fd(wmgr_rootmenu, rootfd, event);
	    else
		mi = (Menu_item)menu_display((struct menu **)(LINT_CAST(
			&wmgr_rootmenu)), event, rootfd);
	    if (mi)
		exit_local = MENU_ITEM(wmgr_rootmenu, mi, rootfd) == -1;
	    if (event->ie_code == MS_LEFT && !exit_local) {
		*event = tevent;
/*		win_setmouseposition(rootfd, event->ie_locx, event->ie_locy);*/
	    } else {
		break;
	    }
	}
	return (!exit_local);
}

static void
root_sigchldhandler()
{
	union	wait status;

	root_SIGCHLD = 0;
	while (wait3(&status, WNOHANG, (struct rusage *)0) > 0)
		{}
}

static void
root_sigwinchhandler()
{
	root_SIGWINCH = 0;
	(void)pw_damaged(pixwin);
	switch (root_image_type) {
	case ROOT_IMAGE_PATTERN:
		(void)pw_replrop(pixwin,
		    screen.scr_rect.r_left, screen.scr_rect.r_top,
		    screen.scr_rect.r_width, screen.scr_rect.r_height,
		    PIX_SRC | PIX_COLOR(root_color), root_image_pixrect, 0, 0);
		break;
	case ROOT_IMAGE_SOLID:
	default:
		(void)pw_writebackground(pixwin,
		    screen.scr_rect.r_left, screen.scr_rect.r_top,
		    screen.scr_rect.r_width, screen.scr_rect.r_height,
		    PIX_SRC | PIX_COLOR(root_color));
	}
	(void)pw_donedamaged(pixwin);
	return;
}

static void
root_sigchldcatcher()
{
	root_SIGCHLD = 1;
}

static void
root_sigwinchcatcher()
{
	root_SIGWINCH = 1;
}

static char *
get_home_dir()
{
	extern	char *getlogin();
	extern	struct	passwd *getpwnam(), *getpwuid();
	struct	passwd *passwdent;
	char	*home_dir = getenv("HOME"), *loginname;

	if (home_dir != NULL)
		return(home_dir);
	loginname = getlogin();
	if (loginname == NULL) {
		passwdent = getpwuid(getuid());
	} else {
		passwdent = getpwnam(loginname);
	}
	if (passwdent == NULL) {
		(void)fprintf(stderr,
		    "suntools: couldn't find user in password file.\n");
		return(NULL);
	}
	if (passwdent->pw_dir == NULL) {
		(void)fprintf(stderr,
		    "suntools: no home directory in password file.\n");
		return(NULL);
	}
	return(passwdent->pw_dir);
}

#define	ROOT_ARGBUFSIZE		1000
#define	ROOT_SETUPFILE		"/.suntools"
#define	ROOT_MAXTOOLDELAY	10
#define	ROOT_DEFAULTSETUPFILE	"/usr/lib/.suntools"

static void
root_initialsetup(requestedfilename)
	char	*requestedfilename;
{
	register i;
	FILE	*file;
	char	filename[MAXNAMLEN], programname[MAXNAMLEN],
		otherargs[ROOT_ARGBUFSIZE];
	struct	rect rectnormal, recticonic;
	int	iconic, topchild, bottomchild, seconds, j;
	char	line[ARG_CHARS], full_programname[MAXPATHLEN];

	if (requestedfilename[0] == NULL) {
		char *home_dir = get_home_dir();
		if (home_dir == NULL)
			return;
		(void) strcpy(filename, home_dir);
		(void) strncat(filename, ROOT_SETUPFILE, sizeof(filename)-1-
				strlen(filename)-strlen(ROOT_SETUPFILE));
	} else
		(void) strncpy(filename, requestedfilename, sizeof(filename)-1);
	file = fopen(filename, "r");
	if (!file && !requestedfilename[0]) {
	/* If default file not found in HOME, look in public library */
		(void) strcpy(filename, ROOT_DEFAULTSETUPFILE);
		file = fopen(filename, "r");
	}
	if (!file) {
/*  We used to not give an error if looking for default .suntools.
    Now that we check the defaults lib dir, we give an error message.
		if (requestedfilename[0] == NULL)
			return;
*/
		(void)fprintf(stderr, "suntools: couldn't open %s\n", filename);
		return;
	}
	while (fgets(line, sizeof (line), file)) {
		register char *t;
		for (t = line; isspace(*t); t++);
		if (*t == '#' || *t == '\0')
			continue;
		otherargs[0] = '\0';
		programname[0] = '\0';
		i = sscanf(line, "%s%hd%hd%hd%hd%hd%hd%hd%hd%hD%[^\n]\n",
		    programname,
		    &rectnormal.r_left, &rectnormal.r_top,
		    &rectnormal.r_width, &rectnormal.r_height,
		    &recticonic.r_left, &recticonic.r_top,
		    &recticonic.r_width, &recticonic.r_height,
		    &iconic, otherargs);
		if (i == EOF)
			break;
		if (i < 10 || i > 11) {
		   /*
		    * Just get progname and args.
		    */
		    otherargs[0] = '\0';
		    programname[0] = '\0';
		    j = sscanf(line, "%s%[^\n]\n", programname, otherargs);
		    if (j > 0) {
			iconic = 0;
			rect_construct(&recticonic, WMGR_SETPOS, WMGR_SETPOS,
			    WMGR_SETPOS, WMGR_SETPOS);
			rect_construct(&rectnormal, WMGR_SETPOS, WMGR_SETPOS,
			    WMGR_SETPOS, WMGR_SETPOS);
		    } else {
		    (void)fprintf(stderr,
		   "suntools: in file=%s fscanf gave %D, correct format is:\n",
			filename, i);
		    (void)fprintf(stderr,
 "program open-left open-top open-width open-height close-left close-top close-width close-height iconicflag [args] <newline>\n OR\nprogram [args] <newline>\n");
		    continue;
		    }
		}
		/*
		 * Remember who top and bottom children windows are for use when
		 * trying to determine when tool is installed.
		 */
		topchild = win_getlink(rootfd, WL_TOPCHILD);
		bottomchild = win_getlink(rootfd, WL_BOTTOMCHILD);
		/*
		 * Fork tool.
		 */
		suntools_mark_close_on_exec();
		expand_path(programname, full_programname);
		(void) wmgr_forktool(full_programname, otherargs,
		    &rectnormal, &recticonic, iconic);
		/*
		 * Give tool chance to intall self in tree before starting next.
		 */
		for (seconds = 0; seconds < ROOT_MAXTOOLDELAY; seconds++) {
			sleep(1);
			if (topchild != win_getlink(rootfd, WL_TOPCHILD) ||
			    bottomchild != win_getlink(rootfd, WL_BOTTOMCHILD))
				break;
		}
	}
	(void) fclose(file);
}

Pkg_private
suntools_mark_close_on_exec()
{
	register i;
	int limit_fds = NOFILE;

	/* Mark all fds (other than stds) as close on exec */
	for (i = 3; i < limit_fds; i++)
		(void) fcntl(i, F_SETFD, 1);
}

static void
root_set_pattern(token, ptr_single_color)
	char *token;
	struct singlecolor *ptr_single_color;
{
	char err[IL_ERRORMSG_SIZE];
	struct pixrect *mpr;

	if (strcmp(token, "on") == 0) {
		root_image_type = ROOT_IMAGE_PATTERN;
	} else if (strcmp(token, "off") == 0) {
		root_image_type = ROOT_IMAGE_SOLID;
	} else if (strcmp(token, "grey") == 0 || strcmp(token, "gray") == 0) {
		ptr_single_color->red = ptr_single_color->green =
		    ptr_single_color->blue = 128;
		root_color = 1;
		root_image_type = ROOT_IMAGE_SOLID;
	} else if ((mpr = icon_load_mpr(token, err))== (struct pixrect *)0) {
		(void)fprintf(stderr, "suntools: ");
		(void)fprintf(stderr, err);
	} else {
		root_image_pixrect = mpr;
		root_image_type = ROOT_IMAGE_PATTERN;
	}
}

static void
root_set_background(filename)
	char	*filename;
{
	int		 x, y;
	FILE		*file;
	register struct pixrect	*tmp_pr, *root_pr;

	if ((file = fopen(filename, "r"))  == (FILE *) NULL)  {
		(void)fprintf(stderr, "Couldn't open background file \"%s\":",
			filename);
		perror("");
		return;
	}
	if ((tmp_pr = pr_load(file, (colormap_t *)NULL/* Ignoring colormap */))
	    == (struct pixrect *) NULL)  {
		(void)fprintf(stderr, "Couldn't load background from %s\n",
			 filename);
		 (void)fclose(file);
		 return;
	}
	(void)fclose(file);
	root_pr =
	    mem_create(screen.scr_rect.r_width, screen.scr_rect.r_height, 1);
	/* Center image */
	x = (screen.scr_rect.r_width - tmp_pr->pr_width) / 2;
	y = (screen.scr_rect.r_height - tmp_pr->pr_height) / 2;
	/* Initialize background */
	switch (root_image_type) {
	case ROOT_IMAGE_PATTERN:
		(void)pr_replrop(root_pr, 0, 0,
		    screen.scr_rect.r_width, screen.scr_rect.r_height,
		    PIX_SRC | PIX_COLOR(root_color), root_image_pixrect, 			    0, 0);
		break;
	case ROOT_IMAGE_SOLID:
	default:
		(void)pr_rop(root_pr, 0, 0,
		    screen.scr_rect.r_width, screen.scr_rect.r_height,
		    PIX_SRC | PIX_COLOR(root_color), (Pixrect *)0, 0, 0);
	}
	/* Draw picture on image pixrect */
	(void)pr_rop(root_pr, x, y, tmp_pr->pr_width, tmp_pr->pr_height,
		PIX_SRC, tmp_pr, 0, 0);
	(void)pr_destroy(tmp_pr);
	root_image_pixrect = root_pr;
	/* PATTERN will cause image pixrect to be roped without replication */
	root_image_type = ROOT_IMAGE_PATTERN;
}

Pkg_private int
wmgr_menufile_changes()
{
	struct stat statb;
	int sa_count;

	if (wmgr_nextfile == 0) return 1;
	/* Whenever existing menu going up, stat menu files */
	for (sa_count = 0;sa_count < wmgr_nextfile;sa_count++) {
		if (stat(wmgr_stat_array[sa_count].name, &statb) < 0) {
			if (errno == ENOENT)
				return(1);
			(void)fprintf(stderr, "suntools: ");
			perror(wmgr_stat_array[sa_count].name);
			return(-1);
		}
		if (statb.st_mtime > wmgr_stat_array[sa_count].mftime)
			return(1);
	}
	return 0;
}

Pkg_private
wmgr_free_changes_array()
{   
    int sa_count = 0;
    
    while (sa_count < wmgr_nextfile) {
	free(wmgr_stat_array[sa_count].name);      /* file name */
	wmgr_stat_array[sa_count].name = NULL;
	wmgr_stat_array[sa_count].mftime = 0;
	sa_count++;
    }
    wmgr_nextfile = 0;
}

static
wmgr_freerootmenus(menu)
	struct menu *menu;
{
	struct menu *next = menu->m_next, *nnext;

	while (next) {
		nnext = next->m_next;
		if (next->m_items) {
			/* free string storage */
			free((char *)(LINT_CAST(next->m_items->mi_data)));
			/* item storage */
			free((char *)(LINT_CAST(next->m_items)));
		}
		free((char *)(LINT_CAST(next)));			      /* menu storage */
		next = nnext;
	}
	wmgr_free_changes_array();
}

static
wmgr_getrootmenu(mn, menu, mf, mi, mis, maxitems)
	char *mn, *mf;
	struct menu *menu;
	struct menuitem *mi;
	struct menuitemstrings *mis;
	int maxitems;
{
	FILE *f;
	int lineno;
	char line[ARG_CHARS], full_mf[MAXPATHLEN];
	char tag[32], prog[256], args[ARG_CHARS];
	struct stat statb;
	struct menu *menunext = (struct menu *)(LINT_CAST(wmgr_rootmenu));
	int nitems = 0;
	static char *nqformat = "%[^ \t\n]%*[ \t]%[^ \t\n]%*[ \t]%[^\n]\n";
	static char *qformat = "\"%[^\"]\"%*[ \t]%[^ \t\n]%*[ \t]%[^\n]\n";

	if (menu == (struct menu *)(LINT_CAST(wmgr_rootmenu)) && wmgr_nextfile != 0) {
		if (wmgr_menufile_changes() != 0)
			wmgr_freerootmenus((struct menu *)(LINT_CAST(wmgr_rootmenu)));
		else
			return menu->m_itemcount;
	}
	if (wmgr_nextfile >= MAX_FILES-1) {
		(void)fprintf(stderr,
		    "suntools: max number of menus is %D\n", MAX_FILES);
		return -1;
	}
	expand_path(mf, full_mf);
	if ((f = fopen(full_mf, "r")) == NULL) {
		(void)fprintf(stderr, "suntools: can't open menu file %s\n", full_mf);
		return -1;
	}
	if (stat(full_mf, &statb) < 0) {
	    (void)fprintf(stderr, "suntools: ");
	    perror(full_mf);
	    (void)fclose(f);
	    return -1;
	}
	wmgr_stat_array[wmgr_nextfile].mftime = statb.st_mtime;
	wmgr_stat_array[wmgr_nextfile].name = wmgr_savestr(full_mf);
	++wmgr_nextfile;
	menu->m_imagetype = MENU_IMAGESTRING;
	menu->m_imagedata = mn;
	menu->m_items = mi;
	for (nitems = 0, lineno = 1; nitems < maxitems &&
	    fgets(line, sizeof (line), f); lineno++) {
		if (line[0] == '#')
			continue;
		args[0] = '\0';
		if (sscanf(line, line[0] == '"' ? qformat : nqformat,
		    tag, prog, args) < 2) {
			(void)fprintf(stderr,
			    "suntools: format error in %s: line %d\n",
			    full_mf, lineno);
			continue;
		}
		if (strcmp(prog, "MENU") == 0) {
		    struct menu *m;
		    struct menuitem *mi_local;
		    struct menuitemstrings *ms;
		    if (menu != (struct menu *)(LINT_CAST(wmgr_rootmenu))) {
			(void)fprintf(stderr,
	"suntools: MENU command illegal in secondary menu file %s: line %d\n",
			    full_mf, lineno);
			continue;
		    }
		    if (wmgr_getrootmenu(
		        wmgr_savestr(tag),			   /* menu name */
		        m = (struct menu *)(LINT_CAST(calloc(1, sizeof(struct menu)))),
			args,				   /* file name */
		        mi_local = (struct menuitem *)(LINT_CAST(
				calloc(ROOTMENUITEMS, sizeof(struct menuitem)))),
		        ms = (struct menuitemstrings *)(LINT_CAST(
				calloc(ROOTMENUITEMS, sizeof(struct menuitemstrings)))),
			ROOTMENUITEMS) <= 0) {
			    (void)fprintf(stderr,
			        "suntools: invalid secondary menu %s\n", args);
			    free((char *)(LINT_CAST(m))); 
			    free((char *)(LINT_CAST(mi_local))); 
			    free((char *)(LINT_CAST(ms)));
			    continue;
		    } else {
			menunext->m_next = m;
			menunext = m;
		    }
		} else {
		    if (mi->mi_imagedata)
		    	free((char *)mi->mi_imagedata);
		    mi->mi_imagetype = MENU_IMAGESTRING;
		    mi->mi_imagedata = (caddr_t)wmgr_savestr(tag);
		    mi->mi_data = (caddr_t)mis;
		    if (mis->mis_prog) free(mis->mis_prog);
		    if (mis->mis_args) free(mis->mis_args);
		    mis->mis_prog = wmgr_savestr(prog);
		    if (args[0] == '\0')
		    	mis->mis_args = (char *)NULL;
		    else
		    	mis->mis_args = wmgr_savestr(args);
		    mi++;
		    mis++;
		    nitems++;
		}
	}
	(void)fclose(f);
	return menu->m_itemcount = nitems;
}

Pkg_private char *
wmgr_savestr(s)
	register char *s;
{
	register char *p;

	if ((p = malloc((unsigned)(strlen(s) + 1))) == NULL) {
		if (rootfd) (void)win_screendestroy(rootfd);
		(void)fprintf(stderr, "suntools: out of memory for menu strings\n");
		exit(1);
	}
	(void)strcpy(p, s);
	return (p);
}

Pkg_private char *
wmgr_save2str(s, t)
	register char *s, *t;
{
	register char *p;

	if ((p = malloc((unsigned)(strlen(s) + strlen(t) + 1 + 1))) == NULL) {
		if (rootfd)
			(void)win_screendestroy(rootfd);
		(void)fprintf(stderr, "suntools: out of memory for menu strings\n");
		exit(1);
	}
	(void)strcpy(p, s);
	(void)strcpy(index(p, '\0') + 1, t);
	return (p);
}

/* ARGSUSED */
static
wmgr_handlerootmenuitem(menu, mi, rootfd_local)
	struct	menu *menu;
	struct	menuitem *mi;
	int	rootfd_local;
{   
    int	returncode = 0;
    struct	rect recticon, rectnormal;
    struct	menuitemstrings *mis;
    char full_prog[MAXPATHLEN];

    /*
     * Get next default tool positions
    */
    rect_construct(&recticon,
		   WMGR_SETPOS, WMGR_SETPOS, WMGR_SETPOS, WMGR_SETPOS);
    rectnormal = recticon;
    mis = (struct menuitemstrings *)(LINT_CAST(mi->mi_data));
    if (strcmp(mis->mis_prog, "EXIT") == 0) {
	returncode = wmgr_confirm(rootfd_local,
			"Press the left mouse button to confirm Exit.  \
To cancel, press the right mouse button now.");
    } else if (strcmp(mis->mis_prog, "REFRESH") == 0) {
	wmgr_refreshwindow(rootfd_local);
    } else {
	suntools_mark_close_on_exec();
	expand_path(mis->mis_prog, full_prog);
	(void) wmgr_forktool(full_prog, mis->mis_args,
			     &rectnormal, &recticon, 0/*!iconic*/);
    }
    return(returncode);
}


/* dummy proc for selection_set()
*/
static int
dummy_proc()
{
}


static void
root_start_service()
{
	register int	i;
static char		*args[2] = {"selection_svc", 0 };
	if (vfork() == 0)  {
		for (i = 30; i > 2; i--)  {
			(void)close(i);
		}
		execvp(seln_svc_file, args, 0);
		perror("Couldn't fork selection service");
		sleep(7);
		exit(1);
	}
}

