#ifndef lint
static char sccsid[] = "@(#)parse.c 1.4 86/12/24 Copyright 1985 Sun Micro";	/* from UCB 4.3 10/21/82 */
#endif

/*-
 *
 *			  Copyright (C) 1976
 *				by the
 *			  Board of Trustees
 *				of the
 *			University of Illinois
 *
 *			 All rights reserved
 *
 *
 * FILE NAME:
 *	parse.c
 *
 * PURPOSE:
 *	Contains the routines which keep track of the parse stack.
 *
 * GLOBALS:
 *	ps.p_stack =	The parse stack, set by both routines
 *	ps.il =		Stack of indentation levels, set by parse
 *	ps.cstk =		Stack of case statement indentation levels, set by parse
 *	ps.tos =		Pointer to top of stack, set by both routines.
 *
 * FUNCTIONS:
 *	parse
 *	reduce
 */
/*-
 * Copyright (C) 1976 by the Board of Trustees of the University of Illinois 
 *
 * All rights reserved 
 *
 *
 * NAME: parse 
 *
 * FUNCTION: Parse is given one input which is a "maxi token" just scanned
 * from input.  Maxi tokens are signifigant constructs such as else, {,
 * do, if (...), etc.  Parse works with reduce to maintain a parse stack
 * of these constructs.  Parse is responsible for the "shift" portion of
 * the parse algorithm, and reduce handles the "reduce" portion. 
 *
 * HISTORY: initial coding 	November 1976	D A Willcox of CAC 
 *
 */

#include "./indent_globs.h";
#include "./indent_codes.h";




parse(tk)
    int         tk;		/* the code for the construct scanned */
{
    int         i;

#ifdef debug
    printf("%2d - %s\n", tk, token);
#endif
    while (ps.p_stack[ps.tos] == ifhead && tk != elselit) {
	/* true if we have an if without an else */
	ps.p_stack[ps.tos] = stmt;	/* apply the if(..) stmt ::= stmt
					 * reduction */
	reduce();		/* see if this allows any reduction */
    }


    switch (tk) {		/* go on and figure out what to do with
				 * the input */

    case decl:			/* scanned a declaration word */
	ps.search_brace = btype_2;
	/* indicate that following brace should be on same line */
	if (ps.p_stack[ps.tos] != decl) {	/* only put one
						 * declaration onto stack */
	    break_comma = true;	/* while in declaration, newline should be
				 * forced after comma */
	    ps.p_stack[++ps.tos] = decl;
	    ps.il[ps.tos] = ps.i_l_follow;

	    if (ps.ljust_decl) {/* only do if we want left justified
				 * declarations */
		ps.ind_level = 0;
		for (i = ps.tos - 1; i > 0; --i)
		    if (ps.p_stack[i] == decl)
			++ps.ind_level;	/* indentation is number of
					 * declaration levels deep we are */
		ps.i_l_follow = ps.ind_level;
	    }
	}
	break;

    case ifstmt:		/* scanned if (...) */
	if (ps.p_stack[ps.tos] == elsehead && ps.else_if)	/* "else if ..." */
	    ps.i_l_follow = ps.il[ps.tos];
    case dolit:		/* 'do' */
    case forstmt:		/* for (...) */
	ps.p_stack[++ps.tos] = tk;
	ps.il[ps.tos] = ps.ind_level = ps.i_l_follow;
	++ps.i_l_follow;	/* subsequent statements should be
				 * indented 1 */
	ps.search_brace = btype_2;
	break;

    case lbrace:		/* scanned { */
	break_comma = false;	/* don't break comma in an initial list */
	if (ps.p_stack[ps.tos] == stmt || ps.p_stack[ps.tos] == decl
	    || ps.p_stack[ps.tos] == stmtl)
	    ++ps.i_l_follow;	/* it is a random, isolated stmt group or
				 * a declaration */
	else {
	    if (s_code == e_code) {
		/*
		 * only do this if there is nothing on the line 
		 */
		--ps.ind_level;
		/*
		 * it is a group as part of a while, for, etc. 
		 */
		if (ps.p_stack[ps.tos] == swstmt && ps.case_indent >= 1)
		    --ps.ind_level;
		/*
		 * for a switch, brace should be two levels out from the
		 * code 
		 */
	    }
	}

	ps.p_stack[++ps.tos] = lbrace;
	ps.il[ps.tos] = ps.ind_level;
	ps.p_stack[++ps.tos] = stmt;
	/* allow null stmt between braces */
	ps.il[ps.tos] = ps.i_l_follow;
	break;

    case whilestmt:		/* scanned while (...) */
	if (ps.p_stack[ps.tos] == dohead) {
	    /* it is matched with do stmt */
	    ps.ind_level = ps.i_l_follow = ps.il[ps.tos];
	    ps.p_stack[++ps.tos] = whilestmt;
	    ps.il[ps.tos] = ps.ind_level = ps.i_l_follow;
	}
	else {			/* it is a while loop */
	    ps.p_stack[++ps.tos] = whilestmt;
	    ps.il[ps.tos] = ps.i_l_follow;
	    ++ps.i_l_follow;
	    ps.search_brace = btype_2;
	}

	break;

    case elselit:		/* scanned an else */

	if (ps.p_stack[ps.tos] != ifhead)
	    diag(1, "Unmatched 'else'");
	else {
	    ps.ind_level = ps.il[ps.tos];	/* indentation for else
						 * should be same as for
						 * if */
	    ps.i_l_follow = ps.ind_level + 1;	/* everything following
						 * should be in 1 level */
	    ps.p_stack[ps.tos] = elsehead;
	    /* remember if with else */
	    ps.search_brace = btype_2 | ps.else_if;
	}
	break;

    case rbrace:		/* scanned a } */
	/* stack should have <lbrace> <stmt> or <lbrace> <stmtl> */
	if (ps.p_stack[ps.tos - 1] == lbrace) {
	    ps.ind_level = ps.i_l_follow = ps.il[--ps.tos];
	    ps.p_stack[ps.tos] = stmt;
	}
	else
	    diag(1, "Stmt nesting error.");
	break;

    case swstmt:		/* had switch (...) */
	ps.p_stack[++ps.tos] = swstmt;
	ps.cstk[ps.tos] = case_ind;
	/* save current case indent level */
	ps.il[ps.tos] = ps.i_l_follow;
	case_ind = ps.i_l_follow + ps.case_indent;	/* cases should be one
							 * level down from
							 * switch */
	ps.i_l_follow + = ps.case_indent + 1;	/* statements should be
						 * two levels in */
	ps.search_brace = btype_2;
	break;

    case semicolon:		/* this indicates a simple stmt */
	break_comma = false;	/* turn off flag to break after commas in
				 * a declaration */
	ps.p_stack[++ps.tos] = stmt;
	ps.il[ps.tos] = ps.ind_level;
	break;

    default:			/* this is an error */
	diag(1, "Unknown code to parser");
	return;


    }				/* end of switch */

    reduce();			/* see if any reduction can be done */
#ifdef debug
    for (i = 1; i <= ps.tos; ++i)
	printf("(%d %d)", ps.p_stack[i], ps.il[i]);
    printf("\n");
#endif
    return;
}

/*
 * Copyright (C) 1976 by the Board of Trustees of the University of
 * Illinois 
 *
 * All rights reserved 
 *
 *
 * NAME: reduce 
 *
 * FUNCTION: Implements the reduce part of the parsing algorithm 
 *
 * ALGORITHM: The following reductions are done.  Reductions are repeated
 * until no more are possible. 
 *
 * Old TOS		New TOS <stmt> <stmt>	<stmtl> <stmtl> <stmt>	<stmtl> do
 * <stmt>	"dostmt" if <stmt>	"ifstmt" switch <stmt>	<stmt>
 * decl <stmt>	<stmt> "ifelse" <stmt>	<stmt> for <stmt>	<stmt>
 * while <stmt>	<stmt> "dostmt" while	<stmt> 
 *
 * On each reduction, ps.i_l_follow (the indentation for the following line)
 * is set to the indentation level associated with the old TOS. 
 *
 * PARAMETERS: None 
 *
 * RETURNS: Nothing 
 *
 * GLOBALS: ps.cstk ps.i_l_follow = ps.il ps.p_stack = ps.tos = 
 *
 * CALLS: None 
 *
 * CALLED BY: parse  
 *
 * HISTORY: initial coding 	November 1976	D A Willcox of CAC 
 *
 */
   /*----------------------------------------------*\
   |   REDUCTION PHASE				    |
   \*----------------------------------------------*/
reduce()
{

    register int i;

    for (;;) {			/* keep looping until there is nothing
				 * left to reduce */

	switch (ps.p_stack[ps.tos]) {

	case stmt:
	    switch (ps.p_stack[ps.tos - 1]) {

	    case stmt:
	    case stmtl:
		/* stmtl stmt or stmt stmt */
		ps.p_stack[--ps.tos] = stmtl;
		break;

	    case dolit:	/* <do> <stmt> */
		ps.p_stack[--ps.tos] = dohead;
		ps.i_l_follow = ps.il[ps.tos];
		break;

	    case ifstmt:
		/* <if> <stmt> */
		ps.p_stack[--ps.tos] = ifhead;
		for (i = ps.tos - 1;
		     (
		      ps.p_stack[i] != stmt
		      &&
		      ps.p_stack[i] != stmtl
		      &&
		      ps.p_stack[i] != lbrace
		      );
		     --i);
		ps.i_l_follow = ps.il[i];
		/*
		 * for the time being, we will assume that there is no
		 * else on this if, and set the indentation level
		 * accordingly. If an else is scanned, it will be fixed up
		 * later 
		 */
		break;

	    case swstmt:
		/* <switch> <stmt> */
		case_ind = ps.cstk[ps.tos - 1];

	    case decl:		/* finish of a declaration */
	    case elsehead:
		/* <<if> <stmt> else> <stmt> */
	    case forstmt:
		/* <for> <stmt> */
	    case whilestmt:
		/* <while> <stmt> */
		ps.p_stack[--ps.tos] = stmt;
		ps.i_l_follow = ps.il[ps.tos];
		break;

	    default:		/* <anything else> <stmt> */
		return;

	    }			/* end of section for <stmt> on top of
				 * stack */
	    break;

	case whilestmt:	/* while (...) on top */
	    if (ps.p_stack[ps.tos - 1] == dohead) {
		/* it is termination of a do while */
		ps.p_stack[--ps.tos] = stmt;
		break;
	    }
	    else
		return;

	default:		/* anything else on top */
	    return;

	}
    }
}
