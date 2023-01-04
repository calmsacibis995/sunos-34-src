
#ifndef lint
static	char sccsid[] = "@(#)setup.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include "setup.h"

Tool		*tool;
Workstation	ws;

Rect		display_rect;

extern void	message_init();
extern void	button_init();
extern void	confirm_init();
extern void	confirm_destroy();

static Notify_value	catch_quit();

unsigned short	tool_icon_image[256] = {
#include "images/setup.icon"
};
DEFINE_ICON_FROM_IMAGE(tool_icon, tool_icon_image);

setup_main(argc, argv)
int	  argc;
char	**argv;
{

	int	       	i;
	int		upgrade;
	char		**tool_attrs 	= 0;
	char		*tool_name	= "    SETUP";
	struct screen	screen;
	
	if (tool_parse_all(&argc, argv, &tool_attrs, tool_name) == -1) {
           tool_usage(tool_name);
           exit(1);
        }

        tool = tool_begin(
            WIN_LABEL,          tool_name,
            WIN_ICON,          	&tool_icon,
            WIN_LAYOUT_LOCK,	1,
            WIN_ATTR_LIST,      tool_attrs,
            0);
        tool_free_attribute_list(tool_attrs);

	notify_interpose_destroy_func(tool, catch_quit);

	win_screenget(tool->tl_windowfd, &screen);
	win_setrect(tool->tl_windowfd, &screen.scr_rect);

	rect_construct(&display_rect, 5, 15,
		       screen.scr_rect.r_width - 10,
		       screen.scr_rect.r_height - 18);

	button_init();		/* initialize the button area */

	message_init();		/* initialize the message area */

	ws = mid_init(argc, argv, message_print);/* initialize the middle end */


	confirm_init();		/* initialize the confirmer */

				/* regeister various procedures */

	setup_set(ws, 
	    SETUP_CALLBACK, handle_error, 
	    SETUP_CONFIRM_PROC, confirm_yes_no,
	    SETUP_CONTINUE_PROC, confirm_ok,
	    0);

        upgrade = (int) setup_get(ws, SETUP_UPGRADE);
        if (upgrade) {
	    upgrade_init(ws);
        }

	show_screen(MACHINE_SCREEN, 0);	/* show the first machine screen */
	
	tool_install(tool);
	notify_start();
	
	mid_cleanup();

	exit(0);
}


static Notify_value
catch_quit(tool, status)
Tool		*tool;
Destroy_status	status;
{
    if (status == DESTROY_CHECKING)
	return;

    confirm_destroy();

    return notify_next_destroy_func(tool, status);
}


/* Print an error message to stderr with printf arguments,
 * and exit with non-zero status.
 */
/*VARARGS1*/
void
setup_error(printf_string, arg1, arg2, arg3, arg4, arg5)
char	*printf_string;
caddr_t	 arg1, arg2, arg3, arg4, arg5;
{
    fprintf(stderr, printf_string, arg1, arg2, arg3, arg4, arg5);
    exit(1);
}
