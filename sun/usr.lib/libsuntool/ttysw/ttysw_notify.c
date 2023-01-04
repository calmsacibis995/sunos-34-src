#ifndef lint
static  char sccsid[] = "@(#)ttysw_notify.c 1.8 87/05/04 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Notifier related routines for the ttysw.
 */
#include <sys/types.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sgtty.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <sunwindow/notify.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_notify.h>
#include <sunwindow/defaults.h>
#include <suntool/ttysw.h>
#include <suntool/window.h>
#include <suntool/tool_struct.h>
#include <suntool/menu.h>
	/* tool.h must be before any indirect include of textsw.h */
#include "ttysw_impl.h"

/* #ifdef CMDSW */
extern void	  ttysw_display_capslock();
extern void	  ttynullselection();
extern Textsw	  textsw_first();
extern Textsw	  textsw_next();
extern caddr_t	  textsw_get();
extern int	  textsw_default_notify();
extern char	  *textsw_checkpoint_undo();
extern Textsw_index	textsw_insert();
extern Textsw_status	textsw_set();
extern void		textsw_post_error();
extern Textsw_index	textsw_erase();
extern void		textsw_display();

extern Cmdsw	*cmdsw;
extern struct ttysubwindow *_ttysw;

#ifdef DEBUG
#define ERROR_RETURN(val)	abort();	/* val */
#else
#define ERROR_RETURN(val)	return(val);
#endif DEBUG


/* #else */
#include "charimage.h"
#include "charscreen.h"
#undef length
#define ITIMER_NULL   ((struct itimerval *)0)

#define	TTYSW_USEC_DELAY 100000

	/* Duplicate of what's in ttysw_tio.c */

Notify_value		ttysw_itimer_expired();
/* #endif CMDSW */

/* #ifndef CMDSW */
static        Notify_value    ttysw_win_input_pending();
/* #endif CMDSW */
static        Notify_value    ttysw_text_input_pending();
static	Notify_value	ttysw_pty_output_pending();
Notify_value  		ttysw_pty_input_pending();
Notify_value		ttysw_prioritizer();
void			ttysw_add_pty_timer();
void			ttysw_remove_pty_timer();
Notify_value		ttysw_pty_timer_expired();
static void		ttysw_sendsig();


static	void		ttysw_sigwinch();

/* shorthand - Duplicate of what's in ttysw_main.c */

#define	iwbp	ttysw->ttysw_ibuf.cb_wbp
#define	irbp	ttysw->ttysw_ibuf.cb_rbp
#define	iebp	ttysw->ttysw_ibuf.cb_ebp
#define	ibuf	ttysw->ttysw_ibuf.cb_buf
#define	owbp	ttysw->ttysw_obuf.cb_wbp
#define	orbp	ttysw->ttysw_obuf.cb_rbp

int	ttysw_waiting_for_pty_input;	/* Accelerator to avoid excessive
					   notifier activity */
static	ttysw_waiting_for_pty_output;	/* Accelerator to avoid excessive
					   notifier activity */
Notify_func ttysw_cached_pri;		/* Default prioritizer */

static	Notify_value ttysw_text_destroy();	/* Destroy func for cmdsw */
static	Notify_value ttysw_text_event();	/* Event func for cmdsw */

/* Only extern for use by tty_window_object() in tty.c */
ttysw_interpose(ttysw)
    Ttysw                *ttysw;
{
    (void) notify_interpose_input_func((Notify_client)ttysw,
	    ttysw_win_input_pending,
	    (int)LINT_CAST(window_get((Window)(LINT_CAST(ttysw)), WIN_FD)));
    if (ttysw_getopt((caddr_t)ttysw, TTYOPT_TEXT)) {
	Textsw		  textsw = (Textsw) ttysw->ttysw_hist;

	(void)notify_interpose_event_func((Notify_client)textsw, ttysw_text_event, NOTIFY_SAFE);
	(void)notify_interpose_destroy_func((Notify_client)textsw, ttysw_text_destroy);
	(void) notify_interpose_input_func((Notify_client)textsw,
	    ttysw_text_input_pending,
	    (int)LINT_CAST(window_get((Window)(LINT_CAST(ttysw->ttysw_hist)), 
	    	WIN_FD)));
    }
    (void) notify_set_input_func((Notify_client)ttysw, ttysw_pty_input_pending,
				 ttysw->ttysw_pty);
    ttysw_waiting_for_pty_input = 1;
    ttysw_cached_pri = notify_set_prioritizer_func((Notify_client)ttysw,
						   ttysw_prioritizer);
}

caddr_t
ttysw_create(tool, name, width, height)
    struct tool          *tool;
    char                 *name;
    short                 width, height;
{
    struct toolsw        *toolsw;
    extern struct pixwin *csr_pixwin;
    Bool retained;
    Ttysw                *ttysw;

    /* Create an empty subwindow */
    if ((toolsw = tool_createsubwindow(tool, name, width, height)) ==
	(struct toolsw *) 0)
	return (NULL);
    /* Make the empty subwindow into a tty subwindow */
    if ((toolsw->ts_data = ttysw_init(toolsw->ts_windowfd)) == NULL)
	return (NULL);
    ((Ttysw *)LINT_CAST(toolsw->ts_data))->ttysw_flags |= TTYSW_FL_USING_NOTIFIER;
    ttysw = (Ttysw *)LINT_CAST(toolsw->ts_data);
    /* Register with window manager */
    if (ttysw_getopt((caddr_t)ttysw, TTYOPT_TEXT)) {
	toolsw->ts_data = (caddr_t)ttysw->ttysw_hist;
	(void)textsw_set((Textsw)ttysw->ttysw_hist, TEXTSW_TOOL, tool, 0);
	(void)ttysw_interpose(ttysw);
    }
    /* begin ttysw only */
    retained = defaults_get_boolean("/Tty/Retained",
	(Bool)FALSE, (int *)NULL);
    if (win_register((char *)ttysw, csr_pixwin, ttysw_event,
		     ttysw_destroy, (unsigned)((retained)? PW_RETAIN: 0)))
	return (NULL);
    /* Draw cursor on the screen and retained portion */
    (void)drawCursor(0, 0);
    /* end ttysw only */
    return ((caddr_t)ttysw);
}

int
ttysw_start(ttysw_client, argv)
    Ttysubwindow        ttysw_client;
    char                **argv;
{
    int                   pid;
    Ttysw		  *ttysw;
#ifdef DEBUG          
    Notify_func		  func;	/* for debugging */
#endif DEBUG                                                 

    ttysw = (Ttysw *)(LINT_CAST(ttysw_client));
    if ((pid = ttysw_fork_it((caddr_t)ttysw, argv)) != -1) {
	/* Wait for child process to die */
	(void) notify_set_wait3_func((Notify_client)(LINT_CAST(ttysw)), 
		notify_default_wait3, pid);
#ifdef DEBUG          
      func = notify_get_output_func(ttysw, ttysw->ttysw_pty);
      func = notify_get_input_func(ttysw, ttysw->ttysw_pty);
      func = notify_get_prioritizer_func(ttysw);
      if (pid == -1) notify_dump(ttysw, 0, 2);
#endif DEBUG                                                 
    }
    return (pid);
}

static Notify_value
ttysw_text_destroy(textsw, status)
    register Textsw     textsw;
    Destroy_status        status;
{
    Ttysw              *ttysw;
    Notify_value	nv = NOTIFY_IGNORED;
    int			last_view;
    

    /* WARNING: call on notify_next_destroy_func invalidates textsw,
     *	and thus the information about the textsw must be extracted first.
     */
    last_view = (textsw_next(textsw_first(textsw)) == (Textsw) 0);
    ttysw = (Ttysw *)LINT_CAST(textsw_get(textsw, TEXTSW_CLIENT_DATA));
	/* WARNING: ttysw may be NULL */

    if (status == DESTROY_CHECKING) {
        return (NOTIFY_IGNORED);
    } else {
	nv = notify_next_destroy_func((Notify_client)(LINT_CAST(textsw)), status);
    }
    if (last_view && ttysw) {
	nv = ttysw_destroy(ttysw, status);
    }
    return nv;
}

Notify_value
ttysw_destroy(ttysw, status)
    Ttysw                *ttysw;
    Destroy_status        status;
{
    if (status != DESTROY_CHECKING) {
    	ttysw_remove_pty_timer(ttysw);
        (void) notify_set_itimer_func((Notify_client)(LINT_CAST(ttysw)), 
		ttysw_itimer_expired,
	    ITIMER_REAL, (struct itimerval *) 0, ITIMER_NULL);
	    
	 /* Send both signal to cover all bases  */    
	ttysw_sendsig(ttysw, (Textsw) 0, SIGTERM);
        ttysw_sendsig(ttysw, (Textsw) 0, SIGHUP);
           
	(void)win_unregister((char *)(LINT_CAST(ttysw)));
	if (ttysw->ttysw_hist) {
	    window_set(ttysw->ttysw_hist, TEXTSW_CLIENT_DATA, 0, 0);
	}
	(void)ttysw_done((caddr_t)ttysw);
	return (NOTIFY_DONE);
    }
    return (NOTIFY_IGNORED);
}

static void
ttysw_sigwinch(ttysw)
    Ttysw               *ttysw;
{
    int                  pgrp;
    int			 sig = SIGWINCH;

    /*
     * 2.0 tty based programs relied on getting SIGWINCHes at times other
     * then when the size changed.  Thus, for compatibility, we also do
     * that here.  However, I wish that I could get away with only sending
     * SIGWINCHes on resize.
     */
    /* Notify process group that terminal has changed. */
    if (ioctl(ttysw->ttysw_tty, TIOCGPGRP, &pgrp) == -1) {
	perror("ttysw_sigwinch, can't get tty process group");
	return;
    }
    /*
     * Only killpg when pgrp is not tool's.  This is the case of haven't
     * completed ttysw_fork yet (or even tried to do it yet). 
     */
    if (getpgrp(0) != pgrp)
	/*
	 * killpg could return -1 with errno == ESRCH but this is OK. 
	 */
	/* (void) killpg(pgrp, SIGWINCH); */
        ioctl(ttysw->ttysw_pty, TIOCSIGNAL, &sig);
    return;
}

/* #ifdef CMDSW */
static Textsw_index
find_and_remove_mark(textsw, mark)
    Textsw		textsw;
    Textsw_mark		mark;
{
    Textsw_index	result;

    result = textsw_find_mark(textsw, mark);
    if (result != TEXTSW_INFINITY)
	textsw_remove_mark(textsw, mark);
    return(result);
}

static void
move_mark(textsw, mark, to, flags)
    Textsw		 textsw;
    Textsw_mark		*mark;
    Textsw_index	 to;
    int			 flags;
{
    textsw_remove_mark(textsw, *mark);
    *mark = textsw_add_mark(textsw, to, (unsigned)flags);
}

static Notify_value
ttysw_cr(ttysw, tty)
    Ttysw                *ttysw;
    int                   tty;
{
    int			  nfds = 0, wfds = 1 << tty;
    static struct timeval timeout = {0, 0};
    
    /*
     *  GROSS HACK:
     *
     *  There is a race condition such that between the notifier's
     *  select() call and our write, the app may write to the tty,
     *  causing our write to block.  The tty cannot be flushed because
     *  we don't get to read the pty because our write is blocked.
     *  This GROSS HACK doesn't eliminate the race condition; it merely
     *  narrows the window, making it less likely to occur.
     *  We don't do an fcntl(tty, FN_NODELAY) because that affects the
     *  file, not merely the file descriptor, and we don't want to change
     *  what the application thinks it sees.
     *
     *  The right solution is either to invent an ioctl that will allow
     *  us to set the tty driver's notion of the cursor position, or to
     *  avoid using the tty driver altogether.
     */
    if ((nfds = select(NOFILE, NULL, &wfds, NULL, &timeout)) < 0) {
	perror("ttysw_cr: select");
	return (NOTIFY_IGNORED);
    }
    if (nfds == 0 || !((1 << tty) & wfds)) {
	return (NOTIFY_IGNORED);
    }
    if (write(tty, "\r", 1) < 0) {
	fprintf(stderr, "for ttysw %x, tty fd %d, ",
		ttysw, ttysw->ttysw_tty);
	perror("TTYSW tty write failure");
    }
    (void) notify_set_output_func((Notify_client)(LINT_CAST(ttysw)), 
    	NOTIFY_FUNC_NULL, tty);
    return (NOTIFY_DONE);
}

static void
ttysw_reset_column(ttysw)
    Ttysw                *ttysw;
{
    if ((cmdsw->sgttyb.sg_flags & XTABS)
    &&  notify_get_output_func((Notify_client)(LINT_CAST(ttysw)),
				ttysw->ttysw_tty) != ttysw_cr) {
	if (notify_set_output_func((Notify_client)(LINT_CAST(ttysw)),
	    ttysw_cr, ttysw->ttysw_tty) == NOTIFY_FUNC_NULL) {
	    fprintf(stderr,
	    	    "cannot set output func on ttysw %x, tty fd %d\n",
	    	    ttysw, ttysw->ttysw_tty);
	}
    }
}

int
ttysw_scan_for_completed_commands(ttysw, start_from, maybe_partial)
    Ttysw		*ttysw;
    int			 start_from;
    int			 maybe_partial;
{
    register Textsw	 textsw = (Textsw)ttysw->ttysw_hist;
    register char	*cp;
    int			 length = (int)textsw_get(textsw, TEXTSW_LENGTH);
    int			 use_mark = (start_from == -1);
    int			 cmd_length;

    if (use_mark) {
	if (TEXTSW_INFINITY == (
		start_from = textsw_find_mark(textsw, cmdsw->user_mark) ))
		ERROR_RETURN(1);
	if (start_from == length)
	    return(0);
    }
    cmd_length = length - start_from;
    /* Copy these commands into the buffer for pty */
    if ((iwbp+cmd_length) < iebp) {
	(void) textsw_get(textsw,
		TEXTSW_CONTENTS, start_from, iwbp, cmd_length);
	if (maybe_partial) {
	    /* Discard partial commands. */
	    for (cp = iwbp+cmd_length-1; cp >= iwbp; --cp) {
		switch (*cp) {
		  case '\n':
		  case '\r':
		    goto Done;
		  default:
		    if (*cp == cmdsw->tchars.t_brkc)
		      goto Done;
		    cmd_length--;
		    break;
		}
	    }
	}
Done:
	if (cmd_length > 0) {
	    iwbp += cmd_length;
	    cp = iwbp-1;
	    (void)ttysw_reset_conditions(ttysw);
	    if (*cp == '\n'
	    ||  *cp == '\r') {
		ttysw_reset_column(ttysw);
	    }
	    move_mark(textsw, &cmdsw->pty_mark,
	    	      (Textsw_index)(start_from+cmd_length),
		      TEXTSW_MARK_DEFAULTS);
	    if (cmdsw->cmd_started) {
		if (start_from+cmd_length < length) {
		    move_mark(textsw, &cmdsw->user_mark,
			      (Textsw_index)(start_from+cmd_length),
			      TEXTSW_MARK_DEFAULTS);
		} else {
		    cmdsw->cmd_started = 0;
		}
		if (cmdsw->append_only_log) {
		    move_mark(textsw, &cmdsw->read_only_mark,
			      (Textsw_index)(start_from+cmd_length),
			      TEXTSW_MARK_READ_ONLY);
		}
	    }
	    cmdsw->pty_owes_newline = 0;
	}
	return(0);
    } else {
	textsw_post_error(textsw, 0, 0,
		"Pty cmd buffer overflow: last cmd ignored.", (char *)0);
	return(0);
    }
}

void
ttysw_doing_pty_insert(textsw, commandsw, toggle)
    register Textsw	 textsw;
    Cmdsw		*commandsw;
    unsigned		 toggle;
{
    unsigned		 notify_level = (unsigned) window_get(textsw,
    					 TEXTSW_NOTIFY_LEVEL);
    commandsw->doing_pty_insert = toggle;
    if (toggle) {
	window_set(textsw,
	    TEXTSW_NOTIFY_LEVEL, notify_level & (~TEXTSW_NOTIFY_EDIT),
	    0);
    } else {
	window_set(textsw,
	    TEXTSW_NOTIFY_LEVEL, notify_level | TEXTSW_NOTIFY_EDIT,
	    0);
    }
}

static 
ttysw_cooked_echo_cmd(ttysw_opaque, buf, buflen)
    caddr_t             ttysw_opaque;
    char               *buf;
    int                 buflen;
{
    Ttysw		*ttysw = (Ttysw *) LINT_CAST(ttysw_opaque);
    register Textsw	 textsw = (Textsw)ttysw->ttysw_hist;
    Textsw_index	 insert = (Textsw_index)textsw_get(textsw,
						  TEXTSW_INSERTION_POINT);
    int			 length = (Textsw_index)textsw_get(textsw,
						  TEXTSW_LENGTH);
    Textsw_index	 insert_at;
    Textsw_mark		 insert_mark;

    if (cmdsw->append_only_log) {
	textsw_remove_mark(textsw, cmdsw->read_only_mark);
    }
    if (cmdsw->cmd_started) {
	insert_at = find_and_remove_mark(textsw, cmdsw->user_mark);
	if (insert_at == TEXTSW_INFINITY)
	    ERROR_RETURN(-1);
	if (insert == insert_at) {
	    insert_mark = TEXTSW_NULL_MARK;
	} else {
	    insert_mark =
		textsw_add_mark(textsw, insert, TEXTSW_MARK_DEFAULTS);
	}
    } else {
	if (insert == length)
	    (void)textsw_checkpoint_again(textsw);
	    cmdsw->next_undo_point = textsw_checkpoint_undo(textsw,
	           		(caddr_t)TEXTSW_INFINITY);
	insert_at = length;
    }
    if (insert != insert_at) {
	(void)textsw_set(textsw, TEXTSW_INSERTION_POINT, insert_at, 0);
    }
    (void)textsw_checkpoint_undo(textsw, cmdsw->next_undo_point);
    /* Stop this insertion from triggering the cmd scanner! */
    ttysw_doing_pty_insert(textsw, cmdsw, TRUE);
    (void)textsw_insert(textsw, buf, (long) buflen);
    ttysw_doing_pty_insert(textsw, cmdsw, FALSE);
    (void) ttysw_scan_for_completed_commands(ttysw, (int) insert_at, TRUE);
    if (cmdsw->cmd_started) {
	insert_at = (Textsw_index)textsw_get(textsw, TEXTSW_INSERTION_POINT);
	if (insert_at == TEXTSW_INFINITY)
	    ERROR_RETURN(-1);
	cmdsw->user_mark =
	    textsw_add_mark(textsw, (Textsw_index)insert_at, TEXTSW_MARK_DEFAULTS);
	if (cmdsw->append_only_log) {
	    cmdsw->read_only_mark =
		textsw_add_mark(textsw,
		    cmdsw->cooked_echo ? insert_at : TEXTSW_INFINITY-1,
		    TEXTSW_MARK_READ_ONLY);
	}
	if (insert_mark != TEXTSW_NULL_MARK) {
	    insert = find_and_remove_mark(textsw, insert_mark);
	    if (insert == TEXTSW_INFINITY)
		ERROR_RETURN(-1);
	    (void)textsw_set(textsw, TEXTSW_INSERTION_POINT, insert, 0);
	}
    } else {
	if (insert < length)
	    (void)textsw_set(textsw, TEXTSW_INSERTION_POINT, insert, 0);
	if (cmdsw->append_only_log) {
	    length = (int)textsw_get(textsw, TEXTSW_LENGTH);
	    cmdsw->read_only_mark =
		textsw_add_mark(textsw,
		    (Textsw_index)(cmdsw->cooked_echo ? length : TEXTSW_INFINITY-1),
		    TEXTSW_MARK_READ_ONLY);
	}
    }
    return (0);
}

extern
ttysw_cmd(ttysw_opaque, buf, buflen)
    caddr_t             ttysw_opaque;
    char               *buf;
    int                 buflen;
{
    Ttysw		*ttysw = (Ttysw *) LINT_CAST(ttysw_opaque);
    
    if (ttysw_getopt((caddr_t)ttysw, TTYOPT_TEXT)
    &&  cmdsw->cooked_echo) {
	return (ttysw_cooked_echo_cmd(ttysw_opaque, buf, buflen));
    } else {
	return (ttysw_input((caddr_t)ttysw, buf, buflen));
    }
}

static void
ttysw_sendsig(ttysw, textsw, sig)
    Ttysw                *ttysw;
    Textsw                textsw;
    int			  sig;
{
    int control_pg;
    
    /* Send the signal to the process group of the controlling tty */
    if (ioctl(ttysw->ttysw_pty, TIOCGPGRP, &control_pg) >= 0) {
	/*
	 *  Flush our buffers of completed and partial commands.
	 *  Be sure to do this BEFORE killpg, or we'll flush
	 *  the prompt coming back from the shell after the
	 *  process dies.
	 */
	(void)ttysw_flush_input((caddr_t)ttysw);
	
	if (textsw)
	    move_mark(textsw, &cmdsw->pty_mark,
		  (Textsw_index)textsw_get(textsw, TEXTSW_LENGTH),
		  TEXTSW_MARK_DEFAULTS);
	cmdsw->cmd_started = 0;
	cmdsw->pty_owes_newline = 0;
        /* (void) killpg(control_pg, sig); */
        ioctl(ttysw->ttysw_pty, TIOCSIGNAL, &sig);
    } else
        perror("ioctl");
}

/* #endif CMDSW */

static Notify_value
ttysw_text_event(textsw, event, arg, type)
    register Textsw	  textsw;
    Event                *event;
    Notify_arg     	  arg;
    Notify_event_type     type;
{
    Ttysw		*ttysw;
    int			 insert = TEXTSW_INFINITY;
    int			 length = TEXTSW_INFINITY;
    int			 cmd_start;
    int			 did_map = 0;
    Notify_value	 nv = NOTIFY_IGNORED;
    Menu		 menu;
    Menu_item		 menu_item;

    ttysw = (Ttysw *)LINT_CAST(textsw_get(textsw, TEXTSW_CLIENT_DATA));
    
    if ((event_id(event) == MS_RIGHT) && event_is_down(event)) {
    /*
     * To ensure the textsw from MENU_CLIENT_DATA is one where the
     * menu come up, instead of the one the menu is created from.
     */
        menu = (Menu)textsw_get(textsw, TEXTSW_MENU);
        /* The reason for using menu_get() is menu_find() has problem. */
        menu_item = menu_get(menu,
        			MENU_NTH_ITEM, menu_get(menu, MENU_NITEMS, 0)); 
        			
        (void)menu_set(menu_item, MENU_CLIENT_DATA, textsw, 0);
    }    
    
    if (!ttysw_getopt((caddr_t)ttysw, TTYOPT_TEXT)) {
	nv = notify_next_event_func((Notify_client)(LINT_CAST(textsw)), 
		(Notify_event)(LINT_CAST(event)), arg, type);
        return (nv);
    }
    
    if (cmdsw->cooked_echo
    && (cmdsw->cmd_started == 0)
    && (insert = (int)textsw_get(textsw, TEXTSW_INSERTION_POINT))
    == (length = (int)textsw_get(textsw, TEXTSW_LENGTH))) {
	(void)textsw_checkpoint_again(textsw);
    }
    if (cmdsw->cooked_echo
    &&  cmdsw->cmd_started
    &&  cmdsw->literal_next
    &&  event_id(event) <= ASCII_LAST
    && (insert = (int)textsw_get(textsw, TEXTSW_INSERTION_POINT))
    == (length = (int)textsw_get(textsw, TEXTSW_LENGTH))) {
	char	input_char = (char)event_id(event);
	textsw_replace_bytes(textsw, length-1, length, &input_char, 1);
	cmdsw->literal_next = FALSE;
	return NOTIFY_DONE;
    }
    /* ^U after prompt, before newline should only erase back to prompt. */
    if (cmdsw->cooked_echo
    &&  event_id(event) == (short)cmdsw->erase_line
    &&  event_is_down(event)
    && !event_shift_is_down(event)
    &&  cmdsw->cmd_started != 0
    &&  ((insert = (int)textsw_get(textsw, TEXTSW_INSERTION_POINT))) >
        (cmd_start = (int)textsw_find_mark(textsw, cmdsw->user_mark))) {
        int			pattern_start = cmd_start;
        int			pattern_end = cmd_start;
        char			newline = '\n';

        if (textsw_find_bytes(
            textsw, (long *)&pattern_start, (long *)&pattern_end, &newline, 1, 0) == -1
        || (pattern_start <= cmd_start || pattern_start >= (insert-1))) {
	    (void)textsw_erase(textsw,
	        (Textsw_index)cmd_start, (Textsw_index)insert);
	    return NOTIFY_DONE;
        }
    }
    if (!cmdsw->cooked_echo
    &&  (event_id(event) == '\r'|| event_id(event) == '\n')
    &&  (event->ie_shiftmask & CTRLMASK)
    &&  (win_get_vuid_value(ttysw->ttysw_wfd, 'm') == 0)
    &&  (win_get_vuid_value(ttysw->ttysw_wfd, 'j') == 0)
    &&  (win_get_vuid_value(ttysw->ttysw_wfd, 'M') == 0)
    &&  (win_get_vuid_value(ttysw->ttysw_wfd, 'J') == 0) ) {
	/* Implement "go to end of file" ourselves. */
	/* First let textsw do it to get at normalize_internal. */
	nv = notify_next_event_func((Notify_client)(LINT_CAST(textsw)), 
		(Notify_client)(LINT_CAST(event)), arg, type);
	/* Now fix it up.
	 * Only necessary when !append_only_log because otherwise
	 * the read-only mark at INFINITY-1 gets text to implement
	 * this function for us.
	 */
	if (!cmdsw->append_only_log)
	    (void)textsw_set(textsw, TEXTSW_INSERTION_POINT,
		       textsw_find_mark(textsw, cmdsw->pty_mark), 0);
    } else if (!cmdsw->cooked_echo
    &&  event_id(event) <= ASCII_LAST
    &&  (iscntrl((char)event_id(event)) || (char)event_id(event) == '\177')
    &&  (insert = (int)textsw_get(textsw, TEXTSW_INSERTION_POINT))
    ==  textsw_find_mark(textsw, cmdsw->pty_mark)) {
	/* In !cooked_echo, ensure textsw doesn't gobble up control chars */
	char	input_char = (char)event_id(event);
	(void)textsw_insert(textsw, &input_char, (long) 1);
	nv = NOTIFY_DONE;
    } else if (cmdsw->cooked_echo
    &&  event_id(event) == cmdsw->tchars.t_stopc) {
	/* implement flow control characters as page mode */
	(void) ttysw_freeze(ttysw, 1);
    } else if (cmdsw->cooked_echo
    &&  event_id(event) == cmdsw->tchars.t_startc) {
	(void) ttysw_freeze(ttysw, 0);
	(void) ttysw_reset_conditions(ttysw);
    } else if (!cmdsw->cooked_echo
    ||  event_id(event) != (short)cmdsw->tchars.t_eofc) {
	/* Nice normal event */
	nv = notify_next_event_func((Notify_client)(LINT_CAST(textsw)), 
		(Notify_event)(LINT_CAST(event)), arg, type);
    }
    if (nv == NOTIFY_IGNORED) {
	if ((event->ie_code > META_LAST) &&
	    (event->ie_code < LOC_MOVE || event->ie_code > WIN_UNUSED_15)) {
	    did_map = (ttysw_domap(ttysw, event) == TTY_DONE);
	    nv = did_map ? NOTIFY_DONE : NOTIFY_IGNORED;
	}
    }
    /* the following switch probably belongs in a state transition table */
    switch (event_id(event)) {
    case WIN_REPAINT:
	ttysw_sigwinch(ttysw);
	nv = NOTIFY_DONE;
	break;
    case WIN_RESIZE:
	(void)ttysw_resize(ttysw);
	nv = NOTIFY_DONE;
	break;
    case KBD_USE:
	if ((Textsw)ttysw->ttysw_hist != textsw) {
	    (Textsw)ttysw->ttysw_hist = textsw;
	    (void)ttynewsize(textsw_screen_column_count(textsw),
		textsw_screen_line_count(textsw));
	    ttysw->ttysw_wfd =
	    	(int)window_get((Window)(LINT_CAST(textsw)), WIN_FD);
	}
	break;
    case KBD_DONE:
	break;
    case LOC_MOVE:
	break;
    case LOC_STILL:
	break;
    case LOC_WINENTER:
 	break;
    case LOC_WINEXIT:
	break;
    default:
#ifdef DEBUG
	if (event_id(event) <= ASCII_LAST) {
	    int		ctrl_state = event->ie_shiftmask & CTRLMASK;
	    int		shift_state = event->ie_shiftmask & SHIFTMASK;
	    char	ie_code = event->ie_code;
	}
#endif DEBUG
	if (!cmdsw->cooked_echo)
	    break;
	/* Only send interrupts when characters are actually typed. */
	if (event->ie_code == cmdsw->tchars.t_intrc) {
	    ttysw_sendsig(ttysw, textsw, SIGINT);
	} else if (event->ie_code == cmdsw->tchars.t_quitc) {
	    ttysw_sendsig(ttysw, textsw, SIGQUIT);
	} else if (event->ie_code == cmdsw->ltchars.t_suspc
	       ||  event->ie_code == cmdsw->ltchars.t_dsuspc) {
	    ttysw_sendsig(ttysw, textsw, SIGTSTP);
	} else if (event->ie_code == cmdsw->tchars.t_eofc) {
	    if (insert == TEXTSW_INFINITY)
		insert = (int)textsw_get(textsw, TEXTSW_INSERTION_POINT);
	    if (length == TEXTSW_INFINITY)
		length = (int)textsw_get(textsw, TEXTSW_LENGTH);
	    if (length == insert) {
		/* handle like newline or carriage return */
		if (cmdsw->cmd_started
		&&  length > textsw_find_mark(textsw, cmdsw->user_mark)) {
		    if (ttysw_scan_for_completed_commands(ttysw, -1, 0))
			nv = NOTIFY_IGNORED;
		} else {
		    /* but remember to send eot. */
		    cmdsw->pty_eot = iwbp - ibuf;
		    cmdsw->cmd_started = 0;
		    (void)ttysw_reset_conditions(ttysw);
		}
	    } else {   /* length != insert */
		nv = notify_next_event_func((Notify_client)(LINT_CAST(textsw)), 
			(Notify_client)(LINT_CAST(event)), arg, type);
	    }
	}
    }   /* switch */
    return(nv);
}

/* ARGSUSED */
Notify_value
ttysw_event(ttysw, event, arg, type)
    Ttysw                *ttysw;
    Event                *event;
    Notify_arg     	  arg;
    Notify_event_type     type;
{
    switch (event_id(event)) {
      case WIN_REPAINT:
	if (ttysw->ttysw_hist && cmdsw->cmd_started) {
	    (void)ttysw_scan_for_completed_commands(ttysw, -1, 0);
	}
	(void)ttysw_display((Ttysubwindow)(LINT_CAST(ttysw)));
	return (NOTIFY_DONE);
      case WIN_RESIZE:
	(void)ttysw_resize(ttysw);
	return (NOTIFY_DONE);
      default:
	if (ttysw_eventstd(ttysw, event) == TTY_DONE)
	    return (NOTIFY_DONE);
	else
	    return (NOTIFY_IGNORED);
    }
}

/* #ifdef CMDSW */
/* ARGSUSED */
static void
ttysw_textsw_changed_handler(textsw, insert_before, length_before,
                             replaced_from, replaced_to, count_inserted)
    Textsw		textsw;
    int			insert_before;
    int			length_before;
    int			replaced_from;
    int			replaced_to;
    int			count_inserted;
{
    Ttysw              *ttysw = (Ttysw *)LINT_CAST(textsw_get(textsw,
    					  TEXTSW_CLIENT_DATA));
    char		last_inserted;
    
    if (insert_before != length_before)
        return;
    if (cmdsw->cmd_started == 0) {
	if (cmdsw->cmd_started = (count_inserted > 0)) {
	    (void)textsw_checkpoint_undo(textsw, cmdsw->next_undo_point);
	    move_mark(textsw, &cmdsw->user_mark,
		      (Textsw_index)length_before,
		      TEXTSW_MARK_DEFAULTS);
	}
    }
    if (!cmdsw->cmd_started)
	cmdsw->next_undo_point =
	       (caddr_t)textsw_checkpoint_undo(textsw,
			(caddr_t)TEXTSW_INFINITY);
    if (count_inserted >= 1) {
        /* Get the last inserted character. */
        (void)textsw_get(textsw, TEXTSW_CONTENTS,
                   replaced_from + count_inserted - 1,
                   &last_inserted, 1);
	if (last_inserted == cmdsw->ltchars.t_rprntc) {
#ifndef	BUFSIZE
#define	BUFSIZE 1024
#endif	BUFSIZE
	    char 		buf[BUFSIZE+1];
	    char 		cr_nl[3];
	    int			buflen = 0;
	    Textsw_index	start_from;
	    Textsw_index	length =
	    	(int)textsw_get(textsw, TEXTSW_LENGTH);
	    
	    cr_nl[0] = '\r';  cr_nl[1] = '\n';  cr_nl[2] = '\0';
	    start_from = textsw_find_mark(textsw, cmdsw->user_mark);
	    if (start_from == (length - 1)) {
		*buf = '\0';
	    } else {
		(void)textsw_get(textsw, TEXTSW_CONTENTS,
                   start_from, buf,
                   (buflen = min(BUFSIZE, length - 1 - start_from)));
            }
            cmdsw->pty_owes_newline = 0;
            cmdsw->cmd_started = 0;
            move_mark(textsw, &cmdsw->pty_mark, length,
		      TEXTSW_MARK_DEFAULTS);
            if (cmdsw->append_only_log) {
		move_mark(textsw, &cmdsw->read_only_mark, length,
			  TEXTSW_MARK_READ_ONLY);
	    }
	    ttysw_output(ttysw, cr_nl, 2);
	    if (buflen > 0)
		ttysw_input(ttysw, buf, buflen);
	} else if (last_inserted == cmdsw->ltchars.t_lnextc) {
	    cmdsw->literal_next = TRUE;
	} else if (last_inserted == cmdsw->tchars.t_brkc
	||  last_inserted == '\n'
	||  last_inserted == '\r') {
	    (void) ttysw_scan_for_completed_commands(ttysw, -1, 0);
	}
    }
}

extern int
ttysw_textsw_changed(textsw, attributes)
    Textsw		textsw;
    Attr_avlist		attributes;
{
    register Attr_avlist	 attrs;
    register Ttysw		*ttysw;
    int				 do_default = 0;

    for (attrs = attributes; *attrs; attrs = attr_next(attrs)) {
        switch ((Textsw_action)(*attrs)) {
	  case TEXTSW_ACTION_CAPS_LOCK:
	    ttysw = (Ttysw *)LINT_CAST(textsw_get(textsw, TEXTSW_CLIENT_DATA));
	    ttysw->ttysw_capslocked = (attrs[1] != 0);
	    (void)ttysw_display_capslock(ttysw);
	    break;
          case TEXTSW_ACTION_REPLACED:
            if (!cmdsw->doing_pty_insert)
		ttysw_textsw_changed_handler(textsw,
		    (int)attrs[1], (int)attrs[2], (int)attrs[3],
		    (int)attrs[4], (int)attrs[5]);
            break;
          case TEXTSW_ACTION_SPLIT_VIEW: {
            int				 text_fd;

	    ttysw = (Ttysw *)
	        LINT_CAST(textsw_get(textsw, TEXTSW_CLIENT_DATA));
	    (void) notify_interpose_event_func(attrs[1], ttysw_text_event,
		                               NOTIFY_SAFE);
	    (void) notify_interpose_destroy_func(attrs[1],
		                                 ttysw_text_destroy);
	    text_fd = (int)
	        LINT_CAST(window_get((Textsw)(LINT_CAST(attrs[1])), WIN_FD));
	    (void) notify_interpose_input_func((Notify_client)attrs[1],
		ttysw_text_input_pending, text_fd);
	    if ((Ttysw *)
	        LINT_CAST(textsw_get(
	        (Textsw)(LINT_CAST(attrs[1])), TEXTSW_CLIENT_DATA))
	    !=  ttysw)
		(void)textsw_set(
		(Textsw)(LINT_CAST(attrs[1])), TEXTSW_CLIENT_DATA, ttysw, 0);
	    if ((int)window_get(textsw, WIN_KBD_FOCUS)
	    &&  (int)window_get((Textsw)(LINT_CAST(attrs[1])), WIN_Y) >
	        (int)window_get(textsw, WIN_Y))
	        (void)window_set(
	        (Textsw)(LINT_CAST(attrs[1])), WIN_KBD_FOCUS, TRUE, 0);
            break;
	  }
          case TEXTSW_ACTION_DESTROY_VIEW: {
            Textsw	coalesce_with = NULL;
            /*
             *  There is no way for the user to directly destroy
             *  the current view if it is replaced by the ttysw.
             *  Therefore, he tried to destroy the first view and
             *  textsw decided to destroy this view instead.
             */
	    ttysw = (Ttysw *)LINT_CAST(textsw_get(textsw, TEXTSW_CLIENT_DATA));
	    if (! (coalesce_with = (Textsw) window_get(textsw,
		TEXTSW_COALESCE_WITH))) {
		coalesce_with = textsw_first(textsw);
	    }
	    if ((Textsw)ttysw->ttysw_hist == textsw) {
		ttysw->ttysw_hist = (FILE *) LINT_CAST(coalesce_with);
	    }
	    if ((int)window_get(textsw, WIN_KBD_FOCUS)) {
		window_set(coalesce_with, WIN_KBD_FOCUS, TRUE, 0);
            }
            break;
          }
          case TEXTSW_ACTION_LOADED_FILE: {
            Textsw_index		insert;
            Textsw_index		length;
            
	    ttysw = (Ttysw *)LINT_CAST(textsw_get(textsw, TEXTSW_CLIENT_DATA));
            insert =
		(Textsw_index)textsw_get(textsw, TEXTSW_INSERTION_POINT);
	    length = (Textsw_index)textsw_get(textsw, TEXTSW_LENGTH);
	    if (length == insert+1) {
		(void)textsw_set(textsw, TEXTSW_INSERTION_POINT, length, 0);
		ttysw_reset_column(ttysw);
	    } else if (length == 0) {
		ttysw_reset_column(ttysw);
	    }
	    if (length < textsw_find_mark(textsw, cmdsw->pty_mark)) {
		move_mark(textsw, &cmdsw->pty_mark, length,
			  TEXTSW_MARK_DEFAULTS);
	    }
	    if (cmdsw->append_only_log) {
		move_mark(textsw, &cmdsw->read_only_mark,
			  length, TEXTSW_MARK_READ_ONLY);
	    }
	    cmdsw->cmd_started = FALSE;
	    cmdsw->pty_owes_newline = 0;
	  }
          default:
            do_default = TRUE;
            break;
        }
    }
    if (do_default) {
	(void)textsw_default_notify(textsw, attributes);
    }
}
/* #endif CMDSW */

ttysw_display(ttysw_client)
    Ttysubwindow         ttysw_client;
{
    Ttysw	*ttysw;
    
    ttysw = (Ttysw *)(LINT_CAST(ttysw_client));
    if (ttysw_getopt((caddr_t)ttysw, TTYOPT_TEXT)) {
	textsw_display((Textsw)ttysw->ttysw_hist);
    } else {
	extern struct pixwin *csr_pixwin;
	extern                wfd;
    
	(void)saveCursor();
	(void)prepair(wfd, &csr_pixwin->pw_clipdata->pwcd_clipping);
	/*
	 * If just hilite the selection part that is damaged then the other
	 * non-damaged selection parts should still be visible, thus creating
	 * the entire selection image. 
	 */
	ttysel_hilite(ttysw);
	(void)restoreCursor();
    }
    ttysw_sigwinch(ttysw);
}

static Notify_value
ttysw_pty_output_pending(ttysw, pty)
    Ttysw                *ttysw;
    int                   pty;
{    
    (void)ttysw_pty_output(ttysw, pty);
    return (NOTIFY_DONE);
}

Notify_value
ttysw_pty_input_pending(ttysw, pty)
    Ttysw                *ttysw;
    int                   pty;
{
    (void)ttysw_pty_input(ttysw, pty);
    return (NOTIFY_DONE);
}

/* #ifndef CMDSW */
/* ARGSUSED */
Notify_value
ttysw_itimer_expired(ttysw, which)
    Ttysw                *ttysw;
    int                   which;
{
    (void)ttysw_handle_itimer(ttysw);
    return (NOTIFY_DONE);
}

static Notify_value
ttysw_win_input_pending(ttysw, win_fd)
    Ttysw                *ttysw;
    int                   win_fd;
{
    /*
     * Differs from readwin in that don't loop around input_readevent
     * until get a EWOULDBLOCK. 
     */
    if (iwbp >= iebp)
	return (NOTIFY_IGNORED);
    return(notify_next_input_func((Notify_client)ttysw, win_fd));
}
/* #endif CMDSW */

static Notify_value
ttysw_text_input_pending(textsw, win_fd)
    Textsw                 textsw;
    int                   win_fd;
{
    Ttysw                *ttysw =
    			 (Ttysw *)LINT_CAST(textsw_get(textsw, TEXTSW_CLIENT_DATA));
    /*
     * Differs from readwin in that don't loop around input_readevent
     * until get a EWOULDBLOCK. 
     */
    if (iwbp >= iebp)
	return (NOTIFY_IGNORED);
    return(notify_next_input_func((Notify_client)textsw, win_fd));
}

/*
 * Conditionally set conditions
 */
ttysw_reset_conditions(ttysw)
    register Ttysw       *ttysw;
{
/* #ifndef CMDSW */
    static struct itimerval ttysw_itimerval = {{0, 0}, {0, TTYSW_USEC_DELAY}};
/* #endif */
    static struct itimerval pty_itimerval = {{0, 0}, {0, 10}};
    register int          pty = ttysw->ttysw_pty;

    /* Send program output to terminal emulator */
    (void)ttysw_consume_output(ttysw);
    /* Toggle between window input and pty output being done */
    if (iwbp > irbp
    || (ttysw_getopt((caddr_t)ttysw, TTYOPT_TEXT) && cmdsw->pty_eot > -1)) {
	if (!ttysw_waiting_for_pty_output) {
	    /* Wait for output to complete on pty */
	    (void) notify_set_output_func((Notify_client)(LINT_CAST(ttysw)),
					  ttysw_pty_output_pending, pty);
	    ttysw_waiting_for_pty_output = 1;
	    (void)ttysw_add_pty_timer(ttysw, &pty_itimerval);
	}
    } else {
	if (ttysw_waiting_for_pty_output) {
	    /* Don't wait for output to complete on pty any more */
	    (void) notify_set_output_func((Notify_client)(LINT_CAST(ttysw)), 
	    		NOTIFY_FUNC_NULL, pty);
	    ttysw_waiting_for_pty_output = 0;
	}
    }
    /* Set pty input pending */
    if (owbp == orbp) {
	if (!ttysw_waiting_for_pty_input) {
	    (void) notify_set_input_func((Notify_client)ttysw,
					 ttysw_pty_input_pending, pty);
	    ttysw_waiting_for_pty_input = 1;
	}
    } else {
	if (ttysw_waiting_for_pty_input) {
	    (void) notify_set_input_func((Notify_client)ttysw, NOTIFY_FUNC_NULL,
					 pty);
	    ttysw_waiting_for_pty_input = 0;
	}
    }
    /*
     * Try to optimize displaying by waiting for image to be completely filled
     * after being cleared (vi(^F ^B) page) before painting. 
     */
    if (!ttysw_getopt((caddr_t)ttysw, TTYOPT_TEXT) && delaypainting)
	(void) notify_set_itimer_func((Notify_client)(LINT_CAST(ttysw)), 
			ttysw_itimer_expired,
		       ITIMER_REAL, &ttysw_itimerval, ITIMER_NULL);
}

Notify_value
ttysw_prioritizer(ttysw, nfd, ibits_ptr, obits_ptr, ebits_ptr,
		  nsig, sigbits_ptr, auto_sigbits_ptr, event_count_ptr, events)
    register Ttysw       *ttysw;
    int                  *ibits_ptr, *obits_ptr, *ebits_ptr;
    int			  nsig, *sigbits_ptr, *event_count_ptr;
    register int         *auto_sigbits_ptr, nfd;
    Notify_event         *events;
{
    register int          bit;
    register int          pty = ttysw->ttysw_pty;


    ttysw->ttysw_flags |= TTYSW_FL_IN_PRIORITIZER;
    if (*auto_sigbits_ptr) {
	/* Send itimers */
	if (*auto_sigbits_ptr & SIG_BIT(SIGALRM)) {
	    (void) notify_itimer((Notify_client)(LINT_CAST(ttysw)), ITIMER_REAL);
	    *auto_sigbits_ptr &= ~SIG_BIT(SIGALRM);
	}
    }
    if (*obits_ptr & (bit = FD_BIT(ttysw->ttysw_tty))) {
	(void) notify_output((Notify_client)(LINT_CAST(ttysw)), ttysw->ttysw_tty);
	*obits_ptr &= ~bit;
    }
    if (*ibits_ptr & (bit = FD_BIT(ttysw->ttysw_wfd))) {
	(void) notify_input((Notify_client)(LINT_CAST(ttysw)), ttysw->ttysw_wfd);
	*ibits_ptr &= ~bit;
    }
    if (*obits_ptr & (bit = FD_BIT(pty))) {
	(void) notify_output((Notify_client)(LINT_CAST(ttysw)), pty);
	*obits_ptr &= ~bit;
	(void)ttysw_remove_pty_timer(ttysw);
    }
    if (*ibits_ptr & (bit = FD_BIT(pty))) {
	(void) notify_input((Notify_client)(LINT_CAST(ttysw)), pty);
	*ibits_ptr &= ~bit;
    }
    (void) ttysw_cached_pri(ttysw, nfd, ibits_ptr, obits_ptr, ebits_ptr,
		nsig, sigbits_ptr, auto_sigbits_ptr, event_count_ptr, events);
    (void)ttysw_reset_conditions(ttysw);
    ttysw->ttysw_flags &= ~TTYSW_FL_IN_PRIORITIZER;

    return (NOTIFY_DONE);
}

ttysw_resize(ttysw)
    Ttysw                *ttysw;
{
    int                   pagemode;

    /*
     * Turn off page mode because send characters through character image
     * manager during resize. 
     */
    pagemode = ttysw_getopt((caddr_t)ttysw, TTYOPT_PAGEMODE);
    (void)ttysw_setopt((caddr_t)ttysw, TTYOPT_PAGEMODE, 0);
    if (ttysw_getopt((caddr_t)ttysw, TTYOPT_TEXT)) {
	(void)ttynewsize(textsw_screen_column_count((Textsw)ttysw->ttysw_hist),
		   textsw_screen_line_count((Textsw)ttysw->ttysw_hist));
    } else {
	/* Have character image update self */
	(void)csr_resize(ttysw);
	/* Have screen update any size change parameters */
	(void)cim_resize(ttysw);
	if (ttysw->ttysw_hist) {
	    cmdsw->ttysw_resized++;
	}
    }
    /* Turn page mode back on */
    (void)ttysw_setopt((caddr_t)ttysw, TTYOPT_PAGEMODE, pagemode);
}

/* #ifndef CMDSW */
cim_resize(ttysw)
    Ttysw                *ttysw;
{
    struct rectlist       rl;
    extern struct pixwin *csr_pixwin;
    int                   (*tmp_getclipping) ();
    int                   ttysw_null_getclipping();

    /* Prevent any screen writing by making clipping null */
    rl = rl_null;
    (void)pw_restrict_clipping(csr_pixwin, &rl);
    /* Make sure clipping remains null */
    tmp_getclipping = csr_pixwin->pw_clipops->pwco_getclipping;
    csr_pixwin->pw_clipops->pwco_getclipping = ttysw_null_getclipping;
    /* Redo character image */
    (void)imagerepair(ttysw);
    /* Restore get clipping function */
    csr_pixwin->pw_clipops->pwco_getclipping = tmp_getclipping;
}

csr_resize(ttysw)
    Ttysw                *ttysw;
{
    struct rect           r_new;
    extern                wfd;

    /* Update notion of size */
    (void)win_getsize(wfd, &r_new);
    winwidthp = r_new.r_width;
    winheightp = r_new.r_height;
    /* Don't currently support selections across size changes */
    ttynullselection(ttysw);
}

ttysw_null_getclipping()
{
}
/* #endif CMDSW */

void
ttysw_add_pty_timer(ttysw, itimer)
    register Ttysw                *ttysw;
    register struct itimerval	*itimer;
{
    
    if (NOTIFY_FUNC_NULL ==
        notify_set_itimer_func((Notify_client)(LINT_CAST(&ttysw->ttysw_pty)),
				   ttysw_pty_timer_expired, ITIMER_REAL,
				   itimer, ITIMER_NULL))
	    notify_perror("textsw_add_timer");
}

void
ttysw_remove_pty_timer(ttysw)
    register Ttysw                *ttysw;
{
    ttysw_add_pty_timer(ttysw, ITIMER_NULL);
} 

Notify_value
ttysw_pty_timer_expired(pty, which)
    register int              pty;
    int				which;
{
    register Ttysw	*ttysw;
/*
**	Added by brentb (8-12-86) to take care of input queue overflow warning
*/
/*    int			max_fds = getdtablesize(); */
	/*
	 * Hard wire the return constant to let 3.4 binary runs in 4.0
	 */
    int			max_fds = NOFILE;
    int			writefds;
    static struct timeval	tv = {0,0};
    struct prompt		prompt;
    struct inputevent		event;
    static int  		warned_inputq_full; /* It is already initialized to 0 */
    int				nfd;
    extern struct pixfont	*pw_pfsysopen();
    
    ttysw = _ttysw; /*global data */
    writefds = FD_BIT(ttysw->ttysw_pty);
    
    if(!ttysw_getopt((caddr_t)ttysw, TTYOPT_TEXT) && 
	ttysw_waiting_for_pty_output)
    {
	if((nfd = select(max_fds, NULL, &writefds, NULL, &tv)) < 0)
		perror("TTYSW select");

	if((nfd == 0) && !warned_inputq_full)
	{
		rect_construct(&prompt.prt_rect,
			PROMPT_FLEXIBLE, PROMPT_FLEXIBLE,
			PROMPT_FLEXIBLE, PROMPT_FLEXIBLE);
		prompt.prt_font = pw_pfsysopen();
		prompt.prt_text = "WARNING - ttysw input buffer full.  Press the \
left mouse button to flush the input queue, right mouse button to ignore.  \
Choosing the \"Flush\" menu item will also flush the input queue.";
		(void)menu_prompt(&prompt, &event, wfd);
		(void)pw_pfsysclose();

		if((event.ie_code == SELECT_BUT) && win_inputposevent(&event))
		{
			(void)ttysw_flush_input((caddr_t)ttysw);
			warned_inputq_full = FALSE;
		}
		else if((event.ie_code == MENU_BUT) && win_inputposevent(&event))
			warned_inputq_full = TRUE;
	}
    }
    else
	warned_inputq_full = FALSE; 
/*
**	End of addition
*/
    return(NOTIFY_DONE);
}
