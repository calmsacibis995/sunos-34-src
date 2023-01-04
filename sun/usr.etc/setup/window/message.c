
#ifndef lint
static	char sccsid[] = "@(#)message.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup.h"
#include <suntool/textsw.h>

static Textsw			textsw;


/* height of the message area */
#define	MESSAGE_HEIGHT		80


/* initialize the read-only message area */
void
message_init()
{

    Rect	rect;

    rect		= display_rect;
    rect.r_height	= MESSAGE_HEIGHT;

    textsw = textsw_build(tool, 
    			TEXTSW_MENU, 0,		/* supress menu */
    			TEXTSW_BROWSING, 1,	/* window is read-only */
     			0);
    win_setrect(win_get_fd(textsw), &rect);

    display_rect.r_top		= rect_bottom(&rect) + 6;
    display_rect.r_height	-= rect.r_height + 6;
}

/*
 * print a message in the message area
 * make sure it is visible, and update the scrollbar
 */
void
message_print(string)
char	*string;
{
    if (string) {
        textsw_set(textsw, TEXTSW_BROWSING, 0, 0);
        textsw_set(textsw, TEXTSW_INSERTION_POINT, TEXTSW_INFINITY, 0);
        textsw_insert(textsw, "\n", 1);
        textsw_insert(textsw, string, strlen(string));
        textsw_possibly_normalize(textsw, 
        		(int) textsw_get(textsw, TEXTSW_INSERTION_POINT));
        textsw_set(textsw, TEXTSW_UPDATE_SCROLLBAR, 0);
        textsw_set(textsw, TEXTSW_BROWSING, 1, 0);
    }
}


void
message_repaint()
{
    textsw_display(textsw);
}
