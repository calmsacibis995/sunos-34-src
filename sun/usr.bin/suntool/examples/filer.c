/***************************************************************************/
#ifndef lint
static char sccsid[] = "@(#)filer.c 1.1 86/09/25 Copyr 1986 Sun Micro";
#endif
/***************************************************************************/

#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/tty.h>
#include <suntool/textsw.h>
#include <suntool/seln.h>
#include <sys/stat.h>       /* stat call needed to verify existence of files */

/* these objects are global so their attributes can be modified or retrieved */
Frame      base_frame, edit_frame, ls_flags_frame;
Panel      panel, ls_flags_panel;
Tty        ttysw;
Textsw     editsw;
Panel_item dir_item, fname_item, msg_item;

#define MAX_FILENAME_LEN 256
#define MAX_PATH_LEN     1024 

char *getwd();

/***************************************************************************/
/* routines to create windows and start up tool                            */
/***************************************************************************/

main(argc, argv)
int argc;
char **argv;
{
    base_frame = window_create(NULL, FRAME,
		 FRAME_ARGS,     argc, argv, 
		 FRAME_LABEL,    "filer",
		 0);

    create_panel_subwindow();
    create_tty_subwindow();
    create_edit_popup();
    create_ls_flags_popup();

    window_main_loop(base_frame);
    exit(0);
}

create_panel_subwindow() 
{
    void ls_proc(), ls_flags_proc(), quit_proc(), edit_proc(), del_proc();

    char current_dir[MAX_PATH_LEN];

    panel = window_create(base_frame, PANEL, 0);

    dir_item = panel_create_item(panel, PANEL_TEXT,
        PANEL_LABEL_X,              ATTR_COL(0),
        PANEL_LABEL_Y,              ATTR_ROW(0),
        PANEL_VALUE_DISPLAY_LENGTH, 22,
        PANEL_VALUE,                getwd(current_dir),
        PANEL_LABEL_STRING,         "Directory: ",
        0);

    (void) panel_create_item(panel, PANEL_BUTTON,
        PANEL_LABEL_IMAGE, panel_button_image(panel,"List",0,0),
        PANEL_NOTIFY_PROC, ls_proc,
        0);

    (void) panel_create_item(panel, PANEL_BUTTON,
        PANEL_LABEL_IMAGE, panel_button_image(panel,"Set ls flags",0,0),
        PANEL_NOTIFY_PROC, ls_flags_proc,
        0);

    (void) panel_create_item(panel, PANEL_BUTTON,
        PANEL_LABEL_IMAGE, panel_button_image(panel,"Edit",0,0),
        PANEL_NOTIFY_PROC, edit_proc,
        0);

    (void) panel_create_item(panel, PANEL_BUTTON,
        PANEL_LABEL_IMAGE, panel_button_image(panel,"Delete",0,0),
        PANEL_NOTIFY_PROC, del_proc,
        0);

    (void) panel_create_item(panel, PANEL_BUTTON,
        PANEL_LABEL_IMAGE, panel_button_image(panel,"Quit",0,0),
        PANEL_NOTIFY_PROC, quit_proc,
        0);

    fname_item = panel_create_item(panel, PANEL_TEXT,
        PANEL_LABEL_X,              ATTR_COL(0),
        PANEL_LABEL_Y,              ATTR_ROW(1),
        PANEL_VALUE_DISPLAY_LENGTH, 22,
        PANEL_LABEL_STRING,         "File:      ",
        0);

    msg_item = panel_create_item(panel, PANEL_MESSAGE, 0);

    window_fit_height(panel);

    window_set(panel, PANEL_CARET_ITEM, fname_item, 0);
}



create_tty_subwindow()
{
    ttysw = window_create(base_frame, TTY, 0);
}

create_edit_popup()
{
    edit_frame = window_create(base_frame, FRAME, 
		 FRAME_SHOW_LABEL, TRUE,
		 0);
    editsw = window_create(edit_frame, TEXTSW, 0);
}

create_ls_flags_popup()
{
    ls_flags_frame = window_create(base_frame, FRAME, 0);

    ls_flags_panel= window_create(ls_flags_frame, PANEL, 0);

    panel_create_item(ls_flags_panel, PANEL_MESSAGE,
		      PANEL_ITEM_X,       ATTR_COL(14),
		      PANEL_ITEM_Y,       ATTR_ROW(0),
		      PANEL_LABEL_STRING, "Options for ls command",
		      PANEL_CLIENT_DATA,  "   ",
                      0);

    panel_create_item(ls_flags_panel, PANEL_CYCLE,
		   PANEL_ITEM_X,         ATTR_COL(0),
		   PANEL_ITEM_Y,         ATTR_ROW(1),
		   PANEL_DISPLAY_LEVEL,  PANEL_CURRENT,
                   PANEL_LABEL_STRING,   "Format:                          ",
                   PANEL_CHOICE_STRINGS, "Short", "Long", 0,
		   PANEL_CLIENT_DATA,    " l ",
                   0);

    panel_create_item(ls_flags_panel, PANEL_CYCLE,
		   PANEL_ITEM_X,         ATTR_COL(0),
		   PANEL_ITEM_Y,         ATTR_ROW(2),
		   PANEL_DISPLAY_LEVEL,  PANEL_CURRENT,
                   PANEL_LABEL_STRING,   "Sort order:                      ",
                   PANEL_CHOICE_STRINGS, "Descending", "Ascending", 0,
		   PANEL_CLIENT_DATA,    " r ",
                   0);

    panel_create_item(ls_flags_panel, PANEL_CYCLE,
		   PANEL_ITEM_X,         ATTR_COL(0),
		   PANEL_ITEM_Y,         ATTR_ROW(3),
		   PANEL_DISPLAY_LEVEL,  PANEL_CURRENT,
                   PANEL_LABEL_STRING,   "Sort criterion:                  ",
                   PANEL_CHOICE_STRINGS, "Name", "Modification Time",
					 "Access Time", 0,
		   PANEL_CLIENT_DATA,    " tu",
                   0);

    panel_create_item(ls_flags_panel, PANEL_CYCLE,
		   PANEL_ITEM_X,         ATTR_COL(0),
		   PANEL_ITEM_Y,         ATTR_ROW(4),
		   PANEL_DISPLAY_LEVEL,  PANEL_CURRENT,
                   PANEL_LABEL_STRING,   "For directories, list:           ",
                   PANEL_CHOICE_STRINGS, "Contents", "Name Only", 0,
		   PANEL_CLIENT_DATA,    " d ",
                   0);

    panel_create_item(ls_flags_panel, PANEL_CYCLE,
		   PANEL_ITEM_X,         ATTR_COL(0),
		   PANEL_ITEM_Y,         ATTR_ROW(5),
		   PANEL_DISPLAY_LEVEL,  PANEL_CURRENT,
                   PANEL_LABEL_STRING,   "Recursively list subdirectories? ",
                   PANEL_CHOICE_STRINGS, "No", "Yes", 0,
		   PANEL_CLIENT_DATA,    " R ",
                   0);

    panel_create_item(ls_flags_panel, PANEL_CYCLE,
		   PANEL_ITEM_X,         ATTR_COL(0),
		   PANEL_ITEM_Y,         ATTR_ROW(6),
		   PANEL_DISPLAY_LEVEL,  PANEL_CURRENT,
                   PANEL_LABEL_STRING,   "List '.' files?                  ",
                   PANEL_CHOICE_STRINGS, "No", "Yes", 0,
		   PANEL_CLIENT_DATA,    " a ",
                   0);

    panel_create_item(ls_flags_panel, PANEL_CYCLE,
		   PANEL_ITEM_X,         ATTR_COL(0),
		   PANEL_ITEM_Y,         ATTR_ROW(7),
		   PANEL_DISPLAY_LEVEL,  PANEL_CURRENT,
                   PANEL_LABEL_STRING,   "Indicate type of file?           ",
                   PANEL_CHOICE_STRINGS, "No", "Yes", 0,
		   PANEL_CLIENT_DATA,    " F ",
                   0);

    window_fit(ls_flags_panel);  /* fit panel around its items */
    window_fit(ls_flags_frame);  /* fit frame around the panel */
}

/***************************************************************************/
/* notify procedures for panel items                                       */
/***************************************************************************/

char *
compose_ls_options()
{
   static char flags[20];
   char *ptr;
   char flag;
   int first_flag = TRUE;
   Panel_item item;
   char *client_data;
   int index;

   ptr = flags;

   panel_each_item(ls_flags_panel, item)
       client_data = panel_get(item, PANEL_CLIENT_DATA, 0);
       index       = (int)panel_get_value(item);
       flag        = client_data[index];
       if (flag != ' ') {
           if (first_flag) {
	      *ptr++     = '-';
	      first_flag = FALSE;
           }
           *ptr++ = flag;
       }
   panel_end_each
   *ptr = '\0';
   return flags;
}

void
ls_proc()
{
   static char previous_dir[MAX_PATH_LEN];
   char *current_dir;
   char cmdstring[100];           /* dir_item's value can be 80, plus flags */

   msg("");

   current_dir = (char *)panel_get_value(dir_item);

   if (strcmp(current_dir, previous_dir)) {
       chdir((char *)panel_get_value(dir_item));
       strcpy(previous_dir, current_dir);
   }
   sprintf(cmdstring, "ls %s %s\n", 
           compose_ls_options(),
	   panel_get_value(fname_item));
   ttysw_input(ttysw, cmdstring, strlen(cmdstring));
}

void
ls_flags_proc()
{
    window_set(ls_flags_frame, WIN_SHOW, TRUE, 0);
}

/* return a pointer to the current selection */
char *
get_selection()
{
    static char   filename[MAX_FILENAME_LEN];
    Seln_holder   holder;
    Seln_request  *buffer;
 
    holder = seln_inquire(SELN_PRIMARY);
    buffer = seln_ask(&holder, SELN_REQ_CONTENTS_ASCII, 0, 0);
    strncpy(filename, buffer->data + sizeof(Seln_attribute), MAX_FILENAME_LEN);
    return (filename);
}

/* return 1 if file exists, else print error message and return 0 */
stat_file(filename)
char *filename;
{
    struct stat statbuf;

    if (stat(filename, &statbuf) < 0) {
	char buf[MAX_FILENAME_LEN+11]; /* big enough for message */
	sprintf(buf, "%s not found.", filename);
	msg(buf);
	return 0;
    }
    return 1;
}
void
edit_proc()
{
    char *filename;

    /* return if no selection */
    if (!strlen(filename = get_selection())) {
	msg("Please select a file to edit.");
	return;
    }

    /* return if file not found */
    if (!stat_file(filename))
	return;

    msg(""); /* clear any old messages */

    window_set(editsw, TEXTSW_FILE, filename, 0);

    window_set(edit_frame, FRAME_LABEL, filename, WIN_SHOW, TRUE, 0);
}

void
del_proc()
{
    char buf[300];
    char *filename;

    /* return if no selection */
    if (!strlen(filename = get_selection())) {
	msg("Please select a file to delete.");
	return;
    }

    /* return if file not found */
    if (!stat_file(filename))
	return;

    msg(""); /* clear any old messages */

    /* usr must confirm the delete */
    sprintf(buf, "Ok to delete %s?", filename);
    if (confirm_yes(buf)) {
        unlink(filename);
        sprintf(buf, "%s deleted.", filename);
        msg(buf);
    }
}

void
quit_proc()
{
    window_destroy(base_frame);
}

msg(msg)
char *msg;
{
    panel_set(msg_item, PANEL_LABEL_STRING, msg, 0);
}
