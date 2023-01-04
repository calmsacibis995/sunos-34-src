#ifndef lint
static	char sccsid[] = "@(#)tty_main.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */


#include "tty_global.h"
#include "tty_list.h"
#include <signal.h>

Form	nav_form;
Form	error_form;
Form	con_form;	/* for CONfirmation and CONtinue procs */

Form	edit_forms[MAX_NUM_FORMS];
Form	ws_form;
Form	edit_disk_form;
Form	software_form;
Form	setup_form;
Form	defaults_form;

Form	client_list_form;
Form	card_list_form;
Form	client_form;
Form	card_form;

Workstation	ws;


static	int	up_and_die();

extern	int	tty_error_msg();
extern	int	tty_confirm_proc();
extern	int	tty_continue_proc();


main(argc, argv)
int	argc;
char	*argv[];
{   
    int		upgrade;
    int		my_argc;
    char	**my_argv;
    
    for (my_argc = argc, my_argv = argv; 
	 my_argc > 1; my_argv++, my_argc--) {
	switch (my_argv[1][1]) {
	  case 'd':
	    my_argv++; 
	    my_argc--;
	    (void) freopen(my_argv[1], "w", stdout);
	    break;
	}
    }

    ws = mid_init(argc, argv, tty_error_msg);
    setup_set(ws, 
	      SETUP_CALLBACK, 		glue_callback_proc, 
	      SETUP_CONFIRM_PROC, 	tty_confirm_proc, 
	      SETUP_CONTINUE_PROC, 	tty_continue_proc, 
	      0);
    
    signal(SIGINT, up_and_die);
    
    tty_initialize_curses();
    get_user_info();
	
    tty_initialize_forms(&nav_form, &error_form, 
			 &client_list_form, &card_list_form, 
			 &client_form, &card_form, 
			 &con_form, 
			 MAX_NUM_FORMS, edit_forms);
    tty_error_init(error_form);
    
    ws_form		= edit_forms[(int)TTY_WORKSTATION_FORM];
    edit_disk_form	= edit_forms[(int)TTY_EDIT_DISK_FORM];
    software_form	= edit_forms[(int)TTY_SOFTWARE_FORM];
    setup_form		= edit_forms[(int)TTY_SETUP_FORM];
    defaults_form	= edit_forms[(int)TTY_DEFAULTS_FORM];
    
    upgrade = (int) setup_get(ws, SETUP_UPGRADE);
    if (upgrade) {
	upgrade_init(ws);
    }

    tty_nav_init();
    tty_workstation_init();
    tty_edit_disk_init();
    tty_client_init();
    tty_card_init();
    tty_software_init();
    tty_setup_init();
    tty_defaults_init();
    
    tty_nav_doit();
    
    endwin();
}



static
int
up_and_die()
{   
    		char	ch;
    extern	WINDOW	*whole_screen_win;
    register	int	row, col;
    
    signal(SIGINT, SIG_IGN);
    
    overwrite(curscr, whole_screen_win);
    getyx(curscr, row, col);
    
    wclear(stdscr);
    clearok(stdscr, TRUE);
    wprintw(stdscr,
      "Setup has been interrupted. Do you really want to exit ('y' or 'n')?");
    wrefresh(stdscr);
    read(fileno(stdin), &ch, 1);
    if (ch == 'y') {
	mid_cleanup();
	move(0, 0);
	clear();
	refresh();
	endwin();
	exit(0);
    }
    else {
	clearok(stdscr, TRUE);
	overwrite(whole_screen_win, stdscr);
	wmove(stdscr, row, col);
	wrefresh(stdscr);
	signal(SIGINT, up_and_die);	
    }
    
}



