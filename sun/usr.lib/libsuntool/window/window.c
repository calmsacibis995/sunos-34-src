#ifndef lint
static	char sccsid[] = "@(#)window.c 1.7 87/01/07 Copyright 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*-
	WINDOW wrapper

	window.c, Sun Aug 8 15:38:39 1985

 */

/* ------------------------------------------------------------------------- */

#include <stdio.h>
#include <varargs.h>

#include <sys/types.h>
#include <sys/time.h>

#include <sunwindow/notify.h>
#include <sunwindow/rect.h>
#include <sunwindow/win_struct.h>
#include <suntool/frame.h>			/* for font scanning */

#include "window_impl.h"

/* ------------------------------------------------------------------------- */

/* 
 * Public
 */

Window window_create();
int window_fd();
void window_main_loop();
int/*bool*/ window_done();
int/*bool*/ window_destroy();
void window_release_event_lock();
void window_refuse_kbd_focus();
int window_read_event();
void window_bell();


/* 
 * Package private
 */

Pkg_extern int/*bool*/ window_set_avlist();
Pkg_private int win_appeal_to_owner();
Pkg_private Notify_value window_default_event_func();
Pkg_private Notify_value window_default_destroy_func();
Pkg_private Window win_set_client();
Pkg_private void win_clear_client();


/* 
 * Private
 */

Private Notify_value destroy_win_struct();


#define max_windows 64 		/* ?? Change this to a call on malloc */

struct window_slot { struct window *window; caddr_t client; };
typedef struct window_slot Window_slot;
Private Window_slot win_clients[max_windows];
Private Window_slot *next_window_slot;

/* Only used in window_create() */
#define eexit(msg) \
  if (error_msg) { \
      (void)fprintf(stderr, "window: %s\n%s\n", msg, error_msg); \
      exit(1); \
  } else { \
      (void)fprintf(stderr, "window: %s\n", msg); \
      return NULL; \
  }

/* ------------------------------------------------------------------------- */


/*VARARGS2*/
Window
window_create(base_frame, create_proc, va_alist)
	struct window *base_frame;
	caddr_t (*create_proc)();
	va_dcl
{
    register struct window *win;
    caddr_t avlist[ATTR_STANDARD_SIZE];
    register Attr_avlist attrs;
    int well_behaved = TRUE, capable_of_layout = FALSE;
    char *error_msg = NULL, *name = "window";
    va_list valist;
    static short	font_scanned; /* = FALSE */
    char *defaults_get_string();
    struct pixfont *pw_pfsysopen();
    struct pixwin *pw_open();

    base_frame = client_to_win((Window)(LINT_CAST(base_frame)));

    va_start(valist);
    (void)attr_make(avlist, ATTR_STANDARD_SIZE,  valist);
    va_end(valist);

    if (!font_scanned) {
	char *fname =
		defaults_get_string("/SunView/Font", "",(int *)NULL);
	if (*fname != '\0')
	    (void)setenv("DEFAULT_FONT", fname);
    }

    for (attrs = avlist; *attrs; attrs = attr_next(attrs)) {
	switch (attrs[0]) {
	  
	  case WIN_NAME:
	    name = (char *)attrs[1];
	    break;
	  
	  case WIN_ERROR_MSG:
	    error_msg = (char *)attrs[1];
	    break;
	  
	  case WIN_COMPATIBILITY:
	    well_behaved = FALSE;
	    break;
	
	  case FRAME_ARGS:
	  case FRAME_ARGC_PTR_ARGV:
	    if (!font_scanned)
		(void)tool_parse_font((((Frame_attribute) attrs[0]) == FRAME_ARGS) ? 
			    (int) (LINT_CAST(attrs[1])) : 
			    *((int *)(LINT_CAST(attrs[1]))),
			    (char **)(LINT_CAST(attrs[2])));
	    break;

	  default:
	    break;
	}
    }

    win = (struct window *)(LINT_CAST(calloc(sizeof(struct window), 1)));
    if (!win) eexit("Unable to allocate additional storage.  Malloc failed.");

    win->type = WINDOW_TYPE;
    win->name = name;
    win->object = (caddr_t)win;  /* Hopefully this is changed by create_proc */
    win->fd = -1;
    win->well_behaved = well_behaved;
    win->show_updates = TRUE;
    win->default_event_proc = (void (*)())window_default_event_func;

    win->font = pw_pfsysopen(); /* ?? Fix this (see frame) */
    font_scanned = TRUE;

    while (base_frame && !base_frame->layout_proc)
	base_frame = base_frame->owner;
    win->owner = base_frame;

    if (well_behaved) {
	win->fd = win_getnewwindow();
	if (win->fd < 0) eexit("Window creation failed to get new fd");
	if (base_frame == NULL) {
	    int parentlink;
	    char parentname[WIN_NAMESIZE];
	
	    if (we_getparentwindow(parentname))
		eexit("Base frame not passed parent window in environment");
	    parentlink = win_nametonumber(parentname);
	    (void)win_setlink(win->fd, WL_PARENT, parentlink);
	} else {
	    int parentlink;
	
	    parentlink = win_fdtonumber(base_frame->fd);
	    (void)win_setlink(win->fd, WL_PARENT, parentlink);
	}    
	win->pixwin = pw_open(win->fd);
	win->show = TRUE;
    }

   /*
    * create subwindow in frame
    */
    (void)win_set_client(win, win->object, TRUE);
    
    if (base_frame && base_frame->layout_proc && well_behaved)
	(base_frame->layout_proc)(base_frame->object, win->object,
				  WIN_CREATE, win->name);
    
    if (!(create_proc)(win->object,avlist)) eexit("Subwindow creation failed.");
    if (win->object == (caddr_t)win)
	eexit("Subwindow failed to create underlying object.");
    if (win->layout_proc)
	(win->layout_proc)(win->object, win->object,
			   WIN_LAYOUT, &capable_of_layout);
    if (!base_frame && !capable_of_layout)
	eexit("Subwindow needs a base frame.");
    
    if (base_frame && base_frame->layout_proc) {
	(base_frame->layout_proc)(base_frame->object, win->object,
				  WIN_INSTALL, win->name);
    }

    if (well_behaved)		/* Set attrs */
	(void)window_set(win->object, ATTR_LIST, avlist, 0);
    else {			/* Set window attrs ONLY */
	window_rc_units_to_pixels(win->object, 
		(Window_attribute *)(LINT_CAST(avlist)));
	(void)window_set_avlist(win, (Window_attribute *)(LINT_CAST(avlist)));
    }

    if (win->object != (caddr_t)win) {
	(void)notify_interpose_destroy_func(win->object, destroy_win_struct);
    }
    
    win->created = TRUE;
    return (Window)(LINT_CAST(win->object));
}


int
window_fd(window)
	Window window;
{   
    struct window *win;
    
    win = client_to_win(window);
    if (!win) return -1;
    return win->fd;
}


void
window_main_loop(frame)
        Window frame;
{   
    struct window *win = client_to_win(frame);
 
    if (!(win && win->object)) return;
    win->show = TRUE;
    (void)win_insert(win->fd);	 /* install win in tree */
    /* do the initial subwindow layout */
    (void)tool_layoutsubwindows((struct tool *)(LINT_CAST(frame)));
    (void)notify_start();		 /* main loop */
}


int/*bool*/
window_done(win)
	Window win;
{   
    register Window owner = win;
    
    while (owner) win = owner, owner = window_get(owner, WIN_OWNER);
    return window_destroy(win);
}
   

int/*bool*/
window_destroy(window)
	Window window;

{
    int result;
    register struct window *win;
    
    win = client_to_win(window);
    if (!win) return FALSE;

    if (win->object) {
	if (NOTIFY_DESTROY_VETOED != notify_post_destroy(win->object,
							 DESTROY_CHECKING,
							 NOTIFY_IMMEDIATE)) {
	    (void)notify_post_destroy(win->object, DESTROY_CLEANUP, NOTIFY_SAFE);
	    result = TRUE;
	} else {
	    result = FALSE;
	}		
    } else {
	win_clear_client((Window)(LINT_CAST(win)));
	result = TRUE;
    }
    return result;
}


void
window_release_event_lock(window)
Window window;
{
    (void)win_release_event_lock(window_fd(window));
}

void
window_refuse_kbd_focus(window)
Window window;
{
    (void)win_refuse_kbd_focus(window_fd(window));
}

int
window_read_event(window, event)
Window window;
struct inputevent *event;
{
    return input_readevent(window_fd(window), event);
}

void
window_bell(window)
Window window;
{
    struct timeval tv;

    tv.tv_sec  = 0;
    tv.tv_usec = 100000;	/* 1/10 second */
    (void)win_bell(window_fd(window), tv,
	     (struct pixwin *)(LINT_CAST(window_get(window, WIN_PIXWIN))));
}


Pkg_private int
win_appeal_to_owner(adjust, win, op, datum)
	int adjust;
	struct window *win;
	caddr_t op;
	caddr_t datum;
{
    if (win->owner && win->owner->layout_proc) {
	(win->owner->layout_proc)
	    (win->owner->object, win->object, op, datum);
	return adjust;
    }
    if (win->layout_proc && (adjust || !win->owner)) {
	(win->layout_proc)(win->owner ? win->object : NULL,
			     win->object, op, datum);
	return TRUE;
    }
    return TRUE;
}


Pkg_private Notify_value
window_default_event_func()
{
    return NOTIFY_DONE;
}


Pkg_private Notify_value
window_default_destroy_func()
{
    return NOTIFY_DONE;
}


/* 
 * This routine converts a client handle into a pointer to a window struct.
 * The only call to this routine should be in the macro client_to_win().
 */
Pkg_private Window
win_from_client(client)
	register Window client;
{   
    register Window_slot *wp = win_clients;
    
    /* Clear the cache (Not sure this feature is used anywhere) */    
    if (client == NULL || next_window_slot == NULL)
	return win_last_client = win_last_window = NULL;

    next_window_slot->client = client;  /* Guarantee that search terminates */
    while (wp->client != client) wp++;  /* Search the assoc table */
    if (wp == next_window_slot) return NULL; /* Search failed */
    win_last_client = client;		/* Search successed, set cache */
    win_last_window = (Window)wp->window;
    
    return win_last_window;
}


Pkg_private caddr_t
win_set_client(win, client, create)
	struct window *win;
	caddr_t client;
	int/*bool*/ create;
{
    register Window_slot *wp = win_clients;
    
    if (!next_window_slot)			/* Initialize slot ptr */
	next_window_slot = win_clients;
    if (create) {				/* Find first empty slot */
	next_window_slot->client = NULL;
	while (wp->client) wp++;
	if (wp == next_window_slot) next_window_slot++;
    } else {					/* Look for existing entry */
	next_window_slot->window = win;
	while (wp->window != win) wp++;
	if (wp == next_window_slot) return NULL;/* Search Failed */
    }
    wp->client = win->object = win_last_client = client;
    wp->window = client ? win : 0;
    win_last_window = (Window)wp->window;

    return win->object;
}


Pkg_private void
win_clear_client(win)
	Window win;
{   
    (void)win_set_client((struct window *)(LINT_CAST(win)), 
    		(caddr_t)NULL, FALSE);
    free(win);
}


Private Notify_value
destroy_win_struct(client, status)
	register caddr_t client;
	Destroy_status status;
{
    register struct window *win = client_to_win(client);
    Notify_value result = notify_next_destroy_func(client, status);

    if (status == DESTROY_CLEANUP && win) {
	if (win->well_behaved) {
	    (void)win_unregister(client);
	    (void)pw_close(win->pixwin);
	}
	/* This is done after the unregister to prevent double destroys */
	if (win->owner && win->owner->layout_proc)
	    (win->owner->layout_proc)
		(win->owner->object, client, WIN_DESTROY);
	win_clear_client((Window)(LINT_CAST(win)));
	result = NOTIFY_DONE;
    }
    return result;
}
