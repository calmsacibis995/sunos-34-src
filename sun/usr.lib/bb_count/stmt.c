#ifndef lint
static	char sccsid[] = "@(#)stmt.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include "scanner.h"
#include "stmt.h"

/*
 * statement_list - process a list of statements
 * The left curly brace has already been seen.
 * Terminate the list by the matching right curly brace.
 */
statement_list(infp)
FILE	*infp;
{
	Token	tok;

	tok = gettoken(infp);
	while (tok != RCURLY && tok != EOFT) {
		prevtok(tok);
		one_stmt(infp);
		tok = gettoken(infp);
	}
	prtok();
}


/*
 * control - search for a control structure keyword
 */
static	Control
control(str)
char	*str;
{
	struct	control_words *cw;

	for (cw = control_words; cw->word != NULL; cw++) {
		if (strcmp(str, cw->word) == 0) {
			return(cw->value);
		}
	}
	return(NONE);
}
	
/*
 * body - handle the body of a control structure
 * See if the body is a compound statement or a single statement.
 * We will be adding a counting statement at the beginning of the 
 * body, so if it is a single statement it will need to be enclosed
 * in braces.
 */
static
body(infp)
FILE	*infp;
{
	Token	tok;

	tok = gettoken(infp);
	if (tok == LCURLY) {
		prtok();
		optional_decls(infp);
		basic_block();
		statement_list(infp);
	} else {
		prevtok(tok);
		printf("{");
		basic_block();
		one_stmt(infp);
		printf("}");
	}
}

/*
 * dofor - handle a for statement
 *	for (e1; cond; e3) {		for (e1; counter, cond; e3) {
 *		body		-->		counter;
 *	}					body
 *					}
 * Watch out for the special case of a null condition
 */
static
dofor(infp)
FILE	*infp;
{
	Token	tok;
	
	prtok();
	findtok(SEMI, infp);
	basic_block_comma();
	tok = gettoken(infp);
	if (tok == SEMI) {
		printf("1");
	}
	prevtok(tok);
	matching(LPAREN, RPAREN, infp);
	body(infp);
	basic_block();
}

/*
 * doif - handle an if statement
 * 	if (cond) {			if (cond) {
 *		ifbody				counter
 *	} else {		-->		ifbody
 *		elsebody		} else {
 *	}					counter
 *						elsebody
 *					}
 */
static
doif(infp)
FILE	*infp;
{
	Token	tok;
	Control	cntrl;
	char	*id;

	pr_basic_block(0);
	prtok();
	findtok(LPAREN, infp);
	matching(LPAREN, RPAREN, infp);
	body(infp);

	tok = gettoken(infp);
	id = getident();
	cntrl = control(id);
	if (cntrl == ELSE) {
		prtok();
		body(infp);
	} else {
		prevtok(tok);
	}
	basic_block();
}

/*
 * dowhile - handle an while statement
 * 	while (cond) {			while (counter, cond) {
 *		body		-->		counter
 *	}					body
 *					}
 */
static
dowhile(infp)
FILE	*infp;
{
	prtok();
	findtok(LPAREN, infp);
	basic_block_comma();
	matching(LPAREN, RPAREN, infp);
	body(infp);
	basic_block();
}

/*
 * dodo - handle a do statement
 * After processing the body of the do loop munch up to the next
 * semicolon so that the while portion will not be interperted as
 * a while loop.
 *	do {				do {
 *		body		-->		counter
 *	} while (cond)				body
 *					} while(cond)
 */
static
dodo(infp)
FILE	*infp;
{
	prtok();
	body(infp);
	findtok(LPAREN, infp);
	basic_block_comma();
	findtok(SEMI, infp);
	basic_block();
}
	
/*
 * docase - handle a case statement
 * 	case label:	stmt	-->	case label:	counter
 *							stmt
 * Look for multiple case statements in a row and insert only
 * one counting statement.
 */
static
docase(infp)
FILE	*infp;
{
	Token	tok;

	prtok();
	findtok(COLON, infp);
	basic_block();
}

/*
 * doswitch - handle a switch statement
 * Don't really do anything here other than
 * keep track of the curly braces.
 * Technically a switch statement may be followed by a
 * single statement, but that is rather silly.
 */
static
doswitch(infp)
FILE	*infp;
{
	pr_basic_block(0);
	prtok();
	findtok(LCURLY, infp);
	statement_list(infp);
	basic_block();
}

/*
 * one_stmt - handle a single statement
 * This of course may be control structure
 * In fact, they are the only interesting cases.
 */
static
one_stmt(infp)
FILE	*infp;
{
	Token	tok;
	Control	cntrl;
	char	*id;

	tok = gettoken(infp);
	id = getident();
	cntrl = control(id);
	switch(cntrl) {
	case IF:
		doif(infp);
		break;
		
	case DO:
		dodo(infp);
		break;

	case FOR:
		dofor(infp);
		break;

	case WHILE:
		dowhile(infp);
		break;

	case SWITCH:
		doswitch(infp);
		break;

	case CASE:
	case DEFAULT:
		docase(infp);
		break;

	case NONE:
		donone(infp, tok);
		break;
	}
}

/*
 * donone - we have a statement that is not a control structure.
 * It may be compound statment, a label, or an expression.
 */
static
donone(infp, tok)
FILE	*infp;
Token	tok;
{
	char	savestr[100];

	if (tok == LCURLY) {			/* A compound stmt */ 
		prtok();
		optional_decls(infp);
		statement_list(infp);
	} else if (tok == SEMI) {		/* A null statement */
		prtok();
	} else {
		sprintf(savestr, "%s%s", ws(), getident());
		tok = gettoken(infp);
		if (tok == COLON) { 		/* A label */
			printf("%s", savestr);
			prtok();
			basic_block();
			one_stmt(infp);
		} else {			/* An expression */
			pr_basic_block(0);
			printf("%s", savestr);
			prevtok(tok);
			findtok(SEMI, infp);
		}
	}
}

/*
 * matching - search for a matching token (paren, or curly brace)
 * Watch for nested pairs of the same kind
 */
matching(left, right, infp)
Token	left;
Token	right;
FILE	*infp;
{
	Token	tok;
	int	nestlev;

	nestlev = 1;
	tok = left;
	while (tok != EOFT) {
		tok = gettoken(infp);
		prtok();
		if (tok == left) {
			nestlev++;
		} else if (tok == right) {
			nestlev--;
			if (nestlev <= 0) {
				break;
			}
		}
	}
}
