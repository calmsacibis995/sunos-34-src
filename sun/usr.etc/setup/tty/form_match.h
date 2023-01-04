/*	@(#)form_match.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

typedef enum {
    NO_MATCH, 
    PARTIAL_MATCH, 
    MATCH
} Match_type;

typedef enum {
    INPUT_NO_ACTION, 
    INPUT_NEXT_ITEM, 
    INPUT_PREV_ITEM, 
    INPUT_NEXT_WINDOW, 
    INPUT_PREV_WINDOW, 
    INPUT_SELECT_ACTION, 
    INPUT_CHAR_DEL, 
    INPUT_WORD_DEL, 
    INPUT_LINE_DEL, 
    INPUT_REFRESH,
} Input_action;

typedef struct {
    Input_action	action;
    char		*str;
} Input_info;


extern Match_type	form_input_match();
