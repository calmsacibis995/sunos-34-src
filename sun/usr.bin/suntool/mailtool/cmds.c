#ifndef lint
static  char sccsid[] = "@(#)cmds.c 1.4 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Mailtool - tool command handling
 */

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <sunwindow/window_hs.h>
#include <sys/stat.h>
#include <sundev/kbd.h>

#include <suntool/window.h>
#include <suntool/frame.h>
#include <suntool/panel.h>
#include <suntool/text.h>
#include <suntool/walkmenu.h>
#include <suntool/scrollbar.h>

#include "glob.h"
#include "tool.h"

char    *strcpy(), *sprintf();
Textsw_index	textsw_position_for_physical_line();
long	textsw_save();
Textsw	textsw_next(), textsw_first();
Textsw_status	textsw_set();
void	textsw_view_line_info(), textsw_reset();

int	mt_replying;		/* A reply/send/forward is in progress */

struct	msg *mt_delp;		/* pointer to linked list of deleted messages */

Menu	mt_folder_menu;		/* menu of all folders */
Menu	mt_file_menu;		/* menu of most recent file names used */
static time_t fdir_time;	/* time fdir last modified */

void	seln_yield_all();

/*
 * Display the next message.
 */
/* ARGSUSED */
mt_next_proc(item, ie)
	Panel_item item;
	Event *ie;
{
	int nextmsg, scroll_dir, topline, bottomline, endch_pos;

	if (mt_nomail) {
		mt_warn("No Mail");
		return;
	}
	/* mt_save_curmsg(); XXX */
	if (event_shift_is_down(ie)) {
		int savescandir;

		savescandir = mt_scandir;
		mt_scandir = -mt_scandir;	/* reverse scan direction */
		nextmsg = mt_next_msg(mt_curmsg);
		if (mt_scandir == -savescandir)	/* if no change, put it back */
			mt_scandir = savescandir;
	} else
		nextmsg = mt_next_msg(mt_curmsg);

	/* figure out if scrolling is needed */
	if (mt_curmsg < nextmsg)
		scroll_dir = 1;
	else if (mt_curmsg > nextmsg)
		scroll_dir = -1;
	else
		scroll_dir = 0;
	textsw_view_line_info(mt_headersw, &topline, &bottomline);
	if (scroll_dir > 0) {		/* moving down */
		/* bottomline--; */
		endch_pos =
		    textsw_position_for_physical_line(mt_headersw, bottomline);
		if (endch_pos > mt_message[mt_curmsg].m_start) /* not last line */
			scroll_dir = 0;			/* need not scroll */
	} else {
		endch_pos =
		    textsw_position_for_physical_line(mt_headersw, topline);
		if (endch_pos < mt_message[mt_curmsg].m_start)
			scroll_dir = 0;
	}
	mt_update_msgsw(nextmsg, 1, 1, scroll_dir);
}

/*
 * Display message specified by current selection.
 * Selection must be numeric and we display the message
 * with that number.
 */
/* ARGSUSED */
mt_show_proc(item, ie)
	Panel_item item;
	Event *ie;
{
	int msg;

	if (mt_nomail) {
		mt_warn("No Mail");
		return;
	}
	if ((msg = mt_get_curselmsg()) == 0)
		return;
	mt_update_msgsw(msg, (event_shift_is_down(ie)) == 0, 1, 0);
}

/*
 * Compose a new mail message.
 */
/* ARGSUSED */
mt_comp_proc(item, ie)
	Panel_item item;
	Event *ie;
{
	int msg, orig;

	if (mt_replying) {
		mt_warn("Already replying.");
		return;
	}
	msg = 0;
	orig = event_ctrl_is_down(ie);
	if (orig && (msg = mt_get_curselmsg()) == 0)
		return;
	mt_save_curmsg();
	mt_send_msg(msg, mt_replyfile, event_shift_is_down(ie), orig);
	mt_start_reply();
	(void)textsw_load_file(mt_replysw, mt_replyfile, 1, 0, 0);
	(void)textsw_set(mt_replysw, TEXTSW_INSERTION_POINT, 4, 0);
		/* after "To: ", before "\n" */
}

/*
/*
 * Reply to the selected message.
 */
/* ARGSUSED */
mt_reply_proc(item, ie)
	Panel_item item;
	Event *ie;
{
	int msg;

	if (mt_nomail) {
		mt_warn("No Mail");
		return;
	}
	if (mt_replying) {
		mt_warn("Already replying.");
		return;
	}
	if ((msg = mt_get_curselmsg()) == 0)
		return;
	mt_save_curmsg();
	mt_reply_msg(msg, mt_replyfile, event_shift_is_down(ie),
	    event_ctrl_is_down(ie));
	mt_start_reply();
	(void)textsw_load_file(mt_replysw, mt_replyfile, 1, 0, 0);
	(void)textsw_set(mt_replysw, TEXTSW_INSERTION_POINT, TEXTSW_INFINITY, 0);
		/* at end */
}

/*
 * Actually send the current reply-type message.
 */
mt_deliver_proc()
{
	/* current reply file may be different from original reply file */
	char current_filename[1024];

	if (!mt_replying)
		return;
	if (textsw_has_been_modified(mt_replysw)) {
		if (textsw_save(mt_replysw, 0, 0)) {
			mt_warn("Can't save changes.");
			return;
		}
	}
	mt_stop_reply();
	current_filename[0] = '\0';
	(void)textsw_append_file_name(mt_replysw, current_filename);
	if (vfork() == 0) {
		register int i;

		for (i = getdtablesize(); i > 2; i--)
			(void)close(i);
		(void)close(0);
		(void)open(current_filename, 0);
		(void)unlink(mt_replyfile);
		execlp("Mail", "Mail", "-t", 0);
		exit(-1);
	}
}

/*
 * Cancel the current reply (or send, or forward).
 */
/* ARGSUSED */
mt_cancel_proc(item, ie)
	Panel_item item;
	Event *ie;
{

	if (!mt_replying)
		return;
	if ((event_ctrl_is_down(ie)) == 0 && !mt_confirm("Confirm cancel."))
		return;
	(void)unlink(mt_replyfile);
	textsw_reset(mt_replysw, 0, 0);
	mt_stop_reply();
}

/*
 * Make a subwindow for the reply.
 * Destroys any msgsw split views.
 */
mt_start_reply()
{
	int percent, height, focus;
	Textsw	next_split;
	char *p;

	while (next_split = (Textsw) textsw_next((Textsw) textsw_first(mt_msgsw)))
		(void)notify_post_destroy(next_split, DESTROY_CLEANUP, NOTIFY_IMMEDIATE);
	if ((p = mt_value("msgpercent")) != NULL) {
		percent = atoi(p);
		if (percent >= 100)
			percent = 50;
	} else
		percent = 50;
	height = (int)window_get(mt_msgsw, WIN_HEIGHT) * percent / 100;
	(void)window_set(mt_msgsw, WIN_HEIGHT, height, 0);
	(void)window_set(mt_replysw,
	    WIN_BELOW, mt_msgsw,
	    WIN_SHOW, TRUE,
	    WIN_HEIGHT, WIN_EXTEND_TO_EDGE, 0);
	if ((focus = mt_get_kbd_process()) < 0 || focus == getpid())
		(void)window_set(mt_replysw, WIN_KBD_FOCUS, TRUE, 0);
	(void)panel_set(mt_deliver_item, PANEL_SHOW_ITEM, 1, 0);
	(void)panel_set(mt_cancel_item, PANEL_SHOW_ITEM, 1, 0);
	mt_replying++;
}

/*
 * Done doing a reply (or send, or forward).
 * Close the reply subwindow destroying its
 * split views.
 */
mt_stop_reply()
{
	Textsw	next_split;

	while (next_split = (Textsw) textsw_next((Textsw) textsw_first(mt_replysw)))
		(void)notify_post_destroy(next_split, DESTROY_CLEANUP, NOTIFY_IMMEDIATE);
	(void)window_set(mt_replysw, WIN_SHOW, FALSE, 0);
	(void)window_set(mt_msgsw, WIN_HEIGHT, WIN_EXTEND_TO_EDGE, 0);
	(void)panel_set(mt_deliver_item, PANEL_SHOW_ITEM, 0, 0);
	(void)panel_set(mt_cancel_item, PANEL_SHOW_ITEM, 0, 0);

	/*
	 * Reset AGAIN history, so the next invocation of reply will
	 * not remember previous operations.  Must be done twice as
	 * AGAIN may treat the first as an unavoidable checkpoint.
	 */
 
	(void)textsw_checkpoint_again(mt_replysw);
	(void)textsw_checkpoint_again(mt_replysw);
	mt_replying = 0;
}

/*
 * Print the selected message on a printer.
 */
mt_print_proc()
{
	int msg;

	if (mt_nomail) {
		mt_warn("No Mail");
		return;
	}
	if ((msg = mt_get_curselmsg()) == 0)
		return;
	(void)unlink(mt_printfile);
	if (msg == mt_curmsg)
		mt_save_curmsg();
	(void)mt_copy_msg(msg, mt_printfile);
	if (vfork() == 0) {
		register int i;
		register char *p;

		for (i = getdtablesize(); i > 2; i--)
			(void)close(i);
		(void)close(0);
		(void)open(mt_printfile, 0);
		(void)unlink(mt_printfile);
		if ((p = mt_value("printmail")) != NULL)
			execl("/bin/sh", "sh", "-c", p, 0);
		else
			execl("/usr/ucb/lpr", "lpr", "-p", 0);
		exit(-1);
	}
}

/*
 * Preserve the selected message.
 */
mt_pre_proc()
{
	int msg;

	if (mt_nomail) {
		mt_warn("No Mail");
		return;
	}
	if ((msg = mt_get_curselmsg()) == 0)
		return;
	mt_pre_msg(msg);
}

/*
 * Delete the current message and display the next message.
 */
/* ARGSUSED */
mt_del_proc(item, ie)
	Panel_item item;
	Event *ie;
{
	int msg;
	char *p;

	if (mt_nomail) {
		mt_warn("No Mail");
		return;
	}
	if ((msg = mt_get_curselmsg()) == 0)
		return;
	seln_yield_all();
	if (p = mt_value("trash")) {
		if (msg == mt_curmsg)
			mt_save_curmsg();
		(void)mt_copy_msg(msg, p);
	}
	mt_do_del(msg, ie);
}

/*
 * Handle input events when over "delete" panel item.
 * Menu request gives menu of possible options.
 */
mt_del_event(item, ie)
	Panel_item item;
	Event *ie;
{
	Menu_item mi;
	static Menu delp_menu, delnop_menu;

	/*
	 * First time through, build the menus.
	 * XXX - there ought to be a better way
	 * to handle menus that change.
	 */
	if (delp_menu == NULL) {
		delp_menu = menu_create(MENU_CLIENT_DATA, item,
	MENU_NOTIFY_PROC, menu_return_item,
	MENU_STRING_ITEM, "delete, show next", 0,
	MENU_STRING_ITEM, "delete, show prev        [Shift]", SHIFTMASK,
	MENU_STRING_ITEM, "delete             [Ctrl]      ", CTRLMASK,
	MENU_STRING_ITEM, "delete, goto prev  [Ctrl][Shift]", SHIFTMASK|CTRLMASK,
		    0);
		delnop_menu = menu_create(MENU_CLIENT_DATA, item,
	MENU_NOTIFY_PROC, menu_return_item,
	MENU_STRING_ITEM, "delete", 0,
	MENU_STRING_ITEM, "delete, goto prev        [Shift]", SHIFTMASK,
	MENU_STRING_ITEM, "delete, show next  [Ctrl]      ", CTRLMASK,
	MENU_STRING_ITEM, "delete, show prev  [Ctrl][Shift]", SHIFTMASK|CTRLMASK,
		    0);
	}
	if (event_id(ie) == MENU_BUT && event_is_down(ie)) {
		mi = (Menu_item)menu_show(mt_value("autoprint") ?
		    delp_menu : delnop_menu, mt_cmdpanel,
		    panel_window_event(mt_cmdpanel, ie), 0);
		if (mi != NULL) {
			event_set_shiftmask(ie, (int)menu_get(mi, MENU_VALUE));
			mt_del_proc(item, ie);
		}
	} else
		(void)panel_default_handle_event(item, ie);
}

/*
 * Handle input events when over "reply" panel item.
 * Menu request gives menu of possible options.
 * Menu item ordering depends on value of "replyall" mail variable.
 */
mt_reply_event(item, ie)
	Panel_item item;
	Event *ie;
{
	Menu_item mi;
	static Menu replyall_menu, reply_menu;

	/*
	 * First time through, build the menus.
	 */
	if (replyall_menu == NULL) {
		replyall_menu = menu_create(MENU_CLIENT_DATA, item,
	MENU_NOTIFY_PROC, menu_return_item,
	MENU_STRING_ITEM, "Reply (all)", 0,
	MENU_STRING_ITEM, "reply                       [Shift]", SHIFTMASK,
	MENU_STRING_ITEM, "Reply (all), include  [Ctrl]      ", CTRLMASK,
	MENU_STRING_ITEM, "reply, include        [Ctrl][Shift]", SHIFTMASK|CTRLMASK,
		    0);
		reply_menu = menu_create(MENU_CLIENT_DATA, item,
	MENU_NOTIFY_PROC, menu_return_item,
	MENU_STRING_ITEM, "reply", 0,
	MENU_STRING_ITEM, "Reply (all)                 [Shift]", SHIFTMASK,
	MENU_STRING_ITEM, "reply, include        [Ctrl]      ", CTRLMASK,
	MENU_STRING_ITEM, "Reply (all), include  [Ctrl][Shift]", SHIFTMASK|CTRLMASK,
		    0);
	}
	if (event_id(ie) == MENU_BUT && event_is_down(ie)) {
		mi = (Menu_item)menu_show(mt_value("replyall") ?
		    replyall_menu : reply_menu, mt_cmdpanel,
		    panel_window_event(mt_cmdpanel, ie), 0);
		if (mi != NULL) {
			event_set_shiftmask(ie, (int)menu_get(mi, MENU_VALUE));
			mt_reply_proc(item, ie);
		}
	} else
		(void)panel_default_handle_event(item, ie);
}

/*
 * Handle input events when over "save" panel item.
 * Menu request gives menu of possible options.
 */
mt_save_event(item, ie)
	Panel_item item;
	Event *ie;
{
	Menu_item mi;
	static Menu savep_menu, savenop_menu;

	/*
	 * First time through, build the menus.
	 */
	if (savep_menu == NULL) {
		savep_menu = menu_create(MENU_CLIENT_DATA, item,
	MENU_NOTIFY_PROC, menu_return_item,
	MENU_STRING_ITEM, "save, show next", 0,
	MENU_STRING_ITEM, "save, show prev        [Shift]", SHIFTMASK,
	MENU_STRING_ITEM, "save             [Ctrl]      ",	    CTRLMASK,
	MENU_STRING_ITEM, "save, goto prev  [Ctrl][Shift]", SHIFTMASK|CTRLMASK,
		    0);
		savenop_menu = menu_create(MENU_CLIENT_DATA, item,
	MENU_NOTIFY_PROC, menu_return_item,
	MENU_STRING_ITEM, "save", 0,
	MENU_STRING_ITEM, "save, goto prev        [Shift]", SHIFTMASK,
	MENU_STRING_ITEM, "save, show next  [Ctrl]      ", CTRLMASK,
	MENU_STRING_ITEM, "save, show prev  [Ctrl][Shift]", SHIFTMASK|CTRLMASK,
		    0);
	}
	if (event_id(ie) == MENU_BUT && event_is_down(ie)) {
		mi = (Menu_item)menu_show(mt_value("autoprint") ?
		    savep_menu : savenop_menu, mt_cmdpanel,
		    panel_window_event(mt_cmdpanel, ie), 0);
		if (mi != NULL) {
			event_set_shiftmask(ie, (int)menu_get(mi, MENU_VALUE));
			mt_save_proc(item, ie);
		}
	} else
		(void)panel_default_handle_event(item, ie);
}

/*
 * Delete a message.
 * Called by "delete" and "save".
 */
/* ARGSUSED */
mt_do_del(msg, ie)
	int msg;
	Event *ie;
{
	int display_next;	/* display next msg after delete? */
	int nmsg;		/* new message after delete */

	mt_del_header(mt_message[msg].m_start, mt_message[msg+1].m_start);
	mt_del_msg(msg);
	mt_message[msg].m_next = mt_delp;
	mt_delp = &mt_message[msg];
	display_next = event_ctrl_is_down(ie) ?
	    mt_value("autoprint") == NULL : mt_value("autoprint") != NULL;
	if (msg == mt_curmsg)
		textsw_reset(mt_msgsw, 0, 0);
	if (msg == mt_curmsg || display_next) {
		if (event_shift_is_down(ie)) {
			int savescandir;

			savescandir = mt_scandir;
			mt_scandir = -mt_scandir;	/* reverse scan direction */
			nmsg = mt_next_msg(msg);
			if (mt_scandir == -savescandir)
				mt_scandir = savescandir;
		} else
			nmsg = mt_next_msg(msg);
		if (nmsg == 0)
			mt_set_nomail();
		else
			if (display_next && mt_curmsg != nmsg)
				mt_update_msgsw(nmsg, 1, 1, 0);
			else
				mt_set_curselmsg(nmsg);
	} else
		mt_set_curselmsg(mt_next_msg(msg));
}

/*
 * Undelete the most recently deleted message.
 */
mt_undel_proc()
{
	int msg;

	if (mt_delp == NULL) {
		mt_warn("No deleted messages.");
		return;
	}
	msg = mt_delp - mt_message;
	mt_save_curmsg();
	mt_undel_msg(msg);
	mt_delp = mt_delp->m_next;
	mt_ins_header(mt_message[msg].m_start, mt_message[msg].m_header);
	mt_curmsg = msg;
	if (mt_nomail) {
		mt_nomail = 0;
		mt_update_msgsw(mt_curmsg, 1, 0, 0);
	} else if (mt_value("autoprint"))
		mt_update_msgsw(mt_curmsg, 1, 0, 0);
	else
		mt_set_curselmsg(mt_curmsg);
}

/*
 * Quit the tool.
 */
/* ARGSUSED */
mt_quit_proc(item, ie)
	Panel_item item;
	Event *ie;
{

	(void)win_release_event_lock(mt_cmdpanel_fd);
	seln_yield_all();
	if (event_ctrl_is_down(ie))
		mt_assign("expert", "");	/* XXX - force no confirm, just quit */
	(void)tool_done((struct tool *)(LINT_CAST(mt_frame)));
}

/*
 * Abort the tool.
 */
/* ARGSUSED */
mt_abort_proc(item, ie)
	Panel_item item;
	Event *ie;
{

	mt_aborting = 1;
	if (event_ctrl_is_down(ie))
		mt_assign("expert", "");	/* XXX - force no confirm, just quit */
	(void)tool_done((struct tool *)(LINT_CAST(mt_frame)));	/* ?? Should free window */
}

/*
 * Save the selected message in the specified file.
 */
/* ARGSUSED */
mt_save_proc(item, ie)
	Panel_item item;
	Event *ie;
{

	mt_do_save(1, ie);
}

/*
 * Copy the selected message to the specified file.
 */
/* ARGSUSED */
mt_copy_proc(item, ie)
	Panel_item item;
	Event *ie;
{

	mt_do_save(0, ie);
}

/*
 * Do the save or copy.
 */
mt_do_save(del, ie)
	int del;
	Event *ie;
{
	int msg;
	char *file;
	static char buf[256];

	if (mt_nomail) {
		mt_warn("No Mail");
		return;
	}
	if ((msg = mt_get_curselmsg()) == 0)
		return;
	file = (char *)panel_get_value(mt_file_item);
	if (file == NULL || *file == '\0') {
		mt_warn("Must specify file name.");
		return;
	}
	seln_yield_all();
	if (msg == mt_curmsg)
		mt_save_curmsg();
	if (mt_copy_msg(msg, file))
		mt_save_filename(file);	/* only save name if copy succeeds */
	else
		del = 0;		/* don't delete if copy fails */
	(void)strcpy(buf, mt_info);
	if (del)
		mt_do_del(msg, ie);
	mt_update_info(buf);
}

/*
 * Handle input events when over the "File" item.
 * Menu request gives menu of recent file names.
 */
mt_file_event(item, ie)
	Panel_item item;
	Event *ie;
{
	Menu_item mi;

	if (event_id(ie) == MENU_BUT && event_is_down(ie)) {
		if (mt_file_menu == NULL) {
			mt_warn("No saved file names");
		} else {
			mi = menu_show(mt_file_menu, mt_cmdpanel,
				panel_window_event(mt_cmdpanel, ie), 0);
			if (mi) {
				(void)panel_set_value(mt_file_item,
				    menu_get(mi, MENU_STRING));
				(void)menu_set(mt_file_menu, MENU_SELECTED_ITEM, mi, 0);
			}
		}
	} else
		(void)panel_default_handle_event(item, ie);
}

/*
 * Save a file name in the "File:" item menu.
 * The last "filemenusize" file names are saved.
 */
mt_save_filename(file)
	char *file;
{
	Menu_item mi, smi;
	int height;
	register int i, n, cn;
	static int fileuse;

	if (mt_file_menu == NULL)
		mt_file_menu = menu_create(MENU_NOTIFY_PROC, menu_return_item, 0);
	n = fileuse + 1;
	smi = NULL;
	for (i = 1; mi = menu_get(mt_file_menu, MENU_NTH_ITEM, i); i++) {
		if (strcmp(file, (char *)menu_get(mi, MENU_STRING)) == 0) {
			(void)menu_set(mi, MENU_VALUE, ++fileuse, 0);
			return;
		}
		if ((cn = (int)menu_get(mi, MENU_VALUE)) < n) {
			smi = mi;
			n = cn;
		}
	}
	height = i - 1;
	if (mt_value("filemenusize") == NULL)
		i = DEFMAXFILES;
	else
		i = atoi(mt_value("filemenusize"));
	if (height < i) {
		(void)menu_set(mt_file_menu,
		    MENU_STRING_ITEM, mt_savestr(file), ++fileuse, 0);
	} else {
		free((char *)menu_get(smi, MENU_STRING));
		(void)menu_set(smi, MENU_STRING, mt_savestr(file), 0);
		(void)menu_set(smi, MENU_VALUE, ++fileuse, 0);
	}
}

/*
 * Initialize the file menu with the names in the
 * "filemenu" variable.
 */
mt_init_filemenu()
{
	register char *p, *s;

	if (mt_file_menu != NULL)
		return;
	if ((p = mt_value("filemenu")) == NULL)
		return;
	for (;;) {
		while (*p && isspace(*p))
			p++;
		if (*p == '\0')
			break;
		s = p;
		while (*p && !isspace(*p))
			p++;
		if (*p)
			*p++ = '\0';
		mt_save_filename(s);
	}
}

/*
 * Change Mail's working directory.
 */
mt_cd_proc()
{
	char *dir;

	dir = (char *)panel_get_value(mt_dir_item);
	if (dir == NULL)
		dir = "~";
	if (strcmp(dir, mt_wdir) == 0)
		return;
	mt_mail_cmd("echo \"%s\"", dir);
	mt_info[strlen(mt_info) - 1] = '\0';
	if (chdir(mt_info) < 0) {
		extern int errno;
		extern int sys_nerr;
		extern char *sys_errlist[];

		if (errno < sys_nerr)
			(void)sprintf(mt_info, "cd: %s", sys_errlist[errno]);
		else
			(void)sprintf(mt_info, "cd: errno=%d", errno);
		mt_update_info(mt_info);
	} else {
		mt_set_mailwd(mt_info);
		(void)strcpy(mt_wdir, dir);
		mt_update_info("");
	}
}

/*
 * Handle input events when over the "folder" button.
 * Menu request gives menu of folder names.
 */
mt_folder_event(item, ie)
	Panel_item item;
	Event *ie;
{
	Menu_item mi;
	struct stat statb;
	char fdir_name[1024];			/* folder directory name */

	if (event_id(ie) == MENU_BUT && event_is_down(ie)) {
		if (mt_getfold(fdir_name) < 0)
			mt_folder_menu_build(); 
		else if (stat(fdir_name, &statb) >= 0 &&
				statb.st_mtime > fdir_time) {
			fdir_time = statb.st_mtime;
			mt_folder_menu_build();
		}

		mi = menu_show(mt_folder_menu, mt_cmdpanel,
			panel_window_event(mt_cmdpanel, ie), 0);
		if (mi) {
			(void)panel_set_value(mt_file_item, menu_get(mi, MENU_STRING));
			(void)menu_set(mt_folder_menu, MENU_SELECTED_ITEM, mi, 0);
		}
	} else
		(void)panel_default_handle_event(item, ie);

}

/*
 * Determine the current folder directory name.
 */
mt_getfold(name)
	char *name;
{
	char *folder;

	if ((folder = mt_value("folder")) == ((char *) 0))
		return (-1);
	if (*folder == '/')
		(void)strcpy(name, folder);
	else
		(void)sprintf(name, "%s/%s", getenv("HOME"), folder);
	return (0);
}


/*
 * Build the folder menu before displaying it.
 */
mt_folder_menu_build()
{
	register int i;
	int ac;
	char **av;
	char **mt_get_folder_list();

	av = mt_get_folder_list(&ac);
	/*
	 * Select aspect ratio for menu.
	 * This is essentially a table lookup to
	 * avoid taking a sqrt.
	 */
	if (ac <= 3)
		i = 1;
	else if (ac <= 12)
		i = 2;
	else if (ac <= 27)
		i = 3;
	else if (ac <= 48)
		i = 4;
	else if (ac <= 75)
		i = 5;
	else if (ac <= 108)
		i = 6;
	else if (ac <= 147)
		i = 7;
	else
		i = 8;
	av[-1] = (char *)MENU_STRINGS;
	av[ac] = av[ac+1] = 0;

	if (mt_folder_menu)
		menu_destroy(mt_folder_menu);

	/*        	
	 * XXX - this should really be in column-major order.
	 * The menu package will be enhanced to do this in
	 * SunView 2.0.
	 */
	mt_folder_menu = menu_create(ATTR_LIST, &av[-1],
				  MENU_LEFT_MARGIN, 6,
				  MENU_NOTIFY_PROC, menu_return_item,
				  MENU_NCOLS, i,
				  0);
}

/*
 * Switch to the specified folder.
 */
mt_folder_proc()
{
	char *file;

	(void)win_release_event_lock(mt_cmdpanel_fd);
	file = (char *)panel_get_value(mt_file_item);
	if (file == NULL || *file == '\0') {
		mt_warn("Must specify file name.");
		return;
	}
	mt_save_filename(file);
	/* mt_save_curmsg(); XXX */
	mt_new_folder(file);
}

mt_new_folder(file)
	char *file;
{
	int n;

	/* allow other processes to proceed */
	(void)win_release_event_lock(mt_cmdpanel_fd);
	seln_yield_all();
	mt_waitcursor();
	mt_save_curmsg();
	/*
	 * If editing system mailbox, turn on preserve item.
	 */
	if ((n = mt_set_folder(file)) > 0) {
		(void)panel_set(mt_pre_item, PANEL_SHOW_ITEM, strcmp(file, "%") == 0, 0);
		mt_get_folder();
		mt_get_headers();
		mt_load_headers();
		mt_restorecursor();
		mt_set_namestripe(mt_folder, "");
		mt_prevmsg = 0;
		mt_update_msgsw(mt_curmsg, 1, 0, 0);
		mt_nomail = 0;
		mt_scandir = 1;
		mt_delp = NULL;
	} else if (n == 0) {
		(void)panel_set(mt_pre_item, PANEL_SHOW_ITEM, strcmp(file, "%") == 0, 0);
		mt_get_folder();
		mt_update_info(mt_info);
		mt_restorecursor();
		mt_set_namestripe(mt_folder, "[No Mail]");
		mt_set_nomail();
		mt_prevmsg = 0;
		mt_delp = NULL;
	} else {
		/* failed, don't change current folder */
		mt_restorecursor();
		mt_set_namestripe(mt_folder, (strcmp(file, "%") == 0)?"":"[No Mail]");
		mt_update_info(mt_info);
	}
}

mt_set_nomail()
{

	mt_nomail = 1;
	textsw_reset(mt_msgsw, 0, 0);
	(void)unlink(mt_msgfile);
	mt_msg_time = 0;
	textsw_reset(mt_headersw, 0, 0);
}

/*
 * Done with the current folder, close the tool.
 */
mt_done_proc()
{
	char *p;

	mt_save_curmsg();
	(void)window_set(mt_frame, FRAME_CLOSED, TRUE, 0);

	/*
	 * XXX - The following should probably be implemented
	 * by FRAME_CLOSED, but are required now to display
	 * the icon immediately.
	 */
	(void)pw_exposed((struct pixwin *)(LINT_CAST(window_get(mt_frame, WIN_PIXWIN))));
	(void)pw_preparesurface((struct pixwin *)(LINT_CAST(
		window_get(mt_frame, WIN_PIXWIN))), (Rect *)0);
	(void)tool_display((struct tool *)(LINT_CAST(mt_frame)));

	(void)win_release_event_lock(mt_cmdpanel_fd);
	seln_yield_all();
	mt_idle = 1;
	mt_idle_mail();
	(void)strcpy(mt_folder, "[None]");
	mt_set_namestripe(mt_folder, "");
	mt_set_nomail();
	if (p = mt_value("trash"))
		mt_del_folder(p);
}

/*
 * Incorporate any new mail.
 */
mt_inc_proc()
{

	(void)win_release_event_lock(mt_cmdpanel_fd);
	seln_yield_all();
	/* save_curmsg(); XXX */
	mt_new_folder("%");
	mt_check_mail_box();
}

/*
 * Commit the current set of changes.
 */
mt_commit_proc()
{

	if (mt_nomail && mt_delp == NULL) {
		mt_warn("No Mail");
		return;
	}
	(void)win_release_event_lock(mt_cmdpanel_fd);
	seln_yield_all();
	/* mt_save_curmsg(); XXX */
	if (strcmp(mt_folder, mt_mailbox) == 0)
		mt_new_folder("%");
	else
		mt_new_folder(mt_folder);
}

/*
 * Reread .mailrc to pick up any changes.
 */
mt_mailrc_proc()
{

	mt_mail_cmd("source %s", mt_value("MAILRC"));
	mt_get_vars();
	mt_start_timer();
	mt_init_filemenu();
	fdir_time = 0;
}

/*
 * Handle input events when over panel items.
 * Menu request gives menu of possible options.
 */
mt_cmdpanel_event(item, ie)
	Panel_item item;
	Event *ie;
{
	Menu_item mi;
	typedef int (*func)();
	func proc;

	if (event_id(ie) == MENU_BUT && event_is_down(ie)) {
		mi = (Menu_item)menu_show(
		    (Menu)panel_get(item, PANEL_CLIENT_DATA), mt_cmdpanel,
			panel_window_event(mt_cmdpanel, ie), 0);
		if (mi != NULL) {
			event_set_shiftmask(ie, (int)menu_get(mi, MENU_VALUE));
			proc = (func)(LINT_CAST(panel_get(item, PANEL_NOTIFY_PROC)));
			(*proc)(item, ie);
		}
	} else
		(void)panel_default_handle_event(item, ie);
}

/*
 * Get the process id of the process holding the keyboard focus.
 */
mt_get_kbd_process()
{
	int fd, num;
	char name[WIN_NAMESIZE];

	if ((fd = (int)window_get(mt_msgsw, WIN_FD, 0)) < 0)
		return (fd);
	if ((num = win_get_kbd_focus(fd)) == WIN_NULLLINK)
		return (-1);
	(void)win_numbertoname(num, name);
	if ((fd = open(name)) < 0)
		return (fd);
	num = win_getowner(fd);
	(void)close(fd);
	return (num);
}
