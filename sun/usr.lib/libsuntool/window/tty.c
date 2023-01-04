#ifndef lint
static  char sccsid[] = "@(#)tty.c 1.6 87/04/20 Copyr 1985 Sun Micro";
#endif

/*****************************************************************************/
/*                               tty.c                                       */
/*               Copyright (c) 1985 by Sun Microsystems, Inc.                */
/* NOTE: wrapper-based command subwindows do not work.  See "do_text_stuff"  */
/* below. 12/3/85 thoeber                                                    */
/*****************************************************************************/

#include <stdio.h>
#include <sys/wait.h>
#include <sunwindow/sun.h>
#include <sunwindow/window_hs.h>
#include <suntool/tool_struct.h>
#include <suntool/window.h>
#include <suntool/frame.h>
#include <suntool/tty.h> 
#include <suntool/ttysw.h>
#include <suntool/textsw.h>
#include "ttysw_impl.h"
#include "ttytlsw_impl.h"
#include "charscreen.h"

extern struct pixwin *csr_pixwin;

static int	init_ttytlsw();
static int	tty_set(), tty_quit_on_death(), tty_handle_death();
static caddr_t	tty_get();
Bool 		defaults_get_boolean();
extern int	cursrow;

#define tty_attr_next(attr) (Tty_attribute *)attr_next((caddr_t *)attr)

/*****************************************************************************/
/*   tty_window_object -- called by window_create()                          */
/*****************************************************************************/

caddr_t
tty_window_object(win)
   Window win;
{
   Ttysw  *ttysw;
   int    notify_info;
   static	struct pixfont	 *font;

   ttysw = (Ttysw *)(LINT_CAST(ttysw_init((int)(LINT_CAST(
   	window_get(win, WIN_FD))))));
   if (!ttysw) return NULL;
   ttysw->ttysw_flags |= TTYSW_FL_USING_NOTIFIER;
   if (!init_ttytlsw((Tool *)(LINT_CAST(
   	window_get(win, WIN_OWNER))), ttysw)) return NULL;

   notify_info = 
	 defaults_get_boolean("/Tty/Retained", (Bool)0, (int *)0) ?
	 PW_RETAIN : 0;
	 
   font = (struct pixfont *)window_get(win, WIN_FONT); 

   if (ttysw_getopt((char *)(LINT_CAST(ttysw)), TTYOPT_TEXT)) {
      (void)window_set(win,
	      WIN_SHOW,			FALSE,
	      WIN_OBJECT,       	ttysw,
	      WIN_SET_PROC,     	tty_set,
	      WIN_GET_PROC,     	tty_get,
	      WIN_NOTIFY_EVENT_PROC,    ttysw_event,
	      WIN_NOTIFY_DESTROY_PROC,  ttysw_destroy,
	      WIN_MENU,			ttysw->ttysw_menu,
	      WIN_TYPE,			TTY_TYPE,
	      WIN_PIXWIN,		csr_pixwin,
	      WIN_NOTIFY_INFO,          notify_info,
	      WIN_FONT,			font,
	      0);
   } else {
      (void)window_set(win,
	      WIN_OBJECT,       	ttysw,
	      WIN_SET_PROC,     	tty_set,
	      WIN_GET_PROC,     	tty_get,
	      WIN_NOTIFY_EVENT_PROC,    ttysw_event,
	      WIN_NOTIFY_DESTROY_PROC,  ttysw_destroy,
	      WIN_MENU,			ttysw->ttysw_menu,
	      WIN_TYPE,			TTY_TYPE,
	      WIN_PIXWIN,		csr_pixwin,
	      WIN_NOTIFY_INFO,          notify_info,
	      WIN_FONT,			font,
	      0);
   }
   (void)ttysw_interpose(ttysw);
   ttysw_resize(ttysw);
   
   (void) pclearscreen(0, cursrow+1);
   /* Draw cursor on the screen and retained portion */
   (void)drawCursor(0, 0);
   return (caddr_t)ttysw;
}

/*****************************************************************************/
/*   term_window_object -- called by window_create()                         */
/*****************************************************************************/

caddr_t
term_window_object(win)
    Window win;
{
    unsigned		  status;
    extern caddr_t	  ts_create();
    extern Textsw	  ttysw_cmdsw;
    static	struct pixfont	 *base_font, *textsw_font;

    ttysw_cmdsw = (Textsw)(LINT_CAST(window_create(
    		   (struct window *)(LINT_CAST(window_get(win, WIN_OWNER))),
    		   TEXTSW,
		   TEXTSW_STATUS, &status,
		   TEXTSW_DISABLE_LOAD, TRUE,
		   TEXTSW_DISABLE_CD, TRUE,
		   TEXTSW_ES_CREATE_PROC, ts_create,
		   TEXTSW_NO_RESET_TO_SCRATCH, TRUE,
		   TEXTSW_TOOL, window_get(win, WIN_OWNER),
		   WIN_SHOW, TRUE,
    		   0)));
    if (!ttysw_cmdsw || status) {
	return NULL;
    } else {
        base_font = (struct pixfont *)window_get(win, WIN_FONT);
        textsw_font = (struct pixfont *)textsw_get(ttysw_cmdsw, TEXTSW_FONT);
        
        if (base_font != textsw_font)
            textsw_set(ttysw_cmdsw, TEXTSW_FONT, base_font,0);
    	return tty_window_object(win);
    }
}

/*****************************************************************************/
/* init_ttytlsw                                                              */
/*****************************************************************************/

static
init_ttytlsw(tool, ttysw)
   Tool *tool;
   struct ttysubwindow *ttysw;
{
   struct ttytoolsubwindow *ttytlsw;
   Notify_value             ttytlsw_destroy();
   Notify_func              func;
   char *calloc();

   ttytlsw = (struct ttytoolsubwindow *)(LINT_CAST(calloc(1, sizeof (*ttytlsw))));
   if (!ttytlsw) return NULL;

   ttytlsw->tool = tool;
   (void)ttytlsw_setup(ttytlsw, ttysw);
   func = notify_set_destroy_func((Notify_client)(LINT_CAST(ttysw)), ttytlsw_destroy);
   if (func == NOTIFY_FUNC_NULL) return NULL;
   ttytlsw->cached_destroyop = (int (*)())func;
   return TRUE;
}

/*****************************************************************************/
/* tty_set                                                                   */
/*****************************************************************************/

static
tty_set(ttysw, avlist)
	Ttysw 		*ttysw;
	Tty_attribute	avlist[];
{   
    register Tty_attribute *attrs;
    int quit_tool = -1, pid = -1, bold_style = -1, argv_set = 0;
    char **argv = 0;

    for (attrs = avlist; *attrs; attrs = tty_attr_next(attrs)) {
	switch (attrs[0]) {

	  case TTY_ARGV:
	    argv_set = 1;
	    argv = (char **)attrs[1];
	    break;

	  case TTY_CONSOLE:
	    if (attrs[1]) (void)ttysw_becomeconsole((char *)(LINT_CAST(ttysw)));
	    break;

	  case TTY_PAGE_MODE:
	    (void)ttysw_setopt((char *)(LINT_CAST(ttysw)), TTYOPT_PAGEMODE, (int)
	    	(LINT_CAST(attrs[1])));
	    break;

	  case TTY_QUIT_ON_CHILD_DEATH:
	    quit_tool = (int)attrs[1];
	    break;	      

	  case TTY_SAVE_PARAMETERS:
            /*
             *  TTY_SAVE_PARAMETERS is at best useless, at worst
             *  damaging, so we are de-implementing it.
             *
             *  ttysw_saveparms(window_fd(ttysw));
             */
	    break;

	  case TTY_BOLDSTYLE:
	    (void)ttysw_setboldstyle((int)attrs[1]);
	    break;

	  case TTY_BOLDSTYLE_NAME:
	    bold_style = ttysw_lookup_boldstyle((char *)attrs[1]);
	    if (bold_style == -1)
	    	(void)ttysw_print_bold_options();
	    else
	    	(void)ttysw_setboldstyle(bold_style);
	    break;

	  case TTY_PID:
	    /* TEXTSW_INFINITY ==> no child process, 0 ==> we want one */
	    ttysw->ttysw_pidchild = (int)attrs[1];
	    break;

	  case WIN_MENU:
	    ttysw->ttysw_menu = (Menu)attrs[1];
	    break;

	  case WIN_FONT:
	  { 
	    struct rect rect;
	    int  fd;
	    extern struct cursor ttysw_cursor;
	    
	    if (attrs[1])  {
	    	/* Cursor for the original font has been drawn, so take down */
	    	removeCursor();
	    	fd = window_fd(ttysw);
		(void)pf_close(pixfont);
	    	pixfont = (Pixfont *)attrs[1]; 
	    	chrwidth = pixfont->pf_defaultsize.x;
	    	chrheight = pixfont->pf_defaultsize.y;
	    	chrbase = -(pixfont->pf_char['n'].pc_home.y);
	    	(void)win_getsize(fd, &rect);
	    	winwidthp = rect.r_width;
	    	winheightp = rect.r_height;
		/* after changing font size, cursor needs to be re-drawn */
		(void)drawCursor(0,0);
	    }
	    break;
	  }
	  default:
	    if (ATTR_PKG_TTY == ATTR_PKG(attrs[0]))
		(void)fprintf(stderr,
			"tty_set: TTY attribute not allowed.\n%s\n",
			attr_sprint((char *)NULL, (unsigned)(LINT_CAST(
				attrs[0]))));
	    break;
	}
    }

    if (argv_set && ttysw->ttysw_pidchild == TEXTSW_INFINITY) {
	ttysw->ttysw_pidchild = 0;
    }
    if ((int)argv == TTY_ARGV_DO_NOT_FORK) {
	ttysw->ttysw_pidchild = TEXTSW_INFINITY;
    }
    if (ttysw->ttysw_pidchild <= 0) {
	pid = ttysw_fork_it((char *)(LINT_CAST(ttysw)), argv ? argv : (char **)&argv);
    }	else {
    }

    if  (pid > 0) {
	if (quit_tool > 0)
	    (void)notify_set_wait3_func((Notify_client)(LINT_CAST(ttysw)), 
	    	(Notify_func)(LINT_CAST(tty_quit_on_death)), pid);
	else if (quit_tool == 0)
	    (void)notify_set_wait3_func((Notify_client)(LINT_CAST(ttysw)), 
	    	(Notify_func)(LINT_CAST(tty_handle_death)), pid);
    }
}

/*****************************************************************************/
/* tty_get                                                                   */
/*****************************************************************************/

static caddr_t
tty_get(ttysw, attr)
	Ttysw *ttysw;
	Tty_attribute attr;
{
   switch (attr) {

      case TTY_PAGE_MODE:
	 return (caddr_t) ttysw_getopt((char *)(LINT_CAST(ttysw)), TTYOPT_PAGEMODE);

      case TTY_QUIT_ON_CHILD_DEATH:
	 return (caddr_t) 0;

      case TTY_PID:
	 return (caddr_t) ttysw->ttysw_pidchild;

      case TTY_TTY_FD:
         return (caddr_t) ttysw->ttysw_tty;

      default:
	 return FALSE;
   }
}


/* ARGSUSED */
static
tty_quit_on_death(client, pid, status, rusage)
	caddr_t client;
	int pid;
	union wait *status;
	struct rusage *rusage;
{   
    Ttysw *ttysw = (Ttysw *)client;

    if (! (WIFSTOPPED(*status))) {
	if (status->w_termsig || status->w_retcode || status->w_coredump) {
		(void)fprintf(stderr,
		"A %s window has exited because its child exited.\n",
			ttysw->ttysw_hist ? "command" : "tty");
		(void)fprintf(stderr,
		"Its child's process id was %d and it ", pid);
	    if (status->w_termsig) {
		    (void)fprintf(stderr, "died due to signal %d",
			    status->w_termsig);
	    } else if (status->w_retcode) {
		    (void)fprintf(stderr, "exited with return code %d",
			    status->w_retcode);
	    }
	    if (status->w_coredump) {
		    (void)fprintf(stderr, " and left a core dump.\n");
	    } else {
		    (void)fprintf(stderr, ".\n");
	    }
	}
	(void)window_set(window_get(client, WIN_OWNER), FRAME_NO_CONFIRM, TRUE, 0);
	(void)window_done(client);
    }
}

/* ARGSUSED */
static
tty_handle_death(ttysw, pid, status, rusage)
	Ttysw *ttysw;
	int pid;
	union wait *status;
	struct rusage *rusage;
{   
    if (! (WIFSTOPPED(*status))) {
	ttysw->ttysw_pidchild = 0;
    }
}
