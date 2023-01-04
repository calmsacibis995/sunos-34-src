#ifndef lint
static	char sccsid[] = "@(#)tty_init.c 1.1 86/09/25 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */


#include "tty_global.h"
#include "user_info.h"
#include "form_match.h"

#define	 CONTROL_B  (1 + 'b' - 'a')
#define	 CONTROL_F  (1 + 'f' - 'a')
#define	 CONTROL_N  (1 + 'n' - 'a')
#define	 CONTROL_P  (1 + 'p' - 'a')
#define	 CONTROL_R  (1 + 'r' - 'a')
#define	 CONTROL_W  (1 + 'w' - 'a')
#define	 ESCAPE     '\033'
#define	 DEL	    '\077'

extern	int	_putchar();
User_info	user_info;

static	char	*space_ptr;
static	void	check_termcap();


get_user_info()
{   
    struct 	sgttyb 	tty;
    struct 	ltchars ltc;
		char	buff[3];
		char	*getenv();
		char	termcap_buff[1024];
		char	space[256];
		char	*str, *tgetstr();
		
    gtty(fileno(stdout), &tty);
    
    ioctl(fileno(stdout), TIOCGLTC, (struct sgttyb *) &ltc);
         
    user_info.c_erase   = tty.sg_erase;
    user_info.line_kill = tty.sg_kill;

    if (ltc.t_werasc == (char) -1) {
        user_info.w_erase = CONTROL_W;
    } 
    else {
        user_info.w_erase = ltc.t_werasc;
    }
    /*
     * Set the editing characters.
     * To be nice, both DEL and Backspace are set to be character erase
     * keys.
     */
    buff[1] = '\0';
    buff[0] = user_info.c_erase;
    form_input_match_add(buff, INPUT_CHAR_DEL);
    buff[0] = user_info.w_erase;
    form_input_match_add(buff, INPUT_WORD_DEL);
    buff[0] = user_info.line_kill;
    form_input_match_add(buff, INPUT_LINE_DEL);
    form_input_match_add("\b", INPUT_CHAR_DEL);
    buff[0] = DEL;
    form_input_match_add(buff, INPUT_CHAR_DEL);

    /*
     * Set chars for selecting items and cursor movement within
     * a window.
     */
    form_input_match_add("X", INPUT_SELECT_ACTION);
    form_input_match_add("x", INPUT_SELECT_ACTION);
    buff[0] = CONTROL_F;
    form_input_match_add(buff, INPUT_NEXT_ITEM);
    buff[0] = CONTROL_B;
    form_input_match_add(buff, INPUT_PREV_ITEM);
    form_input_match_add("\n", INPUT_NEXT_ITEM);

    /*
     * Set chars for cursor movement between windows and refreshing
     * the screen.
     */
    buff[0] = CONTROL_P;
    form_input_match_add(buff, INPUT_PREV_WINDOW);
    buff[0] = CONTROL_N;
    form_input_match_add(buff, INPUT_NEXT_WINDOW);
    buff[0] = CONTROL_R;
    form_input_match_add(buff, INPUT_REFRESH);
    
    tgetent(termcap_buff, getenv("TERM"));
    space_ptr = space;
    str = tgetstr("ks", &space_ptr);
    if (str != NULL)
	_puts(str);	/* initialize/enable keypad  */
    space_ptr = space;	/* don't need startup string anymore */
    
    form_input_match_add(tgetstr("kd", &space_ptr), INPUT_NEXT_ITEM);
    form_input_match_add(tgetstr("kr", &space_ptr), INPUT_NEXT_ITEM);
    form_input_match_add(tgetstr("ku", &space_ptr), INPUT_PREV_ITEM);
    form_input_match_add(tgetstr("kl", &space_ptr), INPUT_PREV_ITEM);
}



/* 
 * initialize curses and create (curses) windows.
 */

WINDOW	*whole_screen_win;

WINDOW	*nav_win;
WINDOW	*line1_win;
WINDOW	*error_win;
WINDOW	*line2_win;
WINDOW	*edit_win;
WINDOW	*client_list_win;
WINDOW	*card_list_win;
WINDOW	*client_win;
WINDOW	*card_win;

WINDOW	*setup_win;
WINDOW	*setup_line_win;
WINDOW	*setup_error_win;
WINDOW	*setup_error_big_win;
				    
tty_initialize_curses()
{   
    initscr();		/* init curses */
    clear();
    refresh();
    noecho();
    crmode();
    
    whole_screen_win = newwin(0, 0, 0, 0);
    
    nav_win = newwin(NAV_WIN_SIZE, COLS, NAV_WIN_START, 0);
    error_win = newwin(ERROR_WIN_SIZE, COLS, ERROR_WIN_START, 0);
    line1_win = newwin(LINE1_WIN_SIZE, COLS, LINE1_WIN_START, 0);
    line2_win = newwin(LINE2_WIN_SIZE, COLS, LINE2_WIN_START, 0);
    edit_win  = newwin(EDIT_WIN_SIZE,  COLS, EDIT_WIN_START,  0);
    client_list_win  = newwin(LIST_WIN_SIZE,  CLIENT_WIN_WIDTH, 
			      LIST_WIN_START,  0);
    card_list_win    = newwin(LIST_WIN_SIZE,  0, 
			      LIST_WIN_START, CLIENT_WIN_WIDTH);
    client_win       = newwin(CARD_CLIENT_WIN_SIZE,  CLIENT_WIN_WIDTH, 
			      CARD_CLIENT_WIN_START,  0);
    card_win         = newwin(CARD_CLIENT_WIN_SIZE,  0, 
			      CARD_CLIENT_WIN_START, CLIENT_WIN_WIDTH);
    
    box(line1_win, '-', '-');
    box(line2_win, '-', '-');
    
    wrefresh(nav_win);
    wrefresh(line1_win);
    wrefresh(error_win);
    wrefresh(line2_win);
    wrefresh(edit_win);
    wrefresh(client_list_win);
    wrefresh(card_list_win);
    wrefresh(client_win);
    wrefresh(card_win);
    
    setup_error_win = newwin(SETUP_ERROR_WIN_SIZE, COLS, SETUP_ERROR_WIN_START, 0);
    setup_line_win = newwin(SETUP_LINE_WIN_SIZE, COLS, SETUP_LINE_WIN_START, 0);
    setup_win = newwin(SETUP_WIN_SIZE, COLS, SETUP_WIN_START, 0);
    box(setup_line_win,  '-', '-');

    setup_error_big_win  = newwin(SETUP_ERROR_BIG_WIN_SIZE,  
				  COLS, ERROR_WIN_START,  0);
    
}
    



tty_initialize_forms(nf, ef, clientlf, cardlf, clientf, cardf, conf, n, edfs)
Form	*nf;
Form	*ef;
Form	*clientlf, *cardlf, *clientf, *cardf;
Form	*conf;
int	n;
Form	edfs[];
{   
    int		i;
    
    *nf = form_create(nav_win, 0, NAV_WIN_SIZE - 1);
    *ef = form_create(error_win, 0, ERROR_WIN_SIZE - 1);
    *clientlf = form_create(client_list_win, 0, LIST_WIN_SIZE - 1);
    *cardlf   = form_create(card_list_win,   0, LIST_WIN_SIZE - 1);
    *clientf  = form_create(client_win,      0, CARD_CLIENT_WIN_SIZE - 1);
    *cardf    = form_create(card_win,        0, CARD_CLIENT_WIN_SIZE - 1);
    *conf     = form_create(setup_win,       0, SETUP_WIN_SIZE - 1);
    
    for (i = 0; i < n; i++) {
	edfs[i] = form_create(edit_win, 0, EDIT_WIN_SIZE - 1);
    }
}
