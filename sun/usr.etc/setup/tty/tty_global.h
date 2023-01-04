/*	@(#)tty_global.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "form.h"
#include "form_attr.h"
#include <setup_attr.h>
#include <setup_message_table.h>

/* 
 * Constants for creating curses windows.
 */

#define NAV_WIN_SIZE	2
#define LINE1_WIN_SIZE	1
#define ERROR_WIN_SIZE	2
#define LINE2_WIN_SIZE	1
#define EDIT_WIN_SIZE	(LINES -			\
			 ( ERROR_WIN_SIZE +	\
			  NAV_WIN_SIZE	 +	\
			  LINE1_WIN_SIZE +	\
			  LINE2_WIN_SIZE ))

#define LIST_WIN_SIZE	4
#define	CARD_CLIENT_WIN_SIZE	(LINES -		\
				 ( ERROR_WIN_SIZE +	\
				  NAV_WIN_SIZE	 +	\
				  LINE1_WIN_SIZE +	\
				  LINE2_WIN_SIZE +	\
				  LIST_WIN_SIZE))
#define CLIENT_WIN_WIDTH	40
#define	CARD_WIN_WIDTH		(COLS - CLIENT_WIN_WIDTH)


#define NAV_WIN_START	0
#define LINE1_WIN_START	(NAV_WIN_START + NAV_WIN_SIZE)
#define ERROR_WIN_START	(LINE1_WIN_START + LINE1_WIN_SIZE)
#define LINE2_WIN_START	(ERROR_WIN_START + ERROR_WIN_SIZE)
#define EDIT_WIN_START	(LINE2_WIN_START + LINE2_WIN_SIZE)
#define LIST_WIN_START	(LINE2_WIN_START + LINE2_WIN_SIZE)
#define CARD_CLIENT_WIN_START	(LIST_WIN_START + LIST_WIN_SIZE)

#define SETUP_ERROR_WIN_START	0
#define SETUP_ERROR_WIN_SIZE	11
#define SETUP_ERROR_BIG_WIN_SIZE	(EDIT_WIN_SIZE + LINE2_WIN_SIZE + ERROR_WIN_SIZE)

#define	SETUP_LINE_WIN_START	(SETUP_ERROR_WIN_START + SETUP_ERROR_WIN_SIZE)
#define	SETUP_LINE_WIN_SIZE	1
#define	SETUP_WIN_START		(SETUP_LINE_WIN_START + SETUP_LINE_WIN_SIZE)
#define	SETUP_WIN_SIZE		(LINES - (SETUP_LINE_WIN_SIZE + SETUP_ERROR_WIN_SIZE))



typedef enum {
    TTY_WORKSTATION_FORM, 
    TTY_EDIT_DISK_FORM,
    TTY_SOFTWARE_FORM,
    TTY_SOFTPART_FORM,
    TTY_SETUP_FORM,
    TTY_DEFAULTS_FORM,
    
    TTY_NUM_FORMS
} TTY_forms;
    
#define NUM_OTHER_FORMS	3

#define MAX_NUM_FORMS	((int)TTY_NUM_FORMS + NUM_OTHER_FORMS)



#define TTY_CMD_SETUP_EXECUTE	0
#define TTY_CMD_SETUP_REBOOT	1



typedef struct {
    Form	new_form;
    char	*str;
} New_form;

New_form *tty_new_form_create();

extern	int	tty_bool_choice_notify_proc();
extern	int	tty_new_form_notify_proc();
extern	int	tty_done_notify_proc();
extern	int	glue_notify_proc();
extern	void	glue_callback_proc();
