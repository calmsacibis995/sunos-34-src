#ifndef lint
static  char sccsid[] = "@(#)tool.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Mailtool - tool handling
 */

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <sunwindow/window_hs.h>

#include <sys/stat.h>
#include <sys/wait.h>

#include <suntool/window.h>
#include <suntool/frame.h>
#include <suntool/panel.h>
#include <suntool/text.h>

#include <suntool/scrollbar.h>
#include <suntool/selection.h>
#include <suntool/selection_svc.h>
#include <suntool/selection_attributes.h>
#include <suntool/menu.h>
#include <suntool/walkmenu.h>
#include <suntool/icon.h>

#include "glob.h"
#include "tool.h"

#define	TOOL_LINES	(mt_headerlines + mt_cmdlines + mt_maillines)
#define	TOOL_COLS	80

Frame	mt_frame;
Textsw	mt_headersw;		/* the header subwindow */
Panel	mt_cmdpanel;		/* the command panel */
int	mt_cmdpanel_fd;		/* the command panel subwindow fd */
Textsw	mt_msgsw;			/* the message subwindow */
Textsw	mt_replysw;		/* the mail reply subwindow */
struct	pixfont *mt_font;		/* the default font */

int	mt_headerlines;		/* lines of headers */
int	mt_cmdlines;		/* command panel height */
int	mt_maillines;		/* lines for mail item */

int	mt_prevmsg;
int	mt_idle;
int	mt_nomail;
char	*mt_wdir;			/* Mail's working directory */
time_t	mt_msg_time;		/* time msgfile was last written */

Panel_item	mt_deliver_item, mt_cancel_item;
Panel_item	mt_file_item, mt_info_item, mt_dir_item;
Panel_item	mt_pre_item;

int	mt_next_proc(), mt_show_proc(), mt_del_proc(), mt_undel_proc(), mt_print_proc();
int	mt_inc_proc(), mt_done_proc(), mt_commit_proc();
int	mt_reply_proc(), mt_comp_proc(), mt_deliver_proc(), mt_cancel_proc();
int	mt_save_proc(), mt_folder_proc(), mt_cd_proc();
int	mt_pre_proc(), mt_copy_proc(), mt_mailrc_proc(), mt_quit_proc(), mt_abort_proc();
int	mt_cmdpanel_event(), mt_folder_event(), mt_file_event();
int	mt_del_event(), mt_reply_event(), mt_save_event();

#ifdef NOTDEF	/*	I think these are vestigial.  jf	*/
int	mail_selected(), mail_sigwinch();
#endif

static int	charheight, charwidth;	/* size of default font */

static Notify_value	mail_itimer(), mail_change_size(), mail_destroy();
static Notify_value	mail_open_itimer();	/* used to schedule opening activity */

static Panel_item	panel_create_button();

/*
 * Icons
 */
#define	static			/* XXX - gross kludge */
short	mt_mail_image[256] = {
#include <images/mail.icon>
};
DEFINE_ICON_FROM_IMAGE(mt_mail_icon, mt_mail_image);

short	mt_nomail_image[256] = {
#include <images/nomail.icon>
};
DEFINE_ICON_FROM_IMAGE(mt_nomail_icon, mt_nomail_image);

short	mt_unknown_image[256] = {
#include <images/dead.icon>
};
DEFINE_ICON_FROM_IMAGE(mt_unknown_icon, mt_unknown_image);
#undef static

struct	icon *mt_icon_ptr;

/*
 * Cursors
 */
static short confirm_data[16] = {
	0x1FF8, 0x3FFC, 0x336C, 0x336C, 0x336C, 0x336C, 0x336C, 0x336C,
	0x3FFC, 0x3FFC, 0x3FFC, 0x3FFC, 0x3FFC, 0x3FC4, 0x3FFC, 0x1FF8
};
mpr_static(mt_confirm_pr, 16, 16, 1, confirm_data);
static struct cursor mailtool_confirm_cur = {4, 4, PIX_SRC | PIX_DST, &mt_confirm_pr};

static short    wait_image[] = {
#include <images/hglass.cursor>
};
DEFINE_CURSOR_FROM_IMAGE(hourglasscursor, 7, 7, PIX_SRC | PIX_DST, wait_image);

#ifdef notdef
short	hourglass_cursorimage[16] = {
	0x7FFE,0x4002,0x200C,0x1A38,0x0FF0,0x07E0,0x03C0,0x0180,
	0x0180,0x0240,0x0520,0x0810,0x1108,0x23C4,0x47E2,0x7FFE
};
mpr_static(hourglass_mpr, 16, 16, 1, hourglass_cursorimage);
struct	cursor hourglass_cursor = { 8, 8, PIX_SRC|PIX_DST, &hourglass_mpr};
#endif

#define	TEXTSW_NO_EDIT	TEXTSW_BROWSING

#define NUMWINDOWS 5
static	Cursor	cursor[NUMWINDOWS];
static  Window	window[NUMWINDOWS];

mt_init_tool_storage()
{
	/* Dynamically allocate mt_wdir */
	mt_wdir = (char *)calloc(1024, sizeof (char));
	/* Initialize data */
	mt_idle = 1;
	mt_nomail = 1;
	mt_icon_ptr = &mt_unknown_icon;
}

/*
 * Build and start the tool.
 */
mt_start_tool(tool_attrs)
	char **tool_attrs;
{
	int i;
	char *p;
	Panel_item item;
	int sigwinched();
	int ondeath();
	int mt_terminate();

	/*
	 * Open the default font and save the
	 * size of characters.
	 */
	if ((mt_font = pw_pfsysopen()) == NULL) {
		fprintf(stderr, "%s: can't open default font\n", mt_cmdname);
		exit(1);
	}
	charwidth = mt_font->pf_defaultsize.x;
	charheight = mt_font->pf_defaultsize.y;

	/*
	 * Get the sizes of the various subwindows.
	 */
	mt_headerlines = 10;
	mt_cmdlines = 4;
	mt_maillines = 30;
	if ((p = mt_value("headerlines")) && (i = atoi(p)) > 0)
		mt_headerlines = i;
	if ((p = mt_value("cmdlines")) && (i = atoi(p)) > 0)
		mt_cmdlines = i;
	if ((p = mt_value("maillines")) && (i = atoi(p)) > 0)
		mt_maillines = i;

	/*
	 * If "autoprint" is set, turn it off.  We do it ourselves.
	 */
	if (mt_value("autoprint"))
		mt_set_var("autoprint", (char *)NULL);

	/*
	 * Get the current directory.
	 */
	mt_get_mailwd(mt_wdir);
	chdir(mt_wdir);

	/*
	 * Initialize the file names menu.
	 */
	mt_init_filemenu();

	/*
	 * Create the tool.
	 */
	mt_frame = window_create(0, FRAME,
	    WIN_WIDTH, ATTR_COLS(TOOL_COLS) + 10 +	/* XXX */
		(int)textsw_get(TEXTSW, TEXTSW_LEFT_MARGIN) + 2 +
		(int)scrollbar_get(SCROLLBAR, SCROLL_THICKNESS),
	    WIN_HEIGHT, (TOOL_LINES + 1) * charheight,
	    WIN_ERROR_MSG, "mailtool: unable to create window\n",
	    FRAME_LABEL, mt_cmdname,
	    FRAME_SUBWINDOWS_ADJUSTABLE, TRUE,
	    FRAME_ICON, mt_icon_ptr,
	    FRAME_CLOSED, TRUE,
	    FRAME_SHOW_LABEL, TRUE,
	    0);
	window_set(mt_frame, ATTR_LIST, tool_attrs, 0);
	tool_free_attribute_list(tool_attrs);

	/* catch going from icon to window */
	notify_interpose_event_func(mt_frame, mail_change_size, NOTIFY_SAFE);

	/* catch when being destroyed */
	notify_interpose_destroy_func(mt_frame, mail_destroy);

	/*
	 * Create the header textsubwindow.
	 */
	mt_headersw = window_create(mt_frame, TEXT,
	    WIN_HEIGHT, mt_headerlines * charheight,
	    WIN_ERROR_MSG, "mailtool: Unable to create header window\n",
	    TEXTSW_CONFIRM_OVERWRITE, 0,
	    TEXTSW_AUTO_INDENT, 0,
	    TEXTSW_DISABLE_LOAD, 1,
	    TEXTSW_NO_EDIT, 1,
	    0);
	textsw_set(mt_headersw,
	    TEXTSW_LINE_BREAK_ACTION, TEXTSW_CLIP,
	    0);
	/*
	 * Create the command panel.
	 */
	mt_cmdpanel = window_create(mt_frame, PANEL,
	    WIN_ROW_GAP, 7,
	    WIN_HEIGHT, ATTR_LINES(mt_cmdlines) + 9, /* XXX */
	    WIN_VERTICAL_SCROLLBAR, scrollbar_create(0),
	    WIN_ERROR_MSG, "mailtool: unable to create panel\n",
	    0);
	mt_cmdpanel_fd = window_fd(mt_cmdpanel);

	/* first line */
	(void) panel_create_button(mt_cmdpanel, "show", 0, 0, mt_show_proc,
		"show", 
		"show all headers  [Shift]", NULL, NULL);
	(void) panel_create_button(mt_cmdpanel, "next", 0, 10, mt_next_proc,
		"next", 
		"prev  [Shift]", NULL, NULL);
	item = panel_create_button(mt_cmdpanel, "delete", 0, 20, mt_del_proc,
	    NULL, NULL, NULL, NULL);
	/* delete menu is special, need a custom event proc */
	panel_set(item, PANEL_EVENT_PROC, mt_del_event, 0);
	(void) panel_create_button(mt_cmdpanel, "undelete", 0, 30, mt_undel_proc,
	    "undelete", NULL, NULL, NULL);
	(void) panel_create_button(mt_cmdpanel, "print", 0, 40, mt_print_proc,
	    "print", NULL, NULL, NULL);
	(void) panel_create_button(mt_cmdpanel, "new mail", 0, 50, mt_inc_proc,
	    "new mail", NULL, NULL, NULL);
	(void) panel_create_button(mt_cmdpanel, "done", 0, 60, mt_done_proc,
	    "done", NULL, NULL, NULL);
	/* second line */
	item = panel_create_button(mt_cmdpanel, "reply", 1, 0, mt_reply_proc,
	    NULL, NULL, NULL, NULL);
	/* reply menu is special, need a custom event proc */
	panel_set(item, PANEL_EVENT_PROC, mt_reply_event, 0);
	(void) panel_create_button(mt_cmdpanel, "compose", 1, 10, mt_comp_proc,
		"compose", 
		"compose with fields        [Shift]", 
		"forward              [Ctrl]      ", 
		"forward with fields  [Ctrl][Shift]");
	mt_deliver_item = panel_create_button(mt_cmdpanel, "deliver", 1, 30, mt_deliver_proc,
	    "deliver", NULL, NULL, NULL);
	panel_set(mt_deliver_item, PANEL_SHOW_ITEM, 0, 0);
	mt_cancel_item = panel_create_button(mt_cmdpanel, "cancel", 1, 40, mt_cancel_proc,
		"cancel", "", 
		"cancel, no confirm  [Ctrl]", NULL);
	panel_set(mt_cancel_item, PANEL_SHOW_ITEM, 0, 0);
	(void) panel_create_button(mt_cmdpanel, "commit", 1, 60, mt_commit_proc,
	    "commit", NULL, NULL, NULL);
	/* third line */
	item = panel_create_button(mt_cmdpanel, "save", 2, 0, mt_save_proc,
	    NULL, NULL, NULL, NULL);
	/* save menu is special, need a custom event proc */
	panel_set(item, PANEL_EVENT_PROC, mt_save_event, 0);
	(void) panel_create_button(mt_cmdpanel, "copy", 2, 10, mt_copy_proc,
	    "copy", NULL, NULL, NULL);
	mt_file_item = panel_create_item(mt_cmdpanel, PANEL_TEXT,
	    PANEL_LABEL_STRING, "File: ",
	    PANEL_LABEL_Y, ATTR_ROW(2) + 7,
	    PANEL_LABEL_X, ATTR_COL(20) + 5,
	    PANEL_VALUE_STORED_LENGTH, 1024,
	    PANEL_VALUE_DISPLAY_LENGTH, 60,
	    0);
	panel_set(mt_file_item, PANEL_EVENT_PROC, mt_file_event, 0);
	/* fourth line */
	item = panel_create_button(mt_cmdpanel, "folder", 3, 0, mt_folder_proc,
	    NULL, NULL, NULL, NULL);
	panel_set(item, PANEL_EVENT_PROC, mt_folder_event, 0);
	mt_info_item = panel_create_item(mt_cmdpanel, PANEL_MESSAGE,
	    PANEL_LABEL_STRING, "",
	    PANEL_LABEL_Y, ATTR_ROW(3) + 7,
	    PANEL_LABEL_X, ATTR_COL(27) + 5,
	    0);
	/* fifth line */
	mt_pre_item = panel_create_button(mt_cmdpanel, "preserve", 4, 0, mt_pre_proc,
	    "preserve", NULL, NULL, NULL);
	(void) panel_create_button(mt_cmdpanel, "cd", 4, 10, mt_cd_proc,
	    "change directory", NULL, NULL, NULL);
	mt_dir_item = panel_create_item(mt_cmdpanel, PANEL_TEXT,
	    PANEL_LABEL_STRING, "Directory: ",
	    PANEL_LABEL_Y, ATTR_ROW(4) + 7,
	    PANEL_LABEL_X, ATTR_COL(20) + 5,
	    PANEL_VALUE, mt_wdir,
	    PANEL_VALUE_STORED_LENGTH, 1024,
	    PANEL_VALUE_DISPLAY_LENGTH, 50,
	    0);
	/* sixth line */
	(void) panel_create_button(mt_cmdpanel, ".mailrc", 5, 0, mt_mailrc_proc,
	    "source .mailrc", NULL, NULL, NULL);
	(void) panel_create_button(mt_cmdpanel, "quit", 5, 50, mt_quit_proc,
		"quit", "", 
		"quit, no confirm  [Ctrl]", NULL);
	(void) panel_create_button(mt_cmdpanel, "abort", 5, 60, mt_abort_proc,
		"abort", "", 
		"abort, no confirm  [Ctrl]", NULL);

	/*
	 * Create the mail message textsubwindow.
	 */
	mt_msgsw = window_create(mt_frame, TEXT,
	    WIN_ERROR_MSG, "mailtool: unable to create message window\n",
	    TEXTSW_CONFIRM_OVERWRITE, 0,
	    TEXTSW_STORE_SELF_IS_SAVE, 1,
	    TEXTSW_DISABLE_LOAD, 1,
	    0);

	/*
	 * Create the mail reply textsubwindow.
	 */
	mt_replysw = window_create(mt_frame, TEXT,
	    WIN_SHOW, FALSE,
	    WIN_ERROR_MSG, "mailtool: unable to create reply window\n",
	    TEXTSW_CONFIRM_OVERWRITE, 0,
	    TEXTSW_STORE_SELF_IS_SAVE, 1,
	    TEXTSW_DISABLE_LOAD, 1,
	    0);

	window[0] = mt_frame;
	window[1] = mt_headersw;
	window[2] = mt_cmdpanel;
	window[3] = mt_msgsw;
	window[4] = mt_replysw;

	/*
	 * Catch signals, install tool.
	 */
	notify_set_signal_func(&mt_mailclient, mt_terminate, SIGTERM, NOTIFY_ASYNC);
	notify_set_signal_func(&mt_mailclient, mt_terminate, SIGXCPU, NOTIFY_ASYNC);
	/*
	 * Kludge - we want the notifier to harvest all dead children
	 * so we tell it to wait for its own death.
	 */
	notify_set_wait3_func(&mt_mailclient, notify_default_wait3, getpid());

	/* start up with correct icon */
	mt_check_mail_box();

	/* set timer intervals */
	mt_start_timer();
	
	window_main_loop(mt_frame);
	
	mt_stop_mail(mt_aborting);
}

/*
 * SIGTERM and SIGXCPU handler.
 * Save mailbox and exit.
 */
mt_terminate(client, sig)
	Notify_client client;
	int sig;
{

	if (!mt_idle)
		mt_stop_mail(0);
	if (sig == SIGXCPU) {
		/* for debugging */
		sigsetmask(0);
		abort();
	}
	mt_done(0);
}

/*
 * Start the timer to look for new mail.
 */
mt_start_timer()
{
	static struct itimerval itv;
	char *p;
	int interval = 0;

	if (p = mt_value("interval"))
		interval = atoi(p);
	if (interval == 0)
		interval = 5*60;	/* 5 minutes */
	itv.it_interval.tv_sec = interval;
	itv.it_value.tv_sec = interval;
	notify_set_itimer_func(&mt_mailclient, mail_itimer, ITIMER_REAL, &itv, 0);
}

/*
 * Check for new mail if timer expired.
 */
/* ARGSUSED */
static Notify_value
mail_itimer(client, which)
	Notify_client client;
	int which;
{

	mt_check_mail_box();
	return (NOTIFY_DONE);
}

/* ARGSUSED */
static Notify_value
mail_change_size(client, event, arg, when)
	Notify_client client;
	Event *event;
	Notify_arg arg;
	Notify_event_type when;
{
	Notify_value rc;

	rc = notify_next_event_func(client, event, arg, NOTIFY_SAFE);
	if ((event_id(event) == WIN_REPAINT) && mt_idle) {
		/*
		 * If window is now open and we are idle
		 * then read new mail.
		 */
		if (!window_get(client, FRAME_CLOSED)) {
			static struct itimerval itv = { {0, 0}, {0, 1} };

			mt_idle = 0;
			/*
			 * Set up timer to go off once and immediately.
			 * This schedules the mail activity to be done upon
			 * opening the tool, to occur after the tool is open
			 * completely.
			 */
			notify_set_itimer_func(&mt_mailclient, mail_open_itimer,
			    ITIMER_VIRTUAL, &itv, 0);
		}
	}
	return (rc);
}

/* ARGSUSED */
static Notify_value
mail_open_itimer(client, which)
	Notify_client client;
	int which;
{

	/*
	 * Read the mail headers into the header subwindow.
	 * Load the current message into the message subwindow.
	 */
	if (!window_get(client, FRAME_CLOSED)) {
		mt_set_namestripe(mt_folder, "Reading new mail...");
		mt_new_folder("%");
		mt_check_mail_box();
	}
	return (NOTIFY_DONE);
}

static Notify_value
mail_destroy(tool, status)
	Frame tool;
	Destroy_status status;
{
	Notify_value val;

	if (status == DESTROY_CHECKING) {
		if (mt_aborting) {
			if (!mt_confirm("ABORTING!!!  ARE YOU SURE?")) {
				tool_veto_destroy(tool);
				mt_aborting = 0;
			} else {
				textsw_reset(mt_headersw, 0, 0);
				textsw_reset(mt_msgsw, 0, 0);
				textsw_reset(mt_replysw, 0, 0);
				tool_confirm_destroy(tool);
				val = notify_next_destroy_func(tool, status);
			}
		} else {
			if (!mt_confirm("Please confirm quit.")) {
				tool_veto_destroy(tool);
			} else {
				mt_save_curmsg();
				/* XXX temp fix to avoid veto by textsw */
				textsw_reset(mt_headersw, 0, 0);
				textsw_reset(mt_msgsw, 0, 0);
				textsw_reset(mt_replysw, 0, 0);
				tool_confirm_destroy(tool);
				val = notify_next_destroy_func(tool, status);
			}
		}
	} else {
		val = notify_next_destroy_func(tool, status);
		notify_remove(&mt_mailclient);
	}
	return (val);
}

/*
 * Load the headers into the header subwindow.
 */
mt_load_headers()
{
	FILE *f;
	register int i, height, start, pos = 0;
	register struct msg *mp;

	f = fopen(mt_hdrfile, "w");
	chmod(mt_hdrfile, 0600);
	for (i = 1, mp = &mt_message[1]; i <= mt_maxmsg; i++, mp++) {
		mp->m_start = pos;
		fputs(mp->m_header, f);
		pos += strlen(mp->m_header);
	}
	mt_message[mt_maxmsg+1].m_start = pos;
	fclose(f);
	/* make the current message be the top line in the window */
	/* fill out the window from the top if too few remaining messages */
	height = textsw_screen_line_count(mt_headersw);
	if (mt_maxmsg <= height)
		start = 1;
	else if (mt_maxmsg - height >= mt_curmsg)
		start = mt_curmsg;
	else
		start = mt_maxmsg - height + 1;
	textsw_set(mt_headersw,
	   TEXTSW_FILE, mt_hdrfile,
	   TEXTSW_FIRST_LINE, start-1,
	   0);
	unlink(mt_hdrfile);
}

/*
 * Move the mark in the header subwindow to point
 * to the new current message.  Also, make the message
 * number part of the header in the header subwindow
 * be the current primary selection.
 */
mt_update_curmsg(scroll)
	int scroll;		/* - up, + down, 0 none */
{
	register int spos;
	int move;

	mt_update_info("");	/* if current message changes, remove info */
	textsw_set(mt_headersw, TEXTSW_NO_EDIT, 0, 0);
	if (mt_prevmsg > 0
	    && mt_prevmsg <= mt_maxmsg
	    && !mt_message[mt_prevmsg].m_deleted) {
		spos = mt_message[mt_prevmsg].m_start;
		textsw_erase(mt_headersw, spos, spos+2);
		textsw_set(mt_headersw, TEXTSW_INSERTION_POINT, spos, 0);
		textsw_insert(mt_headersw, "  ", 2);
	}
	spos = mt_message[mt_curmsg].m_start;
	textsw_erase(mt_headersw, spos, spos+2);
	textsw_set(mt_headersw, TEXTSW_INSERTION_POINT, spos, 0);
	textsw_insert(mt_headersw, "> ", 2);
	textsw_set(mt_headersw, TEXTSW_NO_EDIT, 1, 0);
	mt_set_curselmsg(mt_curmsg);
	if (scroll) {  /* XXX - no reason to believe this will work! 
			* the message could be more than half a window
			* outside the current view...
			*/
		move = (int)window_get(mt_headersw, WIN_ROWS) / 2;
		textsw_scroll_lines(mt_headersw, scroll * move);
		textsw_set(mt_headersw, TEXTSW_UPDATE_SCROLLBAR, 0);
	}
	mt_prevmsg = mt_curmsg;
}

/*
 * If the message displayed in the message subwindow
 * has been modified, write it back out to the file
 * and tell mail to reload the message from the file.
 * If the user stores the message into a different
 * file then the filename for the textsw will change
 * so we have to check for that too.
 */
mt_save_curmsg()
{
	struct stat statb;
	char curmsgfile[1024];

	curmsgfile[0] = '\0';
	if (textsw_append_file_name(mt_msgsw, curmsgfile))
		return;
	if (strcmp(curmsgfile, mt_msgfile) != 0) {
		if (!mt_confirm(
		   "Message filename changed, reload message from new file?")) {
			unlink(mt_msgfile);
			mt_msg_time = 0;
			return;
		}
	}
	if (textsw_has_been_modified(mt_msgsw)) {
		if (!mt_confirm("Message has been modified, confirm changes.")) {
			unlink(mt_msgfile);
			mt_msg_time = 0;
			return;
		}
		if (textsw_save(mt_msgsw, 0, 0)) {
			mt_warn("Can't save changes.");
			unlink(mt_msgfile);
			mt_msg_time = 0;
			return;
		}
		mt_load_msg(mt_curmsg, curmsgfile);
	} else if (stat(curmsgfile, &statb) >= 0 && statb.st_mtime > mt_msg_time) {
		mt_msg_time = statb.st_mtime;
		mt_load_msg(mt_curmsg, curmsgfile);
	}
}

/*
 * Load the message into the message subwindow.
 */
mt_update_msgsw(msg, ign, ask_save, scroll)
	int msg, ign, ask_save;
{
	struct stat statb;

	if (ask_save)
		mt_save_curmsg();
	if (msg == 0)
		return;
	unlink(mt_msgfile);	/* just in case there's one left over */
	mt_print_msg(msg, mt_msgfile, ign);
	stat(mt_msgfile, &statb);
	mt_msg_time = statb.st_mtime;
	mt_curmsg = msg;
	mt_update_curmsg(scroll);
	textsw_set(mt_msgsw, TEXTSW_DISABLE_LOAD, 0, 0);
	textsw_load_file(mt_msgsw, mt_msgfile, 1, 0, 0);
	textsw_set(mt_msgsw, TEXTSW_DISABLE_LOAD, 1, 0);
}

mt_update_info(s)
	char *s;
{
	char *p;

	if (p = index(s, '\n'))
		*p = '\0';
	panel_set(mt_info_item, PANEL_LABEL_STRING, s, 0);
}

mt_set_curselmsg(msg)
	int msg;
{
	int spos, epos;
	extern int mt_last_sel_msg;

	epos = mt_message[msg].m_start + 5;
	if (msg < 10)
		spos = epos - 1;
	else if (msg < 100)
		spos = epos - 2;
	else
		spos = epos - 3;
	textsw_set_selection(mt_headersw, spos, epos, SEL_PRIMARY);
	mt_last_sel_msg = msg;
}

/*
 * Delete characters in the header subwindow.
 */
mt_del_header(from, to)
	int from, to;
{

	textsw_set(mt_headersw, TEXTSW_NO_EDIT, 0, 0);
	textsw_erase(mt_headersw, from, to);
	textsw_set(mt_headersw, TEXTSW_NO_EDIT, 1, 0);
}

/*
 * Insert characters in the header subwindow.
 */
mt_ins_header(start, s)
	int start;
	char *s;
{

	textsw_set(mt_headersw, TEXTSW_NO_EDIT, 0, 0);
	textsw_set(mt_headersw, TEXTSW_INSERTION_POINT, start, 0);
	textsw_insert(mt_headersw, s, strlen(s));
	textsw_set(mt_headersw, TEXTSW_NO_EDIT, 1, 0);
}

/*
 * Set the namestripe.
 */
mt_set_namestripe(f, n)
	char *f, *n;
{
	static char namestripe[256];

	sprintf(namestripe, "%s - folder: %s %s", mt_cmdname, f, n);
	window_set(mt_frame, FRAME_LABEL, namestripe, 0);
}

/*
 * Set the icon.
 */
mt_set_icon(ic)
	struct icon *ic;
{
	window_set(mt_frame, FRAME_ICON, ic, 0);
}

/*
 * Announce the arrival of new mail.
 */
mt_announce_mail()
{
	static struct timeval tv = {0, 300000};
	char *p;
	int bells = 0, flashes = 0;
	int fd;
	Pixwin *pw;

	if (p = mt_value("bell")) {
		bells = atoi(p);
		if (bells <= 0)
			bells = 1;
	}
	if (p = mt_value("flash")) {
		flashes = atoi(p);
		if (flashes <= 0)
			flashes = 1;
	}
	fd = (int)window_get(mt_frame, WIN_FD);
	pw = (Pixwin *)window_get(mt_frame, WIN_PIXWIN);
	/*
	 * XXX - win_bell won't do flashes without bells.
	 * Should be:
	 *	if (bells > 0 || flashes > 0)
	 */
	if (bells > 0)
		win_bell(bells-- > 0 ? fd : -1, tv, flashes-- > 0 ? pw : 0);
	while (bells > 0) {
		select(0, 0, 0, 0, &tv);	/* pause between bells */
		win_bell(bells-- > 0 ? fd : -1, tv, flashes-- > 0 ? pw : 0);
	}
}

/*
 * Create a button in a panel, and a menu behind it.
 * Strings m1 - m4 specify the menu items
 * that correspond to the normal, shifted,
 * ctrled, and ctrl-shifted versions of the
 * button.
 */
static Panel_item
panel_create_button(panel, label, row, col, proc, m1, m2, m3, m4)
	Panel panel;
	char *label;
	int row, col;
	int (*proc)();
	char *m1, *m2, *m3, *m4;
{
	Panel_item item;
	Menu menu;
	register char **p;
	register int i;
	static int menuval[4] = { 0, SHIFTMASK, CTRLMASK, SHIFTMASK|CTRLMASK };

	item = panel_create_item(panel, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE, panel_button_image(panel, label, 8, NULL),
	    PANEL_LABEL_X, ATTR_COL(col) + 5,
	    PANEL_LABEL_Y, ATTR_ROW(row) + 5,
	    PANEL_NOTIFY_PROC, proc,
	    0);
	if (m1 != NULL) {
		menu = menu_create(MENU_CLIENT_DATA, item,
		    MENU_NOTIFY_PROC, menu_return_item, 
		    MENU_LEFT_MARGIN, 10, 
		    0);
		for (p = &m1, i = 0; i < 4 && *p != NULL; p++, i++) {
			if (**p == '\0')	/* empty strings are skipped */
				continue;
			menu_set(menu, MENU_STRING_ITEM, *p, menuval[i], 0);
		}
		panel_set(item,
		    PANEL_EVENT_PROC, mt_cmdpanel_event,
		    PANEL_CLIENT_DATA, menu,
		    0);
	}
	return (item);
}

/*
 * Request confirmation from the user.
 */
mt_confirm(s)
	char *s;
{
	struct prompt p;
	struct inputevent ie;
	Rect *r, *rc;
	Cursor original_cur;
	int x, y, s_width, s_height, fd;

	if (mt_value("expert"))
		return(1);
	
	fd = window_fd(mt_frame);
	s_width = (strlen(s) + 4) * charwidth;
	s_height = 2 * charheight;
	if (!window_get(mt_frame, FRAME_CLOSED)) {	/* center within mailtool */
		r = (Rect *)window_get(mt_frame, WIN_RECT);
		x = (r->r_width / 2) - (s_width / 2);
		y = (r->r_height / 2) - (s_height / 2);
	} else	 {				/* center on the display */
		r = (Rect *)window_get(mt_frame, WIN_SCREEN_RECT);
		rc = (Rect *)window_get(mt_frame, FRAME_CLOSED_RECT);
		x = (r->r_width / 2) - (s_width / 2) - (rc->r_left);
		y = (r->r_height / 2) - (s_height / 2) - (rc->r_top);
	}
	rect_construct(&p.prt_rect, x, y, s_width, s_height);
	p.prt_font = mt_font;
	p.prt_text = s;
	original_cur = window_get(mt_frame, WIN_CURSOR);
	window_set(mt_frame, WIN_CURSOR, &mailtool_confirm_cur, 0);
	menu_prompt(&p, &ie, fd);
	window_set(mt_frame, WIN_CURSOR, original_cur, 0);
	return (ie.ie_code == SELECT_BUT && win_inputposevent(&ie));
}

/*
 * Warn the user about an error.
 */
mt_warn(s)
	char *s;
{
	struct prompt p;
	struct inputevent ie;
	Rect *r, *rc;
	Cursor original_cur;
	int x, y, s_width, s_height, fd;

	fd = window_fd(mt_frame);
	s_width = (strlen(s) + 4) * charwidth;
	s_height = 2 * charheight;
	if (!window_get(mt_frame, FRAME_CLOSED)) {	/* center within mailtool */
		r = (Rect *)window_get(mt_frame, WIN_RECT);
		x = (r->r_width / 2) - (s_width / 2);
		y = (r->r_height / 2) - (s_height / 2);
	} else	 {				/* center on the display */
		r = (Rect *)window_get(mt_frame, WIN_SCREEN_RECT);
		rc = (Rect *)window_get(mt_frame, FRAME_CLOSED_RECT);
		x = (r->r_width / 2) - (s_width / 2) - (rc->r_left);
		y = (r->r_height / 2) - (s_height / 2) - (rc->r_top);
	}
	rect_construct(&p.prt_rect, x, y, s_width, s_height);
	p.prt_font = mt_font;
	p.prt_text = s;
	original_cur = window_get(mt_frame, WIN_CURSOR);
	window_set(mt_frame, WIN_CURSOR, &mailtool_confirm_cur, 0);
	menu_prompt(&p, &ie, fd);
	window_set(mt_frame, WIN_CURSOR, original_cur, 0);
}

/* 
 * Change cursor to hourglass, and change namestripe
 */
mt_waitcursor()
{
	int	fd, i;
	static	first;
	
	if (first == 0) {
		first = 1;
		for (i = 0; i < NUMWINDOWS; i++)
			cursor[i] = cursor_create((char *)0);
	}
	for (i = 0; i < NUMWINDOWS; i++) {
		fd = (int) window_get(window[i], WIN_FD);
		win_getcursor(fd, cursor[i]);
		win_setcursor(fd, &hourglasscursor);
	}
	window_set(mt_frame, FRAME_LABEL, "loading messages...", 0);
}

/* 
 * Restore cursor, but don't change namestripe
 */
mt_restorecursor()
{
	int	i, fd;
	
	for (i = 0; i < NUMWINDOWS; i++) {
		fd = (int) window_get(window[i], WIN_FD);
		win_setcursor(fd, cursor[i]);
	}
}
