#ifndef lint
static  char sccsid[] = "@(#)lockscreen.c 1.7 87/02/24 SMI";
#endif
/*
 * Sun Microsystems, Inc.
 */

/*
 * 	Overview:	Lock Screen: Lockscreen waits for the user that created
 *			it (or the root if the -r switch is given) to login.
 *			Until then, the window described
 *			by WINDOW_PARENT in the environment is covered
 *			and a sun logo is displayed on the screen at random
 *			places. Anyone can exit from the suntools environment
 *			on which lockscreen is running if the "-e" switch
 *			was given when lockscreen was started.  Along with
 *			security, running this program when the machine is not
 *			being used helps limit phorphorus burn of the screen
 *			(not significant for sun 2 bw).
 */

#include <suntool/tool_hs.h>
#include <pwd.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/file.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sunwindow/cms.h>
#include <suntool/panel.h>

#define FIVEMIN 300;

static char *fg_default = "lockscreen_default";
static char *bg_default = "/usr/local/computeserver";
static int timeout = FIVEMIN;

static int sigwinchcatcher();
static make_twinkle();
static void abort_proc();
static char *getpath(), *expandname();

static char fg_prog[255];
static char bg_prog[255];
static char *dummy_argv[2], **fg_argv;
static int fg_pid = 0, bg_pid = 0;
static int fork_var = 1;
static int countdown;
static short first_char;

#define CMSIZE 32		/* must be power of two */
static char cmsname[CMS_NAMESIZE];
static u_char	red[CMSIZE];
static u_char	green[CMSIZE];
static u_char	blue[CMSIZE];

static	struct tool *tool;
static	struct toolsw  *panel_toolsw;
static	struct rect rectcreate;
static	int parentfd = 0;
static	int clear_screen = 0;

static	char *tool_label = "Lockscreen 2.0";
static	char *progname;
static	int exitenable = 0;
static	int rootenable = 0;
static	int no_lock = 0;
static	struct timeval tooltimer = {1, 0};
static	struct timeval sectimer  = {1, 0};
static	struct rect imagerect = {0, 0, 64, 64};
static  colormonitor;		/* true if running on color monitor */

#define BIG_FONT   "/usr/lib/fonts/fixedwidthfonts/gallant.r.19"
#define SMALL_FONT "/usr/lib/fonts/fixedwidthfonts/screen.b.12"
static struct pixfont *big_font, *small_font;

static	Panel_item  name_item, passwd_item, abort_item, 
                  instructions_item, msg_item, exit_item;

static	short icon_data[256] = {
#include <images/lockscreen.icon>
};
mpr_static(lkscr_icpr, 64, 64, 1, icon_data);
static	struct pixrect *lockscreen_mpr;

static	struct icon icon = {64, 64, (struct pixrect *)NULL, 0, 0, 64, 64,
	    &lkscr_icpr, 0, 0, 0, 0, NULL, (struct pixfont *)NULL,
	    ICON_BKGRDCLR};

/* lockscreen choice item choices */
static	short jump1_data[256] = {
#include <images/jump1.icon>
};
mpr_static(jump1_mpr, 64, 64, 1, jump1_data);

static	short jump2_data[256] = {
#include <images/jump2.icon>
};
mpr_static(jump2_mpr, 64, 64, 1, jump2_data);

static	short jump3_data[256] = {
#include <images/jump3.icon>
};
mpr_static(jump3_mpr, 64, 64, 1, jump3_data);

static	short jump4_data[256] = {
#include <images/jump4.icon>
};
mpr_static(jump4_mpr, 64, 64, 1, jump4_data);

static	short jump5_data[256] = {
#include <images/jump5.icon>
};
mpr_static(jump5_mpr, 64, 64, 1, jump5_data);

static	short jump6_data[256] = {
#include <images/jump6.icon>
};
mpr_static(jump6_mpr, 64, 64, 1, jump6_data);

static struct pixrect *jump_pr[] = {
   &jump1_mpr, &jump2_mpr, &jump3_mpr, &jump4_mpr, &jump5_mpr, &jump6_mpr
};

#define exit_and_kill(n) { \
    if (fg_pid) (void)kill(fg_pid, SIGKILL); \
    if (bg_pid) (void)kill(bg_pid, SIGKILL); \
    exit(n); \
    }
    

static union wait pstatus;
static int wpid;

char	*tool_get_attribute(), *ttyname(), *sprintf(), *strcpy();
long	random();
void	wmgr_changestate(), wmgr_top();

static void
sigchild_handler()
{
    struct timeval TimeVal;

    if ((wpid = wait3(&pstatus, WNOHANG, (struct rusage *)0)) > 0) {
	if (wpid == fg_pid || wpid == bg_pid) {
	    (void)gettimeofday(&TimeVal, (struct timezone *)0);
	    if (wpid == fg_pid) {
		(void)fprintf (stderr, "foreground process (%s) died at %s",
			 fg_prog, ctime((time_t *)(LINT_CAST(&(TimeVal.tv_sec)))));
		fg_pid = 0;
		*fg_prog = 0;
	    } else {
		(void)fprintf (stderr, "background process (%s) died at %s",
			 bg_prog, ctime((time_t *)(LINT_CAST(&(TimeVal.tv_sec)))));
		bg_pid = 0;
	    }
	}
	if (!fg_pid) {
	    tooltimer = sectimer;
	    tool->tl_io.tio_timer = &tooltimer;
	    clear_screen = 1;
	}
    }
}


#ifdef STANDALONE
main(argc, argv)
#else
lockscreen_main(argc, argv)
#endif
	int argc;
	char **argv;
{   
    char	**tool_attrs = NULL;
    int k;

    progname = argv[0];
    argv++; argc--;
	
   /*
    * Pick up command line arguments to modify tool behavior
    */
    if (tool_parse_all(&argc, argv, &tool_attrs, progname) == -1) {
	print_help_and_exit();
    }
    *fg_prog = *bg_prog = 0;
    while (argc > 0 && **argv == '-') {
	    switch (argv[0][1]) {

	      case 'e':
		exitenable = 1;
		break;

	      case 'r':
		rootenable = 1;
		break;

	      case 'b':
		if (!*(++argv)) break;  /* Missing name */
		(void)expandname(*argv, getpath(), bg_prog);
		if (*bg_prog == 0) {
		    (void)fprintf(stderr, "Background program (%s) not executable.\n",
			    *argv);
		    exit_and_kill(1);
		}
		argc--;
		break;

	      case 'n':	/* no password */
		no_lock = 1;
		break;

	      case 'f':
		/* This will exit while loop */
		break;

	      case 't':
		if (--argc > 0 && isdigit(**++argv)) timeout = atoi(*argv);
		else {
		    (void)fprintf(stderr, "-t needs integer argument\n");
		    exit_and_kill(1);
		}
		break;

	      case 'H': case 'h': case '?':
		print_help_and_exit();

	      default: /* Unrecognized args are passed to default prog */
		*--argv = fg_default, argc++;
		argv--;	argc++; /* This is undone at the bottom */
		break;
	    }
	argv++;	argc--;
    }
    if (argc > 0) {
	(void)expandname(*argv, getpath(), fg_prog);

	if (*fg_prog == 0 && **argv) {
	    (void)fprintf(stderr, "Foreground program (%s) not executable.\n",
		    *argv);
	    exit_and_kill(1);
	} else if (*fg_prog != 0) {
	    fg_argv = argv;
	    argc = -1;
	} else {
	    argc = -2; /* No foreground program was set explicitly */
	}
    }
    if (!*fg_prog && argc != -2) {
	(void)expandname(fg_default, getpath(), fg_prog);
	fg_argv = dummy_argv;
	dummy_argv[0] = fg_prog, dummy_argv[1] = 0;
    }
    if (!*bg_prog) (void)expandname(bg_default, getpath(), bg_prog);
    fork_var = *fg_prog || *bg_prog;
   /*
    * Determine screen size
    */
    getrectcreate();
   /*
    * Create tool window
    */
    icon.ic_width = rectcreate.r_width;
    icon.ic_height = rectcreate.r_height;
    tool = tool_make(WIN_LABEL,		tool_label,
		     WIN_NAME_STRIPE,	0,
		     WIN_ICON,		&icon,
		     WIN_ATTR_LIST,	tool_attrs,
		     WIN_WIDTH,		rectcreate.r_width,
		     WIN_HEIGHT,	rectcreate.r_height,
		     WIN_LEFT,		rectcreate.r_left,
		     WIN_TOP,		rectcreate.r_top,
		     WIN_ICON_LEFT,	rectcreate.r_left,
		     WIN_ICON_TOP,	rectcreate.r_top,
		     WIN_ICONIC,	1,
		     0);

    if (tool == (struct tool *)NULL) exit_and_kill(1);
    (void)tool_free_attribute_list(tool_attrs);
    lockscreen_mpr = (struct pixrect *)(LINT_CAST(
    	tool_get_attribute(tool, (int)WIN_ICON_IMAGE)));
    inittool();

    if ((big_font = pf_open(BIG_FONT)) == NULL) {
	(void)fprintf(stderr, "lockscreen: Can't find font: %s\n", BIG_FONT);
    }

    panel_toolsw = panel_create(tool,
				PANEL_FONT, big_font, 
				PANEL_TIMER_PROC, make_twinkle,
				PANEL_LABEL_BOLD, TRUE,
				0);

    initpanel(panel_toolsw);

    /* added to disable L5 (expose/hide) function key */
    disable_wmgr_keys(panel_toolsw->ts_windowfd);
    disable_wmgr_keys(tool->tl_windowfd);

    (void)signal(SIGWINCH, sigwinchcatcher);
    (void)tool_install(tool);

   /* 
    * setting up the color map must be done AFTER
    * doing a tool_install
    */
    (void)sprintf(cmsname, "lockscreen%d", getpid());
    (void)pw_setcmsname(tool->tl_pixwin, cmsname);
    for(k = 0; k < CMSIZE-2; k++)
	hsv_to_rgb((360/(CMSIZE-2))*k, 255, 255,
		   &red[k+1], &green[k+1], &blue[k+1]);
    red[0] = green[0] = blue[0] = 255;
    red[CMSIZE-1] = green[CMSIZE-1] = blue[CMSIZE-1] = 0;
    (void)pw_putcolormap(tool->tl_pixwin, 0, CMSIZE, red, green, blue);
    colormonitor = tool->tl_pixwin->pw_pixrect->pr_depth > 1;

#ifndef SUN2.0 INPUT_FOCUS
   /* Direct keyboard input to tool */
    (void)win_set_kbd_focus(tool->tl_windowfd, win_fdtonumber(tool->tl_windowfd));
#endif
   /* Wait for input */
    (void)tool_select(tool, 0/* Dont wait for child process to die */);
    (void)tool_destroy(tool);
    exit(0);
}

static
disable_wmgr_keys(windowfd)
	int windowfd;
{
    struct inputmask mask;
    int designee;

    win_getinputmask(windowfd, &mask, &designee);
    win_unsetinputcodebit(&mask, KEY_LEFT(5));
    win_unsetinputcodebit(&mask, KEY_LEFT(7));
    win_setinputmask(windowfd, &mask, 0, designee);
}

static
toolselected(tool_local, ibits, obits, ebits, timer)
	struct	tool *tool_local;
	int	*ibits, *obits, *ebits;
	struct	timeval **timer;
{   
    extern char **environ;
    char WinName [80];
    int k;
	
    wmgr_top(tool_local->tl_windowfd, parentfd);

    if (clear_screen) {
	(void)pw_writebackground(tool_local->tl_pixwin,
			   rectcreate.r_left, rectcreate.r_top,
			   rectcreate.r_width, rectcreate.r_height, PIX_SET);
	clear_screen = 0;
    }

    if (fork_var &&
	*timer && ((*timer)->tv_sec == 0) && ((*timer)->tv_usec == 0)
	&& tool_local->tl_flags & TOOL_ICONIC) {
	if (!fg_pid && *fg_prog) {
	    (void)signal(SIGCHLD, (int (*)())(LINT_CAST(sigchild_handler)));
	    fg_pid = vfork();
	    if (fg_pid == -1) { /* Possible error? */
		(void)fprintf(stderr, "foreground process vfork failed\n");
		perror(progname);
		fg_pid = 0;
		exit_and_kill(1);
	    }
	    if (fg_pid == 0) { /* child */
		(void)win_fdtoname(tool_local->tl_windowfd, WinName);
		(void)we_setgfxwindow(WinName);
		execvp(fg_prog, fg_argv);
	    }
	    if (!setpriority(PRIO_USER, fg_pid, 20)) perror(progname);
	}
	if (!bg_pid && *bg_prog) {
	    bg_pid = vfork();
	    if (bg_pid == -1) { /* Possible error? */
		(void)fprintf(stderr, "background process vfork failed\n");
		perror(progname);
		bg_pid = 0;
		exit_and_kill(1);
	    }
	    if (bg_pid == 0) { /* child */
		execvp(bg_prog, (char *)0);
	    }
	}
	fork_var = 0;
    } else if (!fg_pid &&
	       *timer && ((*timer)->tv_sec == 0) && ((*timer)->tv_usec == 0)
	       && (tool_local->tl_flags & TOOL_ICONIC)) {
       /*
	* Reset old image.
	*/
	(void)pw_writebackground(tool_local->tl_pixwin,
			   imagerect.r_left, imagerect.r_top,
			   imagerect.r_width, imagerect.r_height, PIX_SET);

       /*
	* Choose new position.
	*/
	imagerect.r_left = r(rectcreate.r_left,
			     rectcreate.r_width-imagerect.r_width);
	imagerect.r_top = r(rectcreate.r_top,
			    rectcreate.r_height-imagerect.r_height);
       /*
	* Draw new image.
	*/
	k = random()%(CMSIZE-2) + 1;
	if (colormonitor)
	    (void)pw_stencil(tool_local->tl_pixwin,
		       imagerect.r_left, imagerect.r_top,
		       imagerect.r_width, imagerect.r_height,
		       PIX_SRC|PIX_COLOR(k),
		       lockscreen_mpr, 0, 0, (struct pixrect *)NULL, 0, 0);
	else
	    (void)pw_write(tool_local->tl_pixwin,
		     imagerect.r_left, imagerect.r_top,
		     imagerect.r_width, imagerect.r_height,
		     PIX_SRC ^ PIX_DST,
		     lockscreen_mpr, 0, 0);
    }
    if (!(tool_local->tl_flags & TOOL_ICONIC) && --countdown == 0) abort_proc();
    if (*ibits & (1 << tool_local->tl_windowfd)) {
	struct	inputevent event;

	countdown = timeout;
	if (input_readevent(tool_local->tl_windowfd, &event) == -1) {
	    perror(progname);
	    return;
	}
#ifndef SUN2.0 INPUT_FOCUS
	if (event_id(&event) > META_LAST && event_id(&event) < SHIFT_LEFT) {
	    *ibits = *obits = *ebits = 0;
	    tooltimer = sectimer;
	    *timer = fg_pid ? 0 : &tooltimer;
	    return;
	}
#endif
	if (tool_local->tl_flags & TOOL_ICONIC) {
	    char *getloginname();
			
	    if (fg_pid) {
		k = fg_pid, fg_pid = 0, fork_var = 1;
		/* Lock screen so sure that child doesn't have it locked */
		(void)pw_lock(tool_local->tl_pixwin, &rectcreate);
		(void)kill(k, SIGKILL);
		(void)pw_unlock(tool_local->tl_pixwin);
	    }
	   /*
	    * blow away if the no_lock option is
	    * set or if the user has no password.
	    */
	    if (no_lock || no_password()) {
		(void)tool_done(tool_local);
		return;
	    }

           /*
	    * Open tool from iconic state on any event.
	    * Note: Do not use wmgr_open because not
	    * really closed.
	    */
	    wmgr_changestate(tool_local->tl_windowfd, tool_local->tl_windowfd, FALSE);
#ifndef SUN2.0 INPUT_FOCUS
           /* Direct keyboard input to panel */
	    (void)win_set_kbd_focus(panel_toolsw->ts_windowfd,
			      win_fdtonumber(panel_toolsw->ts_windowfd));
#endif
           /*
	    * Reset state for open.
	    */
	    (void)panel_set_value(name_item, getloginname());
	    (void)panel_set_value(passwd_item,"");
	    (void)panel_set(panel_toolsw->ts_data, 
		      PANEL_CARET_ITEM, passwd_item,
		      0);
	    first_char = 0; /* event.ie_code; /* ?? Fix read ahead problem */
           /*
	    * Call toolhandlesigwinch explicitly because changing
	    * state in previous call does not envoke SIGWINCH
	    * because clipping stays the same.
	    */
	    toolhandlesigwinch(tool_local);
	}
    }
    *ibits = *obits = *ebits = 0;
    tooltimer = sectimer;
    *timer = fg_pid ? 0 : &tooltimer;
}

static
toolhandlesigwinch(tool_local)
	struct	tool *tool_local;
{
	struct	rect rect;
	int	iconic;
	int	wmgr_iswindowopen();

	tool_local->tl_flags &= ~TOOL_SIGWINCHPENDING;
	iconic = !wmgr_iswindowopen(tool_local->tl_windowfd);
	(void)win_getsize(tool_local->tl_windowfd, &rect);
	if (iconic && (~tool_local->tl_flags&TOOL_ICONIC)) {
		/*
		 * Tool has just gone iconic, so, add y offset to sws
		 * to move them out of the picture.
		 */
		(void)_tool_addyoffsettosws(tool_local, 2048);
		tool_local->tl_flags |= TOOL_ICONIC;
		tool_local->tl_rectcache = rect;
	} else if (!iconic && (tool_local->tl_flags&TOOL_ICONIC)) {
		static struct timeval panel_timer = { 0, 50000 };

		/*
		 * Tool has just gone from iconic to normal, so, subtract
		 * y offset from sws to move them into the picture again.
		 */
		tool_local->tl_flags &= ~TOOL_ICONIC;
		(void)_tool_addyoffsettosws(tool_local, -2048);
		tool_local->tl_rectcache = rect;
		/* set the panel timer */
		panel_toolsw->ts_io.tio_timer = &panel_timer;
	}
	/*
	/*
	 * Refresh tool now
	 */
	(void)pw_damaged(tool_local->tl_pixwin);
	(void)pw_donedamaged(tool_local->tl_pixwin);
	if (tool_local->tl_flags & TOOL_ICONIC)
	    (void)pw_writebackground(tool_local->tl_pixwin, rect.r_left, rect.r_top,
			       rect.r_width, rect.r_height, PIX_SET);
	else
	    (void)tool_displaytoolonly(tool_local);
	/*
	 * Refresh subwindows now
	 */
	(void)_tool_subwindowshandlesigwinch(tool_local);
	return;
}

static	int
showdtop_proc()
/* showdtop_proc gets the current login name & password and checks
   them for validity.  Note that this is the notify proc for 
   the password item, and that the notify arguments are
   ignored.
*/
{
   char	*name, *passwd, *loginname, *crypted;
   struct	passwd *passwdent;
   extern	struct	passwd *getpwnam();
   char	*getloginname(), *crypt(), passwd_buf[100];

   /* get the login name & password */
   name   = (char *)panel_get_value(name_item);
   passwd = (char *)panel_get_value(passwd_item);
   first_char = first_char > ' ' && first_char < 128 ? first_char : 0;
   if (first_char) {
       *passwd_buf = (char)first_char;
       (void)strcpy(passwd_buf + 1, passwd);
       passwd = passwd_buf;
       first_char = 0;
   }

   if (*name) {

      paintmsg("Validating login....");

      /* Find out who is logged in.  */

      if ((loginname = getloginname()) == 0) {
         paintmsg("You don't have a login name.");
	 (void)panel_set_value(passwd_item,"");
	 return 0;
      }

      /* See if should let root try to break into desktop.  */
      else if (strcmp("root", name) == 0 && 
	       strcmp(loginname, "root") != 0) {
	 if (rootenable) loginname = "root";
	 else {
	    paintmsg("Root access not enabled (-r switch).");
	    (void)panel_set_value(passwd_item,"");
	    return 0;
	 }
      }

      if (strcmp(loginname, name))
         /* Only let person who is logged in (or root) login.  */
	 paintmsg("Invalid login.");

      else if ((passwdent = getpwnam(loginname)) == 0)
         /* Get password entry with can compare typed in password.  */
	 paintmsg("You don't have a password.");

      else {
         /* Encrypt passwd so can compare with passwd file entry.  */
	 crypted = crypt(passwd, passwdent->pw_passwd);
	 if (strcmp(crypted, passwdent->pw_passwd))
            paintmsg("Invalid login.");
	 else {
	    (void)panel_set_value(passwd_item,"");
	    (void)tool_done(tool);
	    return 0;
	 }
      }
   } else 
      paintmsg("Invalid login.");
      
   (void)panel_set_value(passwd_item,"");

   return 0;
}

static
paintmsg(str)
char *str;
{
   (void)panel_set(msg_item, PANEL_LABEL_STRING, str, 0);
}


static	void
abort_proc()
{
	/*
	 * Note: Do not use wmgr_open because not really opened.
	 */
	wmgr_changestate(tool->tl_windowfd, tool->tl_windowfd, TRUE);
#ifndef SUN2.0 INPUT_FOCUS
	/* Direct keyboard input to tool */
	(void)win_set_kbd_focus(tool->tl_windowfd, win_fdtonumber(tool->tl_windowfd));
#endif
	/*
	 * Call toolhandlesigwinch explicitely because changing
	 * state in previous call does not envoke SIGWINCH
	 * because clipping stays the same.
	 */
	(void)panel_set_value(passwd_item,"");
	toolhandlesigwinch(tool);
}

static	void
exit_proc()
{
	register int toolfd, link, i;
	int pid;
	struct screen screen;
	char name[WIN_NAMESIZE];
	register Pixwin *pixwin = tool->tl_pixwin;
	char color[256];
	extern int win_screendestroysleep;

	if ((wmgr_confirm(tool->tl_windowfd, "Press the left mouse button to confirm Exit of underlying suntools environment.  To cancel, press the right mouse button now.") != -1))
		return;
	/*
	 * Prevent this process from being killed by the shell that is
	 * running it when we gun tools below.
	 */
	(void) signal(SIGHUP, SIG_IGN);
	/*
	 * Kill off tools so that don't provoke getting logged out due to
	 * killing the console window and the suntools program in the wrong
	 * order.
	 */
	(void)win_screenget(tool->tl_windowfd, &screen);
	for (link = win_getlink(parentfd, WL_OLDESTCHILD);
	    link != WIN_NULLLINK;) {
		/* Open tool window */
		(void)win_numbertoname(link, name);
		if ((toolfd = open(name, O_RDONLY, 0)) < 0)
		    return;
		pid = win_getowner(toolfd);
		if (pid != getpid())
			(void)kill(pid, SIGTERM);
		link = win_getlink(toolfd, WL_YOUNGERSIB); /* Next */
		(void)close(toolfd);
	}
	/* Blank out cursor */
	blank_cursor(panel_toolsw->ts_windowfd);
	/* Wait for tools to cleanup */
	sleep(3);
	if (fg_pid) (void)kill(fg_pid, SIGKILL);
	if (bg_pid) (void)kill(bg_pid, SIGKILL);
	/*
	 * Clear available plane groups
	 */
	bzero(color, 256);
	color[0] = 255;
	for (i = 0; i < PIX_MAX_PLANE_GROUPS; i++) {
	    if (pixwin->pw_clipdata->pwcd_plane_groups_available[i]) {
		/* Write to all plane groups */
		pr_set_planes(pixwin->pw_pixrect, i, PIX_ALL_PLANES);
		/* Clear screen */
		(void)pr_rop(pixwin->pw_pixrect,
		    screen.scr_rect.r_left, screen.scr_rect.r_top,
		    screen.scr_rect.r_width, screen.scr_rect.r_height,
		    PIX_CLR, (Pixrect *)0, 0, 0);
		/* Reset "reasonable" colormap */
		(void)pr_putcolormap(pixwin->pw_pixrect, 0, 256,
		    (unsigned char *)color, (unsigned char *)color, (unsigned char *)color);
	    }
	}
	/* Destroy suntools so don't wait for 15 seconds in win_screendestroy*/
	pid = win_getowner(parentfd);
	(void)close(parentfd);
	(void)kill(pid, SIGTERM);
	/* Wait for suntools to cleanup */
	sleep(1);
	/* Surpress sleep in win_screendestroy (which is normally 3) */
	win_screendestroysleep = 1;
	/* Kill the desktop */
	(void)win_screendestroy(tool->tl_windowfd);
	exit(0);
}

static
blank_cursor(windowfd)
	int windowfd;
{
	struct pixrect *mpr = mem_create(16, 16, 1);
	struct cursor zero;

	/* Build cursor framework */
	zero.cur_shape = mpr;
	/* Get current cursor */
	win_getcursor(windowfd, &zero);
	/* Set "don't show cursor" flag */
	zero.flags = 1;
	/* Reset cursor */
	win_setcursor(windowfd, &zero);
	/* Free allocated memory */
	pr_destroy(mpr);
}

static
initpanel(tsw)
	struct	toolsw *tsw;
{
   Panel panel = tsw->ts_data;
   void	abort_proc(), exit_proc();
   int	showdtop_proc();

   name_item = 
      panel_create_item(panel, PANEL_TEXT,
                       PANEL_LABEL_STRING,"Name:",
		       PANEL_LABEL_X, PANEL_CU(33),
		       PANEL_LABEL_Y, PANEL_CU(7),
		       PANEL_VALUE_DISPLAY_LENGTH, 45,
		       0);

   passwd_item = 
      panel_create_item(panel, PANEL_TEXT,
		       PANEL_LABEL_STRING, "Password:",
		       PANEL_LABEL_X, PANEL_CU(33),
		       PANEL_LABEL_Y, PANEL_CU(8),
		       PANEL_MASK_CHAR, ' ',
		       PANEL_NOTIFY_PROC, showdtop_proc,
		       PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
		       PANEL_NOTIFY_STRING, "\n\r\t",
		       0);

   abort_item = 
      panel_create_item(panel, PANEL_BUTTON,
			 PANEL_LABEL_IMAGE, jump_pr[0],
			 PANEL_LABEL_X,       PANEL_CU(25),
			 PANEL_LABEL_Y,       PANEL_CU(7),
			 PANEL_MENU_TITLE_STRING, "Screen Control",
			 PANEL_MENU_CHOICE_STRINGS, "Re-lock screen", 0,
		         PANEL_NOTIFY_PROC,   abort_proc,
		         0);

    if ((small_font = pf_open(SMALL_FONT)) == NULL) {
	(void)fprintf(stderr, "lockscreen: Can't find font: %s\n", SMALL_FONT);
    }

   instructions_item = 
      panel_create_item(panel, PANEL_MESSAGE,
	               PANEL_LABEL_X,     PANEL_CU(25),
	               PANEL_LABEL_Y,     PANEL_CU(10),
	               PANEL_LABEL_STRING,
                          "Enter password to unlock; select square to lock.",
                       PANEL_LABEL_BOLD,  FALSE,
                       PANEL_LABEL_FONT,  small_font,
	               0);

   msg_item = 
      panel_create_item(panel, PANEL_MESSAGE,
	               PANEL_LABEL_STRING, "",
                       PANEL_LABEL_BOLD,  FALSE,
	               PANEL_LABEL_X,    PANEL_CU(25),
	               PANEL_LABEL_Y,    PANEL_CU(11),
                       PANEL_LABEL_FONT,  small_font,
	               0);

   if (exitenable) {
      exit_item = 
	 panel_create_item(panel, PANEL_BUTTON,
			    PANEL_LABEL_X, PANEL_CU(10),
			    PANEL_LABEL_Y, PANEL_CU(7),
			    PANEL_LABEL_IMAGE, 
			       panel_button_image(panel, "Exit Desktop", 0, 
			       		(Pixfont *)0),
			    PANEL_NOTIFY_PROC, exit_proc,
			    PANEL_MENU_CHOICE_STRINGS,
			       "Exit suntools environment and logout", 0,
			    0);

   }
}

static
inittool()
{
	struct	inputmask im;
	register struct	inputmask *mask = &im;
	int	toolselected(), toolhandlesigwinch();
	int	i;

	/*
	 * Notice every bit of key input
	 */
	(void)input_imnull(mask);
	mask->im_flags |= IM_ASCII | IM_NEGEVENT;
	for (i=1; i<17; i++) {
	    win_setinputcodebit(mask, KEY_LEFT(i));
	    win_setinputcodebit(mask, KEY_TOP(i));
	    win_setinputcodebit(mask, KEY_RIGHT(i));
	}
	win_setinputcodebit(mask, SHIFT_LEFT);		/* Pick up the shift */
	win_setinputcodebit(mask, SHIFT_RIGHT);		/*   keys for VT-100 */
	win_setinputcodebit(mask, SHIFT_LOCK);		/*   compatibility */
	win_setinputcodebit(mask, MS_LEFT);
	win_setinputcodebit(mask, MS_MIDDLE);
	win_setinputcodebit(mask, MS_RIGHT);
	(void)win_setinputmask(tool->tl_windowfd, mask, (struct inputmask *)0, WIN_NULLLINK);
	/*
	 * Notification overrides
	 */
	tool->tl_io.tio_selected = toolselected;
	tool->tl_io.tio_handlesigwinch = toolhandlesigwinch;
	tool->tl_io.tio_timer = &tooltimer;
}

static
getrectcreate()
{   
    char	parentname[WIN_NAMESIZE];

   /*
    * Determine parent
    */
    if (we_getparentwindow(parentname))
	if (strcmp(ttyname(0), "/dev/console")) {
	    (void)printf("%s not passed parent window in environment\n",progname);
	    exit_and_kill(1);
	} else {
	    (void)strcpy(parentname, "/dev/win0");
	}
	    
   /*
    * Open parent window
    */
    if (!parentfd && (parentfd = open(parentname, O_RDONLY, 0)) < 0) {
	(void)printf("%s (parent) would not open\n", parentname);
	perror(progname);
	exit_and_kill(1);
    }
   /*
    * Determine size of parents window.
    */
    (void)win_getsize(parentfd, &rectcreate);
}

static
sigwinchcatcher()
{
	(void)tool_sigwinch(tool);
}

static	char *
getloginname()
{
	struct	passwd *passwdent;
	extern	struct	passwd *getpwuid();
	char		*loginname;
	static char	name_buf[1024];
	extern	char	*getlogin();

	loginname = getlogin();
	if (loginname == 0) {
		if ((passwdent = getpwuid(getuid())) == 0) {
                      paintmsg("Couldn't find user name.");
		      return(0);
		}
		loginname = passwdent->pw_name;
	}


	(void)strcpy(name_buf, loginname);
	return(name_buf);
}

static
make_twinkle()
/*
 * make_twinkle advances the abort_item to the next choice.
 */
{
   static int counter = 0;
   static int cur_choice = 0;

   if (!(counter++ % 3)) {
      (void)panel_set(abort_item, 
		PANEL_LABEL_IMAGE, jump_pr[cur_choice], 
		PANEL_PAINT, PANEL_NO_CLEAR,
		0);
      cur_choice = (cur_choice + 1) % 6;
   }
}

/*
 * Convert hue/saturation/value to red/green/blue.
 */
static
hsv_to_rgb(h, s, v, rp, gp, bp)
	int h, s, v;
	u_char *rp, *gp, *bp;
{
	int i, f;
	u_char p, q, t;

	if (s == 0)
		*rp = *gp = *bp = v;
	else {
		if (h == 360)
			h = 0;
		h = h * 256 / 60;
		i = h / 256 * 256;
		f = h - i;
		p = v * (256 - s) / 256;
		q = v * (256 - (s*f)/256) / 256;
		t = v * (256 - (s * (256 - f))/256) / 256;
		switch (i/256) {
		case 0:
			*rp = v; *gp = t; *bp = p;
			break;
		case 1:
			*rp = q; *gp = v; *bp = p;
			break;
		case 2:
			*rp = p; *gp = v; *bp = t;
			break;
		case 3:
			*rp = p; *gp = q; *bp = v;
			break;
		case 4:
			*rp = t; *gp = p; *bp = v;
			break;
		case 5:
			*rp = v; *gp = p; *bp = q;
			break;
		}
	}
}

/*
 * Return 1 if user has no password.
 */
static int
no_password()
{
   char	*loginname;
   struct	passwd *passwdent;
   extern	struct	passwd *getpwnam();
   char	*getloginname();
   
   if ((loginname = getloginname()) == 0)
   	return 1;
   	
   if ((passwdent = getpwnam(loginname)) == 0)
   	return 1;
   	
   return 0;
}

static char *
getpath()
{   
    char *getenv();
    char *cp = getenv("PATH");
    
    if(cp == 0)	cp=":/usr/ucb:/bin:/usr/bin";
    return cp;
}

#define oktox(n) \
    (!stat((n),&stbuf) && (stbuf.st_mode & S_IFMT) == S_IFREG && !access((n),1))

static char *
expandname(name, spath, buf)
    char *name, *spath, *buf;
{
    register char *cp;
    struct stat stbuf;
    
    if(*spath == ':' || *name == '/') {
	spath++;
	if(oktox(name)) {
	    (void)strcpy(buf, name);
	    return(spath);
	}
	if (*name == '/') goto error_exit;
    }
    for(;*spath;) {
	/* copy over current directory
	   and then append name */

	for(cp = buf; *spath != 0 && *spath != ':'; )
	    *cp++ = *spath++;
	*cp++ = '/';
	(void)strcpy(cp, name);
	if (*spath) spath++;
	if (oktox(buf)) return(spath);
    }

error_exit:
    *buf = 0;
    return spath;
}


static
print_help_and_exit()
{   
    (void)fprintf(stderr, "Usage: %s [flags] [displayprogram]\n", progname);
    (void)fprintf(stderr, "FLAG         DESCRIPTION                       ARGS\n");
    (void)fprintf(stderr, "-r           enable root login\n");
    (void)fprintf(stderr, "-e           enable exit option\n");
    (void)fprintf(stderr, "-n           enable no-lock option\n");
    (void)fprintf(stderr, "-b           run a background program          program\n");
    (void)fprintf(stderr, "-t           number of seconds for timeout     integer\n");
    (void)tool_usage(progname);
    exit_and_kill(1);
}
