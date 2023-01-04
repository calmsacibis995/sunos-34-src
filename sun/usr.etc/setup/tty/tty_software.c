#ifndef lint
static	char sccsid[] = "@(#)tty_software.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
* Copyright (c) 1985 by Sun Microsystems, Inc.
*/



#include "tty_global.h"
#include "tty_item.h"
#include "tty_list.h"


static	Item	arch_item;

#define	MAX_OSWGS	32
static	Item	oswg_items[MAX_OSWGS];
static	int	num_oswg_items;
static	Item	none_item;
static	Item	all_item;
static	Item	reset_item;

static	int	cur_bit_vector;
static	Arch_served	cur_arch;

static	int	set_oswg_items();
static	int	oswg_notify_proc();
static	int	arch_notify_proc();
static	int	oswg_callback_proc();
static	int	bulk_set_notify_proc();

#define	BIT_YES(v, n)  		v |= (1 << n)
#define BIT_NO(v, n)   		v &= ~(1 << n)
#define BIT_IS_YES(v, n)	((v >> n) & 0x01)

extern	Workstation ws;
extern	Form	software_form;




tty_software_init() 
{   
    register	int	i;
    register	Oswg	oswg;
    		char	buff[100];
    
    arch_item	= CHOICE_ITEM(software_form, 1, 0, "Optional Software for:", -1);
    form_item_set(arch_item, ITEM_VALUE, 0, 0);
    form_set(software_form, FORM_CURRENT_IO_ITEM, arch_item, 0);
    form_item_set(arch_item, ITEM_NOTIFY_PROC, arch_notify_proc, 0);
    
    none_item   = BUTTON_ITEM(software_form, 2, 0, "Clear");
    all_item    = BUTTON_ITEM(software_form, 2, 20, "All");
    reset_item  = BUTTON_ITEM(software_form, 2, 40, "Common Choices");
    form_item_set(none_item,  ITEM_NOTIFY_PROC, bulk_set_notify_proc, 0);
    form_item_set(all_item,   ITEM_NOTIFY_PROC, bulk_set_notify_proc, 0);
    form_item_set(reset_item, ITEM_NOTIFY_PROC, bulk_set_notify_proc, 0);
    
    num_oswg_items = 0;
    SETUP_FOREACH_OBJECT(ws, WS_OSWG, i, oswg) {
	oswg_items[num_oswg_items] = TOGGLE_ITEM(software_form, 4 + i, 0, "", -1);
	form_item_set(oswg_items[num_oswg_items], 
		      ITEM_CLIENT_DATA, 	i, 
		      ITEM_NOTIFY_PROC, 	oswg_notify_proc, 
		      0);
	num_oswg_items++;
    } SETUP_END_FOREACH;

}

tty_software_pre_display()
{   
    Arch_served		arch_served;
    register	int	i;
    
    form_item_set(arch_item, ITEM_CHOICE_RESET, 0);
    SETUP_FOREACH_OBJECT(ws, WS_ARCH_SERVED_ARRAY, i, arch_served) {
	form_item_set(arch_item, 
         ITEM_CHOICE_STRING, setup_get(arch_served, ARCH_SERVED_NAME), 0);
	setup_set(arch_served, SETUP_CALLBACK, oswg_callback_proc, 0);
    } SETUP_END_FOREACH
    
    arch_notify_proc(arch_item, 0);
}



static 
int
set_oswg_items()
{   
    register	int	i;
    
    for (i = 0; i < num_oswg_items; i++) {
 	/* oswg_items are toggles; we know they only have one element
	 * of the toggle, so we can set their value to be 1 which means
	 * they have been picked or 0 which means they have not.
	 */
	if (BIT_IS_YES(cur_bit_vector, i)) 
	    form_item_set(oswg_items[i], ITEM_VALUE, 1, 0);
	else 
	    form_item_set(oswg_items[i], ITEM_VALUE, 0, 0);
    }
}




static
int
arch_notify_proc(item, ch)
Item	item;
int	ch;
{   
    register	Oswg	oswg;
    register	int	i;
    int	arch_index;
    
    /*
     * Clear all the categories.
     */
    for (i = 0; i < num_oswg_items; i++) {
	form_item_set(oswg_items[i], ITEM_DISPLAYED, FALSE, 0);
	form_item_display(oswg_items[i]);
    }
    form_set(software_form, FORM_REFRESH, 0);

    /*
     * Fill in the architecturally-dependent info.
     */
    arch_index = (int) form_item_get(item, ITEM_VALUE);
    cur_arch = (Arch_served) setup_get(ws, WS_ARCH_SERVED_ARRAY, arch_index);
    cur_bit_vector = (int) setup_get(cur_arch, ARCH_OSWG);
    SETUP_FOREACH_OBJECT(ws, WS_OSWG, i, oswg) {
	form_item_set(oswg_items[i], ITEM_TOGGLE_RESET, 0);
	form_item_set(oswg_items[i], 
          ITEM_TOGGLE_STRING, setup_get(oswg, OSWG_DESCRIPTION, arch_index), 
          0);
    } SETUP_END_FOREACH;

    set_oswg_items();
    
    /*
     * Display all the categories.
     */
    for (i = 0; i < num_oswg_items; i++) {
	form_item_set(oswg_items[i], ITEM_DISPLAYED, TRUE, 0);
	form_item_display(oswg_items[i]);
    }
    form_set(software_form, FORM_REFRESH, 0);
	
    return(FALSE);
}

static
int
oswg_notify_proc(item, ch)
Item	item;
int	ch;
{   
    register	int	bit_num;
    register	int	bit_val;
    
    bit_num = (int) form_item_get(item, ITEM_CLIENT_DATA);
    bit_val = (int) form_item_get(item, ITEM_VALUE);
    if (bit_val == 1)
	BIT_YES(cur_bit_vector, bit_num);
    else
	BIT_NO(cur_bit_vector, bit_num);
    
    setup_set(cur_arch, ARCH_OSWG, cur_bit_vector, 0);

    return (FALSE);
}


static
int
oswg_callback_proc(obj, attr, display_value, err_msg)
Opaque          obj;
Setup_attribute attr;
caddr_t         display_value;
char            *err_msg;
{   
    register	int	i;
    
    if ((Arch_served)obj != cur_arch)
	return;
    
    if (err_msg != NULL) {
	tty_error_msg(err_msg);
    }
    
    cur_bit_vector = (int) display_value;
    set_oswg_items();
    
    for (i = 0; i < num_oswg_items; i++) {
	form_item_display(oswg_items[i]);
    }
    form_set(software_form, FORM_REFRESH, 0);
    
}



static
int
bulk_set_notify_proc(item, ch)
Item	item;
int	ch;
{   
    register	int	i;
    
    if (item == none_item)
	setup_set(cur_arch, ARCH_OSWG_CLEAR, 0);
    else if (item == all_item) 
	setup_set(cur_arch, ARCH_OSWG_ALL, 0);
    else 
	setup_set(cur_arch, ARCH_OSWG_DEFAULT, 0);
    
    
    cur_bit_vector = (int) setup_get(cur_arch, ARCH_OSWG);
    set_oswg_items();    
    for (i = 0; i < num_oswg_items; i++) {
	form_item_display(oswg_items[i]);
    }
    form_set(software_form, FORM_REFRESH, 0);
    
}
