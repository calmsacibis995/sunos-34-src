#ifndef lint
static	char sccsid[] = "@(#)cmdtool.c 1.3 87/01/07";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 *  shelltool - run a process in a tty subwindow
 */

#include <stdio.h>
#include <suntool/tool_hs.h>
#include <suntool/ttysw.h>
#include <suntool/ttytlsw.h>
#include <suntool/textsw.h>
#include <suntool/scrollbar.h>
#include <suntool/tool.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/stat.h>

#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif

#define	TEXTSW	textsw_window_object, 1
extern int	textsw_window_object();

extern	char *getenv();
extern        caddr_t ttytlsw_create();

static short cmd_image[258] = {
#include <images/cmdtool.icon>
};

mpr_static(cmdic_mpr, 64, 64, 1, cmd_image);

static	struct icon cmd_icon = {64, 64, (struct pixrect *)NULL, 0, 0, 64, 64,
	    &cmdic_mpr, 0, 0, 0, 0, NULL, (struct pixfont *)NULL,
	    ICON_BKGRDCLR};

static	struct pixfont	 *font;

#define	WIDTH_ADJUST \
	((int)textsw_get(TEXTSW, TEXTSW_LEFT_MARGIN) + 2 + \
	 (int)scrollbar_get(SCROLLBAR, SCROLL_THICKNESS) + \
	 (2 * tool_borderwidth(0)))

static	base_width(tool_attrs)
	Attr_avlist	tool_attrs;
{
	int	  width = 80;
	char	**value_ptr;
	
	if (tool_attrs
	&&  tool_find_attribute(tool_attrs, WIN_COLUMNS, value_ptr)) {
		width = (int)(*value_ptr);
	}
	tool_free_attribute(WIN_COLUMNS, value_ptr);
	return (width * font->pf_defaultsize.x);
}

static	width_is_set(attrs)
	Attr_avlist	attrs;
{
	int	is_set = 0;

	if (!attrs)
	    return is_set;
	for (; *attrs; attrs = attr_next(attrs)) {
	    switch ((Win_attribute)(*attrs)) {
	      case WIN_WIDTH:
	        is_set = 1;
		break;
	      case WIN_COLUMNS:
	        is_set = 0;
		break;
	    }
	}
	return is_set;
}

static	Textsw	textsw;

#ifdef STANDALONE
main(argc, argv)
#else
int cmdtool_main(argc, argv)
#endif
	int argc;
	char **argv;
{
	Attr_avlist	tool_attrs = NULL;
	int	become_console = 0;
	char	*tool_name = argv[0], *tmp_str;
	char	*label_default = "cmdtool";
	char	*label_console = " (CONSOLE) - ";
	char	label[150];
	char	icon_label[30];
	char	*sh_argv[2];
	caddr_t	ttysw;
	int	child_pid;
	struct	tool *tool;
	Notify_value wait_child();
	int	bold_style = -1;
	struct	icon *icon = &cmd_icon;
	int	checkpoint = 0;
	Textsw	ttysw_to_textsw();

	setbuf(stderr, 0);	/* unbuffer stderr */
	ttysw_cmdsw = 1;
	sh_argv[0] = NULL;
	sh_argv[1] = NULL;
	argv++;
	argc--;
	/* Pick up command line arguments to modify tool behavior */
	if (tool_parse_all(&argc, argv, &tool_attrs, tool_name) == -1) {
		tool_usage(tool_name);
		EXIT(1);
	}
	if (! (font = pw_pfsysopen())) {
		fprintf(stderr, "Cannot get default font.\n");
		EXIT(1);
	}
	checkpoint =
	    defaults_get_integer_check("/Tty/Checkpoint_frequency",
	    0, 0, TEXTSW_INFINITY, NULL);
	/* Get ttysw related args */
	while (argc > 0 && **argv == '-') {
		switch (argv[0][1]) {
		case 'C':
			become_console = 1;
			break;
		case 'P':
			checkpoint = atoi(argv[1]);
			argc--, argv++;
			break;
		case '?':
			tool_usage(tool_name);
			fprintf(stderr, "To make the console use -C\n");
			fprintf(stderr,
			"To cause checkpointing, use -P n\n");
			EXIT(1);
		default:
			;
		}
		argv++;
		argc--;
	}
	if (argc == 0) {
		argv = sh_argv;
		if ((argv[0] = getenv("SHELL")) == NULL)
			argv[0] = "/bin/sh";
	}
	/* Set default icon label */
	if (tool_find_attribute(tool_attrs, WIN_LABEL, &tmp_str)) {
		/* Using tool label supplied on command line */
		strncat(icon_label, tmp_str, sizeof(icon_label));
		tool_free_attribute(WIN_LABEL, tmp_str);
	} else if (become_console)
		strncat(icon_label, "CONSOLE", sizeof(icon_label));
	else
		/* Use program name that is run under ttysw */
		strncat(icon_label, argv[0], sizeof(icon_label));
	/* Buildup tool label */
	strcat(label, label_default);
	if (become_console)
		strcat(label, label_console);
	else
		strcat(label, " - ");
	strncat(label, *argv, sizeof(label)-
	    strlen(label_default)-strlen(label_console)-1);
	/* Create tool window */
	tool = tool_begin(
	    WIN_LABEL,		label,
	    WIN_LINES,		34,
	    WIN_NAME_STRIPE,	1,
	    WIN_ICON,		icon,
	    WIN_ICON_LABEL,	icon_label,
	    WIN_ATTR_LIST,	tool_attrs,
	    WIN_BOUNDARY_MGR,	TRUE,
	    0);
	if (tool == (struct tool *)NULL)
		EXIT(1);
	/*  Allow -Ws after -Ww to override the -Ww.  */
	if (!width_is_set(tool_attrs)) {
		tool_set_attributes(tool,
		    WIN_WIDTH,	base_width(tool_attrs) + WIDTH_ADJUST,
		    0);
	}
	tool_free_attribute_list(tool_attrs);
	/* Create tty tool subwindow */
	ttysw = ttytlsw_create(tool, "ttysw", TOOL_SWEXTENDTOEDGE,
	    TOOL_SWEXTENDTOEDGE);
	if (ttysw == NULL)
		EXIT(1);
	if (bold_style != -1)
		ttysw_setboldstyle(bold_style);
	/* Install tool in tree of windows */
	tool_install(tool);
	/* Start tty process */
	if (become_console)
		ttysw_becomeconsole(ttysw);
	if ((child_pid = ttysw_start(ttysw, argv)) == -1) {
		perror(tool_name);
		EXIT(1);
	}
	/* Wait for child process to die */
	(void) notify_set_wait3_func(tool, wait_child, child_pid);
	textsw = ttysw_to_textsw(ttysw);
	textsw_set(textsw, TEXTSW_CHECKPOINT_FREQUENCY, checkpoint,
			   TEXTSW_NO_RESET_TO_SCRATCH, TRUE,
			   0);
	/* Handle notifications */
	(void) notify_start();
	
	EXIT(0);
}

static
Notify_value
wait_child(tool, pid, status, rusage)
	Tool *tool;
	int pid;
	union wait *status;
	struct rusage *rusage;
{
	char		*filename = (char *)0;
	/* Note: Could handle child stopping differently from child death */
	/* Break out of notification loop */
	if (WIFSTOPPED(*status))
	    return(NOTIFY_IGNORED);
	else {
	    if (status->w_termsig) {
		fprintf(stderr, "child of cmdtool died due to signal %d\n",
			status->w_termsig);
	    } else if (status->w_retcode) {
		fprintf(stderr,
			"child of cmdtool exited with return code %d\n",
			status->w_retcode);
	    }
	    if (status->w_coredump) {
		fprintf(stderr, "child of cmdtool left a core dump\n");
	    }
	    switch (notify_die(DESTROY_PROCESS_DEATH)) {
	      case NOTIFY_DESTROY_VETOED:
	        fprintf(stderr, "destroy vetoed\n");
	      case NOTIFY_OK:
	      default:
		break;
	    }
	    return(NOTIFY_DONE);
	}
}

