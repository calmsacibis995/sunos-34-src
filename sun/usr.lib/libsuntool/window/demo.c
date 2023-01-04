#ifndef lint
static        char sccsid[] = "@(#)demo.c 1.3 87/01/07 Copyright 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*-
	DEMO PROGRAM for window wrappers

	demo.c, Wed Aug 28 09:05:53 1985

		Craig Taylor,
		Sun Microsystems
 */

#include <stdio.h>

#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/text.h>
#include <suntool/tty.h>

#include <sys/resource.h>

Window		frame, prop_sheet1, prop_sheet2;
Window		control_panel, output_panel, text_window, tty_window;
Window		make_prop_sheet();
Panel_item	output_item;
int		date_proc(), rusage_proc();
int		confirm_proc(), destroy_proc(), quit_proc(), close_proc();
int 		width, height;
Rect		rr, rr1;


main(argc, argv)	
	int argc;
	char *argv[];
{   
   caddr_t *pargs = 0;
   /* 
    *  Create base frame to hold subwindows
    */
    frame = window_create(ROOT_FRAME, FRAME, FRAME_LABEL, argv[0],
			  WIN_HEIGHT, 200, WIN_WIDTH, 500,
			  WIN_ERROR_MSG, "Can't make frame",
  			  FRAME_ARGS, argc, argv, /* See below */
			  0);

/*   This code is the one buggy case left when a tool starts iconic */
/*   tool_parse_all(&argc, argv, &pargs, "foo"); /* Test ATTR_LIST for frames */
/*   window_set(frame, ATTR_LIST, pargs, 0);     /* Work for all attrs? */

    rr = *(Rect *)window_get(frame, FRAME_CURRENT_RECT);
    printf("Rect = %d, %d, %d, %d.\n", rr.r_left, rr.r_top, rr.r_width, rr.r_height);
    
	
   /* 
    * Initialize control panel
    */
    control_panel =
	window_create(frame, PANEL,
		      WIN_ERROR_MSG, "Can't create control_panel",
		      0);

    panel_create_item(control_panel, PANEL_BUTTON,
		      PANEL_LABEL_STRING, "Date", 
		      PANEL_NOTIFY_PROC,  date_proc, 
		      0);
    panel_create_item(control_panel, PANEL_BUTTON,
		      PANEL_LABEL_STRING, "Resources",
		      PANEL_NOTIFY_PROC,  rusage_proc, 
		      0);
    panel_create_item(control_panel, PANEL_BUTTON,
		      PANEL_LABEL_STRING, "Confirm",
		      PANEL_NOTIFY_PROC,  confirm_proc, 
		      0);
    panel_create_item(control_panel, PANEL_BUTTON,
		      PANEL_LABEL_STRING, "Destroy",
		      PANEL_NOTIFY_PROC,  destroy_proc, 
		      0);
    panel_create_item(control_panel, PANEL_BUTTON,
		      PANEL_LABEL_STRING, "Quit",
		      PANEL_NOTIFY_PROC,  quit_proc, 
		      0);
    panel_create_item(control_panel, PANEL_BUTTON,
		      PANEL_LABEL_STRING, "Close",
		      PANEL_NOTIFY_PROC,  close_proc, 
		      0);
    window_fit_height(control_panel);

   /* 
    *  Initialize output panel
    */
    output_panel =
	window_create(frame, PANEL, 
		      WIN_X, 0, WIN_BELOW, control_panel,
		      WIN_ERROR_MSG, "Can't create output_panel",
		      0);

    output_item = panel_create_item(output_panel,
				    PANEL_MESSAGE, PANEL_LABEL_STRING,
				    "Select with left mouse button.",
				    0);
    window_fit_height(output_panel);

    text_window =
	window_create(frame, TEXT,
		      WIN_X, 0, WIN_BELOW, output_panel,
		      WIN_WIDTH, ATTR_LINES(40), WIN_HEIGHT, 300, 
		      WIN_ERROR_MSG, "Can't create text window",
		      0);
    
    tty_window =
	window_create(frame, TTY,
		      WIN_RIGHT_OF, text_window,
		      WIN_BELOW, output_panel, 
		      WIN_COLUMNS, 40, 
		      WIN_ERROR_MSG, "Can't create tty window",
		      TTY_QUIT_ON_CHILD_DEATH, TRUE,
		      0);
	
     window_fit(frame);

   /* 
    * Enter processing loop
    */
    window_main_loop(frame);
    
    exit(0);
}


date_proc(item, event)	/* get and display date and time */
	Panel_item item;
	struct inputevent *event;
{
	long clock;
	char *ctime();

	time(&clock);
	panel_set(output_item, PANEL_LABEL_STRING, ctime(&clock), 0);

    rr = *(Rect *)window_get(frame, FRAME_CURRENT_RECT);
    printf("Rect = %d, %d, %d, %d.\n", rr.r_left, rr.r_top, rr.r_width, rr.r_height);
    
}


rusage_proc(item, event)	/* get and display resource usage */
	Panel_item item;
	struct inputevent *event;
{
	struct rusage rusage;
	static char buf[80];

	getrusage(RUSAGE_SELF, &rusage);
	sprintf(buf, "User %D secs %D millisecs; System %D secs %D millisecs",
	    rusage.ru_utime.tv_sec, rusage.ru_utime.tv_usec/1000,
	    rusage.ru_stime.tv_sec, rusage.ru_stime.tv_usec/1000);
	panel_set(output_item, PANEL_LABEL_STRING, buf, 0);
}


confirm_proc(item, event)
	Panel_item item;
	struct inputevent *event;
{
	if (!prop_sheet1) prop_sheet1 = make_prop_sheet();
	window_set(prop_sheet1, WIN_SHOW, TRUE, 0);
}


destroy_proc(item, event)
	Panel_item item;
	struct inputevent *event;
{
    if (!prop_sheet1) return;
    window_destroy(prop_sheet1);
    prop_sheet1 = NULL;
}


quit_proc(item, event)
	Panel_item item;
	struct inputevent *event;
{
    window_set(frame, FRAME_NO_CONFIRM, TRUE, 0);
    if (window_done(panel_get(item, PANEL_PARENT_PANEL)) == FALSE)
	printf("Quit was vetoed.\n");
}

close_proc(item, event)
	Panel_item item;
	struct inputevent *event;
{
    window_set(frame, FRAME_CLOSED, TRUE, 0);
}


Window
make_prop_sheet()
{   
    Window	prop_sheet;
    Panel	confirm_panel;
    int		yes_proc(), no_proc(), confirm_proc2();
    int		quit_proc(), destroy_proc2();

    prop_sheet = window_create(frame, FRAME,
			       WIN_NAME, "Confirm",
			       WIN_ERROR_MSG, "Can't make property sheet",
			       0);

    /* setup confirm panel */
    confirm_panel = window_create(prop_sheet, PANEL,
				  WIN_ERROR_MSG, "Can't create confirm_panel",
/*   				  PANEL_LAYOUT, PANEL_VERTICAL,*/
				  0);
    
    panel_create_item(confirm_panel, PANEL_BUTTON,
		      PANEL_LABEL_STRING, "Yes", 
		      PANEL_NOTIFY_PROC,  yes_proc, 
		      0);
    panel_create_item(confirm_panel, PANEL_BUTTON,
		      PANEL_LABEL_STRING, "No", 
		      PANEL_NOTIFY_PROC,  no_proc, 
		      0);

    panel_create_item(confirm_panel, PANEL_BUTTON,
		      PANEL_LABEL_STRING, "Confirm",
		      PANEL_NOTIFY_PROC,  confirm_proc2, 
		      0);
    panel_create_item(confirm_panel, PANEL_BUTTON,
		      PANEL_LABEL_STRING, "Destroy",
		      PANEL_NOTIFY_PROC,  destroy_proc2, 
		      0);
    panel_create_item(confirm_panel, PANEL_BUTTON,
		      PANEL_LABEL_STRING, "Quit",
		      PANEL_NOTIFY_PROC,  quit_proc, 
		      0);
    window_fit(confirm_panel);
    window_fit(prop_sheet);
    return prop_sheet;
}


confirm_proc2(item, event)
	Panel_item item;
	struct inputevent *event;
{
	if (!prop_sheet2) prop_sheet2 = make_prop_sheet();
	window_set(prop_sheet2, WIN_SHOW, TRUE, 0);
}


yes_proc(item, event)
	Panel_item item;
	struct inputevent *event;
{
    window_set(window_get(panel_get(item, PANEL_PARENT_PANEL), WIN_OWNER),
	       WIN_SHOW, FALSE, 0);
}


no_proc(item, event)
	Panel_item item;
	struct inputevent *event;
{
    window_set(window_get(panel_get(item, PANEL_PARENT_PANEL), WIN_OWNER),
	       WIN_SHOW, FALSE, 0);
}


destroy_proc2(item, event)
	Panel_item item;
	struct inputevent *event;
{
    if (!prop_sheet2) return;
    window_destroy(prop_sheet2);
    prop_sheet2 = NULL;
}
