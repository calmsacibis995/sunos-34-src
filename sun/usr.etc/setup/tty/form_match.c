#ifndef lint
static	char sccsid[] = "@(#)form_match.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
* Copyright (c) 1985 by Sun Microsystems, Inc.
*/


#include "tty_global.h"
#include "user_info.h"
#include "form_match.h"

typedef	struct {
    char	*str;		/* string we are looking for */
    Input_action match_action;	/* actionue to return when we find it */
} match_info;


#define MAX_NUM_STRINGS	100
static	match_info	strings[MAX_NUM_STRINGS];
static	int		num_strings = 0;

static	Match_type	match();


static
Match_type
match(match_str, s)
register	char	*match_str;
 		int	s;
{   
    register	char	*str;
    
    str = strings[s].str;
    
    for (; (*str != '\0') && (*match_str != '\0') && (*str == *match_str); 
	 str++, match_str++) {
	;
    }
    
    if ((*str == '\0') && (*match_str == '\0'))
	return (MATCH);
    if ((*str != '\0') && (*match_str == '\0'))
	return (PARTIAL_MATCH);
    
    return(NO_MATCH);
}



form_input_match_add(str, action)
char		*str;
Input_action	action;
{   
    if (str == NULL)
	return;
    
    if (num_strings < MAX_NUM_STRINGS) {
	strings[num_strings].str = (char *) malloc(strlen(str) +2);
	strcpy(strings[num_strings].str, str);
	strings[num_strings].match_action = action;
	num_strings++;
    }
    else {
	fprintf(stderr, "Setup - tty - too many input match strings.\n");
    }
}



Match_type
form_input_match(str, action_ptr)
register	char		*str;
register	Input_action	*action_ptr;
{   
    register	int	i;
    register	int	partial_match;
    
    partial_match = FALSE;
    for (i = 0; i < num_strings; i++) {
	switch (match(str, i)) {
	  case MATCH:
	    *action_ptr = strings[i].match_action;
	    return(MATCH);
	    
	  case PARTIAL_MATCH:
	    partial_match = TRUE;
	    break;
	    
	}
    }
    
    if (partial_match)
	return(PARTIAL_MATCH);
    
    return(NO_MATCH);
}

		
							    
    
    
