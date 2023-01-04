#ifndef lint
static	char sccsid[] = "@(#)window_loop.c 1.4 87/01/07 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <suntool/sunview.h>
#include <suntool/fullscreen.h>

extern struct pixrect	*save_bits();
extern int		restore_bits();

static short		no_return /* = FALSE */;
static caddr_t		return_value;


caddr_t
window_loop(frame)
register Frame	frame;
{
    register Window	first_window;
    register int	frame_fd, first_fd;
    struct pixrect	*bitmap_under;
    struct fullscreen	*fs;
    Rect		screen_rect, rect;
    Event		event;

    /* we don't support recursive window_loop() calls yet.
     * Also, no support for already-shown confirmer.
     * Also, can't be iconic.
     */
    if (no_return || window_get(frame, WIN_SHOW) || 
	window_get(frame, FRAME_CLOSED))
	return (caddr_t) 0;

    /* insert the frame in the window tree.
     * Note that this will not alter the screen
     * until the WIN_REPAINT is sent below.
     */
    (void)window_set(frame, WIN_SHOW, TRUE, 0);

    screen_rect 	= *((Rect *) (LINT_CAST(
    				window_get(frame, WIN_SCREEN_RECT))));
    rect 		= *((Rect *) (LINT_CAST(
    				window_get(frame, WIN_RECT))));
    frame_fd 		= (int) window_get(frame, WIN_FD);

    first_window 	= window_get(frame, FRAME_NTH_WINDOW, 0);
    first_fd 		= (int) window_get(first_window, WIN_FD);

    /* constrain the frame rect to be on the
     * screen.
     */
    (void)wmgr_constrainrect(&rect, &screen_rect, 0, 0);

    /* lock the window tree in hopes of
     * preventing screen updates until the
     * grabio.
     */
    (void)win_lockdata(frame_fd);

    rect.r_left = rect.r_top = 0;
    fs = fullscreen_init(frame_fd);
    if ((bitmap_under = save_bits(fs->fs_pixwin, &rect)) == 0) {
	(void)win_unlockdata(frame_fd);
	(void)fullscreen_destroy(fs);
	return (caddr_t) 0;
    }
    (void)pw_preparesurface(fs->fs_pixwin, &rect);
    (void)fullscreen_destroy(fs);

    /* make sure the frame and first_window
     * are in sync with their current size.
     */
    (void)pw_exposed(window_get(frame, WIN_PIXWIN));
    (void)pw_exposed(window_get(first_window, WIN_PIXWIN));
    (void)win_post_id(first_window, WIN_RESIZE, NOTIFY_IMMEDIATE);
    (void)win_post_id(frame,        WIN_RESIZE, NOTIFY_IMMEDIATE);

    /* paint the frame */
    (void)win_post_id(frame, WIN_REPAINT, NOTIFY_IMMEDIATE);

    /* paint the first subwindow */
    (void)win_post_id(first_window, WIN_REPAINT, NOTIFY_IMMEDIATE);
    (void)win_post_id(first_window, LOC_WINENTER, NOTIFY_IMMEDIATE);
    (void)win_post_id(first_window, KBD_USE, NOTIFY_IMMEDIATE);

    (void)win_grabio(first_fd);

    (void)win_unlockdata(frame_fd);

    /* read and post events to the
     * first subwindow until window_return()
     * is called.
     */
    no_return = TRUE;
    while (no_return) {
	(void)input_readevent(first_fd, &event);
	switch (event_id(&event)) {
	    case LOC_WINENTER:
	    case LOC_WINEXIT:
	    case KBD_USE:
	    case KBD_DONE:
	    case KEY_LEFT(5):	/* top/bottom */
	    case KEY_LEFT(7):	/* close/open */
		break;

	    default:
            /* don't mess with the data lock --
             * leave the grabio lock on; either the
             * kernel will be fixed to allow all windows in this
             * process to paint, or we require that only the
             * first_fd window can paint.
                (void)win_lockdata(frame_fd);
                (void)win_releaseio(first_fd);
             */

		(void)win_post_event(first_window, &event, NOTIFY_IMMEDIATE);

	     /*
		(void)win_grabio(first_fd);
		(void)win_unlockdata(frame_fd);
	     */
		break;
	}
    }

    /* lock the window tree until the
     * bits are restored.
     */
    (void)win_lockdata(frame_fd);
    (void)win_releaseio(first_fd);

    (void)win_post_id(first_window, KBD_DONE, NOTIFY_IMMEDIATE);
    (void)win_post_id(first_window, LOC_WINEXIT, NOTIFY_IMMEDIATE);

    fs = fullscreen_init(frame_fd);
    (void)restore_bits(fs->fs_pixwin, &rect, bitmap_under);
    (void)fullscreen_destroy(fs);

    (void)window_set(frame, WIN_SHOW, FALSE, 0);

    (void)win_unlockdata(frame_fd);

    no_return = FALSE;
    return return_value;
}


void
window_return(value)
caddr_t	value;
{
    no_return = FALSE;
    return_value = value;
}
