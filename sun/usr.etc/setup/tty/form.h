/*	@(#)form.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <curses.h>
#include <sys/types.h>

extern	char *malloc();


typedef	enum {
    FORM_NOTIFY_ON_PICK, 
    FORM_NOTIFY_ON_EACH_CHAR
} Form_notify_level;

#define FORM_DEFAULT_STR_LEN	82

#define	FORM_PICK_ITEM	'\n'

#define FORM_ESC 	'\033'


typedef char *Item;
typedef char *Form;

#define	FORM_ITEM_BUTTON	button_create
#define	FORM_ITEM_TEXT		text_create
#define	FORM_ITEM_CHOICE	choice_create
#define	FORM_ITEM_TOGGLE	toggle_create

extern	Item	button_create();
extern	Item	text_create();
extern	Item	choice_create();
extern	Item	toggle_create();

extern	Form	form_create();
extern	Item	form_item_create();
extern	void	form_item_set();
extern	caddr_t	form_item_get();

typedef enum {
    FORM_NEXT, 
    FORM_PREV
} Item_direction;
